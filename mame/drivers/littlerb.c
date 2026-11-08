// license:BSD-3-Clause
// copyright-holders:Pierpaolo Prazzoli, David Haywood, Angelo Salese
/* Little Robin, (c)1994 TCH

   Video commands update two persistent framebuffers. Scroll and copy
   operations are not fully decoded.
*/

#include "driver.h"
#include "vidhrdw/tlc34076.h"
#include "sound/dac.h"

static UINT16 *video_ram, *work_ram, *sprite_list;
static mame_bitmap *foreground, *background;
static UINT16 host_address_low, host_address_high, host_control;
static UINT32 host_address;
static unsigned list_count;
static UINT8 sound_index[2];
static UINT16 sound_pointer[2];
static unsigned sound_frame;

static UINT8 video_byte(UINT32 address)
{
	UINT16 word;
	if (address < 0x400000)
		word = video_ram[(address >> 1) & 0x1fffff];
	else if (address >= 0x1ff80000 && address <= 0x1fffffff)
		word = work_ram[(address >> 1) & 0x3ffff];
	else
		return 0;
	return word >> ((address & 1) * 8);
}

static void draw_sprite(mame_bitmap *target, const rectangle *clip, UINT32 address,
	int xpos, int ypos, int width, int height)
{
	int x, y;
	address >>= 3;
	for (y = 0; y < height; ++y)
		for (x = 0; x < width; ++x)
		{
			UINT8 pixel = video_byte(address);
			int px = xpos + x, py = ypos + y;
			/* Lines above the clipping window do not consume source data. */
			if (py >= clip->min_y)
				++address;
			if (pixel && px >= clip->min_x && px <= clip->max_x &&
				py >= clip->min_y && py <= clip->max_y)
				plot_pixel(target, px, py, pixel);
		}
}

static void draw_list(void)
{
	unsigned offs = 0;
	int layer = 0;
	rectangle clip = { 0, 335, 0, 287 };

	/* A layer clear belongs to the submitted list, not to screen refresh. */
	if ((sprite_list[1] | sprite_list[2]) & 0x1000)
		if (!(sprite_list[2] & 0x1000))
			fillbitmap(background, 0, 0);

	while (offs < list_count)
	{
		UINT16 *entry = &sprite_list[offs];
		UINT32 command = entry[0] | (entry[1] << 16);
		if (entry[0] == 0x0060)
			offs += 6;
		else if (entry[0] == 0x0040)
		{
			if (entry[4] == 0x6000)
			{
				if (entry[3] & 0x1000)
					fillbitmap(background, 0, 0);
			}
			else if (entry[1] != 0xffd6)
				fillbitmap(foreground, 0, 0);
			offs += 6;
		}
		else if (command == 0x00e40020 || command == 0x00e40000)
		{
			layer = entry[4] == 0x6000;
			clip.min_x = MIN(entry[6], 511);
			clip.max_x = MIN(entry[8], 511);
			clip.min_y = MIN(entry[7], 511);
			clip.max_y = MIN(entry[9], 511);
			offs += 10;
		}
		else if (!command)
			++offs;
		else
		{
			int x = entry[2] & 0x7ff;
			int y = entry[3] & 0x3ff;
			if (x & 0x400) x -= 0x800;
			if (y & 0x200) y -= 0x400;
			draw_sprite(layer ? background : foreground, &clip, command, x, y,
				entry[4] & 0x1ff, entry[5] & 0xff);
			offs += 6;
		}
	}
	list_count = 0;
	memset(sprite_list, 0, 0x20000);
}

static READ16_HANDLER( host_r )
{
	switch (offset)
	{
		case 0: return host_address_low;
		case 1: return host_address_high;
		case 2:
			if ((host_address >> 3) == 0x1ff80804) return 0;
			return video_byte(host_address >> 3) | (video_byte((host_address >> 3) + 1) << 8);
		default: return host_control;
	}
}

static WRITE16_HANDLER( host_w )
{
	UINT32 address;
	switch (offset)
	{
		case 0:
			COMBINE_DATA(&host_address_low);
			break;
		case 1:
			COMBINE_DATA(&host_address_high);
			break;
		case 3:
			COMBINE_DATA(&host_control);
			break;
		case 2:
			address = host_address >> 3;
			if (host_control != 0x3800 && host_control != 0x2000 &&
				host_control != 0xe000 && host_control != 0xf800)
				return;
			if (address < 0x400000)
				COMBINE_DATA(&video_ram[address >> 1]);
			else if (address >= 0x800000 && address <= 0x800005)
				tlc34076_lsb_w((address - 0x800000) >> 1, data, mem_mask);
			else if (address == 0x1ff80804)
				draw_list();
			else if (address == 0x1ff80806)
			{
				/* Leave room for the longest command when decoding the list. */
				if (list_count < 0x10000 - 10)
				{
					COMBINE_DATA(&sprite_list[list_count]);
					++list_count;
				}
			}
			else if (address >= 0x1ff80000 && address <= 0x1fffffff)
				COMBINE_DATA(&work_ram[(address >> 1) & 0x3ffff]);
			if (host_control != 0x2000 && host_control != 0xe000 && address != 0x1ff80806)
				host_address += 0x10;
			host_address_low = host_address;
			host_address_high = host_address >> 16;
			return;
	}
	host_address = host_address_low | (host_address_high << 16);
}

static WRITE16_HANDLER( left_sound_w )
{
	sound_index[0] = data >> ((sound_frame % 16 == 0) ? 8 : 0);
	sound_pointer[0] = 0;
}

static WRITE16_HANDLER( right_sound_w )
{
	sound_index[1] = data >> ((sound_frame % 16 == 0) ? 8 : 0);
	sound_pointer[1] = 0;
}

static void sound_tick(int param)
{
	UINT8 *samples = memory_region(REGION_SOUND1);
	DAC_signed_data_w(0, samples[0x40000 | (sound_index[0] << 10) | sound_pointer[0]]);
	DAC_signed_data_w(1, samples[(sound_index[1] << 10) | sound_pointer[1]]);
	sound_pointer[0] = (sound_pointer[0] + 1) & 0x3ff;
	sound_pointer[1] = (sound_pointer[1] + 1) & 0x3ff;
}

static void sound_step(int param)
{
	++sound_frame;
}

static UINT32 frame_step_r(void *param)
{
	return sound_frame & 7;
}

static ADDRESS_MAP_START( littlerb_main, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000008, 0x000017) AM_WRITENOP
	AM_RANGE(0x000020, 0x00002f) AM_WRITENOP
	AM_RANGE(0x000070, 0x000073) AM_WRITENOP
	AM_RANGE(0x060004, 0x060007) AM_WRITENOP
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x200000, 0x203fff) AM_RAM
	AM_RANGE(0x700000, 0x700007) AM_READWRITE(host_r, host_w)
	AM_RANGE(0x740000, 0x740001) AM_WRITE(left_sound_w)
	AM_RANGE(0x760000, 0x760001) AM_WRITE(right_sound_w)
	AM_RANGE(0x780000, 0x780001) AM_WRITENOP
	AM_RANGE(0x7c0000, 0x7c0001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x7e0000, 0x7e0001) AM_READ(input_port_1_word_r)
	AM_RANGE(0x7e0002, 0x7e0003) AM_READ(input_port_2_word_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( littlerb )
	PORT_START
	PORT_DIPNAME( 0x0003, 0x0001, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0003, "2" )
	PORT_DIPSETTING(      0x0001, "3" )
	PORT_DIPSETTING(      0x0002, "4" )
	PORT_DIPSETTING(      0x0000, "5" )
	PORT_DIPNAME( 0x001c, 0x0004, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0014, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x000c, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x001c, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00e0, 0x0020, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x00c0, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x00a0, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0060, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x00e0, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0600, 0x0600, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(      0x0600, "Every 150,000" )
	PORT_DIPSETTING(      0x0200, "Every 200,000" )
	PORT_DIPSETTING(      0x0400, "Every 300,000" )
	PORT_DIPSETTING(      0x0000, DEF_STR( None ) )
	PORT_DIPNAME( 0x3800, 0x2800, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(      0x3800, DEF_STR( Easy ) )
	PORT_DIPSETTING(      0x1800, "Medium Easy" )
	PORT_DIPSETTING(      0x2800, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0800, "Medium Hard" )
	PORT_DIPSETTING(      0x3000, DEF_STR( Hard ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Harder ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Very_Hard ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Hardest ) )
	PORT_SERVICE( 0x4000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( On ) )

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x1000, 0x1000, "???"  )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0xe000, IP_ACTIVE_HIGH, IPT_SPECIAL ) PORT_CUSTOM(frame_step_r, 0)

	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static VIDEO_START( littlerb )
{
	video_ram = auto_malloc(0x400000);
	work_ram = auto_malloc(0x80000);
	sprite_list = auto_malloc(0x20000);
	foreground = auto_bitmap_alloc(512, 512);
	background = auto_bitmap_alloc(512, 512);
	return 0;
}

static VIDEO_UPDATE( littlerb )
{
	fillbitmap(bitmap, Machine->pens[0], cliprect);
	copybitmap(bitmap, background, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
	copybitmap(bitmap, foreground, 0, 0, 0, 0, cliprect, TRANSPARENCY_PEN, 0);
}

static MACHINE_RESET( littlerb )
{
	host_address_low = host_address_high = host_control = 0;
	host_address = list_count = 0;
	memset(video_ram, 0, 0x400000);
	memset(work_ram, 0, 0x80000);
	memset(sprite_list, 0, 0x20000);
	fillbitmap(foreground, 0, 0);
	fillbitmap(background, 0, 0);
	sound_frame = 0;
	memset(sound_index, 0, sizeof(sound_index));
	memset(sound_pointer, 0, sizeof(sound_pointer));
	tlc34076_reset(8);
	/* Sample selection advances at 50 Hz independently of screen refresh. */
	timer_pulse(TIME_IN_HZ(7500), 0, sound_tick);
	timer_pulse(TIME_IN_HZ(50), 0, sound_step);
}

static MACHINE_DRIVER_START( littlerb )
	MDRV_CPU_ADD(M68000, 12000000)
	MDRV_CPU_PROGRAM_MAP(littlerb_main, 0)
	MDRV_CPU_VBLANK_INT(irq4_line_hold, 1)
	MDRV_MACHINE_RESET(littlerb)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_SCREEN_SIZE(512, 288)
	MDRV_VISIBLE_AREA(0, 335, 0, 287)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(littlerb)
	MDRV_VIDEO_UPDATE(littlerb)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "left", 0.5)
	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "right", 0.5)
MACHINE_DRIVER_END

ROM_START( littlerb )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "tch_1.u53", 0x00001, 0x80000, CRC(172fbc13) SHA1(cd165ca0d0546e2634cf182dc98004cbfb02cf9f) )
	ROM_LOAD16_BYTE( "tch_2.u29", 0x00000, 0x80000, CRC(b2fb1d61) SHA1(9a9d7176c241928d07af651e5f7f21d4f019701d) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "tch_3.u26", 0x40000, 0x40000, CRC(f193c5b6) SHA1(95548a40e2b5064c558b36cabbf507d23678b1b2) )
	ROM_LOAD( "tch_4.u32", 0x00000, 0x40000, CRC(d6b81583) SHA1(b7a63d18a41ccac4d3db9211de0b0cdbc914317a) )
ROM_END

GAME( 1994, littlerb, 0, littlerb, littlerb, 0, ROT0, "TCH", "Little Robin", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
