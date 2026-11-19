// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina
/*

Entertainment Enterprises Vampire

TODO:
- correct interrupt handling (main and sound cpu)
- communication between main and audio cpu
- PIT outputs
- correct clocks
- color decode
- fix gfx glitches - extra pixels (column?) on edges (only when gfx is flipped)

Hardware notes:

MAIN CPU:
 - 6809P, 8 MHz xtal
 - NMI - main interrupt vector, currectly there's a hack (int vectors) to use standard irq/firq vectors
         Currently NMI writes error messages to the terminal(?) output ($814, $815)

VIDEO:
 - custom blitter - ROM (256x512, 1bpp) -> VRAM (256x256, 4bpp?, double buffered?)
 - blitter data (8 bytes long 'slots') stored in RAM, format:

   0 - src address (ROM)
   1 /
   2 - 256 - height
   3 - 32 - width (bytes)
   4 - flags
       76543210
       x------- end of list
       --x----- flip
       -?-????? unknown (unused?)
   5 - pen (0-15)
   6 - dsty
   7 - dstx

- there's a base (x,y) offset for each blit($7fa-$7fb writes), write to this address triggers the blitter
- blitter starts to copy gfx from the last accessed slot (even if ram is locked) and stops on the slot with 'end of list' bit set
- blitter control ($7f8):

  76543210
  x------- flip screen
  -x------ blitter ram lock
  ---x---- front/back buffer (?)
  --?-???? unknown

AUDIO:
- 6502 ? (empty socket)
- 4MHz xtal near the cpu socket
- IRQ - ?
- NMI - triggered by main cpu, sound latch read
- AY-3-8910
- PIT (8253) - used to sync the audio
- game works without the sound cpu

Differences from MAME:
 - The sound latch is implemented directly in the driver.
 - The 8253 output 1 and 2 are not connected.

*/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/pit8253.h"
#include "sound/ay8910.h"

#define LAYER_SIZE (256 * 256)

static UINT8 *blitter_slots;
static UINT8 *layer_1;
static UINT8 *layer_2;
static UINT8 blitter_base_x;
static UINT8 blitter_base_y;
static offs_t blitter_last_offset;
static UINT8 blitter_flags;
static UINT8 pit_output;
static UINT8 latch_data;
static UINT8 latch_pending;

/*************************************
 *
 *  Video
 *
 *************************************/

static PALETTE_INIT( vampire )
{
	int i;

	for (i = 0; i < 256; ++i)
	{
		int g = ((color_prom[i] & 0x03) >> 0) * 85;
		int r = ((color_prom[i] & 0x1c) >> 2) * 36;
		int b = ((color_prom[i] & 0xe0) >> 5) * 36;
		palette_set_color(i, r, g, b);
	}
}

static VIDEO_START( vampire )
{
	blitter_slots = auto_malloc(0x1000);
	layer_1 = auto_malloc(LAYER_SIZE);
	layer_2 = auto_malloc(LAYER_SIZE);
	memset(blitter_slots, 0, 0x1000);
	memset(layer_1, 0, LAYER_SIZE);
	memset(layer_2, 0, LAYER_SIZE);

	state_save_register_global_pointer(blitter_slots, 0x1000);
	state_save_register_global_pointer(layer_1, LAYER_SIZE);
	state_save_register_global_pointer(layer_2, LAYER_SIZE);
	state_save_register_global(blitter_base_x);
	state_save_register_global(blitter_base_y);
	state_save_register_global(blitter_last_offset);
	state_save_register_global(blitter_flags);
	return 0;
}

static VIDEO_UPDATE( vampire )
{
	const UINT8 *layer = (blitter_flags & 0x10) ? layer_1 : layer_2;
	int x, y;

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			plot_pixel(bitmap, x, y, Machine->pens[layer[256 * y + x]]);
}

static void blit_trigger(offs_t offset)
{
	const UINT8 *gfxrom = memory_region(REGION_GFX1);
	UINT8 *ptr = (blitter_flags & 0x10) ? layer_2 : layer_1;
	UINT32 index = offset >> 3;

	while (index < 0x200)
	{
		const UINT8 *slot = &blitter_slots[index << 3];
		UINT32 desty = slot[6];
		const UINT32 destx = slot[7];
		const UINT32 start_offset = ((slot[0] << 8) | slot[1]) << 3;
		const UINT8 pen = slot[5];
		const INT32 sy = 256 - slot[2];
		const INT32 sx = (32 - slot[3]) << 3;
		INT32 xx, yy;

		for (yy = 0; yy < sy; ++yy)
		{
			for (xx = 0; xx < sx; ++xx)
			{
				const UINT32 srcptr = start_offset + 256 * yy + xx;
				const UINT32 dstptr = (desty + blitter_base_y) * 256 + destx + xx + blitter_base_x;
				UINT8 pix = 0;

				if (desty + blitter_base_y > 255 || destx + xx + blitter_base_x > 255)
					continue;

				if (dstptr < LAYER_SIZE)
					pix = gfxrom[(srcptr >> 3) & 0x3fff] & (1 << (7 - (srcptr & 7)));

				/* a source address with the top bit set is a solid fill */
				if (slot[0] & 0x80)
					pix = 1;

				if (pix)
					ptr[dstptr] = pen;
			}
			desty += (slot[4] & 0x20) ? 1 : -1;
		}
		if (slot[4] & 0x80)
			break;

		++index;
	}
}

/*************************************
 *
 *  Main CPU
 *
 *************************************/

static WRITE8_HANDLER( blitter_flags_w )
{
	blitter_flags = data;
}

static WRITE8_HANDLER( blitter_control_w )
{
	if (!(offset & 1))
		blitter_base_y = data;
	else
	{
		blitter_base_x = data;
		blit_trigger(blitter_last_offset);
		cpunum_set_input_line(0, 0, HOLD_LINE);
	}
}

static WRITE8_HANDLER( soundlatch_main_w )
{
	latch_data = data;
	latch_pending = 1;
	cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE);
	/* the sound cpu must read the latch before the main cpu goes on */
	cpu_boost_interleave(0, TIME_IN_USEC(100));
}

static READ8_HANDLER( sound_ack_r )
{
	return latch_pending ? 0x00 : 0x80;
}

static READ8_HANDLER( io81x_r )
{
	return mame_rand();
}

static READ8_HANDLER( blitter_r )
{
	return blitter_slots[offset];
}

static WRITE8_HANDLER( blitter_w )
{
	blitter_last_offset = offset;

	/* the blitter ram lock still records the last accessed slot */
	if (blitter_flags & 0x40)
		return;

	blitter_slots[offset] = data;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x07f0, 0x07f0) AM_READ(input_port_2_r)
	AM_RANGE(0x07f1, 0x07f1) AM_READ(input_port_3_r)
	AM_RANGE(0x07f2, 0x07f2) AM_READ(input_port_0_r)
	AM_RANGE(0x07f3, 0x07f3) AM_READ(input_port_1_r)
	AM_RANGE(0x07f4, 0x07f4) AM_WRITE(soundlatch_main_w)
	AM_RANGE(0x07f5, 0x07f5) AM_READ(sound_ack_r)
	AM_RANGE(0x07f8, 0x07f8) AM_WRITE(blitter_flags_w)
	AM_RANGE(0x07fa, 0x07fb) AM_WRITE(blitter_control_w)
	AM_RANGE(0x0814, 0x0815) AM_READ(io81x_r)
	AM_RANGE(0x1000, 0x1fff) AM_READWRITE(blitter_r, blitter_w)
	AM_RANGE(0x2000, 0x3fff) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END

/*************************************
 *
 *  Audio CPU
 *
 *************************************/

static void pit_out_0(int state)
{
	pit_output = state;
}

static READ8_HANDLER( sound_latch_r )
{
	return latch_data;
}

static WRITE8_HANDLER( sound_ack_w )
{
	latch_pending = 0;
	cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE);
}

static READ8_HANDLER( sound_sync_r )
{
	return pit_output << 6;
}

static ADDRESS_MAP_START( audio_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM
	AM_RANGE(0x4000, 0x4000) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x6000, 0x6000) AM_READ(sound_latch_r)
	AM_RANGE(0x8000, 0x8000) AM_WRITE(sound_ack_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0xa001, 0xa001) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0xa008, 0xa00b) AM_READWRITE(pit8253_0_r, pit8253_0_w)
	AM_RANGE(0xa00c, 0xa00c) AM_READ(sound_sync_r)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( vampire )
	PORT_START_TAG("DSW1")
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 4C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x10, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x10, "5" )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Infinite Time (Cheat)" )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("DSW2")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Cocktail ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Upright ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x00, "Infinite Lives (Cheat)" )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x80, DEF_STR( On ) )

	PORT_START_TAG("IN1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START_TAG("IN2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
INPUT_PORTS_END

static const struct pit8253_config pit_config =
{
	TYPE8253,
	{
		{ 2000000.0, pit_out_0, 0 },
		{ 2000000.0, 0, 0 },
		{ 2000000.0, 0, 0 }
	}
};

static MACHINE_RESET( vampire )
{
	blitter_base_x = 0;
	blitter_base_y = 0;
	blitter_last_offset = 0;
	blitter_flags = 0xff;
	pit_output = 0;
	latch_data = 0;
	latch_pending = 0;
	pit8253_reset(0);
}

static INTERRUPT_GEN( vampire_interrupt )
{
	cpunum_set_input_line(0, M6809_FIRQ_LINE, HOLD_LINE);
}

static DRIVER_INIT( vampire )
{
	UINT8 *rom = memory_region(REGION_CPU1);

	/* the nmi is the main interrupt vector, use the standard irq/firq vectors instead */
	rom[0xfff6] = rom[0xffe0];
	rom[0xfff7] = rom[0xffe1];
	rom[0xfff8] = rom[0xffe2];
	rom[0xfff9] = rom[0xffe3];

	pit8253_init(1, &pit_config);
}

static MACHINE_DRIVER_START( vampire )
	MDRV_CPU_ADD(M6809, 8000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(vampire_interrupt, 1)

	MDRV_CPU_ADD(M6502, 2000000)
	MDRV_CPU_PROGRAM_MAP(audio_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(vampire)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 32 * 8 - 1, 2 * 8, 30 * 8 - 1)
	MDRV_PALETTE_LENGTH(256)
	MDRV_PALETTE_INIT(vampire)
	MDRV_VIDEO_START(vampire)
	MDRV_VIDEO_UPDATE(vampire)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 1000000)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

ROM_START( vampire )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "1h.1h", 0x8000, 0x2000, CRC(7e69ff9b) SHA1(85f6303803a0577f96b879e0b9d280a4f22cca8c) )
	ROM_LOAD( "2h.2h", 0xa000, 0x2000, CRC(e94155f8) SHA1(f60d82d9204f7b13aa9be189ca74ca48bf9e879a) )
	ROM_LOAD( "3h.3h", 0xc000, 0x2000, CRC(ce27dd90) SHA1(4edd8ba08f828a0c3c6eb21a2d9e0d1e53fac407) )
	ROM_LOAD( "4h.4h", 0xe000, 0x2000, CRC(a25f00bc) SHA1(db445c1876b79c4d553d4f9cc881f41f68b6667c) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "s_9d.9d", 0xf000, 0x1000, CRC(e13a7aef) SHA1(77a49cb0f3f037826a32bbfa8fab524f17895992) )

	ROM_REGION( 0x4000, REGION_GFX1, 0 )
	ROM_LOAD( "cg_p1.6e", 0x0000, 0x2000, CRC(042661a4) SHA1(58ed7c782a2486aa8c2c650d6d9e54929e5dd50b) )
	ROM_LOAD( "cg_p2.7e", 0x2000, 0x2000, CRC(e9dd9dff) SHA1(8a9ad8659763a9c010d0b482af3a40978c02cec7) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD_NIB_LOW(  "tb24s10.16a", 0x0000, 0x0100, CRC(bc60a2eb) SHA1(73b8f5f6eee90a4d59a5c62a6a342fa49185938c) )
	ROM_LOAD_NIB_HIGH( "tb24s10.16b", 0x0000, 0x0100, CRC(aa6b627b) SHA1(556d6bab46419ce55dd254de9615981852be6e6f) )
ROM_END

GAME( 1983, vampire, 0, vampire, vampire, vampire, ROT90, "Entertainment Enterprises, Ltd.", "Vampire (prototype?)", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_COLORS | GAME_IMPERFECT_SOUND )
