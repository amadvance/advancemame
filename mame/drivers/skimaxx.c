// license:BSD-3-Clause
// copyright-holders:Philip Bennett, Luca Elia
/***************************************************************************

    Skimaxx, Kyle Hodgetts / ICE
    Two 68EC030-40 CPUs, TMS34010-50 and four MSM6295 sound chips.
    Video uses a rotation/zoom blitter and a separate text framebuffer.

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "sound/okim6295.h"

static UINT32 *blitter_regs, *fpga_ctrl;
static UINT16 *fg_buffer;
static UINT32 *bg_buffer, *bg_front, *bg_back;
static UINT32 src_x, src_y, src_dx, src_dy;

static WRITE32_HANDLER( blitter_w )
{
	UINT32 value;
	COMBINE_DATA(&blitter_regs[offset]);
	value = blitter_regs[offset];
	switch (offset)
	{
	case 0:
		src_dy = (INT16)value;
		break;
	case 1:
		src_x = (value & 0x1ff) << 8;
		src_y = (value >> 9) << 8;
		break;
	case 2:
		if ((mem_mask & 0xffff0000) != 0xffff0000) src_dx = (INT16)(value >> 16);
		if ((mem_mask & 0x0000ffff) != 0x0000ffff) src_dx = (INT16)value;
		break;
	}
}

static READ32_HANDLER( blitter_r )
{
	UINT32 address = ((src_x >> 8) & 0x1ff) + ((src_y >> 8) << 9);
	UINT16 pen = ((UINT16 *)memory_region(REGION_GFX2))[address % (0x600000/2)] & 0x7fff;
	if (pen)
	{
		if ((mem_mask & 0xffff0000) != 0xffff0000)
			bg_back[offset] = (bg_back[offset] & 0xffff) | (pen << 16);
		else
			bg_back[offset] = (bg_back[offset] & 0xffff0000) | pen;
	}
	src_x = (src_x & 0x10000) | ((src_x + src_dx) & 0xffff);
	src_y = (src_y & 0xffff0000) | ((src_y + src_dy) & 0xffff);
	return 0;
}

static void select_buffer(void)
{
	int bank = (*fpga_ctrl & 0x40) ? 1 : 0;
	/* CPU writes target the back buffer while video keeps reading the other half. */
	bg_back = bg_buffer + bank * 0x20000;
	bg_front = bg_buffer + (1 - bank) * 0x20000;
	memory_set_bankptr(1, bg_back);
}

static WRITE32_HANDLER( fpga_ctrl_w )
{
	COMBINE_DATA(fpga_ctrl);
	if (ACCESSING_LSB32) select_buffer();
}

static READ32_HANDLER( fpga_status_r ) { return (*fpga_ctrl & 0x20) ? 0x80 : 0; }
static READ32_HANDLER( link_status_r ) { return 0x80; }

static WRITE32_HANDLER( sub_control_w )
{
	if (ACCESSING_LSB32)
	{
		cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 1) ? CLEAR_LINE : ASSERT_LINE);
		cpunum_set_input_line(1, INPUT_LINE_HALT, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
	}
}

static WRITE32_HANDLER( tms_host_w )
{
	if ((mem_mask & 0xffff) != 0xffff) tms34010_host_w(2, offset, data);
}
static READ32_HANDLER( tms_host_r ) { return tms34010_host_r(2, offset); }
static READ32_HANDLER( dsw_r ) { return readinputport(0); }
static READ32_HANDLER( coin_r ) { return readinputport(1); }
static READ32_HANDLER( analog_r ) { return BITSWAP8(readinputport(2 + offset) ^ 0x1f,0,1,2,3,4,5,6,7); }

static READ32_HANDLER( oki_r )
{
	switch (offset)
	{
	case 0: return OKIM6295_status_0_r(0);
	case 1: return OKIM6295_status_1_r(0);
	case 2: return OKIM6295_status_2_r(0);
	default: return OKIM6295_status_3_r(0);
	}
}
static WRITE32_HANDLER( oki_w )
{
	if (!ACCESSING_LSB32) return;
	switch (offset)
	{
	case 0: OKIM6295_data_0_w(0, data); break;
	case 1: OKIM6295_data_1_w(0, data); break;
	case 2: OKIM6295_data_2_w(0, data); break;
	case 3: OKIM6295_data_3_w(0, data); break;
	}
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x001fffff) AM_ROM
	AM_RANGE(0x10000000, 0x10000003) AM_WRITE(sub_control_w)
	AM_RANGE(0x10100000, 0x1010000f) AM_READWRITE(tms_host_r, tms_host_w)
	AM_RANGE(0x10180000, 0x1018ffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x20000000, 0x20000003) AM_READ(MRA32_NOP)
	AM_RANGE(0x20000010, 0x2000001f) AM_READWRITE(oki_r, oki_w)
	AM_RANGE(0x20000020, 0x20000023) AM_READ(link_status_r)
	AM_RANGE(0x20000024, 0x20000027) AM_WRITE(MWA32_NOP)
	AM_RANGE(0x20000040, 0x20000043) AM_RAM
	AM_RANGE(0x20000044, 0x20000047) AM_READ(dsw_r)
	AM_RANGE(0x20000048, 0x2000004b) AM_READ(coin_r)
	AM_RANGE(0x2000004c, 0x2000004f) AM_READ(fpga_status_r)
	AM_RANGE(0x20000050, 0x20000057) AM_READ(analog_r) AM_WRITE(MWA32_NOP)
	AM_RANGE(0xfffc0000, 0xfffdffff) AM_RAM AM_MIRROR(0x20000)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sub_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x003fffff) AM_ROM
	AM_RANGE(0x20000000, 0x2007ffff) AM_READ(blitter_r)
	AM_RANGE(0x30000000, 0x3000000f) AM_RAM AM_WRITE(blitter_w) AM_BASE(&blitter_regs)
	AM_RANGE(0x40000000, 0x40000003) AM_RAM AM_WRITE(fpga_ctrl_w) AM_BASE(&fpga_ctrl)
	AM_RANGE(0x50000000, 0x5007ffff) AM_READWRITE(MRA32_BANK1, MWA32_BANK1)
	AM_RANGE(0xfffc0000, 0xfffcffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xfffe0000, 0xfffeffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0xffff0000, 0xffffffff) AM_RAM
ADDRESS_MAP_END

/* TMS34010 map bounds are bit addresses; the memory core performs byte conversion. */
static ADDRESS_MAP_START( tms_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x0003ffff) AM_RAM
	AM_RANGE(0x00050000, 0x0005ffff) AM_RAM
	AM_RANGE(0x00220000, 0x003fffff) AM_RAM AM_BASE(&fg_buffer)
	AM_RANGE(0x02000000, 0x0200000f) AM_RAM
	AM_RANGE(0x02100000, 0x0210000f) AM_RAM
	AM_RANGE(0x04000000, 0x047fffff) AM_ROM AM_REGION(REGION_GFX1, 0)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, tms34010_io_register_w)
	AM_RANGE(0xff800000, 0xffffffff) AM_ROM AM_REGION(REGION_CPU3, 0)
ADDRESS_MAP_END

INPUT_PORTS_START( skimaxx )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x07, 0x07, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x05, "12" )
	PORT_DIPSETTING(    0x04, "17" )
	PORT_DIPSETTING(    0x03, "25" )
	PORT_DIPSETTING(    0x02, "33" )
	PORT_DIPSETTING(    0x01, "50" )
	PORT_DIPSETTING(    0x07, "100" )
	PORT_DIPSETTING(    0x06, "200" )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, "0" )
	PORT_DIPSETTING(    0x00, "1" )
	PORT_DIPNAME( 0x70, 0x70, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "8" )
	PORT_DIPSETTING(    0x30, "10" )
	PORT_DIPSETTING(    0x40, "12" )
	PORT_DIPSETTING(    0x50, "15" )
	PORT_DIPSETTING(    0x60, "20" )
	PORT_DIPSETTING(    0x70, "30" )
	PORT_DIPNAME( 0x80, 0x80, "Time For Super Course" )
	PORT_DIPSETTING(    0x80, "50 s" )
	PORT_DIPSETTING(    0x00, "60 s" )

	PORT_START_TAG("COIN")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1   )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2   )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START1  )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START_TAG("X")
	PORT_BIT( 0x1f, 0x10, IPT_AD_STICK_X ) PORT_SENSITIVITY(10) PORT_KEYDELTA(1) PORT_MINMAX(0x00,0x1e)

	PORT_START_TAG("Y")
	PORT_BIT( 0x1f, 0x10, IPT_AD_STICK_Y ) PORT_SENSITIVITY(10) PORT_KEYDELTA(1) PORT_MINMAX(0x00,0x1e)


INPUT_PORTS_END



static void to_shiftreg(offs_t address, UINT16 *shiftreg)
{
	UINT32 offset = (address - 0x220000) >> 4;
	if (offset + 512 <= 0x1e000) memcpy(shiftreg, fg_buffer + offset, 512 * sizeof(UINT16));
}
static void from_shiftreg(offs_t address, UINT16 *shiftreg)
{
	UINT32 offset = (address - 0x220000) >> 4;
	if (offset + 512 <= 0x1e000) memcpy(fg_buffer + offset, shiftreg, 512 * sizeof(UINT16));
}
static struct tms34010_config tms_config = { 0, 0, to_shiftreg, from_shiftreg, 0, 0 };

static VIDEO_START( skimaxx )
{
	bg_buffer = auto_malloc(0x100000);
	memset(bg_buffer, 0, 0x100000);
	*fpga_ctrl = 0;
	select_buffer();
	src_x = src_y = src_dx = src_dy = 0;
	return 0;
}

static PALETTE_INIT( skimaxx )
{
	int i;
	for (i = 0; i < 32768; ++i) palette_set_color(i, pal5bit(i >> 10), pal5bit(i >> 5), pal5bit(i));
}

static VIDEO_UPDATE( skimaxx )
{
	UINT16 dpyadr, dpyctl, start;
	int y;
	fillbitmap(bitmap, get_black_pen(), cliprect);
	if (tms34010_io_display_blanked(2)) return;
	cpuintrf_push_context(2);
	start = dpyadr = tms34010_io_register_r(REG_DPYSTRT, 0);
	dpyctl = tms34010_io_register_r(REG_DPYCTL, 0);
	cpuintrf_pop_context();
	for (y = 0; y <= cliprect->max_y; ++y)
	{
		UINT16 row = dpyadr;
		if (!(dpyctl & 0x400)) row ^= 0xfffc;
		row >>= 4;
		if (y >= cliprect->min_y && row >= 0x220 && row < 0x400)
		{
			const UINT16 *fg = fg_buffer + ((row - 0x220) << 8);
			const UINT32 *bg = bg_front + ((row - 0x220)/2 * 512);
			UINT16 *dest = bitmap->line[y];
			int x;
			for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			{
				UINT16 pen = fg[x/2] & 0x7fff;
				if (!pen) pen = (bg[x/2] >> ((x & 1) ? 0 : 16)) & 0x7fff;
				dest[x] = Machine->pens[pen];
			}
		}
		/* The low two address bits count repeated scanlines before the row advances. */
		if (!(dpyadr & 3)) dpyadr = ((dpyadr & 0xfffc) - (dpyctl & 0x3fc)) | (start & 3);
		else dpyadr = (dpyadr & 0xfffc) | ((dpyadr - 1) & 3);
	}
}

static MACHINE_DRIVER_START( skimaxx )
	MDRV_CPU_ADD(M68020, 40000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq3_line_hold, 1)
	MDRV_CPU_ADD(M68020, 40000000)
	MDRV_CPU_PROGRAM_MAP(sub_map, 0)
	MDRV_CPU_ADD(TMS34010, 50000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(tms_map, 0)
	MDRV_INTERLEAVE(100)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(1024, 256)
	MDRV_VISIBLE_AREA(0, 639, 0, 239)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_PALETTE_INIT(skimaxx)
	MDRV_VIDEO_START(skimaxx)
	MDRV_VIDEO_UPDATE(skimaxx)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	/* Two devices feed each channel; reserve mixing headroom for simultaneous effects. */
	MDRV_SOUND_ADD(OKIM6295, 4000000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 2000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_2)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 4000000/165)
	MDRV_SOUND_CONFIG(okim6295_interface_region_3)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
	MDRV_SOUND_ADD(OKIM6295, 2000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_4)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

ROM_START( skimaxx )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD32_BYTE( "main2_0.ve2", 0x000000, 0x80000, CRC(0e139138) SHA1(b28b0f5dba7810712cee99a9a9748ca2dabddf15) )
	ROM_LOAD32_BYTE( "main2_0.ve4", 0x000001, 0x80000, CRC(4e0502cf) SHA1(451efd82656dd32e696f572d848b2717453f05c8) )
	ROM_LOAD32_BYTE( "main2_0.ve3", 0x000002, 0x80000, CRC(53887cfd) SHA1(79d9547e0e9c4c41913279f977bf25cdbe800356) )
	ROM_LOAD32_BYTE( "main2_0.ve5", 0x000003, 0x80000, CRC(6317f54d) SHA1(057141acd2a8a804d8cc0487d5cacc286c344866) )

	ROM_REGION( 0x400000, REGION_CPU2, 0 )
	ROM_LOAD32_BYTE( "ve1v-2_0", 0x000003, 0x100000, CRC(871e16d2) SHA1(9318ebcff4032fffcbe53c8fb805b14578eaebac) )
	ROM_LOAD32_BYTE( "ve2v-2_0", 0x000002, 0x100000, CRC(17b01a96) SHA1(79ef3bd6a22abf774095741c29deeb6af53d1585) )
	ROM_LOAD32_BYTE( "ve3v-2_0", 0x000001, 0x100000, CRC(c1491795) SHA1(3a527ea4bf8cfdab072153b1cd66b0d090ef4d31) )
	ROM_LOAD32_BYTE( "ve4v-2_0", 0x000000, 0x100000, CRC(950773bb) SHA1(68e4a5780701488a934104eb87d10c36d736f049) )

	ROM_REGION16_LE( 0x100000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "vc8v-2_0", 0x000000, 0x80000, CRC(a6e9ef81) SHA1(59a0fb149e17d3773adb980428a0b107647bd4fa) )
	ROM_LOAD16_BYTE( "vc9v-2_0", 0x000001, 0x80000, CRC(b1e8ba65) SHA1(b91cf93ecd9b6067664780ab2c1b69f632c7ae05) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )
	ROM_LOAD16_BYTE( "vc10v2_0", 0x000000, 0x80000, CRC(433651cd) SHA1(9b9801703d16adbbca2b03e1714490fb166d48a0) )
	ROM_LOAD16_BYTE( "vc11v2_0", 0x000001, 0x80000, CRC(a906fc72) SHA1(c61ad560f203c7f507cc7b7dc8834f529a6501a7) )

	ROM_REGION16_LE( 0x600000, REGION_GFX2, 0 )
	ROM_LOAD16_BYTE( "ve5v-2_0", 0x000000, 0x100000, CRC(b95544a3) SHA1(1ad49da54d36543229e241887c3dd8411ef5c89a) )
	ROM_LOAD16_BYTE( "ve6v-2_0", 0x000001, 0x100000, CRC(d98bf89a) SHA1(f7fd66487f7696b53ee90a4cf45a810ec791df59) )
	ROM_LOAD16_BYTE( "ve10v2_0", 0x200000, 0x100000, CRC(c2a16a62) SHA1(3663c19a6517b0a01fb454523b995be3cdf2e4b3) )
	ROM_LOAD16_BYTE( "ve9v-2_0", 0x200001, 0x100000, CRC(587b364d) SHA1(b53412bdcc804727990d959e5b2399a7e5e7fbf3) )
	ROM_LOAD16_BYTE( "ve7v-2_0", 0x400000, 0x100000, CRC(a927716b) SHA1(0189d7b08b65b6a8a8d4c3e2affe93612db38155) )
	ROM_LOAD16_BYTE( "ve8v-2_0", 0x400001, 0x100000, CRC(ad8ed4a4) SHA1(5d1339b3d6ce59fea062273fa2da35988ba94a80) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "main2_0.u3", 0x000000, 0x80000, CRC(24d8c6ad) SHA1(06f51a4c380c91c930d646826246f62c4e1f9cda) )

	ROM_REGION( 0x80000, REGION_SOUND2, 0 )
	ROM_LOAD( "main2_0.u2", 0x000000, 0x80000, CRC(c84b3c46) SHA1(b956358518495aa822a5b699cbad1abac212dd09) )

	ROM_REGION( 0x80000, REGION_SOUND3, 0 )
	ROM_LOAD( "main2_0.u5", 0x000000, 0x80000, CRC(e2ba07ad) SHA1(cf82753975f7b6756cca4e10b5372e00135440bf) )

	ROM_REGION( 0x80000, REGION_SOUND4, 0 )
	ROM_LOAD( "main2_0.u4", 0x000000, 0x80000, CRC(c84b3c46) SHA1(b956358518495aa822a5b699cbad1abac212dd09) )
ROM_END


GAME( 1996, skimaxx, 0, skimaxx, skimaxx, 0, ROT0, "Kyle Hodgetts / ICE", "Skimaxx", GAME_IMPERFECT_GRAPHICS )
