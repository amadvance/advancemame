// license:LGPL-2.1+
// copyright-holders:Tomasz Slanina, Morten Shearman Kirkegaard, Samuel Neves, Peter Wilhelmsen
/********************************************
	Laser Base / Future Flash driver

	PCB contains a large analog sound section,
	currently only partially emulated.
********************************************/
#include "driver.h"
#include "machine/pit8253.h"
#include "vidhrdw/crtc6845.h"
#include "sound/dac.h"
static UINT8 *vram;
static int vrambank;
static UINT8 m_counter[6],m_cnt_out[6],m_nmi;
static UINT8 m_track_prv[2],m_vset,m_bset,m_flipscreen;
static INT8 m_track_cnt[2];
static UINT64 m_z1data;
static mame_timer *scan_timer;
#define FRAME_RATE ((16668000.0/3)/(360*274))
static VIDEO_START(laserbas)
{
	vram=auto_malloc(0x10000);
	memset(vram,0,0x10000);
	return 0;
}
static VIDEO_UPDATE(laserbas)
{
	int y,x;
	int height=crtc6845_max_ras_addr+1;
	int columns=MIN(crtc6845_horiz_disp,32);
	fillbitmap(bitmap,Machine->pens[m_bset],cliprect);
	for(y=0;y<224;y++)
	{
		int ra=y%height;
		int ma=crtc6845_start_addr+(y/height)*crtc6845_horiz_disp;
		int addr=(((ma&0x3ff)<<5)|(ra<<7))+(m_vset<<7);
		for(x=0;x<columns*8;x++)
		{
			int p1=(vram[(addr+(x>>1))&0x7fff]>>((x&1)*4))&15;
			int p2=(vram[0x8000+((((y*128)|(ra<<7))+(x>>1))&0x7fff)]>>((x&1)*4))&15;
			int pen=p2?p2:p1?p1+16:m_bset;
			plot_pixel(bitmap,m_flipscreen?255-x:x,m_flipscreen?223-y:y,Machine->pens[pen]);
		}
	}
}
static READ8_HANDLER(vram_r) { return vram[vrambank*0x8000+offset]; }
static WRITE8_HANDLER(vram_w) { vram[vrambank*0x8000+offset]=data; }
static WRITE8_HANDLER(videoctrl1_w)
{
	vrambank=(data>>6)&1;
	data^=0xff;
	m_flipscreen=data&0x80;
	m_vset=(data>>3)&7;
}
static WRITE8_HANDLER(videoctrl2_w)
{
	data^=0xff;m_bset=(data>>4)&15;m_nmi=data&1;
}
static WRITE8_HANDLER(palette_w)
{
	palette_set_color(offset,pal3bit(data>>5),pal3bit(data>>2),pal2bit(data));
}
static READ8_HANDLER(z1_r)
{

	m_z1data = (m_z1data >> 10) | (((UINT64)(offset & 0x03ff)) << 30);

	#define x(b) ((m_z1data>>(b))&1)
#define nx(b) ((~m_z1data>>(b))&1)
#define MUX2(s,a,b) ((s)?(a):(b))

	const UINT8 bit7 = MUX2(x(36) & x(33), x(31) ^ x(35), (nx(33) & (nx(20) | nx(36))) | (nx(36) & x(24)));
	const UINT8 bit6 = MUX2(x(36), MUX2(x(33), x(29), nx(23) | x(30)), x(33) & x(15));
	const UINT8 bit5 = MUX2(x(36), nx(33) & x(27), x(32) | x(33));
	const UINT8 bit4 = MUX2(x(36), MUX2(x(33), nx(24), nx(35)), MUX2(nx(33), x(4), MUX2(nx(26), x(5) & x(23), MUX2(x(23), x(19), nx(13) | x(20)))));
	const UINT8 bit3 = MUX2(x(36), x(33) & x(11), MUX2(x(33), x(25) | nx(31), x(24)));
	const UINT8 bit2 = MUX2(x(33), MUX2(x(36), x(28), x(20)), MUX2(x(36), nx(11), nx(39)));
	const UINT8 bit1 = MUX2(x(36), MUX2(x(23), MUX2(x(26), nx(18), nx(10)) | x(33), MUX2(x(26), x(1), x(29)) & nx(33)), MUX2(x(33), x(7), x(17)));
	const UINT8 bit0 = MUX2(x(33), MUX2(x(36), x(22), nx(26)), MUX2(x(36), x(14), x(21)));

	return (bit7 << 7) | (bit6 << 6) | (bit5 << 5) | (bit4 << 4) | (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | (bit0 << 0);
}

#undef x
#undef nx
#undef MUX2
static READ8_HANDLER(track_dir_r)
{
	int i;
	for(i=0;i<2;i++)
	{
		UINT8 track=readinputportbytag(i?"TRACK_Y":"TRACK_X");
		int diff=track-m_track_prv[i];m_track_prv[i]=track;
		if(diff>0x20)diff-=0x40;else if(diff< -0x20)diff+=0x40;
		m_track_cnt[i]+=diff;
	}
	return (m_track_cnt[0]<0?1:0)|(m_track_cnt[1]>0?2:0);
}
static READ8_HANDLER(track_val_r)
{
	int x=MAX(-15,MIN(15,m_track_cnt[0]));
	int y=MAX(-15,MIN(15,m_track_cnt[1]));
	m_track_cnt[0]-=x;m_track_cnt[1]-=y;
	return abs(x)|(abs(y)<<4);
}
static void pit_out(int which,int state)
{
	int data;
	state^=1;
	if(!state && m_cnt_out[which])m_counter[which]=(m_counter[which]+1)&15;
	data=state|((m_counter[which]&7)<<1);
	if(m_counter[which]&8)data^=15;
	DAC_signed_data_w(which,data*17);
	m_cnt_out[which]=state;
}
static void pit_out0(int state) { pit_out(0,state); }
static void pit_out1(int state) { pit_out(1,state); }
static void pit_out2(int state) { pit_out(2,state); }
static void pit_out3(int state) { pit_out(3,state); }
static void pit_out4(int state) { pit_out(4,state); }
static void pit_out5(int state) { pit_out(5,state); }
static const struct pit8253_config pit_config[2]={
	{ TYPE8253, { { 16668000.0/12,pit_out0,0 },{ 16668000.0/12,pit_out1,0 },{ 16668000.0/12,pit_out2,0 } } },
	{ TYPE8253, { { 16668000.0/12,pit_out3,0 },{ 16668000.0/12,pit_out4,0 },{ 16668000.0/12,pit_out5,0 } } },
};
static void scanline_callback(int scanline)
{
	int next,delta;
	if(scanline==0 || scanline==128)cpunum_set_input_line(0,0,HOLD_LINE);
	if(scanline==240 && m_nmi)cpunum_set_input_line(0,INPUT_LINE_NMI,PULSE_LINE);
	if(scanline==0){next=128;delta=128;}
	else if(scanline==128){next=240;delta=112;}
	else {next=0;delta=34;}
	timer_adjust(scan_timer,TIME_IN_SEC(delta/(FRAME_RATE*274)),next,0);
}
static MACHINE_START(laserbas)
{
	scan_timer=timer_alloc(scanline_callback);
	return pit8253_init(2,pit_config);
}
static MACHINE_RESET(laserbas)
{
	vrambank=1;m_vset=m_bset=m_flipscreen=m_nmi=0;m_z1data=0;
	memset(m_counter,0,sizeof(m_counter));memset(m_cnt_out,0,sizeof(m_cnt_out));
	memset(m_track_prv,0,sizeof(m_track_prv));memset(m_track_cnt,0,sizeof(m_track_cnt));
	pit8253_reset(0);pit8253_reset(1);
	timer_adjust(scan_timer,TIME_NOW,0,0);
}
static ADDRESS_MAP_START(laserbas_memory,ADDRESS_SPACE_PROGRAM,8)
	AM_RANGE(0x0000,0x3fff) AM_ROM
	AM_RANGE(0x4000,0xbfff) AM_READWRITE(vram_r,vram_w)
	AM_RANGE(0xc000,0xf7ff) AM_ROM
	AM_RANGE(0xf800,0xfbff) AM_READ(z1_r) AM_WRITENOP
	AM_RANGE(0xfc00,0xffff) AM_RAM
ADDRESS_MAP_END
static ADDRESS_MAP_START(laserbas_io,ADDRESS_SPACE_IO,8)
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00,0x00) AM_WRITE(crtc6845_address_w)
	AM_RANGE(0x01,0x01) AM_WRITE(crtc6845_register_w)
	AM_RANGE(0x10,0x10) AM_WRITE(videoctrl1_w)
	AM_RANGE(0x11,0x11) AM_WRITE(videoctrl2_w)
	AM_RANGE(0x20,0x20) AM_READ(input_port_0_r)
	AM_RANGE(0x21,0x21) AM_READ(input_port_1_r)
	AM_RANGE(0x22,0x22) AM_READ(track_dir_r)
	AM_RANGE(0x23,0x23) AM_READ(track_val_r)
	AM_RANGE(0x20,0x23) AM_WRITENOP
	AM_RANGE(0x40,0x43) AM_READWRITE(pit8253_0_r,pit8253_0_w)
	AM_RANGE(0x44,0x47) AM_READWRITE(pit8253_1_r,pit8253_1_w)
	AM_RANGE(0x80,0x9f) AM_WRITE(palette_w)
ADDRESS_MAP_END
static INPUT_PORTS_START( laserbas )
	PORT_START_TAG("DSW")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Normal ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x04, "10k" )
	PORT_DIPSETTING(    0x00, "30k" )
	PORT_DIPNAME( 0x08, 0x00, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Free_Play ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) )

	PORT_START_TAG("INPUTS")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_SERVICE( 0x20, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE1 )

	PORT_START_TAG("TRACK_X")
	PORT_BIT( 0x03f, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(30) PORT_KEYDELTA(20)

	PORT_START_TAG("TRACK_Y")
	PORT_BIT( 0x03f, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(30) PORT_KEYDELTA(20)
INPUT_PORTS_END

static MACHINE_DRIVER_START(laserbas)
	MDRV_CPU_ADD(Z80,16668000/6)
	MDRV_CPU_PROGRAM_MAP(laserbas_memory,0)
	MDRV_CPU_IO_MAP(laserbas_io,0)
	MDRV_MACHINE_START(laserbas)
	MDRV_MACHINE_RESET(laserbas)
	MDRV_FRAMES_PER_SECOND(FRAME_RATE)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256,274)
	MDRV_VISIBLE_AREA(0,255,0,223)
	MDRV_PALETTE_LENGTH(32)
	MDRV_VIDEO_START(laserbas)
	MDRV_VIDEO_UPDATE(laserbas)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
	MDRV_SOUND_ADD(DAC,0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.16)
MACHINE_DRIVER_END
ROM_START( laserbas )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "mb8532.1",   0x0000, 0x1000, CRC(ef85e0c5) SHA1(c26da919c206a23eb6e53ffe39acd5a5dfd47c18) )
	ROM_LOAD( "mb8532.2",   0x1000, 0x1000, CRC(0ba2236c) SHA1(416e4be957c395b05537d2e513e0c4561d8ca7c5) )
	ROM_LOAD( "mb8532.3",   0x2000, 0x1000, CRC(83998e0b) SHA1(ac435fb8dd67aec0737d6c750c527841b2b91a5b) )
	ROM_LOAD( "mb8532.4",   0x3000, 0x1000, CRC(00f9d909) SHA1(90b800cc5fcea53454584f8ad93eebbd193bdd21) )
	ROM_LOAD( "lb2532.5",   0xc000, 0x1000, CRC(6459073e) SHA1(78b8a23534826dd2d3b3c6c5d5708c8a78a4b6bf) )
	ROM_LOAD( "lb2532.6",   0xd000, 0x1000, CRC(a2dc1e7e) SHA1(78643a3aa852c73dab12e09a6cfc53141c936d12) )
	ROM_LOAD( "mb8532.7",   0xe000, 0x1000, CRC(9d2148d7) SHA1(24954d82a09d9fcfdc61e91b7c824daa5dd701c3) )
	ROM_LOAD( "mb8516.8",   0xf000, 0x0800, CRC(623f558f) SHA1(be6c6565df658555f21c43a8c2459cf399794a84) )
ROM_END

ROM_START( laserbsa )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "2732.u1",       0x0000, 0x1000, CRC(f3ab00dc) SHA1(4730b13b55c93c71ed483463e180e71e506cfbd6) )
	ROM_LOAD( "2732.u2",       0x1000, 0x1000, CRC(0ba2236c) SHA1(416e4be957c395b05537d2e513e0c4561d8ca7c5) )
	ROM_LOAD( "mb8532.u3",     0x2000, 0x1000, CRC(c58a7745) SHA1(382e2235d89520860335c1c2760339e116c0466f) )
	ROM_LOAD( "mbm2732.u4",    0x3000, 0x1000, CRC(00f9d909) SHA1(90b800cc5fcea53454584f8ad93eebbd193bdd21) )
	ROM_LOAD( "2732.u5",       0xc000, 0x1000, CRC(6459073e) SHA1(78b8a23534826dd2d3b3c6c5d5708c8a78a4b6bf) )
	ROM_LOAD( "2732.u6",       0xd000, 0x1000, CRC(a2dc1e7e) SHA1(78643a3aa852c73dab12e09a6cfc53141c936d12) )
	ROM_LOAD( "2732.u7",       0xe000, 0x1000, CRC(9d2148d7) SHA1(24954d82a09d9fcfdc61e91b7c824daa5dd701c3) )
	ROM_LOAD( "mb8516.u8",     0xf000, 0x0800, CRC(623f558f) SHA1(be6c6565df658555f21c43a8c2459cf399794a84) )
ROM_END

ROM_START( futflash )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_LOAD( "ff.1",    	  0x0000, 0x1000, CRC(bcd6b998) SHA1(4a210c40ce6015e2b921558b7571b7f2a27e1815) )
	ROM_LOAD( "ff.2",         0x1000, 0x1000, CRC(1b1f6953) SHA1(8cd7b7e2236700ce63c60b4d2286099c8091bdbd) )
	ROM_LOAD( "ff.3",         0x2000, 0x1000, CRC(30008f04) SHA1(e03b2dbcb6d2615650cdd47ecf1d587906ce149b) )
	ROM_LOAD( "ff.4",         0x3000, 0x1000, CRC(e559aa12) SHA1(0fecfb60b0147e8060c640f684f69503478200ff) )
	ROM_LOAD( "ff.5",         0xc000, 0x1000, CRC(6459073e) SHA1(78b8a23534826dd2d3b3c6c5d5708c8a78a4b6bf) )
	ROM_LOAD( "ff.6",         0xd000, 0x1000, CRC(a8b17f49) SHA1(aea349bd19d001233bfb1805e586c950275010b4) )
	ROM_LOAD( "ff.7",         0xe000, 0x1000, CRC(9d2148d7) SHA1(24954d82a09d9fcfdc61e91b7c824daa5dd701c3) )
	ROM_LOAD( "ff.8",         0xf000, 0x0800, CRC(623f558f) SHA1(be6c6565df658555f21c43a8c2459cf399794a84) )
ROM_END

GAME(1980,futflash,0,laserbas,laserbas,0,ROT270,"Hoei","Future Flash (set 1)",GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_COLORS|GAME_IMPERFECT_SOUND)
GAME(1981,laserbas,futflash,laserbas,laserbas,0,ROT270,"Hoei (Amstar license)","Laser Base (set 1)",GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_COLORS|GAME_IMPERFECT_SOUND)
GAME(1981,laserbsa,futflash,laserbas,laserbas,0,ROT270,"Hoei (Amstar license)","Laser Base (set 2)",GAME_IMPERFECT_GRAPHICS|GAME_IMPERFECT_COLORS|GAME_IMPERFECT_SOUND)
