/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * In addition, as a special exception, Andrea Mazzoleni
 * gives permission to link the code of this program with
 * the MAME library (or with modified versions of MAME that use the
 * same license as MAME), and distribute linked combinations including
 * the two.  You must obey the GNU General Public License in all
 * respects for all of the code used other than MAME.  If you modify
 * this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to
 * do so, delete this exception statement from your version.
 */

/*
 * This video driver is STRONGLY derived from the video drivers of the
 * VSyncMAME project by S.Sakamaki (Saka)
 */

#include "compil.h"
#include "cirrus.h"
#include "card.h"

struct cirrus_id {
	unsigned value;
	const char* name;
	int supported;
	int mul_min;
	int mul_max;
	int div_min;
	int div_max;
	int p_min;
	int p_max;
	long ref; /* kHz */
	long vco_min; /* kHz */
	long vco_max; /* kHz */
};

/*
INT 10 - Cirrus Logic BIOS - INQUIRE VGA TYPE
	AH = 12h
	BL = 80h
Return: AX = controller type in bits 13-0 (see #00029)
		bit 14: ???
		bit 15: ???
	BL = silicon revision number (bit 7 set if not available)
	BH = ???
		bit 2 set if using CL-GD 6340 LCD interface
SeeAlso: AH=12h/BL=81h, AH=12h/BL=82h, AH=12h/BL=85h, AH=12h/BL=9Ah, AH=12h/BL=A1h

(Table 00029)
Values for Cirrus Logic video controller type:
 0000h  no extended alternate select support
 0001h  reserved
 0002h  CL-GD510/520
 0003h  CL-GD610/620
 0004h  CL-GD5320
 0005h  CL-GD6410
 0006h  CL-GD5410
 0007h  CL-GD6420
 0008h  CL-GD6412
 0010h  CL-GD5401
 0011h  CL-GD5402
 0012h  CL-GD5420
 0013h  CL-GD5422
 0014h  CL-GD5424
 0015h  CL-GD5426
 0016h  CL-GD5420r1
 0017h  CL-GD5402r1
 0018h  CL-GD5428
 0019h  CL-GD5429
 0020h  CL-GD6205/15/25
 0021h  CL-GD6215
 0022h  CL-GD6225
 0023h  CL-GD6235
 0024h  CL-GD6245
 0030h  CL-GD5432
 0031h  CL-GD5434
 0032h  CL-GD5430
 0033h  CL-GD5434 rev. E and F
 0035h  CL-GD5440
 0036h  CL-GD5436
 0039h  CL-GD5446
 0040h  CL-GD6440
 0041h  CL-GD7542 (Nordic)
 0042h  CL-GD7543 (Viking)
 0043h  CL-GD7541 (Nordic Lite)
 0050h  CL-GD5452 (Northstar)
 0052h  CL-GD5452 (Northstar) ???
*/

/*
Values for Cirrus Logic PCI device code:
 0038h  CL-GD7548 Video Controller
 004Ch  CL-GD7556 64-bit Video/Graphics LCD/CRT Ctrlr
 00A0h  Cirrus 5430
 00A4h  Cirrus 5434-4
 00A8h  Cirrus 5434-8
 00ACh  Cirrus 5436
 00B8h  Cirrus GD5446
 00BCh  CL-GD5480 64-bit SGRAM GUI accelerator
 1100h  Cirrus 6729 PCMCIA Controller
 1110h  Cirrus 6832 PCMCIA/CardBus Ctrlr
 1200h  Cirrus 7542
 1202h  Cirrus 7543
 1204h  Cirrus 7541
 6001h  CS4610/4611 CrystalClear SoundFusion Audio Accelerator
*/

/*
From XFree 4.0.0
#define PCI_CHIP_GD7548         0x0038
#define PCI_CHIP_GD7555         0x0040
#define PCI_CHIP_GD5430         0x00A0
#define PCI_CHIP_GD5434_4       0x00A4
#define PCI_CHIP_GD5434_8       0x00A8
#define PCI_CHIP_GD5436         0x00AC
#define PCI_CHIP_GD5446         0x00B8
#define PCI_CHIP_GD5480         0x00BC
#define PCI_CHIP_GD5462         0x00D0
#define PCI_CHIP_GD5464         0x00D4
#define PCI_CHIP_GD5464BD       0x00D5
#define PCI_CHIP_GD5465         0x00D6
#define PCI_CHIP_6729           0x1100
#define PCI_CHIP_6832           0x1110
#define PCI_CHIP_GD7542         0x1200
#define PCI_CHIP_GD7543         0x1202
#define PCI_CHIP_GD7541         0x1204
*/

#define CIRRUS_CONF_UNSUPPORTED 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#define CIRRUS_CONF_GENERIC 1, 0x10, 0x7E, 0xA, 0x1F, 0, 1, 14318, 28636, 110000

static struct cirrus_id cirrus_id_list[] = {
{ 0x0002, "Cirrus Logic CL-GD510/520", CIRRUS_CONF_UNSUPPORTED },
{ 0x0003, "Cirrus Logic CL-GD610/620", CIRRUS_CONF_UNSUPPORTED },
{ 0x0004, "Cirrus Logic CL-GD5320", CIRRUS_CONF_UNSUPPORTED },
{ 0x0005, "Cirrus Logic CL-GD6410", CIRRUS_CONF_UNSUPPORTED },
{ 0x0006, "Cirrus Logic CL-GD5410", CIRRUS_CONF_GENERIC },
{ 0x0007, "Cirrus Logic CL-GD6420", CIRRUS_CONF_UNSUPPORTED },
{ 0x0008, "Cirrus Logic CL-GD6412", CIRRUS_CONF_UNSUPPORTED },
{ 0x0010, "Cirrus Logic CL-GD5401", CIRRUS_CONF_GENERIC },
{ 0x0011, "Cirrus Logic CL-GD5402", CIRRUS_CONF_GENERIC },
{ 0x0012, "Cirrus Logic CL-GD5420", CIRRUS_CONF_GENERIC },
{ 0x0013, "Cirrus Logic CL-GD5422", CIRRUS_CONF_GENERIC },
{ 0x0014, "Cirrus Logic CL-GD5424", CIRRUS_CONF_GENERIC },
{ 0x0015, "Cirrus Logic CL-GD5426", CIRRUS_CONF_GENERIC },
{ 0x0016, "Cirrus Logic CL-GD5420r1", CIRRUS_CONF_GENERIC },
{ 0x0017, "Cirrus Logic CL-GD5402r1", CIRRUS_CONF_GENERIC },
{ 0x0018, "Cirrus Logic CL-GD5428", CIRRUS_CONF_GENERIC },
{ 0x0019, "Cirrus Logic CL-GD5429", CIRRUS_CONF_GENERIC },
{ 0x0020, "Cirrus Logic CL-GD6205/15/25", CIRRUS_CONF_UNSUPPORTED },
{ 0x0021, "Cirrus Logic CL-GD6215", CIRRUS_CONF_UNSUPPORTED },
{ 0x0022, "Cirrus Logic CL-GD6225", CIRRUS_CONF_UNSUPPORTED },
{ 0x0023, "Cirrus Logic CL-GD6235", CIRRUS_CONF_UNSUPPORTED },
{ 0x0024, "Cirrus Logic CL-GD6245", CIRRUS_CONF_UNSUPPORTED },
{ 0x0030, "Cirrus Logic CL-GD5432", CIRRUS_CONF_GENERIC },
{ 0x0031, "Cirrus Logic CL-GD5434", CIRRUS_CONF_GENERIC }, /* TESTED */
{ 0x0032, "Cirrus Logic CL-GD5430", CIRRUS_CONF_GENERIC },
{ 0x0033, "Cirrus Logic CL-GD5434 rev. E and F", CIRRUS_CONF_GENERIC },
{ 0x0035, "Cirrus Logic CL-GD5440", CIRRUS_CONF_GENERIC },
{ 0x0036, "Cirrus Logic CL-GD5436", CIRRUS_CONF_GENERIC },
{ 0x0039, "Cirrus Logic CL-GD5446", CIRRUS_CONF_GENERIC },
{ 0x0040, "Cirrus Logic CL-GD6440", CIRRUS_CONF_UNSUPPORTED },
{ 0x0041, "Cirrus Logic CL-GD7542 (Nordic)", CIRRUS_CONF_UNSUPPORTED },
{ 0x0042, "Cirrus Logic CL-GD7543 (Viking)", CIRRUS_CONF_UNSUPPORTED },
{ 0x0043, "Cirrus Logic CL-GD7541 (Nordic Lite)", CIRRUS_CONF_UNSUPPORTED },
{ 0x0050, "Cirrus Logic CL-GD5452 (Northstar)", CIRRUS_CONF_GENERIC },
{ 0, 0, CIRRUS_CONF_UNSUPPORTED }
};

static struct cirrus_id* cirrus_card = 0;

const char* cirrus_driver(void)
{
	return cirrus_card->name;
}

static void cirrus_pll_set(void)
{
	BYTE d0;

	d0 = card_in(0x3CC);
	d0 = d0 | 0x0C;
	card_out(0x3C2, d0);
}

static void cirrus_doublescan_set(int flag)
{
	BYTE d0 = card_crtc_get(0x09);
	if(flag == 0){
		card_crtc_set(0x09, d0 & 0xE0);
	}else{
		card_crtc_set(0x09, (d0 & 0xE0) | 0x80);
	}
}

static void cirrus_interlace_set(int flag, card_crtc STACK_PTR *cp)
{
	int d0;

	if(flag == 0){
		d0 = card_crtc_get(0x1A);
		if((d0 & 0x01) != 0){
			d0 = d0 & ~0x01;
			card_crtc_set(0x1A, d0);
		}
	}else{
		d0 = ((cp->HTotal >> 3) - 5) / 2;
		d0 = (d0 * cp->interlaceratio) / 100;
		card_crtc_set(0x19, d0); /* interlace offset */
		d0 = card_crtc_get(0x1A);
		if((d0 & 0x01) == 0){
			d0 = d0 | 0x01;
			card_crtc_set(0x1A, d0);
		}
	}
}

static void cirrus_ext_set(card_crtc STACK_PTR *cp)
{
	int d0;

	d0 = card_crtc_get(0x1B);
	d0 = card_bitmov(d0, 5, 1, 0);
	d0 = card_bitmov(d0, 7, 1, 0);
	card_crtc_set(0x1B, d0); /* enable Blank End ext */

	d0 = 0;
	d0 = card_bitmov(d0, 0, cp->VSStart, 10);
	d0 = card_bitmov(d0, 1, cp->VBStart, 10);
	d0 = card_bitmov(d0, 2, cp->VDisp, 10);
	d0 = card_bitmov(d0, 3, cp->VTotal, 10);
	d0 = card_bitmov(d0, 4, (cp->HSStart >> 3), 8);
	d0 = card_bitmov(d0, 5, (cp->HBStart >> 3), 8);
	d0 = card_bitmov(d0, 6, (cp->HDisp   >> 3) - 1, 8);
	d0 = card_bitmov(d0, 7, (cp->HTotal  >> 3) - 5, 8);
	card_crtc_set(0x1E, d0); /* ext horz vert reg */

	d0 = card_crtc_get(0x1A);
	d0 = card_bitmov(d0, 4, (cp->HBEnd >> 3), 6);
	d0 = card_bitmov(d0, 5, (cp->HBEnd >> 3), 7);
	d0 = card_bitmov(d0, 6, cp->VBEnd, 8);
	d0 = card_bitmov(d0, 7, cp->VBEnd, 9);
	card_crtc_set(0x1A, d0); /* ext Blank End reg */

	cirrus_doublescan_set(cp->doublescan);
	cirrus_interlace_set(cp->interlace, cp);
	card_polarity_set(cp->hpolarity, cp->vpolarity);
}

/*
3C4h index 0Bh (R/W):  VCLK 0 Numerator Register
bit  0-6  VCLK 0 Numerator bits 0-6
Note: See index 1Bh for the frequency calculation

3C4h index 1Bh (R/W):  VCLK 0 Denominator & Post
bit    0  VCLK 0 Post Scalar bit. Divide clock by 2 if set
	1-5  VCLK 0 Denominator Data
Note: The clock is (14.31818MHz * numerator (index 0Bh))/Denominator.
	Divide by 2 if the Post Scalar bit is set.
	3C2h bits 2-3 selects between VCLK0, 1, 2 and 3
	Max frequency supported:
	42MHz:    5401
	65MHz:    5402 and 5420 (original versions)
	75MHz     5402 rev 1 and 5420 rev 1 (and probably later revs)
	80MHz     5422-5428
	86MHz:    5429
	Programming a higher clock frequency may work, give an unstable image
	or fail totally.
*/

static void cirrus_clock_set(int Num, int Den, int PS)
{
	BYTE SR0E;
	BYTE SR1E;

	cirrus_pll_set();

	SR0E = (card_seq_get(0x0E) & 0x80) | Num;
	card_seq_set(0x0E, SR0E);

	SR1E = (card_seq_get(0x1E) & 0xC0) | (Den << 1) | PS;
	card_seq_set(0x1E, SR1E);
}

int cirrus_detect(void)
{
	__dpmi_regs r;
	int i;
	
	r.d.eax = 0x00001200;
	r.d.ebx = 0x00000080;
	__dpmi_int(0x10, &r);

	r.d.eax &= 0x3FFF;
	for(i=0;cirrus_id_list[i].name;++i) {
		if (cirrus_id_list[i].value == r.d.eax)
			break;
	}

	if (!cirrus_id_list[i].name){
		return 0;
	}

	CARD_LOG(("cirrus: found %s, bios ID %04x\n", cirrus_id_list[i].name, cirrus_id_list[i].value));

	if (!cirrus_id_list[i].supported) {
		CARD_LOG(("cirrus: card not supported\n"));
		return 0;
	}

	cirrus_card = cirrus_id_list + i;

	return 1;
}

void cirrus_reset(void)
{
}

int cirrus_set(const card_crtc STACK_PTR* _cp, const card_mode STACK_PTR* cm, const card_mode STACK_PTR* co)
{
	card_crtc cp = *_cp;
	int pll_mul, pll_div, pll_p;
	unsigned d0;

	if (!card_compatible_mode(cm, co)) {
		CARD_LOG(("cirrus: incompatible mode\n"));
		return 0;
	}

	/* Unlock */
	card_seq_set(0x6, 0x12); /* Unlock cirrus special */

/*
3C4h index  7  (R/W):  Extended Sequencer Mode
bit    0  Enable High-Resolution 256 Color modes if set.
	For the 5429 this disables the Set/Reset logic (3CEh index 0 and 1)
	1-2  (542x, 02)  or
	1-3  (543x) Select CRTC Character Clock Divider.
		0: Normal operation
		1: Clock/2 for 16bit pixels. In this mode the video clock is
		programmed for twice the number of pixels (= the number of
		bytes) and the horizontal timing in units of 8 pixels as in
		standard VGA modes.
		2: Clock/3 for 24bit pixels. In this mode the video clock is
		programmed for 3 times the number of pixels (= the number of
		bytes) and the horizontal timing in units of 8 pixels as in
		standard VGA modes.
		3: (5426-3x only) 16bit pixel data at Pixel Rate. In this mode
		the video clock is programmed for the number of pixels (= the
		number of bytes) and the horizontal timing in units of 8
		pixels as in standard VGA modes.
		4: (543x only) 32bit pixel data at Pixel Rate. In this mode the
		video clock is programmed for the number of pixels (= the
		number of bytes) and the horizontal timing in units of 8
		pixels as in standard VGA modes. This works as a 4bytes per
		pixel mode, with the 4th byte being ignored.
	4-7  (5422-3x) Select 1M Video Memory Mapping.
		The address in 1MB units the Video Memory is mapped at (0=no
		mapping). On the 5426-3x if 3CEh index 0Bh is set bit 4 is ignored
		and the 2MB buffer is mapped at an even MB block.
		When in planar modes or using x8 or x16 addressing (3CEh index 0Bh
		bit 1 set) or if less than the max amount of memory is installed
		the memory block will wrap so that there are 2, 4, 8... instances
*/
	d0 = (card_seq_get(0x7) & 0xE) >> 1;
	CARD_LOG(("cirrus: clock divider:%d\n", d0));
	if (d0 == 1) {
		CARD_LOG(("cirrus: double the dotclock for clock divider activated\n"));
		cp.dotclockHz *= 2;
	}

	d0 = card_divider_get();
	CARD_LOG(("cirrus: seqdiv:%d\n", d0));
	if (d0 == 2) {
		CARD_LOG(("cirrus: clear seqdiv\n"));
		card_divider_set(1);
		CARD_LOG(("cirrus: halve the offset for seqdiv\n"));
		card_crtc_set(0x13, card_crtc_get(0x13) / 2);
	}

	d0 = card_crtc_get(0x13);
	CARD_LOG(("cirrus: offset:%d\n", d0));

	cp.dotclockHz = card_clock_compute(cp.dotclockHz, cirrus_card->mul_min,  cirrus_card->mul_max, cirrus_card->div_min,  cirrus_card->div_max, cirrus_card->p_min,  cirrus_card->p_max, cirrus_card->ref, cirrus_card->vco_min,  cirrus_card->vco_max, &pll_mul, &pll_div, &pll_p, 0);
	if (cp.dotclockHz < 0)
		return 0;

	card_signal_disable();
	card_generic_all_set(&cp);
	cirrus_ext_set(&cp);

	cirrus_clock_set(pll_mul, pll_div, pll_p);
	card_signal_enable();

	return 1;
}

