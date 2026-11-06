// license:BSD-3-Clause
// copyright-holders:Tomasz Slanina, Roberto Fresca, AJR
/*
    Night Mare / Clean Octopus
    E.F.O. S.A. Video IV System, Barcelona, Spain.

    Two RCA CDP1802 CPUs, two EFO90501 video processors,
    SDA2006 settings EEPROM and Sound-3 speech synthesizer board.
*/

#include "driver.h"
#include "cpu/cdp1802/cdp1802.h"
#include "vidhrdw/tms9928a.h"
#include "sound/5220intf.h"

static UINT16 eeprom_data[32];
static UINT32 eeprom_stream;
static int eeprom_latch, eeprom_address, eeprom_reading, eeprom_pos;
static int eeprom_length, eeprom_end, eeprom_enable, eeprom_clock;
static int main_mode;
static int sound_command, sound_pending, sound_read;
static mame_bitmap *vdp_bitmap[2];
static int old_backdrop[2];

static void eeprom_clock_w(int state)
{
	if (eeprom_clock && !state)
	{
		if (eeprom_reading)
		{
			eeprom_latch = (eeprom_data[eeprom_address] >> eeprom_pos) & 1;
			if (++eeprom_pos == 16)
			{
				eeprom_reading = 0;
				eeprom_length = 0;
				eeprom_stream = 0;
			}
		}
		else if (eeprom_end)
		{
			UINT32 reversed = 0;
			UINT32 stream = eeprom_stream;
			int i;

			/* The serial command and stored word travel in opposite bit orders. */
			for (i = 0; i < eeprom_length; ++i)
			{
				reversed = (reversed << 1) | (stream & 1);
				stream >>= 1;
			}
			switch (BITSWAP8(eeprom_stream, 7,6,5,4,3,0,1,2) & 3)
			{
			case 0:
				eeprom_data[(reversed >> 16) & 31] = reversed & 0xffff;
				break;
			case 2:
				eeprom_address = reversed & 31;
				eeprom_pos = 0;
				eeprom_reading = 1;
				break;
			}
			eeprom_end = 0;
			eeprom_length = 0;
		}
		else if (eeprom_enable)
		{
			eeprom_stream = (eeprom_stream << 1) | eeprom_latch;
			++eeprom_length;
		}
	}
	eeprom_clock = state;
}

static NVRAM_HANDLER( nightmare )
{
	if (read_or_write)
		mame_fwrite(file, eeprom_data, sizeof(eeprom_data));
	else if (file)
		mame_fread(file, eeprom_data, sizeof(eeprom_data));
	else
		/* Factory settings include the checksums needed to boot without service setup. */
		memcpy(eeprom_data, memory_region(REGION_USER1), sizeof(eeprom_data));
}

static UINT8 main_mode_r(void)
{
	return main_mode;
}

static UINT8 main_ef_r(void)
{
	int inputs = readinputport(2);
	/* The CPU interface samples the physical, active-low EF pins. */
	return ~(((eeprom_latch ^ 1) << 1) | ((inputs & 2) << 1) | ((inputs & 1) << 3)) & 0x0f;
}

static WRITE8_HANDLER( settings_w )
{
	eeprom_latch = (data >> 7) & 1;
	if (eeprom_enable && !(data & 0x40))
		eeprom_end = 1;
	eeprom_enable = (data >> 6) & 1;
	coin_counter_w(0, data & 0x10);
	coin_counter_w(1, data & 0x20);
}

static void latch_sound_command(int data)
{
	sound_command = data;
	sound_pending = 1;
}

static WRITE8_HANDLER( sound_command_w )
{
	/* Synchronize the two CPUs before changing the command and its handshake flag. */
	timer_set(TIME_NOW, data, latch_sound_command);
}

static UINT8 sound_mode_r(void)
{
	return CDP1802_MODE_RUN;
}

static UINT8 sound_ef_r(void)
{
	int sound_status = sound_read ? tms5220_status_r(0) : 0xff;
	return ~((tms5220_ready_r() ? EF1 : 0) | (sound_pending ? EF2 : 0)
		| ((sound_status & 0x40) ? 0 : EF3) | ((sound_status & 0x80) ? 0 : EF4)) & 0x0f;
}

static void sound_q_w(int state)
{
	sound_read = state;
}

static void speech_irq(int state)
{
	cpunum_set_input_line(1, CDP1802_INPUT_LINE_INT, state ? ASSERT_LINE : CLEAR_LINE);
}

static READ8_HANDLER( sound_command_r )
{
	sound_pending = 0;
	return sound_command;
}

static WRITE8_HANDLER( speech_w )
{
	tms5220_data_w(0, data);
}

static CDP1802_CONFIG main_config =
{
	main_mode_r, main_ef_r, 0, eeprom_clock_w, 0, 0
};

static CDP1802_CONFIG sound_cpu_config =
{
	sound_mode_r, sound_ef_r, 0, sound_q_w, 0, 0
};

static const struct TMS5220interface speech_config =
{
	speech_irq, 0, 0, 0
};

static READ8_HANDLER( vdp_r )
{
	return TMS9928A_read_indexed(offset >> 1, offset & 1);
}

static WRITE8_HANDLER( vdp_w )
{
	TMS9928A_write_indexed(offset >> 1, offset & 1, data);
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x5fff) AM_ROM
	AM_RANGE(0x8000, 0x83ff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( main_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(1, 1) AM_READWRITE(input_port_0_r, sound_command_w)
	AM_RANGE(2, 2) AM_READWRITE(input_port_1_r, settings_w)
	AM_RANGE(4, 7) AM_READWRITE(vdp_r, vdp_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_MIRROR(0xc000) AM_ROM
	AM_RANGE(0x2000, 0x201f) AM_MIRROR(0xdfe0) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(1, 1) AM_MIRROR(4) AM_WRITE(speech_w)
	AM_RANGE(2, 2) AM_MIRROR(4) AM_READ(sound_command_r)
ADDRESS_MAP_END

static void vdp_irq(int state)
{
	cpunum_set_input_line(0, CDP1802_INPUT_LINE_INT, state ? ASSERT_LINE : CLEAR_LINE);
}

static const TMS9928a_interface vdp_config[2] =
{
	{ TMS9929A, 0x4000, 0 },
	{ TMS9929A, 0x4000, vdp_irq }
};

static INTERRUPT_GEN( video_interrupt )
{
	TMS9928A_interrupt_indexed(0);
	TMS9928A_interrupt_indexed(1);
}

static VIDEO_START( nightmare )
{
	int i;
	for (i = 0; i < 2; ++i)
	{
		if (TMS9928A_start_indexed(i, &vdp_config[i]))
			return 1;
		vdp_bitmap[i] = auto_bitmap_alloc(286, 294);
	}
	return 0;
}

static const UINT8 vdp_colors[16][3] =
{
	{0,0,0}, {0,0,0}, {33,200,66}, {94,220,120},
	{84,85,237}, {125,118,252}, {212,82,77}, {66,235,245},
	{252,85,84}, {255,121,120}, {212,193,84}, {230,206,128},
	{33,176,59}, {201,91,186}, {204,204,204}, {255,255,255}
};

static PALETTE_INIT( nightmare )
{
	int i;
	for (i = 0; i < 16; ++i)
		palette_set_color(i, vdp_colors[i][0], vdp_colors[i][1], vdp_colors[i][2]);
}

static VIDEO_UPDATE( nightmare )
{
	int backdrop[2], x, y, a, b, channel;
	backdrop[0] = TMS9928A_update_indexed(0, vdp_bitmap[0], cliprect);
	backdrop[1] = TMS9928A_update_indexed(1, vdp_bitmap[1], cliprect);

	if (backdrop[0] != old_backdrop[0] || backdrop[1] != old_backdrop[1])
	{
		/* Mix the palette once per backdrop change instead of adding RGB per pixel. */
		for (a = 0; a < 16; ++a)
			for (b = 0; b < 16; ++b)
			{
				int rgb[3];
				for (channel = 0; channel < 3; ++channel)
				{
					int value = vdp_colors[a ? a : backdrop[0]][channel]
						+ vdp_colors[b ? b : backdrop[1]][channel];
					rgb[channel] = value > 255 ? 255 : value;
				}
				palette_set_color(16 + a * 16 + b, rgb[0], rgb[1], rgb[2]);
			}
		old_backdrop[0] = backdrop[0];
		old_backdrop[1] = backdrop[1];
	}
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *p0 = vdp_bitmap[0]->line[y];
		UINT16 *p1 = vdp_bitmap[1]->line[y];
		UINT16 *dest = bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			dest[x] = Machine->pens[16 + p0[x] * 16 + p1[x]];
	}
}

static void release_reset(int param)
{
	main_mode = CDP1802_MODE_RUN;
}

static MACHINE_RESET( nightmare )
{
	main_mode = CDP1802_MODE_RESET;
	timer_set(TIME_IN_MSEC(200), 0, release_reset);
	eeprom_stream = eeprom_latch = eeprom_address = eeprom_reading = eeprom_pos = 0;
	eeprom_length = eeprom_end = eeprom_enable = eeprom_clock = 0;
	sound_command = sound_pending = sound_read = 0;
	old_backdrop[0] = old_backdrop[1] = -1;
	TMS9928A_reset_indexed(0);
	TMS9928A_reset_indexed(1);
}

INPUT_PORTS_START( nightmare )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_4WAY PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_4WAY PORT_COCKTAIL

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL

	PORT_START
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_SERVICE )
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_TILT )
INPUT_PORTS_END

static MACHINE_DRIVER_START( nightmare )
	MDRV_CPU_ADD(CDP1802, 10816000 / 3)
	MDRV_CPU_CONFIG(main_config)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_IO_MAP(main_io, 0)
	MDRV_CPU_VBLANK_INT(video_interrupt, 1)

	MDRV_CPU_ADD(CDP1802, 2950000)
	MDRV_CPU_CONFIG(sound_cpu_config)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_CPU_IO_MAP(sound_io, 0)

	MDRV_FRAMES_PER_SECOND(10816000.0 / 2 / 346 / 313)
	MDRV_VBLANK_DURATION(2500)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_RESET(nightmare)
	MDRV_NVRAM_HANDLER(nightmare)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_BEFORE_VBLANK)
	MDRV_SCREEN_SIZE(286, 294)
	MDRV_VISIBLE_AREA(15 - 12, 15 + 256 + 12 - 1, 51 - 12, 51 + 192 + 12 - 1)
	MDRV_PALETTE_LENGTH(272)
	MDRV_PALETTE_INIT(nightmare)
	MDRV_VIDEO_START(nightmare)
	MDRV_VIDEO_UPDATE(nightmare)

	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(TMS5220, 640000)
	MDRV_SOUND_CONFIG(speech_config)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( nightmare )
	ROM_REGION( 0x6000, REGION_CPU1, 0 )
	ROM_LOAD( "nm1-ia1.ic11", 0x0000, 0x2000, CRC(5d648f62) SHA1(028a47d4b1b4910d0d4e00f81d4e94a5478834d3) )
	ROM_LOAD( "nm1-ib1.ic12", 0x2000, 0x2000, CRC(c10695f7) SHA1(929467fe7529782e8181d3caae3a67bb0a8d8753) )
	ROM_LOAD( "nm1-ic1.ic13", 0x4000, 0x2000, CRC(a3117246) SHA1(ca9601401f7ab34200c969e41ffae50bee0aca4d) )

	ROM_REGION( 0x2000, REGION_CPU2, 0 )
	ROM_LOAD( "scl-1a1.ic5", 0x0000, 0x2000, CRC(4bba61af) SHA1(b324344081e3d4b5db43a8ff3122c28cf75aec84) )

	ROM_REGION( 0x40, REGION_USER1, 0 )
	ROM_LOAD( "eeprom.ic7", 0x0000, 0x0040, CRC(7824e1f8) SHA1(2ccac62b4e8abcb2b3d66fa4025947fea184664e) )
ROM_END

GAME( 1982, nightmare, 0, nightmare, nightmare, 0, ROT90, "E.F.O.", "Night Mare (Spain)", 0 )
