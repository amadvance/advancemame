// license:BSD-3-Clause
// copyright-holders:David Haywood, ???
/*
 * IGS PGM2: IGS036 CPU, IGS037 video and YMZ774 audio.
 * Converted from MAME 0.287 to the AdvanceMAME C interfaces.
 * Five parent sets only. CPU clock and RTC retain upstream approximations.
 * The legacy memory-card UI exposes the player-one slot.
 */
#include "driver.h"
#include "machine/generic.h"
#include "machine/igs036crypt.h"
#include "machine/pgm2_aic.h"
#include "includes/pgm2.h"
#include "cpu/igs036/igs036.h"
#include "sound/ymz774.h"
#include <stdbool.h>

static UINT32 *mainram, *sram, *romboard_ram;
static UINT8 *mainrom, *encrypted_copy;
static UINT32 mainrom_size;
static UINT8 encryption_table[256], shareram[256];
static UINT32 mcu_regs[8], mcu_result0, mcu_result1, spritekey, pio_out;
static UINT16 share_bank;
static UINT8 mcu_last_cmd;
static int has_decrypted, module_decrypted, board_type;
static mame_timer *mcu_timer;
/* Board variants: 0 standard, 1 ROM+RAM (ddpdojt), 2 protected module (kov3). */
static UINT8 card[0x108];
static int card_authenticated;
static const UINT8 module_key[8] = { 0x40,0xac,0x30,0x00,0x47,0x49,0x00,0x00 };
static const UINT8 module_sum[8] = { 0xeb,0x7d,0x8d,0x90,0x2c,0xf4,0x09,0x82 };
static UINT8 module_rcv[10], module_send[9];
static int module_in, module_out, module_previous, module_count, module_sum_read;

static void decrypt_module(void)
{
	UINT32 i;
	UINT16 *dst, *src = (UINT16 *)mainrom;
	if (module_decrypted) return;
	dst = malloc(mainrom_size);
	if (!dst) fatalerror("PGM2: cannot allocate module decryption buffer");
	for (i=0; i<mainrom_size/2; i++) dst[i] = src[i ^ 0x18ec71] ^ 0xb89d;
	memcpy(src, dst, mainrom_size); free(dst);
	module_decrypted = 1;
}
static void decrypt_program(void)
{
	if (board_type == 1) {
		igs036_decrypt_rom((UINT16 *)romboard_ram, 0x200000, 0, encryption_table);
		igs036_decrypt_rom((UINT16 *)mainrom, mainrom_size, 0x200000, encryption_table);
	} else igs036_decrypt_rom((UINT16 *)mainrom, mainrom_size, 0, encryption_table);
	has_decrypted = 1;
}
static void restore_program(void)
{
	memcpy(mainrom, encrypted_copy, mainrom_size);
	if (module_decrypted) { module_decrypted = 0; decrypt_module(); }
	if (has_decrypted) {
		/* RAM is restored by the save-state system; decrypt only external ROM. */
		igs036_decrypt_rom((UINT16 *)mainrom, mainrom_size, board_type == 1 ? 0x200000 : 0, encryption_table);
	}
}
static READ32_HANDLER(encryption_r)
{
	UINT32 v = 0;
	int i;
	for (i=0; i<4; i++) v |= (UINT32)encryption_table[offset*4+i] << (i*8);
	return v;
}
static WRITE32_HANDLER(encryption_w)
{
	int i;
	for (i=0; i<4; i++) if (!(mem_mask & (0xffU << (i*8)))) encryption_table[offset*4+i] = data >> (i*8);
}
static WRITE32_HANDLER(encryption_do_w) { decrypt_program(); }
static READ32_HANDLER(startup_r) { return 0x180; }
static READ32_HANDLER(rtc_r) { return (UINT32)timer_get_time(); }
static READ32_HANDLER(sound_r)
{
	UINT32 v = 0;
	int i;
	for (i=0; i<4; i++) if (!(mem_mask & (0xffU << (8*i)))) v |= (UINT32)ymz774_read(i) << (8*i);
	return v;
}
static WRITE32_HANDLER(sound_w)
{
	int i;
	for (i=0; i<4; i++) if (!(mem_mask & (0xffU << (8*i)))) ymz774_write(i, data >> (8*i));
}
static READ32_HANDLER(shared_r)
{
	int p = offset*2 + (share_bank & 1)*128;
	return shareram[p] | ((UINT32)shareram[p+1] << 16);
}
static WRITE32_HANDLER(shared_w)
{
	int p = offset*2 + (share_bank & 1)*128;
	if (ACCESSING_LSB32) shareram[p] = data;
	if (!(mem_mask & 0x00ff0000)) shareram[p+1] = data >> 16;
}
static WRITE32_HANDLER(video_regs_w)
{
	switch (offset*4 + 0x14) {
	case 0x18: if (ACCESSING_LSW32) pgm2_aic_irq(12, CLEAR_LINE); break;
	case 0x30: if (ACCESSING_MSW32) share_bank = data >> 16; break;
	case 0x38:
		COMBINE_DATA(&spritekey);
		pgm2_spritekey = BITSWAP32(spritekey ^ 0x90055555,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31);
		break;
	}
}
static void set_palette(int index, UINT32 value)
{
	palette_set_color(index, (value >> 16) & 255, (value >> 8) & 255, value & 255);
}
static UINT32 *sp_palette, *bg_palette, *tx_palette;
static WRITE32_HANDLER(sp_palette_w) { COMBINE_DATA(&sp_palette[offset]); set_palette(offset, sp_palette[offset]); }
static WRITE32_HANDLER(bg_palette_w) { COMBINE_DATA(&bg_palette[offset]); set_palette(0x1200+offset, bg_palette[offset]); }
static WRITE32_HANDLER(tx_palette_w) { COMBINE_DATA(&tx_palette[offset]); set_palette(0x1000+offset, tx_palette[offset]); }
static void mcu_irq(int param) { pgm2_aic_irq(3, ASSERT_LINE); }
static int card_present(int player) { return player == 0 && memcard_present() != -1; }
static UINT8 card_read_sec(int index) { return card_authenticated ? card[0x104+index] : 0xff; }
static void card_write(int index, UINT8 value)
{
	index &= 255;
	if (card_authenticated && (index >= 0x20 || (card[0x100+(index>>3)] & (1 << (index&7))))) card[index] = value;
}
static void mcu_command(int is_command)
{
	UINT8 cmd = mcu_regs[0], arg1 = mcu_regs[0] >> 8, arg2 = mcu_regs[0] >> 16, arg3 = mcu_regs[0] >> 24;
	UINT8 status = 0xf7;
	int p = (~share_bank & 1)*128, i;
	if (!is_command) {
		if (mcu_last_cmd) {
			mcu_regs[3] = (mcu_regs[3] & 0xff00ffff) | 0x00f20000;
			timer_adjust(mcu_timer, TIME_IN_USEC(100), 0, 0); mcu_last_cmd = 0;
		}
		return;
	}
	mcu_last_cmd = cmd;
	switch (cmd) {
	case 0xf6: mcu_regs[3] = mcu_result0; mcu_regs[4] = mcu_result1; mcu_last_cmd = 0; break;
	case 0xe0: mcu_result0 = mcu_regs[0]; mcu_result1 = mcu_regs[1]; break;
	case 0xe1:
		if (arg2 == 2) memset(shareram+p, arg3, 128);
		mcu_result0 = cmd; mcu_result1 = 0; break;
	case 0xc0: case 0xc1:
		if (!card_present(arg1 & 3)) status = 0xf4;
		mcu_result0 = cmd; break;
	case 0xc2: case 0xc3:
		if (!card_present(arg1 & 3)) status = 0xf4;
		else for (i=0; i<arg3; i++) {
			if (cmd == 0xc2) shareram[(p+i)&255] = card[(arg2+i)&255];
			else card_write(arg2+i, shareram[(p+i)&255]);
		}
		mcu_result0 = cmd; break;
	case 0xc4:
		if (card_present(arg1 & 3)) mcu_result1 = card_read_sec(1) | (card_read_sec(2)<<8) | (card_read_sec(3)<<16);
		else status = 0xf4;
		mcu_result0 = cmd; break;
	case 0xc5: case 0xc6:
		if (!card_present(arg1 & 3)) status = 0xf4;
		else if (card_authenticated) {
			if (cmd == 0xc5) card[0x104+(arg2&3)] = arg3;
			else card[0x100+(arg2&3)] &= arg3;
		}
		mcu_result0 = cmd; break;
	case 0xc7:
		if (card_present(arg1 & 3)) mcu_result1 = card[0x100] | (card[0x101]<<8) | (card[0x102]<<16) | ((UINT32)card[0x103]<<24);
		else status = 0xf4;
		mcu_result0 = cmd; break;
	case 0xc8:
		if (card_present(arg1 & 3)) card_write(arg2, arg3); else status = 0xf4;
		mcu_result0 = cmd; break;
	case 0xc9:
		if (!card_present(arg1 & 3)) status = 0xf4;
		else if (card[0x104]&7) {
			card_authenticated = card[0x105] == arg2 && card[0x106] == arg3 && card[0x107] == (mcu_regs[1]&255);
			if (card_authenticated) card[0x104] = 7; else card[0x104] >>= 1;
		}
		mcu_result0 = cmd; break;
	default: logerror("PGM2 MCU unknown command %08x %08x\n", mcu_regs[0], mcu_regs[1]); status = 0xf4; break;
	}
	mcu_regs[3] = (mcu_regs[3] & 0xff00ffff) | (status << 16);
	timer_adjust(mcu_timer, TIME_IN_MSEC(1), 0, 0);
}
static READ32_HANDLER(mcu_r) { return mcu_regs[(offset >> 15)&7]; }
static WRITE32_HANDLER(mcu_w)
{
	int reg = (offset >> 15)&7;
	COMBINE_DATA(&mcu_regs[reg]);
	if (reg == 2 && mcu_regs[2]) mcu_command(1);
	if (reg == 5 && mcu_regs[5]) { mcu_command(0); pgm2_aic_irq(3, CLEAR_LINE); }
}
static void module_clock(int state)
{
	if (module_previous != state && !state) {
		if (module_count < 80) {
			int off = module_count/8, bit = (module_count&7)^7, i;
			module_rcv[off] = (module_rcv[off] & ~(1<<bit)) | (module_in << bit);
			if (++module_count == 80) {
				switch (module_rcv[0]) {
				case 0x0d: memset(module_send, 0xa3, 4); memset(module_send+4, 0x6d, 4); break;
				case 0x25: for (i=0; i<8; i++) module_send[i] = module_key[i^3] ^ module_rcv[i+1]; break;
				default: logerror("PGM2 unknown module command %02x\n", module_rcv[0]); break;
				}
				module_send[8] = 0; for (i=0; i<8; i++) module_send[8] += module_send[i];
			}
		} else {
			module_out = BIT(module_send[(module_count-80)/8], (module_count&7)^7);
			if (++module_count >= 152) module_count = 0;
		}
	}
	module_previous = state;
}
static WRITE32_HANDLER(pio_w)
{
	if (offset) pio_out &= ~(data & ~mem_mask); else pio_out |= data & ~mem_mask;
	module_in = !!(pio_out & 0x100); module_clock(!!(pio_out & 0x200));
}
static READ32_HANDLER(pio_r) { return module_out << 8; }
static UINT16 module_read(int offset)
{
	if (module_sum_read && offset>0 && offset<5) {
		int i = ((offset-1)*2)^2;
		if (offset == 4) module_sum_read = 0;
		return (module_sum[i]^module_key[i]) | ((module_sum[i+1]^module_key[i+1])<<8);
	}
	return ((UINT16 *)mainrom)[offset];
}
static READ32_HANDLER(module_r)
{
	UINT32 value = 0;
	if (ACCESSING_LSW32) value |= module_read(offset*2);
	if (ACCESSING_MSW32) value |= (UINT32)module_read(offset*2+1)<<16;
	return value;
}
static void module_write(int offset, UINT16 data)
{
	UINT16 dec = ((module_key[0] | (module_key[1]<<8) | (module_key[2]<<16))>>6)&0xffff;
	if (data == dec) decrypt_module();
	else if (data == 0xc2) module_sum_read = 1;
	else if (data == 0x84) module_sum_read = 0;
}
static WRITE32_HANDLER(module_w)
{
	if (ACCESSING_LSW32) module_write(offset*2, data);
	if (ACCESSING_MSW32) module_write(offset*2+1, data>>16);
}

#define PGM2_COMMON_MAP \
	AM_RANGE(0x00000000,0x00003fff) AM_ROM \
	AM_RANGE(0x02000000,0x0200ffff) AM_RAM AM_BASE(&sram) \
	AM_RANGE(0x03600000,0x036bffff) AM_READWRITE(mcu_r,mcu_w) \
	AM_RANGE(0x03900000,0x03900003) AM_READ(input_port_0_dword_r) \
	AM_RANGE(0x03a00000,0x03a00003) AM_READ(input_port_1_dword_r) \
	AM_RANGE(0x20000000,0x2007ffff) AM_RAM AM_BASE(&mainram) \
	AM_RANGE(0x30000000,0x30001fff) AM_RAM AM_BASE(&pgm2_sp_videoram) \
	AM_RANGE(0x30020000,0x30021fff) AM_RAM AM_WRITE(pgm2_bg_videoram_w) AM_BASE(&pgm2_bg_videoram) \
	AM_RANGE(0x30040000,0x30045fff) AM_RAM AM_WRITE(pgm2_fg_videoram_w) AM_BASE(&pgm2_fg_videoram) \
	AM_RANGE(0x30060000,0x30063fff) AM_RAM AM_WRITE(sp_palette_w) AM_BASE(&sp_palette) \
	AM_RANGE(0x30080000,0x30081fff) AM_RAM AM_WRITE(bg_palette_w) AM_BASE(&bg_palette) \
	AM_RANGE(0x300a0000,0x300a07ff) AM_RAM AM_WRITE(tx_palette_w) AM_BASE(&tx_palette) \
	AM_RANGE(0x300c0000,0x300c01ff) AM_RAM AM_BASE(&pgm2_sp_zoom) \
	AM_RANGE(0x300e0000,0x300e03ff) AM_RAM AM_MIRROR(0xfc00) AM_BASE(&pgm2_lineram) \
	AM_RANGE(0x30100000,0x301000ff) AM_READWRITE(shared_r,shared_w) \
	AM_RANGE(0x30120000,0x30120003) AM_RAM AM_BASE(&pgm2_bgscroll) \
	AM_RANGE(0x30120008,0x3012000b) AM_RAM AM_BASE(&pgm2_fgscroll) \
	AM_RANGE(0x3012000c,0x3012000f) AM_RAM AM_BASE(&pgm2_vidmode) \
	AM_RANGE(0x30120014,0x3012003b) AM_WRITE(video_regs_w) \
	AM_RANGE(0x40000000,0x40000003) AM_READWRITE(sound_r,sound_w) \
	AM_RANGE(0xfffff000,0xfffff14b) AM_READWRITE(pgm2_aic_r,pgm2_aic_w) \
	AM_RANGE(0xfffffd28,0xfffffd2b) AM_READ(rtc_r) \
	AM_RANGE(0xfffffa08,0xfffffa0b) AM_WRITE(encryption_do_w) \
	AM_RANGE(0xfffffa0c,0xfffffa0f) AM_READ(startup_r) \
	AM_RANGE(0xfffffc00,0xfffffcff) AM_READWRITE(encryption_r,encryption_w)

static ADDRESS_MAP_START(pgm2_map, ADDRESS_SPACE_PROGRAM, 32)
	PGM2_COMMON_MAP
	AM_RANGE(0x10000000,0x10ffffff) AM_ROM AM_REGION(REGION_USER1,0)
	AM_RANGE(0xfffff430,0xfffff437) AM_WRITENOP
ADDRESS_MAP_END
static ADDRESS_MAP_START(pgm2_ramrom_map, ADDRESS_SPACE_PROGRAM, 32)
	PGM2_COMMON_MAP
	AM_RANGE(0x10000000,0x101fffff) AM_RAM AM_BASE(&romboard_ram)
	AM_RANGE(0x10200000,0x103fffff) AM_ROM AM_REGION(REGION_USER1,0)
	AM_RANGE(0xfffff430,0xfffff437) AM_WRITENOP
ADDRESS_MAP_END
static ADDRESS_MAP_START(pgm2_module_map, ADDRESS_SPACE_PROGRAM, 32)
	PGM2_COMMON_MAP
	AM_RANGE(0x10000000,0x1000000f) AM_READWRITE(module_r,module_w)
	AM_RANGE(0x10000010,0x107fffff) AM_ROM AM_REGION(REGION_USER1,0x10) AM_WRITE(module_w)
	AM_RANGE(0x10800000,0x10ffffff) AM_ROM AM_REGION(REGION_USER1,0x800000)
	AM_RANGE(0xfffff430,0xfffff437) AM_WRITE(pio_w)
	AM_RANGE(0xfffff43c,0xfffff43f) AM_READ(pio_r)
ADDRESS_MAP_END
static INTERRUPT_GEN(pgm2_vblank)
{
	pgm2_draw_sprites(); pgm2_aic_irq(12, ASSERT_LINE);
}
static NVRAM_HANDLER(pgm2)
{
	if (read_or_write) mame_fwrite(file, sram, 0x10000);
	else if (file) mame_fread(file, sram, 0x10000);
	else memcpy(sram, memory_region(REGION_USER2), 0x10000);
}
static MEMCARD_HANDLER(pgm2)
{
	card_authenticated = 0;
	switch (action) {
	case MEMCARD_CREATE:
		if (memory_region(REGION_USER3)) mame_fwrite(file, memory_region(REGION_USER3), 0x108);
		break;
	case MEMCARD_INSERT:
		memset(card, 0xff, sizeof(card)); mame_fread(file, card, sizeof(card)); break;
	case MEMCARD_EJECT: mame_fwrite(file, card, sizeof(card)); break;
	}
}
static MACHINE_START(pgm2)
{
	mcu_timer = timer_alloc(mcu_irq);
	pgm2_aic_init();
	state_save_register_global_array(encryption_table); state_save_register_global_array(shareram);
	state_save_register_global_array(mcu_regs); state_save_register_global(mcu_result0); state_save_register_global(mcu_result1);
	state_save_register_global(mcu_last_cmd); state_save_register_global(share_bank); state_save_register_global(spritekey);
	state_save_register_global(pgm2_spritekey); state_save_register_global(has_decrypted); state_save_register_global(module_decrypted);
	state_save_register_global(pio_out); state_save_register_global(module_in); state_save_register_global(module_out);
	state_save_register_global(module_previous); state_save_register_global(module_count); state_save_register_global(module_sum_read);
	state_save_register_global_array(module_rcv); state_save_register_global_array(module_send);
	state_save_register_global_array(card); state_save_register_global(card_authenticated);
	state_save_register_func_postload(restore_program);
	return 0;
}
static MACHINE_RESET(pgm2)
{
	memcpy(mainrom, encrypted_copy, mainrom_size);
	has_decrypted = module_decrypted = 0;
	memset(encryption_table, 0, sizeof(encryption_table)); memset(mcu_regs, 0, sizeof(mcu_regs));
	memset(shareram, 0, sizeof(shareram));
	spritekey = pgm2_spritekey = share_bank = mcu_last_cmd = mcu_result0 = mcu_result1 = pio_out = 0;
	module_in = module_out = module_previous = module_sum_read = 0; module_count = 151;
	memset(module_rcv,0,sizeof(module_rcv)); memset(module_send,0,sizeof(module_send));
	timer_adjust(mcu_timer, TIME_NEVER, 0, 0); pgm2_aic_reset();
}
static INPUT_PORTS_START( pgm2 )
	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(1)
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(2)
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(3)
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(3)
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(3)
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(3)
	PORT_BIT( 0x01000000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(3)
	PORT_BIT( 0x02000000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(3)
	PORT_BIT( 0x04000000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(3)
	PORT_BIT( 0x08000000, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(3)
	PORT_BIT( 0x10000000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20000000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x40000000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x80000000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START
	PORT_BIT( 0x00000001, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(4)
	PORT_BIT( 0x00000002, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(4)
	PORT_BIT( 0x00000004, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(4)
	PORT_BIT( 0x00000008, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(4)
	PORT_BIT( 0x00000010, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(4)
	PORT_BIT( 0x00000020, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(4)
	PORT_BIT( 0x00000040, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(4)
	PORT_BIT( 0x00000080, IP_ACTIVE_LOW, IPT_BUTTON4 ) PORT_PLAYER(4)
	PORT_BIT( 0x00000100, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000200, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00000400, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x00000800, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x00001000, IP_ACTIVE_LOW, IPT_START3 )
	PORT_BIT( 0x00002000, IP_ACTIVE_LOW, IPT_START4 )
	PORT_BIT( 0x00004000, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x00008000, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x00010000, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x00020000, IP_ACTIVE_LOW, IPT_COIN4 )
	PORT_BIT( 0x00040000, IP_ACTIVE_LOW, IPT_SERVICE1 ) PORT_NAME("Test Key P1 & P2") // test key p1+p2
	PORT_BIT( 0x00080000, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Test Key P3 & P4") // test key p3+p4
	PORT_BIT( 0x00100000, IP_ACTIVE_LOW, IPT_SERVICE3 ) PORT_NAME("Service P1 & P2") // service key p1+p2
	PORT_BIT( 0x00200000, IP_ACTIVE_LOW, IPT_SERVICE4 ) PORT_NAME("Service P3 & P4") // service key p3+p4
	PORT_BIT( 0x00400000, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x00800000, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_SERVICE( 0x01000000, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x02000000, 0x02000000, "Music" )
	PORT_DIPSETTING(          0x00000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x02000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x04000000, 0x04000000, "Voice" )
	PORT_DIPSETTING(          0x00000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x04000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x08000000, 0x08000000, "Free" )
	PORT_DIPSETTING(          0x08000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x10000000, 0x10000000, "Stop" )
	PORT_DIPSETTING(          0x10000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x20000000, 0x20000000, DEF_STR( Unused ) )
	PORT_DIPSETTING(          0x20000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x40000000, 0x40000000, DEF_STR( Unused ) )
	PORT_DIPSETTING(          0x40000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
	PORT_DIPNAME( 0x80000000, 0x80000000, "Debug" )
	PORT_DIPSETTING(          0x80000000, DEF_STR( Off ) )
	PORT_DIPSETTING(          0x00000000, DEF_STR( On ) )
INPUT_PORTS_END

static const gfx_layout pgm2_text = {
	8,8, RGN_FRAC(1,1), 4, {0,1,2,3},
	{4,0,12,8,20,16,28,24}, {0,32,64,96,128,160,192,224}, 256
};
static const gfx_layout pgm2_bg = {
	32,32, RGN_FRAC(1,1), 7, {1,2,3,4,5,6,7},
	{STEP16(0,8),STEP16(128,8)}, {STEP16(0,256),STEP16(4096,256)}, 8192
};
static const gfx_decode pgm2_gfx[] = {
	{ REGION_GFX1, 0, &pgm2_text, 0x1000, 32 },
	{ REGION_GFX2, 0, &pgm2_bg, 0x1200, 16 },
	{ -1 }
};

#define ORLEG2_INTERNAL_OVERSEAS \
	ROM_REGION( 0x04000, REGION_CPU1, 0 ) \
	ROM_LOAD( "ol2_fa.igs036", 0x00000000, 0x0004000, CRC(cc4d398a) SHA1(c50bcc81f02cd5aa8ad157d73209dc53bdedc023) ) // Core V100 Oversea

#define ORLEG2_PROGRAM_104(prefix, extension) \
	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) \
	ROM_LOAD( #prefix "_v104" #extension ".u7",  0x000000, 0x800000, CRC(7c24a4f5) SHA1(3cd9f9264ef2aad0869afdf096e88eb8d74b2570) ) // V104 08-03-03 13:25:37

#define ORLEG2_VIDEO_SOUND_ROMS \
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_ERASE00 ) \
	ROM_LOAD( "ig-a_text.u4",            0x00000000, 0x0200000, CRC(fa444c32) SHA1(31e5e3efa92d52bf9ab97a0ece51e3b77f52ce8a) ) \
	\
	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) \
	ROM_LOAD32_WORD( "ig-a_bgl.u35",     0x00000000, 0x0800000, CRC(083a8315) SHA1(0dba25e132fbb12faa59ced648c27b881dc73478) ) \
	ROM_LOAD32_WORD( "ig-a_bgh.u36",     0x00000002, 0x0800000, CRC(e197221d) SHA1(5574b1e3da4b202db725be906dd868edc2fd4634) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX3, 0 ) /* 1bpp sprite mask data (packed) */ \
	ROM_LOAD32_WORD( "ig-a_bml.u12",     0x00000000, 0x1000000, CRC(113a331c) SHA1(ee6b31bb2b052cc8799573de0d2f0a83f0ab4f6a) ) \
	ROM_LOAD32_WORD( "ig-a_bmh.u16",     0x00000002, 0x1000000, CRC(fbf411c8) SHA1(5089b5cc9bbf6496ef1367c6255e63e9ab895117) ) \
	\
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* sprite colour data (6bpp data, 2 bits unused except for 4 bytes that are randomly 0xff - check dump?) */ \
	ROM_LOAD32_WORD( "ig-a_cgl.u18",     0x00000000, 0x2000000, CRC(43501fa6) SHA1(58ccce6d393964b771fec3f5c583e3ede57482a3) ) \
	ROM_LOAD32_WORD( "ig-a_cgh.u26",     0x00000002, 0x2000000, CRC(7051d020) SHA1(3d9b24c6fda4c9699bb9f00742e0888059b623e1) ) \
	\
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF ) /* ymz774 */ \
	ROM_LOAD16_WORD_SWAP( "ig-a_sp.u2",  0x00000000, 0x1000000, CRC(8250688c) SHA1(d2488477afc528aeee96826065deba2bce4f0a7d) ) \
	\
	ROM_REGION( 0x10000, REGION_USER2, 0 ) \
	ROM_LOAD( "xyj2_nvram",            0x00000000, 0x10000, CRC(ccccc71c) SHA1(585b5ccbf89dd28d8532da785d7c8af12f31c6d6) )

#define KOV2NL_INTERNAL_OVERSEA \
	ROM_REGION( 0x04000, REGION_CPU1, 0 ) \
	ROM_LOAD( "kov2nl_igs036_oversea.rom", 0x00000000, 0x0004000, CRC(25ec60cd) SHA1(7dd12d2bc642bfa79520676fe5de458ce7d08ef6) ) /* Core V100 oversea */ \
	ROM_REGION( 0x108, REGION_USER3, 0 ) \
	ROM_LOAD( "blank_kov2nl_overseas_card.pg2", 0x000, 0x108, CRC(1155f01f) SHA1(60f7bed1461b362a3da687503cd72ed2d5e96f30) )

#define KOV2NL_PROGRAM_302(prefix, extension) \
	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 ) \
	ROM_LOAD( #prefix "_v302" #extension ".u7", 0x00000000, 0x0800000, CRC(b19cf540) SHA1(25da5804bbfd7ef2cdf5cc5aabaa803d18b98929) ) // V302 08-12-03 15:27:34

#define KOV2NL_VIDEO_SOUND_ROMS \
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_ERASE00 ) \
	ROM_LOAD( "ig-a3_text.u4",           0x00000000, 0x0200000, CRC(214530ff) SHA1(4231a02054b0345392a077042b95779fd45d6c22) ) \
	\
	ROM_REGION( 0x1000000, REGION_GFX2, 0 ) \
	ROM_LOAD32_WORD( "ig-a3_bgl.u35",    0x00000000, 0x0800000, CRC(2d46b1f6) SHA1(ea8c805eda6292e86a642e9633d8fee7054d10b1) ) \
	ROM_LOAD32_WORD( "ig-a3_bgh.u36",    0x00000002, 0x0800000, CRC(df710c36) SHA1(f826c3f496c4f17b46d18af1d8e02cac7b7027ac) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX3, 0 ) /* 1bpp sprite mask data */ \
	ROM_LOAD32_WORD( "ig-a3_bml.u12",    0x00000000, 0x1000000, CRC(0bf63836) SHA1(b8e4f1951f8074b475b795bd7840c5a375b6f5ef) ) \
	ROM_LOAD32_WORD( "ig-a3_bmh.u16",    0x00000002, 0x1000000, CRC(4a378542) SHA1(5d06a8a8796285a786ebb690c34610f923ef5570) ) \
	\
	ROM_REGION( 0x4000000, REGION_GFX4, 0 ) /* sprite colour data */ \
	ROM_LOAD32_WORD( "ig-a3_cgl.u18",    0x00000000, 0x2000000, CRC(8d923e1f) SHA1(14371cf385dd8857017d3111cd4710f4291b1ae2) ) \
	ROM_LOAD32_WORD( "ig-a3_cgh.u26",    0x00000002, 0x2000000, CRC(5b6fbf3f) SHA1(d1f52e230b91ee6cde939d7c2b74da7fd6527e73) ) \
	\
	ROM_REGION( 0x2000000, REGION_SOUND1, ROMREGION_ERASEFF ) /* ymz774 */ \
	ROM_LOAD16_WORD_SWAP( "ig-a3_sp.u37",            0x00000000, 0x2000000, CRC(45cdf422) SHA1(8005d284bcee73cff37a147fcd1c3e9f039a7203) ) \
	\
	ROM_REGION(0x10000, REGION_USER2, 0) \
	ROM_LOAD("gsyx_nvram", 0x00000000, 0x10000, CRC(22400c16) SHA1(f775a16299c30f2ce23d683161b910e06eff37c1) )

#define DDPDOJT_VIDEO_SOUND_ROMS \
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_ERASE00 ) \
	ROM_LOAD( "ddpdoj_text.u1",          0x00000000, 0x0200000, CRC(f18141d1) SHA1(a16e0a76bc926a158bb92dfd35aca749c569ef50) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX2, 0 ) \
	ROM_LOAD32_WORD( "ddpdoj_bgl.u23",   0x00000000, 0x1000000, CRC(ff65fdab) SHA1(abdd5ca43599a2daa722547a999119123dd9bb28) ) \
	ROM_LOAD32_WORD( "ddpdoj_bgh.u24",   0x00000002, 0x1000000, CRC(bb84d2a6) SHA1(a576a729831b5946287fa8f0d923016f43a9bedb) ) \
	\
	ROM_REGION( 0x1000000, REGION_GFX3, 0 ) /* 1bpp sprite mask data */ \
	ROM_LOAD32_WORD( "ddpdoj_mapl0.u13", 0x00000000, 0x800000, CRC(bcfbb0fc) SHA1(9ec478eba9905913cf997bd9b46c70c1ad383630) ) \
	ROM_LOAD32_WORD( "ddpdoj_maph0.u15", 0x00000002, 0x800000, CRC(0cc75d4e) SHA1(6d1b5ef0fdebf1e84fa199b939ffa07b810b12c9) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX4, 0 ) /* sprite colour data */ \
	ROM_LOAD32_WORD( "ddpdoj_spa0.u9",   0x00000000, 0x1000000, CRC(1232c1b4) SHA1(ecc1c549ae19d2f052a85fe4a993608aedf49a25) ) \
	ROM_LOAD32_WORD( "ddpdoj_spb0.u18",  0x00000002, 0x1000000, CRC(6a9e2cbf) SHA1(8e0a4ea90f5ef534820303d62f0873f8ac9f080e) ) \
	\
	ROM_REGION( 0x1000000, REGION_SOUND1, ROMREGION_ERASEFF ) /* ymz774 */ \
	ROM_LOAD16_WORD_SWAP( "ddpdoj_wave0.u12",        0x00000000, 0x1000000, CRC(2b71a324) SHA1(f69076cc561f40ca564d804bc7bd455066f8d77c) ) \
	\
	ROM_REGION( 0x10000, REGION_USER2, 0 ) \
	ROM_LOAD( "ddpdojt_sram",            0x00000000, 0x10000, CRC(af99e304) SHA1(e44fed22b902431298748eca84533f8685926afd) )

#define KOV3_INTERNAL_CHINA \
	ROM_REGION( 0x04000, REGION_CPU1, 0 ) \
	ROM_LOAD( "kov3_igs036_china.rom", 0x00000000, 0x0004000, CRC(c7d33764) SHA1(5cd48f876e637d60391d39ac6e40bf243300cc75) ) /* Core V100 China */ \
	ROM_REGION( 0x108, REGION_USER3, 0 ) \
	ROM_LOAD( "blank_kov3_china_card.pg2", 0x000, 0x108, CRC(bd5a968f) SHA1(b9045eb70e02afda7810431c592208053d863980) )

#define KOV3_VIDEO_SOUND_ROMS \
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_ERASE00 ) \
	ROM_LOAD( "kov3_text.u1",            0x00000000, 0x0200000, CRC(198b52d6) SHA1(e4502abe7ba01053d16c02114f0c88a3f52f6f40) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX2, 0 ) \
	ROM_LOAD32_WORD( "kov3_bgl.u6",      0x00000000, 0x1000000, CRC(49a4c5bc) SHA1(26b7da91067bda196252520e9b4893361c2fc675) ) \
	ROM_LOAD32_WORD( "kov3_bgh.u7",      0x00000002, 0x1000000, CRC(adc1aff1) SHA1(b10490f0dbef9905cdb064168c529f0b5a2b28b8) ) \
	\
	ROM_REGION( 0x4000000, REGION_GFX3, 0 ) /* 1bpp sprite mask data */ \
	ROM_LOAD32_WORD( "kov3_mapl0.u15",   0x00000000, 0x2000000, CRC(9e569bf7) SHA1(03d26e000e9d8e744546be9649628d2130f2ec4c) ) \
	ROM_LOAD32_WORD( "kov3_maph0.u16",   0x00000002, 0x2000000, CRC(6f200ad8) SHA1(cd12c136d4f5d424bd7daeeacd5c4127beb3d565) ) \
	\
	ROM_REGION( 0x8000000, REGION_GFX4, 0 ) /* sprite colour data */ \
	ROM_LOAD32_WORD( "kov3_spa0.u17",    0x00000000, 0x4000000, CRC(3a1e58a9) SHA1(6ba251407c69ee62f7ea0baae91bc133acc70c6f) ) \
	ROM_LOAD32_WORD( "kov3_spb0.u10",    0x00000002, 0x4000000, CRC(90396065) SHA1(01bf9f69d77a792d5b39afbba70fbfa098e194f1) ) \
	\
	ROM_REGION( 0x4000000, REGION_SOUND1, ROMREGION_ERASEFF ) /* ymz774 */ \
	ROM_LOAD16_WORD_SWAP( "kov3_wave0.u13",              0x00000000, 0x4000000, CRC(aa639152) SHA1(2314c6bd05524525a31a2a4668a36a938b924ba4) ) \
	\
	ROM_REGION( 0x10000, REGION_USER2, 0 ) \
	ROM_LOAD( "kov3_sram",            0x00000000, 0x10000, CRC(d9608102) SHA1(dec5631642393f4ec76912c81fd60249bb45aa13) )

#define KOF98UMH_VIDEO_SOUND_ROMS \
	ROM_REGION( 0x200000, REGION_GFX1, ROMREGION_ERASE00 ) \
	ROM_LOAD( "ig-d3_text.u1",          0x00000000, 0x0200000, CRC(9a0ea82e) SHA1(7844fd7e46c3fbb2164060f160da528254fd177e) ) \
	\
	ROM_REGION( 0x2000000, REGION_GFX2, ROMREGION_ERASE00 ) \
	/* bgl/bgh unpopulated (no background tilemap) */ \
	\
	ROM_REGION( 0x08000000, REGION_GFX3, 0 ) /* 1bpp sprite mask data */ \
	ROM_LOAD32_WORD( "ig-d3_mapl0.u13", 0x00000000, 0x4000000, CRC(5571d63e) SHA1(dad73797a35738013d82e3b8ca96fa001ec56f69) ) \
	ROM_LOAD32_WORD( "ig-d3_maph0.u15", 0x00000002, 0x4000000, CRC(0da7b1b8) SHA1(87741242bd827eca3788b490df6dcb65f7a89733) ) \
	\
	ROM_REGION( 0x20000000, REGION_GFX4, 0 ) /* sprite colour data - some byte are 0x40 or even 0xff, but verified on 2 boards */ \
	ROM_LOAD32_WORD( "ig-d3_spa0.u9",   0x00000000, 0x4000000, CRC(cfef8f7d) SHA1(54f58d1b9eb7d2e4bbe13fbdfd98f5b14ce2086b) ) \
	ROM_LOAD32_WORD( "ig-d3_spb0.u18",  0x00000002, 0x4000000, CRC(f199d5c8) SHA1(91f5e8efd1f6a9e5aada51afdf5a8f52bac24185) ) \
	/* spa1/spb1 unpopulated */ \
	ROM_LOAD32_WORD( "ig-d3_spa2.u10",  0x10000000, 0x4000000, CRC(03bfd35c) SHA1(814998cd5ee01c9da775b73f7a0ba4216fe4970e) ) \
	ROM_LOAD32_WORD( "ig-d3_spb2.u20",  0x10000002, 0x4000000, CRC(9aaa840b) SHA1(3c6078d53bb5eca5c501540214287dd102102ea1) ) \
	/* spa3/spb3 unpopulated */ \
	\
	ROM_REGION( 0x08000000, REGION_SOUND1, ROMREGION_ERASEFF ) /* ymz774 */ \
	ROM_LOAD16_WORD_SWAP( "ig-d3_wave0.u12",        0x00000000, 0x4000000, CRC(edf2332d) SHA1(7e01c7e03e515814d7de117c265c3668d32842fa) ) \
	ROM_LOAD16_WORD_SWAP( "ig-d3_wave1.u11",        0x04000000, 0x4000000, CRC(62321b20) SHA1(a388c8a2489430fbe92fb26b3ef81c66ce97f318) ) \
	\
	ROM_REGION( 0x10000, REGION_USER2, 0 ) \
	ROM_LOAD( "kof98umh_sram",            0x00000000, 0x10000, CRC(60460ed9) SHA1(55cd8de37cee04ff7ad940fb52f8fb8db042c26e) )

ROM_START( orleg2 )
	ORLEG2_INTERNAL_OVERSEAS
	ORLEG2_PROGRAM_104(ol2,fa)
	ORLEG2_VIDEO_SOUND_ROMS
ROM_END

ROM_START( kov2nl )
	KOV2NL_INTERNAL_OVERSEA
	KOV2NL_PROGRAM_302(kov2nl, fa)
	KOV2NL_VIDEO_SOUND_ROMS
ROM_END

ROM_START( ddpdojt )
	ROM_REGION( 0x04000, REGION_CPU1, 0 )
	ROM_LOAD( "ddpdoj_igs036_china.rom",       0x00000000, 0x0004000, CRC(5db91464) SHA1(723d8086285805bd815e62120dfa9a4269bcd932) ) // Core V100 China

	ROM_REGION32_LE( 0x0200000, REGION_USER1, 0 )
	ROM_LOAD( "ddpdoj_v201cn.u4",        0x00000000, 0x0200000, CRC(89e4b760) SHA1(9fad1309da31d12a413731b416a8bbfdb304ed9e) ) // V201 10-03-27 17:45:12

	DDPDOJT_VIDEO_SOUND_ROMS
ROM_END

ROM_START( kov3 )
	KOV3_INTERNAL_CHINA

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 )
	ROM_LOAD( "kov3_v104cn_raw.bin",         0x00000000, 0x0800000, CRC(1b5cbd24) SHA1(6471d4842a08f404420dea2bd1c8b88798c80fd5) ) // V104 11-12-09 14:29:14

	KOV3_VIDEO_SOUND_ROMS
ROM_END

ROM_START( kof98umh )
	ROM_REGION( 0x04000, REGION_CPU1, 0 )
	ROM_LOAD( "kof98umh_internal_rom.bin",       0x00000000, 0x0004000, CRC(3ed2e50f) SHA1(35310045d375d9dda36c325e35257123a7b5b8c7) ) // Core V100 China

	ROM_REGION32_LE( 0x1000000, REGION_USER1, 0 )
	ROM_LOAD( "kof98umh_v100cn.u4",        0x00000000, 0x1000000, CRC(2ea91e3b) SHA1(5a586bb99cc4f1b02e0db462d5aff721512e0640) ) // V100 09-08-23 17:52:03

	KOF98UMH_VIDEO_SOUND_ROMS
ROM_END

static void iga_u16_decode(UINT16 *rom, int len, int ixor)
{
	int i;

	for (i = 1; i < len / 2; i+=2)
	{
		UINT16 x = ixor;

		if ( (i>>1) & 0x000001) x ^= 0x0010;
		if ( (i>>1) & 0x000002) x ^= 0x2004;
		if ( (i>>1) & 0x000004) x ^= 0x0801;
		if ( (i>>1) & 0x000008) x ^= 0x0300;
		if ( (i>>1) & 0x000010) x ^= 0x0080;
		if ( (i>>1) & 0x000020) x ^= 0x0020;
		if ( (i>>1) & 0x000040) x ^= 0x4008;
		if ( (i>>1) & 0x000080) x ^= 0x1002;
		if ( (i>>1) & 0x000100) x ^= 0x0400;
		if ( (i>>1) & 0x000200) x ^= 0x0040;
		if ( (i>>1) & 0x000400) x ^= 0x8000;

		rom[i] ^= x;
		rom[i] = BITSWAP16(rom[i], 8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7);
	}
}

static void iga_u12_decode(UINT16* rom, int len, int ixor)
{
	int i;

	for (i = 0; i < len / 2; i+=2)
	{
		UINT16 x = ixor;

		if ( (i>>1) & 0x000001) x ^= 0x9004;
		if ( (i>>1) & 0x000002) x ^= 0x0028;
		if ( (i>>1) & 0x000004) x ^= 0x0182;
		if ( (i>>1) & 0x000008) x ^= 0x0010;
		if ( (i>>1) & 0x000010) x ^= 0x2040;
		if ( (i>>1) & 0x000020) x ^= 0x0801;
		if ( (i>>1) & 0x000040) x ^= 0x0000;
		if ( (i>>1) & 0x000080) x ^= 0x0000;
		if ( (i>>1) & 0x000100) x ^= 0x4000;
		if ( (i>>1) & 0x000200) x ^= 0x0600;
		if ( (i>>1) & 0x000400) x ^= 0x0000;

		rom[i] ^= x;
		rom[i] = BITSWAP16(rom[i], 8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7);
	}
}

static void sprite_colour_decode(UINT16* rom, int len)
{
	int i;

	for (i = 0; i < len / 2; i++)
	{
		rom[i] = BITSWAP16(rom[i], 15, 14, // unused - 6bpp
								   13, 12, 11,
								   5, 4, 3,
								   7, 6, // unused - 6bpp
								   10, 9, 8,
								   2, 1, 0  );
	}
}

/* Upstream idle-loop optimizations; all checks remain specific to the dumped programs. */
static READ32_HANDLER(orleg2_idle_r)
{
	UINT32 pc = activecpu_get_pc();
	if ((pc == 0x1002faec || pc == 0x1002f9b8) && !mainram[0x20114/4] && !mainram[0x20118/4]) cpu_spinuntil_int();
	return mainram[0x20114/4];
}
static READ32_HANDLER(kov2nl_idle_r)
{
	UINT32 pc = activecpu_get_pc();
	if ((pc == 0x10053a94 || pc == 0x1005332c || pc == 0x1005327c) && !mainram[0x20470/4] && !mainram[0x20474/4]) cpu_spinuntil_int();
	return mainram[0x20470/4];
}
static READ32_HANDLER(kof98umh_idle_r)
{
	if (activecpu_get_pc() == 0x100028f6 && !mainram[0x60/4] && !mainram[0x64/4]) cpu_spinuntil_int();
	return mainram[0x60/4];
}
static READ32_HANDLER(ddpdojt_idle_r)
{
	if (activecpu_get_pc() == 0x10001a7e && !mainram[0x60/4] && !mainram[0x64/4]) cpu_spinuntil_int();
	return mainram[0x60/4];
}
static READ32_HANDLER(ddpdojt_idle2_r)
{
	UINT32 pc = activecpu_get_pc();
	if ((pc == 0x1008fefe || pc == 0x1008fbe8) && (mainram[0x21e04/4] & 0x00ff0000)) cpu_spinuntil_int();
	return mainram[0x21e04/4];
}
static READ32_HANDLER(kov3_idle_r)
{
	UINT32 pc = activecpu_get_pc();
	if ((pc == 0x1000729a || pc == 0x1000729e) && !mainram[0xb4/4] && !mainram[0xb8/4]) cpu_spinuntil_int();
	return mainram[0xb4/4];
}
static void common_init(int board)
{
	board_type = board;
	mainrom = memory_region(REGION_USER1); mainrom_size = memory_region_length(REGION_USER1);
	encrypted_copy = auto_malloc(mainrom_size); memcpy(encrypted_copy, mainrom, mainrom_size);
	iga_u12_decode((UINT16 *)memory_region(REGION_GFX3), memory_region_length(REGION_GFX3), 0);
	iga_u16_decode((UINT16 *)memory_region(REGION_GFX3), memory_region_length(REGION_GFX3), 0);
	sprite_colour_decode((UINT16 *)memory_region(REGION_GFX4), memory_region_length(REGION_GFX4));
}
static DRIVER_INIT(orleg2)
{
	common_init(0);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x20020114, 0x20020117, 0, 0, orleg2_idle_r);
}
static DRIVER_INIT(kov2nl)
{
	common_init(0);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x20020470, 0x20020473, 0, 0, kov2nl_idle_r);
}
static DRIVER_INIT(ddpdojt)
{
	common_init(1);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x20000060, 0x20000063, 0, 0, ddpdojt_idle_r);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x20021e04, 0x20021e07, 0, 0, ddpdojt_idle2_r);
}
static DRIVER_INIT(kov3)
{
	common_init(2);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x200000b4, 0x200000b7, 0, 0, kov3_idle_r);
}
static DRIVER_INIT(kof98umh)
{
	common_init(0);
	memory_install_read32_handler(0, ADDRESS_SPACE_PROGRAM, 0x20000060, 0x20000063, 0, 0, kof98umh_idle_r);
}
static MACHINE_DRIVER_START(pgm2)
	MDRV_CPU_ADD_TAG("main",IGS036, 100000000)
	MDRV_CPU_PROGRAM_MAP(pgm2_map,0)
	MDRV_CPU_VBLANK_INT(pgm2_vblank,1)
	MDRV_FRAMES_PER_SECOND(59.08)
	MDRV_VBLANK_DURATION(0)
	MDRV_MACHINE_START(pgm2)
	MDRV_MACHINE_RESET(pgm2)
	MDRV_NVRAM_HANDLER(pgm2)
	MDRV_MEMCARD_HANDLER(pgm2)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_RGB_DIRECT | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(512,256)
	MDRV_VISIBLE_AREA(0,447,0,223)
	MDRV_GFXDECODE(pgm2_gfx)
	MDRV_PALETTE_LENGTH(0x1a00)
	MDRV_VIDEO_START(pgm2)
	MDRV_VIDEO_UPDATE(pgm2)
	MDRV_SPEAKER_STANDARD_STEREO("left","right")
	MDRV_SOUND_ADD(CUSTOM,22579200)
	MDRV_SOUND_CONFIG(ymz774_interface)
	MDRV_SOUND_ROUTE(0,"left",1.0)
	MDRV_SOUND_ROUTE(1,"right",1.0)
MACHINE_DRIVER_END
static MACHINE_DRIVER_START(pgm2_ramrom)
	MDRV_IMPORT_FROM(pgm2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pgm2_ramrom_map,0)
MACHINE_DRIVER_END
static MACHINE_DRIVER_START(pgm2_hires)
	MDRV_IMPORT_FROM(pgm2)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(pgm2_module_map,0)
	MDRV_VISIBLE_AREA(0,511,0,239)
MACHINE_DRIVER_END
static MACHINE_DRIVER_START(pgm2_lores)
	MDRV_IMPORT_FROM(pgm2)
	MDRV_FRAMES_PER_SECOND(15625.0/264.0)
	MDRV_VISIBLE_AREA(0,319,0,239)
MACHINE_DRIVER_END
GAME(2007, orleg2,   0, pgm2,        pgm2, orleg2,   ROT0,   "IGS", "Oriental Legend 2 (V104, Oversea)", 0)
GAME(2008, kov2nl,   0, pgm2,        pgm2, kov2nl,   ROT0,   "IGS", "Knights of Valour 2 New Legend / Sanguo Zhan Ji 2 Guang Sao Yu Jun (V302, Oversea)", 0)
GAME(2010, ddpdojt,  0, pgm2_ramrom, pgm2, ddpdojt,  ROT270, "IGS / Cave (Tong Li Animation license)", "DoDonPachi Dai-Ou-Jou Tamashii (V201, China)", 0)
GAME(2011, kov3,     0, pgm2_hires,  pgm2, kov3,     ROT0,   "IGS (Huatong license)", "Knights of Valour 3 / Sanguo Zhan Ji 3 (V104, China, Hong Kong, Taiwan)", 0)
GAME(2009, kof98umh, 0, pgm2_lores,  pgm2, kof98umh, ROT0,   "IGS / SNK Playmore / New Channel", "The King of Fighters '98: Ultimate Match HERO (China, V100, 09-08-23)", 0)
