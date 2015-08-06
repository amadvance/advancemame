/*
Pinball Champ '95 / Witch

witch   : Witch
pbchmp95: Pinball Champ '95. Seems to be a simple mod with the following differences:
                        -The title screen is changed
                        -The sample saying "witch" is not played (obviously)
                        -Different configuration values (time limit, etc)
                        -Auto-initialization on NVRAM error(?)
                        -Stars keep falling at the title screen


rom will be banked


This is so far what could be reverse-engineered from the code.
BEWARE : these are only suppositions, not facts.

Featured hardware

    2xZ80 ; frequency unknown (CPU2 used mainly for sound effects)
    2xYM2203 (or compatible?) ; frequency unknown (music + sound effects + video scrolling access)
    1xES8712 ; frequency unknown (samples)


GFX

    2 gfx layers accessed by cpu1 (& cpu2 for scrolling) + 1 sprite layer

    In (assumed) order of priority :
        - Top layer @0xc000-0xc3ff(vram) + 0xc400-0xc7ff(cram) apparently not scrollable (gfx0)
            Uses tiles from REGION_GFX2

            tileno =    vram | ((cram & 0xe0) << 3)
            color  =    cram & 0x0f

        - Sprites @0xd000-0xd7ff + 0xd800-0xdfff
                One sprite every 0x20 bytes
                0x40 sprites
                Tiles are from REGION_GFX2
                Seems to be only 16x16 sprites (2x2 tiles)
                xflip and yflip available

                tileno                = sprite_ram[i*0x20] << 2 | (( sprite_ram[i*0x20+0x800] & 0x07 ) << 10 );
                sx                    = sprite_ram[i*0x20+1];
                sy              = sprite_ram[i*0x20+2];
                flags+colors    = sprite_ram[i*0x20+3];

        - Background layer @0xc800-0xcbff(vram) + 0xcc00-0xcfff(cram) (gfx1)
                Uses tiles from REGION_GFX1
                    tileno = vram | ((cram & 0xf0) << 4),
                    color  = cram & 0x0f

                The background is scrolled via 2 registers accessed through one of the ym2203, port A&B
                The scrolling is set by CPU2 in its interrupt handler.
                CPU1 doesn't seem to offset vram accesses for the scrolling, so it's assumed to be done
                in hardware.
                This layer looks misaligned with the sprites, but the top layer is not. This is perhaps
                due to the weird handling of the scrolling. For now we just offset it by 7 pixels.


Palette

    3*0x100 palette banks @ 0xe000-0xe300 & 0xe800-0xe8ff (xBBBBBGGGGGRRRRR_split format?)
    Bank 1 is used for gfx0 (top layer) and sprites
    Bank 2 is for gfx1 (background layer)

    Could not find any use of bank 0 ; I'm probably missing a flag somewhere.


Sound

    Mainly handled by CPU2

    2xYM2203

    0x8000-0x8001 : Mainly used for sound effects & to read dipswitches
    0x8008-0x8009 : Music & scrolling

    1xES8712

    Mapped @0x8010-0x8016
    Had to patch es8712.c to start playing on 0x8016 write and to prevent continuous looping.
    There's a test on bit1 at offset 0 (0x8010), so this may be a "read status" kind of port.
    For now reading at 8010 always reports ready.


Ports

    0xA000-0xA00f : Various ports yet to figure out...

      - 0xA000 : unknown ; seems muxed with a002
      - 0xA002 : banking?
                         bank number = bits 7&6 (swapped?)
                 mapped 0x0800-0x7fff?
                 0x0000-0x07ff ignored?
                 see code @ 61d
                 lower bits seems to mux port A000 reads
      - 0xA003 : ?
      - 0xA004 : dipswitches
      - 0xA005 : dipswitches
      - 0xA006 : bit1(out) = release coin?
      - 0xA007 : ?
      - 0xA008 : cpu1 sets it to 0x80 on reset ; cleared in interrupt handler
                             cpu2 sets it to 0x40 on reset ; cleared in interrupt handler
      - 0xA00C : bit0 = payout related?
                         bit3 = reset? (see cpu2 code @14C)
      - 0xA00E : ?


Memory

    RAM:
        Considering that
            -CPU1 busy loops on fd00 and that CPU2 modifies fd00 once it is initialized
            -CPU1 writes to fd01-fd05 and CPU2 reads there and plays sounds accordingly
            -CPU1 writes to f208-f209 and CPU2 forwards this to the scrolling registers
        we can assume that the 0xf2xx and 0fdxx segments are shared.

        From the fact that
            -CPU1's SP is set to 0xf100 on reset
            -CPU2's SP is set to 0xf080 on reset
        we may suppose that this memory range (0xf000-0xf0ff) is shared too.

        Moreover, range 0xf100-0xf17f is checked after reset without prior initialization and
        is being reset ONLY by changing a particular port bit whose modification ends up with
        a soft reboot. This looks like a good candidate for an NVRAM segment.
        Whether CPU2 can access the NVRAM or not is still a mystery considering that it never
        attempts to do so.

        From these we consider that the 0xfxxx segment, except for the NVRAM range, is shared
        between the two CPUs.

  CPU1:
      The ROM segment (0x0000-0x7fff) is banked, but the 0x0000-0x07ff region does not look
      like being affected (the SEGA Master System did something similar IIRC). A particular
      bank is selected by changing the two most significant bits of port 0xa002 (swapped?).

    CPU2:
        Doesn't seem to be banking going on. However there's a strange piece of code @0x021a:

        -----------------------------------------
        ROM:021A                 xor     a
        ROM:021B                 ld      (0FD11h), a
        ROM:021E                 ld      a, 7
        ROM:0220                 ld      (byte_700F), a
        ROM:0223                 halt
        ROM:0224                 ld      a, 6
        ROM:0226                 ld      (byte_700F), a
        ROM:0229                 halt
        ROM:022A                 ld      a, 0
        ROM:022C                 ld      (byte_700F), a
        ROM:022F                 ld      b, 3Ch
        ROM:0231 _1SEC_WAIT:
        ROM:0231                 push    bc
        ROM:0232                 halt
        ROM:0233                 pop     bc
        ROM:0234                 djnz    _1SEC_WAIT
        ROM:0236                 ld      a, 1
        ROM:0238                 ld      (byte_700D), a
        ROM:023B loc_23B:
        ROM:023B                 halt
        ROM:023C                 ld      a, (byte_700D)
        ROM:023F                 bit     1, a
        ROM:0241                 jr      nz, loc_23B
        ROM:0243                 ld      a, (byte_7000)
        ROM:0246                 ld      (0FD12h), a
        ROM:0249                 ld      a, (byte_7001)
        ROM:024C                 ld      (0FD13h), a
        ROM:024F                 ld      a, (byte_7002)
        ROM:0252                 ld      (0FD14h), a
        ROM:0255                 ld      a, (byte_7003)
        ROM:0258                 ld      (0FD15h), a
        ROM:025B                 ld      a, (byte_7004)
        ROM:025E                 ld      (0FD16h), a
        ROM:0261                 ld      ix, 0FD12h
        ROM:0265                 ld      a, (ix+0)
        ROM:0268                 cp      (ix+1)
        ROM:026B                 jr      nz, loc_27D
        ROM:026D                 cp      (ix+2)
        ROM:0270                 jr      nz, loc_27D
        ROM:0272                 cp      (ix+3)
        ROM:0275                 jr      nz, loc_27D
        ROM:0277                 cp      (ix+4)
        ROM:027A                 jr      nz, loc_27D
        ROM:027C                 ret
        ROM:027D loc_27D:
        ROM:027D                 ld      a, 1
        ROM:027F                 ld      (0FD11h), a
        ROM:0282                 ret
        -----------------------------------------

        Considering that 0x700x is ROM(?), I'm clueless about what's going on here.
        It looks like some initialization routine, but the fd12-fd16 values are not
        used afterward(?). For now, I just put a hack to keep CPU2 happy.


Interesting memory locations

        +f180-f183 : dipswitches stored here (see code@2746). Beware, all values are "CPL"ed!
            *f180   : kkkbbppp / A005
                             ppp  = PAY OUT | 60 ; 65 ; 70 ; 75 ; 80 ; 85 ; 90 ; 95
                             bb   = MAX BET | 20 ; 30 ; 40 ; 60
                             kkk  = KEY IN  | 1-10 ; 1-20 ; 1-40 ; 1-50 ; 1-100 ; 1-200 ; 1-250 ; 1-500

            *f181   : ccccxxxd / A004
                             d    = DOUBLE UP | ON ; OFF
                             cccc = COIN IN1 | 1-1 ; 1-2 ; 1-3 ; 1-4 ; 1-5 ; 1-6 ; 1-7 ; 1-8 ; 1-9 ; 1-10 ; 1-15 ; 1-20 ; 1-25 ; 1-30 ; 1-40 ; 1-50

            *f182   : sttpcccc / PortA
                             cccc = COIN IN2 | 1-1 ; 1-2 ; 1-3 ; 1-4 ; 1-5 ; 1-6 ; 1-7 ; 1-8 ; 1-9 ; 1-10 ; 2-1 ; 3-1 ; 4-1 ; 5-1 ; 6-1 ; 10-1
                             p    = PAYOUT SWITCH | ON ; OFF
                             tt   = TIME | 40 ; 45 ; 50 ; 55
                             s    = DEMO SOUND | ON ; OFF
            *f183 : xxxxhllb / PortB
                             b    = AUTO BET | ON ; OFF
                             ll   = GAME LIMIT | 500 ; 1000 ; 5000 ; 990000
                             h    = HOPPER ACTIVE | LOW ; HIGH


        +f15c-f15e : MAX WIN
        +f161      : JACK POT
        +f166-f168 : DOUBLE UP
        +f16b-f16d : MAX D-UP WIN

        +f107-f109 : TOTAL IN
        +f10c-f10e : TOTAL OUT

        +f192-f194 : credits (bcd)

        +fd00 = cpu2 ready
        +f211 = input port cache?

    CPU2 Commands :
        -0xfd01 start music
        -0xfd02 play sound effect
        -0xfd03 play sample on the ES8712
        -0xfd04 ?
        -0xfd05 ?


TODO :

    -Figure out the input ports
    -Figure out the ports for the "PayOut" stuff (a006/a00c?)
    -Find out why the ball freezes ingame

*/

#include "driver.h"
#include "sound/es8712.h"
#include "sound/2203intf.h"

#define UNBANKED_SIZE 0x800

static tilemap *gfx0_tilemap;
static tilemap *gfx1_tilemap;

static UINT8 *gfx0_cram;
static UINT8 *gfx0_vram;

static UINT8 *gfx1_cram;
static UINT8 *gfx1_vram;

static UINT8 *sprite_ram;

static int scrollx=0;
static int scrolly=0;

static UINT8 reg_a002=0;
static int bank=-1;

static void get_gfx0_tile_info(int tile_index)
{
	int code  = gfx0_vram[tile_index];
	int color = gfx0_cram[tile_index];

	SET_TILE_INFO(
			1,
			code | ((color & 0xe0) << 3),//tiles beyond 0x7ff only for sprites?
			(color>>0) & 0x0f,
			0)
}

static void get_gfx1_tile_info(int tile_index)
{
	int code  = gfx1_vram[tile_index];
	int color = gfx1_cram[tile_index];

	SET_TILE_INFO(
			0,
			code | ((color & 0xf0) << 4),
			(color>>0) & 0x0f,
			0)
}

static WRITE8_HANDLER( gfx0_vram_w )
{
	if(gfx0_vram[offset] != data)
	{
		gfx0_vram[offset] = data;
		tilemap_mark_tile_dirty(gfx0_tilemap,offset);
	}
}

static WRITE8_HANDLER( gfx0_cram_w )
{
	if(gfx0_cram[offset] != data)
	{
		gfx0_cram[offset] = data;
		tilemap_mark_tile_dirty(gfx0_tilemap,offset);
	}
}
static READ8_HANDLER( gfx0_vram_r )
{
	return gfx0_vram[offset];
}

static READ8_HANDLER( gfx0_cram_r )
{
	return gfx0_cram[offset];
}

#define FIX_OFFSET() do { offset=(offset+scrolly/8*32)&0x3ff; } while(0)
//#define FIX_OFFSET() do { offset=(offset+((signed char)scrollx+7)/8+(scrolly)/8*32)&0x3ff; } while(0)
//#define FIX_OFFSET() do { offset=(offset+(scrollx+7)/8+(scrolly+7)/8*32)&0x3ff; }while(0)
//#define FIX_OFFSET() do {offset=((offset+(scrollx)/8)&0x01f)|(((offset&0x3e0)+(scrolly)/8*32)&0x3e0);} while(0)
//#define FIX_OFFSET()

static WRITE8_HANDLER( gfx1_vram_w )
{
	FIX_OFFSET();
	if(gfx1_vram[offset] != data)
	{
		gfx1_vram[offset] = data;
		tilemap_mark_tile_dirty(gfx1_tilemap,offset);
	}
}

static WRITE8_HANDLER( gfx1_cram_w )
{
	FIX_OFFSET();
	if(gfx1_cram[offset] != data)
	{
		gfx1_cram[offset] = data;
		tilemap_mark_tile_dirty(gfx1_tilemap,offset);
	}
}
static READ8_HANDLER( gfx1_vram_r )
{
	FIX_OFFSET();
	return gfx1_vram[offset];
}

static READ8_HANDLER( gfx1_cram_r )
{
	FIX_OFFSET();
	return gfx1_cram[offset];
}

static READ8_HANDLER(read_a00x)
{
	//printf("0xA00%X read at 0x%04X\n",offset,activecpu_get_pc());

	switch(offset)
	{
		case 0x02:
	    return reg_a002;
		case 0x04:
			return readinputportbytag("A004");
	  case 0x05:
	    return readinputportbytag("A005");
		case 0x0c:
			return input_port_0_r(0); // stats / reset
		case 0x0e:
			return readinputportbytag("A00E");// coin/reset
/*      case 0x01:
            return 0;*/
	}


  if(offset==0x00) //muxed with A002?
  {
    switch(reg_a002&0x3f) {
      case 0x3b:
        return input_port_2_r(0);//bet10 / pay out
      case 0x3e:
        return input_port_3_r(0);//TODO : trace f564
      case 0x3d:
      	return input_port_4_r(0);
      default:
        logerror("A000 read with mux=0x%02x\n",reg_a002&0x3f);
    }
  }

  return 0xff;
}

static WRITE8_HANDLER(write_a00x)
{
  switch(offset)
  {
		case 0x02: //A002 bit 7&6 = bank ????
		{
			int newbank;

	    /*if(cpu_getactivecpu()!=0)
          return;*/

		  reg_a002=data;

		  newbank=(data>>6)&3;
		  newbank=((newbank<<1)|(newbank>>1))&0x3;

	    if(newbank!=bank)
	    {
	      UINT8 *ROM = memory_region(REGION_CPU1);
	      bank=newbank;
		    ROM = &ROM[0x8000 * newbank + UNBANKED_SIZE];
		    memory_set_bankptr(1,ROM);

		   // printf("Bank switched @ 0x%04X: bank #%d data=0x%02X\n",activecpu_get_pc(),newbank,data);
	     }
	     return;
	     break;
		}
		case 0x06:
/*
            if(data&0x02) {
                printf("COIN LOCK\n");
            } else {
                printf("COIN RELEASE\n");
            }
*/
			//printf("0xA006 Written@ 0x%04X: value=0x%02X\n",activecpu_get_pc(),data);
			break;

    case 0x08: //A008
//      if(cpu_getactivecpu()==0) cpunum_set_input_line(1,0,HOLD_LINE);
    	cpunum_set_input_line(cpu_getactivecpu(),0,CLEAR_LINE);
    /*
      switch(data)
      {
        case 0x00:
          cpunum_set_input_line(cpu_getactivecpu(),0,CLEAR_LINE);
//          cpunum_set_input_line(0,0,CLEAR_LINE);
//          cpunum_set_input_line(1,0,CLEAR_LINE);
          break;
        case 0x80:
        //  cpunum_set_input_line(1,0,ASSERT_LINE);
          break;
        case 0x40:
         // cpunum_set_input_line(0,0,ASSERT_LINE);
          break;
      }*/
      break;

    default:
//      printf("cpu #%d (PC=%08X): Write @ 0xa0%02x = 0x%02x\n",cpu_getactivecpu(), activecpu_get_pc(),offset,data);
			break;
  }
  //printf("cpu #%d (PC=%08X): Write @ 0xa0%02x = 0x%02x\n",cpu_getactivecpu(), activecpu_get_pc(),offset,data);
}

static READ8_HANDLER(read_700x)
{
	switch(offset)
	{
		case 0xd://0x700d : CPU2 waiting loop on bit1 @ 0x023B
			return 0xfd;
	}
	return offset;//needs differents values
}

/*
 * Status from ES8712?
 * BIT1 is zero when no sample is playing?
 */
static READ8_HANDLER(read_8010) {	return 0x00; }

static WRITE8_HANDLER(xscroll_w)
{
	if(scrollx!=data)
	{
		//printf("set scrollx to 0x%02X\n",data);
	}
	scrollx=data;
}
static WRITE8_HANDLER(yscroll_w)
{
	if(scrolly!=data)
	{
		//printf("set scrolly to 0x%02X\n",data);
	}
	scrolly=data;
}

static READ8_HANDLER(portA_r) {	return readinputportbytag("YM_PortA"); }
static READ8_HANDLER(portB_r) {	return readinputportbytag("YM_PortB");}

static struct YM2203interface ym2203_interface_0 =
{
	portA_r,
	portB_r,
	NULL,
	NULL
};

static struct YM2203interface ym2203_interface_1 =
{
	NULL,
	NULL,
	xscroll_w,
	yscroll_w
};

static ADDRESS_MAP_START( readmem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, UNBANKED_SIZE-1) AM_READ(MRA8_ROM)
	AM_RANGE(UNBANKED_SIZE, 0x7fff) AM_READ(MRA8_BANK1)

	AM_RANGE(0x8000, 0x8000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(YM2203_read_port_0_r)
	AM_RANGE(0x8008, 0x8008) AM_READ(YM2203_status_port_1_r)
	AM_RANGE(0x8001, 0x8001) AM_READ(YM2203_read_port_1_r)

	AM_RANGE(0xa000, 0xa00f) AM_READ(read_a00x)

	AM_RANGE(0xc000, 0xc3ff) AM_READ(gfx0_vram_r)
	AM_RANGE(0xc400, 0xc7ff) AM_READ(gfx0_cram_r)

	AM_RANGE(0xc800, 0xcbff) AM_READ(gfx1_vram_r)
	AM_RANGE(0xcc00, 0xcfff) AM_READ(gfx1_cram_r)

	AM_RANGE(0xd000, 0xdfff) AM_READ(MRA8_RAM) AM_BASE(&sprite_ram)

	AM_RANGE(0xe000, 0xe2ff) AM_READ(paletteram_r)   AM_BASE(&paletteram)

	AM_RANGE(0xe300, 0xe7ff) AM_READ(MRA8_NOP)

	AM_RANGE(0xe800, 0xeaff) AM_READ(paletteram_2_r) AM_BASE(&paletteram_2)

	AM_RANGE(0xeb00, 0xefff) AM_READ(MRA8_NOP)

	AM_RANGE(0xf000, 0xf0ff) AM_READ(MRA8_RAM) AM_SHARE(1)
	AM_RANGE(0xf100, 0xf17f) AM_READ(MRA8_RAM) /*AM_SHARE(2)*/ //AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf180, 0xffff) AM_READ(MRA8_RAM) AM_SHARE(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0x8000, 0x8000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8008, 0x8008) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x8009, 0x8009) AM_WRITE(YM2203_write_port_1_w)

	AM_RANGE(0xa000, 0xa00f) AM_WRITE(write_a00x)

	AM_RANGE(0xc000, 0xc3ff) AM_WRITE(gfx0_vram_w) AM_BASE(&gfx0_vram)
	AM_RANGE(0xc400, 0xc7ff) AM_WRITE(gfx0_cram_w) AM_BASE(&gfx0_cram)

	AM_RANGE(0xc800, 0xcbff) AM_WRITE(gfx1_vram_w) AM_BASE(&gfx1_vram)
	AM_RANGE(0xcc00, 0xcfff) AM_WRITE(gfx1_cram_w) AM_BASE(&gfx1_cram)

	AM_RANGE(0xd000, 0xdfff) AM_WRITE(MWA8_RAM) AM_BASE(&sprite_ram)

	AM_RANGE(0xe000, 0xe2ff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_split1_w) AM_BASE(&paletteram)

	AM_RANGE(0xe300, 0xe7ff) AM_WRITE(MWA8_NOP)

	AM_RANGE(0xe800, 0xeaff) AM_WRITE(paletteram_xBBBBBGGGGGRRRRR_split2_w) AM_BASE(&paletteram_2)

	AM_RANGE(0xeb00, 0xefff) AM_WRITE(MWA8_NOP)

	AM_RANGE(0xf000, 0xf0ff) AM_WRITE(MWA8_RAM) AM_SHARE(1)
	AM_RANGE(0xf100, 0xf17f) AM_WRITE(MWA8_RAM) /*AM_SHARE(2)*/ AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf180, 0xffff) AM_WRITE(MWA8_RAM) AM_SHARE(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_readmem, ADDRESS_SPACE_PROGRAM, 8 )
//  AM_RANGE(0x0000, 0x7fff) AM_READ(MRA8_ROM)

	AM_RANGE(0x0000, 0x6fff) AM_READ(MRA8_ROM)
	AM_RANGE(0x7000, 0x700f) AM_READ(read_700x)

	AM_RANGE(0x8000, 0x8000) AM_READ(YM2203_status_port_0_r)
	AM_RANGE(0x8008, 0x8008) AM_READ(YM2203_status_port_1_r)

	AM_RANGE(0x8010, 0x8010) AM_READ(read_8010)

	AM_RANGE(0xa000, 0xa00f) AM_READ(read_a00x)

  AM_RANGE(0xf000, 0xf0ff) AM_READ(MRA8_RAM) AM_SHARE(1)
	//AM_RANGE(0xf100, 0xf17f) AM_READ(MRA8_RAM) AM_SHARE(2) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf180, 0xffff) AM_READ(MRA8_RAM) AM_SHARE(2)

ADDRESS_MAP_END

static ADDRESS_MAP_START( cpu2_writemem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_WRITE(MWA8_ROM)

	AM_RANGE(0x8000, 0x8000) AM_WRITE(YM2203_control_port_0_w)
	AM_RANGE(0x8001, 0x8001) AM_WRITE(YM2203_write_port_0_w)
	AM_RANGE(0x8008, 0x8008) AM_WRITE(YM2203_control_port_1_w)
	AM_RANGE(0x8009, 0x8009) AM_WRITE(YM2203_write_port_1_w)

	AM_RANGE(0x8010, 0x8016) AM_WRITE(ES8712_data_0_w)
	AM_RANGE(0xa000, 0xa00f) AM_WRITE(write_a00x)

	AM_RANGE(0xf000, 0xf0ff) AM_WRITE(MWA8_RAM) AM_SHARE(1)
	//AM_RANGE(0xf100, 0xf17f) AM_WRITE(MWA8_RAM) AM_SHARE(2) AM_BASE(&generic_nvram) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xf180, 0xffff) AM_WRITE(MWA8_RAM) AM_SHARE(2)
ADDRESS_MAP_END


INPUT_PORTS_START( witch )
	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, "A0" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPNAME( 0x02, 0x02, "A1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPNAME( 0x04, 0x04, "A2" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPNAME( 0x08, 0x08, "RESET NVRAM" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x10, "A4" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPNAME( 0x20, 0x20, "STATS" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPNAME( 0x40, 0x40, "A6" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x80, "A7" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )

	PORT_START_TAG("A00E")	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, "KEY IN" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPNAME( 0x02, 0x02, "COIN IN1" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPNAME( 0x04, 0x04, "RESET (ONLY IN GAME)" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPNAME( 0x08, 0x08, "COIN IN2" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x10, "E4" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPNAME( 0x20, 0x20, "E5" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPNAME( 0x40, 0x40, "E6" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x80, "E7" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )


	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x01, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "41" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPNAME( 0x04, 0x04, "42" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPNAME( 0x08, 0x08, "43" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x10, "44" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPNAME( 0x20, 0x20, "45" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPNAME( 0x40, 0x40, "46" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x80, "47" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )


	PORT_START	/* DSW */
	PORT_DIPNAME( 0x01, 0x01, "50" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x01, "1" )
	PORT_DIPNAME( 0x02, 0x02, "51" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x02, "1" )
	PORT_DIPNAME( 0x04, 0x04, "52" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x04, "1" )
	PORT_DIPNAME( 0x08, 0x08, "53" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x08, "1" )
	PORT_DIPNAME( 0x10, 0x10, "54" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x10, "1" )
	PORT_DIPNAME( 0x20, 0x20, "START" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x20, "1" )
	PORT_DIPNAME( 0x40, 0x40, "BET10" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x40, "1" )
	PORT_DIPNAME( 0x80, 0x80, "57" )
	PORT_DIPSETTING(    0x00, "0" )
	PORT_DIPSETTING(    0x80, "1" )

/*
F180 kkkbbppp ; Read on port 0xA005
 ppp  = PAY OUT | 60 ; 65 ; 70 ; 75 ; 80 ; 85 ; 90 ; 95
 bb   = MAX BET | 20 ; 30 ; 40 ; 60
 kkk  = KEY IN  | 1-10 ; 1-20 ; 1-40 ; 1-50 ; 1-100 ; 1-200 ; 1-250 ; 1-500
*/
	PORT_START_TAG("A005")	/* DSW */
		PORT_DIPNAME( 0x07, 0x07, "PAY OUT" )
			PORT_DIPSETTING(    0x07, "60" )
			PORT_DIPSETTING(    0x06, "65" )
			PORT_DIPSETTING(    0x05, "70" )
			PORT_DIPSETTING(    0x04, "75" )
			PORT_DIPSETTING(    0x03, "80" )
			PORT_DIPSETTING(    0x02, "85" )
			PORT_DIPSETTING(    0x01, "90" )
			PORT_DIPSETTING(    0x00, "95" )
		PORT_DIPNAME( 0x18, 0x00, "MAX BET" )
			PORT_DIPSETTING(    0x18, "20" )
			PORT_DIPSETTING(    0x10, "30" )
			PORT_DIPSETTING(    0x08, "40" )
			PORT_DIPSETTING(    0x00, "60" )
		PORT_DIPNAME( 0xe0, 0xe0, "KEY IN" )
			PORT_DIPSETTING(    0xE0, "1-10"  )
			PORT_DIPSETTING(    0xC0, "1-20"  )
			PORT_DIPSETTING(    0xA0, "1-40"  )
			PORT_DIPSETTING(    0x80, "1-50"  )
			PORT_DIPSETTING(    0x60, "1-100" )
			PORT_DIPSETTING(    0x40, "1-200" )
			PORT_DIPSETTING(    0x20, "1-250" )
			PORT_DIPSETTING(    0x00, "1-500" )

/*
*f181   : ccccxxxd ; Read on port 0xA004
 d    = DOUBLE UP | ON ; OFF
 cccc = COIN IN1 | 1-1 ; 1-2 ; 1-3 ; 1-4 ; 1-5 ; 1-6 ; 1-7 ; 1-8 ; 1-9 ; 1-10 ; 1-15 ; 1-20 ; 1-25 ; 1-30 ; 1-40 ; 1-50
*/
	PORT_START_TAG("A004")	/* DSW */
		PORT_DIPNAME( 0x01, 0x00, "DOUBLE UP" )
			PORT_DIPSETTING(    0x01, DEF_STR( Off  ) )
			PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0xf0, 0xf0, "COIN IN1" )
			PORT_DIPSETTING(    0xf0, "1-1" )
			PORT_DIPSETTING(    0xe0, "1-2" )
			PORT_DIPSETTING(    0xd0, "1-3" )
			PORT_DIPSETTING(    0xc0, "1-4" )
			PORT_DIPSETTING(    0xb0, "1-5" )
			PORT_DIPSETTING(    0xa0, "1-6" )
			PORT_DIPSETTING(    0x90, "1-7" )
			PORT_DIPSETTING(    0x80, "1-8" )
			PORT_DIPSETTING(    0x70, "1-9" )
			PORT_DIPSETTING(    0x60, "1-10" )
			PORT_DIPSETTING(    0x50, "1-15" )
			PORT_DIPSETTING(    0x40, "1-20" )
			PORT_DIPSETTING(    0x30, "1-25" )
			PORT_DIPSETTING(    0x20, "1-30" )
			PORT_DIPSETTING(    0x10, "1-40" )
			PORT_DIPSETTING(    0x00, "1-50" )

/*
*f182   : sttpcccc ; Read on Port A of YM2203 @ 0x8001
 cccc = COIN IN2 | 1-1 ; 1-2 ; 1-3 ; 1-4 ; 1-5 ; 1-6 ; 1-7 ; 1-8 ; 1-9 ; 1-10 ; 2-1 ; 3-1 ; 4-1 ; 5-1 ; 6-1 ; 10-1
 p    = PAYOUT SWITCH | ON ; OFF
 tt   = TIME | 40 ; 45 ; 50 ; 55
 s    = DEMO SOUND | ON ; OFF
*/
	PORT_START_TAG("YM_PortA")	/* DSW */
		PORT_DIPNAME( 0x0f, 0x0f, "COIN IN2" )
			PORT_DIPSETTING(    0x0f, "1-1" )
			PORT_DIPSETTING(    0x0e, "1-2" )
			PORT_DIPSETTING(    0x0d, "1-3" )
			PORT_DIPSETTING(    0x0c, "1-4" )
			PORT_DIPSETTING(    0x0b, "1-5" )
			PORT_DIPSETTING(    0x0a, "1-6" )
			PORT_DIPSETTING(    0x09, "1-7" )
			PORT_DIPSETTING(    0x08, "1-8" )
			PORT_DIPSETTING(    0x07, "1-9" )
			PORT_DIPSETTING(    0x06, "1-10" )
			PORT_DIPSETTING(    0x05, "2-1" )
			PORT_DIPSETTING(    0x04, "3-1" )
			PORT_DIPSETTING(    0x03, "4-1" )
			PORT_DIPSETTING(    0x02, "5-1" )
			PORT_DIPSETTING(    0x01, "6-1" )
			PORT_DIPSETTING(    0x00, "10-1" )
		PORT_DIPNAME( 0x10, 0x00, "PAYOUT SWITCH" )
			PORT_DIPSETTING(    0x10, DEF_STR( Off  ) )
			PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x60, 0x00, "TIME" )
			PORT_DIPSETTING(    0x60, "40" )
			PORT_DIPSETTING(    0x40, "45" )
			PORT_DIPSETTING(    0x20, "50" )
			PORT_DIPSETTING(    0x00, "55" )
		PORT_DIPNAME( 0x80, 0x00, "DEMO SOUND" )
			PORT_DIPSETTING(    0x80, DEF_STR( Off  ) )
			PORT_DIPSETTING(    0x00, DEF_STR( On ) )

/*
*f183 : xxxxhllb ; Read on Port B of YM2203 @ 0x8001
 b    = AUTO BET | ON ; OFF
 ll   = GAME LIMIT | 500 ; 1000 ; 5000 ; 990000
 h    = HOPPER ACTIVE | LOW ; HIGH
*/
	PORT_START_TAG("YM_PortB")	/* DSW */
		PORT_DIPNAME( 0x01, 0x01, "AUTO BET" )
			PORT_DIPSETTING(    0x01, DEF_STR( Off  ) )
			PORT_DIPSETTING(    0x00, DEF_STR( On ) )
		PORT_DIPNAME( 0x06, 0x06, "GAME LIMIT" )
			PORT_DIPSETTING(    0x06, "500" )
			PORT_DIPSETTING(    0x04, "1000" )
			PORT_DIPSETTING(    0x02, "5000" )
			PORT_DIPSETTING(    0x00, "990000" )
		PORT_DIPNAME( 0x08, 0x08, "HOPPER" )
			PORT_DIPSETTING(    0x08, DEF_STR(Low) )
			PORT_DIPSETTING(    0x00, DEF_STR(High) )

INPUT_PORTS_END

struct ES8712interface es8712_interface =
{
	REGION_SOUND1
};


static const gfx_layout tiles8x8_layout =
{
	8,8,
	RGN_FRAC(1,2),
	4,
	{ 0, 1, 2,3 },
	{ 0, 4, RGN_FRAC(1,2)+0, RGN_FRAC(1,2)+4, 8, 12, RGN_FRAC(1,2)+8, RGN_FRAC(1,2)+12},
	{ 0*16, 1*16, 2*16, 3*16, 4*16, 5*16, 6*16, 7*16 },
	16*8
};

static const gfx_decode gfxdecodeinfo[] =
{
	{ REGION_GFX1, 0, &tiles8x8_layout, 0, 16 },
	{ REGION_GFX2, 0, &tiles8x8_layout, 0, 16 },
	{ -1 }
};

VIDEO_START(witch)
{
	gfx0_tilemap = tilemap_create(get_gfx0_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	gfx1_tilemap = tilemap_create(get_gfx1_tile_info,tilemap_scan_rows,TILEMAP_OPAQUE,8,8,32,32);

	if(!gfx0_tilemap||!gfx1_tilemap)
		return 1;

	tilemap_set_transparent_pen(gfx0_tilemap,0);
  tilemap_set_palette_offset(gfx0_tilemap,0x100);
  tilemap_set_palette_offset(gfx1_tilemap,0x200);
//What about 0x00-0xff?

	return 0;
}

static void draw_sprites(mame_bitmap *bitmap)
{
	int i,sx,sy,tileno,flags,color;
	int flipx=0;
	int flipy=0;

	for(i=0;i<0x800;i+=0x20) {
		int gfx=1;

		sx     = sprite_ram[i+1];
		if(sx!=0xF8) {
			tileno = sprite_ram[i] << 2 | (( sprite_ram[i+0x800] & 0x07 ) << 10 );

			sy     = sprite_ram[i+2];
			flags  = sprite_ram[i+3];

			flipx  = (flags & 0x10 ) ? 1 : 0;
			flipy  = (flags & 0x20 ) ? 1 : 0;

			color  =  flags & 0x0f;

			drawgfx(bitmap,Machine->gfx[gfx],
				tileno, color,
				flipx, flipy,
				sx+8*flipx,sy+8*flipy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);

			if(1) {//only 4x4 sprites?
				drawgfx(bitmap,Machine->gfx[gfx],
				tileno+1, color,
				flipx, flipy,
				sx+8-8*flipx,sy+8*flipy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);

				drawgfx(bitmap,Machine->gfx[gfx],
				tileno+2, color,
				flipx, flipy,
				sx+8*flipx,sy+8-8*flipy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);

				drawgfx(bitmap,Machine->gfx[gfx],
				tileno+3, color,
				flipx, flipy,
				sx+8-8*flipx,sy+8-8*flipy,
				&Machine->visible_area,TRANSPARENCY_PEN,0);
			}
		}
	}

}

VIDEO_UPDATE(witch)
{
	tilemap_set_scrollx( gfx1_tilemap, 0, scrollx-7 ); //offset to have it aligned with the sprites
	tilemap_set_scrolly( gfx1_tilemap, 0, scrolly );

	tilemap_draw(bitmap,cliprect,gfx1_tilemap,0,0);
	draw_sprites(bitmap);
	tilemap_draw(bitmap,cliprect,gfx0_tilemap,0,0);
}

static INTERRUPT_GEN( witch_interrupt ) {
	cpunum_set_input_line(0,0,ASSERT_LINE);
	cpunum_set_input_line(1,0,ASSERT_LINE);
}

static MACHINE_DRIVER_START( witch )
	/* basic machine hardware */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(readmem,writemem)
	MDRV_CPU_VBLANK_INT(witch_interrupt,1)

	/* 2nd z80 */
	MDRV_CPU_ADD(Z80,8000000)		 /* ? MHz */
	MDRV_CPU_PROGRAM_MAP(cpu2_readmem,cpu2_writemem)

	MDRV_FRAMES_PER_SECOND(60)
//  MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)

	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER )
	MDRV_SCREEN_SIZE(256, 256)
	MDRV_VISIBLE_AREA(8, 256-1-8, 8*2, 256-8*2)
//  MDRV_VISIBLE_AREA(0, 256-1, 0, 256-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(0x300)

	MDRV_VIDEO_START(witch)
	MDRV_VIDEO_UPDATE(witch)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(ES8712, 8000)
	MDRV_SOUND_CONFIG(es8712_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface_0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

	MDRV_SOUND_ADD(YM2203, 1500000)
	MDRV_SOUND_CONFIG(ym2203_interface_1)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.5)

MACHINE_DRIVER_END

/* this set has (c)1992 Sega / Vic Tokai in the roms? */
ROM_START( witch )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )
	ROM_LOAD( "rom.u5", 0x00000, 0x08000, CRC(348fccb8) SHA1(947defd86c4a597fbfb9327eec4903aa779b3788)  )
	ROM_CONTINUE ( 0x10000, 0x18000)

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "rom.u3", 0x00000, 0x20000,  CRC(7007ced4) SHA1(6a0aac3ff9a4d5360c8ba1142f010add1b430ada)  )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "rom.a1", 0x00000, 0x40000,  CRC(512300a5) SHA1(1e9ba58d1ddbfb8276c68f6d5c3591e6b77abf21)  )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "rom.s6", 0x00000, 0x08000, CRC(82460b82) SHA1(d85a9d77edaa67dfab8ff6ac4cb6273f0904b3c0)  )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "rom.v10", 0x00000, 0x40000, CRC(62e42371) SHA1(5042abc2176d0c35fd6b698eca4145f93b0a3944) )
ROM_END

/* no sega logo? a bootleg? */
ROM_START( pbchmp95 )
	ROM_REGION( 0x28000, REGION_CPU1, 0 )
	ROM_LOAD( "3.bin", 0x00000, 0x08000, CRC(e881aa05) SHA1(10d259396cac4b9a1b72c262c11ffa5efbdac433)  )
	ROM_CONTINUE ( 0x10000, 0x18000)

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "2.bin", 0x00000, 0x20000,  CRC(7007ced4) SHA1(6a0aac3ff9a4d5360c8ba1142f010add1b430ada)  )

	ROM_REGION( 0x40000, REGION_GFX2, 0 )
	ROM_LOAD( "1.bin", 0x00000, 0x40000,  CRC(f6cf7ed6) SHA1(327580a17eb2740fad974a01d97dad0a4bef9881)  )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD( "4.bin", 0x00000, 0x08000, CRC(82460b82) SHA1(d85a9d77edaa67dfab8ff6ac4cb6273f0904b3c0)  )

	ROM_REGION( 0x40000, REGION_SOUND1, 0 )
	ROM_LOAD( "5.bin", 0x00000, 0x40000, CRC(62e42371) SHA1(5042abc2176d0c35fd6b698eca4145f93b0a3944) )
ROM_END

DRIVER_INIT(witch)
{
 	UINT8 *ROM = (UINT8 *)memory_region(REGION_CPU1);

	memory_set_bankptr(1,&ROM[UNBANKED_SIZE]);
}

GAME( 1992, witch,    0,     witch, witch, witch, ROT0, "Sega / Vic Tokai", "Witch", GAME_NOT_WORKING )
GAME( 1995, pbchmp95, witch, witch, witch, witch, ROT0, "Veltmeijer Automaten", "Pinball Champ '95 (bootleg?)", GAME_NOT_WORKING )
