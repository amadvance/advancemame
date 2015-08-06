/***************************************************************************

  Bellfruit Adder2 video board driver, (under heavy construction !!!)

  16-08-2005: Decoupled from AGEMAME by El Condor
  19-08-2005: Re-Animator


CPU memorymap:

   hex     |r/w| D D D D D D D D |
 location  |   | 7 6 5 4 3 2 1 0 | function
-----------+---+-----------------+-----------------------------------------
0000       | W | ? ? ? ? ? ? D D | Screen Page latch

                                   bit0 --> 0 = display screen0
                                            1 = display screen1

                                   bit1 --> 0 = CPU can access screen0
                                            1 = CPU can access screen1

-----------+---+-----------------+-----------------------------------------
0000-7FFF  | R | D D D D D D D D | Paged ROM (4 pages)
-----------+---+-----------------+-----------------------------------------
8000-917F  |R/W| D D D D D D D D | Paged Screen RAM (2 pages)
                                 | screen size 128 x 35 bytes
-----------+---+-----------------+-----------------------------------------
9180-9FFF  |R/W| D D D D D D D D | RAM (used for program + stack)
-----------+---+-----------------+-----------------------------------------
A000-BFFF  |R/W| D D D D D D D D | ?window into character RAM/ROM?
-----------+---+-----------------+-----------------------------------------
C000-DFFF  |?/W| D D D D D D D D | I/O registers
C000       | W | ? ? ? ? ? ? D D | program ROM page select
                                   controls what portion of the eprom is mapped
                                   at 0000 - 7FFFF

                                   _______________________________________
                                   bit1 | bit0 | Address in eprom        |
                                   -----+------+-------------------------+
                                   0    | 0    | 00000 - 07FFF           |
                                   -----+------+-------------------------+
                                   0    | 1    | 08000 - 0FFFF (not used)|
                                   -----+------+-------------------------+
                                   1    | 0    | 10000 - 17FFF (not used)|
                                   -----+------+-------------------------+
                                   1    | 1    | 18000 - 1FFFF           |

-----------+---+-----------------+-----------------------------------------
C001       | W | ? ? ? ? ? ? ? D | Palette enable (seems to turn off red)
           |   |                 | 0 = palette disabled (red signal always 0 )
           |   |                 | 1 = palette enabled
-----------+---+-----------------+-----------------------------------------
C002       | W | ? ? ? ? D D D D | Character page register (not used)
-----------+---+-----------------+-----------------------------------------
C100       |R/W| ? ? ? ? ? ? ? ? | Raster IRQ ? (not used in game software)
-----------+---+-----------------+-----------------------------------------
C101       |R/W| ? ? ? ? ? ? ? D | Vertical Blanking IRQ enable
           |   |                 |  bit0  0 = disabled
           |   |                 |        1 = enabled, generate IRQ
-----------+---+-----------------+-----------------------------------------
C102       |R/W| ? ? ? ? ? ? ? D | Pre Vertical Blanking IRQ enable
           |   |                 |  bit0  0 = disabled
           |   |                 |        1 = enabled,
           |   |                 |            generate IRQ 100 cycles before VBL
-----------+---+-----------------+-----------------------------------------
C103       | R | ? ? ? D D D D D | IRQ status
           |   |                 |
           |   |                 |   b0 = Raster IRQ status
           |   |                 |   b1 = VBL start
           |   |                 |   b2 = VBL end
           |   |                 |   b3 = UART IRQ
           |   |                 |   b4 = ???
           |   |                 |
C103       | W | D D D D D D D D | Raster IRQ line number
-----------+---+-----------------+-----------------------------------------
C200       |R/W| D D D D D D D D | UART control reg. (MC6850 compatible)
-----------+---+-----------------+-----------------------------------------
C201       |R/W| D D D D D D D D | UART data    reg. (MC6850 compatible)
-----------+---+-----------------+-----------------------------------------
C202       | W | ? ? ? ? ? ? ? ? | ??
-----------+---+-----------------+-----------------------------------------
C300-C301  |R/W| D D D D D D D D | ?external MC6850??
-----------+---+-----------------+-----------------------------------------
C302       |R/W| D D D D D D D D | board unlock? something something?
-----------+---+-----------------+-----------------------------------------
E000-FFFF  | R | D D D D D D D D | 8K ROM
-----------+---+-----------------+-----------------------------------------

***************************************************************************/

#include "driver.h"
#include "cpu/m6809/m6809.h"
#include "machine/vacfdisp.h"  // vfd
#include "vidhrdw/bfm_adr2.h"
#include "bfm_sc2.h"

int adder2_show_alpha_display;	  // flag, set if alpha display need to be displayed

//#define LOG_CTRL // show UART information
#define FAKE_VIDEO // show alpha display and door

// local prototypes ///////////////////////////////////////////////////////



// local vars /////////////////////////////////////////////////////////////

#define SL_DISPLAY    0x02	// displayed screen,  1=screen1 0=screen0
#define SL_ACCESS     0x01	// accessable screen, 1=screen1 0=screen0



static int adder2_screen_page_reg;		  // access/display select
static int adder2_c101;
static int adder2_rx;
static int adder_vbl_triggered;			  // flag <>0, VBL IRQ triggered
int adder_acia_triggered;		  // flag <>0, ACIA receive IRQ

static UINT8 adder_ram[0xE80];				// normal RAM
static UINT8 adder_screen_ram[2][0x1180];	// paged  display RAM

static tilemap *tilemap0;  // tilemap screen0
static tilemap *tilemap1;  // timemap screen1

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static const gfx_layout charlayout =
{
	8,8,		  // 8 * 8 characters
	8192,		  // 8192  characters
	4,		  // 4     bits per pixel
	{ 0,1,2,3 },
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*8*4, 1*8*4, 2*8*4, 3*8*4, 4*8*4, 5*8*4, 6*8*4, 7*8*4 },
	8*8*4
};

// this is a strange beast !!!!
//
// characters are grouped by 64 (512 pixels)
// there are max 128 of these groups

gfx_decode adder2_gfxdecodeinfo[] =
{
	{ REGION_GFX1,  0, &charlayout, 0, 16 },
	{ -1 } /* end of array */
};

///////////////////////////////////////////////////////////////////////////

static void get_tile0_info(int tile_index)
{
	short data;
	int  code,  color, flags,x,y;

	y = tile_index / 50;
	x = tile_index - (y*50);

	tile_index = y * 128 + (x * 2);

	data =  adder_screen_ram[0][tile_index    ]<<8;
	data |= adder_screen_ram[0][tile_index + 1];

	code  = data & 0x1FFF;
	color = 0;
	flags = ((data & 0x4000)?TILE_FLIPY:0) |
			((data & 0x2000)?TILE_FLIPX:0);

	SET_TILE_INFO(0, code, color, flags)
}


///////////////////////////////////////////////////////////////////////////

static void get_tile1_info(int tile_index)
{
	short data;
	int  code,  color, flags,x,y;

	y = tile_index / 50;
	x = tile_index - (y*50);

	tile_index = y * 128 + (x * 2);

	data =  adder_screen_ram[1][tile_index    ]<<8;
	data |= adder_screen_ram[1][tile_index + 1];

	code  = data & 0x1FFF;
	color = 0;
	flags = ((data & 0x4000)?TILE_FLIPY:0) |
			((data & 0x2000)?TILE_FLIPX:0);

	SET_TILE_INFO(0, code, color, flags)
}

// video initialisation ///////////////////////////////////////////////////

VIDEO_RESET( adder2 )
{
	adder2_screen_page_reg   = 0;
	adder2_c101              = 0;
	adder2_rx                = 0;
	adder_vbl_triggered      = 0;
	adder_acia_triggered     = 0;
	adder2_data_from_sc2     = 0;
	sc2_data_from_adder      = 0;

	{
		UINT8 *rom = memory_region(REGION_CPU2);

		memory_configure_bank(2, 0, 4, &rom[0x00000], 0x08000);

		memory_set_bank(2,0&0x03);

	}
}

VIDEO_START( adder2 )
{
	tilemap0 = tilemap_create(get_tile0_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 50, 35);

	if ( !tilemap0 ) return 1;

	tilemap1 = tilemap_create(get_tile1_info, tilemap_scan_rows, TILEMAP_OPAQUE, 8, 8, 50, 35);

	if ( !tilemap1 ) return 1;


	return 0;
}

// video update ///////////////////////////////////////////////////////////
static rectangle visible1 = { 0, 400,  0,  280 };  //minx,maxx, miny,maxy

VIDEO_UPDATE( adder2 )
{
	if (adder2_screen_page_reg & SL_DISPLAY) tilemap_draw(bitmap, &visible1, tilemap1, 0, 0);
	else                                     tilemap_draw(bitmap, &visible1, tilemap0, 0, 0);

	#ifdef FAKE_VIDEO

	if ( adder2_show_alpha_display )
			draw_16seg(bitmap,10, 280,0,3,1);

	if ( sc2_show_door )
	{
		if ( Scorpion2_GetSwitchState(sc2_door_state>>4, sc2_door_state & 0x0F) )
			ui_draw_text("Door Closed", 320, 284);
		else
			ui_draw_text("Door Open  ", 320, 284);
	}
	#endif
}


// adder2 palette initialisation //////////////////////////////////////////

PALETTE_INIT( adder2 )
{
	palette_set_color( 0,0x00,0x00,0x00);
	palette_set_color( 1,0x00,0x00,0xFF);
	palette_set_color( 2,0x00,0xFF,0x00);
	palette_set_color( 3,0x00,0xFF,0xFF);
	palette_set_color( 4,0xFF,0x00,0x00);
	palette_set_color( 5,0xFF,0x00,0xFF);
	palette_set_color( 6,0xFF,0xFF,0x00);
	palette_set_color( 7,0xFF,0xFF,0xFF);
	palette_set_color( 8,0x80,0x80,0x80);
	palette_set_color( 9,0x00,0x00,0x80);
	palette_set_color(10,0x00,0x80,0x00);
	palette_set_color(11,0x00,0x80,0x80);
	palette_set_color(12,0x80,0x00,0x00);
	palette_set_color(13,0x80,0x00,0x80);
	palette_set_color(14,0x80,0x80,0x00);
	palette_set_color(15,0x80,0x80,0x80);
}

///////////////////////////////////////////////////////////////////////////

MACHINE_RESET( adder2_init_vid )
{
	// setup the standard bellfruit BD1 display /////////////////////////////

	vfd_init(0, VFDTYPE_BFMBD1);

	// reset the board //////////////////////////////////////////////////////

	on_scorpion2_reset();

}

///////////////////////////////////////////////////////////////////////////

INTERRUPT_GEN( adder2_vbl )
{
	if ( adder2_c101 & 0x01 )
	{
		adder_vbl_triggered = 1;
		cpunum_set_input_line(1, M6809_IRQ_LINE, HOLD_LINE );
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( screen_ram_r )
{
	return adder2_screen_page_reg & SL_ACCESS ? adder_screen_ram[1][offset]:adder_screen_ram[0][offset];
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( screen_ram_w )
{
	int dirty_off = (offset>>7)*50 + ((offset & 0x7F)>>1);

	if ( offset > 102 && offset < 102+1+16 )
	{ // format xxxrrggb ////////////////////////////////////////////////////
		int pal;
		UINT8 r,g,b;

		pal = offset-102-1;

		r = ((data & 0x18)>>3) *  85;  // 00011000b = 0x18
		g = ((data & 0x06)>>1) *  85;  // 00000110b = 0x06
		b = ((data & 0x01)   ) * 255;
		palette_set_color( pal, r,g,b);
	}

	if ( adder2_screen_page_reg & SL_ACCESS )
	{
		adder_screen_ram[1][offset] = data;
		tilemap_mark_tile_dirty(tilemap1, dirty_off);
	}

	else
	{
		adder_screen_ram[0][offset] = data;
		tilemap_mark_tile_dirty(tilemap0, dirty_off);
	}
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( normal_ram_r )
{
	return adder_ram[offset];
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( normal_ram_w )
{
	adder_ram[offset] = data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_rom_page_w )
{
	memory_set_bank(2,data&0x03);
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_c001_w )
{
	logerror("c101 = %02X\n",data);

	//adder2_screen_page_reg = 0;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_screen_page_w )
{
	adder2_screen_page_reg = data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( adder2_vbl_ctrl_r )
{
	adder_vbl_triggered = 0;	// clear VBL start IRQ

	return adder2_c101;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_vbl_ctrl_w )
{
	adder2_c101 = data;
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( adder2_uart_ctrl_r )
{
	return get_adder2_uart_status();
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_uart_ctrl_w )
{
	adder2_data_from_sc2 = 0;	// data available for adder from sc2
	adder2_sc2data       = 0;	// data
	sc2_data_from_adder  = 0;	// data available for sc2 from adder
	sc2_adderdata		 = 0;	// data

	#ifdef LOG_CTRL
	logerror("adder2 uart ctrl:%02X\n", data);
	#endif
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( adder2_uart_rx_r )
{
	int data = read_from_sc2();

	return data;
}

///////////////////////////////////////////////////////////////////////////

static WRITE8_HANDLER( adder2_uart_tx_w )
{
	send_to_sc2(data);
}

///////////////////////////////////////////////////////////////////////////

static READ8_HANDLER( adder2_irq_r )
{
	int status = 0;

	if ( adder_vbl_triggered )  status |= 0x02;
	if ( adder_acia_triggered ) status |= 0x08;

	return status;
}

////////////////////////////////////////////////////////////////////
//                                                                //
// decode character data to a format which can be decoded by MAME //
//                                                                //
////////////////////////////////////////////////////////////////////

void adder2_decode_char_roms(void)
{
	UINT8 *p;

	p = memory_region(REGION_GFX1);

	if ( p )
	{
		UINT8 *s;

		s = malloc( 0x40000 );
		if ( s )
		{
			int x, y;

			memcpy(s, p, 0x40000);

			y = 0;

			while ( y < 128 )
			{
				x = 0;
				while ( x < 64 )
				{
					UINT8 *src = s + (y*256*8)+(x*4);

					*p++ = src[0*256+0];*p++ = src[0*256+1];*p++ = src[0*256+2];*p++ = src[0*256+3];
					*p++ = src[1*256+0];*p++ = src[1*256+1];*p++ = src[1*256+2];*p++ = src[1*256+3];
					*p++ = src[2*256+0];*p++ = src[2*256+1];*p++ = src[2*256+2];*p++ = src[2*256+3];
					*p++ = src[3*256+0];*p++ = src[3*256+1];*p++ = src[3*256+2];*p++ = src[3*256+3];
					*p++ = src[4*256+0];*p++ = src[4*256+1];*p++ = src[4*256+2];*p++ = src[4*256+3];
					*p++ = src[5*256+0];*p++ = src[5*256+1];*p++ = src[5*256+2];*p++ = src[5*256+3];
					*p++ = src[6*256+0];*p++ = src[6*256+1];*p++ = src[6*256+2];*p++ = src[6*256+3];
					*p++ = src[7*256+0];*p++ = src[7*256+1];*p++ = src[7*256+2];*p++ = src[7*256+3];
					x++;
				}
				y++;
			}
			free(s);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
// adder2 board memorymap /////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

ADDRESS_MAP_START( adder2_memmap, ADDRESS_SPACE_PROGRAM, 8 )

	AM_RANGE(0x0000, 0x7FFF) AM_READ(MRA8_BANK2)				// 8k  paged ROM (4 pages)
	AM_RANGE(0xE000, 0xFFFF) AM_READ(MRA8_ROM)					// 8k  ROM

	AM_RANGE(0x0000, 0x0000) AM_WRITE(adder2_screen_page_w)		// screen access/display select

	AM_RANGE(0x8000, 0x917F) AM_WRITE(screen_ram_w)
	AM_RANGE(0x8000, 0x917F) AM_READ( screen_ram_r)

	AM_RANGE(0x9180, 0x9FFF) AM_WRITE(normal_ram_w)
	AM_RANGE(0x9180, 0x9FFF) AM_READ( normal_ram_r)

	AM_RANGE(0xC000, 0xC000) AM_WRITE( adder2_rom_page_w )		// ROM page select
	AM_RANGE(0xC001, 0xC001) AM_WRITE( adder2_c001_w )			// ??

	AM_RANGE(0xC101, 0xC101) AM_WRITE( adder2_vbl_ctrl_w )
	AM_RANGE(0xC101, 0xC101) AM_READ(  adder2_vbl_ctrl_r )
	AM_RANGE(0xC103, 0xC103) AM_READ(  adder2_irq_r );			// IRQ latch read

  // MC6850 compatible uart connected to main (scorpion2) board /////////////////////////////////////

	AM_RANGE(0xC200, 0xC200) AM_READ(  adder2_uart_ctrl_r );	// 6850 compatible uart control reg read
	AM_RANGE(0xC200, 0xC200) AM_WRITE( adder2_uart_ctrl_w );	// 6850 compatible uart control reg write
	AM_RANGE(0xC201, 0xC201) AM_READ(  adder2_uart_rx_r );  	// 6850 compatible uart read  data
	AM_RANGE(0xC201, 0xC201) AM_WRITE( adder2_uart_tx_w );  	// 6850 compatible uart write data

ADDRESS_MAP_END
