#ifndef __GB_H
#define __GB_H

#include "mame.h"
#include "sound/custom.h"

void gameboy_sound_w(int offset, int data);

#ifdef __MACHINE_GB_C
#define EXTERN
#else
#define EXTERN extern
#endif

/* Interrupt flags */
#define VBL_IFLAG	0x01	/* V-Blank    */
#define LCD_IFLAG	0x02	/* LCD Status */
#define TIM_IFLAG	0x04	/* Timer      */
#define SIO_IFLAG	0x08	/* Serial I/O */
#define EXT_IFLAG	0x10	/* Joypad     */

/* Interrupts */
#define VBL_INT		0		/* V-Blank    */
#define LCD_INT		1		/* LCD Status */
#define TIM_INT		2		/* Timer      */
#define SIO_INT		3		/* Serial I/O */
#define EXT_INT		4		/* Joypad     */

/* Memory bank controller types */
#define NONE		0x000	/*  32KB ROM - No memory bank controller         */
#define MBC1		0x001	/*  ~2MB ROM,   8KB RAM -or- 512KB ROM, 32KB RAM */
#define MBC2		0x002	/* 256KB ROM,  32KB RAM                          */
#define MBC3		0x003	/*   2MB ROM,  32KB RAM, RTC                     */
#define MBC5		0x004	/*   8MB ROM, 128KB RAM (32KB w/ Rumble)         */
#define TAMA5		0x005	/*    ?? ROM     ?? RAM - What is this?          */
#define HUC1		0x006	/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
#define HUC3		0x007	/*    ?? ROM,    ?? RAM - Hudson Soft Controller */
#define MEGADUCK	0x100	/* MEGADUCK style banking, using 0x100 to not    */
                            /* interfere with possible GBx banking methods   */
#ifdef TIMER
#undef TIMER
#endif

/* Cartridge types */
#define RAM			0x01	/* Cartridge has RAM                             */
#define BATTERY		0x02	/* Cartridge has a battery to save RAM           */
#define TIMER		0x04	/* Cartridge has a real-time-clock (MBC3 only)   */
#define RUMBLE		0x08	/* Cartridge has a rumble motor (MBC5 only)      */
#define SRAM		0x10	/* Cartridge has SRAM                            */
#define UNKNOWN		0x80	/* Cartridge is of an unknown type               */

extern UINT8 gb_ie;
extern UINT8 gb_io[];
#define JOYPAD  gb_io[0x00] /* Joystick: 1.1.P15.P14.P13.P12.P11.P10		 */
#define SIODATA gb_io[0x01] /* Serial IO data buffer 					 */
#define SIOCONT gb_io[0x02] /* Serial IO control register				 */
#define DIVREG  gb_io[0x04] /* Divider register (???)					 */
#define TIMECNT gb_io[0x05] /* Timer counter. Gen. int. when it overflows */
#define TIMEMOD gb_io[0x06] /* New value of TimeCount after it overflows  */
#define TIMEFRQ gb_io[0x07] /* Timer frequency and start/stop switch 	 */
#define IFLAGS  gb_io[0x0F] /* Interrupt flags: 0.0.0.JST.SIO.TIM.LCD.VBL */
#define ISWITCH gb_ie /* Switches to enable/disable interrupts 	 */

extern UINT8 gb_vid_regs[];
#define LCDCONT gb_vid_regs[0x0] /* LCD control register						 */
#define LCDSTAT gb_vid_regs[0x1] /* LCD status register						 */
#define SCROLLY gb_vid_regs[0x2] /* Starting Y position of the background 	 */
#define SCROLLX gb_vid_regs[0x3] /* Starting X position of the background 	 */
#define CURLINE gb_vid_regs[0x4] /* Current screen line being scanned 		 */
#define CMPLINE gb_vid_regs[0x5] /* Gen. int. when scan reaches this line 	 */
#define BGRDPAL gb_vid_regs[0x7] /* Background palette						 */
#define SPR0PAL gb_vid_regs[0x8] /* Sprite palette #0 						 */
#define SPR1PAL gb_vid_regs[0x9] /* Sprite palette #1 						 */
#define WNDPOSY gb_vid_regs[0xA] /* Window Y position 						 */
#define WNDPOSX gb_vid_regs[0xB] /* Window X position 						 */

#define OAM						0xFE00
#define VRAM					0x8000
#define DMG_FRAMES_PER_SECOND	59.732155
#define SGB_FRAMES_PER_SECOND	61.17

EXTERN UINT8 gb_bpal[4];				/* Background palette			*/
EXTERN UINT8 gb_spal0[4];				/* Sprite 0 palette				*/
EXTERN UINT8 gb_spal1[4];				/* Sprite 1 palette				*/
EXTERN UINT8 *gb_chrgen;				/* Character generator			*/
EXTERN UINT8 *gb_bgdtab;				/* Background character table	*/
EXTERN UINT8 *gb_wndtab;				/* Window character table		*/
EXTERN unsigned int gb_divcount;
EXTERN unsigned int gb_timer_count;
EXTERN UINT8 gb_timer_shift;
EXTERN UINT8 gb_tile_no_mod;

extern WRITE8_HANDLER( gb_rom_bank_select_mbc1 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc1 );
extern WRITE8_HANDLER( gb_mem_mode_select_mbc1 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc2 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc3 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc3 );
extern WRITE8_HANDLER( gb_mem_mode_select_mbc3 );
extern WRITE8_HANDLER( gb_rom_bank_select_mbc5 );
extern WRITE8_HANDLER( gb_ram_bank_select_mbc5 );
extern WRITE8_HANDLER( gb_ram_enable );
extern WRITE8_HANDLER( gb_io_w );
extern  READ8_HANDLER ( gb_io_r );
extern WRITE8_HANDLER( gb_bios_w );
extern READ8_HANDLER( gb_ie_r );
extern WRITE8_HANDLER( gb_ie_w );
extern DEVICE_INIT(gb_cart);
extern DEVICE_LOAD(gb_cart);
extern void gb_scanline_interrupt(void);
extern void gb_scanline_interrupt_set_mode0(int param);
extern void gb_scanline_interrupt_set_mode3(int param);

extern MACHINE_RESET( gb );

extern MACHINE_RESET( gbpocket );

/* from vidhrdw/gb.c */
extern READ8_HANDLER( gb_video_r );
extern WRITE8_HANDLER( gb_video_w );
extern VIDEO_START( gb );
extern VIDEO_UPDATE( gb );
void gb_refresh_scanline(void);
EXTERN double lcd_time;
/* Custom Sound Interface */
extern READ8_HANDLER( gb_sound_r );
extern WRITE8_HANDLER( gb_sound_w );
extern READ8_HANDLER( gb_wave_r );
extern WRITE8_HANDLER( gb_wave_w );
void *gameboy_sh_start(int clock, const struct CustomSound_interface *config);

/* -- Super GameBoy specific -- */
#define SGB_BORDER_PAL_OFFSET	64	/* Border colours stored from pal 4-7	*/
#define SGB_XOFFSET				48	/* GB screen starts at column 48		*/
#define SGB_YOFFSET				40	/* GB screen starts at row 40			*/

EXTERN UINT16 sgb_pal_data[4096];	/* 512 palettes of 4 colours			*/
EXTERN UINT8 sgb_pal_map[20][18];	/* Palette tile map						*/
extern UINT8 *sgb_tile_data;		/* 256 tiles of 32 bytes each			*/
EXTERN UINT8 sgb_tile_map[2048];	/* 32x32 tile map data (0-tile,1-attribute)	*/
EXTERN UINT8 sgb_window_mask;		/* Current GB screen mask				*/
EXTERN UINT8 sgb_hack;				/* Flag set if we're using a hack		*/

extern MACHINE_RESET( sgb );
extern WRITE8_HANDLER ( sgb_io_w );
/* from vidhrdw/gb.c */
void sgb_refresh_scanline(void);
void sgb_refresh_border(void);

/* -- GameBoy Color specific -- */
#define KEY1    gb_vid_regs[0x0D]		/* Prepare speed switch					*/
#define HDMA1   gb_vid_regs[0x11]		/* HDMA source high byte				*/
#define HDMA2   gb_vid_regs[0x12]		/* HDMA source low byte					*/
#define HDMA3   gb_vid_regs[0x13]		/* HDMA destination high byte			*/
#define HDMA4   gb_vid_regs[0x14]		/* HDMA destination low byte			*/
#define HDMA5   gb_vid_regs[0x15]		/* HDMA length/mode/start				*/
#define GBCBCPS gb_vid_regs[0x28]		/* Backgound palette spec				*/
#define GBCBCPD gb_vid_regs[0x29]		/* Backgound palette data				*/
#define GBCOCPS gb_vid_regs[0x2A]		/* Object palette spec					*/
#define GBCOCPD gb_vid_regs[0x2B]		/* Object palette data					*/
#define GBC_MODE_GBC		1		/* GBC is in colour mode				*/
#define GBC_MODE_MONO		2		/* GBC is in mono mode					*/
#define GBC_PAL_OBJ_OFFSET	32		/* Object palette offset				*/

extern UINT8 *GBC_VRAMMap[2];		/* Addressses of GBC video RAM banks	*/
extern UINT8 GBC_VRAMBank;			/* VRAM bank currently used				*/
EXTERN UINT8 *gbc_chrgen;			/* Character generator					*/
EXTERN UINT8 *gbc_bgdtab;			/* Background character table			*/
EXTERN UINT8 *gbc_wndtab;			/* Window character table				*/
EXTERN UINT8 gbc_mode;				/* is the GBC in mono/colour mode?		*/
EXTERN UINT8 gbc_hdma_enabled;		/* is HDMA enabled?						*/

extern MACHINE_RESET( gbc );
extern WRITE8_HANDLER ( gbc_video_w );
extern void gbc_hdma(UINT16 length);
/* from vidhrdw/gb.c */
void gbc_refresh_scanline(void);

/* -- Megaduck specific -- */
extern DEVICE_LOAD(megaduck_cart);
extern MACHINE_RESET( megaduck );
extern  READ8_HANDLER( megaduck_video_r );
extern WRITE8_HANDLER( megaduck_video_w );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type1 );
extern WRITE8_HANDLER( megaduck_rom_bank_select_type2 );
extern  READ8_HANDLER( megaduck_sound_r1 );
extern WRITE8_HANDLER( megaduck_sound_w1 );
extern  READ8_HANDLER( megaduck_sound_r2 );
extern WRITE8_HANDLER( megaduck_sound_w2 );

#endif
