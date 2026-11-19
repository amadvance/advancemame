// license:BSD-3-Clause
// copyright-holders:Eugene Sandulenko
/*****************************************************************************

  TIA-MC1 driver

  driver by Eugene Sandulenko
  special thanks to Shiru for his standalone emulator and documentation

  Games supported:
      * Billiard
      * Konek-Gorbunok (Little Humpbacked Horse)
      * Snezhnaja Koroleva (Snow Queen)
      * S.O.S.
      * Gorodki
      * Kot-Rybolov

  Other games known to exist on this hardware (interchangeable by the ROM swap):
      * Avtogonki
      * Istrebitel'
      * Kotigoroshko
      * Ostrov Drakona
      * Ostrov Sokrovisch
      * Perekhvatchik
      * Zvezdnyi Rytsar'

 ***************************************************************

  During bootup hold F2 to enter test mode.
  Also use F2 to switch the screens during the gameplay (feature of the original
    machine)

 ***************************************************************

  This is one of the last USSR-made arcades. Several games are known to exist on
  this hardware. It was created by the state company Terminal (Vinnitsa, Ukraine),
  which was later turned into EXTREMA-Ukraine company. These days it manufactures
  various gambling machines (http://www.extrema-ua.com).

 ***************************************************************

  TIA-MC1 arcade internals

  This arcade machine contains four PCBs:

    BEIA-100
      Main CPU board. Also contains address PROM, color DAC, input ports and sound

    BEIA-101
      Video board 1. Background generator, video tiles RAM and video sync schematics

    BEIA-102
      Video board 2. Sprite generator, video buffer RAM (hardware triple buffering)

    BEIA-103
      ROM banks, RAM. Contains ROMs and multiplexors


  BEIA-103 PCB Layout

    |----------------------------------|
    |                                  |
  |--| g1.d17   RU8A                   |
  |  |                                 |
  |  |                                 |
  |  | g2.d17   RU8A   IR13   a2.b07   |
  |  |                                 |
  |  |                                 |
  |  | g3.d17   RU8A   IR13   a3.g07  |--|
  |  |                                |  |
  |--|                                |  |
    |  g4.d17   RU8A    kp11          |  |
    |                                 |  |
    |                                 |  |
    |  g5.d17          IR13   a5.l07  |  |
  |--|                                |  |
  |  |           id7                  |  |
  |  | g6.d17          IR13   a6.r07  |--|
  |  |           id7                   |
  |  |                                 |
  |  | g7.d17                          |
  |  |           ll1                   |
  |  |                                 |
  |--|  ap6      le1                   |
    |                                  |
    |----------------------------------|

  Notes:

   g1.d17 \
   g2.d17 |
   g3.d17 |
   g4.d17 |
   g5.d17 |
   g6.d17 +- EPROM K573RF4A (2764 analog)
   g7.d17 |
   a2.b07 |
   a3.g07 |
   a5.l07 |
   a6.r07 /

   RU8A - KR537RU8A SRAM 2k x8 (TC5516 analog)
   IR13 - K155IR13 register (74198 analog)
   kp11 - K555KP11 selector/multiplexor (74LS257 analog)
   id7  - K555ID7 (74LS138 analog)
   ap6  - K555AP6 (74LS245 analog)
   ll1  - K555LL1 (74LS32 analog)
   le1  - K555LE1 (74LS02 analog)

 ***************************************************************

   TODO:
     - Use machine/pit8253.c in sound
     - Check sprites priorities on the real hardware
     - Check vertical background scrolling on the real hardware
     - Kot Rybolov viewable area size likely controlled by pit8253 timers 0 and 1, used in test mode only

*/

/*

  Differences from MAME:
    The 8253 timers are emulated by a private implementation, also used by
    Kot-Rybolov for its sound timer. The timers used to blank the video of
    Kot-Rybolov are not emulated.

*/

#include "driver.h"
#include "machine/8255ppi.h"
#include "sound/custom.h"
#include "streams.h"

#define MASTER_CLOCK	15750000
#define CPU_CLOCK		(MASTER_CLOCK / 9)
#define SND_CLOCK		(MASTER_CLOCK / 9)
#define PIXEL_CLOCK		(MASTER_CLOCK / 3)

/*************************************
 *
 *  Video
 *
 *************************************/

static UINT8 video_ram[0x3050];
static UINT8 *tileram;
static UINT8 *charram;
static UINT8 *spriteram_x;
static UINT8 *spriteram_y;
static UINT8 *spriteram_a;
static UINT8 *spriteram_n;
static UINT8 *color_ram;
static UINT8 layers_ctrl;
static UINT8 bg_vshift;
static UINT8 bg_hshift;
static UINT8 bg_bplctrl;
static UINT8 palette_rgb[256][3];

static tilemap *bg_tilemap1, *bg_tilemap2;

static void set_palette_entry(int pen, int data)
{
	palette_set_color(pen, palette_rgb[data][0], palette_rgb[data][1], palette_rgb[data][2]);
}

static void update_bg_palette(void)
{
	int i;
	int bplmask = ((bg_bplctrl >> 0) & 1) | ((bg_bplctrl >> 1) & 2) | ((bg_bplctrl >> 2) & 4) | ((bg_bplctrl >> 3) & 8);

	for (i = 0; i < 16; ++i)
		set_palette_entry(i + 16, color_ram[i | bplmask]);
}

static WRITE8_HANDLER( tiamc1_videoram_w )
{
	/* the four bit planes of the character RAM are written together, each one if enabled */
	int plane;
	int changed = 0;

	for (plane = 0; plane < 4; ++plane)
	{
		if (!(layers_ctrl & (2 << plane)))
		{
			UINT8 *p = &charram[offset + plane * 0x800];

			if (*p != data)
			{
				*p = data;
				changed = 1;
			}
		}
	}

	if (changed)
	{
		decodechar(Machine->gfx[0], (offset / 8) & 0xff, charram, Machine->drv->gfxdecodeinfo[0].gfxlayout);
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);
	}

	if (!(layers_ctrl & 1))
	{
		tileram[offset] = data;
		if (offset < 1024)
			tilemap_mark_tile_dirty(bg_tilemap1, offset);
		else
			tilemap_mark_tile_dirty(bg_tilemap2, offset & 0x3ff);
	}
}

static WRITE8_HANDLER( kot_videoram_w )
{
	if (!(layers_ctrl & 1))
	{
		tileram[offset] = data;
		tilemap_mark_tile_dirty(bg_tilemap1, offset);
	}
}

static WRITE8_HANDLER( tiamc1_bankswitch_w )
{
	if ((data & 128) != (layers_ctrl & 128))
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	layers_ctrl = data;
}

static WRITE8_HANDLER( kot_bankswitch_w )
{
	/* selects the 256 characters window inside the 1024 characters of the ROM */
	if ((data >> 1) != (layers_ctrl >> 1))
		tilemap_mark_all_tiles_dirty(ALL_TILEMAPS);

	layers_ctrl = data;
}

static WRITE8_HANDLER( tiamc1_sprite_x_w )
{
	spriteram_x[offset] = data;
}

static WRITE8_HANDLER( tiamc1_sprite_y_w )
{
	spriteram_y[offset] = data;
}

static WRITE8_HANDLER( tiamc1_sprite_a_w )
{
	spriteram_a[offset] = data;
}

static WRITE8_HANDLER( tiamc1_sprite_n_w )
{
	spriteram_n[offset] = data;
}

static WRITE8_HANDLER( tiamc1_bg_vshift_w )
{
	bg_vshift = data;
}

static WRITE8_HANDLER( tiamc1_bg_hshift_w )
{
	bg_hshift = data;
}

static WRITE8_HANDLER( tiamc1_bg_bplctrl_w )
{
	bg_bplctrl = data;
	update_bg_palette();
}

static WRITE8_HANDLER( tiamc1_palette_w )
{
	color_ram[offset] = data;
	set_palette_entry(offset, data);
	update_bg_palette();
}

static void init_palette_rgb(void)
{
	/* voltage adjusted by Shiru */
	static const float g_v[8] = { 1.2071f, 0.9971f, 0.9259f, 0.7159f, 0.4912f, 0.2812f, 0.2100f, 0.0000f };
	static const float r_v[8] = { 1.5937f, 1.3125f, 1.1562f, 0.8750f, 0.7187f, 0.4375f, 0.2812f, 0.0000f };
	static const float b_v[4] = { 1.3523f, 0.8750f, 0.4773f, 0.0000f };
	int col;

	for (col = 0; col < 256; ++col)
	{
		int ir = (col >> 3) & 7;
		int ig = col & 7;
		int ib = (col >> 6) & 3;

		palette_rgb[col][0] = 255 - (((int)(255.0f * r_v[ir] / r_v[0])) & 255);
		palette_rgb[col][1] = 255 - (((int)(255.0f * g_v[ig] / g_v[0])) & 255);
		palette_rgb[col][2] = 255 - (((int)(255.0f * b_v[ib] / b_v[0])) & 255);
	}
}

static void get_bg1_tile_info(int tile_index)
{
	SET_TILE_INFO(0, tileram[tile_index], 0, 0)
}

static void get_bg2_tile_info(int tile_index)
{
	SET_TILE_INFO(0, tileram[tile_index + 1024], 0, 0)
}

static void get_kot_tile_info(int tile_index)
{
	SET_TILE_INFO(0, tileram[tile_index] + (layers_ctrl >> 1) * 32, 0, 0)
}

static void init_video_state(void)
{
	init_palette_rgb();
	memset(video_ram, 0, sizeof(video_ram));
	bg_vshift = 0;
	bg_hshift = 0;
	bg_bplctrl = 0;
	layers_ctrl = 0;
}

static VIDEO_START( tiamc1 )
{
	init_video_state();

	/* the character RAM is banked */
	charram = video_ram + 0x0800;
	tileram = video_ram + 0x0000;
	spriteram_y = video_ram + 0x3000;
	spriteram_x = video_ram + 0x3010;
	spriteram_n = video_ram + 0x3020;
	spriteram_a = video_ram + 0x3030;
	color_ram = video_ram + 0x3040;

	bg_tilemap1 = tilemap_create(get_bg1_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);
	bg_tilemap2 = tilemap_create(get_bg2_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap1 || !bg_tilemap2)
		return 1;

	tilemap_set_scrolldx(bg_tilemap1, 4, 4);
	tilemap_set_scrolldx(bg_tilemap2, 4, 4);

	return 0;
}

static VIDEO_START( kot )
{
	init_video_state();

	tileram = video_ram + 0x000;
	spriteram_y = video_ram + 0x400;
	spriteram_x = video_ram + 0x410;
	spriteram_n = video_ram + 0x420;
	spriteram_a = video_ram + 0x430;
	color_ram = video_ram + 0x440;

	bg_tilemap1 = tilemap_create(get_kot_tile_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 32, 32);

	if (!bg_tilemap1)
		return 1;

	tilemap_set_scrolldx(bg_tilemap1, 4, 4);

	return 0;
}

static void draw_sprites(mame_bitmap *bitmap, const rectangle *cliprect)
{
	int offs;

	for (offs = 0; offs < 16; ++offs)
	{
		int flipx = !(spriteram_a[offs] & 0x08);
		int flipy = !(spriteram_a[offs] & 0x02);
		int sx = spriteram_x[offs] ^ 0xff;
		int sy = spriteram_y[offs] ^ 0xff;
		int code = spriteram_n[offs] ^ 0xff;

		if (!(spriteram_a[offs] & 0x01))
			drawgfx(bitmap, Machine->gfx[1], code, 0, flipx, flipy, sx, sy, cliprect, TRANSPARENCY_PEN, 15);
	}
}

static VIDEO_UPDATE( tiamc1 )
{
	tilemap *bg = (layers_ctrl & 0x80) ? bg_tilemap2 : bg_tilemap1;

	tilemap_set_scrolly(bg, 0, bg_vshift);
	tilemap_set_scrollx(bg, 0, bg_hshift);
	tilemap_draw(bitmap, cliprect, bg, 0, 0);

	draw_sprites(bitmap, cliprect);
}

static VIDEO_UPDATE( kot )
{
	tilemap_set_scrolly(bg_tilemap1, 0, bg_vshift);
	tilemap_set_scrollx(bg_tilemap1, 0, bg_hshift);
	tilemap_draw(bitmap, cliprect, bg_tilemap1, 0, 0);

	draw_sprites(bitmap, cliprect);
}

/*************************************
 *
 *  Sound
 *
 *            timer1       timer0
 *          |--------|   |--------|
 *          |  8253  |   |  8253  |   |---|
 *      in0 |        |   |        |   | 1 |   |---|
 *      ----+ g0  o0 +---+ g0  o0 +---+   |   | & |
 *      in1 |        |   |        |   |   o---+   |  out
 *      ----+ g1  o1 +---+ g1  o1 +---+   |   |   +-----
 *      in2 |        |   |        |   |---|   |   |
 *      ----+ g2  o2 +---+ g2  o2 +-----------+   |
 *      clk1|        |   |        |           |---|
 *      ----+ clk    | +-+ clk    |
 *          |        | | |        |
 *          |--------| | |--------|
 *      clk0           |
 *      ---------------+
 *
 *  in0-in2 comes from port #da
 *  clk0    comes from 8224 and equals to 1777777Hz, i.e. processor clock
 *  clk1    comes from divider 16000000/4/16/16/2 = 7812Hz
 *
 *************************************/

#define CLOCK_DIVIDER 16

typedef struct
{
	UINT16 count;
	UINT16 cnval;
	UINT8 bcdMode;
	UINT8 cntMode;
	UINT8 valMode;
	UINT8 gate;
	UINT8 output;
	UINT8 loadCnt;
	UINT8 enable;
} timer8253chan;

typedef struct
{
	timer8253chan channel[3];
} timer8253;

#define T8253_CWORD 3

static sound_stream *sound_channel;
static int timer1_divider;
static timer8253 timer0;
static timer8253 timer1;

static void timer8253_tick(timer8253 *t, int chn)
{
	timer8253chan *c = &t->channel[chn];

	if (c->enable && c->gate)
	{
		switch (c->cntMode)
		{
		case 0:
			--c->count;
			if (c->count == 0xffff)
				c->output = 1;
			break;

		case 2:
			--c->count;
			if (c->count == 1)
				c->output = 0;
			else if (c->count == 0)
			{
				c->output = 1;
				c->count = c->cnval;
			}
			break;

		case 3:
			--c->count;

			if (c->count < (c->cnval >> 1))
				c->output = 0;
			else
				c->output = 1;

			if (c->count == 0xffff)
				c->count = c->cnval;
			break;

		case 4:
			--c->count;
			if (c->count == 0)
				c->output = 1;

			if (c->count == 0xffff)
			{
				c->enable = 0;
				c->output = 1;
			}
			break;
		}
	}
}

static void timer8253_wr(timer8253 *t, int reg, UINT8 val)
{
	int chn;
	timer8253chan *c;

	if (reg == T8253_CWORD)
	{
		chn = val >> 6;
		if (chn < 3)
		{
			c = &t->channel[chn];
			c->bcdMode = (val & 1) ? 1 : 0;
			c->cntMode = (val >> 1) & 0x07;
			c->valMode = (val >> 4) & 0x03;

			switch (c->valMode)
			{
			case 1:
			case 2:
				c->loadCnt = 1;
				break;

			case 3:
				c->loadCnt = 2;
				break;

			default:
				logerror("unhandled val mode %i\n", c->valMode);
			}

			switch (c->cntMode)
			{
			case 0:
				c->output = 0;
				c->enable = 0;
				break;

			case 2:
			case 3:
				c->output = 1;
				break;

			case 4:
				c->output = 1;
				c->enable = 0;
				break;

			default:
				logerror("unhandled cnt mode %i\n", c->cntMode);
			}
		}
	}
	else
	{
		c = &t->channel[reg];

		switch (c->valMode)
		{
		case 1:
			c->cnval = (c->cnval & 0xff00) | val;
			break;
		case 2:
			c->cnval = (c->cnval & 0x00ff) | (val << 8);
			break;
		case 3:
			c->cnval = (c->cnval >> 8) | (val << 8);
			break;
		default:
			logerror("unhandled val mode %i\n", c->valMode);
		}

		if (c->cntMode == 0)
			c->enable = 0;

		--c->loadCnt;

		if (c->loadCnt == 0)
		{
			switch (c->valMode)
			{
			case 1:
			case 2:
				c->loadCnt = 1;
				break;

			case 3:
				c->loadCnt = 2;
				break;

			default:
				logerror("unhandled val mode %i\n", c->valMode);
			}

			switch (c->cntMode)
			{
			case 0:
			case 2:
			case 3:
			case 4:
				c->count = c->cnval;
				c->enable = 1;
				break;

			default:
				logerror("unhandled cnt mode %i\n", c->cntMode);
			}
		}
	}
}

static WRITE8_HANDLER( tiamc1_timer0_w )
{
	stream_update(sound_channel, 0);
	timer8253_wr(&timer0, offset, data);
}

static WRITE8_HANDLER( tiamc1_timer1_w )
{
	stream_update(sound_channel, 0);
	timer8253_wr(&timer1, offset, data);
}

static WRITE8_HANDLER( tiamc1_timer1_gate_w )
{
	stream_update(sound_channel, 0);
	timer1.channel[0].gate = (data & 1) ? 1 : 0;
	timer1.channel[1].gate = (data & 2) ? 1 : 0;
	timer1.channel[2].gate = (data & 4) ? 1 : 0;
}

static void tiamc1_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	int count;
	int len = length * CLOCK_DIVIDER;
	int orval = 0;

	for (count = 0; count < len; ++count)
	{
		int o0, o1, o2;

		++timer1_divider;
		if (timer1_divider == 228)
		{
			int i;

			timer1_divider = 0;
			for (i = 0; i < 3; ++i)
			{
				timer8253_tick(&timer1, i);
				timer0.channel[i].gate = timer1.channel[i].output;
			}
		}

		timer8253_tick(&timer0, 0);
		timer8253_tick(&timer0, 1);
		timer8253_tick(&timer0, 2);

		o0 = timer0.channel[0].output ? 1 : 0;
		o1 = timer0.channel[1].output ? 1 : 0;
		o2 = timer0.channel[2].output ? 1 : 0;

		orval = (orval << 1) | (((o0 | o1) ^ 0xff) & o2);

		if ((count + 1) % CLOCK_DIVIDER == 0)
		{
			outputs[0][count / CLOCK_DIVIDER] = orval ? 0x2828 : 0;
			orval = 0;
		}
	}
}

/* the single 8253 of Kot-Rybolov drives the speaker with the counter 2 only */
static void kot_sound_update(void *param, stream_sample_t **inputs, stream_sample_t **outputs, int length)
{
	int count;
	int len = length * CLOCK_DIVIDER;
	int orval = 0;

	for (count = 0; count < len; ++count)
	{
		timer8253_tick(&timer0, 2);

		orval = (orval << 1) | (timer0.channel[2].output ? 1 : 0);

		if ((count + 1) % CLOCK_DIVIDER == 0)
		{
			outputs[0][count / CLOCK_DIVIDER] = orval ? 0x2828 : 0;
			orval = 0;
		}
	}
}

static void *tiamc1_sh_start(int clock, const struct CustomSound_interface *config)
{
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));
	timer1_divider = 0;

	sound_channel = stream_create(0, 1, clock / CLOCK_DIVIDER, 0, tiamc1_sound_update);
	return sound_channel;
}

static void *kot_sh_start(int clock, const struct CustomSound_interface *config)
{
	memset(&timer0, 0, sizeof(timer0));
	memset(&timer1, 0, sizeof(timer1));

	/* the gate of the counter 2 is always enabled */
	timer0.channel[2].gate = 1;

	sound_channel = stream_create(0, 1, clock / CLOCK_DIVIDER, 0, kot_sound_update);
	return sound_channel;
}

static const struct CustomSound_interface tiamc1_sound_interface =
{
	tiamc1_sh_start
};

static const struct CustomSound_interface kot_sound_interface =
{
	kot_sh_start
};

/*************************************
 *
 *  Machine
 *
 *************************************/

static WRITE8_HANDLER( tiamc1_control_w )
{
	coin_lockout_w(0, ~data & 0x02);
	coin_counter_w(0, data & 0x04);
}

static ppi8255_interface ppi8255_intf =
{
	1,
	{ input_port_0_r },
	{ input_port_1_r },
	{ input_port_2_r },
	{ NULL },
	{ NULL },
	{ tiamc1_control_w }
};

static MACHINE_RESET( tiamc1 )
{
	ppi8255_init(&ppi8255_intf);
	tiamc1_bankswitch_w(0, 0);
}

static ADDRESS_MAP_START( tiamc1_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xdfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xe000, 0xffff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xb000, 0xb7ff) AM_WRITE(tiamc1_videoram_w)
	AM_RANGE(0xe000, 0xffff) AM_WRITE(MWA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kot_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xcfff) AM_READ(MRA8_RAM)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kot_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0xc000, 0xcfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(kot_videoram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xd0, 0xd3) AM_READ(ppi8255_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( tiamc1_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x40, 0x4f) AM_WRITE(tiamc1_sprite_y_w)	/* sprites Y */
	AM_RANGE(0x50, 0x5f) AM_WRITE(tiamc1_sprite_x_w)	/* sprites X */
	AM_RANGE(0x60, 0x6f) AM_WRITE(tiamc1_sprite_n_w)	/* sprites # */
	AM_RANGE(0x70, 0x7f) AM_WRITE(tiamc1_sprite_a_w)	/* sprites attributes */
	AM_RANGE(0xa0, 0xaf) AM_WRITE(tiamc1_palette_w)		/* color ram */
	AM_RANGE(0xbc, 0xbc) AM_WRITE(tiamc1_bg_vshift_w)	/* background V scroll */
	AM_RANGE(0xbd, 0xbd) AM_WRITE(tiamc1_bg_hshift_w)	/* background H scroll */
	AM_RANGE(0xbe, 0xbe) AM_WRITE(tiamc1_bankswitch_w)	/* VRAM selector */
	AM_RANGE(0xbf, 0xbf) AM_WRITE(tiamc1_bg_bplctrl_w)	/* charset control */
	AM_RANGE(0xc0, 0xc3) AM_WRITE(tiamc1_timer0_w)		/* timer 0 */
	AM_RANGE(0xd0, 0xd3) AM_WRITE(ppi8255_0_w)			/* input ports + coin counters & lockout */
	AM_RANGE(0xd4, 0xd7) AM_WRITE(tiamc1_timer1_w)		/* timer 1 */
	AM_RANGE(0xda, 0xda) AM_WRITE(tiamc1_timer1_gate_w)	/* timer 1 gate control */
ADDRESS_MAP_END

static ADDRESS_MAP_START( kot_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0xd0, 0xd3) AM_READ(ppi8255_0_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( kot_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x0f) AM_WRITE(tiamc1_sprite_y_w)	/* sprites Y */
	AM_RANGE(0x10, 0x1f) AM_WRITE(tiamc1_sprite_x_w)	/* sprites X */
	AM_RANGE(0x20, 0x2f) AM_WRITE(tiamc1_sprite_n_w)	/* sprites # */
	AM_RANGE(0x30, 0x3f) AM_WRITE(tiamc1_sprite_a_w)	/* sprites attributes */
	AM_RANGE(0xc0, 0xc3) AM_WRITE(tiamc1_timer0_w)		/* timer */
	AM_RANGE(0xd0, 0xd3) AM_WRITE(ppi8255_0_w)			/* input ports + coin counters & lockout */
	AM_RANGE(0xe0, 0xef) AM_WRITE(tiamc1_palette_w)		/* color ram */
	AM_RANGE(0xf0, 0xf0) AM_WRITE(tiamc1_bg_vshift_w)	/* background V scroll */
	AM_RANGE(0xf4, 0xf4) AM_WRITE(tiamc1_bg_hshift_w)	/* background H scroll */
	AM_RANGE(0xf8, 0xf8) AM_WRITE(kot_bankswitch_w)		/* character rom offset */
ADDRESS_MAP_END

INPUT_PORTS_START( tiamc1 )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 0 JOYSTICK_RIGHT */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 2 JOYSTICK_RIGHT */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 3 JOYSTICK_RIGHT */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 0 JOYSTICK_LEFT */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 2 JOYSTICK_LEFT */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 3 JOYSTICK_LEFT */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 0 JOYSTICK_UP */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 2 JOYSTICK_UP */
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 3 JOYSTICK_UP */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 0 JOYSTICK_DOWN */
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )	/* Player 2 JOYSTICK_DOWN */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* OUT:game counter */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* RAZR ??? */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)	/* Kick */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)	/* Jump */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( gorodki )
	PORT_START	/* IN0 */
	PORT_BIT( 0xff, 152, IPT_AD_STICK_X ) PORT_MINMAX(96,208) PORT_SENSITIVITY(20) PORT_KEYDELTA(10) PORT_REVERSE PORT_PLAYER(1)

	PORT_START	/* IN1 */
	PORT_BIT( 0x7f, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_SERVICE )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* OUT:game counter */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* RAZR ??? */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(1)	/* right button */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)	/* left button */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

INPUT_PORTS_START( kot )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 0 JOYSTICK_RIGHT */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 2 JOYSTICK_RIGHT */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 3 JOYSTICK_RIGHT */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 0 JOYSTICK_LEFT */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 2 JOYSTICK_LEFT */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 3 JOYSTICK_LEFT */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 0 JOYSTICK_UP */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 2 JOYSTICK_UP */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 3 JOYSTICK_UP */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 0 JOYSTICK_DOWN */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )	/* Player 2 JOYSTICK_DOWN */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SPECIAL )	/* OUT:coin lockout */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_SPECIAL )	/* OUT:game counter */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* RAZR ??? */
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)	/* Punch / Right */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)	/* Kick / Left */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_VBLANK )
INPUT_PORTS_END

static const gfx_layout sprites16x16_layout =
{
	16,16,
	256,
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 256*16*8+0, 256*16*8+1, 256*16*8+2, 256*16*8+3, 256*16*8+4, 256*16*8+5, 256*16*8+6, 256*16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	16*8
};

static const gfx_layout char_layout =
{
	8,8,
	256,
	4,
	{ 256*8*8*3, 256*8*8*2, 256*8*8*1, 256*8*8*0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_layout char_rom_layout =
{
	8,8,
	1024,
	4,
	{ 1024*8*8*3, 1024*8*8*2, 1024*8*8*1, 1024*8*8*0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

/* the characters are decoded at runtime from the video RAM */
static const gfx_decode tiamc1_gfxdecodeinfo[] =
{
	{ 0,           0, &char_layout,         16, 1 },
	{ REGION_GFX1, 0, &sprites16x16_layout,  0, 1 },
	{ -1 }
};

static const gfx_decode kot_gfxdecodeinfo[] =
{
	{ REGION_GFX2, 0, &char_rom_layout,     16, 1 },
	{ REGION_GFX1, 0, &sprites16x16_layout,  0, 1 },
	{ -1 }
};

static MACHINE_DRIVER_START( tiamc1 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", 8080, CPU_CLOCK)
	MDRV_CPU_PROGRAM_MAP(tiamc1_readmem, tiamc1_writemem)
	MDRV_CPU_IO_MAP(tiamc1_readport, tiamc1_writeport)

	MDRV_FRAMES_PER_SECOND(PIXEL_CLOCK / (336.0 * 312))
	MDRV_VBLANK_DURATION(3590)
	MDRV_MACHINE_RESET(tiamc1)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 255, 0, 255)
	MDRV_GFXDECODE(tiamc1_gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(32)
	MDRV_VIDEO_START(tiamc1)
	MDRV_VIDEO_UPDATE(tiamc1)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD_TAG("2x8253", CUSTOM, SND_CLOCK)
	MDRV_SOUND_CONFIG(tiamc1_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( kot )
	MDRV_IMPORT_FROM(tiamc1)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(kot_readmem, kot_writemem)
	MDRV_CPU_IO_MAP(kot_readport, kot_writeport)

	MDRV_GFXDECODE(kot_gfxdecodeinfo)
	MDRV_VIDEO_START(kot)
	MDRV_VIDEO_UPDATE(kot)

	MDRV_SOUND_REPLACE("2x8253", CUSTOM, SND_CLOCK)
	MDRV_SOUND_CONFIG(kot_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( konek )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "g1.d17", 0x00000, 0x2000, CRC(f41d82c9) SHA1(63ac1be2ad58af0e5ef2d33e5c8d790769d80af9) )
	ROM_LOAD( "g2.d17", 0x02000, 0x2000, CRC(b44e7491) SHA1(ff4cb1d76a36f504d670a207ee25556c5faad435) )
	ROM_LOAD( "g3.d17", 0x04000, 0x2000, CRC(91301282) SHA1(cb448a1bb7a9c1768f870a8c062e37807431c9c7) )
	ROM_LOAD( "g4.d17", 0x06000, 0x2000, CRC(3ff0c20b) SHA1(3d999c05b3986149e569630779ed5581fc202842) )
	ROM_LOAD( "g5.d17", 0x08000, 0x2000, CRC(e3196d30) SHA1(a03d9f75926be9fcf5ee05df8b00fbf87361ea5b) )
	ROM_FILL( 0xa000, 0x2000, 0x00 ) /* g6.d17 is unpopulated */
	ROM_LOAD( "g7.d17", 0x0c000, 0x2000, CRC(fe4e9fdd) SHA1(2033585a6c53455d1dafee85cbb807d424ed231d) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "a2.b07", 0x00000, 0x2000, CRC(9eed06ee) SHA1(1b64a3f8fe3df4b4870315dbdf69bf60b1c272d0) )
	ROM_LOAD( "a3.g07", 0x02000, 0x2000, CRC(eeff9b77) SHA1(5dc66292a59f24277a8c2f38158a2e1d58f81338) )
	ROM_LOAD( "a5.l07", 0x04000, 0x2000, CRC(fff9e089) SHA1(f0d64dceaf72da785d55316bf8a7433faa09fabb) )
	ROM_LOAD( "a6.r07", 0x06000, 0x2000, CRC(092e8ee2) SHA1(6c4842e992c592b9f0663e039668f61a7b56700f) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */
ROM_END

ROM_START( sosterm )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "04.1g", 0x00000, 0x2000, CRC(d588081e) SHA1(5dd9f889e932ee356f8e511b22b424eaeb502ef9) )
	ROM_LOAD( "05.2g", 0x02000, 0x2000, CRC(b44e7491) SHA1(ff4cb1d76a36f504d670a207ee25556c5faad435) )
	ROM_LOAD( "06.3g", 0x04000, 0x2000, CRC(34dacde6) SHA1(6c91e4dc1d3c85768a94fb4c7d38f29c23664740) )
	ROM_LOAD( "07.4g", 0x06000, 0x2000, CRC(9f6f8cdd) SHA1(3fa3928935d98906fdf07ed372764456d7a9729a) )
	ROM_LOAD( "08.5g", 0x08000, 0x2000, CRC(25e70da4) SHA1(ec77b0b79c0477c0939022d7f2a24ae48e4530bf) )
	ROM_FILL( 0xa000, 0x2000, 0x00 ) /* 09.6g is unpopulated */
	ROM_LOAD( "10.7g", 0x0c000, 0x2000, CRC(22bc9997) SHA1(fd638529e29d9fd32dd22534cb748841dde9a2c3) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "00.2a", 0x00000, 0x2000, CRC(a1c7f07a) SHA1(2ae702258be48ba70c126bfe94fbeec3353fc75a) )
	ROM_LOAD( "01.3a", 0x02000, 0x2000, CRC(788b4036) SHA1(a0020ae1720cc2e5a6db0f8fe9350de43246f552) )
	ROM_LOAD( "02.5a", 0x04000, 0x2000, CRC(9506cf9b) SHA1(3e54593d4452b956509877d9b6b26aa3e3a90beb) )
	ROM_LOAD( "03.6a", 0x06000, 0x2000, CRC(5a0c14e1) SHA1(3eebe2c3ce114b87723fa6571623ee065a0b5646) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */
ROM_END

ROM_START( koroleva )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "04.1g", 0x00000, 0x2000, CRC(c3701225) SHA1(ac059116521b06cb5347741d3ee2778c2e68a74e) )
	ROM_LOAD( "05.2g", 0x02000, 0x2000, CRC(1b3742ce) SHA1(908ad9eb0e79baac53eed195355d0d1bdf6b5a25) )
	ROM_LOAD( "06.3g", 0x04000, 0x2000, CRC(48074786) SHA1(145749053cd00c8547024c9afe3ab0ff7d8f5ff9) )
	ROM_LOAD( "07.4g", 0x06000, 0x2000, CRC(41a4adb5) SHA1(cdbdf6884307dd0f1fc991e6e1bc4c4fdc351ab1) )
	ROM_LOAD( "08.5g", 0x08000, 0x2000, CRC(8f379d95) SHA1(0ea70bc14c52b1f4b38b0d14e4249252a2577f2a) )
	ROM_FILL( 0xa000, 0x2000, 0x00 ) /* 09.6g is unpopulated */
	ROM_LOAD( "10.7g", 0x0c000, 0x2000, CRC(397f41f8) SHA1(2d07462afad39dda067114ce8d47e64d6a854283) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "00.2a", 0x00000, 0x2000, CRC(6f39f8be) SHA1(2b20cdab7064851c552d92d5bc9084df854eafd1) )
	ROM_LOAD( "01.3a", 0x02000, 0x2000, CRC(7bdfdd19) SHA1(8b971689050f9d608225226eb5cadbb4050c7d1f) )
	ROM_LOAD( "02.5a", 0x04000, 0x2000, CRC(97770b0f) SHA1(cf4605e31f8c57a76bfda6a7ea329058da8b8c9c) )
	ROM_LOAD( "03.6a", 0x06000, 0x2000, CRC(9b0a686a) SHA1(f02910db9f862ec017bb3834c58e96e780fb6322) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */
ROM_END

/* this game ROM board have only one 2KB main CPU RAM IC populated */
ROM_START( bilyard )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "04.1g", 0x00000, 0x2000, CRC(a44f913d) SHA1(f01a0e931fb0f719bc7b3f1ca7802dd747c8a56f) )
	ROM_LOAD( "05.2g", 0x02000, 0x2000, CRC(6e41219f) SHA1(b09a16c9bd48b503ec0f2c636f021199d7ac7924) )
	ROM_FILL( 0x04000, 0x2000, 0x00 ) /* 06.3g is unpopulated */
	ROM_FILL( 0x06000, 0x2000, 0x00 ) /* 07.4g is unpopulated */
	ROM_FILL( 0x08000, 0x2000, 0x00 ) /* 08.5g is unpopulated */
	ROM_FILL( 0x0a000, 0x2000, 0x00 ) /* 09.6g is unpopulated */
	ROM_LOAD( "10.7g", 0x0c000, 0x2000, CRC(173adb85) SHA1(53f27b45e61365907e8996c283ae70ca5b498129) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "00.2a", 0x00000, 0x2000, CRC(6f72e043) SHA1(a60f66326ff0a55e2624231efcbceff700d9ceee) )
	ROM_LOAD( "01.3a", 0x02000, 0x2000, CRC(daddbbb5) SHA1(1460aebcbb57180e05930845703ff6325d85702a) )
	ROM_LOAD( "02.5a", 0x04000, 0x2000, CRC(3d744d33) SHA1(f1375098e81986715d0497b09df0c6622bd75b9a) )
	ROM_LOAD( "03.6a", 0x06000, 0x2000, CRC(8bfc0b15) SHA1(221efdce516274d3b1d9009d11dc9ed6cd67ef12) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */
ROM_END

/* this game ROM board have only one 2KB main CPU RAM IC populated */
ROM_START( gorodki )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "70.1g", 0x00000, 0x2000, CRC(bd3eb624) SHA1(acfc7e7186daf399f858a0d0cf462b0eaabb3f9e) )
	ROM_LOAD( "71.2g", 0x02000, 0x2000, CRC(5a9ebd8d) SHA1(4a6682a4bd2eb3c852c1383b0564fe491e7af30a) )
	ROM_LOAD( "72.3g", 0x04000, 0x2000, CRC(edcc5c13) SHA1(6bebb4c28758a3b3c45318201eb13e5d81db7521) )
	ROM_LOAD( "73.4g", 0x06000, 0x2000, CRC(ae69b9f3) SHA1(08882fbf917ba17e95b27ba21db666e7832c1894) )
	ROM_FILL( 0x08000, 0x2000, 0x00 ) /* 08.5g is unpopulated */
	ROM_FILL( 0x0a000, 0x2000, 0x00 ) /* 09.6g is unpopulated */
	ROM_FILL( 0x0c000, 0x2000, 0x00 ) /* 10.7g is unpopulated */

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "66.2a", 0x00000, 0x2000, CRC(b3dd4dec) SHA1(2e399fca4ff0b98724f26a27b1ea8450d650cfb4) )
	ROM_LOAD( "67.3a", 0x02000, 0x2000, CRC(c94f5579) SHA1(757f063c857c81478925b1ae169de3c81b3533d4) )
	ROM_LOAD( "68.5a", 0x04000, 0x2000, CRC(0d64708d) SHA1(6a84b4293f0e983424ef361ab3ebf62ab5f8b21c) )
	ROM_LOAD( "69.6a", 0x06000, 0x2000, CRC(57c8ae81) SHA1(c73bbfaa53195a19599dd2bbc3948c819597b035) )

	ROM_REGION( 0x0400, REGION_PROMS, 0 )
	ROM_LOAD( "prom100.e10", 0x0000, 0x100, NO_DUMP ) /* i/o ports map 256x8 */
	ROM_LOAD( "prom101.a01", 0x0100, 0x100, NO_DUMP ) /* video sync 256x8 */
	ROM_LOAD( "prom102.b03", 0x0200, 0x080, NO_DUMP ) /* sprites rom index 256x4 */
	ROM_LOAD( "prom102.b06", 0x0280, 0x080, NO_DUMP ) /* buffer optimization logic 256x4 */
	ROM_LOAD( "prom102.b05", 0x0300, 0x100, NO_DUMP ) /* sprites rom index 256x8 */
ROM_END

/*
  later (cost reduced ?) version of TIA-MC1 hardware, uses single PCB main board design,
  ROM-based tile generator, sinle bank tile map RAM, single i8253

  Specs:
  1x KR580VM80A (Russian: KP580BM80A. Soviet clone of i8080).
  1x KR580VV55A (Russian: KP580BB55A. Soviet clone of i8255).
  1x KR580VI53 (Soviet clone of i8253)
  1x KR580VK28 (Russian: KP580BK28. Soviet clone of i8228: bus controllers/drivers).

  1x K174YH7 (soviet clone of TBA810, 7W audio amplifier).

  3x KR573RU8 (2Kx8 SRAM).
  2x KR541RU2 (1Kx4 RAM).
  9x KR531RU8 (16x4 RAM).

  There are some issues with the KR580VV55A.
  Sometimes the code tries to write through a port that was
  previously set as input. Not sure if it's a leftover, a bug,
  or just some differences against the i8255. Needs more research.

  KR580VI53/i8253 timers usage:
  0 - used to blank part of display in horizontal direction, apparently it's CLK0 is PIXEL_CLOCK/4, GATE0 is HBlank, OUT0 - blank video
  1 - used to blank part of display in vertical direction, apparently it's CLK1 is HBlank or OUT0, GATE1 is VBlank, OUT1 - blank video
  2 - sound generation

*/
ROM_START( kot )
	ROM_REGION( 0xc000, REGION_CPU1, ROMREGION_ERASE00 )
	ROM_LOAD( "854.6", 0x00000, 0x2000, CRC(44e5e8fc) SHA1(dafbace689f3834d5c6e952a2f6188fb190845e4) )
	ROM_LOAD( "855.7", 0x02000, 0x2000, CRC(0bb2e4b2) SHA1(7bbb45b18e3b444e3b6006a4670453ec0792e5d3) )
	ROM_LOAD( "856.8", 0x04000, 0x2000, CRC(9180c98f) SHA1(4085180b9e9772e965c487e7b02d88fcae973e87) )

	ROM_REGION( 0x8000, REGION_GFX1, 0 )
	ROM_LOAD( "850.5", 0x00000, 0x2000, CRC(5dc3a102) SHA1(e97d219f7004291438b991435b7fe5d5be01d468) )  /* sprite gfx */
	ROM_LOAD( "851.6", 0x02000, 0x2000, CRC(7db239a0) SHA1(af5772afff9009f63e2ab95c1cb00e047f3ed7e4) )
	ROM_LOAD( "852.7", 0x04000, 0x2000, CRC(c7700f88) SHA1(1a20cc60b083259070e4f1687b09a31fc763d47e) )
	ROM_LOAD( "853.8", 0x06000, 0x2000, CRC(b94bf1af) SHA1(da403c51fd78f99b82304c67f2197078f4ea0bf5) )

	ROM_REGION( 0x8000, REGION_GFX2, 0 )
	ROM_LOAD( "846.1", 0x00000, 0x2000, CRC(42447f4a) SHA1(bd35f2f5e468f9191680bf2c1800e09bb9ae1691) )  /* tile gfx */
	ROM_LOAD( "847.2", 0x02000, 0x2000, CRC(99ada5e8) SHA1(9425a515105ec9e9989aae736645b270e39420be) )
	ROM_LOAD( "848.3", 0x04000, 0x2000, CRC(a124cff4) SHA1(d1d8e6f725a6f30058d52cdbe80b598149cd6052) )
	ROM_LOAD( "849.4", 0x06000, 0x2000, CRC(5d27fda6) SHA1(f1afb39c7422caaa5eff53388f1b7241dd7c1cd7) )
ROM_END

/* actual release dates unknown. According to one of developers konek, bilyard and kot was released before 1988, koroleva after 1988. */
GAME( 198?, konek,    0, tiamc1, tiamc1,  0, ROT0, "Terminal", "Konek-Gorbunok", 0 )
GAME( 198?, sosterm,  0, tiamc1, tiamc1,  0, ROT0, "Terminal", "S.O.S.", 0 )
GAME( 198?, koroleva, 0, tiamc1, tiamc1,  0, ROT0, "Terminal", "Snezhnaja Koroleva", 0 )
GAME( 198?, bilyard,  0, tiamc1, tiamc1,  0, ROT0, "Terminal", "Billiard", 0 )
GAME( 198?, gorodki,  0, tiamc1, gorodki, 0, ROT0, "Terminal", "Gorodki", 0 )
GAME( 198?, kot,      0, kot,    kot,     0, ROT0, "Terminal", "Kot-Rybolov (Terminal)", GAME_IMPERFECT_SOUND )
