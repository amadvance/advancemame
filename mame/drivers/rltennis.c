// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/**************************************************************************************************
Reality Tennis - (c) 1993 TCH

 driver by Tomasz Slanina

    based on informations provided by Antonio 'Peluko' Carrillo

Game Credits:
Antonio 'Peluko' Carrillo: programmer and game designer
David Sandoval: hardware designer

PCB Layout
----------

|-----------------------------------------------------|
|  |-----|  KM424C257  tennis_6  tennis_12     -      |
|  |Bt478|  KM424C257                                 |
|  |-----|             tennis_5  tennis_11     -      |
|           |-------|                                 |
|           |Actel  | |-------|  tennis_10     -      |
|           | A1020B| |Actel  |                       |
|J          |-------| | A1020B|  tennis_9      -      |
|A  32MHz    JOAQUIN  |-------|                       |
|M                      JUANA    tennis_8      -      |
|M            28C264                                  |
|A          tennis_1  tennis_2   tennis_7  tennis_14  |
|                                 GAL22V10            |
|            MT5C256  MT5C256              tennis_13  |
|          |------------------|     74HC404           |
|          |      68000P8     |  tennis_3  tennis_4   |
|          |------------------|                       |
|                NE555              DAC        DAC    |
|-----------------------------------------------------|

Video hardware:
---------------
Blitter based. Two layers with tricky doublebuffering.
Two Actel FPGA chips (marked as JOAQUIN and JUANA).
Juana can read data from ROMs. JOAQUIN - write to VRAM.
Both can access 256x256 pixel pages.
Size and direction of data read/write, as well as active page is
selectable for each of the chips.

Sound hardware (verify):
------------------------
~15 kHz 8 bit signed (music) and unsigned (sfx) sample player.
Two custom DACs are conencted directly to data lines of sound ROMs.
A0-A10 address lines are controlled by a counter, clocked by scaline
clock ( not verified, just guessed ). Top lines are controlled by cpu,
and select 2k sample to play. There's probably no way to stop the sample
player - when there's nothing to play - first, empty 2k of ROMs are selected.

 TODO:
- proper timing and interrupts (remove extra hacky blitter int generation @ vblank)
- fix various gfx glitches here and there, mostly related to wrong size of data
  (what's the correct size? based on src or dest rectangle ? is there some kind of zoom?
  or just rect clipping?)
- what the 70000a blitter reg is for ?
- awfully similar to littlerb;
**************************************************************************************************/

/*
Differences from MAME:
- The parallel EEPROM is mapped as plain battery backed RAM.
- The RAMDAC is emulated with simple palette index/data registers.
- The PLD dumps are not loaded.
*/

#include "driver.h"
#include "sound/dac.h"

#define RLT_REFRESH_RATE   60
#define RLT_TIMER_FREQ     (RLT_REFRESH_RATE * 256)

#define RLT_NUM_BLITTER_REGS 8
#define RLT_NUM_BITMAPS      4

#define BLTFLAG_DST_X_DIR   (1 << 0)
#define BLTFLAG_DST_Y_DIR   (1 << 1)
#define BLTFLAG_DST_LR      (1 << 2)
#define BLTFLAG_DST_UD      (1 << 3)
#define BLTFLAG_DISPLAY_UD  (1 << 5)

#define BLTFLAG_SRC_X_DIR   (1 << 8)
#define BLTFLAG_SRC_Y_DIR   (1 << 9)
#define BLTFLAG_SRC_LR      (1 << 10)
#define BLTFLAG_SRC_UD      (1 << 11)

#define BLTSTRT_ROM_MSB     (1 << 8)
#define BLTSTRT_TRIGGER     (1 << 14)
#define BLTSTRT_LAYER       (1 << 15)

#define SRC_SHIFT           8

enum
{
	BLT_X_START = 0,
	BLT_X_END,
	BLT_Y_START,
	BLT_Y_END,
	BLT_FLAGS,
	BLT_UNK,
	BLT_START,
	BLT_UNK2
};

enum
{
	BITMAP_BG = 0,
	BITMAP_FG_1,
	BITMAP_FG_2,
	BITMAP_FG_DISPLAY
};

static UINT16 blitter[RLT_NUM_BLITTER_REGS];
static UINT8 palette_data[256 * 3];
static int palpos_r, palpos_w;
static int data740000, data760000;
static int dac_counter;
static int sample_rom_offset[2];
static int offset_shift;
static int unk_counter;
static mame_bitmap *tmp_bitmap[RLT_NUM_BITMAPS];

/*
Blitter registers (names chosen arbitrarily):

700000 - BLT_X_START
  -------- xxxxxxxx dst x start
  xxxxxxxx -------- src x start

700002 - BLT_X_END
  -------- xxxxxxxx dst x end
  xxxxxxxx -------- src x end

700004 - BLT_Y_START
  -------- xxxxxxxx dst y start
  xxxxxxxx -------- src y start

700006 - BLT_Y_END
  -------- xxxxxxxx dst y end
  xxxxxxxx -------- src y end

700008 - BLT_FLAGS
  -------- -------x BLTFLAG_DST_X_DIR    x dst direction (step inc or dec)
  -------- ------x- BLTFLAG_DST_Y_DIR    y dst direction
  -------- -----x-- BLTFLAG_DST_LR       LR and UD controls the quarter of framebuffer to use
  -------- ----x--- BLTFLAG_DST_UD       /
  -------- --x----- BLTFLAG_DISPLAY_UD   display buffer select
  -------x -------- BLTFLAG_SRC_X_DIR    x src direction
  ------x- -------- BLTFLAG_SRC_Y_DIR    y src direction
  -----x-- -------- BLTFLAG_SRC_LR       LR and UD controls the quarter of src buffer to use
  ----x--- -------- BLTFLAG_SRC_UD       /
  xxxx---- --------                      src ROM num

70000c - BLT_START
  -------x -------- BLTSTRT_ROM_MSB       src ROM MSB
  -x------ -------- BLTSTRT_TRIGGER       blit start
  x------- -------- BLTSTRT_LAYER         FG or BG layer of framebuffer
*/

static WRITE16_HANDLER( blitter_w )
{
	int old_data = blitter[offset];
	int new_data;

	COMBINE_DATA(&blitter[offset]);
	new_data = blitter[offset];

	/* visible page flip and clear */
	if (offset == BLT_FLAGS && ((new_data ^ old_data) & BLTFLAG_DISPLAY_UD))
	{
		if (new_data & BLTFLAG_DISPLAY_UD)
		{
			copybitmap(tmp_bitmap[BITMAP_FG_DISPLAY], tmp_bitmap[BITMAP_FG_1], 0, 0, 0, 0, 0, TRANSPARENCY_NONE, 0);
			fillbitmap(tmp_bitmap[BITMAP_FG_1], 0, 0);
		}
		else
		{
			copybitmap(tmp_bitmap[BITMAP_FG_DISPLAY], tmp_bitmap[BITMAP_FG_2], 0, 0, 0, 0, 0, TRANSPARENCY_NONE, 0);
			fillbitmap(tmp_bitmap[BITMAP_FG_2], 0, 0);
		}
	}

	/* blit strobe 0->1 */
	if (offset == BLT_START && (((new_data ^ old_data) & new_data) & BLTSTRT_TRIGGER))
	{
		const UINT8 *gfx = memory_region(REGION_GFX1);
		int src_x0, src_y0, src_x1, src_y1;
		int dst_x0, dst_y0, dst_x1, dst_y1;
		int x_dst_step, y_dst_step, x_src_step, y_src_step;
		int blit_w, blit_h, blit_w1, blit_h1;
		int layer, force_blit;
		int x, y, idx_x, idx_y;

		cpunum_set_input_line(0, 1, HOLD_LINE);

		src_x0 = (blitter[BLT_X_START] >> SRC_SHIFT) + ((blitter[BLT_FLAGS] & BLTFLAG_SRC_LR) ? 256 : 0);
		src_y0 = (blitter[BLT_Y_START] >> SRC_SHIFT) + ((blitter[BLT_FLAGS] >> 3) & 0xff00) + ((blitter[BLT_START] & BLTSTRT_ROM_MSB) ? (1 << 0xd) : 0);

		dst_x0 = blitter[BLT_X_START] & 0xff;
		dst_y0 = blitter[BLT_Y_START] & 0xff;

		dst_x1 = blitter[BLT_X_END] & 0xff;
		dst_y1 = blitter[BLT_Y_END] & 0xff;

		src_x1 = ((blitter[BLT_X_END] >> SRC_SHIFT) & 0xff) + ((blitter[BLT_FLAGS] & BLTFLAG_SRC_LR) ? 256 : 0);
		src_y1 = ((blitter[BLT_Y_END] >> SRC_SHIFT) & 0xff) + ((blitter[BLT_FLAGS] >> 3) & 0xff00) + ((blitter[BLT_START] & BLTSTRT_ROM_MSB) ? (1 << 0xd) : 0);

		x_dst_step = (blitter[BLT_FLAGS] & BLTFLAG_DST_X_DIR) ? 1 : -1;
		y_dst_step = (blitter[BLT_FLAGS] & BLTFLAG_DST_Y_DIR) ? 1 : -1;

		x_src_step = (blitter[BLT_FLAGS] & BLTFLAG_SRC_X_DIR) ? 1 : -1;
		y_src_step = (blitter[BLT_FLAGS] & BLTFLAG_SRC_Y_DIR) ? 1 : -1;

		blit_w = src_x1 - src_x0;
		blit_h = src_y1 - src_y0;

		blit_w1 = dst_x1 - dst_x0;
		blit_h1 = dst_y1 - dst_y0;

		if (blit_w1 < 0) blit_w1 = (-blit_w1) ^ 0xff;
		if (blit_h1 < 0) blit_h1 = -blit_h1;

		if (blit_w < 0) blit_w = -blit_w;
		if (blit_h < 0) blit_h = -blit_h;

		/* the real size rule is unknown, this gives the best results */
		if (blit_w1 < blit_w) blit_w1 = blit_w;
		if (blit_h1 < blit_h) blit_h1 = blit_h;

		layer = (blitter[BLT_START] & BLTSTRT_LAYER) ? BITMAP_BG : BITMAP_FG_1;
		if (layer == BITMAP_FG_1 && (blitter[BLT_FLAGS] & BLTFLAG_DST_UD))
			layer = BITMAP_FG_2;

		/* a 1x1 source seems to be a color fill of the background layer */
		force_blit = (blit_w == 1 && blit_h == 1);

		for (x = dst_x0, idx_x = 0; idx_x <= blit_w1; x += x_dst_step, ++idx_x)
		{
			for (y = dst_y0, idx_y = 0; idx_y <= blit_h1; y += y_dst_step, ++idx_y)
			{
				int xx = src_x0 + x_src_step * idx_x;
				int yy = src_y0 + y_src_step * idx_y;
				int pix, screen_x;

				if (force_blit)
				{
					xx = src_x0;
					yy = src_y0;
				}

				pix = gfx[(yy * 512 + xx) & 0x0ffffff];
				screen_x = (x & 0xff) + ((blitter[BLT_FLAGS] & BLTFLAG_DST_LR) ? 256 : 0);

				if ((pix || force_blit) && screen_x > 0 && y > 0 && screen_x < 512 && y < 256)
					plot_pixel(tmp_bitmap[layer], screen_x, y, Machine->pens[pix]);
			}
		}
	}
}

static WRITE16_HANDLER( ramdac_address_wm_w )
{
	palpos_w = (data & 0xff) * 3;
}

static WRITE16_HANDLER( ramdac_address_rm_w )
{
	palpos_r = (data & 0xff) * 3;
}

static WRITE16_HANDLER( ramdac_data_w )
{
	int color = palpos_w / 3;

	palette_data[palpos_w] = data & 0xff;
	palpos_w = (palpos_w + 1) % (256 * 3);

	palette_set_color(color, palette_data[color * 3], palette_data[color * 3 + 1], palette_data[color * 3 + 2]);
}

static READ16_HANDLER( ramdac_data_r )
{
	int data = palette_data[palpos_r];

	palpos_r = (palpos_r + 1) % (256 * 3);
	return data;
}

static VIDEO_START( rltennis )
{
	int i;

	for (i = 0; i < RLT_NUM_BITMAPS; ++i)
	{
		tmp_bitmap[i] = auto_bitmap_alloc(512, 256);
		if (!tmp_bitmap[i])
			return 1;
	}
	return 0;
}

static VIDEO_UPDATE( rltennis )
{
	copybitmap(bitmap, tmp_bitmap[BITMAP_BG], 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	copybitmap(bitmap, tmp_bitmap[BITMAP_FG_DISPLAY], 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, Machine->pens[0]);
}

static READ16_HANDLER( io_r )
{
	/* the top 3 bits control the sample address update */
	return (readinputport(0) & 0x1fff) | ((unk_counter << 13) & 0xe000);
}

static WRITE16_HANDLER( snd1_w )
{
	COMBINE_DATA(&data760000);
}

static WRITE16_HANDLER( snd2_w )
{
	COMBINE_DATA(&data740000);
}

static ADDRESS_MAP_START( rltennis_main, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x700000, 0x70000f) AM_WRITE(blitter_w)
	AM_RANGE(0x720000, 0x720001) AM_WRITE(ramdac_address_wm_w)
	AM_RANGE(0x720002, 0x720003) AM_READWRITE(ramdac_data_r, ramdac_data_w)
	AM_RANGE(0x720006, 0x720007) AM_WRITE(ramdac_address_rm_w)
	AM_RANGE(0x740000, 0x740001) AM_WRITE(snd1_w)
	AM_RANGE(0x760000, 0x760001) AM_WRITE(snd2_w)
	AM_RANGE(0x780000, 0x780001) AM_WRITENOP
	AM_RANGE(0x7a0000, 0x7a0003) AM_READNOP
	AM_RANGE(0x7e0000, 0x7e0001) AM_READ(io_r)
	AM_RANGE(0x7e0002, 0x7e0003) AM_READ(input_port_1_word_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( rltennis )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SERVICE )
	PORT_BIT( 0xe000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0xff80, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static void sample_player(int param)
{
	const UINT8 *samples1 = memory_region(REGION_SOUND1);
	const UINT8 *samples2 = memory_region(REGION_SOUND2);

	/* reload the top address bits */
	if ((dac_counter & 0x7ff) == 0x7ff)
	{
		sample_rom_offset[0] = ((data740000 >> offset_shift) & 0xff) << 11;
		sample_rom_offset[1] = ((data760000 >> offset_shift) & 0xff) << 11;
		offset_shift ^= 8; /* switch between MSB and LSB */
	}
	++dac_counter; /* update the low address bits */

	DAC_signed_data_w(0, samples1[sample_rom_offset[0] + (dac_counter & 0x7ff)]);
	DAC_data_w(1, samples2[sample_rom_offset[1] + (dac_counter & 0x7ff)]);
}

static INTERRUPT_GEN( rltennis_interrupt )
{
	++unk_counter;
	cpunum_set_input_line(0, 4, HOLD_LINE);
	/* the extra level 1 interrupt avoids a dead loop, the real timing is unknown */
	cpunum_set_input_line(0, 1, HOLD_LINE);
}

static MACHINE_RESET( rltennis )
{
	memset(blitter, 0, sizeof(blitter));
	data740000 = data760000 = 0;
	dac_counter = 0;
	sample_rom_offset[0] = sample_rom_offset[1] = 0;
	offset_shift = 0;
	unk_counter = 0;
	timer_pulse(TIME_IN_HZ(RLT_TIMER_FREQ), 0, sample_player);
}

static MACHINE_DRIVER_START( rltennis )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(rltennis_main, 0)
	MDRV_CPU_VBLANK_INT(rltennis_interrupt, 1)

	MDRV_FRAMES_PER_SECOND(RLT_REFRESH_RATE)
	MDRV_VBLANK_DURATION(0)

	MDRV_MACHINE_RESET(rltennis)
	MDRV_NVRAM_HANDLER(generic_0fill)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 240)
	MDRV_VISIBLE_AREA(0, 319, 0, 239)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_START(rltennis)
	MDRV_VIDEO_UPDATE(rltennis)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.25)
MACHINE_DRIVER_END

ROM_START( rltennis )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tennis_1.u12", 0x00001, 0x80000, CRC(2ded10d7) SHA1(cca1e858c9c759ef5c0aca6ee50d23d5d532534c) )
	ROM_LOAD16_BYTE( "tennis_2.u19", 0x00000, 0x80000, CRC(a0dbd2ed) SHA1(8db7dbb6a36fd0fb382a4938d7eba1f7662aa672) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD( "tennis_5.u33", 0x000000, 0x80000, CRC(067a2e4b) SHA1(ab5a227de2b0c51b17aeca68c8af1bf224904ac8) )
	ROM_LOAD( "tennis_6.u34", 0x080000, 0x80000, CRC(901df2c1) SHA1(7e57d7c7e281ddc02a3e34178d3e471bd8e1d572) )
	ROM_LOAD( "tennis_7.u35", 0x100000, 0x80000, CRC(8d70fb37) SHA1(250c4c3d32e5a7e17413ee41e1abccb0492b63fd) )
	ROM_LOAD( "tennis_8.u36", 0x180000, 0x80000, CRC(26d202ba) SHA1(0e841e35de328f23624a19780a734a18f5409d69) )
	ROM_LOAD( "tennis_9.u37", 0x200000, 0x80000, CRC(1d164ee0) SHA1(b9c80b3c0dadbff36a04141b8995a5282a8d10f7) )
	ROM_LOAD( "tennis_10.u38",0x280000, 0x80000, CRC(fd2c6647) SHA1(787f236d5b72ee24d39e783eb2453bea58f07290) )
	ROM_LOAD( "tennis_11.u39",0x300000, 0x80000, CRC(a59dc0c8) SHA1(48f258e74fbb64b7538c9777d7598774ca8396eb) )
	ROM_LOAD( "tennis_12.u40",0x380000, 0x80000, CRC(b9677887) SHA1(84b79864555d3d6e9c443913910a055e27d30d08) )
	ROM_LOAD( "tennis_13.u41",0x400000, 0x80000, CRC(3d4fbcac) SHA1(e01f479d7d516ff83cbbd82d83617146d7a242d3) )
	ROM_LOAD( "tennis_14.u42",0x480000, 0x80000, CRC(37fe0f5d) SHA1(7593f1ea07bc0a741c952e6850bed1bf0a824510) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "tennis_4.u59", 0x00000, 0x80000, CRC(f56462ea) SHA1(638777e12f2649a5b4366f034f0ba721fc4580a8) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )
	ROM_LOAD( "tennis_3.u52", 0x00000, 0x80000, CRC(517dcd0e) SHA1(b2703e185ee8cf7e115ea07151e7bee8be34948b) )

ROM_END

// PCB marked as "TCH-MAVIR.001
// Two TI TPC1020AF instead of two Actel A1020B, also labeled as JOAQUIN (A) and JUANA (B)
ROM_START( rltennisa )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tennis_1_a.bin", 0x00001, 0x80000, CRC(30bd9f1d) SHA1(52225793fb1fee4d395f21c45756e9244cf60b9e) )
	ROM_LOAD16_BYTE( "tennis_2_a.bin", 0x00000, 0x80000, CRC(f2997957) SHA1(c2ccaecd337c46cd9e4a40d43f30f203efc10111) )

	ROM_REGION( 0x1000000, REGION_GFX1, ROMREGION_ERASE00 )
	ROM_LOAD( "tennis_5.u33", 0x000000, 0x80000, CRC(067a2e4b) SHA1(ab5a227de2b0c51b17aeca68c8af1bf224904ac8) )
	ROM_LOAD( "tennis_6.u34", 0x080000, 0x80000, CRC(901df2c1) SHA1(7e57d7c7e281ddc02a3e34178d3e471bd8e1d572) )
	ROM_LOAD( "tennis_7.u35", 0x100000, 0x80000, CRC(8d70fb37) SHA1(250c4c3d32e5a7e17413ee41e1abccb0492b63fd) )
	ROM_LOAD( "tennis_8.u36", 0x180000, 0x80000, CRC(26d202ba) SHA1(0e841e35de328f23624a19780a734a18f5409d69) )
	ROM_LOAD( "tennis_9.u37", 0x200000, 0x80000, CRC(1d164ee0) SHA1(b9c80b3c0dadbff36a04141b8995a5282a8d10f7) )
	ROM_LOAD( "tennis_10.u38",0x280000, 0x80000, CRC(fd2c6647) SHA1(787f236d5b72ee24d39e783eb2453bea58f07290) )
	ROM_LOAD( "tennis_11.u39",0x300000, 0x80000, CRC(a59dc0c8) SHA1(48f258e74fbb64b7538c9777d7598774ca8396eb) )
	ROM_LOAD( "tennis_12.u40",0x380000, 0x80000, CRC(b9677887) SHA1(84b79864555d3d6e9c443913910a055e27d30d08) )
	ROM_LOAD( "tennis_13.u41",0x400000, 0x80000, CRC(3d4fbcac) SHA1(e01f479d7d516ff83cbbd82d83617146d7a242d3) )
	ROM_LOAD( "tennis_14.u42",0x480000, 0x80000, CRC(37fe0f5d) SHA1(7593f1ea07bc0a741c952e6850bed1bf0a824510) )

	ROM_REGION( 0x080000, REGION_SOUND1, 0 )
	ROM_LOAD( "tennis_4.u59", 0x00000, 0x80000, CRC(f56462ea) SHA1(638777e12f2649a5b4366f034f0ba721fc4580a8) )

	ROM_REGION( 0x080000, REGION_SOUND2, 0 )
	ROM_LOAD( "tennis_3.u52", 0x00000, 0x80000, CRC(517dcd0e) SHA1(b2703e185ee8cf7e115ea07151e7bee8be34948b) )

ROM_END

GAME( 1993, rltennis,         0, rltennis, rltennis, 0, ROT0, "TCH", "Reality Tennis (set 1)", GAME_IMPERFECT_GRAPHICS )
GAME( 1993, rltennisa, rltennis, rltennis, rltennis, 0, ROT0, "TCH", "Reality Tennis (set 2)", GAME_IMPERFECT_GRAPHICS )
