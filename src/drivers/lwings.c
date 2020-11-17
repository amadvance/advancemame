/***************************************************************************

  Legendary Wings
  Section Z
  Trojan
  Avengers

  Driver provided by Paul Leaman

To Do:
-   sectionz does "false contacts" on the coin counters, causing them to
    increment twice per coin.
-   clean up Avengers protection; it currently checks against hard-coded program
    counter rather than behaving as a memory-mapped black box.


Change Log:

FEB-2003 (AT)

- bug fixes:

    avengers061gre: missing sound effects in Avengers
  avengers37b16gre: screen artifacts in Avengers
    lwingsc37b7gre: incorrect sprite clipping in all games

Notes:

  avengers061gre2: corrupted graphics in Avengers' ending not fixed.
  This bug is not in the Japanese set "Buraiken".
  It might just be a bug in the original: the tiles for the character
  image are just not present in the US version, replaced by more tiles
  for the title animation. The tile map ROM is the same between the two
  versions.

  trojan37b1gre: stage 2-1 boss x flip glitches not fixed.
  This could be a side effect of sprite RAM buffering. Suggest buffering
  on-screen content instead of sprite memory.

  Previous clock settings were too low. Sometimes Avengers and Trojan
  could not finish clearing VRAM before a new frame is drawn and left
  behind screen artifacts. Avengers' second CPU was forced to pre-empt
  during soundlatch operations, resulting in double or missing sound
  effects.

***************************************************************************/

#include "driver.h"
#include "lwings.h"
#include "sound/2203intf.h"
#include "sound/msm5205.h"

/* Avengers runs on hardware almost identical to Trojan, but with a protection
 * device and some small changes to the memory map and videohardware.
 *
 * Background colors are fetched 64 bytes at a time and copied to palette RAM.
 *
 * Another function takes as input 2 pairs of (x,y) coordinates, and returns
 * a code reflecting the direction (8 angles) from one point to the other.
 */
static UINT8 avengers_param[4];
static int avengers_palette_pen;
static UINT8 *avengers_soundlatch2, avengers_soundstate=0;
static UINT8 avengers_adpcm;

WRITE8_HANDLER( avengers_adpcm_w )
{
	avengers_adpcm = data;
}

READ8_HANDLER( avengers_adpcm_r )
{
	return avengers_adpcm;
}

static WRITE8_HANDLER( lwings_bankswitch_w )
{
	unsigned char *RAM;
	int bank;

	/* bit 0 is flip screen */
	flip_screen_set(~data & 0x01);

	/* bits 1 and 2 select ROM bank */
	RAM = memory_region(REGION_CPU1);
	bank = (data & 0x06) >> 1;
	memory_set_bankptr(1,&RAM[0x10000 + bank*0x4000]);

	/* bit 3 enables NMI */
	interrupt_enable_w(0,data & 0x08);

	/* bits 6 and 7 are coin counters */
	coin_counter_w(1,data & 0x40);
	coin_counter_w(0,data & 0x80);
}

static INTERRUPT_GEN( lwings_interrupt )
{
	if (interrupt_enable_r(0))
		cpunum_set_input_line_and_vector(0,0,HOLD_LINE,0xd7); /* RST 10h */
}


static WRITE8_HANDLER( avengers_protection_w )
{
	int pc = activecpu_get_pc();

	if( pc == 0x2eeb )
	{
		avengers_param[0] = data;
	}
	else if( pc == 0x2f09 )
	{
		avengers_param[1] = data;
	}
	else if( pc == 0x2f26 )
	{
		avengers_param[2] = data;
	}
	else if( pc == 0x2f43 )
	{
		avengers_param[3] = data;
	}
	else if( pc == 0x0445 )
	{
		avengers_soundstate = 0x80;
		soundlatch_w( 0, data );
	}
}

static WRITE8_HANDLER( avengers_prot_bank_w )
{
	avengers_palette_pen = data*64;
}

static int avengers_fetch_paldata( void )
{
	static const char pal_data[] =
	/* page 1: 0x03,0x02,0x01,0x00 */
	"0000000000000000" "A65486A6364676D6" "C764C777676778A7" "A574E5E5C5756AE5"
	"0000000000000000" "F51785D505159405" "A637B6A636269636" "F45744E424348824"
	"0000000000000000" "A33263B303330203" "4454848454440454" "A27242C232523632"
	"0000000000000000" "1253327202421102" "3386437373631373" "41A331A161715461"
	"0000000000000000" "1341715000711203" "4442635191622293" "5143D48383D37186"
	"0000000000000000" "2432423000412305" "6633343302333305" "7234A565A5A4A2A8"
	"0000000000000000" "46232422A02234A7" "88241624A21454A7" "A3256747A665D3AA"
	"0000000000000000" "070406020003050B" "0A05090504050508" "05060A090806040C"

	/* page2: 0x07,0x06,0x05,0x04 */
	"0000000000000000" "2472030503230534" "6392633B23433B53" "0392846454346423"
	"0000000000000000" "1313052405050423" "3223754805354832" "323346A38686A332"
	"0000000000000000" "72190723070723D2" "81394776070776D1" "A15929F25959F2F1"
	"0000000000000000" "650706411A2A1168" "770737C43A3A3466" "87071F013C0C3175"
	"0000000000000000" "2001402727302020" "4403048F4A484344" "4A050B074E0E4440"
	"0000000000000000" "3003800C35683130" "5304035C587C5453" "5607080C5B265550"
	"0000000000000000" "4801D00043854245" "6C020038669A6569" "6604050A69446764"
	"0000000000000000" "0504000001030504" "0A05090504060307" "04090D0507010403"

	/* page3: 0x0b,0x0a,0x09,0x08 */
	"0000000000000000" "685A586937F777F7" "988A797A67A7A7A7" "B8CA898DC737F787"
	"0000000000000000" "4738A61705150505" "8797672835250535" "7777072A25350525"
	"0000000000000000" "3525642404340404" "6554453554440454" "5544053634540434"
	"0000000000000000" "2301923203430303" "4333834383630373" "3324034473730363"
	"0000000000000000" "3130304000762005" "5352525291614193" "6463635483D06581"
	"0000000000000000" "4241415100483107" "6463631302335304" "76757415A5A077A3"
	"0000000000000000" "53525282A02A43AA" "76747424A31565A5" "88888536A66089A4"
	"0000000000000000" "05040304000D050C" "0806050604070707" "0A0A060808000C06"

	/* page4: 0x0f,0x0e,0x0d,0x0c */
	"0000000000000000" "3470365956342935" "5590578997554958" "73C078A8C573687A"
	"0000000000000000" "5355650685030604" "2427362686042607" "010A070584010508"
	"0000000000000000" "0208432454022403" "737A243455733406" "000D050353000307"
	"0000000000000000" "000A023233003202" "424C134234424204" "000F241132001105"
	"0000000000000000" "3031113030300030" "5152215252512051" "7273337374723272"
	"0000000000000000" "4141214041411041" "6263326363623162" "8385448585834383"
	"0000000000000000" "5153225152512051" "7375437475734273" "9598559697946495"
	"0000000000000000" "0205020303020102" "0407040606040304" "060A060809060506"

	/* page5: 0x13,0x12,0x11,0x10 */
	"0000000000000000" "4151D141D3D177F7" "5454C44482C4A7A7" "0404D45491D4F787"
	"0000000000000000" "0303032374230505" "9696962673560535" "0505054502850525"
	"0000000000000000" "0303030355030404" "7777770754470454" "0606060603760434"
	"0000000000000000" "0505053547050303" "4949492945390373" "0808083804580363"
	"0000000000000000" "0B0C444023442005" "3D3F333433334193" "0000043504046581"
	"0000000000000000" "0809565085863107" "0B6A352374455304" "00700644050677A3"
	"0000000000000000" "06073879C8C843AA" "09492739A58765A5" "0050084A060889A4"
	"0000000000000000" "05060B070B0B050C" "0707090707090707" "00000B08070B0C06"

	/* page6: 0x17,0x16,0x15,0x14 */
	"0000000000000000" "0034308021620053" "0034417042512542" "0034526064502E31"
	"0000000000000000" "0106412032733060" "11A6522053628350" "22A6632072620D42"
	"0000000000000000" "1308223052242080" "2478233071235170" "3578243090230960"
	"0000000000000000" "2111334333331404" "3353324232324807" "45B5314131310837"
	"0000000000000000" "3232445444445302" "445443534343B725" "567642524242B745"
	"0000000000000000" "4343556555550201" "5575546454540524" "6787536353537554"
	"0000000000000000" "6474667676660100" "7696657575650423" "88A8647474645473"
	"0000000000000000" "0001070701050004" "0003060603040303" "0005050505040302";

	int bank = avengers_palette_pen/64;
	int offs = avengers_palette_pen%64;
	int page = bank/4;					/* 0..7 */
	int base = (3-(bank&3));			/* 0..3 */
	int row = offs&0xf;					/* 0..15 */
	int col = offs/16 + base*4;			/* 0..15 */
	int digit0 = pal_data[page*256*2 + (31-row*2)*16+col];
	int digit1 = pal_data[page*256*2 + (30-row*2)*16+col];
	int result;

	if( digit0>='A' ) digit0 += 10 - 'A'; else digit0 -= '0';
	if( digit1>='A' ) digit1 += 10 - 'A'; else digit1 -= '0';
	result = digit0 * 16 + digit1;

	if( (avengers_palette_pen&0x3f)!=0x3f ) avengers_palette_pen++;

	return result;
}

static READ8_HANDLER( avengers_protection_r )
{
	static const int xpos[8] = { 10, 7,  0, -7, -10, -7,   0,  7 };
	static const int ypos[8] = {  0, 7, 10,  7,   0, -7, -10, -7 };
	int best_dist = 0;
	int best_dir = 0;
	int x,y;
	int dx,dy,dist,dir;

	if( activecpu_get_pc() == 0x7c7 )
	{
		/* palette data */
		return avengers_fetch_paldata();
	}

	/*  Point to Angle Function

        Input: two cartesian points
        Output: direction code (north,northeast,east,...)
     */
	x = avengers_param[0] - avengers_param[2];
	y = avengers_param[1] - avengers_param[3];
	for( dir=0; dir<8; dir++ )
	{
		dx = xpos[dir]-x;
		dy = ypos[dir]-y;
		dist = dx*dx+dy*dy;
		if( dist < best_dist || dir==0 )
		{
			best_dir = dir;
			best_dist = dist;
		}
	}
	return best_dir<<5;
}

static READ8_HANDLER( avengers_soundlatch2_r )
{
	UINT8 data = *avengers_soundlatch2 | avengers_soundstate;
	avengers_soundstate = 0;
	return(data);
}

static WRITE8_HANDLER( msm5205_w )
{
	MSM5205_reset_w(offset,(data>>7)&1);
	MSM5205_data_w(offset,data);
	MSM5205_vclk_w(offset,1);
	MSM5205_vclk_w(offset,0);
}

static ADDRESS_MAP_START( avengers_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xf7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf808, 0xf808) AM_READ(input_port_0_r)
	AM_RANGE(0xf809, 0xf809) AM_READ(input_port_1_r)
	AM_RANGE(0xf80a, 0xf80a) AM_READ(input_port_2_r)
	AM_RANGE(0xf80b, 0xf80b) AM_READ(input_port_3_r)
	AM_RANGE(0xf80c, 0xf80c) AM_READ(input_port_4_r)
	AM_RANGE(0xf80d, 0xf80d) AM_READ(avengers_protection_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( avengers_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xddff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xde00, 0xdf7f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xdf80, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(lwings_fgvideoram_w) AM_BASE(&lwings_fgvideoram)
	AM_RANGE(0xe800, 0xefff) AM_WRITE(lwings_bg1videoram_w) AM_BASE(&lwings_bg1videoram)
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xf800, 0xf801) AM_WRITE(lwings_bg1_scrollx_w)
	AM_RANGE(0xf802, 0xf803) AM_WRITE(lwings_bg1_scrolly_w)
	AM_RANGE(0xf804, 0xf804) AM_WRITE(trojan_bg2_scrollx_w)
	AM_RANGE(0xf805, 0xf805) AM_WRITE(trojan_bg2_image_w)
	AM_RANGE(0xf808, 0xf808) AM_WRITE(MWA8_NOP) /* ? */
	AM_RANGE(0xf809, 0xf809) AM_WRITE(avengers_protection_w)
	AM_RANGE(0xf80c, 0xf80c) AM_WRITE(avengers_prot_bank_w)
	AM_RANGE(0xf80d, 0xf80d) AM_WRITE(avengers_adpcm_w)
	AM_RANGE(0xf80e, 0xf80e) AM_WRITE(lwings_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 ) /* common to trojan and lwings */
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x8000, 0xbfff) AM_READ(MRA8_BANK1)
	AM_RANGE(0xc000, 0xf7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xf808, 0xf808) AM_READ(input_port_0_r)
	AM_RANGE(0xf809, 0xf809) AM_READ(input_port_1_r)
	AM_RANGE(0xf80a, 0xf80a) AM_READ(input_port_2_r)
	AM_RANGE(0xf80b, 0xf80b) AM_READ(input_port_3_r)
	AM_RANGE(0xf80c, 0xf80c) AM_READ(input_port_4_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 ) /* lwings */
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xddff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xde00, 0xdfff) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(lwings_fgvideoram_w) AM_BASE(&lwings_fgvideoram)
	AM_RANGE(0xe800, 0xefff) AM_WRITE(lwings_bg1videoram_w) AM_BASE(&lwings_bg1videoram)
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xf808, 0xf809) AM_WRITE(lwings_bg1_scrollx_w)
	AM_RANGE(0xf80a, 0xf80b) AM_WRITE(lwings_bg1_scrolly_w)
	AM_RANGE(0xf80c, 0xf80c) AM_WRITE(soundlatch_w)
	AM_RANGE(0xf80d, 0xf80d) AM_WRITE(watchdog_reset_w)
	AM_RANGE(0xf80e, 0xf80e) AM_WRITE(lwings_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trojan_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xbfff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xddff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xde00, 0xdf7f) AM_WRITE(MWA8_RAM) AM_BASE(&spriteram) AM_SIZE(&spriteram_size)
	AM_RANGE(0xdf80, 0xdfff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe7ff) AM_WRITE(lwings_fgvideoram_w) AM_BASE(&lwings_fgvideoram)
	AM_RANGE(0xe800, 0xefff) AM_WRITE(lwings_bg1videoram_w) AM_BASE(&lwings_bg1videoram)
	AM_RANGE(0xf000, 0xf3ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split2_w) AM_BASE(&paletteram_2)
	AM_RANGE(0xf400, 0xf7ff) AM_WRITE(paletteram_RRRRGGGGBBBBxxxx_split1_w) AM_BASE(&paletteram)
	AM_RANGE(0xf800, 0xf801) AM_WRITE(lwings_bg1_scrollx_w)
	AM_RANGE(0xf802, 0xf803) AM_WRITE(lwings_bg1_scrolly_w)
	AM_RANGE(0xf804, 0xf804) AM_WRITE(trojan_bg2_scrollx_w)
	AM_RANGE(0xf805, 0xf805) AM_WRITE(trojan_bg2_image_w)
	AM_RANGE(0xf808, 0xf808) AM_WRITE(MWA8_NOP) /* watchdog */
	AM_RANGE(0xf80c, 0xf80c) AM_WRITE(soundlatch_w)
	AM_RANGE(0xf80d, 0xf80d) AM_WRITE(soundlatch2_w)
	AM_RANGE(0xf80e, 0xf80e) AM_WRITE(lwings_bankswitch_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_READ(MRA8_RAM)
	AM_RANGE(0xc800, 0xc800) AM_READ(soundlatch_r)
	AM_RANGE(0xe006, 0xe006) AM_READ(avengers_soundlatch2_r) //AT: (avengers061gre)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sound_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)
	AM_RANGE(0xc000, 0xc7ff) AM_WRITE(MWA8_RAM)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0xe001, 0xe001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0xe002, 0xe002) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0xe003, 0xe003) AM_WRITE(YM2203_write_port_1_w)
	AM_RANGE(0xe006, 0xe006) AM_WRITE(MWA8_RAM) AM_BASE(&avengers_soundlatch2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( adpcm_readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0xffff) AM_READ(MRA8_ROM)
ADDRESS_MAP_END

/* Yes, _no_ ram */
static ADDRESS_MAP_START( adpcm_writemem, ADDRESS_SPACE_PROGRAM, 8 )
/*  AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_ROM) avoid cluttering up error.log */
	AM_RANGE(0x0000, 0xffff) AM_WRITE(MWA8_NOP)
ADDRESS_MAP_END

static ADDRESS_MAP_START( avengers_adpcm_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(avengers_adpcm_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( trojan_adpcm_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch2_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( adpcm_readport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READ(soundlatch_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( adpcm_writeport, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x01, 0x01) AM_WRITE(msm5205_w)
ADDRESS_MAP_END

INPUT_PORTS_START( sectionz )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x04, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x04, "Difficult" )
	PORT_DIPSETTING(    0x00, "Very Difficult" )
	PORT_DIPNAME( 0x38, 0x38, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x38, "20000 50000" )
	PORT_DIPSETTING(    0x18, "20000 60000" )
	PORT_DIPSETTING(    0x28, "20000 70000" )
	PORT_DIPSETTING(    0x08, "30000 60000" )
	PORT_DIPSETTING(    0x30, "30000 70000" )
	PORT_DIPSETTING(    0x10, "30000 80000" )
	PORT_DIPSETTING(    0x20, "40000 100000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0xc0, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright One Player" )
	PORT_DIPSETTING(    0x40, "Upright Two Players" )
/*      PORT_DIPSETTING(    0x80, "???" )       probably unused */
	PORT_DIPSETTING(    0xc0, DEF_STR( Cocktail ) )
INPUT_PORTS_END

INPUT_PORTS_START( lwings )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x08, "5" )
	PORT_DIPSETTING(    0x00, "6" )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_4C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x06, 0x06, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x06, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0xe0, "20000 and every 50000" )
	PORT_DIPSETTING(    0x60, "20000 and every 60000" )
	PORT_DIPSETTING(    0xa0, "20000 and every 70000" )
	PORT_DIPSETTING(    0x20, "30000 and every 60000" )
	PORT_DIPSETTING(    0xc0, "30000 and every 70000" )
	PORT_DIPSETTING(    0x40, "30000 and every 80000" )
	PORT_DIPSETTING(    0x80, "40000 and every 100000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
INPUT_PORTS_END

INPUT_PORTS_START( trojan )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x03, DEF_STR( Cocktail ) )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x0c, "20000 70000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x1c, "30000 60000" )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x14, "30000 80000" )
	PORT_DIPSETTING(    0x04, "40000 80000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0xe0, 0xe0, "Starting Level" )
	PORT_DIPSETTING(    0xe0, "1" )
	PORT_DIPSETTING(    0xc0, "2" )
	PORT_DIPSETTING(    0xa0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x60, "5" )
	PORT_DIPSETTING(    0x40, "6" )
/* 0x00 and 0x20 start at level 6 */

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

/* Trojan with level selection - starting level dip switches not used */
INPUT_PORTS_START( trojanls )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_COCKTAIL
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_COCKTAIL
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_COCKTAIL
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSW0 */
	PORT_DIPNAME( 0x03, 0x00, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x00, "Upright 1 Player" )
	PORT_DIPSETTING(    0x02, "Upright 2 Players" )
	PORT_DIPSETTING(    0x03, DEF_STR( Cocktail ) )
/* 0x01 same as 0x02 or 0x03 */
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Bonus_Life ) )
	PORT_DIPSETTING(    0x10, "20000 60000" )
	PORT_DIPSETTING(    0x0c, "20000 70000" )
	PORT_DIPSETTING(    0x08, "20000 80000" )
	PORT_DIPSETTING(    0x1c, "30000 60000" )
	PORT_DIPSETTING(    0x18, "30000 70000" )
	PORT_DIPSETTING(    0x14, "30000 80000" )
	PORT_DIPSETTING(    0x04, "40000 80000" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START      /* DSW1 */
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x03, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x02, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x01, DEF_STR( 1C_3C ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_1C ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0x20, "2" )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Yes ) )
INPUT_PORTS_END

INPUT_PORTS_START( avengers )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_COIN2 )

	PORT_START      /* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )    /* probably unused */

	PORT_START      /* DSWB */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x0c, 0x0c, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Hard ) )
	PORT_DIPNAME( 0x30, 0x30, "Bonus" )
	PORT_DIPSETTING(    0x30, "20k 60k" )
	PORT_DIPSETTING(    0x10, "20k 70k" )
	PORT_DIPSETTING(    0x20, "20k 80k" )
	PORT_DIPSETTING(    0x00, "30k 80k" )
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Lives ) )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x40, "4" )
	PORT_DIPSETTING(    0x80, "5" )
	PORT_DIPSETTING(    0x00, "6" )

	PORT_START      /* DSWA */
	PORT_SERVICE( 0x01, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02, 0x02, DEF_STR( Flip_Screen ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x1c, 0x1c, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x08, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0x1c, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x0c, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0x14, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x04, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0x18, DEF_STR( 1C_6C ) )
	PORT_DIPNAME( 0xe0, 0xe0, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(    0x80, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(    0xe0, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(    0x60, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(    0xa0, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_4C ) )
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_6C ) )
INPUT_PORTS_END

static const gfx_layout charlayout =
{
	8,8,
	RGN_FRAC(1,1),
	2,
	{ 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_layout spritelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+4, RGN_FRAC(1,2)+0, 4, 0 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};

static const gfx_layout bg1_tilelayout =
{
	16,16,
	RGN_FRAC(1,4),
	4,
	{ RGN_FRAC(3,4), RGN_FRAC(2,4), RGN_FRAC(1,4), RGN_FRAC(0,4) },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8,
			8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8
};

static const gfx_layout bg2_tilelayout =
{
	16,16,
	RGN_FRAC(1,2),
	4,
	{ RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 0, 4 },
	{ 0, 1, 2, 3, 8+0, 8+1, 8+2, 8+3,
			32*8+0, 32*8+1, 32*8+2, 32*8+3, 33*8+0, 33*8+1, 33*8+2, 33*8+3 },
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16,
			8*16, 9*16, 10*16, 11*16, 12*16, 13*16, 14*16, 15*16 },
	64*8
};


static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &charlayout,     512, 16 }, /* colors 512-575 */
	{ REGION_GFX2, 0, &bg1_tilelayout,   0,  8 }, /* colors   0-127 */
	{ REGION_GFX3, 0, &spritelayout,   384,  8 }, /* colors 384-511 */
	{ -1 } /* end of array */
};

static const gfx_decode gfxdecodeinfo_trojan[] =
{
	{ REGION_GFX1, 0, &charlayout,     768, 16 }, /* colors 768-831 */
	{ REGION_GFX2, 0, &bg1_tilelayout, 256,  8 }, /* colors 256-383 */
	{ REGION_GFX3, 0, &spritelayout,   640,  8 }, /* colors 640-767 */
	{ REGION_GFX4, 0, &bg2_tilelayout,   0,  8 }, /* colors   0-127 */
	{ -1 } /* end of array */
};

static struct MSM5205interface msm5205_interface =
{
	0,				/* interrupt function */
	MSM5205_SEX_4B	/* slave mode */
};

static MACHINE_DRIVER_START( lwings )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)        /* 4 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(lwings_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(lwings)
	MDRV_VIDEO_EOF(lwings)
	MDRV_VIDEO_UPDATE(lwings)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( trojan )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000)        /* 4 MHz (?) */
	MDRV_CPU_PROGRAM_MAP(readmem,trojan_writemem)
	MDRV_CPU_VBLANK_INT(lwings_interrupt,1)

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_CPU_ADD(Z80, 4000000) // 3.579545 Mhz (?)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(adpcm_readmem,adpcm_writemem)
	MDRV_CPU_IO_MAP(trojan_adpcm_readport,adpcm_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,TIME_IN_HZ(4000))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_trojan)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(trojan)
	MDRV_VIDEO_EOF(lwings)
	MDRV_VIDEO_UPDATE(trojan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( avengers )

	/* basic machine hardware */
	MDRV_CPU_ADD(Z80, 6000000) //AT: (avengers37b16gre)
	MDRV_CPU_PROGRAM_MAP(avengers_readmem,avengers_writemem)
	MDRV_CPU_VBLANK_INT(nmi_line_pulse,1) // RST 38h triggered by software

	MDRV_CPU_ADD(Z80, 4000000)
	/* audio CPU */
	MDRV_CPU_PROGRAM_MAP(sound_readmem,sound_writemem)
	MDRV_CPU_VBLANK_INT(irq0_line_hold,4)

	MDRV_CPU_ADD(Z80, 4000000) // 3.579545 Mhz (?)
	/* audio CPU */	/* ? */
	MDRV_CPU_PROGRAM_MAP(adpcm_readmem,adpcm_writemem)
	MDRV_CPU_IO_MAP(avengers_adpcm_readport,adpcm_writeport)
	MDRV_CPU_PERIODIC_INT(irq0_line_hold,TIME_IN_HZ(4000))

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_BUFFERS_SPRITERAM)
	MDRV_SCREEN_SIZE(32*8, 32*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 1*8, 31*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo_trojan)
	MDRV_PALETTE_LENGTH(1024)

	MDRV_VIDEO_START(avengers)
	MDRV_VIDEO_EOF(lwings)
	MDRV_VIDEO_UPDATE(trojan)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_ROUTE(0, "mono", 0.20)
	MDRV_SOUND_ROUTE(1, "mono", 0.20)
	MDRV_SOUND_ROUTE(2, "mono", 0.20)
	MDRV_SOUND_ROUTE(3, "mono", 0.10)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
MACHINE_DRIVER_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( lwings )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_lw01.bin",  0x00000, 0x8000, CRC(b55a7f60) SHA1(e28cc540892a9ad050693900356744f8f5d05237) )
	ROM_LOAD( "7c_lw02.bin",  0x10000, 0x8000, CRC(a5efbb1b) SHA1(9126efa78fd39a50032826d0b4bd3acffceba508) )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, CRC(ec5cc201) SHA1(1043c6a9678c18fef920be91b0796c93b83e0f73) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, CRC(a20337a2) SHA1(649e13a69ad9154657894fa7bf7c6e49b029a506) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, CRC(091d923c) SHA1(d686c860f147c4749ac1ee23cde5a7b570312622) ) /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3e_lw14.bin",  0x00000, 0x8000, CRC(5436392c) SHA1(c33925c87e61aad278bef57fe9a8148ff2d4377f) ) /* tiles */
	ROM_LOAD( "1e_lw08.bin",  0x08000, 0x8000, CRC(b491bbbb) SHA1(474fc84667d978abfd5c9d94cf1e2ce55f70f865) )
	ROM_LOAD( "3d_lw13.bin",  0x10000, 0x8000, CRC(fdd1908a) SHA1(0b2de3d2f8e50f11c57822147bec6f2d9c9ff586) )
	ROM_LOAD( "1d_lw07.bin",  0x18000, 0x8000, CRC(5c73d406) SHA1(85386f6b387a85d8df7d800ffcecb2590613a42c) )
	ROM_LOAD( "3b_lw12.bin",  0x20000, 0x8000, CRC(32e17b3c) SHA1(db5488b7c48cd0df4571104169e42ff4094f1abd) )
	ROM_LOAD( "1b_lw06.bin",  0x28000, 0x8000, CRC(52e533c1) SHA1(9f333c9fb6e35db1264286be5b4f7e4dd18150de) )
	ROM_LOAD( "3f_lw15.bin",  0x30000, 0x8000, CRC(99e134ba) SHA1(9818a6ad3146ed95b29b9aeba2331a0e8e2a76b5) )
	ROM_LOAD( "1f_lw09.bin",  0x38000, 0x8000, CRC(c8f28777) SHA1(d08571d34f96e7d33506e374d047647f131dce71) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "3j_lw17.bin",  0x00000, 0x8000, CRC(5ed1bc9b) SHA1(717c80e180bc38cb66ac0135709e8df2cd7375aa) )  /* sprites */
	ROM_LOAD( "1j_lw11.bin",  0x08000, 0x8000, CRC(2a0790d6) SHA1(a0a8b5748b562e4c44cdb2e48cefbea0d4e9e6a8) )
	ROM_LOAD( "3h_lw16.bin",  0x10000, 0x8000, CRC(e8834006) SHA1(7d7ec16be325cbbaccf5dce101cb7bc719a5bef2) )
	ROM_LOAD( "1h_lw10.bin",  0x18000, 0x8000, CRC(b693f5a5) SHA1(134e255e670848f8aec82fcd848d1a4f1aefa636) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.15g",   0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
ROM_END

ROM_START( lwings2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "u13-l",        0x00000, 0x8000, CRC(3069c01c) SHA1(84dfffeb58f7c5a75d2a59c2ce72c6db813af1be) )
	ROM_LOAD( "u14-k",        0x10000, 0x8000, CRC(5d91c828) SHA1(e0b9eab5b290203f71de27a78689adb2e7b07cea) )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, CRC(ec5cc201) SHA1(1043c6a9678c18fef920be91b0796c93b83e0f73) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, CRC(a20337a2) SHA1(649e13a69ad9154657894fa7bf7c6e49b029a506) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, CRC(091d923c) SHA1(d686c860f147c4749ac1ee23cde5a7b570312622) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b_03e.rom",    0x00000, 0x8000, CRC(176e3027) SHA1(31947205c7a28d25b5982a9e6c079112c404d6b4) )  /* tiles */
	ROM_LOAD( "b_01e.rom",    0x08000, 0x8000, CRC(f5d25623) SHA1(ff520df50011af5688be7e88712faa8f8436b462) )
	ROM_LOAD( "b_03d.rom",    0x10000, 0x8000, CRC(001caa35) SHA1(2042136c592ce124a321fc6d05447b13a612b6b9) )
	ROM_LOAD( "b_01d.rom",    0x18000, 0x8000, CRC(0ba008c3) SHA1(ed5c0d7191d021d6445f8f31a61eb99172fd2dc1) )
	ROM_LOAD( "b_03b.rom",    0x20000, 0x8000, CRC(4f8182e9) SHA1(d0db174995be3937f5e5fe62ffe2112583dd78d7) )
	ROM_LOAD( "b_01b.rom",    0x28000, 0x8000, CRC(f1617374) SHA1(01b77bc16c1e7d669f62adf759f820bc0241d959) )
	ROM_LOAD( "b_03f.rom",    0x30000, 0x8000, CRC(9b374dcc) SHA1(3cb4243c304579536880ced86f0118c43413c1b4) )
	ROM_LOAD( "b_01f.rom",    0x38000, 0x8000, CRC(23654e0a) SHA1(d97689b348ac4e1b380ad65133ede4bdd5ecaaee) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b_03j.rom",    0x00000, 0x8000, CRC(8f3c763a) SHA1(b34e62ab6652a2e9783351dde6a60af38a6ba084) )  /* sprites */
	ROM_LOAD( "b_01j.rom",    0x08000, 0x8000, CRC(7cc90a1d) SHA1(ff194749397f06ad054917664bd4583b0e4e8d92) )
	ROM_LOAD( "b_03h.rom",    0x10000, 0x8000, CRC(7d58f532) SHA1(debfb14cd1cefa1f61a8650cbc9f6e0fff3abe8b) )
	ROM_LOAD( "b_01h.rom",    0x18000, 0x8000, CRC(3e396eda) SHA1(a736f108e0ed5fab6177f0d8a21feab8b686ee85) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.15g",   0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
ROM_END

ROM_START( lwingsjp )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "a_06c.rom",    0x00000, 0x8000, CRC(2068a738) SHA1(1bbceee8138cdc3832a9330b967561b78b03933e) )
	ROM_LOAD( "a_07c.rom",    0x10000, 0x8000, CRC(d6a2edc4) SHA1(ce7eef643b1570cab241355bfd7c2d7adb1e74b6) )
	ROM_LOAD( "9c_lw03.bin",  0x18000, 0x8000, CRC(ec5cc201) SHA1(1043c6a9678c18fef920be91b0796c93b83e0f73) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_lw04.bin", 0x0000, 0x8000, CRC(a20337a2) SHA1(649e13a69ad9154657894fa7bf7c6e49b029a506) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9h_lw05.bin",  0x00000, 0x4000, CRC(091d923c) SHA1(d686c860f147c4749ac1ee23cde5a7b570312622) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "b_03e.rom",    0x00000, 0x8000, CRC(176e3027) SHA1(31947205c7a28d25b5982a9e6c079112c404d6b4) )  /* tiles */
	ROM_LOAD( "b_01e.rom",    0x08000, 0x8000, CRC(f5d25623) SHA1(ff520df50011af5688be7e88712faa8f8436b462) )
	ROM_LOAD( "b_03d.rom",    0x10000, 0x8000, CRC(001caa35) SHA1(2042136c592ce124a321fc6d05447b13a612b6b9) )
	ROM_LOAD( "b_01d.rom",    0x18000, 0x8000, CRC(0ba008c3) SHA1(ed5c0d7191d021d6445f8f31a61eb99172fd2dc1) )
	ROM_LOAD( "b_03b.rom",    0x20000, 0x8000, CRC(4f8182e9) SHA1(d0db174995be3937f5e5fe62ffe2112583dd78d7) )
	ROM_LOAD( "b_01b.rom",    0x28000, 0x8000, CRC(f1617374) SHA1(01b77bc16c1e7d669f62adf759f820bc0241d959) )
	ROM_LOAD( "b_03f.rom",    0x30000, 0x8000, CRC(9b374dcc) SHA1(3cb4243c304579536880ced86f0118c43413c1b4) )
	ROM_LOAD( "b_01f.rom",    0x38000, 0x8000, CRC(23654e0a) SHA1(d97689b348ac4e1b380ad65133ede4bdd5ecaaee) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "b_03j.rom",    0x00000, 0x8000, CRC(8f3c763a) SHA1(b34e62ab6652a2e9783351dde6a60af38a6ba084) )  /* sprites */
	ROM_LOAD( "b_01j.rom",    0x08000, 0x8000, CRC(7cc90a1d) SHA1(ff194749397f06ad054917664bd4583b0e4e8d92) )
	ROM_LOAD( "b_03h.rom",    0x10000, 0x8000, CRC(7d58f532) SHA1(debfb14cd1cefa1f61a8650cbc9f6e0fff3abe8b) )
	ROM_LOAD( "b_01h.rom",    0x18000, 0x8000, CRC(3e396eda) SHA1(a736f108e0ed5fab6177f0d8a21feab8b686ee85) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "63s141.15g",   0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
ROM_END

ROM_START( sectionz )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "6c_sz01.bin",  0x00000, 0x8000, CRC(69585125) SHA1(a341e3a5507e961d5763be6acf420695bb32709e) )
	ROM_LOAD( "7c_sz02.bin",  0x10000, 0x8000, CRC(22f161b8) SHA1(094ee6b6c8750de682c1ba4e387b31d58f734604) )
	ROM_LOAD( "9c_sz03.bin",  0x18000, 0x8000, CRC(4c7111ed) SHA1(57c6ad6a86c64ffb17ec8f584c5e003440390344) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, CRC(a6073566) SHA1(d7dc382ba780cc4f25f7d7e7630cff1090488843) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9h_sz05.bin",  0x00000, 0x4000, CRC(3173ba2e) SHA1(4e0b4fc1efd7b5eb598fe5d5d7f1de01ba52dbdc) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3e_sz14.bin",  0x00000, 0x8000, CRC(63782e30) SHA1(9a23b4849ff210bd4482e4e8c57e578387d19c46) )  /* tiles */
	ROM_LOAD( "1e_sz08.bin",  0x08000, 0x8000, CRC(d57d9f13) SHA1(1d07b9eca588985a5e0cec27394ad5b3191c8dc4) )
	ROM_LOAD( "3d_sz13.bin",  0x10000, 0x8000, CRC(1b3d4d7f) SHA1(66eed80865b2a480762cc8d9fda9e82c9c463e71) )
	ROM_LOAD( "1d_sz07.bin",  0x18000, 0x8000, CRC(f5b3a29f) SHA1(0dbf8caf09e319fb2303e7e865f55effa59c761c) )
	ROM_LOAD( "3b_sz12.bin",  0x20000, 0x8000, CRC(11d47dfd) SHA1(bc8a7369ed671ef714472ead2d17228de2567865) )
	ROM_LOAD( "1b_sz06.bin",  0x28000, 0x8000, CRC(df703b68) SHA1(ae98a718dab96f3c0e4827e78938c3984a6641d6) )
	ROM_LOAD( "3f_sz15.bin",  0x30000, 0x8000, CRC(36bb9bf7) SHA1(53f6d375947f9fb28f295935a0fe27f826234765) )
	ROM_LOAD( "1f_sz09.bin",  0x38000, 0x8000, CRC(da8f06c9) SHA1(c0eb4406cdf0d5f25bab28de8222b28da9a97943) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "3j_sz17.bin",  0x00000, 0x8000, CRC(8df7b24a) SHA1(078789d0912010fa96b6f267de3ebec9beca6681) )  /* sprites */
	ROM_LOAD( "1j_sz11.bin",  0x08000, 0x8000, CRC(685d4c54) SHA1(ef580e04b6dcb0b65f12c519a4085c98ac0bc261) )
	ROM_LOAD( "3h_sz16.bin",  0x10000, 0x8000, CRC(500ff2bb) SHA1(eb20148388e5271b1fed23a536035e8490474489) )
	ROM_LOAD( "1h_sz10.bin",  0x18000, 0x8000, CRC(00b3d244) SHA1(ed923bd5371f4665744344b94df3547c5db5058c) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114e.15g",  0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
ROM_END

ROM_START( sctionza )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "sz-01a.bin",   0x00000, 0x8000, CRC(98df49fd) SHA1(80d7d9f83ea2f606e48606dbfe69cf347aadf079) )
	ROM_LOAD( "7c_sz02.bin",  0x10000, 0x8000, CRC(22f161b8) SHA1(094ee6b6c8750de682c1ba4e387b31d58f734604) )
	ROM_LOAD( "sz-03j.bin",   0x18000, 0x8000, CRC(94547abf) SHA1(9af9e76e6657d7fd742630cfe2f2eb76d231dec4) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "11e_sz04.bin", 0x0000, 0x8000, CRC(a6073566) SHA1(d7dc382ba780cc4f25f7d7e7630cff1090488843) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "9h_sz05.bin",  0x00000, 0x4000, CRC(3173ba2e) SHA1(4e0b4fc1efd7b5eb598fe5d5d7f1de01ba52dbdc) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "3e_sz14.bin",  0x00000, 0x8000, CRC(63782e30) SHA1(9a23b4849ff210bd4482e4e8c57e578387d19c46) )  /* tiles */
	ROM_LOAD( "1e_sz08.bin",  0x08000, 0x8000, CRC(d57d9f13) SHA1(1d07b9eca588985a5e0cec27394ad5b3191c8dc4) )
	ROM_LOAD( "3d_sz13.bin",  0x10000, 0x8000, CRC(1b3d4d7f) SHA1(66eed80865b2a480762cc8d9fda9e82c9c463e71) )
	ROM_LOAD( "1d_sz07.bin",  0x18000, 0x8000, CRC(f5b3a29f) SHA1(0dbf8caf09e319fb2303e7e865f55effa59c761c) )
	ROM_LOAD( "3b_sz12.bin",  0x20000, 0x8000, CRC(11d47dfd) SHA1(bc8a7369ed671ef714472ead2d17228de2567865) )
	ROM_LOAD( "1b_sz06.bin",  0x28000, 0x8000, CRC(df703b68) SHA1(ae98a718dab96f3c0e4827e78938c3984a6641d6) )
	ROM_LOAD( "3f_sz15.bin",  0x30000, 0x8000, CRC(36bb9bf7) SHA1(53f6d375947f9fb28f295935a0fe27f826234765) )
	ROM_LOAD( "1f_sz09.bin",  0x38000, 0x8000, CRC(da8f06c9) SHA1(c0eb4406cdf0d5f25bab28de8222b28da9a97943) )

	ROM_REGION( 0x20000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "3j_sz17.bin",  0x00000, 0x8000, CRC(8df7b24a) SHA1(078789d0912010fa96b6f267de3ebec9beca6681) )  /* sprites */
	ROM_LOAD( "1j_sz11.bin",  0x08000, 0x8000, CRC(685d4c54) SHA1(ef580e04b6dcb0b65f12c519a4085c98ac0bc261) )
	ROM_LOAD( "3h_sz16.bin",  0x10000, 0x8000, CRC(500ff2bb) SHA1(eb20148388e5271b1fed23a536035e8490474489) )
	ROM_LOAD( "1h_sz10.bin",  0x18000, 0x8000, CRC(00b3d244) SHA1(ed923bd5371f4665744344b94df3547c5db5058c) )

	ROM_REGION( 0x0100, REGION_PROMS, 0 )
	ROM_LOAD( "mb7114e.15g",  0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
ROM_END

ROM_START( trojan )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "t4",           0x00000, 0x8000, CRC(c1bbeb4e) SHA1(248ae4184d25b642b282ef44ac729c0f7952834d) )
	ROM_LOAD( "t6",           0x10000, 0x8000, CRC(d49592ef) SHA1(b538bac3c73f35474cc6745a4e4dc3ab6217eaac) )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, CRC(9273b264) SHA1(ab23b16bf53b5baf106ea0cac50754aa967300cf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, CRC(21154797) SHA1(e1a3006746cc2d692ecd4369cc0a77c596abd60b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, CRC(1c0f91b2) SHA1(163bf6aa1936994659661653eabdc368199b0070) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, CRC(581a2b4c) SHA1(705b499da5d01a946f06234a4bab72a291c79034) )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, CRC(285a052b) SHA1(8ce055c7ac9ce1560552fc7f857f60e7a5af0779) )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, CRC(aeb693f7) SHA1(a811ea67abdd4adfc68224257973802e2a36fc36) )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, CRC(dfb0fe5c) SHA1(82542692ab71b9126e6c301ed0803db58734273c) )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, CRC(d3a4c9d1) SHA1(3d787f6a4583b80f2d254947890f676cda17b242) )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, CRC(00f0f4fd) SHA1(3a862360a26ae1c3a945949d6d47f88aa4b728a4) )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, CRC(dff2ee02) SHA1(4877c52f2a0e24a95bcda1d8636ea993c2c3c240) )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, CRC(14bfac18) SHA1(84266140e9679912dbbb185fd3b9b497297dcb16) )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, CRC(71ba8a6d) SHA1(53ff6850f9f8a19c57c19ef56fd45975f0ec133e) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, CRC(862c4713) SHA1(a3707d950f4f5de5208e64207016ef2256eb8c5b) )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, CRC(d86f8cbd) SHA1(8a16130632e20ad3cae8e817da7b661c3ac60f30) )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, CRC(12a73b3f) SHA1(6bb54d4fdf01fd2cdd76a0b47be4d8cae8a1e19b) )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, CRC(bb1a2769) SHA1(9884dceb00e6d88908a1c107b83cc1711b0cf1f7) )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, CRC(39daafd4) SHA1(1e49a273f51cccec3141d540032fd9a3041a3cbd) )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, CRC(94615d2a) SHA1(112a299ff1bb878cf7e24c2ad337440c3df0a6d5) )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, CRC(66c642bd) SHA1(b57f0f8d8e21c9f94ffc0e9f9304b5ab5d4ed3fc) )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, CRC(81d5ab36) SHA1(31103759676a8d1badaf7bde79e7f28d69486106) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, CRC(6e38c6fa) SHA1(c51228d5d063dcf4361c76fa49dbe18db80c50a0) )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, CRC(14fc6cf2) SHA1(080a2d845cb36c637f76d8e062725bd13dd1aed0) )

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "tb23.bin",     0x00000, 0x08000, CRC(eda13c0e) SHA1(806f0819af8b25c2b46de3d1fd95bc9c0e883bd9) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.7j",  0x0000, 0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "mb7114e.1e",   0x0100, 0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

ROM_START( trojanr )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "tb04.bin",     0x00000, 0x8000, CRC(92670f27) SHA1(d2cb35a9fade971770db1a58e961bc03cc3de6ff) )
	ROM_LOAD( "tb06.bin",     0x10000, 0x8000, CRC(a4951173) SHA1(2d3db0ee3a1680f2cce21cf15f8bd434325d8648) )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, CRC(9273b264) SHA1(ab23b16bf53b5baf106ea0cac50754aa967300cf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, CRC(21154797) SHA1(e1a3006746cc2d692ecd4369cc0a77c596abd60b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 ) /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, CRC(1c0f91b2) SHA1(163bf6aa1936994659661653eabdc368199b0070) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, CRC(581a2b4c) SHA1(705b499da5d01a946f06234a4bab72a291c79034) )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, CRC(285a052b) SHA1(8ce055c7ac9ce1560552fc7f857f60e7a5af0779) )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, CRC(aeb693f7) SHA1(a811ea67abdd4adfc68224257973802e2a36fc36) )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, CRC(dfb0fe5c) SHA1(82542692ab71b9126e6c301ed0803db58734273c) )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, CRC(d3a4c9d1) SHA1(3d787f6a4583b80f2d254947890f676cda17b242) )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, CRC(00f0f4fd) SHA1(3a862360a26ae1c3a945949d6d47f88aa4b728a4) )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, CRC(dff2ee02) SHA1(4877c52f2a0e24a95bcda1d8636ea993c2c3c240) )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, CRC(14bfac18) SHA1(84266140e9679912dbbb185fd3b9b497297dcb16) )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, CRC(71ba8a6d) SHA1(53ff6850f9f8a19c57c19ef56fd45975f0ec133e) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, CRC(862c4713) SHA1(a3707d950f4f5de5208e64207016ef2256eb8c5b) )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, CRC(d86f8cbd) SHA1(8a16130632e20ad3cae8e817da7b661c3ac60f30) )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, CRC(12a73b3f) SHA1(6bb54d4fdf01fd2cdd76a0b47be4d8cae8a1e19b) )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, CRC(bb1a2769) SHA1(9884dceb00e6d88908a1c107b83cc1711b0cf1f7) )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, CRC(39daafd4) SHA1(1e49a273f51cccec3141d540032fd9a3041a3cbd) )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, CRC(94615d2a) SHA1(112a299ff1bb878cf7e24c2ad337440c3df0a6d5) )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, CRC(66c642bd) SHA1(b57f0f8d8e21c9f94ffc0e9f9304b5ab5d4ed3fc) )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, CRC(81d5ab36) SHA1(31103759676a8d1badaf7bde79e7f28d69486106) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, CRC(6e38c6fa) SHA1(c51228d5d063dcf4361c76fa49dbe18db80c50a0) )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, CRC(14fc6cf2) SHA1(080a2d845cb36c637f76d8e062725bd13dd1aed0) )

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "tb23.bin",     0x0000,  0x8000, CRC(eda13c0e) SHA1(806f0819af8b25c2b46de3d1fd95bc9c0e883bd9) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.7j",  0x0000,  0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "mb7114e.1e",   0x0100,  0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

ROM_START( trojanj )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "troj-04.rom",  0x00000, 0x8000, CRC(0b5a7f49) SHA1(eebdfaf905a2b7ac8a0f0f9a7ae4a0daf130a5ea) )
	ROM_LOAD( "troj-06.rom",  0x10000, 0x8000, CRC(dee6ed92) SHA1(80aa16f2ae23581d00f4d58a2075993e7171ed0c) )
	ROM_LOAD( "tb05.bin",     0x18000, 0x8000, CRC(9273b264) SHA1(ab23b16bf53b5baf106ea0cac50754aa967300cf) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "tb02.bin",     0x0000, 0x8000, CRC(21154797) SHA1(e1a3006746cc2d692ecd4369cc0a77c596abd60b) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     /* 64k for ADPCM CPU (CPU not emulated) */
	ROM_LOAD( "tb01.bin",     0x0000, 0x4000, CRC(1c0f91b2) SHA1(163bf6aa1936994659661653eabdc368199b0070) )

	ROM_REGION( 0x04000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "tb03.bin",     0x00000, 0x4000, CRC(581a2b4c) SHA1(705b499da5d01a946f06234a4bab72a291c79034) )     /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE )
	ROM_LOAD( "tb13.bin",     0x00000, 0x8000, CRC(285a052b) SHA1(8ce055c7ac9ce1560552fc7f857f60e7a5af0779) )     /* tiles */
	ROM_LOAD( "tb09.bin",     0x08000, 0x8000, CRC(aeb693f7) SHA1(a811ea67abdd4adfc68224257973802e2a36fc36) )
	ROM_LOAD( "tb12.bin",     0x10000, 0x8000, CRC(dfb0fe5c) SHA1(82542692ab71b9126e6c301ed0803db58734273c) )
	ROM_LOAD( "tb08.bin",     0x18000, 0x8000, CRC(d3a4c9d1) SHA1(3d787f6a4583b80f2d254947890f676cda17b242) )
	ROM_LOAD( "tb11.bin",     0x20000, 0x8000, CRC(00f0f4fd) SHA1(3a862360a26ae1c3a945949d6d47f88aa4b728a4) )
	ROM_LOAD( "tb07.bin",     0x28000, 0x8000, CRC(dff2ee02) SHA1(4877c52f2a0e24a95bcda1d8636ea993c2c3c240) )
	ROM_LOAD( "tb14.bin",     0x30000, 0x8000, CRC(14bfac18) SHA1(84266140e9679912dbbb185fd3b9b497297dcb16) )
	ROM_LOAD( "tb10.bin",     0x38000, 0x8000, CRC(71ba8a6d) SHA1(53ff6850f9f8a19c57c19ef56fd45975f0ec133e) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE )
	ROM_LOAD( "tb18.bin",     0x00000, 0x8000, CRC(862c4713) SHA1(a3707d950f4f5de5208e64207016ef2256eb8c5b) )     /* sprites */
	ROM_LOAD( "tb16.bin",     0x08000, 0x8000, CRC(d86f8cbd) SHA1(8a16130632e20ad3cae8e817da7b661c3ac60f30) )
	ROM_LOAD( "tb17.bin",     0x10000, 0x8000, CRC(12a73b3f) SHA1(6bb54d4fdf01fd2cdd76a0b47be4d8cae8a1e19b) )
	ROM_LOAD( "tb15.bin",     0x18000, 0x8000, CRC(bb1a2769) SHA1(9884dceb00e6d88908a1c107b83cc1711b0cf1f7) )
	ROM_LOAD( "tb22.bin",     0x20000, 0x8000, CRC(39daafd4) SHA1(1e49a273f51cccec3141d540032fd9a3041a3cbd) )
	ROM_LOAD( "tb20.bin",     0x28000, 0x8000, CRC(94615d2a) SHA1(112a299ff1bb878cf7e24c2ad337440c3df0a6d5) )
	ROM_LOAD( "tb21.bin",     0x30000, 0x8000, CRC(66c642bd) SHA1(b57f0f8d8e21c9f94ffc0e9f9304b5ab5d4ed3fc) )
	ROM_LOAD( "tb19.bin",     0x38000, 0x8000, CRC(81d5ab36) SHA1(31103759676a8d1badaf7bde79e7f28d69486106) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "tb25.bin",     0x00000, 0x8000, CRC(6e38c6fa) SHA1(c51228d5d063dcf4361c76fa49dbe18db80c50a0) )     /* Bk Tiles */
	ROM_LOAD( "tb24.bin",     0x08000, 0x8000, CRC(14fc6cf2) SHA1(080a2d845cb36c637f76d8e062725bd13dd1aed0) )

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "tb23.bin",     0x0000,  0x8000, CRC(eda13c0e) SHA1(806f0819af8b25c2b46de3d1fd95bc9c0e883bd9) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbp24s10.7j",  0x0000,  0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "mb7114e.1e",   0x0100,  0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

ROM_START( avengers )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "04.10n",       0x00000, 0x8000, CRC(a94aadcc) SHA1(796545ab5c69c093aaac58f7cff36109dea8df80) )
	ROM_LOAD( "06.13n",       0x10000, 0x8000, CRC(39cd80bd) SHA1(3f8df0096f393efae2d76982640ccc4d33bde8ca) )
	ROM_LOAD( "05.12n",       0x18000, 0x8000, CRC(06b1cec9) SHA1(db5370f3ff1b4456461698af64962cad028561cd) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000, 0x8000, CRC(107a2e17) SHA1(5aae2f4ac9f15ccb4122f3ba9fba588438d62f4f) ) /* ?? */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000, 0x8000, CRC(c1e5d258) SHA1(88ed978e6df72ce22f9371930360aa9cde73abe9) ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, CRC(efb5883e) SHA1(08aebf579f2c5ff472db66597cde1c6871d7d757) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, CRC(9b5ff305) SHA1(8843c757e040b58efd36299eb3c56d9c51362b20) ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, CRC(08323355) SHA1(c5778c6835f2801fba0250cea21796ea201642f7) )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, CRC(6d5261ba) SHA1(667e3b8df871c3052bde7a3c79daa7f70eaa0b8b) ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, CRC(a13d9f54) SHA1(e1bcb6d12cdfc9ad780f131272d12d9af751f429) )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, CRC(a2911d8b) SHA1(f51ef7bb8a275fdd92a9a9ad516218d2f8c3e1fb) ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, CRC(cde78d32) SHA1(8cb69b7a25e935073887628565cb4f9787186ea9) )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, CRC(44ac2671) SHA1(60baa541debd8aa7d32a512906d0d6c6e9955968) ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, CRC(b1a717cb) SHA1(2730764ece0e9231955b9c07de537f1f97729599) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, CRC(3c876a17) SHA1(1f06b695b78a2e1db151f3c5baa1bb17ccef951e) ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, CRC(4b1ff3ac) SHA1(5166f2a2c9ba2483a4e340d756303cba46b7de88) )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, CRC(4eb543ef) SHA1(5dfdd2568a50b179e724643880d79f79d831be19) )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, CRC(8041de7f) SHA1(c301b20edad1981dd20cd6d4f7de703d9dc80b83) )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, CRC(bdaa8b22) SHA1(9a03d20cc7010f9b7c602db86808d54fdd7e228d) ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, CRC(566e3059) SHA1(cf3e5cfcb5ebbff3f9a8e1da9f7242a7a00fee83) )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, CRC(301059aa) SHA1(c529ad83d4e4139ce4d4d912c00aef9ece297706) )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, CRC(a00485ec) SHA1(cc24e7243f55bdfaedeabb7dddf7e1ef32811c45) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE ) /* bg tiles */
	ROM_LOAD( "25.15n",       0x00000, 0x8000, CRC(230d9e30) SHA1(05a20bb32ce1299d7645312624de8a1d074bacee) ) /* planes 0,1 */
	ROM_LOAD( "24.13n",       0x08000, 0x8000, CRC(a6354024) SHA1(ce2aaec8349c08f58cc469514100bcd3a97d24d7) ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, CRC(c0a93ef6) SHA1(2dc9cd4eb142d74aea8d151904cb60a0767c6393) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

ROM_START( avenger2 )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "avg4.bin",     0x00000, 0x8000, CRC(0fea7ac5) SHA1(b978adf5fc90e1e51a995dbec2246d2776264afd) )
	ROM_LOAD( "av_06a.13n",   0x10000, 0x8000, CRC(491a712c) SHA1(67a335b57117ba498d3ae412ac0025477bc79b16) )
	ROM_LOAD( "av_05.12n",    0x18000, 0x8000, CRC(9a214b42) SHA1(e13d47dcf9fa055fef467a10751badffcc3b8734) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000,  0x8000, CRC(107a2e17) SHA1(5aae2f4ac9f15ccb4122f3ba9fba588438d62f4f) ) /* MISSING from this set */

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000,  0x8000, CRC(c1e5d258) SHA1(88ed978e6df72ce22f9371930360aa9cde73abe9) ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, CRC(efb5883e) SHA1(08aebf579f2c5ff472db66597cde1c6871d7d757) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, CRC(9b5ff305) SHA1(8843c757e040b58efd36299eb3c56d9c51362b20) ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, CRC(08323355) SHA1(c5778c6835f2801fba0250cea21796ea201642f7) )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, CRC(6d5261ba) SHA1(667e3b8df871c3052bde7a3c79daa7f70eaa0b8b) ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, CRC(a13d9f54) SHA1(e1bcb6d12cdfc9ad780f131272d12d9af751f429) )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, CRC(a2911d8b) SHA1(f51ef7bb8a275fdd92a9a9ad516218d2f8c3e1fb) ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, CRC(cde78d32) SHA1(8cb69b7a25e935073887628565cb4f9787186ea9) )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, CRC(44ac2671) SHA1(60baa541debd8aa7d32a512906d0d6c6e9955968) ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, CRC(b1a717cb) SHA1(2730764ece0e9231955b9c07de537f1f97729599) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, CRC(3c876a17) SHA1(1f06b695b78a2e1db151f3c5baa1bb17ccef951e) ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, CRC(4b1ff3ac) SHA1(5166f2a2c9ba2483a4e340d756303cba46b7de88) )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, CRC(4eb543ef) SHA1(5dfdd2568a50b179e724643880d79f79d831be19) )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, CRC(8041de7f) SHA1(c301b20edad1981dd20cd6d4f7de703d9dc80b83) )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, CRC(bdaa8b22) SHA1(9a03d20cc7010f9b7c602db86808d54fdd7e228d) ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, CRC(566e3059) SHA1(cf3e5cfcb5ebbff3f9a8e1da9f7242a7a00fee83) )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, CRC(301059aa) SHA1(c529ad83d4e4139ce4d4d912c00aef9ece297706) )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, CRC(a00485ec) SHA1(cc24e7243f55bdfaedeabb7dddf7e1ef32811c45) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE ) /* bg tiles */
	ROM_LOAD( "25.15n",       0x00000, 0x8000, CRC(230d9e30) SHA1(05a20bb32ce1299d7645312624de8a1d074bacee) ) /* planes 0,1 */
	ROM_LOAD( "24.13n",       0x08000, 0x8000, CRC(a6354024) SHA1(ce2aaec8349c08f58cc469514100bcd3a97d24d7) ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, CRC(c0a93ef6) SHA1(2dc9cd4eb142d74aea8d151904cb60a0767c6393) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

ROM_START( buraiken )
	ROM_REGION( 0x20000, REGION_CPU1, 0 )     /* 64k for code + 3*16k for the banked ROMs images */
	ROM_LOAD( "av_04a.10n",   0x00000, 0x8000, CRC(361fc614) SHA1(0ecd9400dfcb03fc94685b33b060a524a5d3c575) )
	ROM_LOAD( "av_06a.13n",   0x10000, 0x8000, CRC(491a712c) SHA1(67a335b57117ba498d3ae412ac0025477bc79b16) )
	ROM_LOAD( "av_05.12n",    0x18000, 0x8000, CRC(9a214b42) SHA1(e13d47dcf9fa055fef467a10751badffcc3b8734) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )     /* 64k for the audio CPU */
	ROM_LOAD( "02.15h",       0x0000,  0x8000, CRC(107a2e17) SHA1(5aae2f4ac9f15ccb4122f3ba9fba588438d62f4f) )

	ROM_REGION( 0x10000, REGION_CPU3, 0 )     /* ADPCM CPU (not emulated) */
	ROM_LOAD( "01.6d",        0x0000,  0x8000, CRC(c1e5d258) SHA1(88ed978e6df72ce22f9371930360aa9cde73abe9) ) /* adpcm player - "Talker" ROM */

	ROM_REGION( 0x08000, REGION_GFX1, ROMREGION_DISPOSE )
	ROM_LOAD( "03.8k",        0x00000, 0x8000, CRC(efb5883e) SHA1(08aebf579f2c5ff472db66597cde1c6871d7d757) )  /* characters */

	ROM_REGION( 0x40000, REGION_GFX2, ROMREGION_DISPOSE ) /* tiles */
	ROM_LOAD( "13.6b",        0x00000, 0x8000, CRC(9b5ff305) SHA1(8843c757e040b58efd36299eb3c56d9c51362b20) ) /* plane 1 */
	ROM_LOAD( "09.6a",        0x08000, 0x8000, CRC(08323355) SHA1(c5778c6835f2801fba0250cea21796ea201642f7) )
	ROM_LOAD( "12.4b",        0x10000, 0x8000, CRC(6d5261ba) SHA1(667e3b8df871c3052bde7a3c79daa7f70eaa0b8b) ) /* plane 2 */
	ROM_LOAD( "08.4a",        0x18000, 0x8000, CRC(a13d9f54) SHA1(e1bcb6d12cdfc9ad780f131272d12d9af751f429) )
	ROM_LOAD( "11.3b",        0x20000, 0x8000, CRC(a2911d8b) SHA1(f51ef7bb8a275fdd92a9a9ad516218d2f8c3e1fb) ) /* plane 3 */
	ROM_LOAD( "07.3a",        0x28000, 0x8000, CRC(cde78d32) SHA1(8cb69b7a25e935073887628565cb4f9787186ea9) )
	ROM_LOAD( "14.8b",        0x30000, 0x8000, CRC(44ac2671) SHA1(60baa541debd8aa7d32a512906d0d6c6e9955968) ) /* plane 4 */
	ROM_LOAD( "10.8a",        0x38000, 0x8000, CRC(b1a717cb) SHA1(2730764ece0e9231955b9c07de537f1f97729599) )

	ROM_REGION( 0x40000, REGION_GFX3, ROMREGION_DISPOSE ) /* sprites */
	ROM_LOAD( "18.7l",        0x00000, 0x8000, CRC(3c876a17) SHA1(1f06b695b78a2e1db151f3c5baa1bb17ccef951e) ) /* planes 0,1 */
	ROM_LOAD( "16.3l",        0x08000, 0x8000, CRC(4b1ff3ac) SHA1(5166f2a2c9ba2483a4e340d756303cba46b7de88) )
	ROM_LOAD( "17.5l",        0x10000, 0x8000, CRC(4eb543ef) SHA1(5dfdd2568a50b179e724643880d79f79d831be19) )
	ROM_LOAD( "15.2l",        0x18000, 0x8000, CRC(8041de7f) SHA1(c301b20edad1981dd20cd6d4f7de703d9dc80b83) )
	ROM_LOAD( "22.7n",        0x20000, 0x8000, CRC(bdaa8b22) SHA1(9a03d20cc7010f9b7c602db86808d54fdd7e228d) ) /* planes 2,3 */
	ROM_LOAD( "20.3n",        0x28000, 0x8000, CRC(566e3059) SHA1(cf3e5cfcb5ebbff3f9a8e1da9f7242a7a00fee83) )
	ROM_LOAD( "21.5n",        0x30000, 0x8000, CRC(301059aa) SHA1(c529ad83d4e4139ce4d4d912c00aef9ece297706) )
	ROM_LOAD( "19.2n",        0x38000, 0x8000, CRC(a00485ec) SHA1(cc24e7243f55bdfaedeabb7dddf7e1ef32811c45) )

	ROM_REGION( 0x10000, REGION_GFX4, ROMREGION_DISPOSE )
	ROM_LOAD( "av_25.15n",    0x00000, 0x8000, CRC(88a505a7) SHA1(ef4371e082b2370fcbfc96bfef5a94910acd9eff) ) /* planes 0,1 */
	ROM_LOAD( "av_24.13n",    0x08000, 0x8000, CRC(1f4463c8) SHA1(04cdb0187dcbdd4f5f53e60c856d4925ade8d7df) ) /* planes 2,3 */

	ROM_REGION( 0x08000, REGION_GFX5, 0 )
	ROM_LOAD( "23.9n",        0x0000,  0x8000, CRC(c0a93ef6) SHA1(2dc9cd4eb142d74aea8d151904cb60a0767c6393) )  /* Tile Map */

	ROM_REGION( 0x0200, REGION_PROMS, 0 )
	ROM_LOAD( "tbb_2bpr.7j",  0x0000,  0x0100, CRC(d96bcc98) SHA1(99e69a624d5586e5eedacd2083fa68b36e7b5e40) )	/* timing (not used) */
	ROM_LOAD( "tbb_1bpr.1e",  0x0100,  0x0100, CRC(5052fa9d) SHA1(8cd240f4795a7ae76499573c09069dba37182be2) )	/* priority (not used) */
ROM_END

GAME( 1985, sectionz, 0,        lwings,   sectionz, 0, ROT0,  "Capcom", "Section Z (set 1)", 0 )
GAME( 1985, sctionza, sectionz, lwings,   sectionz, 0, ROT0,  "Capcom", "Section Z (set 2)", 0 )
GAME( 1986, lwings,   0,        lwings,   lwings,   0, ROT90, "Capcom", "Legendary Wings (US set 1)", 0 )
GAME( 1986, lwings2,  lwings,   lwings,   lwings,   0, ROT90, "Capcom", "Legendary Wings (US set 2)", 0 )
GAME( 1986, lwingsjp, lwings,   lwings,   lwings,   0, ROT90, "Capcom", "Ares no Tsubasa (Japan)", 0 )
GAME( 1986, trojan,   0,        trojan,   trojanls, 0, ROT0,  "Capcom", "Trojan (US)", 0 )
GAME( 1986, trojanr,  trojan,   trojan,   trojan,   0, ROT0,  "Capcom (Romstar license)", "Trojan (Romstar)", 0 )
GAME( 1986, trojanj,  trojan,   trojan,   trojan,   0, ROT0,  "Capcom", "Tatakai no Banka (Japan)", 0 )
GAME( 1987, avengers, 0,        avengers, avengers, 0, ROT90, "Capcom", "Avengers (US set 1)", 0 )
GAME( 1987, avenger2, avengers, avengers, avengers, 0, ROT90, "Capcom", "Avengers (US set 2)", 0 )
GAME( 1987, buraiken, avengers, avengers, avengers, 0, ROT90, "Capcom", "Hissatsu Buraiken (Japan)", 0 )
