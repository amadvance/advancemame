/*

note:
is it working properly?
no idea how the colours work
is there actually a sound chip?

it executes some illegal opcodes??

M6808: illegal opcode: address FDC2, op 53
M6808: illegal opcode: address FDC5, op 41
M6808: illegal opcode: address FDC6, op 49
M6808: illegal opcode: address FDCB, op 46
M6808: illegal opcode: address FDCE, op 53
M6808: illegal opcode: address FDD1, op 46
M6808: illegal opcode: address FDD3, op 4C
M6808: illegal opcode: address FDC2, op 53
M6808: illegal opcode: address FDC5, op 41
M6808: illegal opcode: address FDC6, op 49
M6808: illegal opcode: address FDCB, op 46
M6808: illegal opcode: address FDCE, op 53
M6808: illegal opcode: address FDD1, op 46
M6808: illegal opcode: address FDD3, op 4C
M6808: illegal opcode: address FDC2, op 53
M6808: illegal opcode: address FDC5, op 41
M6808: illegal opcode: address FDC6, op 49
M6808: illegal opcode: address FDCB, op 46
M6808: illegal opcode: address FDCE, op 53
M6808: illegal opcode: address FDD1, op 46
M6808: illegal opcode: address FDD3, op 4C


PCB layout
2005-07-26
f205v

?Poker?
----------
|----------------------|
| Fully boxed = socket |
|----------------------|


| separation = solder


------------------------------------------------------------------------
|                                                                      |
|      E          D          C          B          A                   |
|                                                                      |
| 1 | 74ls08   | 74ls175  | 74ls175  | 74s288   | CD4098BE  | D31B3100 |
|                                                                      |
|                                                                      |
| 2 | 74ls161  | OSCLLTR  | 74ls367  | 74ls05                          |
|                                                                      |
|                         |                                            |
| 3 | 74ls74   | 74ls04   | M                   | 74ls365              |C
|                         | C                                          |O
|                         | 6                                          |N
| 4 | 74ls139  | 74ls32   | 8                   | 74ls365              |N
|                         | 4                                          |E
|                         | 5                                          |C
| 5 | 74ls32   | 74ls32   | P        | 74ls175                         |T
|                         |                                            |O
|                                                                      |R
| 6 | rom A2   | 74ls273  | ram2114  | 74ls157                         |
|                                                                      |2
|                                                                      |2
| 7 | 74ls166  | 74ls245  | ram2114  | 74ls157                         |
|                                                                      |
|   ----------                                                         |
| 8 | rom A11| | 74ls245  | ram2114  | 74ls157                         |
|   ----------                                                         |
|   ----------                                                         |
| 9 | rom A10| | 8017DK-S6802P                                         |
|   ----------                                                         |
|                                                                      |
------------------------------------------------------------------------

*/

#include "driver.h"
#include "vidhrdw/crtc6845.h"

static UINT8 *murogem_videoram;

static ADDRESS_MAP_START( murogem_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x007f) AM_RAM
	AM_RANGE(0x4000, 0x4000) AM_WRITE(crtc6845_address_w)// i think
	AM_RANGE(0x4001, 0x4001) AM_WRITE(crtc6845_register_w)// i think
	AM_RANGE(0x5000, 0x5000) AM_READ(input_port_0_r)
	AM_RANGE(0x5800, 0x5800) AM_READ(input_port_1_r)
	AM_RANGE(0x7000, 0x7000) AM_WRITE(MWA8_NOP) // sound? payout?
	AM_RANGE(0x8000, 0x87ff) AM_RAM AM_BASE(&murogem_videoram)
	AM_RANGE(0xf000, 0xffff) AM_ROM
ADDRESS_MAP_END


INPUT_PORTS_START( murogem )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON8 ) PORT_CODE(KEYCODE_1) PORT_NAME("Bet (Replay)")
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON7 ) PORT_CODE(KEYCODE_2) PORT_NAME("Deal")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON6 ) PORT_CODE(KEYCODE_SPACE) PORT_NAME("Clear Selection")
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_BUTTON5 ) PORT_CODE(KEYCODE_B) PORT_NAME("Select Card 5")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_CODE(KEYCODE_V) PORT_NAME("Select Card 4")
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_CODE(KEYCODE_C) PORT_NAME("Select Card 3")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_CODE(KEYCODE_X) PORT_NAME("Select Card 2")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_CODE(KEYCODE_Z) PORT_NAME("Select Card 1")

	PORT_START
	PORT_DIPNAME( 0x01, 0x01, "Reset" ) // reduces credits to 0 and resets game??
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x60, 0x20, "Scoring" )
	PORT_DIPSETTING(    0x00, "Type 1" )
	PORT_DIPSETTING(    0x20, "Type 2" )
	PORT_DIPSETTING(    0x40, "Type 3" )
	PORT_DIPSETTING(    0x60, "Invalid" )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN1 )
INPUT_PORTS_END



static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	2,
	{ RGN_FRAC(0,2),RGN_FRAC(1,2) },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};

VIDEO_START(murogem)
{
	return 0;
}

VIDEO_UPDATE(murogem)
{
	int xx,yy,count;
	count = 0x000;

	fillbitmap(bitmap, 0, cliprect);

	for (yy=0;yy<32;yy++)
	{
		for(xx=0;xx<32;xx++)
		{
			int tileno = murogem_videoram[count]&0x3f;
			int attr = murogem_videoram[count+0x400]&0x0f;

			drawgfx(bitmap,Machine->gfx[0],tileno,attr,0,0,xx*8,yy*8,cliprect,TRANSPARENCY_PEN,0);

			count++;

		}

	}

}


static MACHINE_DRIVER_START( murogem )
	/* basic machine hardware */
	MDRV_CPU_ADD(M6802,2000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(murogem_map,0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,1)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(0, 256-1, 0, 256-16-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x100)

	MDRV_VIDEO_START(murogem)
	MDRV_VIDEO_UPDATE(murogem)
MACHINE_DRIVER_END


ROM_START( murogem )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "a11-8.8e", 0xf000, 0x0800, CRC(1135345e) SHA1(ae23786a6e2b1b077ce1a183d547af42318ac4d9)  )
	ROM_LOAD( "a10-9.9e", 0xf800, 0x0800, CRC(f96791d9) SHA1(12b85e0f8b20ea9331f8cb2b2cf2a4383bdb8003)  )

	ROM_REGION( 0x400, REGION_GFX1, 0 )
	ROM_LOAD( "a2.6e", 0x000, 0x400, CRC(86e053da) SHA1(b7cdddca273204513c818384860883bf54cf9434)  )

ROM_END


GAME( 198?, murogem, 0, murogem, murogem, 0, ROT0, "unknown", "Muroge Monaco",GAME_NO_SOUND|GAME_WRONG_COLORS )
