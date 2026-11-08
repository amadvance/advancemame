// license:BSD-3-Clause
// copyright-holders:David Haywood
/*

Twins
Electronic Devices, 1994

PCB Layout
----------

This is a very tiny PCB,
only about 6 inches square.

|-----------------------|
|     6116   16MHz 62256|
|TEST 6116 24C02        |
|             PAL  62256|
|J   62256 |--------|   |
|A         |TPC1020 | 2 |
|M   62256 |AFN-084C|   |
|M         |        | 1 |
|A         |--------|   |
| AY3-8910              |
|                 D70116|
|-----------------------|
Notes:
    V30 clock      : 8.000MHz (16/2)
    AY3-8910 clock : 2.000MHz (16/8)
    VSync          : 50Hz



seems a similar board to hotblocks

same TPC1020 AFN-084C chip
same 24c02 eeprom
V30 instead of I88
AY3-8910 instead of YM2149 (compatible)

video is not banked in this case instead palette data is sent to the ports
strange palette format.

todo:
hook up eeprom
takes a long time to boot (eeprom?)


Electronic Devices was printed on rom labels
1994 date string is in ROM

*/

#include "driver.h"
#include "sound/ay8910.h"

static UINT8 *twins_videoram;
static UINT16 *twins_pal;
static UINT16 paloff = 0;

/* port 4 is eeprom */
static READ8_HANDLER( twins_port4_r )
{
	return 0xff;
}

static WRITE8_HANDLER( twins_port4_w )
{
}

static WRITE8_HANDLER( port6_pal0_w )
{
	twins_pal[paloff] = (twins_pal[paloff] & 0xff00) | data;
}

static WRITE8_HANDLER( port7_pal1_w )
{
	twins_pal[paloff] = (twins_pal[paloff] & 0x00ff) | (data<<8);
	paloff = (paloff + 1) & 0xff;

}

/* ??? weird ..*/
static WRITE8_HANDLER( porte_paloff0_w )
{
	paloff = 0;
}

/* ??? weird ..*/
static WRITE8_HANDLER( portf_paloff1_w )
{
	paloff = 0;
}

static ADDRESS_MAP_START( twins_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_RAM
	AM_RANGE(0x10000, 0x1ffff) AM_RAM AM_BASE(&twins_videoram)
	AM_RANGE(0x20000, 0xfffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( twins_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x0000) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0002, 0x0002) AM_READ(AY8910_read_port_0_r) AM_WRITE(AY8910_write_port_0_w)
	AM_RANGE(0x0004, 0x0004) AM_READ(twins_port4_r) AM_WRITE(twins_port4_w)
	AM_RANGE(0x0006, 0x0006) AM_WRITE(port6_pal0_w)
	AM_RANGE(0x0007, 0x0007) AM_WRITE(port7_pal1_w)
	AM_RANGE(0x000e, 0x000e) AM_WRITE(porte_paloff0_w)
	AM_RANGE(0x000f, 0x000f) AM_WRITE(portf_paloff1_w)
ADDRESS_MAP_END

VIDEO_START(twins)
{
	twins_pal = auto_malloc(0x100*2);
	return 0;
}

VIDEO_UPDATE(twins)
{
	int y,x,count;
	int i;
	static int xxx=320,yyy=204;

	fillbitmap(bitmap, get_black_pen(), 0);

	for (i=0;i<0x100;i++)
	{
		int dat,r,g,b;
		dat = twins_pal[i];

		r = dat & 0x1f;
		r = BITSWAP8(r,7,6,5,0,1,2,3,4);

		g = (dat>>5) & 0x1f;
		g = BITSWAP8(g,7,6,5,0,1,2,3,4);

		b = (dat>>10) & 0x1f;
		b = BITSWAP8(b,7,6,5,0,1,2,3,4);

		palette_set_color(i, r*8,g*8,b*8);
	}

	count=0;
	for (y=0;y<yyy;y++)
	{
		for(x=0;x<xxx;x++)
		{
			plot_pixel(bitmap, x,y, twins_videoram[count]);
			count++;
		}
	}
}


INPUT_PORTS_START(twins)
	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
INPUT_PORTS_END


static struct AY8910interface ay8910_interface =
{
	input_port_0_r,
	input_port_1_r
};

static MACHINE_DRIVER_START( twins )
	/* basic machine hardware */
	MDRV_CPU_ADD(V30, 8000000)
	MDRV_CPU_PROGRAM_MAP(twins_map, 0)
	MDRV_CPU_IO_MAP(twins_io,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)

	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320,256)
	MDRV_VISIBLE_AREA(0, 320-1, 0, 200-1)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(twins)
	MDRV_VIDEO_UPDATE(twins)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

static UINT8 spider_ram[0x10000], spider_bg[0x10000], spider_fg[0x10000];
static UINT8 spider_eeprom[0x100];
static UINT16 spider_palette[0x100];
static int spider_bank, spider_pal_index, spider_blit_state, spider_blit_addr, spider_blit_width;
static UINT32 spider_last_pc, spider_last_addr;
static int spider_skip_high, spider_skip_operation;
static int i2c_scl, i2c_sda, i2c_out, i2c_state, i2c_bits, i2c_byte, i2c_addr, i2c_next;

enum { I2C_IDLE, I2C_CONTROL, I2C_ADDRESS, I2C_WRITE, I2C_ACK, I2C_SEND, I2C_MASTER_ACK };

static void spider_i2c_write(int data)
{
	int scl = (data >> 1) & 1, sda = data & 1;
	if (i2c_scl && scl && i2c_sda && !sda)
	{
		i2c_state = I2C_CONTROL;
		i2c_bits = i2c_byte = 0;
		i2c_out = 1;
	}
	else if (i2c_scl && scl && !i2c_sda && sda)
	{
		i2c_state = I2C_IDLE;
		i2c_out = 1;
	}
	else if (!i2c_scl && scl)
	{
		if (i2c_state == I2C_CONTROL || i2c_state == I2C_ADDRESS || i2c_state == I2C_WRITE)
		{
			i2c_byte = (i2c_byte << 1) | sda;
			if (++i2c_bits == 8)
			{
				if (i2c_state == I2C_CONTROL)
					i2c_next = (i2c_byte & 0xf0) == 0xa0 ? ((i2c_byte & 1) ? I2C_SEND : I2C_ADDRESS) : I2C_IDLE;
				else if (i2c_state == I2C_ADDRESS)
				{
					i2c_addr = i2c_byte & 0xff;
					i2c_next = I2C_WRITE;
				}
				else
				{
					spider_eeprom[i2c_addr] = i2c_byte;
					i2c_addr = (i2c_addr + 1) & 0xff;
					i2c_next = I2C_WRITE;
				}
				i2c_state = I2C_ACK;
				i2c_bits = 0;
			}
		}
		else if (i2c_state == I2C_ACK)
			i2c_bits = 1;
		else if (i2c_state == I2C_SEND)
		{
			if (++i2c_bits == 8) i2c_state = I2C_MASTER_ACK;
		}
		else if (i2c_state == I2C_MASTER_ACK && i2c_bits == 8)
		{
			i2c_bits = 9;
			i2c_next = sda ? I2C_IDLE : I2C_SEND;
			if (!sda) i2c_addr = (i2c_addr + 1) & 0xff;
		}
	}
	else if (i2c_scl && !scl)
	{
		if (i2c_state == I2C_ACK)
		{
			if (!i2c_bits) i2c_out = (i2c_next == I2C_IDLE);
			else
			{
				i2c_state = i2c_next;
				i2c_bits = i2c_byte = 0;
				i2c_out = i2c_state == I2C_SEND ? ((spider_eeprom[i2c_addr] >> 7) & 1) : 1;
			}
		}
		else if (i2c_state == I2C_SEND)
			i2c_out = (spider_eeprom[i2c_addr] >> (7 - i2c_bits)) & 1;
		else if (i2c_state == I2C_MASTER_ACK)
		{
			i2c_out = 1;
			if (i2c_bits == 9)
			{
				i2c_state = i2c_next;
				i2c_bits = 0;
				if (i2c_state == I2C_SEND) i2c_out = (spider_eeprom[i2c_addr] >> 7) & 1;
			}
		}
	}
	i2c_scl = scl;
	i2c_sda = sda;
}

static NVRAM_HANDLER( spider )
{
	if (read_or_write)
		mame_fwrite(file, spider_eeprom, sizeof(spider_eeprom));
	else if (file)
		mame_fread(file, spider_eeprom, sizeof(spider_eeprom));
	else
		memcpy(spider_eeprom, memory_region(REGION_USER1), sizeof(spider_eeprom));
}

static MACHINE_RESET( spider )
{
	spider_bank = spider_pal_index = spider_blit_state = spider_blit_addr = spider_blit_width = 0;
	spider_skip_high = spider_skip_operation = 0;
	i2c_scl = i2c_sda = i2c_out = 1;
	i2c_state = I2C_IDLE;
}

static READ8_HANDLER( spider_mem_r )
{
	if (offset < 0x10000) return spider_ram[offset];
	if (offset < 0x20000) return (spider_bank & 1 ? spider_fg : spider_bg)[offset & 0xffff];
	return memory_region(REGION_CPU1)[offset];
}

static WRITE8_HANDLER( spider_mem_w )
{
	UINT8 *vram = (spider_bank & 1) ? spider_fg : spider_bg;
	UINT32 pc = activecpu_get_pc();
	int paired = spider_skip_high && offset == spider_last_addr + 1 && pc == spider_last_pc;
	int skip = spider_skip_operation;
	spider_skip_high = spider_skip_operation = 0;
	if (!paired && !(offset & 1))
	{
		spider_skip_high = 1;
		spider_last_addr = offset;
		spider_last_pc = pc;
		spider_skip_operation = spider_blit_state || (offset >= 0x20000 && offset < 0x30000);
	}
	if (paired && skip) return;
	if (spider_blit_state)
	{
		if (paired) return;
		if (spider_blit_state == 1)
		{
			spider_blit_addr = offset >> 1;
			spider_blit_state = 2;
		}
		else
		{
			spider_blit_width = (offset >> 1) & 0xff;
			if (!spider_blit_width) spider_blit_width = 80;
			spider_blit_state = 0;
		}
		return;
	}
	if (offset < 0x10000) spider_ram[offset] = data;
	else if (offset < 0x20000) vram[offset & 0xffff] = data;
	else if (offset < 0x30000 && !paired)
	{
		int i, dest = offset & 0xfffe;
		const UINT8 *rom = memory_region(REGION_CPU1);
		for (i = 0; i < spider_blit_width; i++)
		{
			int src = (spider_blit_addr * 2) & 0xfffff;
			if (rom[src]) vram[dest] = rom[src];
			if (rom[(src + 1) & 0xfffff]) vram[(dest + 1) & 0xffff] = rom[(src + 1) & 0xfffff];
			spider_blit_addr = (spider_blit_addr + 1) & 0x7ffff;
			dest = (dest + 2) & 0xffff;
		}
	}
}

static READ8_HANDLER( spider_i2c_r ) { return i2c_out & i2c_sda; }
static WRITE8_HANDLER( spider_i2c_w ) { if (!offset) spider_i2c_write(data); }
static WRITE8_HANDLER( spider_bank_w ) { if (!offset) spider_bank = data; }
static READ8_HANDLER( spider_blit_r ) { spider_blit_state = 1; spider_skip_high = 0; return 0xff; }
static WRITE8_HANDLER( spider_clear_w ) { if (!offset) memset(spider_fg, 0, sizeof(spider_fg)); }
static WRITE8_HANDLER( spider_pal_w )
{
	static UINT8 low;
	int r, g, b, val;
	if (!offset) { low = data; return; }
	val = low | (data << 8);
	if (spider_pal_index)
	{
		spider_palette[spider_pal_index - 1] = val;
		r = val & 31; g = (val >> 5) & 31; b = (val >> 10) & 31;
		palette_set_color(spider_pal_index - 1, (r << 3) | (r >> 2), (g << 3) | (g >> 2), (b << 3) | (b >> 2));
	}
	if (++spider_pal_index == 0x101) spider_pal_index = 0;
}

static ADDRESS_MAP_START( spider_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0xfffff) AM_READWRITE(spider_mem_r, spider_mem_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( spider_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x0000) AM_READ(MRA8_NOP) AM_WRITE(AY8910_control_port_0_w)
	AM_RANGE(0x0002, 0x0002) AM_READWRITE(AY8910_read_port_0_r, AY8910_write_port_0_w)
	AM_RANGE(0x0004, 0x0005) AM_READWRITE(spider_i2c_r, spider_i2c_w)
	AM_RANGE(0x0008, 0x0009) AM_WRITE(spider_pal_w)
	AM_RANGE(0x0010, 0x0011) AM_WRITE(spider_bank_w)
	AM_RANGE(0x0018, 0x0018) AM_READ(spider_blit_r)
	AM_RANGE(0x0019, 0x0019) AM_READ(MRA8_NOP)
	AM_RANGE(0x001a, 0x001b) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x001c, 0x001d) AM_WRITE(spider_clear_w)
	AM_RANGE(0x001e, 0x001f) AM_READ(MRA8_NOP)
ADDRESS_MAP_END

static VIDEO_START( spider )
{
	memset(spider_ram, 0, sizeof(spider_ram));
	memset(spider_bg, 0, sizeof(spider_bg));
	memset(spider_fg, 0, sizeof(spider_fg));
	memset(spider_palette, 0, sizeof(spider_palette));
	return 0;
}

static VIDEO_UPDATE( spider )
{
	int x, y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			int offset = y * 320 + x;
			int pixel = spider_fg[offset] ? spider_fg[offset] : spider_bg[offset];
			plot_pixel(bitmap, x, y, Machine->pens[pixel]);
		}
}

static MACHINE_DRIVER_START( spider )
	MDRV_CPU_ADD(V30, 8000000)
	MDRV_CPU_PROGRAM_MAP(spider_map, 0)
	MDRV_CPU_IO_MAP(spider_io, 0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse, 1)
	MDRV_FRAMES_PER_SECOND(50.080128)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(spider)
	MDRV_NVRAM_HANDLER(spider)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 256)
	MDRV_VISIBLE_AREA(0, 319, 0, 203)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_START(spider)
	MDRV_VIDEO_UPDATE(spider)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(AY8910, 2000000)
	MDRV_SOUND_CONFIG(ay8910_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( twins )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "1.bin", 0x000000, 0x080000, CRC(d5ef7b0d) SHA1(7261dca5bb0aef755b4f2b85a159b356e7ac8219) )
	ROM_LOAD16_BYTE( "2.bin", 0x000001, 0x080000, CRC(8a5392f4) SHA1(e6a2ecdb775138a87d27aa4ad267bdec33c26baa) )
ROM_END

ROM_START( spider )
	ROM_REGION( 0x100000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "20.bin", 0x000001, 0x080000, CRC(25e15f11) SHA1(b728f35c817f60a294e38d66559da8977b94a1f5) )
	ROM_LOAD16_BYTE( "21.bin", 0x000000, 0x080000, CRC(ff224206) SHA1(d8d45850983542e811facc917d016841fc56a97f) )
	ROM_REGION( 0x100, REGION_USER1, 0 )
	ROM_LOAD( "24c02", 0x000, 0x100, CRC(6f710d66) SHA1(1cc6d1134c5b81b7d0913f09c07d73675770d817) )
ROM_END

GAME( 1994, twins, 0, twins, twins, 0, ROT0, "Electronic Devices", "Twins", 0 )
GAME( 1994, spider, 0, spider, twins, 0, ROT0, "Buena Vision", "Spider (Buena Vision, without nudity)", GAME_IMPERFECT_GRAPHICS )
