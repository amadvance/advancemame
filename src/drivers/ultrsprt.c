// license:BSD-3-Clause
// copyright-holders:Ville Linde, smf, AJR, Olivier Galibert
/* Konami Ultra Sports hardware */

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "machine/eeprom.h"
#include "sound/k054539.h"

static UINT32 *vram[2];
static UINT8 vram_page, track_reset, host_to_sound[4], sound_to_host[2];
static UINT8 sound_pending, sound_enabled;
static int track_last[4], track_count[4];
static mame_timer *pcm_timer;
static UINT8 pcm_control, pcm_timer_state;

static READ32_HANDLER( framebuffer_r ) { return vram[vram_page][offset]; }
static WRITE32_HANDLER( framebuffer_w ) { COMBINE_DATA(&vram[vram_page][offset]); }

static VIDEO_START( ultrsprt )
{
	vram[0] = auto_malloc(0x80000);
	vram[1] = auto_malloc(0x80000);
	memset(vram[0], 0, 0x80000);
	memset(vram[1], 0, 0x80000);
	state_save_register_global_pointer(vram[0], 0x80000 / 4);
	state_save_register_global_pointer(vram[1], 0x80000 / 4);
	return 0;
}

static VIDEO_UPDATE( ultrsprt )
{
	const UINT8 *pixels = (const UINT8 *)vram[vram_page ^ 1];
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int p = y * 1024 + x;
			UINT8 top = pixels[BYTE4_XOR_BE(p + 512)];
			((UINT16 *)bitmap->line[y])[x] = Machine->pens[top ? 0x100 | top : pixels[BYTE4_XOR_BE(p)]];
		}
}

static WRITE32_HANDLER( palette_w )
{
	int i;
	COMBINE_DATA(&paletteram32[offset]);
	for (i = 0; i < 2; i++)
	{
		UINT16 color = paletteram32[offset] >> (16 - 16 * i);
		palette_set_color(offset * 2 + i, pal5bit(color >> 10), pal5bit(color >> 5), pal5bit(color));
	}
}

static void track_update(void)
{
	int i;
	for (i = 0; i < 4; i++)
	{
		int value = readinputport(i);
		if (!track_reset) track_count[i] = (track_count[i] + value - track_last[i]) & 0xfff;
		track_last[i] = value;
	}
}

static UINT32 track_read(int player)
{
	UINT32 switches = (~readinputport(4) >> (player * 4)) & 7;
	track_update();
	if (switches) switches |= 8;
	return ((track_count[player * 2] | switches << 12) << 16) | track_count[player * 2 + 1] | switches << 12;
}
static READ32_HANDLER( track1_r ) { return track_read(0); }
static READ32_HANDLER( track2_r ) { return track_read(1); }

static READ32_HANDLER( system_r )
{
	UINT32 data = (readinputport(5) & ~2) | EEPROM_read_bit() << 1;
	return data << 24;
}
static WRITE32_HANDLER( system_w )
{
	if (!(mem_mask & 0xff000000))
	{
		UINT8 control = data >> 24;
		EEPROM_write_bit(control & 1);
		EEPROM_set_clock_line(control & 2 ? CLEAR_LINE : ASSERT_LINE);
		EEPROM_set_cs_line(control & 4 ? CLEAR_LINE : ASSERT_LINE);
		vram_page = (control >> 3) & 1;
		coin_counter_w(0, control & 0x10);
		track_update();
		track_reset = !(control & 0x40);
		if (track_reset) memset(track_count, 0, sizeof(track_count));
		cpunum_set_input_line(1, INPUT_LINE_RESET, control & 0x80 ? CLEAR_LINE : ASSERT_LINE);
	}
}
static WRITE32_HANDLER( int_ack_w ) { cpunum_set_input_line(0, INPUT_LINE_IRQ1, CLEAR_LINE); }

static UINT32 host_read(int reg) { return (reg & 7) < 2 ? sound_to_host[reg & 7] : 0; }
static void host_write(int reg, UINT8 data)
{
	reg &= 7;
	if (reg < 4) host_to_sound[reg] = data;
	else if (reg == 7 && sound_enabled)
	{
		sound_pending = 1;
		cpunum_set_input_line(1, 6, ASSERT_LINE);
	}
}
static READ32_HANDLER( sound_host_r )
{
	UINT32 result = 0;
	int i;
	for (i = 0; i < 4; i++) result |= host_read(offset * 4 + i) << (24 - 8 * i);
	return result;
}
static WRITE32_HANDLER( sound_host_w )
{
	int i;
	for (i = 0; i < 4; i++)
		if (!(mem_mask & (0xff000000U >> (8 * i)))) host_write(offset * 4 + i, data >> (24 - 8 * i));
}
static UINT8 sound_read(int reg) { reg &= 7; return reg < 4 ? host_to_sound[reg] : 0; }
static void sound_write(int reg, UINT8 data)
{
	reg &= 7;
	if (reg < 2) sound_to_host[reg] = data;
	else if (reg == 4)
	{
		sound_enabled = data & 1;
		if (!sound_enabled) sound_pending = 0;
		cpunum_set_input_line(1, 6, sound_pending && sound_enabled ? ASSERT_LINE : CLEAR_LINE);
	}
}
static READ16_HANDLER( sound_comm_r )
{
	UINT16 data = 0;
	if (ACCESSING_MSB) data |= sound_read(offset * 2) << 8;
	if (ACCESSING_LSB) data |= sound_read(offset * 2 + 1);
	return data;
}
static WRITE16_HANDLER( sound_comm_w )
{
	if (ACCESSING_MSB) sound_write(offset * 2, data >> 8);
	if (ACCESSING_LSB) sound_write(offset * 2 + 1, data);
}
static READ16_HANDLER( pcm_r )
{
	UINT16 data = 0;
	if (ACCESSING_MSB) data |= K054539_0_r(offset * 2) << 8;
	if (ACCESSING_LSB) data |= K054539_0_r(offset * 2 + 1);
	return data;
}
static void pcm_tick(int param)
{
	if (pcm_control & 0x20)
	{
		pcm_timer_state ^= 1;
		cpunum_set_input_line(1, 5, pcm_timer_state ? ASSERT_LINE : CLEAR_LINE);
	}
}
static void pcm_write(int reg, UINT8 data)
{
	if (reg == 0x227)
	{
		/* IRQ5 follows both edges of the programmable PCM timer. */
		double period = TIME_IN_HZ((38 + data) * 48000.0 / 14400.0 * 2);
		timer_adjust(pcm_timer, period, 0, period);
		pcm_timer_state = 0;
		cpunum_set_input_line(1, 5, CLEAR_LINE);
	}
	else if (reg == 0x22f)
	{
		pcm_control = data;
		if (!(data & 0x20))
		{
			pcm_timer_state = 0;
			cpunum_set_input_line(1, 5, CLEAR_LINE);
		}
	}
	K054539_0_w(reg, data);
}
static WRITE16_HANDLER( pcm_w )
{
	if (ACCESSING_MSB) pcm_write(offset * 2, data >> 8);
	if (ACCESSING_LSB) pcm_write(offset * 2 + 1, data);
}
static struct K054539interface pcm_interface = { REGION_SOUND1 };

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x0007ffff) AM_MIRROR(0x80000000) AM_READWRITE(framebuffer_r, framebuffer_w)
	AM_RANGE(0x70000000, 0x70000003) AM_MIRROR(0x80000000) AM_READWRITE(system_r, system_w)
	AM_RANGE(0x70000020, 0x70000023) AM_MIRROR(0x80000000) AM_READ(track1_r)
	AM_RANGE(0x70000040, 0x70000043) AM_MIRROR(0x80000000) AM_READ(track2_r)
	AM_RANGE(0x70000080, 0x7000008f) AM_MIRROR(0x80000000) AM_READWRITE(sound_host_r, sound_host_w)
	AM_RANGE(0x700000c0, 0x700000cf) AM_MIRROR(0x80000000) AM_WRITENOP
	AM_RANGE(0x700000e0, 0x700000e3) AM_MIRROR(0x80000000) AM_WRITE(int_ack_w)
	AM_RANGE(0x7f000000, 0x7f01ffff) AM_MIRROR(0x80000000) AM_RAM
	AM_RANGE(0x7f700000, 0x7f703fff) AM_MIRROR(0x80000000) AM_RAM AM_WRITE(palette_w) AM_BASE(&paletteram32)
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80600000) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x100000, 0x101fff) AM_RAM
	AM_RANGE(0x200000, 0x20000f) AM_READWRITE(sound_comm_r, sound_comm_w)
	AM_RANGE(0x400000, 0x4002ff) AM_READWRITE(pcm_r, pcm_w)
ADDRESS_MAP_END

static NVRAM_HANDLER( ultrsprt )
{
	if (read_or_write) EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface_93C46);
		if (file) EEPROM_load(file);
		else EEPROM_set_data(memory_region(REGION_USER2), 0x80);
	}
}

static MACHINE_START( ultrsprt )
{
	pcm_timer = timer_alloc(pcm_tick);
	state_save_register_global(vram_page);
	state_save_register_global(track_reset);
	state_save_register_global(sound_pending);
	state_save_register_global(sound_enabled);
	state_save_register_global(pcm_control);
	state_save_register_global(pcm_timer_state);
	state_save_register_global_array(host_to_sound);
	state_save_register_global_array(sound_to_host);
	state_save_register_global_array(track_last);
	state_save_register_global_array(track_count);
	return 0;
}

static MACHINE_RESET( ultrsprt )
{
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	vram_page = sound_pending = sound_enabled = pcm_control = pcm_timer_state = 0;
	timer_adjust(pcm_timer, TIME_NEVER, 0, 0);
	cpunum_set_input_line(1, 5, CLEAR_LINE);
	cpunum_set_input_line(1, 6, CLEAR_LINE);
	memset(host_to_sound, 0, sizeof(host_to_sound));
	memset(sound_to_host, 0, sizeof(sound_to_host));
	memset(track_count, 0, sizeof(track_count));
	memset(track_last, 0, sizeof(track_last));
	track_reset = 1;
}
static INTERRUPT_GEN( ultrsprt )
{
	track_update();
	cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
}

static INPUT_PORTS_START( ultrsprt )
	PORT_START
	PORT_BIT( 0xfff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(200) PORT_KEYDELTA(30) PORT_REVERSE PORT_PLAYER(1)
	PORT_START
	PORT_BIT( 0xfff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(200) PORT_KEYDELTA(30) PORT_PLAYER(1)
	PORT_START
	PORT_BIT( 0xfff, 0, IPT_TRACKBALL_X ) PORT_SENSITIVITY(200) PORT_KEYDELTA(30) PORT_REVERSE PORT_PLAYER(2)
	PORT_START
	PORT_BIT( 0xfff, 0, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(200) PORT_KEYDELTA(30) PORT_PLAYER(2)
	PORT_START
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x88, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_START
	PORT_BIT( 0x07, IP_ACTIVE_HIGH, IPT_SPECIAL )
	PORT_SERVICE_NO_TOGGLE( 0x08, IP_ACTIVE_LOW )
INPUT_PORTS_END

static ppc_config ultrsprt_ppc_config = { PPC_MODEL_403GA };

static MACHINE_DRIVER_START( ultrsprt )
	MDRV_CPU_ADD(PPC403, 25000000)
	MDRV_CPU_CONFIG(ultrsprt_ppc_config)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(ultrsprt, 1)
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(sound_map, 0)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(100)
	MDRV_MACHINE_START(ultrsprt)
	MDRV_MACHINE_RESET(ultrsprt)
	MDRV_NVRAM_HANDLER(ultrsprt)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 511, 0, 399)
	MDRV_PALETTE_LENGTH(8192)
	MDRV_VIDEO_START(ultrsprt)
	MDRV_VIDEO_UPDATE(ultrsprt)
	MDRV_SPEAKER_STANDARD_STEREO("left", "right")
	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(pcm_interface)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( fiveside )
	ROM_REGION32_BE(0x200000, REGION_USER1, 0)
	ROM_LOAD32_BYTE("479uaa01.bin", 0x000000, 0x80000, CRC(1bc4893d) SHA1(2c9df38ecb7efa7b686221ee98fa3aad9a63e152))
	ROM_LOAD32_BYTE("479uaa02.bin", 0x000001, 0x80000, CRC(ae74a6d0) SHA1(6113c2eea1628b22737c7b87af0e673d94984e88))
	ROM_LOAD32_BYTE("479uaa03.bin", 0x000002, 0x80000, CRC(5c0b176f) SHA1(9560259bc081d4cfd72eb485c3fdcecf484ba7a8))
	ROM_LOAD32_BYTE("479uaa04.bin", 0x000003, 0x80000, CRC(01a3e4cb) SHA1(819df79909d57fa12481698ffdb32b00586131d8))

	ROM_REGION(0x20000, REGION_CPU2, 0)
	ROM_LOAD("479_a05.bin", 0x000000, 0x20000, CRC(251ae299) SHA1(5ffd74357e3c6ddb3a208c39a3b32b53fea90282))

	ROM_REGION(0x100000, REGION_SOUND1, 0)
	ROM_LOAD("479_a06.bin", 0x000000, 0x80000, CRC(8d6ac8a2) SHA1(7c4b8bd47cddc766cbdb6a486acc9221be55b579))
	ROM_LOAD("479_a07.bin", 0x080000, 0x80000, CRC(75835df8) SHA1(105b95c16f2ce6902c2e4c9c2fd9f2f7a848c546))

	ROM_REGION( 0x80, REGION_USER2, 0 )
	ROM_LOAD( "fiveside.nv", 0x0000, 0x0080, CRC(aad11072) SHA1(8f777ee47801faa7ce8420c3052034720225aae7) )
ROM_END

GAME( 1995, fiveside, 0, ultrsprt, ultrsprt, 0, ROT90, "Konami", "Five a Side Soccer (ver UAA)", 0 )
