// license:BSD-3-Clause
// copyright-holders:David Haywood, XingXing, Vas Crabb, Angelo Salese, cam900
/* Fearless Pinocchio, IGS PCB-0433: IGS027A, MX10EXA and ICS2115. */

#include "driver.h"
#include "cpu/arm7/arm7core.h"
#include "cpu/xa/xa.h"
#include "sound/ics2115.h"
#include "machine/ticket.h"
#include <time.h>

static UINT32 *video_ram;
static UINT32 xor_table[256];
static UINT8 gpio_o, irq_enable, irq_pending;
static mame_timer *irq_timer[2];
static UINT8 xa_ports[4], xa_latch0, xa_latch2, xa_irq_state;
static UINT16 xa_command, xa_response[2];
static UINT8 rtc_ram[16], rtc_addr, rtc_data, rtc_count, rtc_mode, rtc_io;
static time_t rtc_base;
static double rtc_epoch;

static void update_irq(void)
{
	cpunum_set_input_line(0, ARM7_IRQ_LINE, irq_pending != 0xff ? ASSERT_LINE : CLEAR_LINE);
}

static void peripheral_irq(int index)
{
	if (!(irq_enable & (1 << index)))
	{
		irq_pending &= ~(1 << index);
		update_irq();
	}
}

static READ32_HANDLER( irq_pending_r )
{
	UINT32 value = irq_pending;
	irq_pending = 0xff;
	update_irq();
	return value;
}

static WRITE32_HANDLER( irq_enable_w )
{
	if (ACCESSING_LSB32) irq_enable = data;
}

static WRITE32_HANDLER( timer_rate_w )
{
	if (ACCESSING_LSB32)
	{
		double period = data & 0xff ? 4263.0 * ((data & 0xff) + 1) / 25000000.0 : TIME_NEVER;
		timer_adjust(irq_timer[offset], period, offset, period);
	}
}

static READ32_HANDLER( gpio_r )
{
	UINT32 dsw = 0xff;
	if (!(gpio_o & 1)) dsw &= readinputport(2);
	if (!(gpio_o & 2)) dsw &= readinputport(3);
	return 0xff800007 | ((0x400 | dsw) << 3);
}

static WRITE32_HANDLER( gpio_w ) { if (ACCESSING_LSB32) gpio_o = data & 0x1f; }
static WRITE32_HANDLER( xor_table_w )
{
	if (ACCESSING_LSB32)
	{
		UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);
		UINT32 value = ((data & 255) << 24) | ((data & 255) << 8);
		UINT32 delta = value ^ xor_table[offset];
		int i;
		/* Opcode fetches use direct ROM pointers, so data and instructions must change together. */
		for (i = offset; i < 0x80000/4; i += 256) rom[i] ^= delta;
		xor_table[offset] = value;
	}
}

static void xa_main_irq(int state)
{
	/* The ARM interrupt controller latches rising edges until its status is read. */
	if (state && !xa_irq_state && !(irq_enable & 8))
	{
		irq_pending &= ~8;
		update_irq();
	}
	xa_irq_state = state;
}

static UINT8 xa_port_r(int port)
{
	if (port == 0) return xa_latch0;
	if (port == 2) return xa_latch2;
	return xa_ports[port];
}

static void xa_port_w(int port, UINT8 data)
{
	UINT8 falling = xa_ports[port] & ~data;
	xa_ports[port] = data;
	if (port == 1) xa_main_irq((data >> 3) & 1);
	if (port != 3) return;
	if (falling & 0x80)
	{
		if (!(data & 0x10)) xa_latch0 = ics2115_r(xa_ports[1] & 7);
		else if (!(data & 0x20))
		{
			xa_latch2 = xa_command >> 8;
			xa_latch0 = xa_command;
		}
	}
	if (falling & 0x40)
	{
		if (!(data & 0x10)) ics2115_w(xa_ports[1] & 7, xa_ports[0]);
		else if (!(data & 0x20))
		{
			UINT16 value = (xa_ports[2] << 8) | xa_ports[0];
			if ((xa_ports[1] & 7) == 1) xa_response[1] = value;
			if ((xa_ports[1] & 7) == 2) xa_response[0] = value;
		}
	}
}

static struct xa_config xa_interface = { xa_port_r, xa_port_w };
static void ics_irq(int state) { cpunum_set_input_line(1, 2, state ? ASSERT_LINE : CLEAR_LINE); }
static struct ics2115_interface ics_interface = { REGION_SOUND1, ics_irq };

static void sound_command_callback(int data)
{
	xa_command = data;
	cpunum_set_input_line(1, 0, PULSE_LINE);
}

static WRITE32_HANDLER( sound_command_w )
{
	if ((mem_mask & 0xffff) != 0xffff) timer_set(TIME_NOW, data & 0xffff, sound_command_callback);
}

static READ32_HANDLER( sound_response_r )
{
	if (offset == 0) return xa_response[0];
	if (offset == 0x80/4) return (UINT32)xa_response[1] << 16;
	return 0xffffffff;
}

static WRITE32_HANDLER( cpld_w )
{
	if (offset == 2 && ACCESSING_LSB32)
	{
		coin_counter_w(0, data & 4);
		coin_counter_w(1, data & 8);
		ticket_dispenser_w(0, data & 0x80);
	}
}

static READ32_HANDLER( input0_r ) { return readinputport(0); }
static READ32_HANDLER( input1_r )
{
	return (readinputport(1) & ~8) | ((ticket_dispenser_r(0) & 0x80) ? 0 : 8);
}

static WRITE32_HANDLER( palette_w )
{
	int half;
	COMBINE_DATA(&paletteram32[offset]);
	for (half = 0; half < 2; ++half)
	{
		UINT16 value = paletteram32[offset] >> (half * 16);
		palette_set_color(offset * 2 + half, pal5bit(value), pal5bit(value >> 5), pal5bit(value >> 10));
	}
}

static UINT8 bcd(int value) { return (value / 10) * 16 + value % 10; }
static int unbcd(UINT8 value) { return (value >> 4) * 10 + (value & 15); }

static void rtc_clock_to_ram(void)
{
	time_t now = rtc_base + (time_t)(timer_get_time() - rtc_epoch);
	struct tm *t = localtime(&now);
	rtc_ram[1] = 0xff;
	rtc_ram[2] = bcd(t->tm_sec);
	rtc_ram[3] = bcd(t->tm_min);
	rtc_ram[4] = bcd(t->tm_hour);
	rtc_ram[5] = bcd(t->tm_mday);
	rtc_ram[6] = bcd(t->tm_mon + 1);
	rtc_ram[7] = bcd(t->tm_year % 100);
	rtc_ram[8] = bcd(t->tm_wday + 1);
	rtc_ram[9] = bcd(t->tm_yday / 7);
}

static void rtc_clock(void)
{
	if (!rtc_mode)
	{
		rtc_addr = ((rtc_addr >> 1) | (rtc_io << 3)) & 15;
		if (++rtc_count == 4)
		{
			rtc_count = 0;
			if (rtc_addr == 15) rtc_clock_to_ram();
			else if (rtc_addr == 14)
			{
				if (!(rtc_ram[0] & 16))
				{
					struct tm t;
					memset(&t, 0, sizeof(t));
					t.tm_year = 100 + unbcd(rtc_ram[7]);
					t.tm_mon = unbcd(rtc_ram[6]) - 1;
					t.tm_mday = unbcd(rtc_ram[5]);
					t.tm_hour = unbcd(rtc_ram[4]);
					t.tm_min = unbcd(rtc_ram[3]);
					t.tm_sec = unbcd(rtc_ram[2]);
					t.tm_isdst = -1;
					rtc_base = mktime(&t);
					rtc_epoch = timer_get_time();
				}
			}
			else { rtc_data = rtc_ram[rtc_addr]; rtc_mode = 1; }
		}
	}
	else
	{
		rtc_data = (rtc_data >> 1) | (rtc_io << 7);
		if (++rtc_count == 8)
		{
			if (rtc_addr != 1 && rtc_addr <= 9) rtc_ram[rtc_addr] = rtc_data;
			rtc_mode = rtc_count = 0;
		}
	}
}
static READ32_HANDLER( rtc_r ) { UINT32 value = rtc_data & 1; rtc_clock(); return value; }
static WRITE32_HANDLER( rtc_w ) { if (ACCESSING_LSB32) { rtc_io = data & 1; rtc_clock(); } }

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x00003fff) AM_ROM
	AM_RANGE(0x08000000, 0x0807ffff) AM_ROM AM_REGION(REGION_USER1, 0)
	AM_RANGE(0x10000000, 0x100003ff) AM_RAM
	AM_RANGE(0x18000000, 0x1800ffff) AM_RAM AM_BASE(&generic_nvram32) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x28000000, 0x28000003) AM_READWRITE(rtc_r, rtc_w)
	AM_RANGE(0x38000000, 0x38001fff) AM_RAM AM_BASE(&video_ram)
	AM_RANGE(0x38004000, 0x38007fff) AM_RAM AM_WRITE(palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x38008500, 0x380085ff) AM_READ(sound_response_r)
	AM_RANGE(0x38008500, 0x38008503) AM_WRITE(sound_command_w)
	AM_RANGE(0x4000000c, 0x4000000f) AM_READ(gpio_r)
	AM_RANGE(0x40000014, 0x40000017) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x40000018, 0x4000001b) AM_WRITE(gpio_w)
	AM_RANGE(0x50000000, 0x500003ff) AM_WRITE(xor_table_w)
	AM_RANGE(0x58000000, 0x58000003) AM_READ(input0_r)
	AM_RANGE(0x58100000, 0x58100003) AM_READ(input1_r)
	AM_RANGE(0x68000000, 0x6800000f) AM_WRITE(cpld_w)
	AM_RANGE(0x70000100, 0x70000107) AM_WRITE(timer_rate_w)
	AM_RANGE(0x70000200, 0x70000203) AM_READWRITE(irq_pending_r, irq_enable_w)
	AM_RANGE(0xf0000008, 0xf000000b) AM_WRITE(MWA32_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( xa_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( fear )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE2 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(5)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(5)
	PORT_SERVICE_NO_TOGGLE( 0x0020, IP_ACTIVE_LOW )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:1")
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:2")
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:3")
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:4")
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:5")
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:6")
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW1:7")
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:1")
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:2")
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:4")
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:5")
	PORT_DIPSETTING( 0x10, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:6")
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING( 0x40, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Unknown ) ) PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING( 0x80, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END


static VIDEO_UPDATE( igs_fear )
{
	const UINT8 *gfx = memory_region(REGION_GFX1);
	int i;
	fillbitmap(bitmap, Machine->pens[0x3ff], cliprect);
	for (i = 0; i < 0x800; i += 4)
	{
		int x = (INT16)video_ram[i], y = (INT16)(video_ram[i] >> 16);
		int height = (video_ram[i + 1] & 0x3ff) + 1;
		int width = (((video_ram[i + 1] >> 16) & 0x3ff) + 1) * 4;
		int flip = !(video_ram[i + 2] & 0x100);
		int palette = (video_ram[i + 2] & 63) << 7;
		UINT32 address = (video_ram[i + 2] >> 16) | (video_ram[i + 3] << 16);
		int sx, sy;
		if (!address || address == 0xffffffff) continue;
		address <<= 2;
		/* Clip before computing a destination pointer; sprites can start off-screen. */
		for (sy = MAX(y, cliprect->min_y); sy <= MIN(y + height - 1, cliprect->max_y); ++sy)
		{
			UINT16 *dest = bitmap->line[sy];
			for (sx = MAX(x, cliprect->min_x); sx <= MIN(x + width - 1, cliprect->max_x); ++sx)
			{
				int column = flip ? width - 1 - (sx - x) : sx - x;
				UINT8 pixel = gfx[(address + (sy - y) * width + column) & 0x3ffffff];
				if (pixel) dest[sx] = Machine->pens[pixel | palette];
			}
		}
	}
}

static INTERRUPT_GEN( vblank )
{
	if (cpu_getcurrentframe() & 1)
	{
		cpunum_set_input_line(0, ARM7_FIRQ_LINE, ASSERT_LINE);
		cpunum_set_input_line(0, ARM7_FIRQ_LINE, CLEAR_LINE);
	}
}

static MACHINE_RESET( igs_fear )
{
	gpio_o = 0;
	irq_enable = irq_pending = 0xff;
	{
		UINT32 *rom = (UINT32 *)memory_region(REGION_USER1);
		int i;
		/* A soft reset must restore the ROM before the internal firmware uploads a new key. */
		for (i = 0; i < 0x80000/4; ++i) rom[i] ^= xor_table[i & 255];
		memset(xor_table, 0, sizeof(xor_table));
	}
	memset(xa_ports, 0, sizeof(xa_ports));
	memset(xa_response, 0, sizeof(xa_response));
	xa_command = xa_latch0 = xa_latch2 = xa_irq_state = 0;
	rtc_mode = rtc_count = rtc_addr = rtc_data = rtc_io = 0;
	memset(rtc_ram, 0, sizeof(rtc_ram));
	rtc_base = time(0);
	rtc_epoch = timer_get_time();
	ticket_dispenser_init(200, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
	update_irq();
}

static DRIVER_INIT( fearless )
{
	UINT16 *src = (UINT16 *)memory_region(REGION_USER1);
	int i;
	for (i = 0; i < 0x80000/2; ++i)
	{
		UINT16 x = 0;
		if ((i & 0x040480) != 0x000080) x ^= 0x0001;
		if ((i & 0x000030) == 0x000010) x ^= 0x0004;
		if ((i & 0x000242) != 0x000042) x ^= 0x0008;
		if ((i & 0x002004) != 0x000004) x ^= 0x0020;
		if ((i & 0x011800) != 0x010000) x ^= 0x0040;
		if ((i & 0x004820) == 0x004820) x ^= 0x0080;
		src[i] ^= x;
	}
	irq_timer[0] = timer_alloc(peripheral_irq);
	irq_timer[1] = timer_alloc(peripheral_irq);
}

static MACHINE_DRIVER_START( igs_fear )
	MDRV_CPU_ADD(ARM7, 25000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(vblank, 1)
	MDRV_CPU_ADD(XA, 50000000/3)
	MDRV_CPU_CONFIG(xa_interface)
	MDRV_CPU_PROGRAM_MAP(xa_map, 0)
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_RESET(igs_fear)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 639, 0, 479)
	MDRV_PALETTE_LENGTH(0x2000)
	MDRV_VIDEO_UPDATE(igs_fear)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(ICS2115, 33868800)
	MDRV_SOUND_CONFIG(ics_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 5.0)
MACHINE_DRIVER_END

ROM_START( fearless )
	ROM_REGION( 0x04000, REGION_CPU1, 0 )
	ROM_LOAD( "fearless_igs027a.bin", 0x00000, 0x4000, CRC(2121f01b) SHA1(de44016b590fdcf6bfb63e3f8dbbbd679938fe87) )

	ROM_REGION32_LE( 0x80000, REGION_USER1, 0 )
	ROM_LOAD( "fearlessp_v-101us.u37", 0x000000, 0x80000, CRC(2522873c) SHA1(8db709877311b6d2796353fc9a44a820937e35c2) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "o7.u33", 0x000000, 0x10000, CRC(7dae4900) SHA1(bbf7ba7c9e95ff2ffeb1dc0fc7ccedd4da274d01) )

	ROM_REGION( 0x4000000, REGION_GFX1, ROMREGION_ERASEFF )
	ROM_LOAD32_WORD( "fearlessp_u7_cg-0l.u7",   0x0000000, 0x800000, CRC(ca254db4) SHA1(f5670c2ff0720c84c9aff3cea95b118b6044e469) )
	ROM_LOAD32_WORD( "fearlessp_u6_cg-0h.u6",   0x0000002, 0x800000, CRC(02d8bbbf) SHA1(7cf36c909a5d76096a725ffe0a697bcbafbcf985) )
	ROM_LOAD32_WORD( "fearlessp_u14_cg-1l.u14", 0x1000000, 0x800000, CRC(7fe312d2) SHA1(c0add22d9fc4c0e32a03922cb709b947bfff429d) )
	ROM_LOAD32_WORD( "fearlessp_u13_cg-1h.u13", 0x1000002, 0x800000, CRC(c9d2a56d) SHA1(91d1665623bca743f68f15a27bbf433e2ffc0808) )
	ROM_LOAD32_WORD( "fearlessp_u18_cg-2l.u18", 0x2000000, 0x800000, CRC(07623d66) SHA1(041d5e44917bc16caa720ea98bdc0a4f5fb4b8e0) )
	ROM_LOAD32_WORD( "fearlessp_u17_cg-2h.u17", 0x2000002, 0x800000, CRC(756fe1f2) SHA1(48ee81c5fa4808406b57b2521b836db3ff5a7fa9) )

	ROM_REGION( 0x800000, REGION_SOUND1, 0 )
	ROM_LOAD( "fearlessp_u25_music0.u25", 0x000000, 0x400000, CRC(a015b9b1) SHA1(7b129c59acd523dec82e58a75d873bbc5341fb28) )
	ROM_LOAD( "fearlessp_u26_music1.u26", 0x400000, 0x400000, CRC(9d5f18da) SHA1(42e5224c1af0898cc2e02b2e051ea8b629d5fb6d) )
ROM_END


GAME( 2006, fearless, 0, igs_fear, fear, fearless, ROT0, "IGS (American Alpha license)", "Fearless Pinocchio (V101US)", 0 )
