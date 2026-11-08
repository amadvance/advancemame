// license:BSD-3-Clause
// copyright-holders:David Haywood
/* Table Tennis Champions
   (c) 1995 Gamart

 ___________________________________________________
|        __     _________ __________   __________  |
|__      |_|   |UM62256D| |UM62256D|   |UM62256D|  |
  |    LN358N  |________| |________|   |________|  |
 _|            _________ _________     __________  |
|_  _1_______  |TI     | |TI     |     |UM62256D|  |
|_  |27C020 |  |52C1HXW| |52ALROW|     |________|  |
|_  |_______|  |       | |       |     __________  |
|__            |       | |       |     |UM62256D|  |
 _| __         |_______| |_______|     |________|  |
|_  |_| 74HC74                                     |
|_  OKI        74LS244   _3_______  _5_______      |
|_  M6295      74LS244   |27C040 |  |27C040 |      |
|_    57C55LK            |_______|  |_______|      |
|_             ________  _2_______  _4_______      |
|_    57C55LK  |GM76C28| |27C040 |  |27C040 |      |
|_             |_______| |_______|  |_______|      |
|_    74LS244  ________                            |
|_             |GM76C28| PAL16L8B PAL16L8B 74LS245 |
|_             |_______|                           |
|_                       74LS244  74LS373N 74LS373 |
|_    74LS244  PIC16C84                            |
|_                       74HC273N 74LS373N 74LS245 |
|_             74HC74B1              _____________ |
  |                            OSC   |NEC V30    | |
 _|   74LS244  74LS14N 74HC74  16MHz |D70116C-10 | |
|__________________________________________________|

The PCB is Spanish and manufactured by Gamart.


--- hw is similar to hotblock and twins


*/

#include "driver.h"
#include "sound/okim6295.h"

static UINT8 mainram[0x10000];
static UINT8 vram[2][0x10000];
static UINT8 backram[0x100];
static UINT8 *rom;
static UINT16 paloff, paldata, port10;
static int rombank, spritesinit, spriteswidth;
static UINT32 spritesaddr, pair_address, pair_pc;
static int pair_pending;
static int picmode, pic_readaddr, pic_writeaddr;
static UINT8 pic_latched, pic_writelatched;
static UINT16 pic_data;
static int pic_pending;

enum { PIC_IDLE, PIC_READADDRESS, PIC_WRITEADDRESS, PIC_WRITELATCH, PIC_READLATCH };

static int video_bank(void)
{
	return (port10 & 0x0f) != 0;
}

static VIDEO_START( ttchamp )
{
	return 0;
}

static VIDEO_UPDATE( ttchamp )
{
	int x, y;
	for (y = MAX(0, cliprect->min_y); y <= MIN(203, cliprect->max_y); y++)
	{
		UINT16 pixels[320];
		for (x = 0; x < 320; x++)
		{
			int offset = y * 320 + x;
			int fg = vram[1][offset];
			int bg = vram[0][offset];
			pixels[x] = fg == 0 ? bg + 0x300 :
				(fg == 1 ? bg + 0x200 : (fg == 2 ? bg + 0x100 : fg));
		}
		draw_scanline16(bitmap, 0, y, 320, pixels, Machine->pens, -1);
	}
}

static WRITE8_HANDLER( mem_w )
{
	UINT32 wordoffs = offset >> 1;
	UINT8 *dest = vram[video_bank()];
	int i;

	/* Coalesce the two byte callbacks made by a V30 word store. */
	if (pair_pending && offset == pair_address && activecpu_get_pc() == pair_pc)
	{
		pair_pending = 0;
		return;
	}
	pair_pending = 0;

	if (spritesinit == 1 || spritesinit == 2)
	{
		if (spritesinit == 1)
			spritesaddr = wordoffs;
		else
			spriteswidth = wordoffs & 0xff;
		spritesinit++;
	}
	else if (offset < 0x10000)
	{
		mainram[offset] = data;
		return;
	}
	else if (offset < 0x20000)
	{
		dest[offset & 0xffff] = data;
		return;
	}
	else if (offset >= 0x30000 && offset < 0x40000 && spritesinit == 3)
	{
		spritesinit = 0;
		wordoffs &= 0x7fff;
		for (i = 0; i < spriteswidth; i++)
		{
			int target = (wordoffs++ & 0x7fff) * 2;
			if (port10 & 0x30)
			{
				if (port10 & 0x10)
					vram[0][target] = vram[0][target + 1] = 0;
				if (port10 & 0x20)
					vram[1][target] = vram[1][target + 1] = 0;
			}
			else
			{
				UINT32 source = (rombank * 0x100000) + ((spritesaddr++ * 2) & 0xfffff);
				UINT8 lo = rom[source];
				UINT8 hi = rom[source + 1];
				if (lo || !(port10 & 2)) dest[target] = lo;
				if (hi || !(port10 & 2)) dest[target + 1] = hi;
			}
		}
	}
	else
		return;

	pair_address = offset + 1;
	pair_pc = activecpu_get_pc();
	pair_pending = 1;
}

static READ8_HANDLER( blit_start_r )
{
	spritesinit = 1;
	pair_pending = 0;
	return (offset & 1) ? 0 : 0xff;
}

static READ8_HANDLER( blit_repeat_r )
{
	spritesinit = 3;
	pair_pending = 0;
	return (offset & 1) ? 0 : 0xff;
}

static WRITE8_HANDLER( paloff_w )
{
	if (offset & 1) paloff = (paloff & 0x00ff) | (data << 8);
	else paloff = (paloff & 0xff00) | data;
}

static WRITE8_HANDLER( paldat_w )
{
	if (offset & 1) paldata = (paldata & 0x00ff) | (data << 8);
	else paldata = (paldata & 0xff00) | data;
	palette_set_color(paloff & 0x3ff, pal5bit(paldata), pal5bit(paldata >> 5), pal5bit(paldata >> 10));
}

static WRITE8_HANDLER( port10_w )
{
	if (offset & 1) port10 = (port10 & 0x00ff) | (data << 8);
	else port10 = (port10 & 0xff00) | data;
	memory_set_bankptr(2, vram[video_bank()]);
}

static WRITE8_HANDLER( port20_w )
{
	rombank = 1;
}
static WRITE8_HANDLER( port62_w )
{
	rombank = 0;
}

static void pic_commit(int param)
{
	UINT16 data = pic_data;
	if (!pic_pending)
		return;
	pic_pending = 0;
	if (picmode == PIC_READADDRESS)
	{
		pic_readaddr = data & 0x1ff;
		picmode = PIC_IDLE;
	}
	else if (picmode == PIC_WRITEADDRESS)
	{
		pic_writeaddr = data & 0xff;
		picmode = PIC_IDLE;
	}
	else if (picmode == PIC_WRITELATCH)
	{
		pic_writelatched = data;
		picmode = PIC_IDLE;
	}
	else if (picmode == PIC_IDLE)
	{
		switch (data)
		{
			case 0x11: picmode = PIC_READADDRESS; break;
			case 0x12: picmode = PIC_WRITELATCH; break;
			case 0x20: picmode = PIC_WRITEADDRESS; break;
			case 0x21: backram[pic_writeaddr] = pic_writelatched; break;
			case 0x22:
				pic_latched = backram[pic_readaddr >> 1];
				picmode = PIC_READLATCH;
				break;
		}
	}
}

static WRITE8_HANDLER( pic_w )
{
	if (!(offset & 1))
	{
		/* Defer the command until both halves of an OUT word have arrived. */
		pic_data = data;
		pic_pending = 1;
		timer_set(TIME_NOW, 0, pic_commit);
	}
	else if (pic_pending)
		pic_data |= data << 8;
}

static READ8_HANDLER( pic_r )
{
	if ((offset & 1) && picmode == PIC_READLATCH)
	{
		picmode = PIC_IDLE;
		return pic_latched;
	}
	return 0;
}

static ADDRESS_MAP_START( ttchamp_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x0ffff) AM_READ(MRA8_BANK1)
	AM_RANGE(0x10000, 0x1ffff) AM_READ(MRA8_BANK2)
	AM_RANGE(0x20000, 0xfffff) AM_READ(MRA8_BANK3)
	AM_RANGE(0x00000, 0xfffff) AM_WRITE(mem_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ttchamp_io, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0x0001) AM_WRITENOP
	AM_RANGE(0x0002, 0x0002) AM_READ(input_port_0_r)
	AM_RANGE(0x0003, 0x0003) AM_READ(input_port_1_r)
	AM_RANGE(0x0004, 0x0004) AM_READ(input_port_2_r)
	AM_RANGE(0x0005, 0x0005) AM_READ(input_port_3_r)
	AM_RANGE(0x0006, 0x0006) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0x0008, 0x0009) AM_WRITE(paldat_w)
	AM_RANGE(0x000a, 0x000b) AM_WRITE(paloff_w)
	AM_RANGE(0x0010, 0x0011) AM_WRITE(port10_w)
	AM_RANGE(0x0018, 0x0019) AM_READ(blit_start_r)
	AM_RANGE(0x001e, 0x001f) AM_READ(blit_repeat_r)
	AM_RANGE(0x0020, 0x0021) AM_WRITE(port20_w)
	AM_RANGE(0x0034, 0x0035) AM_READWRITE(pic_r, pic_w)
	AM_RANGE(0x0062, 0x0063) AM_WRITE(port62_w)
ADDRESS_MAP_END

static MACHINE_START( ttchamp )
{
	rom = memory_region(REGION_USER1);
	memory_set_bankptr(1, mainram);
	memory_set_bankptr(2, vram[0]);
	memory_set_bankptr(3, rom + 0x120000);
	generic_nvram = backram;
	generic_nvram_size = sizeof(backram);
	return 0;
}

static MACHINE_RESET( ttchamp )
{
	memset(mainram, 0, sizeof(mainram));
	memset(vram, 0, sizeof(vram));
	paloff = paldata = port10 = 0;
	memory_set_bankptr(2, vram[0]);
	rombank = spritesinit = spriteswidth = 0;
	spritesaddr = pair_address = pair_pc = 0;
	pair_pending = pic_pending = 0;
	picmode = PIC_IDLE;
	pic_readaddr = pic_writeaddr = 0;
	pic_latched = pic_writelatched = 0;
	pic_data = 0;
}

INPUT_PORTS_START(ttchamp)
	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8bit */
	PORT_DIPNAME( 0x01, 0x01, "0x0003" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
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
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )

	PORT_START	/* 8bit */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

INPUT_PORTS_END

static INTERRUPT_GEN( ttchamp_irq )
{
	cpunum_set_input_line(0, INPUT_LINE_NMI, PULSE_LINE);
}

static MACHINE_DRIVER_START( ttchamp )
	MDRV_CPU_ADD(V30, 8000000)
	MDRV_CPU_PROGRAM_MAP(ttchamp_map,0)
	MDRV_CPU_IO_MAP(ttchamp_io,0)
	MDRV_CPU_VBLANK_INT(ttchamp_irq,1)
	MDRV_MACHINE_START(ttchamp)
	MDRV_MACHINE_RESET(ttchamp)
	MDRV_NVRAM_HANDLER(generic_0fill)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320,204)
	MDRV_VISIBLE_AREA(0,319,0,199)
	MDRV_PALETTE_LENGTH(0x400)
	MDRV_VIDEO_START(ttchamp)
	MDRV_VIDEO_UPDATE(ttchamp)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(OKIM6295, 1000000/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( ttchamp )
	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "2.bin", 0x000000, 0x080000,  CRC(6a6c6d75) SHA1(3742b82462176d77732a69e142db9e6f61f25dc5) )
	ROM_LOAD16_BYTE( "3.bin", 0x000001, 0x080000,  CRC(6062c0b2) SHA1(c5f0ac58c847ce2588c805f40180f2586a6477b7) )
	ROM_LOAD16_BYTE( "4.bin", 0x100000, 0x080000,  CRC(4388dead) SHA1(1965e4b84452b244e32c8d218aace8d287c67ec2) )
	ROM_LOAD16_BYTE( "5.bin", 0x100001, 0x080000,  CRC(fdbf9b28) SHA1(2d260555586097c8a396f65111f55ace801c7a5d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pic16c84.rom", 0x000000, 0x4280,  BAD_DUMP CRC(900f2ef8) SHA1(08f206fe52f413437436e4b0d2b4ec310767446c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "27c020.1", 0x000000, 0x040000,  CRC(e2c4fe95) SHA1(da349035cc348db220a1e12b4c2a6021e2168425) )
ROM_END

ROM_START( ttchampa )
	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD16_BYTE( "ttennis2.bin", 0x000000, 0x080000,  CRC(b060e72c) SHA1(376e71bb4b1687fec4b719cbc5a7b25b64d159ac) )
	ROM_LOAD16_BYTE( "ttennis3.bin", 0x000001, 0x080000,  CRC(33e085a8) SHA1(ea6af05690b4b0803c303a3c858df10e4d907fb1) )
	ROM_LOAD16_BYTE( "4.bin", 0x100000, 0x080000,  CRC(4388dead) SHA1(1965e4b84452b244e32c8d218aace8d287c67ec2) )
	ROM_LOAD16_BYTE( "5.bin", 0x100001, 0x080000,  CRC(fdbf9b28) SHA1(2d260555586097c8a396f65111f55ace801c7a5d) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "pic16c84.rom", 0x000000, 0x4280, BAD_DUMP CRC(900f2ef8) SHA1(08f206fe52f413437436e4b0d2b4ec310767446c) )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "27c020.1", 0x000000, 0x040000,  CRC(e2c4fe95) SHA1(da349035cc348db220a1e12b4c2a6021e2168425) )
ROM_END

GAME( 1995, ttchamp, 0, ttchamp, ttchamp, 0, ROT0, "Gamart", "Table Tennis Champions", 0 )
GAME( 1995, ttchampa, ttchamp, ttchamp, ttchamp, 0, ROT0, "Gamart (Palencia Elektronik license)", "Table Tennis Champions (Palencia Elektronik license)", 0 )
