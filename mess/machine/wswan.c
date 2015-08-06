/***************************************************************************

  wswan.c

  Machine file to handle emulation of the Bandai WonderSwan.

  Anthony Kruize
  Wilbert Pol

***************************************************************************/

#include "driver.h"
#include "includes/wswan.h"
#include "image.h"

#define INTERNAL_EEPROM_SIZE	1024

enum enum_system { WSWAN=0, WSC };
enum enum_sram { SRAM_NONE=0, SRAM_64K, SRAM_256K, SRAM_1M, SRAM_2M, EEPROM_1K, EEPROM_16K, EEPROM_8K, SRAM_UNKNOWN };
const char* wswan_sram_str[] = { "none", "64K SRAM", "256K SRAM", "1M SRAM", "2M SRAM", "1K EEPROM", "16K EEPROM", "8K EEPROM", "Unknown" };
const int wswan_sram_size[] = { 0, 64*1024, 256*1024, 1024*1024, 2*1024*1024,  1024, 16*1024, 8*1024, 0 };

struct EEPROM {
	UINT8	mode;		/* eeprom mode */
	UINT16	address;	/* Read/write address */
	UINT8	command;	/* Commands: 00, 01, 02, 03, 04, 08, 0C */
	UINT8	start;		/* start bit */
	UINT8	write_enabled;	/* write enabled yes/no */
	int	size;		/* size of eeprom/sram area */
	UINT8	*data;		/* pointer to start of sram/eeprom data */
};

struct RTC {
	UINT8	present;	/* Is an RTC present */
	UINT8	setting;	/* Timer setting byte */
	UINT8	year;		/* Year */
	UINT8	month;		/* Month */
	UINT8	day;		/* Day */
	UINT8	day_of_week;	/* Day of the week */
	UINT8	hour;		/* Hour, high bit = 0 => AM, high bit = 1 => PM */
	UINT8	minute;		/* Minute */
	UINT8	second;		/* Second */
	UINT8	index;		/* index for reading/writing of current of alarm time */
};

static UINT8 *ROMMap[256];
static UINT32 ROMBanks;
static UINT8 internal_eeprom[INTERNAL_EEPROM_SIZE];
static UINT8 system_type;
struct VDP vdp;
static struct EEPROM eeprom;
static struct RTC rtc;
UINT8 *ws_ram;
UINT8 ws_portram[256];
static UINT8 ws_portram_init[256] =
{
	0x00, 0x00, 0x00/*?*/, 0xbb, 0x00, 0x00, 0x00, 0x26, 0xfe, 0xde, 0xf9, 0xfb, 0xdb, 0xd7, 0x7f, 0xf5,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x9e, 0x9b, 0x00, 0x00, 0x00, 0x00, 0x99, 0xfd, 0xb7, 0xdf,
	0x30, 0x57, 0x75, 0x76, 0x15, 0x73, 0x70/*77?*/, 0x77, 0x20, 0x75, 0x50, 0x36, 0x70, 0x67, 0x50, 0x77,
	0x57, 0x54, 0x75, 0x77, 0x75, 0x17, 0x37, 0x73, 0x50, 0x57, 0x60, 0x77, 0x70, 0x77, 0x10, 0x73,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00,
	0x87, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x4f, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xdb, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x42, 0x00, 0x83, 0x00,
	0x2f, 0x3f, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1,
	0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1, 0xd1
};

void wswan_handle_irqs( void ) {
	if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_HBLTMR ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_HBLTMR );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_VBL ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_VBL );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_VBLTMR ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_VBLTMR );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_LCMP ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_LCMP );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_SRX ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_SRX );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_RTC ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_RTC );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_KEY ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_KEY );
	} else if ( ws_portram[0xb2] & ws_portram[0xb6] & WSWAN_IFLAG_STX ) {
		cpunum_set_input_line_and_vector( 0, 0, HOLD_LINE, ws_portram[0xb0] + WSWAN_INT_STX );
	} else {
		cpunum_set_input_line( 0, 0, CLEAR_LINE );
	}
}

void wswan_set_irq_line(int irq) {
	if ( ws_portram[0xb2] & irq ) {
		ws_portram[0xb6] |= irq;
		wswan_handle_irqs();
	}
}

void wswan_clear_irq_line(int irq) {
	ws_portram[0xb6] &= ~irq;
	wswan_handle_irqs();
}

static void wswan_rtc_callback( int dummy ) {
	/* A second passed */
	rtc.second = rtc.second + 1;
	if ( ( rtc.second & 0x0F ) > 9 ) {
		rtc.second = ( rtc.second & 0xF0 ) + 0x10;
	}

	/* Check for minute passed */
	if ( rtc.second >= 0x60 ) {
		rtc.second = 0;
		rtc.minute = rtc.minute + 1;
		if ( ( rtc.minute & 0x0F ) > 9 ) {
			rtc.minute = ( rtc.minute & 0xF0 ) + 0x10;
		}
	}

	/* Check for hour passed */
	if ( rtc.minute >= 0x60 ) {
		rtc.minute = 0;
		rtc.hour = rtc.hour + 1;
		if ( ( rtc.hour & 0x0F ) > 9 ) {
			rtc.hour = ( rtc.hour & 0xF0 ) + 0x10;
		}
		if ( rtc.hour == 0x12 ) {
			rtc.hour |= 0x80;
		}
	}

	/* Check for day passed */
	if ( rtc.hour >= 0xA4 ) {
		rtc.hour = 0;
		rtc.day = rtc.day + 1;
	}
}

static void wswan_machine_stop( void ) {
	if ( eeprom.size ) {
		image_battery_save( image_from_devtype_and_index(IO_CARTSLOT,0), eeprom.data, eeprom.size );
	}
}

MACHINE_START( wswan ) {
	system_type = WSWAN;
	add_exit_callback( wswan_machine_stop );
	return 0;
}

MACHINE_START( wscolor ) {
	system_type = WSC;
	add_exit_callback( wswan_machine_stop );
	return 0;
}

MACHINE_RESET( wswan )
{
	/* Intialize ports */
	memcpy( ws_portram, ws_portram_init, 256 );

	/* Initialize VDP */
	memset( &vdp, 0, sizeof( vdp ) );

	vdp.vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, 0 );
	vdp.palette_vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, ( system_type == WSC ) ? 0xFE00 : 0 );
	vdp.current_line = 145;  /* Randomly chosen, beginning of VBlank period to give cart some time to boot up */
	vdp.new_display_vertical = ROMMap[ROMBanks-1][0xfffc] & 0x01;
	vdp.display_vertical = ~vdp.new_display_vertical;
	vdp.color_mode = 0;
	vdp.colors_16 = 0;
	vdp.tile_packed = 0;

	/* Switch in the banks */
	memory_set_bankptr( 1, eeprom.data );
	memory_set_bankptr( 2, ROMMap[(ROMBanks - 1) & (ROMBanks - 1)] );
	memory_set_bankptr( 3, ROMMap[(ROMBanks - 1) & (ROMBanks - 1)] );
	memory_set_bankptr( 4, ROMMap[(ROMBanks - 12) & (ROMBanks - 1)] );
	memory_set_bankptr( 5, ROMMap[(ROMBanks - 11) & (ROMBanks - 1)] );
	memory_set_bankptr( 6, ROMMap[(ROMBanks - 10) & (ROMBanks - 1)] );
	memory_set_bankptr( 7, ROMMap[(ROMBanks - 9) & (ROMBanks - 1)] );
	memory_set_bankptr( 8, ROMMap[(ROMBanks - 8) & (ROMBanks - 1)] );
	memory_set_bankptr( 9, ROMMap[(ROMBanks - 7) & (ROMBanks - 1)] );
	memory_set_bankptr( 10, ROMMap[(ROMBanks - 6) & (ROMBanks - 1)] );
	memory_set_bankptr( 11, ROMMap[(ROMBanks - 5) & (ROMBanks - 1)] );
	memory_set_bankptr( 12, ROMMap[(ROMBanks - 4) & (ROMBanks - 1)] );
	memory_set_bankptr( 13, ROMMap[(ROMBanks - 3) & (ROMBanks - 1)] );
	memory_set_bankptr( 14, ROMMap[(ROMBanks - 2) & (ROMBanks - 1)] );
	memory_set_bankptr( 15, ROMMap[(ROMBanks - 1) & (ROMBanks - 1)] );

	/* Set up RTC timer */
	if ( rtc.present ) {
		timer_pulse( TIME_IN_SEC(1), 0, wswan_rtc_callback );
	}
}

NVRAM_HANDLER( wswan ) {
	if ( read_or_write ) {
		/* Load the EEPROM data */
		mame_fwrite( file, internal_eeprom, INTERNAL_EEPROM_SIZE );
	} else {
		/* Load the EEPROM data */
		if ( file ) {
			mame_fread( file, internal_eeprom, INTERNAL_EEPROM_SIZE );
		} else {
			/* Initialize the EEPROM data */
			memset( internal_eeprom, 0xFF, sizeof( internal_eeprom ) );
		}
	}
}

READ8_HANDLER( wswan_port_r )
{
	UINT8 value = ws_portram[offset];

	if ( offset != 2 ) 
	logerror( "PC=%X: port read %02X\n", activecpu_get_pc(), offset );
	switch( offset )
	{
		case 0x02:		/* Current line */
			value = vdp.current_line;
			break;
		case 0xA0:		/* Hardware type */
			value = value & ~ 0x02;
			if ( system_type == WSC ) {
				value |= 2;
			}
			break;
		case 0xA8:
			value = vdp.timer_hblank_count & 0xFF;
			break;
		case 0xA9:
			value = vdp.timer_hblank_count >> 8;
			break;
		case 0xAA:
			value = vdp.timer_vblank_count & 0xFF;
			break;
		case 0xAB:
			value = vdp.timer_vblank_count >> 8;
			break;
		case 0xCB:		/* RTC data */
			if ( ws_portram[0xca] == 0x95 && ( rtc.index < 7 ) ) {
				switch( rtc.index ) {
				case 0: value = rtc.year; break;
				case 1: value = rtc.month; break;
				case 2: value = rtc.day; break;
				case 3: value = rtc.day_of_week; break;
				case 4: value = rtc.hour; break;
				case 5: value = rtc.minute; break;
				case 6: value = rtc.second; break;
				}
				rtc.index++;
			}
	}

	return value;
}

WRITE8_HANDLER( wswan_port_w )
{
	logerror( "PC=%X: port write %02X <- %02X\n", activecpu_get_pc(), offset, data );
	switch( offset )
	{
		case 0x00:		/* Display control */
			vdp.layer_bg_enable = data & 0x1;
			vdp.layer_fg_enable = (data & 0x2) >> 1;
			vdp.sprites_enable = (data & 0x4) >> 2;
			vdp.window_sprites_enable = (data & 0x8) >> 3;
			vdp.window_fg_mode = (data & 0x30) >> 4;
			break;
		case 0x01:		/* Background colour */
			/* FIXME: implement */
			break;
		case 0x02:		/* Current scanline - most likely read-only */
			logerror( "Write to current scanline! Current value: %d  Data to write: %d\n", vdp.current_line, data );
			/* Returning so we don't overwrite the value here, not that it
			 * really matters */
			return;
		case 0x03:		/* Line compare */
			vdp.line_compare = data;
			break;
		case 0x04:		/* Sprite table base address */
			vdp.sprite_table_address = ( data & 0x3F ) << 9;
			break;
		case 0x05:		/* Number of sprite to start drawing with */
			vdp.sprite_first = data;
			break;
		case 0x06:		/* Number of sprites to draw */
			vdp.sprite_count = data;
			break;
		case 0x07:		/* Screen addresses */
			vdp.layer_bg_address = (data & 0x7) << 11;
			vdp.layer_fg_address = (data & 0x70) << 7;
			break;
		case 0x08:		/* Left coordinate of foreground window */
			vdp.window_fg_left = data;
			break;
		case 0x09:		/* Top coordinate of foreground window */
			vdp.window_fg_top = data;
			break;
		case 0x0A:		/* Right coordinate of foreground window */
			vdp.window_fg_right = data;
			break;
		case 0x0B:		/* Bottom coordinate of foreground window */
			vdp.window_fg_bottom = data;
			break;
		case 0x0C:		/* Left coordinate of sprite window */
			vdp.window_sprites_left = data;
			break;
		case 0x0D:		/* Top coordinate of sprite window */
			vdp.window_sprites_top = data;
			break;
		case 0x0E:		/* Right coordinate of sprite window */
			vdp.window_sprites_right = data;
			break;
		case 0x0F:		/* Bottom coordinate of sprite window */
			vdp.window_sprites_bottom = data;
			break;
		case 0x10:		/* Background layer X scroll */
			vdp.layer_bg_scroll_x = data;
			break;
		case 0x11:		/* Background layer Y scroll */
			vdp.layer_bg_scroll_y = data;
			break;
		case 0x12:		/* Foreground layer X scroll */
			vdp.layer_fg_scroll_x = data;
			break;
		case 0x13:		/* Foreground layer Y scroll */
			vdp.layer_fg_scroll_y = data;
			break;
		case 0x14:		/* LCD enable */
			vdp.lcd_enable = data & 0x1;
			break;
		case 0x15:		/* LCD icons */
			vdp.icons = data;	/* ummmmm */
			break;
		case 0x1c:		/* Palette colors 1 and 2 */
			if ( system_type == WSC ) {
				int i = 15 - ( data & 0x0F );
				int j = 15 - ( ( data & 0xF0 ) >> 4 );
				vdp.main_palette[0] = ( i << 8 ) | ( i << 4 ) | i;
				vdp.main_palette[1] = ( j << 8 ) | ( j << 4 ) | j;
			} else {
				vdp.main_palette[0] = data & 0x0F;
				vdp.main_palette[1] = ( data & 0xF0 ) >> 4;
			}
			break;
		case 0x1d:		/* Palette colors 3 and 4 */
			if ( system_type == WSC ) {
				int i = 15 - ( data & 0x0F );
				int j = 15 - ( ( data & 0xF0 ) >> 4 );
				vdp.main_palette[2] = ( i << 8 ) | ( i << 4 ) | i;
				vdp.main_palette[3] = ( j << 8 ) | ( j << 4 ) | j;
			} else {
				vdp.main_palette[2] = data & 0x0F;
				vdp.main_palette[3] = ( data & 0xF0 ) >> 4;
			}
			break;
		case 0x1e:		/* Palette colors 5 and 6 */
			if ( system_type == WSC ) {
				int i = 15 - ( data & 0x0F );
				int j = 15 - ( ( data & 0xF0 ) >> 4 );
				vdp.main_palette[4] = ( i << 8 ) | ( i << 4 ) | i;
				vdp.main_palette[5] = ( j << 8 ) | ( j << 4 ) | j;
			} else {
				vdp.main_palette[4] = data & 0x0F;
				vdp.main_palette[5] = ( data & 0xF0 ) >> 4;
			}
			break;
		case 0x1f:		/* Palette colors 7 and 8 */
			if ( system_type == WSC ) {
				int i = 15 - ( data & 0x0F );
				int j = 15 - ( ( data & 0xF0 ) >> 4 );
				vdp.main_palette[6] = ( i << 8 ) | ( i << 4 ) | i;
				vdp.main_palette[7] = ( j << 8 ) | ( j << 4 ) | j;
			} else {
				vdp.main_palette[6] = data & 0x0F;
				vdp.main_palette[7] = ( data & 0xF0 ) >> 4;
			}
			break;
		case 0x20:		/* tile/sprite palette settings */
		case 0x21:
		case 0x22:
		case 0x23:
		case 0x24:
		case 0x25:
		case 0x26:
		case 0x27:
		case 0x28:
		case 0x29:
		case 0x2A:
		case 0x2B:
		case 0x2C:
		case 0x2D:
		case 0x2E:
		case 0x2F:
		case 0x30:
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
		case 0x3F:
			break;
		case 0x40:		/* DMA source address (low) */
		case 0x41:		/* DMA source address (high) */
		case 0x42:		/* DMA source bank */
		case 0x43:		/* DMA destination bank */
		case 0x44:		/* DMA destination address (low) */
		case 0x45:		/* DMA destination address (hgih) */
		case 0x46:		/* Size of copied data (low) */
		case 0x47:		/* Size of copied data (high) */
			break;
		case 0x48:		/* DMA start/stop */
			if( data & 0x80 )
			{
				UINT32 src, dst;
				UINT16 length;

				src = ws_portram[0x40] + (ws_portram[0x41] << 8) + (ws_portram[0x42] << 16);
				dst = ws_portram[0x44] + (ws_portram[0x45] << 8) + (ws_portram[0x43] << 16);
				length = ws_portram[0x46] + (ws_portram[0x47] << 8);
				for( ; length > 0; length-- ) {
					program_write_byte_8( dst, program_read_byte_8( src ) );
					src++;
					dst++;
				}
#ifdef DEBUG
					logerror( "DMA  src:%X dst:%X length:%d\n", src, dst, length );
#endif
				ws_portram[0x40] = src & 0xFF;
				ws_portram[0x41] = ( src >> 8 ) & 0xFF;
				ws_portram[0x44] = dst & 0xFF;
				ws_portram[0x45] = ( dst >> 8 ) & 0xFF;
				ws_portram[0x46] = length & 0xFF;
				ws_portram[0x47] = ( length >> 8 ) & 0xFF;
				data &= 0x7F;
			}
			break;
		case 0x60:		/* Video mode */
			/*
			 * 111	- packed, 16 color, use 4000/8000, color
			 * 110	- not packed, 16 color, use 4000/8000, color
			 * 101	- packed, 4 color, use 2000, color
			 * 100	- not packed, 4 color, use 2000, color
			 * 011	- packed, 16 color, use 4000/8000, monochrome
			 * 010	- not packed, 16 color , use 4000/8000, monochrome
			 * 001	- packed, 4 color, use 2000, monochrome
			 * 000  - not packed, 4 color, use 2000, monochrome - Regular WS monochrome
			 */
			if ( system_type == WSC ) {
				vdp.color_mode = data & 0x80;
				vdp.colors_16 = data & 0x40;
				vdp.tile_packed = data & 0x20;
			}
			break;
		case 0x80:		/* Audio 1 freq (lo) */
		case 0x81:		/* Audio 1 freq (hi) */
		case 0x82:		/* Audio 2 freq (lo) */
		case 0x83:		/* Audio 2 freq (hi) */
		case 0x84:		/* Audio 3 freq (lo) */
		case 0x85:		/* Audio 3 freq (hi) */
		case 0x86:		/* Audio 4 freq (lo) */
		case 0x87:		/* Audio 4 freq (hi) */
		case 0x88:		/* Audio 1 volume */
		case 0x89:		/* Audio 2 volume */
		case 0x8A:		/* Audio 3 volume */
		case 0x8B:		/* Audio 4 volume */
		case 0x8C:		/* Sweep value */
		case 0x8D:		/* Sweep step */
		case 0x8E:		/* Noise control */
		case 0x8F:		/* Sample location */
		case 0x90:		/* Audio control */
		case 0x91:		/* Audio output */
		case 0x92:		/* Noise counter shift register (lo) */
		case 0x93:		/* Noise counter shift register (hi) */
		case 0x94:		/* Master volume */
			wswan_sound_port_w( offset, data );
			break;
		case 0xa0:		/* Hardware type - this is probably read only */
			break;
		case 0xa2:		/* Timer control */
			vdp.timer_hblank_enable = data & 0x1;
			vdp.timer_hblank_mode = (data & 0x2) >> 1;
			vdp.timer_vblank_enable = (data & 0x4) >> 2;
			vdp.timer_vblank_mode = (data & 0x8) >> 3;
			break;
		case 0xa4:		/* HBlank timer frequency (low) - reload value */
			vdp.timer_hblank_reload &= 0xff00;
			vdp.timer_hblank_reload += data;
			vdp.timer_hblank_count = vdp.timer_hblank_reload;
			break;
		case 0xa5:		/* HBlank timer frequenct (high) - reload value */
			vdp.timer_hblank_reload &= 0xff;
			vdp.timer_hblank_reload += data << 8;
			vdp.timer_hblank_count = vdp.timer_hblank_reload;
			break;
		case 0xa6:		/* VBlank timer frequency (low) - reload value */
			vdp.timer_vblank_reload &= 0xff00;
			vdp.timer_vblank_reload += data;
			vdp.timer_vblank_count = vdp.timer_vblank_reload;
			break;
		case 0xa7:		/* VBlank timer frequency (high) - reload value */
			vdp.timer_vblank_reload &= 0xff;
			vdp.timer_vblank_reload += data << 8;
			vdp.timer_vblank_count = vdp.timer_vblank_reload;
			break;
		case 0xa8:		/* Timer counters */
		case 0xa9:
		case 0xaa:
		case 0xab:
			break;

		case 0xb0:		/* Interrupt base vector */
			break;
		case 0xb2:		/* Interrupt enable */
			break;
		case 0xb3:		/* serial communication */
//			data |= 0x02;
			ws_portram[0xb1] = 0xFF;
			if ( data & 0x80 ) {
//				ws_portram[0xb1] = 0x00;
				data |= 0x04;
			}
			if (data & 0x20 ) {
//				data |= 0x01;
			}
			break;
		case 0xb5:		/* Read controls */
			data = data & 0xF0;
			switch( data ) {
			case 0x10:	/* Read Y cursors: Y1 - Y2 - Y3 - Y4 */
				data = data | readinputport( 2 );
				break;
			case 0x20:	/* Read X cursors: X1 - X2 - X3 - X4 */
				data = data | readinputport( 0 );
				break;
			case 0x40:	/* Read buttons: START - A - B */
				data = data | readinputport( 1 );
				break;
			}
			break;
		case 0xb6:		/* Interrupt acknowledge */
			wswan_clear_irq_line( data );
			data = ws_portram[0xB6];
			break;
		case 0xba:		/* Internal EEPROM data */
		case 0xbb:
			break;
		case 0xbc:		/* Internal EEPROM address */
		case 0xbd:
			break;
		case 0xbe:		/* Internal EEPROM command */
			if ( data & 0x20 ) {
				UINT16 addr = ( ( ( ws_portram[0xbd] << 8 ) | ws_portram[0xbc] ) << 1 ) & 0x1FF;
				internal_eeprom[ addr ] = ws_portram[0xba];
				internal_eeprom[ addr + 1 ] = ws_portram[0xbb];
				data |= 0x02;
			} else if ( data & 0x10 ) {
				UINT16 addr = ( ( ( ws_portram[0xbd] << 8 ) | ws_portram[0xbc] ) << 1 ) & 0x1FF;
				ws_portram[0xba] = internal_eeprom[ addr ];
				ws_portram[0xbb] = internal_eeprom[ addr + 1];
				data |= 0x01;
			} else {
				logerror( "Unsupported internal EEPROM command: %X\n", data );
			}
			break;
		case 0xc0:		/* ROM bank select for banks 4-15 */
			memory_set_bankptr( 4, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 4 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 5, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 5 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 6, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 6 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 7, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 7 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 8, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 8 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 9, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 9 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 10, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 10 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 11, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 11 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 12, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 12 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 13, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 13 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 14, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 14 ) & ( ROMBanks - 1 ) ] );
			memory_set_bankptr( 15, ROMMap[ ( ( ( data & 0x0F ) << 4 ) | 15 ) & ( ROMBanks - 1 ) ] );
			break;
		case 0xc1:		/* SRAM bank select */
			if ( eeprom.mode == SRAM_64K || eeprom.mode == SRAM_256K || eeprom.mode == SRAM_1M || eeprom.mode == SRAM_2M ) {
				memory_set_bankptr( 1, &eeprom.data[ ( data * 64 * 1024 ) & ( eeprom.size - 1 ) ] );
			}
			break;
		case 0xc2:		/* ROM bank select for bank 1 (0x2000-0x2fff) */
			memory_set_bankptr( 2, ROMMap[ data & ( ROMBanks - 1 ) ]);
			break;
		case 0xc3:		/* ROM bank select for bank 2 (0x3000-0x3fff) */
			memory_set_bankptr( 3, ROMMap[ data & ( ROMBanks - 1 ) ]);
			break;
		case 0xc6:		/* EEPROM address lower bits port */
			switch( eeprom.mode ) {
			case EEPROM_1K:
				eeprom.address = data & 0x3F;
				eeprom.command = data >> 4;
				if ( ( eeprom.command & 0x0C ) != 0x00 ) {
					eeprom.command = eeprom.command & 0x0C;
				}
				break;
			case EEPROM_16K:
				eeprom.address = ( eeprom.address & 0xFF00 ) | data;
				break;
			default:
				logerror( "Write EEPROM address/register register C6 for unsupported EEPROM type\n" );
				break;
			}
			break;
		case 0xc7:		/* EEPROM higher bits/command bits port */
			switch( eeprom.mode ) {
			case EEPROM_1K:
				eeprom.start = data & 0x01;
				break;
			case EEPROM_16K:
				eeprom.address = ( ( data & 0x03 ) << 8 ) | ( eeprom.address & 0xFF );
				eeprom.command = data & 0x0F;
				if ( ( eeprom.command & 0x0C ) != 0x00 ) {
					eeprom.command = eeprom.command & 0x0C;
				}
				eeprom.start = ( data >> 4 ) & 0x01;
				break;
			default:
				logerror( "Write EEPROM address/command register C7 for unsupported EEPROM type\n" );
				break;
			}
			break;
		case 0xc8:
			if ( eeprom.mode == EEPROM_1K || eeprom.mode == EEPROM_16K ) {
				if ( data & 0x80 ) {	/* Initialize */
					logerror( "Unsupported EEPROM command 'Initialize'\n" );
				}
				if ( data & 0x40 ) {	/* Protect */
					switch( eeprom.command ) {
					case 0x00:
						eeprom.write_enabled = 0;
						data |= 0x02;
						break;
					case 0x03:
						eeprom.write_enabled = 1;
						data |= 0x02;
						break;
					default:
						logerror( "Unsupported 'Protect' command %X\n", eeprom.command );
					}
				}
				if ( data & 0x20 ) {	/* Write */
					if ( eeprom.write_enabled ) {
						switch( eeprom.command ) {
						case 0x04:
							eeprom.data[ ( eeprom.address << 1 ) + 1 ] = ws_portram[0xc4];
							eeprom.data[ eeprom.address << 1 ] = ws_portram[0xc5];
							data |= 0x02;
							break;
						default:
							logerror( "Unsupported 'Write' command %X\n", eeprom.command );
						}
					}
				}
				if ( data & 0x10 ) {	/* Read */
					ws_portram[0xc4] = eeprom.data[ ( eeprom.address << 1 ) + 1 ];
					ws_portram[0xc5] = eeprom.data[ eeprom.address << 1 ];
					data |= 0x01;
				}
			} else {
				logerror( "EEPROM command for unknown EEPROM type\n" );
			}
			break;
		case 0xca:		/* RTC Command */
			switch( data ) {
			case 0x10:	/* Reset */
				rtc.index = 8;
				rtc.year = 0;
				rtc.month = 1;
				rtc.day = 1;
				rtc.day_of_week = 0;
				rtc.hour = 0;
				rtc.minute = 0;
				rtc.second = 0;
				rtc.setting = 0xFF;
				data |= 0x80;
				break;
			case 0x12:	/* Write Timer Settings (Alarm) */
				rtc.index = 8;
				rtc.setting = ws_portram[0xcb];
				data |= 0x80;
				break;
			case 0x13:	/* Read Timer Settings (Alarm) */
				rtc.index = 8;
				ws_portram[0xcb] = rtc.setting;
				data |= 0x80;
				break;
			case 0x14:	/* Set Time/Date */
				rtc.year = ws_portram[0xcb];
				rtc.index = 1;
				data |= 0x80;
				break;
			case 0x15:	/* Get Time/Date */
				rtc.index = 0;
				data |= 0x80;
				ws_portram[0xcb] = rtc.year;
				break;
			default:
				logerror( "%X: Unknown RTC command (%X) requested\n", activecpu_get_pc(), data );
			}
			break;
		case 0xcb:		/* RTC Data */
			if ( ws_portram[0xca] == 0x94 && rtc.index < 7 ) {
				switch( rtc.index ) {
				case 0:	rtc.year = data; break;
				case 1: rtc.month = data; break;
				case 2: rtc.day = data; break;
				case 3: rtc.day_of_week = data; break;
				case 4: rtc.hour = data; break;
				case 5: rtc.minute = data; break;
				case 6: rtc.second = data; break;
				}
				rtc.index++;
			}
			break;
		default:
			logerror( "Write to unsupported port: %X - %X\n", offset, data );
			break;
	}

	/* Update the port value */
	ws_portram[offset] = data;
}

static const char* wswan_determine_sram( UINT8 data ) {
	eeprom.write_enabled = 0;
	eeprom.mode = SRAM_UNKNOWN;
	switch( data ) {
	case 0x00: eeprom.mode = SRAM_NONE; break;
	case 0x01: eeprom.mode = SRAM_64K; break;
	case 0x02: eeprom.mode = SRAM_256K; break;
	case 0x03: eeprom.mode = SRAM_1M; break;
	case 0x04: eeprom.mode = SRAM_2M; break;
	case 0x10: eeprom.mode = EEPROM_1K; break;
	case 0x20: eeprom.mode = EEPROM_16K; break;
	case 0x50: eeprom.mode = EEPROM_8K; break;
	}
	eeprom.size = wswan_sram_size[ eeprom.mode ];
	return wswan_sram_str[ eeprom.mode ];
}

enum enum_romsize { ROM_4M=0, ROM_8M, ROM_16M, ROM_32M, ROM_64M, ROM_128M, ROM_UNKNOWN };
const char* wswan_romsize_str[] = {
	"4Mbit", "8Mbit", "16Mbit", "32Mbit", "64Mbit", "128Mbit", "Unknown"
};

static const char* wswan_determine_romsize( UINT8 data ) {
	switch( data ) {
	case 0x02:	return wswan_romsize_str[ ROM_4M ];
	case 0x03:	return wswan_romsize_str[ ROM_8M ];
	case 0x04:	return wswan_romsize_str[ ROM_16M ];
	case 0x06:	return wswan_romsize_str[ ROM_32M ];
	case 0x08:	return wswan_romsize_str[ ROM_64M ];
	case 0x09:	return wswan_romsize_str[ ROM_128M ];
	}
	return wswan_romsize_str[ ROM_UNKNOWN ];
}

DEVICE_INIT(wswan_cart)
{
	/* Initialize EEPROM structure */
	memset( &eeprom, 0, sizeof( eeprom ) );
	eeprom.data = NULL;

	/* Initialize RTC structure */
	rtc.present = 0;
	rtc.index = 0;
	rtc.year = 0;
	rtc.month = 0;
	rtc.day = 0;
	rtc.day_of_week = 0;
	rtc.hour = 0;
	rtc.minute = 0;
	rtc.second = 0;
	rtc.setting = 0xFF;

	return INIT_PASS;
}

DEVICE_LOAD(wswan_cart)
{
	UINT32 ii;
	const char *sram_str;

	ws_ram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, 0 );
	memset( ws_ram, 0, 0xFFFF );
	ROMBanks = mame_fsize( file ) / 65536;

	for( ii = 0; ii < ROMBanks; ii++ )
	{
		if( (ROMMap[ii] = (UINT8 *)malloc( 0x10000 )) )
		{
			if( mame_fread( file, ROMMap[ii], 0x10000 ) != 0x10000 )
			{
				logerror( "Error while reading loading rom!\n" );
				return INIT_FAIL;
			}
		}
		else
		{
			logerror( "Memory allocation failed reading rom!\n" );
			return INIT_FAIL;
		}
	}

	sram_str = wswan_determine_sram( ROMMap[ROMBanks-1][0xfffb] );

	rtc.present = ROMMap[ROMBanks-1][0xfffd] ? 1 : 0;

#ifdef MAME_DEBUG
	/* Spit out some info */
	printf( "ROM DETAILS\n" );
	printf( "\tDeveloper ID: %X\n", ROMMap[ROMBanks-1][0xfff6] );
	printf( "\tMinimum system: %s\n", ROMMap[ROMBanks-1][0xfff7] ? "WonderSwan Color" : "WonderSwan" );
	printf( "\tCart ID: %X\n", ROMMap[ROMBanks-1][0xfff8] );
	printf( "\tROM size: %s\n", wswan_determine_romsize( ROMMap[ROMBanks-1][0xfffa] ) );
	printf( "\tSRAM size: %s\n", sram_str );
	printf( "\tFeatures: %X\n", ROMMap[ROMBanks-1][0xfffc] );
	printf( "\tRTC: %s\n", ( ROMMap[ROMBanks-1][0xfffd] ? "yes" : "no" ) );
	printf( "\tChecksum: %X%X\n", ROMMap[ROMBanks-1][0xffff], ROMMap[ROMBanks-1][0xfffe] );
#endif

	if ( eeprom.size != 0 ) {
		eeprom.data = auto_malloc( eeprom.size );
		image_battery_load( image, eeprom.data, eeprom.size );
	}

	logerror( "Image Name: %s\n", image_longname( image ) );
	logerror( "Image Year: %s\n", image_year( image ) );
	logerror( "Image Manufacturer: %s\n", image_manufacturer( image ) );

	/* All done */
	return INIT_PASS;
}

INTERRUPT_GEN(wswan_scanline_interrupt)
{
	if( vdp.current_line < 144 ) {
		wswan_refresh_scanline();
	}

	/* Decrement 12kHz (HBlank) counter */
	if ( vdp.timer_hblank_enable && vdp.timer_hblank_reload != 0 ) {
		vdp.timer_hblank_count--;
		logerror( "timer_hblank_count: %X\n", vdp.timer_hblank_count );
		if ( vdp.timer_hblank_count == 0 ) {
			if ( vdp.timer_hblank_mode ) {
				vdp.timer_hblank_count = vdp.timer_hblank_reload;
			} else {
				vdp.timer_hblank_reload = 0;
			}
			logerror( "trigerring hbltmr interrupt\n" );
			wswan_set_irq_line( WSWAN_IFLAG_HBLTMR );
		}
	}

//	vdp.current_line = (vdp.current_line + 1) % 159;

	if( vdp.current_line == 144 ) {
		wswan_set_irq_line( WSWAN_IFLAG_VBL );
		/* Decrement 75Hz (VBlank) counter */
		if ( vdp.timer_vblank_enable && vdp.timer_vblank_reload != 0 ) {
			vdp.timer_vblank_count--;
			logerror( "timer_vblank_count: %X\n", vdp.timer_vblank_count );
			if ( vdp.timer_vblank_count == 0 ) {
				if ( vdp.timer_vblank_mode ) {
					vdp.timer_vblank_count = vdp.timer_vblank_reload;
				} else {
					vdp.timer_vblank_reload = 0;
				}
				logerror( "triggering vbltmr interrupt\n" );
				wswan_set_irq_line( WSWAN_IFLAG_VBLTMR );
			}
		}
	}

//	vdp.current_line = (vdp.current_line + 1) % 159; 

	if ( vdp.current_line == vdp.line_compare ) {
		wswan_set_irq_line( WSWAN_IFLAG_LCMP );
	}

	vdp.current_line = (vdp.current_line + 1) % 159;

	if ( vdp.current_line == 0 ) {
		if ( vdp.display_vertical != vdp.new_display_vertical ) {
			vdp.display_vertical = vdp.new_display_vertical;
			if ( vdp.display_vertical ) {
				set_visible_area( 5*8, 5*8 + WSWAN_Y_PIXELS - 1, 0, WSWAN_X_PIXELS - 1 );
			} else {
				set_visible_area( 0, WSWAN_X_PIXELS - 1, 5*8, 5*8 + WSWAN_Y_PIXELS - 1 );
			}
		}
	}
}

