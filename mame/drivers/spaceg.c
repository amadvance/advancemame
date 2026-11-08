// license:GPL-2.0+
// copyright-holders:Jarek Burczynski, Tomasz Slanina
#include "driver.h"
#include "cpu/z80/z80.h"
#include "sound/samples.h"
#include "sound/sn76477.h"


/*
Space Guerrilla PCB Layout
--------------------------

All PCBs slot into a metal frame. Each PCB is joined by the 14way/28way edge connectors
on one side, and the 50 pin connectors via flat cables on the other side.

                       14-way
OEC-5            |----------------|
|----------------|----------------|-----------------|
|      1       2       3       4       5      6     |
|A   74273   7417             555                   |
|                                                   |
|B   74273   7417     555     7493    76477  LM380  |
|                                                   |
|C   DIP16   74121    4066    7400           4030   |
|                                                   |
|D   LM324   LM324    LM3900  LM3900  458    4006   |
|                                                   |
|                                                   |
|                                                   |
|                                                   |
| VR1   VR2   VR3   VR4   VR5   VR6   VR7    VOL    |
|---------------------------------------------------|

Notes:
      DIP16 - Joins to DIP16 socket at 3F on IOC2-A PCB via a 16-wire flat cable.
      VR1-7 - Potentiometers for volume of sounds
      VOL   - Master Volume
      Note PCB also contains a lot of resistors, transistors, capacitors etc



                            28-way
IOC2-A           |-----------------------------|
|----------------|-----------------------------|----------------|
|      1         2       3       4       5      6      7        |
|A     DIP16   DIP16   DIP16   DIP16                            |
|                                                           VR1 |
|B     D1-8    7404    7404    7404    LM324          4066      |
|                                                               |
|C     D9-16   74240   7406    DIP8           LM324         VR2 |
|                                                               |
|D     74240   74240   74175   75472   555           555        |
|                                                               |
|E     DSW1    74240   74273   7417                         VR3 |
|                                                               |
|F     DIP16   DIP16   DIP16   555     458    LM3900   LM3900   |
|                                                               |
|G     74244   74244   7432    555                          VR4 |
|                                                               |
|H                                                              |
|                                                               |
|                                                               |
|  |-------------|                                              |
|--|-------------|----------------------------------------------|
      50 pin

Notes:
      DIP8 - Unpopulated position for DIP8 IC
      DIP16- Unpopulated position for DIP16 IC
      D1-16- Diodes
      VR1-4- Potentiometers for volume of sounds



                            28-way
CRTC1-A          |-----------------------------|
|----------------|-----------------------------|----------------|
|   1      2      3      4      5      6      7      8      9   |
|A 20MHz  7455   Diodes 1-4   DIP16  DIP16  DIP16  4027   4027  |
|                      &                                        |
|B 74163  7404   Transtrs 1-7 DIP16  DIP16  DIP16  4027   4027  |
|                                                               |
|C 7486   74161  7404         DIP16  DIP16  DIP16  4027   4027  |
|                                                               |
|D 7486   74161  7408      82S09     74374  74157  4027   4027  |
|                                                               |
|E 7486   74161  7402  7411   74157  74377  7408   4027   4027  |
|                                                               |
|F 7486   74161  7411  74157  74244  74377  7400   74153  74157 |
|                                                               |
|G 74107  7404   7474  74175  74107  74157  7404   74377  74157 |
|                                                               |
|H 7404   7404   74244 74139  7404   7427   7474   74164  7404  |
|                                                               |
|                                                               |
|  |-------------|       |-------------|       |-------------|  |
|--|-------------|-------|-------------|-------|-------------|--|
      50 pin                 50 pin                50 pin

Notes:
      4027 - MOSTEK MK4027-4 RAM (DIP16)
      DIP16- Unpopulated position for DIP16 IC



                            28-way
CRTC2-A          |-----------------------------|
|----------------|-----------------------------|----------------|
|   1      2      3      4      5      6      7      8      9   |
|A 7402  74377  74157  7400   74194  74S288        4027   4027  |
|                                                               |
|B 7486  74153  7486   7400   74194  74S288        4027   4027  |
|                                                               |
|C 7408  74374  74158         74273  7427          4027   4027  |
|                                                               |
|D 7400  74244  25S10  74181  7430   7408          4027   4027  |
|                                                               |
|E 7400  74153  25S10  74181         7402          4027   4027  |
|                                                               |
|F 7408  74153         7411   7404   7404          4027   4027  |
|                                                               |
|G 7404  74153  DIP16  74153  74153  74283         4027   4027  |
|                                                               |
|H 74138 74244  7474   74153  74153  74157         4027   4027  |
|                                                               |
|                                                               |
|  |-------------|       |-------------|       |-------------|  |
|--|-------------|-------|-------------|-------|-------------|--|
      50 pin                 50 pin                50 pin

Notes:
      74S288 - 32bytes x8 Bipolar PROM (DIP16, both PROMs contain identical contents)
      4027   - MOSTEK MK4027-4 RAM (DIP16)



                            28-way
SBC-A            |-----------------------------|
|----------------|-----------------------------|----------------|
|   1      2      3      4      5      6      7      8      9   |
|A 74161        DIP18  4045   4045                 ROM2   ROM1  |
|                                                               |
|B 7404  7402   74139  DIP18  DIP18                ROM4   ROM3  |
|                                                               |
|C 7404  7404   7420   4045   4045                 ROM6   ROM5  |
|                                                               |
|D              7420   DIP18  DIP18                ROM8   ROM7  |
|                                                               |
|E 74241 74241         8216   8216                 DIP24  DIP24 |
|                                                               |
|F    Z80              74241                       DIP24  DIP24 |
|                                                               |
|G       7407          74241                       ROM14  ROM13 |
|                                                               |
|H                     74241  7442   7442   74244  ROM16  ROM15 |
|                                                               |
|                                                               |
|  |-------------|       |-------------|                        |
|--|-------------|-------|-------------|------------------------|
      50 pin                 50 pin

Notes:
      4045 - TMS4045-45NL RAM (DIP18)
      8216 - Mitsubishi M5L8216 RAM (DIP16)
      DIP18- Unpopulated position for DIP18 IC
      DIP24- Unpopulated position for DIP24 IC
      All ROMs type 2708 (DIP24)

*/




static UINT8 *spaceg_colorram;
static UINT8 color_latch, palette_latch, pixel_shift, gfx_control;
static UINT8 sound1, sound3;

static WRITE8_HANDLER( spaceg_videoram_w )
{
	int next = (offset + 0x100) & 0x1fff;
	UINT16 shifted = data << (8 - pixel_shift);
	UINT16 pixels = (videoram[offset] << 8) | videoram[next];

	switch (gfx_control & 0x0f)
	{
		case 0:
			pixels &= ~(0xff00 >> pixel_shift);
		case 1:
			pixels |= shifted;
			if (shifted & 0xff00) spaceg_colorram[offset] = color_latch;
			if (shifted & 0x00ff) spaceg_colorram[next] = color_latch;
			break;
		case 0x0d:
			pixels &= ~shifted;
			break;
		default:
			return;
	}
	videoram[offset] = pixels >> 8;
	videoram[next] = pixels & 0xff;
}

static WRITE8_HANDLER( spaceg_colorram_w )
{
	if (offset == 0x400)
		color_latch = data & 0x1f;
	else if (!(offset & 0x1ce0))
		palette_latch = data;
	else if (offset & 0xe0)
		spaceg_colorram[offset] = data;
}

static READ8_HANDLER( spaceg_palette_r )
{
	if (offset & 0x1ce0)
		return 0xff;
	if (gfx_control & 0x40)
	{
		int index = (offset & 0x1f) | ((offset >> 4) & 0x20);
		int rgb = (palette_latch << 1) | ((offset & 0x100) >> 8);
		palette_set_color(index,pal3bit(rgb),pal3bit(rgb >> 6),pal3bit(rgb >> 3));
	}
	return 0;
}

static PALETTE_INIT( spaceg )
{
	int i;
	for (i = 0; i < 64; i++)
		palette_set_color(i,0,0,0);
}

VIDEO_UPDATE( spaceg )
{
	int x,y;
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
		for (x = cliprect->min_x; x <= cliprect->max_x; x++)
		{
			UINT8 sx = x, sy = y;
			int offset,pixel,color;
			if (flip_screen)
			{
				sx = ~sx;
				sy = ~sy + 32;
			}
			offset = ((sx << 5) & 0x1f00) | sy;
			pixel = (videoram[offset] >> (~sx & 7)) & 1;
			color = (spaceg_colorram[offset] & 0x1f) | (pixel << 5);
			plot_pixel(bitmap,x,y,Machine->pens[color]);
		}
}

static WRITE8_HANDLER( spaceg_shift_w )
{
	pixel_shift = data >> 5;
}

static WRITE8_HANDLER( spaceg_gfxctrl_w )
{
	gfx_control = data;
}

static WRITE8_HANDLER( spaceg_output_w )
{
	set_led_status(0,data & 1);
	set_led_status(1,data & 2);
	flip_screen_set(data & 8);
}

static WRITE8_HANDLER( spaceg_sound1_w )
{
	if (!(sound1 & 2) && (data & 2)) sample_start(1,1,0);
	if (!(sound1 & 4) && (data & 4)) sample_start(0,0,0);
	SN76477_enable_w(0,!(data & 8));
	sound1 = data;
}

static WRITE8_HANDLER( spaceg_sound3_w )
{
	if (!(sound3 & 1) && (data & 1)) sample_start(4,8,0);
	if (!(sound3 & 2) && (data & 2)) sample_start(5,8,0);
	if (!(sound3 & 4) && (data & 4)) sample_start(2,2,0);
	if (!(sound3 & 8) && (data & 8)) sample_start(3,7,0);
	sound3 = data;
}

static MACHINE_RESET( spaceg )
{
	color_latch = palette_latch = pixel_shift = gfx_control = 0;
	sound1 = sound3 = 0;
	flip_screen_set(0);
	SN76477_envelope_w(0,1);
	SN76477_mixer_w(0,0);
	SN76477_vco_w(0,1);
	SN76477_enable_w(0,1);
}

static ADDRESS_MAP_START( spaceg_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x3000, 0x3fff) AM_ROM
	AM_RANGE(0x7000, 0x77ff) AM_RAM
	AM_RANGE(0xa000, 0xbfff) AM_READWRITE(spaceg_palette_r,spaceg_colorram_w) AM_BASE(&spaceg_colorram)
	AM_RANGE(0xc000, 0xdfff) AM_RAM AM_WRITE(spaceg_videoram_w) AM_BASE(&videoram)
	AM_RANGE(0x9400, 0x9400) AM_WRITE(spaceg_shift_w)
	AM_RANGE(0x9401, 0x9401) AM_WRITE(spaceg_gfxctrl_w)
	AM_RANGE(0x9402, 0x9402) AM_WRITE(spaceg_output_w)
	AM_RANGE(0x9405, 0x9405) AM_WRITE(spaceg_sound1_w)
	AM_RANGE(0x9406, 0x9406) AM_WRITE(MWA8_NOP)
	AM_RANGE(0x9407, 0x9407) AM_WRITE(spaceg_sound3_w)
	AM_RANGE(0x9800, 0x9800) AM_READ(input_port_0_r)
	AM_RANGE(0x9801, 0x9801) AM_READ(input_port_1_r)
	AM_RANGE(0x9802, 0x9802) AM_READ(input_port_2_r)
	AM_RANGE(0x9805, 0x9805) AM_READ(input_port_3_r)
	AM_RANGE(0x9806, 0x9806) AM_READ(input_port_4_r)
ADDRESS_MAP_END

static INPUT_PORTS_START( spaceg )
	PORT_START_TAG("9800")
	PORT_DIPNAME( 0x01, 0x00, DEF_STR( Coinage ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x02, 0x00, "Swarm Trips" )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPNAME( 0x0c, 0x00, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x00, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x0c, "6" )
	PORT_DIPNAME( 0x30, 0x00, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x20, "30000" )
	PORT_DIPSETTING(    0x10, "40000" )
	PORT_DIPSETTING(    0x30, "50000" )

	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, "10000" )
	PORT_DIPSETTING(    0x40, "30000" )
	PORT_DIPSETTING(    0x80, "40000" )
	PORT_DIPSETTING(    0xc0, "50000" )

	PORT_START_TAG("9801")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 )

	PORT_START_TAG("9802")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )

	PORT_START_TAG("9805")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY

	PORT_START_TAG("9806")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_2WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_2WAY PORT_COCKTAIL
INPUT_PORTS_END


static const char *spaceg_sample_names[] =
{
	"*invaders", "1.wav", "2.wav", "3.wav", "4.wav", "5.wav",
	"6.wav", "7.wav", "8.wav", "9.wav", 0
};

static struct Samplesinterface spaceg_samples = { 6, spaceg_sample_names };

static struct SN76477interface spaceg_sn76477 =
{
	0, 0, 0, 0, 0, RES_K(100), RES_K(56), RES_K(10),
	0, CAP_U(0.1), RES_K(8.2), 5.0, RES_K(120), CAP_U(1.0), 0, 0
};

static MACHINE_DRIVER_START( spaceg )
	MDRV_CPU_ADD(Z80,2500000)
	MDRV_CPU_PROGRAM_MAP(spaceg_map,0)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(TIME_IN_USEC(2500))
	MDRV_MACHINE_RESET(spaceg)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(256,256)
	MDRV_VISIBLE_AREA(0,255,32,255)
	MDRV_PALETTE_LENGTH(64)
	MDRV_PALETTE_INIT(spaceg)
	MDRV_VIDEO_UPDATE(spaceg)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(SN76477,0)
	MDRV_SOUND_CONFIG(spaceg_sn76477)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.25)
	MDRV_SOUND_ADD(SAMPLES,0)
	MDRV_SOUND_CONFIG(spaceg_samples)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS,"mono",0.5)
MACHINE_DRIVER_END

ROM_START( spaceg )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )	/* 64k for code */
	ROM_LOAD( "1.9a",		0x0000, 0x0400, CRC(d6f80b0e) SHA1(503e832c065bb302ec60ed239c4d99a605cb931a) )
	ROM_LOAD( "2.8a",		0x0400, 0x0400, CRC(708b8eec) SHA1(70f9b1506a653985d9d3eacba220f0a4eb241318) )
	ROM_LOAD( "3.9b",		0x0800, 0x0400, CRC(7f0b5cb4) SHA1(97a7125d046e7191b5d3be8f203d1bbb6f988209) )
	ROM_LOAD( "4.8b",		0x0c00, 0x0400, CRC(8b40a154) SHA1(f42bdd8e94090fc5eae58e833b8443300d3ce991) )
	ROM_LOAD( "5.9c",		0x1000, 0x0400, CRC(5279241c) SHA1(7278b6b037322b2f75311ed247f2de3c4816681b) )
	ROM_LOAD( "6.8c",		0x1400, 0x0400, CRC(9b84fe3a) SHA1(7ebeca10ee11d22f4af06be9f381f46864464ec2) )
	ROM_LOAD( "7.9d",		0x1800, 0x0400, CRC(95279b25) SHA1(367d129d4dd2cfea2a2f4703f41f24cc49453715) )
	ROM_LOAD( "8.8d",		0x1c00, 0x0400, CRC(6a824383) SHA1(7c43f2c7d1f070d93f6a8b5b4f7f97f3578bd91d) )
	ROM_LOAD( "13.9g",	0x3000, 0x0400, CRC(dccc386f) SHA1(5d493da3e7b8269314dd54f0b3ba9f71829a14da) )
	ROM_LOAD( "14.8g",	0x3400, 0x0400, CRC(dc9a10c2) SHA1(8fb2316d6e8aeef558d0da5029e2932abf47a6b4) )
	ROM_LOAD( "15.9h",	0x3800, 0x0400, CRC(55e2950d) SHA1(2241c3620c9a6df8b8bd234ccee9af5d3d19a5d4) )
	ROM_LOAD( "16.8h",	0x3c00, 0x0400, CRC(567259c4) SHA1(b2c3f7aaceabea075af6a43b89fb7331732278c8) )
	ROM_REGION( 0x40, REGION_PROMS, 0 )
	ROM_LOAD( "74s288.6a", 0x00, 0x20, CRC(ae1f4acd) SHA1(1d502b61db73cf6a4ac3d235455a5c464f12652a) )
	ROM_LOAD( "74s288.6b", 0x20, 0x20, CRC(ae1f4acd) SHA1(1d502b61db73cf6a4ac3d235455a5c464f12652a) )
ROM_END


GAME( 1979, spaceg, 0, spaceg, spaceg, 0, ROT270, "Omori Electric Co., Ltd.", "Space Guerrilla", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND)
