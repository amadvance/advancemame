/*
Double Wings
Mitchell 1993
This game runs on Data East hardware.
PCB Layout
----------
S-NK-3220
DEC-22VO
|---------------------------------------------|
|MB3730 C3403    32.22MHz           MBE-01.16A|
|  Y3014B  KP_03-.16H       77                |
|           M6295                   MBE-00.14A|
|  YM2151                            |------| |
|          Z80      CXK5864          |      | |
| VOL                       VG-02.11B|  52  | |
|        LH5168     CXK5864          |      | |
|                                    |------| |
|                  |------|              28MHz|
|J       KP_02-.10H|      |                   |
|A                 | 141  |         CXK5814   |
|M       MBE-02.8H |      |                   |
|M                 |      |         CXK5814   |
|A                 |------|                   |
|                                   CXK5814   |
|                 KP_01-.5D                   |
|                                   CXK5814   |
|                 CXK5864                     |
| |----|          KP_00-.3D         |------|  |
| |104 |                            | 102  |  |
| |    |          CXK5864           |      |  |
| |----|                            |      |  |
|SW2 SW1 VG-01.1H VG-00.1F          |------|  |
|---------------------------------------------|
Notes:
       102     - Custom encrypted 68000 CPU. Clock 14.000MHz [28/2]
       Z80     - Toshiba TMPZ84C000AP-6 Z80 CPU. Clock 3.58MHz [32.22/9]
       YM2151  - Yamaha YM2151 FM Operator Type-M (OPM) sound chip. Clock 3.58MHz [32.22/9]
       M6295   - Oki M6295 4-channel mixing ADPCM LSI. Clock 1.000MHz [28/28]. Pin 7 HIGH
       LH6168  - Sharp LH6168 8kx8 SRAM (DIP28)
       CXK5814 - Sony CXK5816 2kx8 SRAM (DIP24)
       CXK5864 - Sony CXK5864 8kx8 SRAM (DIP28)
       VG-*    - MMI PAL16L8 (DIP20)
       SW1/SW2 - 8-position DIP switch
       HSync   - 15.6250kHz
       VSync   - 58.4443Hz
       Other DATA EAST Chips
       --------------------------------------
       DATA EAST 52  9235EV 205941 VC5259-0001 JAPAN   (Sprite Generator IC, 128 pin PQFP)
       DATA EAST 102 (M) DATA EAST 250 JAPAN           (Encrypted 68000 CPU, 128 Pin PQFP)
       DATA EAST 141 24220F008                         (Tile Generator IC, 160 pin PQFP)
       DATA EAST 104 L7A0717 9143 (M) DATA EAST        (IO/Protection, 100 pin PQFP)
       Small surface-mounted chip with number scratched off (28 pin SOP), but has number 9303K9200
       A similar chip exists on Capt. America PCB and has the number 77 on it. Possibly the same chip?
	   
 - Main program writes two commands to soundlatch without pause in some places. Should the 104 custom
   chip be handling this through an internal FIFO?
 - should sprites be buffered, is the Deco '77' a '71' or similar?
*/
#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/z80/z80.h"
#include "decocrpt.h"
#include "deco16ic.h"
#include "sound/2151intf.h"
#include "sound/okim6295.h"


static UINT8 dblewing_sound_irq;


static void dblewing_drawsprites(mame_bitmap *bitmap,const rectangle *cliprect)
{
	int offs;

	for (offs = 0x400-4;offs >= 0;offs -= 4)
	{
		int x,y,sprite,colour,multi,mult2,fx,fy,inc,flash,mult,xsize,pri;

		sprite = spriteram16[offs+1];

		y = spriteram16[offs];
		flash=y&0x1000;
		xsize = y&0x0800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		x = spriteram16[offs+2];
		colour = (x >>9) & 0x1f;

		pri = (x&0xc000); // 2 bits or 1?

		switch (pri&0xc000) {
		case 0x0000: pri=0; break;
		case 0x4000: pri=0xf0; break;
		case 0x8000: pri=0xf0|0xcc; break;
		case 0xc000: pri=0xf0|0xcc; break; /*  or 0xf0|0xcc|0xaa ? */
		}

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x0600) >> 9)) - 1;	/* 1x, 2x, 4x, 8x height */

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 320) x -= 512;
		if (y >= 256) y -= 512;
		y = 240 - y;
        x = 304 - x;

		if (x>320) continue;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flip_screen)
		{
			y=240-y;
			x=304-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		mult2 = multi+1;

		while (multi >= 0)
		{
			pdrawgfx(bitmap,Machine->gfx[2],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0,pri);

			if (xsize)
			pdrawgfx(bitmap,Machine->gfx[2],
					(sprite - multi * inc)-mult2,
					colour,
					fx,fy,
					x-16,y + mult * multi,
					cliprect,TRANSPARENCY_PEN,0,pri);


			multi--;
		}
	}
}

static int dblewing_bank_callback(const int bank)
{
	return ((bank>>4) & 0x7) * 0x1000;
}

VIDEO_START(dblewing)
{
	if (deco16_1_video_init())
		return 1;

	deco16_set_tilemap_bank_callback(0,dblewing_bank_callback);
	deco16_set_tilemap_bank_callback(1,dblewing_bank_callback);

	return 0;
}

VIDEO_UPDATE(dblewing)
{

	flip_screen_set( deco16_pf12_control[0]&0x80 );
	deco16_pf12_update(deco16_pf1_rowscroll,deco16_pf2_rowscroll);

	fillbitmap(bitmap,Machine->pens[0x0],cliprect); /* not Confirmed */
	fillbitmap(priority_bitmap,0,NULL);

	deco16_tilemap_2_draw(bitmap,cliprect,0,2);
	deco16_tilemap_1_draw(bitmap,cliprect,0,4);
	dblewing_drawsprites(bitmap,cliprect);
}


/*

cheats.. to make testing a bit easier

; [ Double Wings ]
:dblewing:00000000:FF3C1F:00000009:FFFFFFFF:Infinite Credits
:dblewing:00000000:FF381C:000000F8:FFFFFFFF:1P Rapid Fire
:dblewing:00000000:FF3821:00000002:FFFFFFFF:1P Invincibility
:dblewing:00000000:FF389D:00000008:FFFFFFFF:1P Infinite Lives
:dblewing:00000000:FF389F:00000006:FFFFFFFF:1P Infinite Bombs
:dblewing:00000000:FF38A5:0000001C:0000001C:1P Always Maximum Shot Power
:dblewing:62000000:FF38A5:00000000:00000000:1P Select Weapon
:dblewing:00010000:FF38A5:00000000:00000060:Vulcan
:dblewing:00010000:FF38A5:00000020:00000060:Laser
:dblewing:00010000:FF38A5:00000040:00000060:Break Vulcan
:dblewing:00000000:FF38A7:0000000C:0000000C:1P Always Max Sub Wepon Power
:dblewing:62000000:FF38A7:00000000:00000000:1P Select Sub Weapon
:dblewing:00010000:FF38A7:00000000:000000F0:None
:dblewing:00010000:FF38A7:00000060:000000F0:Missile
:dblewing:00010000:FF38A7:00000070:000000F0:Homing
:dblewing:00000000:FF38B5:00000001:FFFFFFFF:1P Always Have Restart Item
:dblewing:62000000:FF38A1:00000000:FFFFFFFF:1P Select Character
:dblewing:00010000:FF38A1:00000000:FFFFFFFF:Nick (Red)
:dblewing:00010000:FF38A1:00000001:FFFFFFFF:Sophie (Blue)
:dblewing:00010000:FF38A1:00000002:FFFFFFFF:Elan (Yellow)
:dblewing:00000000:FF38DC:000000F8:FFFFFFFF:2P Rapid Fire
:dblewing:00000000:FF38E1:00000002:FFFFFFFF:2P Invincibility
:dblewing:00000000:FF395D:00000008:FFFFFFFF:2P Infinite Lives
:dblewing:00000000:FF395F:00000006:FFFFFFFF:2P Infinite Bombs
:dblewing:00000000:FF3965:0000001C:0000001C:2P Always Maximum Shot Power
:dblewing:62000000:FF3965:00000000:00000000:2P Select Weapon
:dblewing:00010000:FF3965:00000000:00000060:Vulcan
:dblewing:00010000:FF3965:00000020:00000060:Laser
:dblewing:00010000:FF3965:00000040:00000060:Break Vulcan
:dblewing:00000000:FF3967:0000000C:0000000C:2P Always Max Sub Wepon Power
:dblewing:62000000:FF3967:00000000:00000000:2P Select Sub Weapon
:dblewing:00010000:FF3967:00000000:000000F0:None
:dblewing:00010000:FF3967:00000060:000000F0:Missile
:dblewing:00010000:FF3967:00000070:000000F0:Homing
:dblewing:00000000:FF3975:00000001:FFFFFFFF:2P Always Have Restart Item
:dblewing:62000000:FF3961:00000000:FFFFFFFF:2P Select Character
:dblewing:00010000:FF3961:00000000:FFFFFFFF:Nick (Red)
:dblewing:00010000:FF3961:00000001:FFFFFFFF:Sophie (Blue)
:dblewing:00010000:FF3961:00000002:FFFFFFFF:Elan (Yellow)

*/

/* protection.. involves more addresses than this .. */
/* this is going to be typical deco '104' protection...
 writes one place, reads back data shifted in another
 the addresses below are the ones seen accessed by the
 game so far...

 we need to log the PC of each read/write and check to
 see if the code makes any of them move obvious
*/
static UINT16 dblwings_008_data;
static UINT16 dblwings_104_data;
static UINT16 dblwings_406_data;
static UINT16 dblwings_608_data;
static UINT16 dblwings_70c_data;
static UINT16 dblwings_78a_data;
static UINT16 dblwings_088_data;
static UINT16 dblwings_58c_data;
static UINT16 dblwings_408_data;
static UINT16 dblwings_40e_data;
static UINT16 dblwings_080_data;
static UINT16 dblwings_788_data;
static UINT16 dblwings_38e_data;
static UINT16 dblwings_580_data;
static UINT16 dblwings_60a_data;
static UINT16 dblwings_200_data;
static UINT16 dblwings_28c_data;
static UINT16 dblwings_18a_data;
static UINT16 dblwings_280_data;
static UINT16 dblwings_384_data;

static UINT16 boss_move,boss_shoot_type,boss_3_data,boss_4_data,boss_5_data,boss_5sx_data,boss_6_data;

static READ16_HANDLER ( dblewing_prot_r )
{
	switch(offset*2)
	{
		case 0x16a: return boss_move;          // boss 1 movement
		case 0x6d6: return boss_move;          // boss 1 2nd pilot
		case 0x748: return boss_move;          // boss 1 3rd pilot

		case 0x566: return 0x0009;   	   	   // boss BGM,might be a variable one (read->write to the sound latch)
		case 0x1ea: return boss_shoot_type;    // boss 1 shoot type
		case 0x596: return boss_3_data;		   // boss 3 appearing
		case 0x692:	return boss_4_data;
		case 0x6b0: return boss_5_data;
		case 0x51e: return boss_5sx_data;
		case 0x784: return boss_6_data;

		case 0x330: return 0; // controls bonuses such as shoot type,bombs etc.
		case 0x1d4: return dblwings_70c_data;  //controls restart points

		case 0x0ac: return (readinputportbytag("DSW") & 0x40)<<4;//flip screen
		case 0x4b0: return dblwings_608_data;//coinage
		case 0x068:
		{
			switch(readinputportbytag("DSW") & 0x0300) //I don't know how to relationate this...
			{
				case 0x0000: return 0x000;//0
				case 0x0100: return 0x060;//3
				case 0x0200: return 0x0d0;//6
				case 0x0300: return 0x160;//b
			}
		}
		case 0x094: return dblwings_104_data;// p1 inputs select screen  OK
		case 0x24c: return dblwings_008_data;//read DSW (mirror for coinage/territory)
		case 0x298: return readinputportbytag("SYSTEM");//vblank
		case 0x476: return readinputportbytag("SYSTEM");//mirror for coins
		case 0x506: return readinputportbytag("DSW");
		case 0x5d8: return dblwings_406_data;
		case 0x2b4: return readinputportbytag("P1_P2");
		case 0x1a8: return (readinputportbytag("DSW") & 0x4000) >> 12;//allow continue
		case 0x3ec: return dblwings_70c_data; //score entry
		case 0x246: return dblwings_580_data; // these three controls "perfect bonus" I suppose...
		case 0x52e: return dblwings_580_data;
		case 0x532: return dblwings_580_data;
	}


//  printf("dblewing prot r %08x, %04x, %04x\n",cpu_get_pc(space->cpu), offset*2, mem_mask);

	if ((offset*2)==0x0f8) return 0; // dblwings_080_data;
	if ((offset*2)==0x104) return 0;
	if ((offset*2)==0x10e) return 0;
	if ((offset*2)==0x206) return 0; // dblwings_70c_data;
	if ((offset*2)==0x25c) return 0;
	if ((offset*2)==0x284) return 0; // 3rd player 2nd boss
	if ((offset*2)==0x432) return 0; // boss on water level?
	if ((offset*2)==0x54a) return 0; // 3rd player 2nd boss
	if ((offset*2)==0x786) return 0;

	printf("dblewing prot r %08x, %04x, %04x\n",activecpu_get_pc(), offset*2, mem_mask);

	return 0;//mame_rand();
}

static WRITE16_HANDLER( dblewing_prot_w )
{
//  if(offset*2 != 0x380)
//  printf("dblewing prot w %08x, %04x, %04x %04x\n",cpu_get_pc(space->cpu), offset*2, mem_mask,data);

	switch(offset*2)
	{
		case 0x088:
			dblwings_088_data = data;
			if(dblwings_088_data == 0)          { boss_4_data = 0;    }
			else if(dblwings_088_data & 0x8000) { boss_4_data = 0x50; }
			else                                { boss_4_data = 0x40; }

			return;

		case 0x104:
			dblwings_104_data = data;
			return; // p1 inputs select screen  OK

		case 0x18a:
			dblwings_18a_data = data;
			switch(dblwings_18a_data)
			{
				case 0x6b94: boss_5_data = 0x10; break; //initialize
				case 0x7c68: boss_5_data = 0x60; break; //go up
				case 0xfb1d: boss_5_data = 0x50; break;
				case 0x977c: boss_5_data = 0x50; break;
				case 0x8a49: boss_5_data = 0x60; break;
			}
			return;
		case 0x200:
			dblwings_200_data = data;
			switch(dblwings_200_data)
			{
				case 0x5a19: boss_move = 1; break;
				case 0x3b28: boss_move = 2; break;
				case 0x1d4d: boss_move = 1; break;
			}
			//popmessage("%04x",dblwings_200_data);
			return;
		case 0x280:
			dblwings_280_data = data;
			switch(dblwings_280_data)
			{
				case 0x6b94: boss_5sx_data = 0x10; break;
				case 0x7519: boss_5sx_data = 0x60; break;
				case 0xfc68: boss_5sx_data = 0x50; break;
				case 0x02dd: boss_5sx_data = 0x50; break;
				case 0x613c: boss_5sx_data = 0x50; break;
			}
			//printf("%04x\n",dblwings_280_data);
			return;
		case 0x380: // sound write
			soundlatch_w(0,data&0xff);
			dblewing_sound_irq |= 0x02;
		 	cpunum_set_input_line(1,0,(dblewing_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
			return;
		case 0x384:
			dblwings_384_data = data;
			switch(dblwings_384_data)
			{
				case 0xaa41: boss_6_data = 1; break;
				case 0x5a97: boss_6_data = 2; break;
				case 0xbac5: boss_6_data = 3; break;
				case 0x0afb: boss_6_data = 4; break;
				case 0x6a99: boss_6_data = 5; break;
				case 0xda8f: boss_6_data = 6; break;
			}
			return;
		case 0x38e:
			dblwings_38e_data = data;
			switch(dblwings_38e_data)
			{
				case 0x6c13: boss_shoot_type = 3; break;
				case 0xc311: boss_shoot_type = 0; break;
				case 0x1593: boss_shoot_type = 1; break;
				case 0xf9db: boss_shoot_type = 2; break;
				case 0xf742: boss_shoot_type = 3; break;

				case 0xeff5: boss_move = 1; break;
				case 0xd2f1: boss_move = 2; break;
				//default:   printf("%04x\n",dblwings_38e_data); break;
				//case 0xe65a: boss_shoot_type = 0; break;
			}
			return;
		case 0x58c: // 3rd player 1st level
			dblwings_58c_data = data;
			if(dblwings_58c_data == 0)     { boss_move = 5; }
			else                           { boss_move = 2; }

			return;
		case 0x60a:
			dblwings_60a_data = data;
			if(dblwings_60a_data & 0x8000) { boss_3_data = 2; }
			else                           { boss_3_data = 9; }

			return;
		case 0x580:
			dblwings_580_data = data;
			return;
		case 0x406:
			dblwings_406_data = data;
			return;  // p2 inputs select screen  OK
	}

//  printf("dblewing prot w %08x, %04x, %04x %04x\n",cpu_get_pc(space->cpu), offset*2, mem_mask,data);

	if ((offset*2)==0x008) { dblwings_008_data = data; return; }
	if ((offset*2)==0x080) { dblwings_080_data = data; return; } // p3 3rd boss?
	if ((offset*2)==0x28c) { dblwings_28c_data = data; return; }
	if ((offset*2)==0x408) { dblwings_408_data = data; return; } // 3rd player 1st level?
	if ((offset*2)==0x40e) { dblwings_40e_data = data; return; } // 3rd player 2nd level?
	if ((offset*2)==0x608) { dblwings_608_data = data; return; }
	if ((offset*2)==0x70c) { dblwings_70c_data = data; return; }
	if ((offset*2)==0x78a) { dblwings_78a_data = data; return; }
	if ((offset*2)==0x788) { dblwings_788_data = data; return; }
}




static ADDRESS_MAP_START( dblewing_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x100fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf1_data_w) AM_BASE(&deco16_pf1_data)
	AM_RANGE(0x102000, 0x102fff) AM_READ(MRA16_RAM) AM_WRITE(deco16_pf2_data_w) AM_BASE(&deco16_pf2_data)
	AM_RANGE(0x104000, 0x104fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf1_rowscroll)
	AM_RANGE(0x106000, 0x106fff) AM_READ(MRA16_RAM) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf2_rowscroll)
	AM_RANGE(0x280000, 0x2807ff) AM_READWRITE(dblewing_prot_r,dblewing_prot_w)
	AM_RANGE(0x284000, 0x284001) AM_RAM
	AM_RANGE(0x288000, 0x288001) AM_RAM
	AM_RANGE(0x28C000, 0x28C00f) AM_WRITE(MWA16_RAM) AM_BASE(&deco16_pf12_control)
	AM_RANGE(0x300000, 0x3007ff) AM_RAM AM_BASE(&spriteram16) AM_SIZE(&spriteram_size)
	AM_RANGE(0x320000, 0x3207ff) AM_READWRITE(MRA16_RAM,paletteram16_xxxxBBBBGGGGRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0xff0000, 0xff3fff) AM_MIRROR(0xc000) AM_RAM
ADDRESS_MAP_END


static READ8_HANDLER(irq_latch_r)
{
	/* bit 1 of dblewing_sound_irq specifies IRQ command writes */
	dblewing_sound_irq &= ~0x02;
	cpunum_set_input_line(1,0, (dblewing_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
	return dblewing_sound_irq;
}


static ADDRESS_MAP_START( sound_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
 	AM_RANGE(0xa000, 0xa001) AM_READWRITE(YM2151_status_port_0_r,YM2151_word_0_w)
	AM_RANGE(0xb000, 0xb000) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
	AM_RANGE(0xc000, 0xc000) AM_READ(soundlatch_r)
 	AM_RANGE(0xd000, 0xd000) AM_READ(irq_latch_r) //timing? sound latch?
 	AM_RANGE(0xf000, 0xf000) AM_READWRITE(OKIM6295_status_0_r, OKIM6295_data_0_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dblewing_io_sound, ADDRESS_SPACE_IO, 8 )
	AM_RANGE(0x0000, 0xffff) AM_ROM AM_REGION(REGION_SOUND2, 0)
ADDRESS_MAP_END

static const gfx_layout tile_8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	8*16
};

static const gfx_layout tile_16x16_layout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+8,RGN_FRAC(1,2)+0,RGN_FRAC(0,2)+8,RGN_FRAC(0,2)+0 },
	{ 256,257,258,259,260,261,262,263,0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,8*16,9*16,10*16,11*16,12*16,13*16,14*16,15*16 },
	32*16
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 24,8,16,0 },
	{ 512,513,514,515,516,517,518,519, 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32,
	  8*32, 9*32,10*32,11*32,12*32,13*32,14*32,15*32},
	32*32
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tile_8x8_layout,     0x000, 32 },	/* Tiles (8x8) */
	{ REGION_GFX1, 0, &tile_16x16_layout,   0x000, 32 },	/* Tiles (16x16) */
	{ REGION_GFX2, 0, &spritelayout,        0x200, 32 },	/* Sprites (16x16) */
	{ -1 } /* end of array */
};

INPUT_PORTS_START( dblewing )
	PORT_START_TAG("P1_P2")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_VBLANK )

	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x0007, 0x0007, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0038, 0x0038, DEF_STR( Coin_B ) ) 
	PORT_DIPSETTING(      0x0000, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_5C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Flip_Screen ) ) 
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "Territory" )  /*Manual says "don't change this" */
	PORT_DIPSETTING(      0x0080, "Japan" )
	PORT_DIPSETTING(      0x0000, "Korea" )
	/* 16bit - These values are for Dip Switch #2 */
	PORT_DIPNAME( 0x0300, 0x0300, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0100, "1" )
	PORT_DIPSETTING(      0x0000, "2" )
	PORT_DIPSETTING(      0x0300, "3" )
	PORT_DIPSETTING(      0x0200, "5" )
	PORT_DIPNAME( 0x0c00, 0x0c00, "Difficulty" )
	PORT_DIPSETTING(      0x0800, "Easy" ) 
	PORT_DIPSETTING(      0x0c00, "Normal" )
	PORT_DIPSETTING(      0x0400, "Hard" )
	PORT_DIPSETTING(      0x0000, "Hardest" ) 
	PORT_DIPNAME( 0x3000, 0x3000, DEF_STR( Bonus_Life ) ) 
	PORT_DIPSETTING(      0x2000, "Every 100,000" )
	PORT_DIPSETTING(      0x3000, "Every 150,000" )
	PORT_DIPSETTING(      0x1000, "Every 300,000" )
	PORT_DIPSETTING(      0x0000, "250,000 Only" )
	PORT_DIPNAME( 0x4000, 0x4000, "Allow_Continue" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x0000, DEF_STR( Demo_Sounds ) ) 
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START_TAG("UNK")
	PORT_DIPNAME( 0x0001, 0x0001, "2" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0004, 0x0004, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0008, 0x0000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0100, 0x0100, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0100, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0400, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x2000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x2000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x8000, 0x8000, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x8000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static void sound_irq_dblewing( int state)
{
	/* bit 0 of dblewing_sound_irq specifies IRQ from sound chip */
	if (state)
		dblewing_sound_irq |= 0x01;
	else
		dblewing_sound_irq &= ~0x01;
	cpunum_set_input_line(1,0, (dblewing_sound_irq != 0) ? ASSERT_LINE : CLEAR_LINE);
}

static struct YM2151interface ym2151_interface =
{
	sound_irq_dblewing
};


static MACHINE_DRIVER_START( dblewing )
	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 14000000)	/* DECO102 */
	MDRV_CPU_PROGRAM_MAP(dblewing_map,0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold,1)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(sound_map,0)
	MDRV_CPU_IO_MAP(dblewing_io_sound,0)

        MDRV_INTERLEAVE(100)	/* high interleave to ensure proper synchronization of CPUs */
	MDRV_FRAMES_PER_SECOND(58)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(64*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 1*8, 31*8-1)
	MDRV_PALETTE_LENGTH(4096)
	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_START(dblewing)
	MDRV_VIDEO_UPDATE(dblewing)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 32220000/9)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.40)

	MDRV_SOUND_ADD(OKIM6295, 32220000/32/132)
	MDRV_SOUND_CONFIG(okim6295_interface_region_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)
MACHINE_DRIVER_END


/*

Double Wings (JPN Ver.)
(c)1993 Mitchell
DEC-22V0 (S-NK-3220)

Software is by Mitchell, but the PCB is pure Data East.

Data East ROM code = KP


CPU     :DE102 - Encrypted 68000
Sound   :TMPZ84C00AP-6,YM2151,OKI M6295, YM3014B
OSC     :28.0000MHz,32.2200MHz
RAM     :LH6168 x 1, CXK5814 x 6, CXK5864 x 4
DIP     :2 x 8 position
Other   :DATA EAST Chips (numbers scratched)
         --------------------------------------
         DATA EAST #?  9235EV 205941  VC5259-0001 JAPAN (confirmed #52) - 128 pin PQFP
         DATA EAST #?  DATA EAST 250 JAPAN (#102, the CPU) - 128 Pin PQFP
         DATA EAST #?  24220F008 (confirmed #141) - 160 pin PQFP
         DATA EAST #?  L7A0717   9143  (confirmed #104, IO/Protection) - 100 pin PQFP

         PALs: PAL16L8 (x 2, VG-00, VG-01) between program ROMs and CPU
               PAL16L8 (x 1, VG-02) next to #52

         Small surface-mounted chip with number scratched off (28 pin SOP), but has number 9303K9200.
         A similar chip exists on Capt. America PCB and has the number 77 on it. Possibly the same chip?

KP_00-.3D    [547dc83e]
KP_01-.5D    [7a210c33]

KP_02-.10H   [def035fa]

KP_03-.16H   [5d7f930d]

MBE-00.14A   [e33f5c93]
MBE-01.16A   [ef452ad7]
MBE-02.8H    [5a6d3ac5]

*/

ROM_START( dblewing )
	ROM_REGION( 0x80000, REGION_CPU1, 0 ) /* DECO102 code (encrypted) */
	ROM_LOAD16_BYTE( "kp_00-.3d",    0x000001, 0x040000, CRC(547dc83e) SHA1(f6f96bd4338d366f06df718093f035afabc073d1) )
	ROM_LOAD16_BYTE( "kp_01-.5d",    0x000000, 0x040000, CRC(7a210c33) SHA1(ced89140af6d6a1bc0ffb7728afca428ed007165) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 ) /* sound cpu*/
	ROM_LOAD( "kp_02-.10h",    0x00000, 0x08000, CRC(def035fa) SHA1(fd50314e5c94c25df109ee52c0ce701b0ff2140c) )
        ROM_CONTINUE(              0x10000, 0x08000 )

        ROM_REGION( 0x10000, REGION_SOUND2, 0 ) /* sound data*/
	ROM_COPY( REGION_CPU2,  0x00000, 0x00000, 0x8000 )
	ROM_COPY( REGION_CPU2,  0x10000, 0x08000, 0x8000 )
	
	ROM_REGION( 0x100000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "mbe-02.8h",    0x00000, 0x100000, CRC(5a6d3ac5) SHA1(738bb833e2c5d929ac75fe4e69ee0af88197d8a6) )

	ROM_REGION( 0x200000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD16_BYTE( "mbe-00.14a",    0x000000, 0x100000, CRC(e33f5c93) SHA1(720904b54d02dace2310ac6bd07d5ed4bc4fd69c) )
	ROM_LOAD16_BYTE( "mbe-01.16a",    0x000001, 0x100000, CRC(ef452ad7) SHA1(7fe49123b5c2778e46104eaa3a2104ce09e05705) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 ) /* Oki samples */
	ROM_LOAD( "kp_03-.16h",    0x00000, 0x20000, CRC(5d7f930d) SHA1(ad23aa804ea3ccbd7630ade9b53fc3ea2718a6ec) )
	ROM_RELOAD(                0x20000, 0x20000 )
	ROM_RELOAD(                0x40000, 0x20000 )
	ROM_RELOAD(                0x60000, 0x20000 )
ROM_END

extern void deco102_decrypt(int region, int address_xor, int data_select_xor, int opcode_select_xor);

static DRIVER_INIT( dblewing )
{
	deco56_decrypt(REGION_GFX1);
	deco102_decrypt(REGION_CPU1, 0x399d, 0x25, 0x3d);
}


GAME(1993, dblewing, 0,  dblewing, dblewing, dblewing,  ROT90, "Mitchell", "Double Wings", GAME_UNEMULATED_PROTECTION | GAME_IMPERFECT_SOUND )
