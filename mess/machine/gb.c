/***************************************************************************

  gb.c

  Machine file to handle emulation of the Nintendo GameBoy.

  Changes:

	13/2/2002		AK - MBC2 and MBC3 support and added NVRAM support.
	23/2/2002		AK - MBC5 support, and MBC2 RAM support.
	13/3/2002		AK - Tidied up the MBC code, window layer now has it's
						 own palette. Tidied init code.
	15/3/2002		AK - More init code tidying with a slight hack to stop
						 sound when the machine starts.
	19/3/2002		AK - Changed NVRAM code to the new battery_* functions.
	24/3/2002		AK - Added MBC1 mode switching, and partial MBC3 RTC support.
	28/3/2002		AK - Improved LCD status timing and interrupts.
						 Free memory when we shutdown instead of leaking.
	31/3/2002		AK - Handle IO memory reading so we return 0xFF for registers
						 that are unsupported.
	 7/4/2002		AK - Free memory from battery load/save. General tidying.
	13/4/2002		AK - Ok, don't free memory when we shutdown as that causes
						 a crash on reset.
	28/4/2002		AK - General code tidying.
						 Fixed MBC3's RAM/RTC banking.
						 Added support for games with more than 128 ROM banks.
	12/6/2002		AK - Rewrote the way bg and sprite palettes are handled.
						 The window layer no longer has it's own palette.
						 Added Super GameBoy support.
	13/6/2002		AK - Added GameBoy Color support.

	17/5/2004       WP - Added Megaduck/Cougar Boy support.
	13/6/2005		WP - Added support for bootstrap rom banking.

***************************************************************************/
#define __MACHINE_GB_C

#include "driver.h"
#include "includes/gb.h"
#include "image.h"

#define MAX_ROMBANK 512
#define MAX_RAMBANK 256

static UINT16 MBCType;				   /* MBC type: 0 for none                        */
static UINT8 CartType;				   /* Cart Type (battery, ram, timer etc)         */
static UINT8 *ROMMap[MAX_ROMBANK];		   /* Addresses of ROM banks                      */
static UINT16 ROMBank;				   /* Number of ROM bank currently used           */
static UINT8 ROMMask;				   /* Mask for the ROM bank number                */
static UINT16 ROMBanks;				   /* Total number of ROM banks                   */
static UINT8 *RAMMap[MAX_RAMBANK];		   /* Addresses of RAM banks                      */
static UINT8 RAMBank;				   /* Number of RAM bank currently used           */
static UINT8 RAMMask;				   /* Mask for the RAM bank number                */
static UINT8 RAMBanks;				   /* Total number of RAM banks                   */
static UINT32 SIOCount;				   /* Serial I/O counter                          */
static UINT8 MBC1Mode;				   /* MBC1 ROM/RAM mode                           */
static UINT8 MBC3RTCMap[5];			   /* MBC3 Real-Time-Clock banks                  */
static UINT8 MBC3RTCBank;			   /* Number of RTC bank for MBC3                 */
static UINT8 *GBC_RAMMap[8];		   /* (GBC) Addresses of internal RAM banks       */
static UINT8 GBC_RAMBank;			   /* (GBC) Number of RAM bank currently used     */
UINT8 *GBC_VRAMMap[2];				   /* (GBC) Addressses of video RAM banks         */
UINT8 GBC_VRAMBank;					   /* (GBC) Number of video RAM bank currently used */
static UINT8 sgb_atf_data[4050];	   /* (SGB) Attribute files                       */
UINT8 *sgb_tile_data;
UINT8 *gb_cart = NULL;
UINT8 *gb_cart_ram = NULL;
UINT8 *gb_vram = NULL;				/* points to start of VRAM area */
UINT8 gb_io[0x10];
UINT8 gb_vid_regs[0x40];
UINT8 gb_ie;
UINT8 *gb_dummy_rom_bank = NULL;
UINT8 *gb_dummy_ram_bank = NULL;

static void gb_machine_stop(void);
void (*refresh_scanline)(void);

#ifdef MAME_DEBUG
/* #define V_GENERAL*/		/* Display general debug information */
/* #define V_BANK*/			/* Display bank switching debug information */
#endif

static void gb_init_regs(void)
{
	/* Initialize the registers */
	LCDSTAT = 0x00;
	CURLINE = CMPLINE = 0x00;
	IFLAGS = ISWITCH = 0x00;
	SIODATA = 0x00;
	SIOCONT = 0x7E;
	SCROLLX = SCROLLY = 0x00;
	WNDPOSX = WNDPOSY = 0x00;

	gb_io_w( 0x05, 0x00 );		/* TIMECNT */
	gb_io_w( 0x06, 0x00 );		/* TIMEMOD */
	gb_io_w( 0x07, 0x00 );		/* TIMEFRQ */
	gb_video_w( 0x0, 0x91 );	/* LCDCONT */
	gb_video_w( 0x7, 0xFC );	/* BGRDPAL */
	gb_video_w( 0x8, 0xFC );	/* SPR0PAL */
	gb_video_w( 0x9, 0xFC );	/* SPR1PAL */
}

static void gb_init(void)
{
	int ii;

        gb_vram = memory_get_read_ptr( 0, ADDRESS_SPACE_PROGRAM, 0x8000 );

	/* Initialize the memory banks */
	MBC1Mode = 0;
	MBC3RTCBank = 0;
	ROMBank = 1;
	RAMBank = 0;
	memory_set_bankptr (1, ROMMap[ROMBank] ? ROMMap[ROMBank] : gb_dummy_rom_bank);
	if ( MBCType != MEGADUCK ) {
		memory_set_bankptr (2, RAMMap[RAMBank] ? RAMMap[RAMBank] : gb_dummy_ram_bank);
	} else {
		memory_set_bankptr( 10, ROMMap[0] );
	}

	/* Initialize palette arrays */
	for( ii = 0; ii < 4; ii++ )
		gb_bpal[ii] = gb_spal0[ii] = gb_spal1[ii] = ii;

	/* Set handlers based on the Memory Bank Controller in the cart */
	switch( MBCType )
	{
		case NONE:
		case TAMA5:	/* Definitely wrong, but don't know how this one works */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MBC1:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );	/* We don't emulate RAM enable yet */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, gb_mem_mode_select_mbc1 );
			break;
		case MBC2:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc2 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, MWA8_ROM );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MBC3:
		case HUC1:	/* Possibly wrong */
		case HUC3:	/* Possibly wrong */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );	/* We don't emulate RAM enable yet */
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc3 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc3 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, gb_mem_mode_select_mbc3 );
			break;
		case MBC5:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0000, 0x1fff, 0, 0, gb_ram_enable );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x2000, 0x3fff, 0, 0, gb_rom_bank_select_mbc5 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x4000, 0x5fff, 0, 0, gb_ram_bank_select_mbc5 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x6000, 0x7fff, 0, 0, MWA8_ROM );
			break;
		case MEGADUCK:
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0x0001, 0x0001, 0, 0, megaduck_rom_bank_select_type1 );
			memory_install_write8_handler( 0, ADDRESS_SPACE_PROGRAM, 0xB000, 0xB000, 0, 0, megaduck_rom_bank_select_type2 );
			break;
	}

	gb_vid_regs[0x01] = 0x80;
	gb_sound_w( 0x16, 0x00 );       /* Initialize sound hardware */
}

MACHINE_RESET( gb )
{
        gb_init();

        /* Enable BIOS rom */
        memory_set_bankptr(5, memory_region(REGION_CPU1) );
        memory_set_bankptr(10, ROMMap[0] ? ROMMap[0] + 0x0100 : gb_dummy_rom_bank + 0x0100);

	/* set the scanline refresh function */
	refresh_scanline = gb_refresh_scanline;
}

MACHINE_RESET( sgb )
{
	gb_init();

	gb_init_regs();

	memory_set_bankptr(5, ROMMap[0] ? ROMMap[0] : gb_dummy_rom_bank );

	if ( sgb_tile_data == NULL ) {
		sgb_tile_data = auto_malloc( 0x2000 );
	}
	memset( sgb_tile_data, 0, 0x2000 );

	/* Initialize the Sound Registers */
	gb_sound_w(0x16,0xF0);	/* F0 for SGB */

	sgb_window_mask = 0;
	memset( sgb_pal_map, 0, 20*18 );
	memset( sgb_atf_data, 0, 4050 );

	/* HACKS for Donkey Kong Land 2 + 3.
	   For some reason that I haven't figured out, they store the tile
	   data differently.  Hacks will go once I figure it out */
	sgb_hack = 0;
	if( strncmp( (const char*)(gb_cart + 0x134), "DONKEYKONGLAND 2", 16 ) == 0 ||
		strncmp( (const char*)(gb_cart + 0x134), "DONKEYKONGLAND 3", 16 ) == 0 )
	{
		sgb_hack = 1;
	}

	/* set the scanline refresh function */
	refresh_scanline = sgb_refresh_scanline;
}

MACHINE_RESET( gbpocket )
{
	gb_init();

	gb_init_regs();

	/* Enable BIOS rom if we have one */
	memory_set_bankptr(5, ROMMap[0] ? ROMMap[0] : gb_dummy_rom_bank );
	memory_set_bankptr(10, ROMMap[0] ? ROMMap[0] + 0x0100 : gb_dummy_rom_bank + 0x0100);

	/* set the scanline refresh function */
	refresh_scanline = gb_refresh_scanline;
}

MACHINE_RESET( gbc )
{
	int ii;

	gb_init();

	gb_init_regs();

	memory_set_bankptr(5, ROMMap[0] ? ROMMap[0] : gb_dummy_rom_bank );

	/* Allocate memory for internal ram */
	for( ii = 0; ii < 8; ii++ )
	{
		if( (GBC_RAMMap[ii] = malloc (0x1000)) )
			memset (GBC_RAMMap[ii], 0, 0x1000);
		else
		{
			logerror("Error allocating memory\n");
		}
	}
	GBC_RAMBank = 0;
	memory_set_bankptr (3, GBC_RAMMap[GBC_RAMBank]);

	/* Allocate memory for video ram */
	for( ii = 0; ii < 2; ii++ )
	{
		if( (GBC_VRAMMap[ii] = malloc (0x2000)) )
		{
			memset (GBC_VRAMMap[ii], 0, 0x2000);
		}
		else
		{
			printf("Error allocating video memory\n");
		}
	}
	GBC_VRAMBank = 0;
	memory_set_bankptr (4, GBC_VRAMMap[GBC_VRAMBank]);

	gb_chrgen = GBC_VRAMMap[0];
	gbc_chrgen = GBC_VRAMMap[1];
	gb_bgdtab = gb_wndtab = GBC_VRAMMap[0] + 0x1C00;
	gbc_bgdtab = gbc_wndtab = GBC_VRAMMap[1] + 0x1C00;

	/* Initialise registers */
	gb_io_w( 0x6C, 0xFE );
	gb_io_w( 0x72, 0x00 );
	gb_io_w( 0x73, 0x00 );
	gb_io_w( 0x74, 0x8F );
	gb_io_w( 0x75, 0x00 );
	gb_io_w( 0x76, 0x00 );
	gb_io_w( 0x77, 0x00 );

	/* Are we in colour or mono mode? */
	if( gb_cart[0x143] == 0x80 || gb_cart[0x143] == 0xC0 )
		gbc_mode = GBC_MODE_GBC;
	else
		gbc_mode = GBC_MODE_MONO;

	/* HDMA disabled */
	gbc_hdma_enabled = 0;

	/* set the scanline refresh function */
	refresh_scanline = gbc_refresh_scanline;

	/*add_exit_callback(gb_machine_stop);*/
}

static void gb_machine_stop(void)
{
/*	int I; */

	/* Don't save if there was no battery */
	if( !(CartType & BATTERY) )
		return;

	/* NOTE: The reason we save the carts RAM this way instead of using MAME's
	   built in macros is because they force the filename to be the name of
	   the machine.  We need to have a separate name for each game. */
	image_battery_save(image_from_devtype_and_index(IO_CARTSLOT, 0), gb_cart_ram, RAMBanks * 0x2000 );

	/* FIXME: We should release memory here, but this function is called upon reset
	   and we don't reload the rom, so we're going to have to leak for now. */
/*	for( I = 0; I < RAMBanks; I++ )
	{
		free( RAMMap[I] );
	}
	for( I = 0; I < ROMBanks; I++ )
	{
		free( ROMMap[I] );
	}*/
}

WRITE8_HANDLER( gb_rom_bank_select_mbc1 )
{
	if( ROMMask )
	{
		data &= ROMMask;
		/* Selecting bank 0 == selecting bank 1 */
		if( data == 0 )
			data = 1;

		ROMBank = data & 0x1F; /* Only uses lower 5 bits */
		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank] );
	}
}

WRITE8_HANDLER( gb_rom_bank_select_mbc2 )
{
	if( ROMMask )
	{
		data &= ROMMask;
		/* Selecting bank 0 == selecting bank 1 */
		if( data == 0 )
			data = 1;

		/* The least significant bit of the upper address byte must be 1 */
		if( offset & 0x0100 )
			ROMBank = data;
		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank] );
	}
}

WRITE8_HANDLER( gb_rom_bank_select_mbc3 )
{
	if( ROMMask )
	{
		/* Selecting bank 0 == selecting bank 1 */
		if( data == 0 )
			data = 1;

		data &= ROMMask;

		ROMBank = data;
		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank] );
	}
}

WRITE8_HANDLER( gb_rom_bank_select_mbc5 )
{
	if( ROMMask )
	{
		data &= ROMMask;
		/* MBC5 has a 9 bit bank select */
		if( offset < 0x1000 )
		{
			ROMBank = (ROMBank & 0x100 ) | data;
		}
		else
		{
			ROMBank = (ROMBank & 0xFF ) | ((UINT16)(data & 0x1) << 8);
		}
		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank] );
	}
}

WRITE8_HANDLER( gb_ram_bank_select_mbc1 )
{
	if( RAMMask )
	{
		data &= RAMMask;
		data &= 0x3; /* Only uses the lower 2 bits */
		if( MBC1Mode )
		{
			/* Select the upper bits of the ROMMask */
			ROMBank |= data << 5;
			memory_set_bankptr (1, ROMMap[ROMBank] );
			return;
		}
		else
		{
			RAMBank = data;
		}
		/* Switch banks */
		memory_set_bankptr (2, RAMMap[RAMBank] );
	}
}

WRITE8_HANDLER( gb_ram_bank_select_mbc3 )
{
	if( RAMMask )
	{
		data &= RAMMask;
		if( data & 0x8 )	/* RTC banks */
		{
			MBC3RTCBank = (data & 0xf) - 8;
			memory_set_bankptr (2, &MBC3RTCMap[MBC3RTCBank]);
			return;
		}
		else	/* RAM banks */
		{
			RAMBank = data & 0x3;
		}
		/* Switch banks */
		memory_set_bankptr (2, RAMMap[RAMBank] );
	}
}

WRITE8_HANDLER( gb_ram_bank_select_mbc5 )
{
	if( RAMMask )
	{
		data &= RAMMask;
		if( CartType & RUMBLE )
		{
			data &= 0x7;
		}
		RAMBank = data;
		/* Switch banks */
		memory_set_bankptr (2, RAMMap[RAMBank] );
	}
}

WRITE8_HANDLER ( gb_ram_enable )
{
	/* FIXME: Currently we don't handle this, but a value of 0xA will enable
	 * writing to the cart's RAM banks */
}

WRITE8_HANDLER( gb_mem_mode_select_mbc1 )
{
	MBC1Mode = data & 0x1;
}

WRITE8_HANDLER( gb_mem_mode_select_mbc3 )
{
	if( CartType & TIMER )
	{
		/* FIXME: RTC Latch goes here */
	}
}

/* READ8_HANDLER( gb_echoram_r )
{
	return program_read_byte_8( 0xc000 + offset );
}

WRITE8_HANDLER( gb_echoram_w )
{
	program_write_byte_8( 0xc000 + offset, data );
}*/

WRITE8_HANDLER ( gb_io_w )
{
	static UINT8 timer_shifts[4] = {10, 4, 6, 8};

	switch (offset)
	{
	case 0x00:						/* JOYP - Joypad */
		JOYPAD = 0xCF | data;
		if (!(data & 0x20))
			JOYPAD &= (readinputport (0) >> 4) | 0xF0;
		if (!(data & 0x10))
			JOYPAD &= readinputport (0) | 0xF0;
		return;
	case 0x01:						/* SB - Serial transfer data */
		break;
	case 0x02:						/* SC - SIO control */
		if ((data & 0x81) == 0x81)		/* internal clock && enable */
		{
			SIODATA = 0xFF;
			SIOCount = 8;
		}
		else							/* external clock || disable */
			SIOCount = 0;
		break;
	case 0x04:						/* DIV - Divider register */
		gb_divcount = 0;
		return;
	case 0x05:						/* TIMA - Timer counter */
		gb_timer_count = data << gb_timer_shift;
		break;
	case 0x07:						/* TAC - Timer control */
		gb_timer_shift = timer_shifts[data & 0x03];
		data |= 0xF8;
		break;
	case 0x0F:						/* IF - Interrupt flag */
		data &= 0x1F;
		break;
	}

	gb_io [offset] = data;
}

WRITE8_HANDLER ( gb_bios_w )
{
	/* disable BIOS ROM */
	memory_set_bankptr(5, ROMMap[0]);
}

#ifdef MAME_DEBUG
static const char *sgbcmds[26] =
{
	"PAL01   ",
	"PAL23   ",
	"PAL03   ",
	"PAL12   ",
	"ATTR_BLK",
	"ATTR_LIN",
	"ATTR_DIV",
	"ATTR_CHR",
	"SOUND   ",
	"SOU_TRN ",
	"PAL_SET ",
	"PAL_TRN ",
	"ATRC_EN ",
	"TEST_EN ",
	"ICON_EN ",
	"DATA_SND",
	"DATA_TRN",
	"MLT_REG ",
	"JUMP    ",
	"CHR_TRN ",
	"PCT_TRN ",
	"ATTR_TRN",
	"ATTR_SET",
	"MASK_EN ",
	"OBJ_TRN ",
	"????????",
};
#endif

WRITE8_HANDLER ( sgb_io_w )
{
	static UINT8 sgb_bitcount = 0, sgb_bytecount = 0, sgb_start = 0, sgb_rest = 0;
	static UINT8 sgb_controller_no = 0, sgb_controller_mode = 0;
	static INT8 sgb_packets = -1;
	static UINT8 sgb_data[112];
	static UINT32 sgb_atf;

	offset += 0xFF00;

	switch( offset )
	{
		case 0xFF00:
			switch (data & 0x30)
			{
			case 0x00:				   /* start condition */
				if (sgb_start)
					logerror("SGB: Start condition before end of transfer ??");
				sgb_bitcount = 0;
				sgb_start = 1;
				sgb_rest = 0;
				JOYPAD = 0x0F & ((readinputport (0) >> 4) | readinputport (0) | 0xF0);
				break;
			case 0x10:				   /* data true */
				if (sgb_rest)
				{
					/* We should test for this case , but the code below won't
					   work with the current setup */
					/* if (sgb_bytecount == 16)
					{
						logerror("SGB: end of block is not zero!");
						sgb_start = 0;
					}*/
					sgb_data[sgb_bytecount] >>= 1;
					sgb_data[sgb_bytecount] |= 0x80;
					sgb_bitcount++;
					if (sgb_bitcount == 8)
					{
						sgb_bitcount = 0;
						sgb_bytecount++;
					}
					sgb_rest = 0;
				}
				JOYPAD = 0x1F & ((readinputport (0) >> 4) | 0xF0);
				break;
			case 0x20:				/* data false */
				if (sgb_rest)
				{
					if( sgb_bytecount == 16 && sgb_packets == -1 )
					{
#ifdef MAME_DEBUG
						logerror("SGB: %s (%02X) pkts: %d data: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
								sgbcmds[sgb_data[0] >> 3],sgb_data[0] >> 3, sgb_data[0] & 0x07, sgb_data[1], sgb_data[2], sgb_data[3],
								sgb_data[4], sgb_data[5], sgb_data[6], sgb_data[7],
								sgb_data[8], sgb_data[9], sgb_data[10], sgb_data[11],
								sgb_data[12], sgb_data[13], sgb_data[14], sgb_data[15]);
#endif
						sgb_packets = sgb_data[0] & 0x07;
						sgb_start = 0;
					}
					if (sgb_bytecount == (sgb_packets << 4) )
					{
						switch( sgb_data[0] >> 3 )
						{
							case 0x00:	/* PAL01 */
								Machine->remapped_colortable[0*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[0*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[0*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[0*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[1*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[1*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[1*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[1*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x01:	/* PAL23 */
								Machine->remapped_colortable[2*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[2*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[2*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[2*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[3*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[3*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[3*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[3*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x02:	/* PAL03 */
								Machine->remapped_colortable[0*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[0*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[0*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[0*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[3*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[3*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[3*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[3*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x03:	/* PAL12 */
								Machine->remapped_colortable[1*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[1*4 + 1] = sgb_data[3] | (sgb_data[4] << 8);
								Machine->remapped_colortable[1*4 + 2] = sgb_data[5] | (sgb_data[6] << 8);
								Machine->remapped_colortable[1*4 + 3] = sgb_data[7] | (sgb_data[8] << 8);
								Machine->remapped_colortable[2*4 + 0] = sgb_data[1] | (sgb_data[2] << 8);
								Machine->remapped_colortable[2*4 + 1] = sgb_data[9] | (sgb_data[10] << 8);
								Machine->remapped_colortable[2*4 + 2] = sgb_data[11] | (sgb_data[12] << 8);
								Machine->remapped_colortable[2*4 + 3] = sgb_data[13] | (sgb_data[14] << 8);
								break;
							case 0x04:	/* ATTR_BLK */
								{
									UINT8 I, J, K, o;
									for( K = 0; K < sgb_data[1]; K++ )
									{
										o = K * 6;
										if( sgb_data[o + 2] & 0x1 )
										{
											for( I = sgb_data[ o + 4]; I <= sgb_data[o + 6]; I++ )
											{
												for( J = sgb_data[o + 5]; J <= sgb_data[o + 7]; J++ )
												{
													sgb_pal_map[I][J] = sgb_data[o + 3] & 0x3;
												}
											}
										}
									}
								}
								break;
							case 0x05:	/* ATTR_LIN */
								{
									UINT8 J, K;
									if( sgb_data[1] > 15 )
										sgb_data[1] = 15;
									for( K = 0; K < sgb_data[1]; K++ )
									{
										if( sgb_data[K + 1] & 0x80 )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][sgb_data[K + 1] & 0x1f] = (sgb_data[K + 1] & 0x60) >> 5;
											}
										}
										else
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[sgb_data[K + 1] & 0x1f][J] = (sgb_data[K + 1] & 0x60) >> 5;
											}
										}
									}
								}
								break;
							case 0x06:	/* ATTR_DIV */
								{
									UINT8 I, J;
									if( sgb_data[1] & 0x40 ) /* Vertical */
									{
										for( I = 0; I < sgb_data[2]; I++ )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][I] = (sgb_data[1] & 0xC) >> 2;
											}
										}
										for( J = 0; J < 20; J++ )
										{
											sgb_pal_map[J][sgb_data[2]] = (sgb_data[1] & 0x30) >> 4;
										}
										for( I = sgb_data[2] + 1; I < 18; I++ )
										{
											for( J = 0; J < 20; J++ )
											{
												sgb_pal_map[J][I] = sgb_data[1] & 0x3;
											}
										}
									}
									else /* Horizontal */
									{
										for( I = 0; I < sgb_data[2]; I++ )
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[I][J] = (sgb_data[1] & 0xC) >> 2;
											}
										}
										for( J = 0; J < 18; J++ )
										{
											sgb_pal_map[sgb_data[2]][J] = (sgb_data[1] & 0x30) >> 4;
										}
										for( I = sgb_data[2] + 1; I < 20; I++ )
										{
											for( J = 0; J < 18; J++ )
											{
												sgb_pal_map[I][J] = sgb_data[1] & 0x3;
											}
										}
									}
								}
								break;
							case 0x07:	/* ATTR_CHR */
								{
									UINT16 I, sets;
									UINT8 x, y;
									sets = (sgb_data[3] | (sgb_data[4] << 8) );
									if( sets > 360 )
										sets = 360;
									sets >>= 2;
									sets += 6;
									x = sgb_data[1];
									y = sgb_data[2];
									if( sgb_data[5] ) /* Vertical */
									{
										for( I = 6; I < sets; I++ )
										{
											sgb_pal_map[x][y++] = (sgb_data[I] & 0xC0) >> 6;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = (sgb_data[I] & 0x30) >> 4;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = (sgb_data[I] & 0xC) >> 2;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}

											sgb_pal_map[x][y++] = sgb_data[I] & 0x3;
											if( y > 17 )
											{
												y = 0;
												x++;
												if( x > 19 )
													x = 0;
											}
										}
									}
									else /* horizontal */
									{
										for( I = 6; I < sets; I++ )
										{
											sgb_pal_map[x++][y] = (sgb_data[I] & 0xC0) >> 6;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = (sgb_data[I] & 0x30) >> 4;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = (sgb_data[I] & 0xC) >> 2;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}

											sgb_pal_map[x++][y] = sgb_data[I] & 0x3;
											if( x > 19 )
											{
												x = 0;
												y++;
												if( y > 17 )
													y = 0;
											}
										}
									}
								}
								break;
							case 0x08:	/* SOUND */
								/* This command enables internal sound effects */
								/* Not Implemented */
								break;
							case 0x09:	/* SOU_TRN */
								/* This command sends data to the SNES sound processor.
								   We'll need to emulate that for this to be used */
								/* Not Implemented */
								break;
							case 0x0A:	/* PAL_SET */
								{
									UINT16 index_, J, I;

									/* Palette 0 */
									index_ = (UINT16)(sgb_data[1] | (sgb_data[2] << 8)) * 4;
									Machine->remapped_colortable[0] = sgb_pal_data[index_];
									Machine->remapped_colortable[1] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[2] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[3] = sgb_pal_data[index_ + 3];
									/* Palette 1 */
									index_ = (UINT16)(sgb_data[3] | (sgb_data[4] << 8)) * 4;
									Machine->remapped_colortable[4] = sgb_pal_data[index_];
									Machine->remapped_colortable[5] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[6] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[7] = sgb_pal_data[index_ + 3];
									/* Palette 2 */
									index_ = (UINT16)(sgb_data[5] | (sgb_data[6] << 8)) * 4;
									Machine->remapped_colortable[8] = sgb_pal_data[index_];
									Machine->remapped_colortable[9] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[10] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[11] = sgb_pal_data[index_ + 3];
									/* Palette 3 */
									index_ = (UINT16)(sgb_data[7] | (sgb_data[8] << 8)) * 4;
									Machine->remapped_colortable[12] = sgb_pal_data[index_];
									Machine->remapped_colortable[13] = sgb_pal_data[index_ + 1];
									Machine->remapped_colortable[14] = sgb_pal_data[index_ + 2];
									Machine->remapped_colortable[15] = sgb_pal_data[index_ + 3];
									/* Attribute File */
									if( sgb_data[9] & 0x40 )
										sgb_window_mask = 0;
									sgb_atf = (sgb_data[9] & 0x3f) * (18 * 5);
									if( sgb_data[9] & 0x80 )
									{
										for( J = 0; J < 18; J++ )
										{
											for( I = 0; I < 5; I++ )
											{
												sgb_pal_map[I * 4][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC0) >> 6;
												sgb_pal_map[(I * 4) + 1][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0x30) >> 4;
												sgb_pal_map[(I * 4) + 2][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC) >> 2;
												sgb_pal_map[(I * 4) + 3][J] = sgb_atf_data[(J * 5) + sgb_atf + I] & 0x3;
											}
										}
									}
								}
								break;
							case 0x0B:	/* PAL_TRN */
								{
									UINT16 I, col;

									for( I = 0; I < 2048; I++ )
									{
										col = program_read_byte_8 (0x8800 + (I*2));
										col |= (UINT16)(program_read_byte_8 (0x8800 + (I*2) + 1)) << 8;
										sgb_pal_data[I] = col;
									}
								}
								break;
							case 0x0C:	/* ATRC_EN */
								/* Not Implemented */
								break;
							case 0x0D:	/* TEST_EN */
								/* Not Implemented */
								break;
							case 0x0E:	/* ICON_EN */
								/* Not Implemented */
								break;
							case 0x0F:	/* DATA_SND */
								/* Not Implemented */
								break;
							case 0x10:	/* DATA_TRN */
								/* Not Implemented */
								break;
							case 0x11:	/* MLT_REQ - Multi controller request */
								if (sgb_data[1] == 0x00)
									sgb_controller_mode = 0;
								else if (sgb_data[1] == 0x01)
									sgb_controller_mode = 2;
								break;
							case 0x12:	/* JUMP */
								/* Not Implemented */
								break;
							case 0x13:	/* CHR_TRN */
								if( sgb_data[1] & 0x1 )
									memcpy( sgb_tile_data + 4096, gb_vram + 0x0800, 4096 );
								else
									memcpy( sgb_tile_data, gb_vram + 0x0800, 4096 );
								break;
							case 0x14:	/* PCT_TRN */
								{
									int I;
									UINT16 col;
									if( sgb_hack )
									{
										memcpy( sgb_tile_map, gb_vram + 0x1000, 2048 );
										for( I = 0; I < 64; I++ )
										{
											col = program_read_byte_8 (0x8800 + (I*2));
											col |= (UINT16)(program_read_byte_8 (0x8800 + (I*2) + 1)) << 8;
											Machine->remapped_colortable[SGB_BORDER_PAL_OFFSET + I] = col;
										}
									}
									else /* Do things normally */
									{
										memcpy( sgb_tile_map, gb_vram + 0x0800, 2048 );
										for( I = 0; I < 64; I++ )
										{
											col = program_read_byte_8 (0x9000 + (I*2));
											col |= (UINT16)(program_read_byte_8 (0x9000 + (I*2) + 1)) << 8;
											Machine->remapped_colortable[SGB_BORDER_PAL_OFFSET + I] = col;
										}
									}
								}
								break;
							case 0x15:	/* ATTR_TRN */
								memcpy( sgb_atf_data, gb_vram + 0x0800, 4050 );
								break;
							case 0x16:	/* ATTR_SET */
								{
									UINT8 J, I;

									/* Attribute File */
									if( sgb_data[1] & 0x40 )
										sgb_window_mask = 0;
									sgb_atf = (sgb_data[1] & 0x3f) * (18 * 5);
									for( J = 0; J < 18; J++ )
									{
										for( I = 0; I < 5; I++ )
										{
											sgb_pal_map[I * 4][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC0) >> 6;
											sgb_pal_map[(I * 4) + 1][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0x30) >> 4;
											sgb_pal_map[(I * 4) + 2][J] = (sgb_atf_data[(J * 5) + sgb_atf + I] & 0xC) >> 2;
											sgb_pal_map[(I * 4) + 3][J] = sgb_atf_data[(J * 5) + sgb_atf + I] & 0x3;
										}
									}
								}
								break;
							case 0x17:	/* MASK_EN */
								sgb_window_mask = sgb_data[1];
								break;
							case 0x18:	/* OBJ_TRN */
								/* Not Implemnted */
								break;
							case 0x19:	/* ? */
								/* Called by: dkl,dkl2,dkl3,zeldadx
								   But I don't know what it is for. */
								/* Not Implemented */
								break;
							default:
								logerror( "\tSGB: Unknown Command!\n" );
						}

						sgb_start = 0;
						sgb_bytecount = 0;
						sgb_packets = -1;
					}
					if( sgb_start )
					{
						sgb_data[sgb_bytecount] >>= 1;
						sgb_bitcount++;
						if (sgb_bitcount == 8)
						{
							sgb_bitcount = 0;
							sgb_bytecount++;
						}
					}
					sgb_rest = 0;
				}
				JOYPAD = 0x2F & (readinputport (0) | 0xF0);
				break;
			case 0x30:				   /* rest condition */
				if (sgb_start)
					sgb_rest = 1;
				if (sgb_controller_mode)
				{
					sgb_controller_no++;
					if (sgb_controller_no == sgb_controller_mode)
						sgb_controller_no = 0;
					JOYPAD = 0x3F - sgb_controller_no;
				}
				else
					JOYPAD = 0x3F;
				break;
			}
			return;
		default:
			/* we didn't handle the write, so pass it to the GB handler */
			gb_io_w( offset - 0xFF00, data );
			return;
	}

	gb_io[offset] = data;
}

/* Interrupt Enable register */
READ8_HANDLER( gb_ie_r )
{
	return gb_ie;
}

WRITE8_HANDLER ( gb_ie_w )
{
	gb_ie = data & 0x1F;
}

/* IO read */
READ8_HANDLER ( gb_io_r )
{
	switch(offset)
	{
		case 0x04:
			return ((gb_divcount >> 8) & 0xFF);
		case 0x05:
			return (gb_timer_count >> gb_timer_shift);
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x06:
		case 0x07:
		case 0x0F:
			return gb_io[offset];
		default:
			/* It seems unsupported registers return 0xFF */
			return 0xFF;
	}
}

DEVICE_INIT(gb_cart)
{
	int I;

	if ( gb_dummy_rom_bank == NULL ) {
		gb_dummy_rom_bank = auto_malloc( 0x4000 );
		memset( gb_dummy_rom_bank, 0xFF, 0x4000 );
	}

	if ( gb_dummy_ram_bank == NULL ) {
		gb_dummy_ram_bank = auto_malloc( 0x2000 );
		memset( gb_dummy_ram_bank, 0x00, 0x2000 );
	}

	for(I = 0; I < MAX_ROMBANK; I++) {
		ROMMap[I] = NULL;
	}
	for(I = 0; I < MAX_RAMBANK; I++) {
		RAMMap[I] = NULL;
	}
	ROMBanks = 0;
	RAMBanks = 0;
	MBCType = NONE;
	CartType = 0;
	ROMMask = 0;
	return INIT_PASS;
}

DEVICE_LOAD(gb_cart)
{
	static const char *CartTypes[] =
	{
		"ROM ONLY",
		"ROM+MBC1",
		"ROM+MBC1+RAM",
		"ROM+MBC1+RAM+BATTERY",
        "UNKNOWN",
		"ROM+MBC2",
		"ROM+MBC2+BATTERY",
        "UNKNOWN",
		"ROM+RAM",
		"ROM+RAM+BATTERY",
        "UNKNOWN",
		"ROM+MMM01",
		"ROM+MMM01+SRAM",
		"ROM+MMM01+SRAM+BATTERY",
        "UNKNOWN",
		"ROM+MBC3+TIMER+BATTERY",
		"ROM+MBC3+TIMER+RAM+BATTERY",
		"ROM+MBC3",
		"ROM+MBC3+RAM",
		"ROM+MBC3+RAM+BATTERY",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
        "UNKNOWN",
		"ROM+MBC5",
		"ROM+MBC5+RAM",
		"ROM+MBC5+RAM+BATTERY",
		"ROM+MBC5+RUMBLE",
		"ROM+MBC5+RUMBLE+SRAM",
		"ROM+MBC5+RUMBLE+SRAM+BATTERY",
		"Pocket Camera",
		"Bandai TAMA5",
		/* Need heaps of unknowns here */
		"Hudson HuC-3",
		"Hudson HuC-1"
	};

/*** Following are some known manufacturer codes *************************/
	static struct
	{
		UINT16 Code;
		const char *Name;
	}
	Companies[] =
	{
		{0x3301, "Nintendo"},
		{0x7901, "Accolade"},
		{0xA400, "Konami"},
		{0x6701, "Ocean"},
		{0x5601, "LJN"},
		{0x9900, "ARC?"},
		{0x0101, "Nintendo"},
		{0x0801, "Capcom"},
		{0x0100, "Nintendo"},
		{0xBB01, "SunSoft"},
		{0xA401, "Konami"},
		{0xAF01, "Namcot?"},
		{0x4901, "Irem"},
		{0x9C01, "Imagineer"},
		{0xA600, "Kawada?"},
		{0xB101, "Nexoft"},
		{0x5101, "Acclaim"},
		{0x6001, "Titus"},
		{0xB601, "HAL"},
		{0x3300, "Nintendo"},
		{0x0B00, "Coconuts?"},
		{0x5401, "Gametek"},
		{0x7F01, "Kemco?"},
		{0xC001, "Taito"},
		{0xEB01, "Atlus"},
		{0xE800, "Asmik?"},
		{0xDA00, "Tomy?"},
		{0xB100, "ASCII?"},
		{0xEB00, "Atlus"},
		{0xC000, "Taito"},
		{0x9C00, "Imagineer"},
		{0xC201, "Kemco?"},
		{0xD101, "Sofel?"},
		{0x6101, "Virgin"},
		{0xBB00, "SunSoft"},
		{0xCE01, "FCI?"},
		{0xB400, "Enix?"},
		{0xBD01, "Imagesoft"},
		{0x0A01, "Jaleco?"},
		{0xDF00, "Altron?"},
		{0xA700, "Takara?"},
		{0xEE00, "IGS?"},
		{0x8300, "Lozc?"},
		{0x5001, "Absolute?"},
		{0xDD00, "NCS?"},
		{0xE500, "Epoch?"},
		{0xCB00, "VAP?"},
		{0x8C00, "Vic Tokai"},
		{0xC200, "Kemco?"},
		{0xBF00, "Sammy?"},
		{0x1800, "Hudson Soft"},
		{0xCA01, "Palcom/Ultra"},
		{0xCA00, "Palcom/Ultra"},
		{0xC500, "Data East?"},
		{0xA900, "Technos Japan?"},
		{0xD900, "Banpresto?"},
		{0x7201, "Broderbund?"},
		{0x7A01, "Triffix Entertainment?"},
		{0xE100, "Towachiki?"},
		{0x9300, "Tsuburava?"},
		{0xC600, "Tonkin House?"},
		{0xCE00, "Pony Canyon"},
		{0x7001, "Infogrames?"},
		{0x8B01, "Bullet-Proof Software?"},
		{0x5501, "Park Place?"},
		{0xEA00, "King Records?"},
		{0x5D01, "Tradewest?"},
		{0x6F01, "ElectroBrain?"},
		{0xAA01, "Broderbund?"},
		{0xC301, "SquareSoft"},
		{0x5201, "Activision?"},
		{0x5A01, "Bitmap Brothers/Mindscape"},
		{0x5301, "American Sammy"},
		{0x4701, "Spectrum Holobyte"},
		{0x1801, "Hudson Soft"},
		{0x0000, NULL}
	};

	int Checksum, I, J, filesize;
	UINT16 reported_rom_banks;
	int rambanks[5] = {0, 1, 1, 4, 16};

	for (I = 0; I < 256; I++)
		RAMMap[I] = ROMMap[I] = NULL;

	filesize = image_length(image);

	/* Check for presence of a header, and skip that header */
	J = filesize % 0x4000;
	if ( J == 512 ) {
		logerror( "Rom-header found, skipping\n" );
		mame_fseek( file, J, SEEK_SET );
		filesize -= 512;
	}

	/* Verify that the file contains 16kb blocks */
	if ( ( filesize == 0 ) || ( ( filesize % 0x4000 ) != 0 ) ) {
		logerror( "Error loading cartridge: Invalid rom file: %s.\n", image_filename(image) );
		return INIT_FAIL;
	}

	/* Claim memory */
	gb_cart = auto_malloc( filesize );

	/* Read cartridge */
	if ( mame_fread( file, gb_cart, filesize ) != filesize ) {
		logerror( "Error loading cartridge: Unable to read from file: %s.\n", image_filename(image) );
		return INIT_FAIL;
	}

	/* Fill in our cart details */
	switch( gb_cart[0x0147] )
	{
		case 0x00:
			MBCType = NONE;
			CartType = 0;
			break;
		case 0x01:
			MBCType = MBC1;
			CartType = 0;
			break;
		case 0x02:
			MBCType = MBC1;
			CartType = RAM;
			break;
		case 0x03:
			MBCType = MBC1;
			CartType = RAM | BATTERY;
			break;
		case 0x05:
			MBCType = MBC2;
			CartType = 0;
			break;
		case 0x06:
			MBCType = MBC2;
			CartType = BATTERY;
			break;
		case 0x08:
			MBCType = NONE;
			CartType = RAM;
		case 0x09:
			MBCType = NONE;
			CartType = RAM | BATTERY;
			break;
		case 0x0F:
			MBCType = MBC3;
			CartType = TIMER | BATTERY;
			break;
		case 0x10:
			MBCType = MBC3;
			CartType = TIMER | RAM | BATTERY;
			break;
		case 0x11:
			MBCType = MBC3;
			CartType = 0;
			break;
		case 0x12:
			MBCType = MBC3;
			CartType = RAM;
			break;
		case 0x13:
			MBCType = MBC3;
			CartType = RAM | BATTERY;
			break;
		case 0x19:
			MBCType = MBC5;
			CartType = 0;
			break;
		case 0x1A:
			MBCType = MBC5;
			CartType = RAM;
			break;
		case 0x1B:
			MBCType = MBC5;
			CartType = RAM | BATTERY;
			break;
		case 0x1C:
			MBCType = MBC5;
			CartType = RUMBLE;
			break;
		case 0x1D:
			MBCType = MBC5;
			CartType = RUMBLE | SRAM;
			break;
		case 0x1E:
			MBCType = MBC5;
			CartType = RUMBLE | SRAM | BATTERY;
			break;
		case 0xFE:
			MBCType = HUC3;
			CartType = 0;
			break;
		case 0xFF:
			MBCType = HUC1;
			CartType = 0;
			break;
		default:
			MBCType = NONE;
			CartType = UNKNOWN;
	}

	if ( CartType & UNKNOWN )
	{
		logerror("Error loading cartridge: Unknown ROM type.\n");
		return INIT_FAIL;
	}

	ROMBanks = filesize / 0x4000;
	switch( gb_cart[0x0148] ) {
	case 0x52:
		reported_rom_banks = 72;
		break;
	case 0x53:
		reported_rom_banks = 80;
		break;
	case 0x54:
		reported_rom_banks = 96;
		break;
	case 0x00: case 0x01: case 0x02: case 0x03:
	case 0x04: case 0x05: case 0x06: case 0x07:
		reported_rom_banks = 2 << gb_cart[0x0148];
		break;
	default:
		logerror( "Warning loading cartridge: Unknown ROM size in header.\n" );
		reported_rom_banks = 256;
		break;
	}
	if ( ROMBanks != reported_rom_banks ) {
		logerror( "Warning loading cartridge: Filesize and reported ROM banks don't match.\n" );
	}

        RAMBanks = rambanks[gb_cart[0x0149] & 3];

	/* Calculate and check checksum */
        Checksum = ((UINT16) gb_cart[0x014E] << 8) + gb_cart[0x014F];
        Checksum += gb_cart[0x014E] + gb_cart[0x014F];
        for (I = 0; I < filesize; I++)
        {
                Checksum -= gb_cart[I];
        }
        if (Checksum & 0xFFFF)
        {
                logerror("Warning loading cartridge: Checksum is wrong.");
        }

	/* Initialize ROMMap pointers */
        for (I = 0; I < ROMBanks; I++)
        {
                ROMMap[I] = gb_cart + ( I * 0x4000 );
        }

	/* Log cart information */
	{
		const char *P;
		char S[50];

		strncpy (S, (char *)&gb_cart[0x0134], 16);
		S[16] = '\0';
		logerror("Cart Information\n");
		logerror("\tName:             %s\n", S);
		logerror("\tType:             %s [0x%2X]\n", CartTypes[gb_cart[0x0147]], gb_cart[0x0147] );
		logerror("\tGameBoy:          %s\n", (gb_cart[0x0143] == 0xc0) ? "No" : "Yes" );
		logerror("\tSuper GB:         %s [0x%2X]\n", (gb_cart[0x0146] == 0x03) ? "Yes" : "No", gb_cart[0x0146] );
		logerror("\tColor GB:         %s [0x%2X]\n", (gb_cart[0x0143] == 0x80 || gb_cart[0x0143] == 0xc0) ? "Yes" : "No", gb_cart[0x0143] );
		logerror("\tROM Size:         %d 16kB Banks [0x%2X]\n", ROMBanks, gb_cart[0x0148]);
		J = (gb_cart[0x0149] & 0x03) * 2;
		J = J ? (1 << (J - 1)) : 0;
		logerror("\tRAM Size:         %d kB [0x%2X]\n", J, gb_cart[0x0149]);
		logerror("\tLicense code:     0x%2X%2X\n", gb_cart[0x0145], gb_cart[0x0144] );
		J = ((UINT16) gb_cart[0x014B] << 8) + gb_cart[0x014A];
		for (I = 0, P = NULL; !P && Companies[I].Name; I++)
			if (J == Companies[I].Code)
				P = Companies[I].Name;
		logerror("\tManufacturer ID:  0x%2X", J);
		logerror(" [%s]\n", P ? P : "?");
		logerror("\tVersion Number:   0x%2X\n", gb_cart[0x014C]);
		logerror("\tComplement Check: 0x%2X\n", gb_cart[0x014D]);
		logerror("\tChecksum:         0x%2X\n", ( ( gb_cart[0x014E] << 8 ) + gb_cart[0x014F] ) );
		J = ((UINT16) gb_cart[0x0103] << 8) + gb_cart[0x0102];
		logerror("\tStart Address:    0x%2X\n", J);
	}

	/* MBC2 has 512 * 4bits (8kb) internal RAM */
	if( MBCType == MBC2 )
		RAMBanks = 1;

	if (RAMBanks && MBCType)
	{
		/* Release any previously claimed memory */
		if ( gb_cart_ram != NULL ) {
			free( gb_cart_ram );
			gb_cart_ram = NULL;
		}
		/* Claim memory */
		gb_cart_ram = auto_malloc( RAMBanks * 0x2000 );

		for (I = 0; I < RAMBanks; I++)
		{
			RAMMap[I] = gb_cart_ram + ( I * 0x2000 );
		}
	}

	/* Load the saved RAM if this cart has a battery */
	if( CartType & BATTERY )
	{
		image_battery_load( image, gb_cart_ram, RAMBanks * 0x2000 );
	}

	/* Build rom bank Mask */
	if ( ! reported_rom_banks )
		ROMMask = 0;
	else
	{
		for (I = 1; I < ROMBanks; I <<= 1) ;
		ROMMask = I - 1;
	}
	/* Build ram bank Mask */
	if (!RAMBanks)
		RAMMask = 0;
	else
	{
		for (I = 1; I < RAMBanks; I <<= 1) ;
		RAMMask = I - 1;
	}

	return INIT_PASS;
}

void gb_scanline_interrupt (void)
{
	/* This is a little dodgy, but it works... mostly */
	static UINT8 count = 0;
	count = (count + 1) % 3;
	if ( count )
		return;

	/* First let's draw the current scanline */
	if (CURLINE < 144)
		refresh_scanline ();

	/* The rest only makes sense if the display is enabled */
	if (LCDCONT & 0x80)
	{
		if (CURLINE < 144)
		{
			/* Set Mode 2 lcdstate */
			LCDSTAT = (LCDSTAT & 0xFC) | 0x02;
			/* Generate lcd interrupt if requested */
			if (LCDSTAT & 0x20)
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);

			/* First  lcdstate change after aprox 19 uS */
			timer_set (19.0 / 1000000.0, 0, gb_scanline_interrupt_set_mode3);
			/* Second lcdstate change after aprox 60 uS */
			timer_set (60.0 / 1000000.0, 0, gb_scanline_interrupt_set_mode0);
		}
		else
		{
			/* Generate VBlank interrupt */
			if (CURLINE == 144)
			{
				/* Cause VBlank interrupt */
				cpunum_set_input_line(0, VBL_INT, HOLD_LINE);
				/* Set VBlank lcdstate */
				LCDSTAT = (LCDSTAT & 0xFC) | 0x01;
				/* Generate lcd interrupt if requested */
				if( LCDSTAT & 0x10 )
					cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
			}
		}
		CURLINE = (CURLINE + 1) % 154;

		if ( CURLINE == CMPLINE )
		{
			LCDSTAT |= 0x04;
			/* Generate lcd interrupt if requested */
			if ( LCDSTAT & 0x40 )
				cpunum_set_input_line(0, LCD_INT, HOLD_LINE);
		}
		else
			LCDSTAT &= 0xFB;
	}

	/* Generate serial IO interrupt */
	if (SIOCount)
	{
		SIODATA = (SIODATA << 1) | 0x01;
		if (!--SIOCount)
		{
			SIOCONT &= 0x7F;
			cpunum_set_input_line(0, SIO_INT, HOLD_LINE);
		}
	}
}

void gb_scanline_interrupt_set_mode0 (int param)
{
	/* only perform mode changes when LCD controller is still on */
	if (LCDCONT & 0x80)
	{
		/* Set Mode 0 lcdstate */
		LCDSTAT &= 0xFC;
		/* Generate lcd interrupt if requested */
		if( LCDSTAT & 0x08 )
			cpunum_set_input_line(0, LCD_INT, HOLD_LINE);

		/* Check for HBLANK DMA */
		if( gbc_hdma_enabled && (CURLINE < 144) )
			gbc_hdma(0x10);
	}
}

void gb_scanline_interrupt_set_mode3 (int param)
{
	/* only perform mode changes when LCD controller is still on */
	if (LCDCONT & 0x80)
	{
		/* Set Mode 3 lcdstate */
		LCDSTAT = (LCDSTAT & 0xFC) | 0x03;
	}
}

void gbc_hdma(UINT16 length)
{
	UINT16 src, dst;

	src = ((UINT16)HDMA1 << 8) | (HDMA2 & 0xF0);
	dst = ((UINT16)(HDMA3 & 0x1F) << 8) | (HDMA4 & 0xF0);
	dst |= 0x8000;
	while( length > 0 )
	{
		program_write_byte_8( dst++, program_read_byte_8( src++ ) );
		length--;
	}
	HDMA1 = src >> 8;
	HDMA2 = src & 0xF0;
	HDMA3 = 0x1f & (dst >> 8);
	HDMA4 = dst & 0xF0;
	HDMA5--;
	if( (HDMA5 & 0x7f) == 0 )
	{
		HDMA5 = 0xff;
		gbc_hdma_enabled = 0;
	}
}

READ8_HANDLER( gb_video_r )
{
	switch( offset ) {
	case 0x06:
		return 0xFF;
	case 0x36:
	case 0x37:
		return 0;
	default:
		return gb_vid_regs[offset];
	}
}

WRITE8_HANDLER ( gb_video_w )
{
	switch (offset)
	{
	case 0x00:						/* LCDC - LCD Control */
		gb_chrgen = gb_vram + ((data & 0x10) ? 0x0000 : 0x0800);
		gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
		gb_bgdtab = gb_vram + ((data & 0x08) ? 0x1C00 : 0x1800 );
		gb_wndtab = gb_vram + ((data * 0x40) ? 0x1C00 : 0x1800 );
		/* if LCD controller is switched off, set STAT to 00 */
		if ( ! ( data & 0x80 ) ) {
			LCDSTAT &= ~0x03;
		}
		break;
	case 0x01:						/* STAT - LCD Status */
		data = (data & 0xF8) | (LCDSTAT & 0x07);
		break;
	case 0x04:						/* LY - LCD Y-coordinate */
		data = 0;
		break;
	case 0x06:						/* DMA - DMA Transfer and Start Address */
		{
			UINT8 *P = memory_get_write_ptr(0, ADDRESS_SPACE_PROGRAM, 0xFE00 );
			offset = (UINT16) data << 8;
			for (data = 0; data < 0xA0; data++)
				*P++ = program_read_byte_8 (offset++);
		}
		return;
	case 0x07:						/* BGP - Background Palette */
		gb_bpal[0] = data & 0x3;
		gb_bpal[1] = (data & 0xC) >> 2;
		gb_bpal[2] = (data & 0x30) >> 4;
		gb_bpal[3] = (data & 0xC0) >> 6;
		break;
	case 0x08:						/* OBP0 - Object Palette 0 */
		gb_spal0[0] = data & 0x3;
		gb_spal0[1] = (data & 0xC) >> 2;
		gb_spal0[2] = (data & 0x30) >> 4;
		gb_spal0[3] = (data & 0xC0) >> 6;
		break;
	case 0x09:						/* OBP1 - Object Palette 1 */
		gb_spal1[0] = data & 0x3;
		gb_spal1[1] = (data & 0xC) >> 2;
		gb_spal1[2] = (data & 0x30) >> 4;
		gb_spal1[3] = (data & 0xC0) >> 6;
		break;
	}
	gb_vid_regs[ offset ] = data;
}

WRITE8_HANDLER ( gbc_video_w )
{
	static const UINT16 gbc_to_gb_pal[4] = {32767, 21140, 10570, 0};
	static UINT16 BP = 0, OP = 0;

	switch( offset )
	{
		case 0x00:	/* LCDC - LCD Control */
			gb_chrgen = GBC_VRAMMap[0] + ((data & 0x10) ? 0x0000 : 0x0800);
			gbc_chrgen = GBC_VRAMMap[1] + ((data & 0x10) ? 0x0000 : 0x0800);
			gb_tile_no_mod = (data & 0x10) ? 0x00 : 0x80;
			gb_bgdtab = GBC_VRAMMap[0] + ((data & 0x08) ? 0x1C00 : 0x1800);
			gbc_bgdtab = GBC_VRAMMap[1] + ((data & 0x08) ? 0x1C00 : 0x1800);
			gb_wndtab = GBC_VRAMMap[0] + ((data & 0x40) ? 0x1C00 : 0x1800);
			gbc_wndtab = GBC_VRAMMap[1] + ((data & 0x40) ? 0x1C00 : 0x1800);
			/* if LCD controller is switched off, set STAT to 00 */
			if ( ! ( data & 0x80 ) ) {
				LCDSTAT &= ~0x03;
			}
			break;
		case 0x07:	/* BGP - GB background palette */
			if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
			{
				Machine->remapped_colortable[0] = gbc_to_gb_pal[(data & 0x03)];
				Machine->remapped_colortable[1] = gbc_to_gb_pal[(data & 0x0C) >> 2];
				Machine->remapped_colortable[2] = gbc_to_gb_pal[(data & 0x30) >> 4];
				Machine->remapped_colortable[3] = gbc_to_gb_pal[(data & 0xC0) >> 6];
			}
			break;
		case 0x08:	/* OBP0 - GB Object 0 palette */
			if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
			{
				Machine->remapped_colortable[4] = gbc_to_gb_pal[(data & 0x03)];
				Machine->remapped_colortable[5] = gbc_to_gb_pal[(data & 0x0C) >> 2];
				Machine->remapped_colortable[6] = gbc_to_gb_pal[(data & 0x30) >> 4];
				Machine->remapped_colortable[7] = gbc_to_gb_pal[(data & 0xC0) >> 6];
			}
			break;
		case 0x09:	/* OBP1 - GB Object 1 palette */
			if( gbc_mode == GBC_MODE_MONO ) /* Some GBC games are lazy and still call this */
			{
				Machine->remapped_colortable[8] = gbc_to_gb_pal[(data & 0x03)];
				Machine->remapped_colortable[9] = gbc_to_gb_pal[(data & 0x0C) >> 2];
				Machine->remapped_colortable[10] = gbc_to_gb_pal[(data & 0x30) >> 4];
				Machine->remapped_colortable[11] = gbc_to_gb_pal[(data & 0xC0) >> 6];
			}
			break;
		case 0x0D:	/* KEY1 - Prepare speed switch */
			if( data & 0x1 )
			{
				data = (gb_vid_regs[offset] & 0x80)?0x00:0x80;
/*				cpunum_set_clockscale( 0, (data & 0x80)?2.0:1.0 );*/
#ifdef V_GENERAL
				logerror( "Switched to %s mode.\n", (data & 0x80) ? "FAST":"NORMAL" );
#endif /* V_GENERAL */
			}
			break;
		case 0x0F:	/* VBK - VRAM bank select */
			GBC_VRAMBank = data & 0x1;
			memory_set_bankptr (4, GBC_VRAMMap[GBC_VRAMBank]);
			data |= 0xFE;
			break;
		case 0x11:	/* HDMA1 - HBL General DMA - Source High */
			break;
		case 0x12:	/* HDMA2 - HBL General DMA - Source Low */
			data &= 0xF0;
			break;
		case 0x13:	/* HDMA3 - HBL General DMA - Destination High */
			data &= 0x1F;
			break;
		case 0x14:	/* HDMA4 - HBL General DMA - Destination Low */
			data &= 0xF0;
			break;
		case 0x15:	/* HDMA5 - HBL General DMA - Mode, Length */
			if( !(data & 0x80) )
			{
				if( gbc_hdma_enabled )
				{
					gbc_hdma_enabled = 0;
					data = HDMA5 & 0x80;
				}
				else
				{
					/* General DMA */
					gbc_hdma( ((data & 0x7F) + 1) * 0x10 );
					lcd_time -= ((KEY1 & 0x80)?110:220) + (((data & 0x7F) + 1) * 7.68);
					data = 0xff;
				}
			}
			else
			{
				/* H-Blank DMA */
				gbc_hdma_enabled = 1;
				data &= 0x7f;
			}
			break;
		case 0x16:	/* RP - Infrared port */
		case 0x28:	/* BCPS - Background palette specification */
			break;
		case 0x29:	/* BCPD - background palette data */
			if( GBCBCPS & 0x1 )
				Machine->remapped_colortable[(GBCBCPS & 0x3e) >> 1] = ((UINT16)(data & 0x7f) << 8) | BP;
			else
				BP = data;
			if( GBCBCPS & 0x80 )
			{
				GBCBCPS++;
				GBCBCPS &= 0xBF;
			}
			break;
		case 0x2A:	/* OCPS - Object palette specification */
			break;
		case 0x2B:	/* OCPD - Object palette data */
			if( GBCOCPS & 0x1 )
				Machine->remapped_colortable[GBC_PAL_OBJ_OFFSET + ((GBCOCPS & 0x3e) >> 1)] = ((UINT16)(data & 0x7f) << 8) | OP;
			else
				OP = data;
			if( GBCOCPS & 0x80 )
			{
				GBCOCPS++;
				GBCOCPS &= 0xBF;
			}
			break;
		case 0x30:	/* SVBK - RAM bank select */
			GBC_RAMBank = data & 0x7;
			memory_set_bankptr (3, GBC_RAMMap[GBC_RAMBank]);
			break;
		/* Undocumented registers */
		case 0x2C:
			/* bit 0 can be read/written */
			logerror( "Write to undoco'ed register: %X = %X\n", offset, data );
			data = 0xfe | (data & 0x01);
			break;
		case 0x32:
		case 0x33:
		case 0x34:
			/* whole byte can be read/written */
			logerror( "Write to undoco'ed register: %X = %X\n", offset, data );
			break;
		case 0x35:
			/* bit 4-6 can be read/written */
			logerror( "Write to undoco'ed register: %X = %X\n", offset, data );
			data = 0x8f | (data & 0x70);
			break;
		case 0x36:
		case 0x37:
			logerror( "Write to undoco'ed register: %X = %X\n", offset, data );
			return;
		default:
			/* we didn't handle the write, so pass it to the GB handler */
			gb_video_w( offset, data );
			return;
	}

	gb_vid_regs[offset] = data;
}

/****************************************************************************

  Megaduck routines

 ****************************************************************************/

MACHINE_RESET( megaduck )
{
	/* We may have to add some more stuff here, if not then it can be merged back into gb */
	gb_init();

	/* set the scanline refresh function */
	refresh_scanline = gb_refresh_scanline;
}

/* Map megaduck video related area on to regular Gameboy video area */
/* Swap two bits, if they're set */

/**************
 Different LCDC register

 GameBoy	MegaDuck
 0			0	- BG & Window Display : 0 - Off, 1 - On
 1			6	- OBJ Display: 0 - Off, 1 - On
 2			1	- OBJ Size: 0 - 8x8, 1 - 8x16
 3			2	- BG Tile Map Display: 0 - 9800, 1 - 9C00
 4			4	- BG & Window Tile Data Select: 0 - 8800, 1 - 8000
 5			5	- Window Display: 0 - Off, 1 - On
 6			3	- Window Tile Map Display Select: 0 - 9800, 1 - 9C00
 7			7	- LCD Operation
 **************/

 READ8_HANDLER( megaduck_video_r )
{
	UINT8 data;

	if ( (offset & 0x0C) && ((offset & 0x0C) ^ 0x0C) ) {
		offset ^= 0x0C;
	}
	if ( offset )
		return gb_vid_regs[offset];
	data = gb_vid_regs[offset];
	return (data&0xB1) | ((data&0x40)>>3) | ((data&0x0C)>>1) | ((data&0x02)<<5);
}

WRITE8_HANDLER ( megaduck_video_w )
{
	if ( !offset ) {
		data = (data&0xB1) | ((data&0x08)<<3) | ((data&0x06)<<1) | ((data&0x40)>>5);
	}
	if ( (offset & 0x0C) && ((offset & 0x0C) ^ 0x0C) ) {
		offset ^= 0x0C;
	}
	gb_video_w(offset, data );
}

WRITE8_HANDLER( megaduck_sound_w1 )
{
	switch(offset) {
		case 0x00:	gb_sound_w( 0, data );	break;
		case 0x01:	gb_sound_w( 2, data );	break;
		case 0x02:	gb_sound_w( 1, data );	break;
		case 0x03:	gb_sound_w( 3, data );	break;
		case 0x04:	gb_sound_w( 4, data );	break;
		case 0x05:	gb_sound_w( 6, data );	break;
		case 0x07:	gb_sound_w( 7, data );	break;
		case 0x08:	gb_sound_w( 8, data );	break;
		case 0x09:	gb_sound_w( 9, data );	break;
		case 0x06:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			break;
	}
}

static UINT8 megaduck_sound_offsets[16] = { 0, 2, 1, 3, 4, 6, 5, 7, 8, 9, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

/* this one needs some work */
 READ8_HANDLER( megaduck_sound_r1 )
{
	return gb_sound_r( megaduck_sound_offsets[offset] );
}

WRITE8_HANDLER( megaduck_sound_w2 )
{
	switch(offset) {
		case 0x00:	gb_sound_w( 0x10, data );	break;
		case 0x01:	gb_sound_w( 0x12, data );	break;
		case 0x02:	gb_sound_w( 0x11, data );	break;
		case 0x03:	gb_sound_w( 0x13, data );	break;
		case 0x04:	gb_sound_w( 0x14, data );	break;
		case 0x05:	gb_sound_w( 0x16, data );	break;
		case 0x06:	gb_sound_w( 0x15, data );	break;
		case 0x07:
		case 0x08:
		case 0x09:
		case 0x0A:
		case 0x0B:
		case 0x0C:
		case 0x0D:
		case 0x0E:
		case 0x0F:
			break;
	}
}

READ8_HANDLER( megaduck_sound_r2 )
{
	return gb_sound_r(0x10 + megaduck_sound_offsets[offset]);
}

WRITE8_HANDLER( megaduck_rom_bank_select_type1 )
{
	if( ROMMask )
	{
		ROMBank = data & ROMMask;

		/* Switch banks */
		memory_set_bankptr (1, ROMMap[ROMBank]);
	}
}

WRITE8_HANDLER( megaduck_rom_bank_select_type2 )
{
	if( ROMMask )
	{
		ROMBank = (data << 1) & ROMMask;

		/* Switch banks */
		memory_set_bankptr( 10, ROMMap[ROMBank]);
		memory_set_bankptr( 1, ROMMap[ROMBank + 1]);
	}
}

DEVICE_LOAD(megaduck_cart)
{
	int I, filesize;

	for (I = 0; I < MAX_ROMBANK; I++)
		ROMMap[I] = NULL;
	for (I = 0; I < MAX_RAMBANK; I++)
		RAMMap[I] = NULL;

	filesize = image_length(image);
	ROMBanks = filesize / 0x4000;

	if ( ( filesize == 0 ) || ( ( filesize % 0x4000 ) != 0 ) ) {
		logerror( "Error loading cartridge: Invalid rom file: %s.\n", image_filename(image) );
		return INIT_FAIL;
	}

	/* Claim memory */
	gb_cart = auto_malloc( filesize );

	/* Read cartridge */
	if (mame_fread (file, gb_cart, filesize) != filesize) {
		logerror("Error loading cartridge: Unable to read from file: %s.\n", image_filename(image));
		return INIT_FAIL;
	}

	/* Log cart information */
	{
		logerror("Cart Information\n");
		logerror("\tRom Banks:        %d\n", ROMBanks);
	}

	for (I = 0; I < ROMBanks; I++)
	{
		ROMMap[I] = gb_cart + ( I * 0x4000 );
	}

	/* Build rom bank Mask */
	if (ROMBanks < 3)
		ROMMask = 0;
	else
	{
		for (I = 1; I < ROMBanks; I <<= 1) ;
		ROMMask = I - 1;
	}

	MBCType = MEGADUCK;

	return INIT_PASS;
}
