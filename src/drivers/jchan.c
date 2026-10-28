// license:BSD-3-Clause
// copyright-holders:David Haywood
#include "driver.h"
#include "sound/ymz280b.h"
#include "includes/kaneko16.h"

static mame_bitmap *sprite_bitmap_1;
static mame_bitmap *sprite_bitmap_2;

static UINT32* jchan_sprite_ram32_1;
static UINT32* jchan_sprite_ram32_2;
static UINT32* jchan_sprite_regs32_1;
static UINT32* jchan_sprite_regs32_2;

static int jchan_irq_sub_enable;

static UINT16 *jchan_spriteram_1;
static UINT16* jchan_sprregs_1;
static UINT16 *jchan_spriteram_2;
static UINT16* jchan_sprregs_2;

extern void skns_draw_sprites(mame_bitmap *, const rectangle *, UINT32 *, int, int, UINT32 *, int);
extern void skns_sprite_kludge(int, int);
static UINT16 *mainsub_shared_ram;

static UINT16 *mcu_ram, jchan_mcu_com[4];

static void jchan_mcu_run(void)
{
	UINT16 mcu_command = mcu_ram[0x0010/2];
	UINT16 mcu_offset  = mcu_ram[0x0012/2] / 2;
	UINT16 mcu_subcmd  = mcu_ram[0x0014/2];

	logerror("CPU #0 (PC=%06X) : MCU executed command: %04X %04X %04X ",activecpu_get_pc(),mcu_command,mcu_offset*2,mcu_subcmd);

	switch (mcu_command >> 8)
	{
		case 0x04:
		{
			toybox_handle_04_region(mcu_subcmd, mcu_ram, REGION_USER1);
		}
		break;

		case 0x03:
		{
			mcu_ram[mcu_offset] = readinputportbytag("DSW");
			logerror("PC=%06X : MCU executed command: %04X %04X (read DSW)\n",activecpu_get_pc(),mcu_command,mcu_offset*2);
		}
		break;

		case 0x02:
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 0)) != 0)
			{
				mame_fread(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
			logerror("(load NVRAM settings)\n");
		}
		break;

		case 0x42:
		{
			mame_file *f;
			if ((f = mame_fopen(Machine->gamedrv->name, 0, FILETYPE_NVRAM, 1)) != 0)
			{
				mame_fwrite(f,&mcu_ram[mcu_offset], 128);
				mame_fclose(f);
			}
			logerror("(save NVRAM settings)\n");
		}
		break;

		default:
			logerror("- UNKNOWN COMMAND!!!\n");
	}
}

#define JCHAN_MCU_COM_W(_n_) \
static WRITE16_HANDLER( jchan_mcu_com##_n_##_w ) \
{ \
	COMBINE_DATA(&jchan_mcu_com[_n_]); \
	if (jchan_mcu_com[0] != 0xFFFF)	return; \
	if (jchan_mcu_com[1] != 0xFFFF)	return; \
	if (jchan_mcu_com[2] != 0xFFFF)	return; \
	if (jchan_mcu_com[3] != 0xFFFF)	return; \
\
	memset(jchan_mcu_com, 0, 4 * sizeof( UINT16 ) ); \
	jchan_mcu_run(); \
}

JCHAN_MCU_COM_W(0)
JCHAN_MCU_COM_W(1)
JCHAN_MCU_COM_W(2)
JCHAN_MCU_COM_W(3)

static READ16_HANDLER( jchan_mcu_status_r )
{
	logerror("cpu #%d (PC=%06X): read mcu status\n", cpu_getactivecpu(), activecpu_get_previouspc());
	return 0;
}

static void jchan_scanline(int scanline)
{
	int next;
	if (scanline == 11)
	{
		cpunum_set_input_line(0, 2, HOLD_LINE);
		if (jchan_irq_sub_enable)
			cpunum_set_input_line(1, 3, HOLD_LINE);
		next = 240;
	}
	else if (scanline == 240)
	{
		int i;
		for (i = 0; i < 0x1000; i++)
		{
			jchan_sprite_ram32_1[i] = ((UINT32)jchan_spriteram_1[i * 2 + 1] << 16) | jchan_spriteram_1[i * 2];
			jchan_sprite_ram32_2[i] = ((UINT32)jchan_spriteram_2[i * 2 + 1] << 16) | jchan_spriteram_2[i * 2];
		}
		cpunum_set_input_line(0, 1, HOLD_LINE);
		if (jchan_irq_sub_enable)
			cpunum_set_input_line(1, 1, HOLD_LINE);
		next = 249;
	}
	else
	{
		if (jchan_irq_sub_enable)
			cpunum_set_input_line(1, 2, HOLD_LINE);
		next = 11;
	}
	timer_set(cpu_getscanlinetime(next), next, jchan_scanline);
}

static VIDEO_START(jchan)
{

	jchan_sprite_ram32_1 = auto_malloc ( 0x4000 );
	jchan_sprite_ram32_2 = auto_malloc ( 0x4000 );

	memset(jchan_sprite_ram32_1, 0, 0x4000);
	memset(jchan_sprite_ram32_2, 0, 0x4000);
	jchan_sprite_regs32_1 = auto_malloc (0x40);
	jchan_sprite_regs32_2 = auto_malloc (0x40);
	memset(jchan_sprite_regs32_1, 0, 0x40);
	memset(jchan_sprite_regs32_2, 0, 0x40);

	sprite_bitmap_1 = auto_bitmap_alloc(320,240);
	sprite_bitmap_2 = auto_bitmap_alloc(320,240);

	skns_sprite_kludge(0, 0);

	return video_start_kaneko16_1xVIEW2();
}

static VIDEO_UPDATE(jchan)
{
	int x, y;
	fillbitmap(bitmap, Machine->pens[0x7f00], cliprect);
	video_update_jchan_view2(screen, bitmap, cliprect);
	fillbitmap(sprite_bitmap_1, 0, cliprect);
	fillbitmap(sprite_bitmap_2, 0, cliprect);
	skns_draw_sprites(sprite_bitmap_1, cliprect, jchan_sprite_ram32_1,
		0x4000, REGION_GFX1, jchan_sprite_regs32_1, 1);
	skns_draw_sprites(sprite_bitmap_2, cliprect, jchan_sprite_ram32_2,
		0x4000, REGION_GFX2, jchan_sprite_regs32_2, 1);
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *src1 = (UINT16 *)sprite_bitmap_1->line[y];
		UINT16 *src2 = (UINT16 *)sprite_bitmap_2->line[y];
		UINT16 *dst = (UINT16 *)bitmap->line[y];
		UINT8 *tilepri = (UINT8 *)priority_bitmap->line[y];
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			UINT16 pixel = 0;
			int priority = tilepri[x] >> 1;
			if ((src1[x] & 0xff) && (src1[x] >> 14) >= priority)
			{
				pixel = src1[x];
				priority = pixel >> 14;
			}
			if ((src2[x] & 0xff) && (src2[x] >> 14) >= priority)
				pixel = src2[x];
			if (pixel & 0xff)
				dst[x] = (pixel & 0x3fff) | 0x4000;
		}
	}
}

static UINT16 *jchan_ctrl;

static WRITE16_HANDLER( jchan_ctrl_w )
{
	jchan_irq_sub_enable = data & 0x8000;
}

static READ16_HANDLER ( jchan_ctrl_r )
{
	switch(offset)
	{
		case 0/2: return readinputportbytag("P1");
		case 2/2: return readinputportbytag("P2");
		case 4/2: return readinputportbytag("SYSTEM");
		case 6/2: return readinputportbytag("EXTRA");
		default: logerror("jchan_ctrl_r unknown!"); break;
	}
	return jchan_ctrl[offset];
}

static WRITE16_HANDLER( main2sub_cmd_w )
{
	COMBINE_DATA(&mainsub_shared_ram[0x03ffe/2]);
	cpunum_set_input_line(1, 4, HOLD_LINE);
}

static WRITE16_HANDLER( sub2main_cmd_w )
{
	COMBINE_DATA(&mainsub_shared_ram[0x0000/2]);
	cpunum_set_input_line(0, 3, HOLD_LINE);
}

static WRITE16_HANDLER( jchan_suprnova_sprite32regs_1_w )
{
	COMBINE_DATA(&jchan_sprregs_1[offset]);
	offset>>=1;
	jchan_sprite_regs32_1[offset]=(jchan_sprregs_1[offset*2+1]<<16) | (jchan_sprregs_1[offset*2]);
}

static WRITE16_HANDLER( jchan_suprnova_sprite32regs_2_w )
{
	COMBINE_DATA(&jchan_sprregs_2[offset]);
	offset>>=1;
	jchan_sprite_regs32_2[offset]=(jchan_sprregs_2[offset*2+1]<<16) | (jchan_sprregs_2[offset*2]);
}

static ADDRESS_MAP_START( jchan_main, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x1fffff) AM_ROM
	AM_RANGE(0x200000, 0x20ffff) AM_RAM

	AM_RANGE(0x300000, 0x30ffff) AM_RAM AM_BASE(&mcu_ram)
	AM_RANGE(0x330000, 0x330001) AM_WRITE(jchan_mcu_com0_w)
	AM_RANGE(0x340000, 0x340001) AM_WRITE(jchan_mcu_com1_w)
	AM_RANGE(0x350000, 0x350001) AM_WRITE(jchan_mcu_com2_w)
	AM_RANGE(0x360000, 0x360001) AM_WRITE(jchan_mcu_com3_w)
	AM_RANGE(0x370000, 0x370001) AM_READ(jchan_mcu_status_r)

	AM_RANGE(0x400000, 0x403fff) AM_RAM AM_BASE(&mainsub_shared_ram) AM_SHARE(1)

	AM_RANGE(0x500000, 0x503fff) AM_RAM AM_BASE(&jchan_spriteram_1)
	AM_RANGE(0x600000, 0x60003f) AM_RAM AM_WRITE(jchan_suprnova_sprite32regs_1_w) AM_BASE(&jchan_sprregs_1)

	AM_RANGE(0x700000, 0x70ffff) AM_RAM AM_WRITE(paletteram16_xGGGGGRRRRRBBBBB_word_w) AM_BASE(&paletteram16)

	AM_RANGE(0xf00000, 0xf00007) AM_READWRITE(jchan_ctrl_r, jchan_ctrl_w) AM_BASE(&jchan_ctrl)

	AM_RANGE(0xf80000, 0xf80001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( jchan_sub, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x0fffff) AM_ROM
	AM_RANGE(0x100000, 0x10ffff) AM_RAM

	AM_RANGE(0x400000, 0x403fff) AM_RAM AM_BASE(&mainsub_shared_ram) AM_SHARE(1)

	AM_RANGE(0x500000, 0x500fff) AM_RAM AM_WRITE(kaneko16_vram_1_w) AM_BASE(&kaneko16_vram_1)
	AM_RANGE(0x501000, 0x501fff) AM_RAM AM_WRITE(kaneko16_vram_0_w) AM_BASE(&kaneko16_vram_0)
	AM_RANGE(0x502000, 0x502fff) AM_RAM AM_BASE(&kaneko16_vscroll_1)
	AM_RANGE(0x503000, 0x503fff) AM_RAM AM_BASE(&kaneko16_vscroll_0)
	AM_RANGE(0x600000, 0x60001f) AM_RAM AM_WRITE(kaneko16_layers_0_regs_w) AM_BASE(&kaneko16_layers_0_regs)

	AM_RANGE(0x700000, 0x703fff) AM_RAM AM_BASE(&jchan_spriteram_2)
	AM_RANGE(0x780000, 0x78003f) AM_RAM AM_BASE(&jchan_sprregs_2) AM_WRITE(jchan_suprnova_sprite32regs_2_w)

	AM_RANGE(0x800000, 0x800001) AM_WRITE(YMZ280B_register_0_lsb_w)
	AM_RANGE(0x800002, 0x800003) AM_WRITE(YMZ280B_data_0_lsb_w)

	AM_RANGE(0xa00000, 0xa00001) AM_READWRITE(watchdog_reset16_r, watchdog_reset16_w)
ADDRESS_MAP_END

static const gfx_layout tilelayout =
{
	16,16,
	RGN_FRAC(1,1),
	4,
	{ 0,1,2,3 },
	{ 4, 0, 12, 8, 20, 16, 28, 24, 8*32+4, 8*32+0, 8*32+12, 8*32+8, 8*32+20, 8*32+16, 8*32+28, 8*32+24 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32, 16*32,17*32, 18*32, 19*32, 20*32, 21*32, 22*32, 23*32 },
	32*32
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX3, 0, &tilelayout, 0, 0x4000/16 },
	{ REGION_GFX3, 0, &tilelayout, 0, 0x4000/16 },
	{ -1 }
};

static INPUT_PORTS_START( jchan )
	PORT_START_TAG("P1")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )                PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("P2")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT	) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )                PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("SYSTEM")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_START1	)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_START2	)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_COIN1 ) PORT_IMPULSE(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_COIN2 ) PORT_IMPULSE(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_SPECIAL ) PORT_NAME( DEF_STR( Test ) ) PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("EXTRA")
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_UNKNOWN )                PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNKNOWN )                PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x8000)
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_CONDITION("DSW",0x8000,PORTCOND_EQUALS,0x0000)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START_TAG("DSW")
	PORT_SERVICE( 0x0100, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x0200, 0x0200, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(      0x0200, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0400, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0400, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0800, 0x0800, "Sound" )
	PORT_DIPSETTING(      0x0000, DEF_STR( Mono ) )
	PORT_DIPSETTING(      0x0800, DEF_STR( Stereo ) )
	PORT_DIPNAME( 0x1000, 0x1000, DEF_STR( Free_Play ) )
	PORT_DIPSETTING(      0x1000, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x2000, 0x0000, "Blood Mode" )
	PORT_DIPSETTING(      0x2000, DEF_STR( Normal ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( High ) )
	PORT_DIPNAME( 0x4000, 0x4000, "Special Prize Available" )
	PORT_DIPSETTING(      0x4000, DEF_STR( No ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x8000, 0x8000, "Buttons Layout" )
	PORT_DIPSETTING(      0x8000, "3+1" )
	PORT_DIPSETTING(      0x0000, "2+2" )
INPUT_PORTS_END

static INPUT_PORTS_START( jchan2 )
	PORT_INCLUDE( jchan )

	PORT_MODIFY("DSW")
	PORT_DIPNAME( 0x4000, 0x4000, DEF_STR( Unused ) )
	PORT_DIPSETTING( 0x4000, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x0000, DEF_STR( On ) )

INPUT_PORTS_END

static struct YMZ280Binterface ymz280b_intf =
{
	REGION_SOUND1,
	0
};

static MACHINE_RESET( jchan )
{
	memset(jchan_mcu_com, 0, sizeof(jchan_mcu_com));
	jchan_irq_sub_enable = 0;
	timer_set(cpu_getscanlinetime(11), 11, jchan_scanline);
}

static MACHINE_DRIVER_START( jchan )

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(jchan_main,0)

	MDRV_CPU_ADD(M68000, 16000000)
	MDRV_CPU_PROGRAM_MAP(jchan_sub,0)

	MDRV_GFXDECODE(gfxdecodeinfo)

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_FRAMES_PER_SECOND(28636363.0 / (4 * 460 * 261))
	MDRV_INTERLEAVE(10)
	MDRV_MACHINE_RESET(jchan)
	MDRV_VBLANK_DURATION(1349)

	MDRV_SCREEN_SIZE(320, 261)
	MDRV_VISIBLE_AREA(0*8, 40*8-1, 0*8, 30*8-1)

	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(jchan)
	MDRV_VIDEO_UPDATE(jchan)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(YMZ280B, 16000000)
	MDRV_SOUND_CONFIG(ymz280b_intf)
	MDRV_SOUND_ROUTE(0, "left", 1.0)
	MDRV_SOUND_ROUTE(1, "right", 1.0)
MACHINE_DRIVER_END

ROM_START( jchan )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "jm01x3.u67", 0x000001, 0x080000, CRC(c0adb141) SHA1(de265e1da06e723492e0c2465cd04e25ce1c237f) )
	ROM_LOAD16_BYTE( "jm00x3.u68", 0x000000, 0x080000, CRC(b1aadc5a) SHA1(0a93693088c0a4b8a79159fb0ebac47d5556d800) )
	ROM_LOAD16_BYTE( "jm11x3.u69", 0x100001, 0x080000, CRC(d2e3f913) SHA1(db2d790fba5351660a9525f545ab1b23dfe319b0) )
	ROM_LOAD16_BYTE( "jm10x3.u70", 0x100000, 0x080000, CRC(ee08fee1) SHA1(5514bd8c625bc7cf8dd5da2f76b760716609b925) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "jsp1x3.u86", 0x000001, 0x080000, CRC(d15d2b8e) SHA1(e253f2d64fee6627f68833b441f41ea6bbb3ab07) )
	ROM_LOAD16_BYTE( "jsp0x3.u87", 0x000000, 0x080000, CRC(ebec50b1) SHA1(57d7bd728349c2b9d662bcf20a3be92902cb3ffb) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD( "jc-100-00.179", 0x0000000, 0x0400000, CRC(578d928c) SHA1(1cfe04f9b02c04f95a85d6fe7c4306a535ff969f) )
	ROM_LOAD( "jc-101-00.180", 0x0400000, 0x0400000, CRC(7f5e1aca) SHA1(66ed3deedfd55d88e7dcd017b9c2ce523ccb421a) )
	ROM_LOAD( "jc-102-00.181", 0x0800000, 0x0400000, CRC(72caaa68) SHA1(f6b98aa949768a306ac9bc5f9c05a1c1a3fb6c3f) )
	ROM_LOAD( "jc-103-00.182", 0x0c00000, 0x0400000, CRC(4e9e9fc9) SHA1(bf799cdee930b7f71aea4d55c3dd6a760f7478bb) )
	ROM_LOAD( "jc-104-00.183", 0x1000000, 0x0200000, CRC(6b2a2e93) SHA1(e34010e39043b67493bcb23a04828ab7cda8ba4d) )
	ROM_LOAD( "jc-105-00.184", 0x1200000, 0x0200000, CRC(73cad1f0) SHA1(5dbe4e318948e4f74bfc2d0d59455d43ba030c0d) )
	ROM_LOAD( "jc-108-00.185", 0x1400000, 0x0200000, CRC(67dd1131) SHA1(96f334378ae0267bdb3dc528635d8d03564bd859) )
	ROM_LOAD16_BYTE( "jcs0x3.164", 0x1600000, 0x040000, CRC(9a012cbc) SHA1(b3e7390220c90d55dccfb96397f0af73925e36f9) )
	ROM_LOAD16_BYTE( "jcs1x3.165", 0x1600001, 0x040000, CRC(57ae7c8d) SHA1(4086f638c2aabcee84e838243f0fd15cec5c040d) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD( "jc-106-00.171", 0x000000, 0x200000, CRC(bc65661b) SHA1(da28b8fcd7c7a0de427a54be2cf41a1d6a295164) )
	ROM_LOAD( "jc-107-00.172", 0x200000, 0x200000, CRC(92a86e8b) SHA1(c37eddbc9d84239deb543504e27b5bdaf2528f79) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "jc-200.00", 0x000000, 0x100000, CRC(1f30c24e) SHA1(0c413fc67c3ec020e6786e7157d82aa242c8d2ad) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "jc-301-00.85", 0x000000, 0x100000, CRC(9c5b3077) SHA1(db9a31e1c65d9f12d0f2fb316ced48a02aae089d) )
	ROM_RELOAD(0x100000,0x100000)
	ROM_LOAD( "jc-300-00.84", 0x200000, 0x200000, CRC(13d5b1eb) SHA1(b047594d0f1a71d89b8f072879ccba480f54a483) )
	ROM_LOAD( "jcw0x0.u56",   0x400000, 0x040000, CRC(bcf25c2a) SHA1(b57a563ab5c05b05d133eed3d099c4de997f37e4) )

	ROM_REGION( 0x020000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "jcd0x1.u13", 0x000000, 0x020000, CRC(2a41da9c) SHA1(7b1ba0efc0544e276196b9605df1881fde871708) )
ROM_END

ROM_START( jchan2 )
	ROM_REGION( 0x200000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "j2p1x1.u67", 0x000001, 0x080000, CRC(5448c4bc) SHA1(447835275d5454f86a51879490a6b22b06a23e81) )
	ROM_LOAD16_BYTE( "j2p1x2.u68", 0x000000, 0x080000, CRC(52104ab9) SHA1(d6647e628662bdb832270540ece18b265b7ce62d) )
	ROM_LOAD16_BYTE( "j2p1x3.u69", 0x100001, 0x080000, CRC(8763ebca) SHA1(daf6af42a34802ef9aa996e340e218779bad695f) )
	ROM_LOAD16_BYTE( "j2p1x4.u70", 0x100000, 0x080000, CRC(0f8e5e69) SHA1(1f71042458f76b7d99382db6412fb6c362cd3ded) )

	ROM_REGION( 0x100000, REGION_CPU2, 0 )
	ROM_LOAD16_BYTE( "j2p1x5.u86", 0x000001, 0x080000, CRC(dc897725) SHA1(d3e94bac96497deb2f79996c2d4a349f6da5b1d6) )
	ROM_LOAD16_BYTE( "j2p1x6.u87", 0x000000, 0x080000, CRC(594224f9) SHA1(bc546a98c5f3c5b08f521c54a4b0e9e2cdf83ced) )

	ROM_REGION( 0x2000000, REGION_GFX1, 0 )
	ROM_LOAD( "jc-100-00.179", 0x0000000, 0x0400000, CRC(578d928c) SHA1(1cfe04f9b02c04f95a85d6fe7c4306a535ff969f) )
	ROM_LOAD( "jc-101-00.180", 0x0400000, 0x0400000, CRC(7f5e1aca) SHA1(66ed3deedfd55d88e7dcd017b9c2ce523ccb421a) )
	ROM_LOAD( "jc-102-00.181", 0x0800000, 0x0400000, CRC(72caaa68) SHA1(f6b98aa949768a306ac9bc5f9c05a1c1a3fb6c3f) )
	ROM_LOAD( "jc-103-00.182", 0x0c00000, 0x0400000, CRC(4e9e9fc9) SHA1(bf799cdee930b7f71aea4d55c3dd6a760f7478bb) )
	ROM_LOAD( "jc-104-00.183", 0x1000000, 0x0200000, CRC(6b2a2e93) SHA1(e34010e39043b67493bcb23a04828ab7cda8ba4d) )
	ROM_LOAD( "jc-105-00.184", 0x1200000, 0x0200000, CRC(73cad1f0) SHA1(5dbe4e318948e4f74bfc2d0d59455d43ba030c0d) )
	ROM_LOAD( "jc-108-00.185", 0x1400000, 0x0200000, CRC(67dd1131) SHA1(96f334378ae0267bdb3dc528635d8d03564bd859) )
	ROM_LOAD16_BYTE( "j2g1x1.164", 0x1600000, 0x080000, CRC(66a7ea6a) SHA1(605cbc1eb50fb0decbea790f2a11e999d5fde762) )
	ROM_LOAD16_BYTE( "j2g1x2.165", 0x1600001, 0x080000, CRC(660e770c) SHA1(1e385a6ee83559b269d2179e6c247238c0f3c850) )

	ROM_REGION( 0x1000000, REGION_GFX2, 0 )
	ROM_LOAD( "jc-106-00.171", 0x000000, 0x200000, CRC(bc65661b) SHA1(da28b8fcd7c7a0de427a54be2cf41a1d6a295164) )
	ROM_LOAD( "jc-107-00.172", 0x200000, 0x200000, CRC(92a86e8b) SHA1(c37eddbc9d84239deb543504e27b5bdaf2528f79) )

	ROM_REGION( 0x100000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "jc-200.00", 0x000000, 0x100000, CRC(1f30c24e) SHA1(0c413fc67c3ec020e6786e7157d82aa242c8d2ad) )

	ROM_REGION( 0x1000000, REGION_SOUND1, 0 )
	ROM_LOAD( "jc-301-00.85", 0x000000, 0x100000, CRC(9c5b3077) SHA1(db9a31e1c65d9f12d0f2fb316ced48a02aae089d) )
	ROM_RELOAD(0x100000,0x100000)
	ROM_LOAD( "jc-300-00.84", 0x200000, 0x200000, CRC(13d5b1eb) SHA1(b047594d0f1a71d89b8f072879ccba480f54a483) )
	ROM_LOAD( "j2m1x1.u56",   0x400000, 0x040000, CRC(baf6e25e) SHA1(6b02f3eb1eafcd43022a9f60f98573d02277adfe) )

	ROM_REGION( 0x020000, REGION_USER1, 0 )
	ROM_LOAD16_WORD_SWAP( "j2d1x1.u13", 0x000000, 0x020000, CRC(b2b7fc90) SHA1(1b90c13bb41a313c4ed791a15d56073a7c29928b) )
ROM_END

static DRIVER_INIT( jchan )
{
	toybox_decrypt_region(REGION_USER1);

	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x403ffe, 0x403fff, 0, 0, main2sub_cmd_w );
	memory_install_write16_handler(1, ADDRESS_SPACE_PROGRAM, 0x400000, 0x400001, 0, 0, sub2main_cmd_w );

	memset(jchan_mcu_com, 0, 4 * sizeof( UINT16 ) );
}

GAME( 1995, jchan,     0,        jchan,    jchan,    jchan,    ROT0, "Kaneko", "Jackie Chan - The Kung-Fu Master", GAME_IMPERFECT_GRAPHICS )
GAME( 1995, jchan2,    0,    jchan,    jchan2,   jchan,    ROT0, "Kaneko", "Jackie Chan in Fists of Fire", GAME_IMPERFECT_GRAPHICS )

