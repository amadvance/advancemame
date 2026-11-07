// license:BSD-3-Clause
// copyright-holders:R. Belmont, MetalliC
/* Konami PWB 452574A medal hardware: Z80, K054156/157, K053252, SCC and OKI6295. */

#include "driver.h"
#include "vidhrdw/konamiic.h"
#include "sound/k051649.h"
#include "sound/okim6295.h"
#include "machine/ticket.h"

static UINT8 control, control2, scc_enable;
static UINT8 ccu_regs[16];
static int ccu_time, ccu_count, avac_state;
static mame_timer *scan_timer;

/* The Z80 presents the register pairs low byte first. */
static WRITE8_HANDLER( video_regs_w ) { K056832_w(offset ^ 1, data); }
static WRITE8_HANDLER( video_b_regs_w ) { K056832_b_w(offset ^ 1, data); }

static READ8_HANDLER( video_r )
{
	if (!(control2 & 0x80))
		return (offset & 1) ? K056832_ram_code_hi_r(offset >> 1) : K056832_ram_code_lo_r(offset >> 1);
	if (!control)
	{
		UINT32 r1a = K056832_read_register(0x1a), r1b = K056832_read_register(0x1b);
		UINT32 address = ((r1a << 9) & 0x60000) | ((r1b << 15) & 0x18000) | ((r1a << 3) & 0x6000);
		return memory_region(REGION_GFX1)[address + offset];
	}
	return 0;
}

static WRITE8_HANDLER( video_w )
{
	if (offset & 1) K056832_ram_code_hi_w(offset >> 1, data);
	else K056832_ram_code_lo_w(offset >> 1, data);
}

static WRITE8_HANDLER( bank_w )
{
	memory_set_bankptr(1, memory_region(REGION_CPU1) + (data >> 4) * 0x2000);
	control = data & 15;
}

static WRITE8_HANDLER( control2_w )
{
	control2 = data;
	coin_counter_w(0, data & 4);
	coin_counter_w(1, data & 8);
	coin_lockout_w(0, !(data & 0x10));
	coin_lockout_w(1, !(data & 0x20));
	ticket_dispenser_w(0, (data & 0x40) << 1);
}
static WRITE8_HANDLER( medal_w )
{
	coin_counter_w(2, data & 2);
	coin_lockout_w(2, !(data & 4));
}
static WRITE8_HANDLER( lamps_w )
{
	int i;
	for (i = 0; i < 8; ++i) set_led_status(i, (data >> i) & 1);
}
static READ8_HANDLER( inputs_r )
{
	return (readinputport(3) & ~0x10) | ((ticket_dispenser_r(0) & 0x80) ? 0 : 0x10);
}

static READ8_HANDLER( scc_r )
{
	if (!scc_enable) return 0xff;
	offset &= 0xff;
	if (offset < 0x80) return K051649_waveform_r(offset);
	return 0xff;
}
static WRITE8_HANDLER( scc_w )
{
	if (!scc_enable) return;
	offset &= 0xff;
	if (offset < 0x80) K051649_waveform_w(offset, data);
	else
	{
		offset &= ~0x10;
		if (offset < 0x8a) K051649_frequency_w(offset - 0x80, data);
		else if (offset < 0x8f) K051649_volume_w(offset - 0x8a, data);
		else if (offset == 0x8f) K051649_keyonoff_w(0, data);
	}
}
static WRITE8_HANDLER( scc_enable_w ) { scc_enable = (data & 0x3f) == 0x3f; }

static void ccu_visible_area(void)
{
	int hc = (((ccu_regs[0] << 8) | ccu_regs[1]) & 0x3ff) + 1;
	int vc = (((ccu_regs[8] << 8) | ccu_regs[9]) & 0x1ff) + 1;
	int hfp = ((ccu_regs[2] << 8) | ccu_regs[3]) & 0x1ff;
	int hbp = ((ccu_regs[4] << 8) | ccu_regs[5]) & 0x1ff;
	int vfp = ccu_regs[10], vbp = ccu_regs[11] + 1;
	int hsw = (ccu_regs[12] & 15) + 1, vsw = (ccu_regs[12] >> 4) + 1;
	int width = hc - hfp - hbp - 8 * hsw;
	int height = vc - vfp - vbp - vsw;
	/* Registers arrive separately, so publish only complete, valid display timings. */
	if (hfp && hbp && vfp && width > 0 && height > 0 && width <= 480 && height <= 240)
		set_visible_area(32, 32 + width - 1, 16, 16 + height - 1);
}
static READ8_HANDLER( ccu_r )
{
	int vc = (((ccu_regs[8] << 8) | ccu_regs[9]) & 0x1ff) + 1;
	if (offset == 14) return ((cpu_getscanline() - vc) >> 8) & 1;
	if (offset == 15) return (cpu_getscanline() - vc) & 255;
	return ccu_regs[offset];
}
static WRITE8_HANDLER( ccu_w )
{
	ccu_regs[offset] = data;
	switch (offset)
	{
	case 13: ccu_time = data; break;
	case 14: cpunum_set_input_line(0, 0, CLEAR_LINE); break;
	case 15: cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE); break;
	default: if (offset < 13) ccu_visible_area(); break;
	}
}
static void scanline(int line)
{
	/* Both interrupt inputs are latched until the game writes their CCU acknowledgements. */
	if (line == 255) cpunum_set_input_line(0, 0, ASSERT_LINE);
	if (--ccu_count <= 0)
	{
		cpunum_set_input_line(0, INPUT_LINE_NMI, ASSERT_LINE);
		ccu_count = ccu_time;
	}
	line = (line + 1) & 255;
	timer_adjust(scan_timer, cpu_getscanlinetime(line), line, 0);
}

static ADDRESS_MAP_START( ddboy_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x9fff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xa000, 0xbfff) AM_RAM AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc000, 0xc03f) AM_WRITE(video_regs_w)
	AM_RANGE(0xc100, 0xc100) AM_WRITE(control2_w)
	AM_RANGE(0xc200, 0xc200) AM_WRITE(medal_w)
	AM_RANGE(0xc300, 0xc300) AM_WRITE(lamps_w)
	AM_RANGE(0xc400, 0xc400) AM_WRITE(bank_w)
	AM_RANGE(0xc500, 0xc500) AM_READ(MRA8_NOP)
	AM_RANGE(0xc600, 0xc607) AM_WRITE(video_b_regs_w)
	/* Only four secondary words exist; upper-window writes must not overrun them. */
	AM_RANGE(0xc608, 0xc60f) AM_WRITE(MWA8_NOP)
	AM_RANGE(0xc700, 0xc700) AM_READ(input_port_0_r)
	AM_RANGE(0xc701, 0xc701) AM_READ(input_port_1_r)
	AM_RANGE(0xc702, 0xc702) AM_READ(input_port_2_r)
	AM_RANGE(0xc703, 0xc703) AM_READ(inputs_r)
	AM_RANGE(0xc800, 0xc80f) AM_READWRITE(ccu_r, ccu_w)
	AM_RANGE(0xcc00, 0xcc00) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xd000, 0xd7ff) AM_WRITE(scc_enable_w)
	AM_RANGE(0xd800, 0xdfff) AM_READWRITE(scc_r, scc_w)
	AM_RANGE(0xe000, 0xffff) AM_READWRITE(video_r, video_w)
ADDRESS_MAP_END

INPUT_PORTS_START( ddboy )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x07, 0x07, "Coin Slot 1" )   PORT_DIPLOCATION("SW1:1,2,3")
	PORT_DIPSETTING(    0x03, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x00, "5 Coins/2 Credits" )
	PORT_DIPSETTING(    0x06, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x07, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 2C_3C ) )
	PORT_DIPNAME( 0x78, 0x00, "Coin Slot 2" )   PORT_DIPLOCATION("SW1:4,5,6,7")

	PORT_DIPSETTING(    0x70, "2 Medals" )
	PORT_DIPSETTING(    0x68, "3 Medals" )
	PORT_DIPSETTING(    0x60, "4 Medals" )
	PORT_DIPSETTING(    0x58, "5 Medals" )
	PORT_DIPSETTING(    0x50, "6 Medals" )
	PORT_DIPSETTING(    0x48, "7 Medals" )
	PORT_DIPSETTING(    0x40, "8 Medals" )
	PORT_DIPSETTING(    0x38, "9 Medals" )
	PORT_DIPSETTING(    0x30, "10 Medals" )
	PORT_DIPSETTING(    0x28, "11 Medals" )
	PORT_DIPSETTING(    0x20, "12 Medals" )
	PORT_DIPSETTING(    0x18, "13 Medals" )
	PORT_DIPSETTING(    0x10, "14 Medals" )
	PORT_DIPSETTING(    0x08, "15 Medals" )
	PORT_DIPSETTING(    0x00, "16 Medals" )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )   PORT_DIPLOCATION("SW1:8")
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x0f, 0x00, "Standard of Payout" ) PORT_DIPLOCATION("SW2:1,2,3,4")
	PORT_DIPSETTING(    0x0f, "15%" )
	PORT_DIPSETTING(    0x0e, "20%" )
	PORT_DIPSETTING(    0x0d, "25%" )
	PORT_DIPSETTING(    0x0c, "30%" )
	PORT_DIPSETTING(    0x0b, "35%" )
	PORT_DIPSETTING(    0x0a, "40%" )
	PORT_DIPSETTING(    0x09, "45%" )
	PORT_DIPSETTING(    0x08, "50%" )
	PORT_DIPSETTING(    0x07, "55%" )
	PORT_DIPSETTING(    0x06, "60%" )
	PORT_DIPSETTING(    0x05, "65%" )
	PORT_DIPSETTING(    0x04, "70%" )
	PORT_DIPSETTING(    0x03, "75%" )
	PORT_DIPSETTING(    0x02, "80%" )
	PORT_DIPSETTING(    0x01, "85%" )
	PORT_DIPSETTING(    0x00, "90%" )
	PORT_DIPNAME( 0x30, 0x00, "Play Timer" )         PORT_DIPLOCATION("SW2:5,6")
	PORT_DIPSETTING(    0x30, "10 sec" )
	PORT_DIPSETTING(    0x20, "12 sec" )
	PORT_DIPSETTING(    0x10, "15 sec" )
	PORT_DIPSETTING(    0x00, "18 sec" )
	PORT_DIPNAME( 0x40, 0x40, "Backup Memory" )      PORT_DIPLOCATION("SW2:7")
	PORT_DIPSETTING(    0x00, "Clear" )
	PORT_DIPSETTING(    0x40, "Keep" )
	PORT_DIPNAME( 0x80, 0x00, "Demo Sound" )         PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0e, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN3 ) PORT_NAME("Medal")
	PORT_BIT( 0xd0, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_SERVICE_NO_TOGGLE( 0x02, IP_ACTIVE_LOW )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_SPECIAL )
	PORT_BIT( 0xe0, IP_ACTIVE_LOW, IPT_UNKNOWN )

INPUT_PORTS_END

static void tile_callback(int layer, int *code, int *color)
{
	int raw = *code, mode, avac;
	K056832_read_AVAC(&mode, &avac);
	if (mode) *code = (((avac >> ((raw >> 8) & 12)) & 15) << 10) | (raw & 0x3ff);
	else *code = raw & 0xfff;
	*code = BITSWAP16(*code,15,14,8,9,13,12,11,10,7,6,5,4,3,2,1,0);
	*color = ((layer & 1) ? 8 : 0) + ((raw >> 13) & 7);
	tile_info.priority = (raw >> 12) & 1;
}
static VIDEO_START( konmedal )
{
	return K056832_vh_start(REGION_GFX1, K056832_BPP_4, 1, 0, tile_callback, 0);
}
static VIDEO_UPDATE( konmedal )
{
	int mode, data, pass, layer;
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	fillbitmap(priority_bitmap, 0, cliprect);
	K056832_read_AVAC(&mode, &data);
	data |= mode << 3;
	if (data != avac_state) { avac_state = data; K056832_MarkAllTilemapsDirty(); }
	for (pass = 0; pass < 2; ++pass)
		for (layer = 3; layer >= 0; --layer)
			K056832_tilemap_draw(bitmap, cliprect, layer, pass, 0);
}
static PALETTE_INIT( konmedal )
{
	int i;
	for (i = 0; i < 256; ++i)
		palette_set_color(i, color_prom[i] << 4, color_prom[0x100 + i] << 4, color_prom[0x200 + i] << 4);
}
static NVRAM_HANDLER( konmedal )
{
	if (read_or_write) mame_fwrite(file, generic_nvram, generic_nvram_size);
	else if (file) mame_fread(file, generic_nvram, generic_nvram_size);
	else
	{
		memset(generic_nvram, 0, generic_nvram_size);
		/* Factory signatures let a fresh board pass its battery-backed RAM check. */
		generic_nvram[0x303] = 0xff;
		generic_nvram[0x309] = 0xaa;
	}
}
static MACHINE_RESET( konmedal )
{
	control = control2 = scc_enable = 0;
	ccu_count = 0;
	ccu_time = 31;
	avac_state = 0;
	memset(ccu_regs, 0, sizeof(ccu_regs));
	memory_set_bankptr(1, memory_region(REGION_CPU1) + 0x8000);
	ticket_dispenser_init(100, TICKET_MOTOR_ACTIVE_HIGH, TICKET_STATUS_ACTIVE_HIGH);
	cpunum_set_input_line(0, 0, CLEAR_LINE);
	cpunum_set_input_line(0, INPUT_LINE_NMI, CLEAR_LINE);
	timer_adjust(scan_timer, cpu_getscanlinetime(0), 0, 0);
}
static DRIVER_INIT( ddboy ) { scan_timer = timer_alloc(scanline); }

static MACHINE_DRIVER_START( ddboy )
	MDRV_CPU_ADD(Z80, 14318181/2)
	MDRV_CPU_PROGRAM_MAP(ddboy_map, 0)
	MDRV_MACHINE_RESET(konmedal)
	MDRV_NVRAM_HANDLER(konmedal)
	MDRV_FRAMES_PER_SECOND(59.62)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 256)
	MDRV_VISIBLE_AREA(80, 399, 16, 239)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(konmedal)
	MDRV_VIDEO_START(konmedal)
	MDRV_VIDEO_UPDATE(konmedal)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 14318181/14/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(K051649, 14318181/4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.225)
MACHINE_DRIVER_END

ROM_START( ddboy )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )
	ROM_LOAD( "342_c02.27c010.4d", 0x000000, 0x020000, CRC(dc33af9f) SHA1(db22f3b28e3aba69f70fd2581c77755373b582d0) )

	ROM_REGION( 0x80000, REGION_GFX1, 0 )
	ROM_LOAD32_BYTE( "342_a03.27c010.4f", 0x000002, 0x020000, CRC(424f80dd) SHA1(fb7648960ce0951aebcf5cf4465a9acb3ab49cd8) )
	ROM_LOAD32_BYTE( "342_a04.27c010.4g", 0x000003, 0x020000, CRC(a4d4e15e) SHA1(809afab3f2adc58ca5d18e2413b40a6f33bd0cfa) )
	ROM_LOAD32_BYTE( "342_a05.27c010.4h", 0x000000, 0x020000, CRC(e7e50901) SHA1(5e01377a3ad8ccb2a2b56610e8225b9b6bf15122) )
	ROM_LOAD32_BYTE( "342_a06.27c010.4j", 0x000001, 0x020000, CRC(49f35d66) SHA1(3d5cf3b6eb6a3497609117acd002169a31130418) )

	ROM_REGION( 0x100000, REGION_SOUND1, 0 )
	ROM_LOAD( "342_a01.27c010.8b", 0x000000, 0x020000, CRC(e9ce569c) SHA1(ce9b3e60eac3543aca9e82a9ccf77c53a6aff504) )

	ROM_REGION( 0x400, REGION_PROMS, 0 )
	ROM_LOAD( "342_a07.82s129.13f", 0x000000, 0x000100, CRC(f8c11f4d) SHA1(95061d0af7c8bac702aa48e16c0711719250653f) )
	ROM_LOAD( "342_a08.82s129.14f", 0x000100, 0x000100, CRC(1814db4b) SHA1(08b25f96dc3af15b3fa3c88b2884845abd3ff620) )
	ROM_LOAD( "342_a09.82s129.15f", 0x000200, 0x000100, CRC(21e2dd13) SHA1(721c7fa1a01c810a7ce35b4331d280704b4e04fd) )
	ROM_LOAD( "342_a10.82s129.14g", 0x000300, 0x000100, CRC(1fa443f9) SHA1(84b0a36a4e49bf75bda1871bf52090ee5a75cd03) )
ROM_END


GAME( 1994, ddboy, 0, ddboy, ddboy, ddboy, ROT0, "Konami", "Dam Dam Boy (on dedicated PCB)", GAME_IMPERFECT_GRAPHICS )
