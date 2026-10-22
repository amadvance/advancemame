/*
 * Archimedes arcade hardware: MEMC paging, IOC interrupts and VIDC DMA.
 * Driver authors: Tomasz Slanina, Steve Ellenoff and Nicola Salmoria.
 * Chip emulation contributors: Angelo Salese and R. Belmont.
 */
#include "driver.h"
#include "cpu/arm/arm.h"
#include "sound/dac.h"

static UINT32 *physical_ram;
static INT16 memc_pages[0x2000];
static UINT32 memc_page_size;
static int boot_rom;
static UINT8 ioc[32];
static UINT32 vidc[256];
static UINT32 video_start, video_end, video_pointer;
static UINT32 sound_start, sound_end, sound_current, sound_limit;
static int video_enabled, sound_enabled;
static UINT32 timer_count[4], timer_output[4];
static mame_timer *ioc_timer[4], *sound_timer;
static const int page_sizes[4] = { 4096, 8192, 16384, 32768 };

static void update_interrupts(void)
{
	int irq = (ioc[4] & ioc[6]) | (ioc[8] & ioc[10]);
	cpunum_set_input_line(0, ARM_IRQ_LINE, irq ? ASSERT_LINE : CLEAR_LINE);
	cpunum_set_input_line(0, ARM_FIRQ_LINE, (ioc[12] & ioc[14]) ? ASSERT_LINE : CLEAR_LINE);
}

static UINT8 physical_byte(UINT32 address)
{
	UINT32 value = physical_ram[(address & 0xffffff) >> 2];
	return value >> ((address & 3) * 8);
}

static READ32_HANDLER( logical_r )
{
	UINT32 address = offset * 4;
	UINT32 size = page_sizes[memc_page_size];
	int page = memc_pages[address / size];
	if (boot_rom)
		return ((UINT32 *)memory_region(REGION_USER1))[offset & 0x1fffff];
	if (page < 0)
		return 0xffffffff;
	return physical_ram[(page * size + address % size) >> 2];
}

static WRITE32_HANDLER( logical_w )
{
	UINT32 address = offset * 4;
	UINT32 size = page_sizes[memc_page_size];
	int page = memc_pages[address / size];
	if (!boot_rom && page >= 0)
		COMBINE_DATA(&physical_ram[(page * size + address % size) >> 2]);
}

static WRITE32_HANDLER( memc_page_w )
{
	UINT32 logical = 0, physical = 0, bank = 0;
	int i;
	switch (memc_page_size) {
	case 0:
		physical = data & 0x7f;
		logical = ((data & 0x7ff000) >> 12) | ((data & 0xc00) << 1);
		bank = (data >> 7) & 1;
		break;
	case 1:
		physical = ((data & 0x7e) >> 1) | ((data & 1) << 6);
		logical = ((data & 0x7fe000) >> 13) | (data & 0xc00);
		bank = ((data >> 7) & 1) | ((data >> 11) & 2);
		break;
	case 2:
		physical = ((data & 0x7c) >> 2) | ((data & 3) << 5);
		logical = ((data & 0x7fc000) >> 14) | ((data & 0xc00) >> 1);
		bank = ((data >> 7) & 1) | ((data >> 11) & 2);
		break;
	case 3:
		physical = ((data & 0x78) >> 3) | ((data & 1) << 4) | ((data & 2) << 5) | ((data & 4) << 3);
		logical = ((data & 0x7f8000) >> 15) | ((data & 0xc00) >> 2);
		bank = ((data >> 7) & 1) | ((data >> 11) & 2);
		break;
	}
	physical += bank * 0x80;
	boot_rom = 0;
	for (i = 0; i < 0x2000; i++)
		if (memc_pages[i] == physical) memc_pages[i] = -1;
	memc_pages[logical] = physical;
}

static void sound_tick(int param)
{
	int ch;
	for (ch = 0; ch < 8; ch++) {
		UINT8 raw = physical_byte(sound_current + ch);
		int value = (((16 + ((raw >> 1) & 15)) << (raw >> 5)) - 16) * 8;
		if (raw & 1) value = -value;
		DAC_signed_data_16_w(ch, value + 0x8000);
	}
	sound_current += 8;
	if (sound_current >= sound_limit) {
		ioc[8] |= 2;
		update_interrupts();
		if (sound_enabled) {
			sound_current = sound_start;
			sound_limit = sound_end;
		} else {
			timer_adjust(sound_timer, TIME_NEVER, 0, 0);
			for (ch = 0; ch < 8; ch++) DAC_signed_data_16_w(ch, 0x8000);
		}
	}
}

static void update_sound_timer(void)
{
	if (sound_enabled) {
		double period = TIME_IN_HZ(125000.0 / ((vidc[0xc0] & 255) + 2));
		timer_adjust(sound_timer, period, 0, period);
	}
}

static WRITE32_HANDLER( memc_w )
{
	UINT32 pointer = ((data >> 2) & 0x7fff) * 16;
	if ((data & 0x03e00000) != 0x03600000) return;
	switch ((data >> 17) & 7) {
	case 0: video_pointer = pointer; break;
	case 1: video_start = pointer; break;
	case 2: video_end = pointer; break;
	case 4:
		sound_start = pointer;
		ioc[8] &= ~2;
		update_interrupts();
		break;
	case 5: sound_end = pointer; break;
	case 6:
		sound_current = sound_start;
		sound_limit = sound_end;
		ioc[8] |= 2;
		update_interrupts();
		break;
	case 7:
		memc_page_size = (data >> 2) & 3;
		video_enabled = (data >> 10) & 1;
		if (!sound_enabled && (data & 0x800)) {
			sound_current = sound_start;
			sound_limit = sound_end;
		}
		sound_enabled = (data >> 11) & 1;
		update_sound_timer();
		break;
	}
}

static void start_ioc_timer(int which)
{
	double period;
	if (which < 2)
		period = TIME_IN_USEC((timer_count[which] + 1) / 2.0);
	else
		period = TIME_IN_USEC((timer_count[which] + 1) * (which == 3 ? 16 : 1));
	timer_adjust(ioc_timer[which], period, which, 0);
}

static void ioc_tick(int which)
{
	start_ioc_timer(which);
	if (which < 2) {
		ioc[4] |= 0x20 << which;
		update_interrupts();
	}
}

static READ32_HANDLER( ioc_r )
{
	int reg = offset & 31;
	if (reg >= 16) {
		int which = (reg - 16) / 4;
		if ((reg & 3) == 0) return timer_output[which] & 255;
		if ((reg & 3) == 1) return (timer_output[which] >> 8) & 255;
	}
	switch (reg) {
	case 0: return (input_port_5_r(0) & 0x80) | (ioc[0] & 0x7c) | 3;
	case 4: return ioc[4] | 0x80;
	case 5: return ioc[4] & ioc[6];
	case 9: return ioc[8] & ioc[10];
	case 12: return ioc[12] | 0x80;
	case 13: return ioc[12] & ioc[14];
	}
	return ioc[reg];
}

static WRITE32_HANDLER( ioc_w )
{
	int reg = offset & 31;
	if (!ACCESSING_LSB32) return;
	if (reg >= 16) {
		int which = (reg - 16) / 4;
		switch (reg & 3) {
		case 0: case 1: ioc[reg] = data; break;
		case 2:
			timer_count[which] = ioc[16 + which * 4] | (ioc[17 + which * 4] << 8);
			start_ioc_timer(which);
			break;
		case 3:
			timer_output[which] = timer_count[which] - (UINT32)(timer_timeelapsed(ioc_timer[which]) * 2000000.0);
			break;
		}
		return;
	}
	if (reg == 5) ioc[4] &= ~data;
	else ioc[reg] = data;
	update_interrupts();
}

static READ32_HANDLER( podule_r )
{
	ioc[8] &= ~0x20;
	update_interrupts();
	switch (offset & 7) {
	case 1: return readinputport(3) & 255;
	case 2: return readinputport(4) & 255;
	case 4: return readinputport(0) & 255;
	case 5: return readinputport(2) & 255;
	case 6: return readinputport(1) & 255;
	}
	return 0;
}

static WRITE32_HANDLER( vidc_w )
{
	int reg = data >> 24;
	UINT32 value = data & 0xffffff;
	vidc[reg] = value;
	if (reg <= 0x4c) {
		int i, r = value & 15, g = (value >> 4) & 15, b = (value >> 8) & 15;
		palette_set_color(reg >> 2, r * 17, g * 17, b * 17);
		if (reg <= 0x3c) {
			for (i = 0; i < 256; i += 16) {
				r = (value & 7) | ((i & 16) >> 1);
				g = ((value >> 4) & 3) | ((i & 0x60) >> 3);
				b = ((value >> 8) & 7) | ((i & 0x80) >> 4);
				palette_set_color(0x100 + (reg >> 2) + i, r * 17, g * 17, b * 17);
			}
		}
	} else if (reg == 0xc0) update_sound_timer();
}

static VIDEO_UPDATE( ertictac )
{
	int bpp = 1 << ((vidc[0xe0] >> 2) & 3);
	int width = ((vidc[0x90] >> 14) - (vidc[0x8c] >> 14)) * 2;
	int height = (vidc[0xb0] >> 14) - (vidc[0xac] >> 14);
	UINT32 pointer = video_pointer;
	int x, y, pixels = 8 / bpp;
	fillbitmap(bitmap, Machine->pens[16], cliprect);
	if (!video_enabled || width < 1 || height < 1 || width > 1280 || height > 1024) return;
	if (Machine->visible_area.max_x != width - 1 || Machine->visible_area.max_y != height - 1)
		set_visible_area(0, width - 1, 0, height - 1);
	if (pointer >= video_end + 16) pointer = video_start;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x += pixels) {
			UINT8 data = physical_byte(pointer++);
			int i;
			for (i = 0; i < pixels && x + i < width; i++) {
				int pen = (data >> (i * bpp)) & ((1 << bpp) - 1);
				if (bpp == 8) pen += 0x100;
				if (y >= cliprect->min_y && y <= cliprect->max_y && x + i >= cliprect->min_x && x + i <= cliprect->max_x)
					plot_pixel(bitmap, x + i, y, Machine->pens[pen]);
			}
			if (pointer >= video_end + 16) pointer = video_start;
		}
	}
}

static INTERRUPT_GEN( ertictac_interrupt )
{
	ioc[4] |= 8;
	ioc[8] |= 0x20;
	update_interrupts();
}

static MACHINE_START( ertictac )
{
	int i;
	for (i = 0; i < 4; i++) ioc_timer[i] = timer_alloc(ioc_tick);
	sound_timer = timer_alloc(sound_tick);
	return 0;
}

static MACHINE_RESET( ertictac )
{
	int i;
	memset(ioc, 0, sizeof(ioc));
	memset(vidc, 0, sizeof(vidc));
	memset(memc_pages, 0xff, sizeof(memc_pages));
	memset(timer_count, 0, sizeof(timer_count));
	memset(timer_output, 0, sizeof(timer_output));
	boot_rom = 1;
	memc_page_size = 0;
	video_enabled = sound_enabled = 0;
	video_start = video_end = video_pointer = 0;
	sound_start = sound_end = sound_current = sound_limit = 0;
	ioc[0] = 0xff;
	ioc[4] = 0x90;
	ioc[8] = 2;
	ioc[12] = 0x80;
	for (i = 0; i < 4; i++) timer_adjust(ioc_timer[i], TIME_NEVER, i, 0);
	timer_adjust(sound_timer, TIME_NEVER, 0, 0);
	update_interrupts();
}

static ADDRESS_MAP_START( ertictac_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x01ffffff) AM_READWRITE(logical_r, logical_w)
	AM_RANGE(0x02000000, 0x02ffffff) AM_RAM AM_BASE(&physical_ram)
	AM_RANGE(0x03200000, 0x0320007f) AM_READWRITE(ioc_r, ioc_w)
	AM_RANGE(0x03340000, 0x0334001f) AM_READ(podule_r)
	AM_RANGE(0x033c0000, 0x033c001f) AM_READ(podule_r)
	AM_RANGE(0x03400000, 0x035fffff) AM_WRITE(vidc_w)
	AM_RANGE(0x03600000, 0x037fffff) AM_WRITE(memc_w)
	AM_RANGE(0x03800000, 0x03ffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_WRITE(memc_page_w)
ADDRESS_MAP_END

INPUT_PORTS_START( ertictac )
	PORT_START_TAG("IN 0")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN2 ) PORT_IMPULSE(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 ) PORT_IMPULSE(1)

	PORT_START_TAG("IN 1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)

	PORT_BIT( 0xc4, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN 2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)

	PORT_BIT( 0xc4, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("DSW 1")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x01, DEF_STR( French ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sound" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "Test Mode" )
	PORT_DIPSETTING(    0x04, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x00, "Music" )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x30, 0x30, "Game Timing" )
	PORT_DIPSETTING(    0x30, "Normal Game" )
	PORT_DIPSETTING(    0x20, "3.0 Minutes" )
	PORT_DIPSETTING(    0x10, "2.5 Minutes" )
	PORT_DIPSETTING(    0x00, "2.0 Minutes" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )

	PORT_START_TAG("DSW 2")
	PORT_DIPNAME( 0x05, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x05, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )

	PORT_DIPNAME( 0x0a, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x0a, DEF_STR( 1C_4C ) )

	PORT_DIPNAME( 0x10, 0x00, "Sexy Views" )
	PORT_DIPSETTING(    0x10, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(    0x20, DEF_STR( No ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Yes ) )

	PORT_START_TAG("dummy")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )


INPUT_PORTS_END

INPUT_PORTS_START( poizone )
	PORT_INCLUDE( ertictac )
	PORT_MODIFY("IN 1")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_MODIFY("IN 2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_MODIFY("DSW 1")
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING( 0x30, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING( 0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING( 0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING( 0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING( 0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING( 0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING( 0xc0, DEF_STR( 1C_4C ) )
	PORT_MODIFY("DSW 2")
	PORT_DIPNAME( 0x01, 0x01, "Setting Mode" )
	PORT_DIPSETTING( 0x01, "Manual" )
	PORT_DIPSETTING( 0x00, "Automatic" )
	PORT_DIPNAME( 0x1a, 0x00, "Clear Target" )
	PORT_DIPSETTING( 0x00, "20%" )
	PORT_DIPSETTING( 0x02, "30%" )
	PORT_DIPSETTING( 0x08, "40%" )
	PORT_DIPSETTING( 0x0a, "50%" )
	PORT_DIPSETTING( 0x10, "60%" )
	PORT_DIPSETTING( 0x12, "70%" )
	PORT_DIPSETTING( 0x18, "80%" )
	PORT_DIPSETTING( 0x1a, "90%" )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING( 0x20, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

static MACHINE_DRIVER_START( ertictac )
	MDRV_CPU_ADD(ARM, 8000000)
	MDRV_CPU_PROGRAM_MAP(ertictac_map, 0)
	MDRV_CPU_VBLANK_INT(ertictac_interrupt, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_START(ertictac)
	MDRV_MACHINE_RESET(ertictac)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1280, 1024)
	MDRV_VISIBLE_AREA(0, 319, 0, 255)
	MDRV_PALETTE_LENGTH(512)
	MDRV_VIDEO_START(generic_bitmapped)
	MDRV_VIDEO_UPDATE(ertictac)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.125)
MACHINE_DRIVER_END

ROM_START( ertictac )
	ROM_REGION(0x800000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "01", 0x00000, 0x10000, CRC(8dce677c) SHA1(9f12b1febe796038caa1ecad1d17864dc546cfd8) )
	ROM_LOAD32_BYTE( "02", 0x00001, 0x10000, CRC(7c5c916c) SHA1(d4ed5fc3a7b27253551e7d9176ed9e6513092e60) )
	ROM_LOAD32_BYTE( "03", 0x00002, 0x10000, CRC(edca5ac6) SHA1(f6c4b8030f3c1c93922c5f7232f2159e0471b93a) )
	ROM_LOAD32_BYTE( "04", 0x00003, 0x10000, CRC(959be81c) SHA1(3e8eaacf8809863fd712ad605d23fdda4e904aee) )
	ROM_LOAD32_BYTE( "05", 0x40000, 0x10000, CRC(d08a6c89) SHA1(17b0f5fb2094126146b09d69c91bf413737b0c9e) )
	ROM_LOAD32_BYTE( "06", 0x40001, 0x10000, CRC(d727bcd8) SHA1(4627f8d4d6e6f323445b3ffcfc0d9c699a9a4f89) )
	ROM_LOAD32_BYTE( "07", 0x40002, 0x10000, CRC(23b75de2) SHA1(e18f5339ca2dd362298784ff3e5479d780d823f8) )
	ROM_LOAD32_BYTE( "08", 0x40003, 0x10000, CRC(9448ccdf) SHA1(75fe3c31625f8ba1eedd806b52993e92b1f585b6) )
	ROM_LOAD32_BYTE( "09", 0x80000, 0x10000, CRC(2bfb312e) SHA1(af7a4a1926c9c3d0b3ad41a4729a17383581c070) )
	ROM_LOAD32_BYTE( "10", 0x80001, 0x10000, CRC(e7a05477) SHA1(0ec6f94a35b87e1e4529dea194fce1fde9a5b0ad) )
	ROM_LOAD32_BYTE( "11", 0x80002, 0x10000, CRC(3722591c) SHA1(e0c4075bc4b1c90a6decba3005a1f8298bd61bd1) )
	ROM_LOAD32_BYTE( "12", 0x80003, 0x10000, CRC(fe022b7e) SHA1(056f7283bc54eff555fd1db91410c020fd905063) )
	ROM_LOAD32_BYTE( "13", 0xc0000, 0x10000, CRC(83550842) SHA1(0fee78dbf13ba970e0544c48f8939550f9347822) )
	ROM_LOAD32_BYTE( "14", 0xc0001, 0x10000, CRC(3029567c) SHA1(6d49bea3a3f6f11f4182a602d37b53f1f896c154) )
	ROM_LOAD32_BYTE( "15", 0xc0002, 0x10000, CRC(500997ab) SHA1(028c7b3ca03141e5b596ab1e2ab98d0ccd9bf93a) )
	ROM_LOAD32_BYTE( "16", 0xc0003, 0x10000, CRC(70a8d136) SHA1(50b11f5701ed5b79a5d59c9a3c7d5b7528e66a4d) )
ROM_END

ROM_START( poizone )
	ROM_REGION(0x800000, REGION_USER1, 0 )
	ROM_LOAD32_BYTE( "p_son01.bin", 0x00000, 0x10000, CRC(28793c9f) SHA1(2d9f7d667203e745b47cd2cc97501ae961ae1a66) )
	ROM_LOAD32_BYTE( "p_son02.bin", 0x00001, 0x10000, CRC(2d4b6f4b) SHA1(8df2680d6e5dc41787b3a72e594f01f5e732d0ec) )
	ROM_LOAD32_BYTE( "p_son03.bin", 0x00002, 0x10000, CRC(0834d46e) SHA1(bf1cc9b47759ef39ed8fd8f334ed8f2902be3bf8) )
	ROM_LOAD32_BYTE( "p_son04.bin", 0x00003, 0x10000, CRC(9e9b1f6e) SHA1(d95067f3ecca3c079a67bd0b80e3b45c5b42151e) )
	ROM_LOAD32_BYTE( "p_son05.bin", 0x40000, 0x10000, CRC(be62ad42) SHA1(5eb51ad277ec7b7f1b5995bcdea35114f805baae) )
	ROM_LOAD32_BYTE( "p_son06.bin", 0x40001, 0x10000, CRC(c2f9141c) SHA1(e910fefcd6f0b99ab299b3a5f099b9ef84e1cc23) )
	ROM_LOAD32_BYTE( "p_son07.bin", 0x40002, 0x10000, CRC(8929c748) SHA1(35c108170590fbe97fdd4a1db7d660b4ee0adac8) )
	ROM_LOAD32_BYTE( "p_son08.bin", 0x40003, 0x10000, CRC(0ef5b14f) SHA1(425f130b2a94a4152fab763e0734e71f2913b25f) )
	ROM_LOAD32_BYTE( "p_son09.bin", 0x80000, 0x10000, CRC(e8cd75a6) SHA1(386a4ff576574e49711e72640dd3f33c8b7e04b3) )
	ROM_LOAD32_BYTE( "p_son10.bin", 0x80001, 0x10000, CRC(1dc01da7) SHA1(d37456e3407cab5eff5bbd9735c3a54e73b27545) )
	ROM_LOAD32_BYTE( "p_son11.bin", 0x80002, 0x10000, CRC(85e973ad) SHA1(850cd0dbda42eab78625038c6ea1f5b31674018a) )
	ROM_LOAD32_BYTE( "p_son12.bin", 0x80003, 0x10000, CRC(b89376d1) SHA1(cff29c2a8db88d4d104bae19a90de034158fe9e7) )

	ROM_LOAD32_BYTE( "p_son21.bin", 0x140000, 0x10000, CRC(a0c06c1e) SHA1(8d065117788e96ecd147d3d7ffdd273d4b69bb7a) )
	ROM_LOAD32_BYTE( "p_son22.bin", 0x140001, 0x10000, CRC(16f0bb52) SHA1(893ab1e72b84de7a38f88f9d713769968ebd4553) )
	ROM_LOAD32_BYTE( "p_son23.bin", 0x140002, 0x10000, CRC(e9c118b2) SHA1(110d9a204e701b9b54d89f027f8892c3f3a819c7) )
	ROM_LOAD32_BYTE( "p_son24.bin", 0x140003, 0x10000, CRC(a09d7f55) SHA1(e0d562c655c16034b40db93de801b98b7948beb2) )
ROM_END

GAME( 1990, ertictac, 0, ertictac, ertictac, 0, ROT0, "Sisteme", "Erotictac/Tactic", GAME_IMPERFECT_SOUND )
GAME( 1991, poizone,  0, ertictac, poizone,  0, ROT0, "Eterna", "Poizone", GAME_IMPERFECT_SOUND | GAME_IMPERFECT_GRAPHICS )
