/*  Konami M2 Hardware

    Preliminary driver by Ville Linde

    Games on this hardware:
    =======================

    Game                 | Konami ID
    ---------------------------------
    Evil Night           | GN810
    Heat of Eleven '98   | GX703
    Tobe! Polystars      | GX623
    Battle Tryst         | G?636

*/

/*
Tobe! PolyStars
Konami, 1997

This game runs on 3DO-based Konami M2 hardware.

PCB Layout
----------

Top Board

[M]DFUP0882ZAM1
FZ-20B1AK 7BKSA03500 (sticker)
|---------------------------------------------------|
|            |--------------------|    |----------| |
|            |--------------------|    |----------| |
|    2902             |---|  |--------|             |
| AK4309 CY2292S|---| |*2 |  |  3DO   |  |-------|  |
|               |*1 | |---|  |        |  |IBM    |  |
|        18MHz  |---|        |        |  |POWERPC|  |
|                            |        |  |602    |  |
|                            |--------|  |-------|  |
|    D4516161  D4516161                             |
|                                 |---|  |-------|  |
|DSW                    |-------| |*3 |  |IBM    |  |
|                       |       | |---|  |POWERPC|  |
|    D4516161  D4516161 |  *4   |        |602    |  |
|                       |       |        |-------|  |
|                       |-------|                   |
|---------------------------------------------------|
Notes:
      AK4309  - Asahi Kasei Microsystems AK4309-VM Digital to Analog Converter (SOIC24)
      2902    - Japan Radio Co. JRC2902 Quad Operational Amplifier (SOIC14)
      CY2292S - Cypress CY2292S Three-PLL General-Purpose EPROM Programmable Clock Generator (SOIC16)
                XTALIN - 18.000MHz, XTALOUT - 18.000MHz, XBUF - 18.000MHz, CPUCLK - 25.2000MHz
                CLKA - , CLKB -  , CLKC - 16.9345MHz, CLKD -
      3DO     - 9701 B861131 VY21118- CDE2 3DO 02473-001-0F (QFP208)
      *1      - [M] JAPAN ASUKA 9651HX001 044 (QFP44)
      *2      - Motorola MC44200FT
      *3      - [M] BIG BODY 2 BU6244KS 704 157 (QFP56)
      *4      - Unknown BGA chip (Graphics Engine, with heatsink attached)
      DSW     - 2 position dip switch


Bottom Board

PWB403045B (C) 1997 KONAMI CO., LTD.
|----------------------------------------------------------|
|           CN16    |--------------------|    |----------| |
|LA4705             |--------------------|    |----------| |
|       NJM5532D                    9.83MHz                |
|                                   19.66MHz               |
|J                |--------|   93C46.7K                    |-|
|A                | 058232 |                623B01.8Q      | |
|M                |--------|   |------|                    | |
|M       |------|              |003461|                    | |
|A       |056879|              |      |                    | |CN15
|        |      |              |------|                    | |
| TEST   |------|                                          | |
|                                                          | |
|   DSW                                                    | |
|                                                          |-|
|                                                          |
|----------------------------------------------------------|
Notes:
      056879   - Konami custom IC, location 10E (QFP120)
      058232   - Konami custom ceramic flat pack IC, DAC?
      003461   - Konami custom IC, location 11K (QFP100)
      CN16     - 4 pin connector for CD-DA in from CDROM
      CN15     - Standard (PC-compatible) 40 pin IDE CDROM flat cable connector and 4 pin power plug connector,
                 connected to Panasonic CR-583 8-speed CDROM drive.
                 CDROM disc contains 1 data track & 24 audio tracks. Total Capacity: 581 MBytes
                 Konami part number: 003894
                 Software revision: 623JAA02
      LA4705   - LA4705 Power Amplifier
      623B01.8Q- 16MBit MASKROM. Location 8Q (DIP42)
      93C46.7K - 128bytes x8bit Serial EEPROM. Location 7K (DIP8)
      DSW      - 8 position dip switch

*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"

static UINT64 *main_ram;

static UINT32 vdl0_address;
static UINT32 vdl1_address;

VIDEO_START( m2 )
{
	return 0;
}

VIDEO_UPDATE( m2 )
{
	int i, j;

	UINT32 fb_start = 0xffffffff;
	if (vdl0_address != 0)
	{
		fb_start = *(UINT32*)&main_ram[(vdl0_address - 0x40000000) / 8] - 0x40000000;
	}



	if (fb_start <= 0x800000)
	{
		UINT16 *frame = (UINT16*)&main_ram[fb_start/8];
		for (j=0; j < 384; j++)
		{
			UINT16 *fb = &frame[(j*512)];
			UINT16 *d = bitmap->line[j];
			for (i=0; i < 512; i++)
			{
				d[i^3] = *fb++;
			}
		}
	}
	else
	{
		fillbitmap(bitmap, 0, cliprect);
	}
}

static READ64_HANDLER(unk1_r)
{
	return U64(0xffffffffffffffff);
	//return 0;
}

static READ64_HANDLER(unk2_r)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return (UINT64)0xa5 << 32;
	}
	return 0;
}

static UINT64 unk3 = -1;
static READ64_HANDLER(unk3_r)
{
	//return U64(0xffffffffffffffff);
	return unk3;
}

static READ64_HANDLER(unk4_r)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		return ((UINT64)0 << (13+32)) | ((UINT64)5 << (10+32));
	}
	return 0;
}

static int counter1 = 0;
static READ64_HANDLER(unk30000_r)
{
	counter1++;
	return (UINT64)(counter1 & 0x7f) << 32;
}

static READ64_HANDLER(unk4000280_r)
{
	//return (UINT64)(0x03640000) << 32;
	//return (UINT64)(0x0364ffff) << 32;
	UINT32 sys_config = 0x03640000;

	sys_config |= 0x00000000;		// Bit 0:       PAL/NTSC switch (default is selected by encoder)
	sys_config |= 0 << 2;			// Bit 2-3:     Video Encoder (0 = MEIENC, 1 = VP536, 2 = BT9103, 3 = DENC)
	sys_config |= 0 << 11;			// Bit 11-12:   Country
	sys_config |= 3 << 15;
	sys_config |= 1 << 29;			// Bit 29-30:   Audio chip (1 = CS4216, 3 = Asahi AK4309)

	return (UINT64)(sys_config) << 32;

}

static WRITE64_HANDLER(unk4000010_w)
{
	if ((data & 0xff) == 0xd)
	{
		printf("\n");
	}
	else
	{
		printf("%c", (UINT8)(data & 0xff));
	}
}

static WRITE64_HANDLER(unk4000418_w)
{
}

static WRITE64_HANDLER(reset_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		if (data & U64(0x100000000))
		{
			cpunum_set_input_line(0, INPUT_LINE_RESET, PULSE_LINE);
			unk3 = 0;
		}
	}
}

static WRITE64_HANDLER(video_w)
{
	if (!(mem_mask & U64(0xffffffff00000000)))
	{
		vdl0_address = (UINT32)(data >> 32);
	}
	if (!(mem_mask & U64(0x00000000ffffffff)))
	{
		vdl1_address = (UINT32)(data);
	}
}

static ADDRESS_MAP_START( m2_main, ADDRESS_SPACE_PROGRAM, 64 )
	AM_RANGE(0x00020000, 0x00020007) AM_READ(unk4_r)
//  AM_RANGE(0x00020000, 0x00020007) AM_WRITENOP
	AM_RANGE(0x00030000, 0x00030007) AM_READ(unk30000_r)
	AM_RANGE(0x00030010, 0x00030017) AM_WRITE(video_w)
	AM_RANGE(0x04000010, 0x04000017) AM_WRITE(unk4000010_w)
	AM_RANGE(0x04000018, 0x0400001f) AM_READ(unk1_r)
	AM_RANGE(0x04000020, 0x04000027) AM_WRITE(reset_w)
	AM_RANGE(0x04000418, 0x0400041f) AM_WRITE(unk4000418_w)
	AM_RANGE(0x04000208, 0x0400020f) AM_READ(unk3_r)
	AM_RANGE(0x04000280, 0x04000287) AM_READ(unk4000280_r)
//  AM_RANGE(0x10000000, 0x10000007) AM_RAM
	AM_RANGE(0x20000000, 0x201fffff) AM_ROM AM_SHARE(2)
	AM_RANGE(0x40000000, 0x407fffff) AM_RAM AM_BASE(&main_ram)
	AM_RANGE(0xff000000, 0xff000fff) AM_RAM//AM_READ(unk1_r)
	AM_RANGE(0xfff00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)
ADDRESS_MAP_END

INPUT_PORTS_START( m2 )
INPUT_PORTS_END


static ppc_config ppc602_config =
{
	PPC_MODEL_602,
	0x20,				/* Multiplier 2, Bus = 33MHz, Core = 66MHz */
	BUS_FREQUENCY_33MHZ
};

static INTERRUPT_GEN(m2)
{

	//cpunum_set_input_line(0, INPUT_LINE_IRQ1, ASSERT_LINE);
}

static MACHINE_DRIVER_START( m2 )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC602, 33000000)	/* actually PPC602, 66MHz */
	MDRV_CPU_CONFIG(ppc602_config)
	MDRV_CPU_PROGRAM_MAP(m2_main, 0)
	MDRV_CPU_VBLANK_INT(m2, 1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(640, 480)
	MDRV_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(m2)
	MDRV_VIDEO_UPDATE(m2)

MACHINE_DRIVER_END


#define ROM_REGION64_BE(length,type,flags)	      ROM_REGION(length, type, (flags) | ROMREGION_64BIT | ROMREGION_BE)

ROM_START(polystar)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("623b01.8q", 0x000000, 0x200000, CRC(bd879f93) SHA1(e2d63bfbd2b15260a2664082652442eadea3eab6))

	ROM_REGION( 0x80, REGION_USER2, 0 ) /* serial eeprom */
	ROM_LOAD( "93c46.7k",  0x000000, 0x000080, CRC(66d02984) SHA1(d07c57d198c611b6ff67a783c20a3d038ba34cd1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "623jaa02", 0, MD5(6071c1b70c190fa7c50676eb5308e024) SHA1(a7dc6086d0244b5472f61b41992623c7a9dc2e9c))
ROM_END

ROM_START(btltryst)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "btltryst", 0, MD5(4286d25a896d1450705f742cccd26ef2) SHA1(becc606b8480f6a09365b611565d83cfdc82b0b3))
ROM_END

ROM_START(heatof11)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x2000, REGION_USER2, 0 ) /* timekeeper eeprom */
	ROM_LOAD( "dallas.5e",  0x000000, 0x002000, CRC(8611ff09) SHA1(6410236947d99c552c4a1f7dd5fd8c7a5ae4cba1) )

	DISK_REGION( REGION_DISKS )
	DISK_IMAGE( "heatof11", 0, MD5(1eef9f191439f13d1a867629226b0230) SHA1(87599d03aecadf5d119d05173bdef2940bcce515))
ROM_END

ROM_START(evilngt)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x1000, REGION_USER2, 0 ) /* timekeeper eeprom */
	ROM_LOAD( "m48t58y.u1",  0x000000, 0x001000, CRC(169bb8f4) SHA1(55c0bafab5d309fe69156489186e232aa87ca0dd) )
ROM_END

ROM_START(totlvice)
	ROM_REGION64_BE(0x200000, REGION_USER1, 0)	/* boot rom */
	ROM_LOAD16_WORD("636a01.8q", 0x000000, 0x200000, CRC(7b1dc738) SHA1(32ae8e7ddd38fcc70b4410275a2cc5e9a0d7d33b))

	ROM_REGION( 0x100000, REGION_USER2, 0 ) /* sound rom on sub board */
	ROM_LOAD( "639jaa02.bin",  0x000000, 0x100000, CRC(c6163818) SHA1(b6f8f2d808b98610becc0a5be5443ece3908df0b) )
ROM_END

static DRIVER_INIT( polystar )
{
}

static DRIVER_INIT( btltryst )
{
	UINT32 *rom = (UINT32*)memory_region(REGION_USER1);
	rom[(0x16508^4) / 4] = 0x60000000;
}

static DRIVER_INIT( m2 )
{
}


GAME( 1997, polystar,	0,	m2, m2, polystar,		ROT0,	"Konami",	"Tobe! Polystars (ver JAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1997, totlvice,	0,	m2, m2, m2,				ROT0,	"Konami",	"Total Vice (ver JAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, btltryst,	0,	m2,	m2,	btltryst,		ROT0,	"Konami",	"Battle Tryst (ver JAC)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, heatof11,	0,	m2,	m2,	m2,				ROT0,	"Konami",	"Heat of Eleven '98 (ver EAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
GAME( 1998, evilngt,	0,	m2,	m2,	m2,				ROT0,	"Konami",	"Evil Night (ver EAA)", GAME_NOT_WORKING | GAME_NO_SOUND )
