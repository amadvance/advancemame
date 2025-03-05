/***************************************************************************

    AmeriDarts      (c) 1989 Ameri Corporation
    Cool Pool       (c) 1992 Catalina
    9 Ball Shootout (c) 1993 E-Scape/Bundra

    driver by Nicola Salmoria and Aaron Giles


    The main cpu is a 34010; it is encrypted in 9 Ball Shootout.

    The second CPU in AmeriDarts is a 32015, whose built-in ROM hasn't
    been read. A simulation of the I/O behavior is included, but since the
    second CPU controls sound, there is no sound.

    The second CPU in Cool Pool and 9 Ball Shootout is a 320C26; the code
    is the same in the two games.

    Cool Pool:
    - The checksum test routine is wrong, e.g. when it says to be testing
      4U/8U it is actually reading 4U/8U/3U/7U, when testing 3U/7U it
      actually reads 2U/6U/1U/5U. The placement cannot therefore be exactly
      determined by the check passing.

***************************************************************************/

#include "driver.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/tms34010/34010ops.h"
#include "cpu/tms32025/tms32025.h"
#include "vidhrdw/tlc34076.h"
#include "sound/dac.h"



/*************************************
 *
 *  Local variables
 *
 *************************************/

static UINT16 *vram_base;

static UINT16 dpyadr;
static int dpyadrscan;
static int last_dpyint;

static UINT8 cmd_pending;
static UINT16 iop_cmd;
static UINT16 iop_answer;
static int iop_romaddr;
static UINT8 amerdart_iop_echo;

static const UINT16 nvram_unlock_seq[] =
{
	0x3fb, 0x3fb, 0x3f8, 0x3fc, 0x3fa, 0x3fe, 0x3f9, 0x3fd, 0x3fb, 0x3ff
};
#define NVRAM_UNLOCK_SEQ_LEN (sizeof(nvram_unlock_seq) / sizeof(nvram_unlock_seq[0]))
static UINT16 nvram_write_seq[NVRAM_UNLOCK_SEQ_LEN];
static UINT8 nvram_write_enable;
static mame_timer *nvram_write_timer;



/*************************************
 *
 *  Prototypes
 *
 *************************************/

static void nvram_write_timeout(int param);




/*************************************
 *
 *  Video updates
 *
 *************************************/

static VIDEO_UPDATE( amerdart )
{
	UINT16 *base = &vram_base[TOWORD(0x800) + cliprect->min_y * TOWORD(0x800)];
	int x, y;

	/* if we're blank, just blank the screen */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* update the palette */
	for (x = 0; x < 16; x++)
	{
		UINT16 pal = vram_base[x];
		int r = (pal >> 4) & 0x0f;
		int g = (pal >> 8) & 0x0f;
		int b = (pal >> 12) & 0x0f;
		palette_set_color(x, (r << 4) | r, (g << 4) | g, (b << 4) | b);
	}

	/* loop over scanlines */
	for (y = cliprect->min_y; y <= cliprect->max_y; y++)
	{
		UINT16 *dest = &((UINT16 *)bitmap->line[y])[cliprect->min_x];

		/* render 4bpp data */
		for (x = cliprect->min_x; x <= cliprect->max_x; x += 4)
		{
			UINT16 pixels = base[x / 4];
			*dest++ = (pixels >> 0) & 15;
			*dest++ = (pixels >> 4) & 15;
			*dest++ = (pixels >> 8) & 15;
			*dest++ = (pixels >> 12) & 15;
		}
		base += TOWORD(0x800);
	}
}


static VIDEO_UPDATE( coolpool )
{
	UINT16 dpytap, dudate, dumask, vsblnk, veblnk;
	int startscan = cliprect->min_y, endscan = cliprect->max_y;
	int x, y, offset, scanoffs = 0;

	/* if we're blank, just blank the screen */
	if (tms34010_io_display_blanked(0))
	{
		fillbitmap(bitmap, get_black_pen(), cliprect);
		return;
	}

	/* fetch current scanline advance and column offset values */
	/* these are used aggressively in 9ballsht opening scene */
	cpuintrf_push_context(0);
	dudate = (tms34010_io_register_r(REG_DPYCTL, 0) & 0x03fc) << 4;
	dumask = dudate - 1;
	dpytap = tms34010_io_register_r(REG_DPYTAP, 0) & 0x3fff & dumask;
	vsblnk = tms34010_io_register_r(REG_VSBLNK, 0);
	veblnk = tms34010_io_register_r(REG_VEBLNK, 0);
	cpuintrf_pop_context();

	/* adjust drawing area based on blanking (9 ball shootout tweaks it) */
	if (vsblnk > veblnk && vsblnk - veblnk < Machine->drv->screen_height)
	{
		/* compute starting scanline offset (assume centered) */
		scanoffs = ((Machine->visible_area.max_y - Machine->visible_area.min_y + 1) - (vsblnk - veblnk)) / 2;

		/* adjust start/end scanlines */
		if (startscan != Machine->visible_area.min_y)
			startscan += scanoffs;
		endscan += scanoffs;
		if (endscan >= Machine->visible_area.max_y)
			endscan = Machine->visible_area.max_y;
	}

	/* compute the offset */
	offset = (dpyadr << 4) + dpytap;

	/* adjust for when DPYADR was written */
	if (cliprect->min_y - scanoffs >= dpyadrscan)
		offset += (cliprect->min_y - scanoffs - dpyadrscan) * dudate;

	/* loop over scanlines */
	for (y = startscan; y <= endscan; y++)
	{
		UINT16 *dest = &((UINT16 *)bitmap->line[y])[cliprect->min_x];

		/* if we're in outer bands, just clear */
		if (y < Machine->visible_area.min_y + scanoffs || y > Machine->visible_area.max_y - scanoffs)
			memset(dest, 0, (cliprect->max_x - cliprect->min_x + 1) * 2);

		/* render 8bpp data */
		else
		{
			for (x = cliprect->min_x; x <= cliprect->max_x; x += 2)
			{
				UINT16 pixels = vram_base[(offset & ~dumask & TOWORD(0x1fffff)) | ((offset + x/2) & dumask)];
				*dest++ = (pixels >> 0) & 0xff;
				*dest++ = (pixels >> 8) & 0xff;
			}
			offset += dudate;
		}
	}
}



/*************************************
 *
 *  Shift register access
 *
 *************************************/

static void coolpool_to_shiftreg(unsigned int address, UINT16 *shiftreg)
{
	memcpy(shiftreg, &vram_base[TOWORD(address) & ~TOWORD(0xfff)], TOBYTE(0x1000));
}


static void coolpool_from_shiftreg(unsigned int address, UINT16 *shiftreg)
{
	memcpy(&vram_base[TOWORD(address) & ~TOWORD(0xfff)], shiftreg, TOBYTE(0x1000));
}



/*************************************
 *
 *  Mid-screen addressing updates
 *
 *************************************/

static void coolpool_dpyint_callback(int scanline)
{
	/* log when we got the DPYINT so that changes are tagged to this scanline */
	last_dpyint = scanline + 1;
	if (scanline < Machine->visible_area.max_y)
		force_partial_update(scanline);
}


static void coolpool_reset_dpyadr(int param)
{
	/* at the start of each screen, re-fetch the DPYADR */
	cpuintrf_push_context(0);
	dpyadr = ~tms34010_io_register_r(REG_DPYADR, 0) & 0xfffc;
	dpyadrscan = last_dpyint = 0;
	cpuintrf_pop_context();

	/* come again next screen */
	timer_set(cpu_getscanlinetime(0), 0, coolpool_reset_dpyadr);
}


static WRITE16_HANDLER( coolpool_34010_io_register_w )
{
	tms34010_io_register_w(offset, data, mem_mask);

	/* track writes to the DPYADR register which takes effect on the following scanline */
	if (offset == REG_DPYADR)
	{
		dpyadr = ~data & 0xfffc;
		dpyadrscan = last_dpyint;
	}
}



/*************************************
 *
 *  Game initialzation
 *
 *************************************/

static MACHINE_RESET( amerdart )
{
	nvram_write_enable = 0;
	nvram_write_timer = timer_alloc(nvram_write_timeout);
}


static MACHINE_RESET( coolpool )
{
	timer_set(cpu_getscanlinetime(0), 0, coolpool_reset_dpyadr);
	tlc34076_reset(6);
	nvram_write_enable = 0;
	nvram_write_timer = timer_alloc(nvram_write_timeout);
}



/*************************************
 *
 *  NVRAM writes with thrash protect
 *
 *************************************/

static void nvram_write_timeout(int param)
{
	nvram_write_enable = 0;
}


static WRITE16_HANDLER( nvram_thrash_w )
{
	/* keep track of the last few writes */
	memmove(&nvram_write_seq[0], &nvram_write_seq[1], (NVRAM_UNLOCK_SEQ_LEN - 1) * sizeof(nvram_write_seq[0]));
	nvram_write_seq[NVRAM_UNLOCK_SEQ_LEN - 1] = offset;

	/* if they match the unlock sequence, enable writes and set a timeout */
	if (!memcmp(nvram_unlock_seq, nvram_write_seq, sizeof(nvram_unlock_seq)))
	{
		nvram_write_enable = 1;
		timer_adjust(nvram_write_timer, TIME_IN_MSEC(1000), 0, 0);
	}
}


static WRITE16_HANDLER( nvram_data_w )
{
	/* only the low 8 bits matter */
	if (ACCESSING_LSB)
	{
		if (nvram_write_enable)
			generic_nvram16[offset] = data & 0xff;
	}
}


static WRITE16_HANDLER( nvram_thrash_data_w )
{
	nvram_data_w(offset, data, mem_mask);
	nvram_thrash_w(offset, data, mem_mask);
}



/*************************************
 *
 *  AmeriDarts fake IOP handling
 *
 *************************************/

static WRITE16_HANDLER( amerdart_misc_w )
{
	logerror("%08x:IOP_reset_w %04x\n",activecpu_get_pc(),data);

	coin_counter_w(0, ~data & 0x0001);
	coin_counter_w(1, ~data & 0x0002);

	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x0400) ? ASSERT_LINE : CLEAR_LINE);

	/* bits 10-15 are counted down over time */
	if (data & 0x0400) amerdart_iop_echo = 1;
}


static void amerdart_iop_response(int param)
{
	/* echo values until we get 0x19 */
	iop_answer = iop_cmd;
	if (amerdart_iop_echo && iop_cmd != 0x19)
	{
		cpunum_set_input_line(0, 1, ASSERT_LINE);
		return;
	}
	amerdart_iop_echo = 0;

	/* rest is based off the command */
	switch (iop_cmd)
	{
		default:
			logerror("Unknown IOP command %04X\n", iop_cmd);

		case 0x000:
		case 0x019:
		case 0x600:
		case 0x60f:
		case 0x6f0:
		case 0x6ff:
			iop_answer = 0x6c00;
			break;

		case 0x100:
			iop_answer = (INT8)(-readinputportbytag("YAXIS2") - readinputportbytag("XAXIS2")) << 6;
			break;
		case 0x101:
			iop_answer = (INT8)(-readinputportbytag("YAXIS2") + readinputportbytag("XAXIS2")) << 6;
			break;
		case 0x102:
			iop_answer = (INT8)(-readinputportbytag("YAXIS1") - readinputportbytag("XAXIS1")) << 6;
			break;
		case 0x103:
			iop_answer = (INT8)(-readinputportbytag("YAXIS1") + readinputportbytag("XAXIS1")) << 6;
			break;

		case 0x500:
			iop_answer = readinputport(0);
			break;
	}

	cpunum_set_input_line(0, 1, ASSERT_LINE);
}


static WRITE16_HANDLER( amerdart_iop_w )
{
	logerror("%08x:IOP write %04x\n", activecpu_get_pc(), data);
	COMBINE_DATA(&iop_cmd);
	timer_set(TIME_IN_USEC(100), 0, amerdart_iop_response);
}



/*************************************
 *
 *  Cool Pool IOP control
 *
 *************************************/

static WRITE16_HANDLER( coolpool_misc_w )
{
	logerror("%08x:IOP_reset_w %04x\n",activecpu_get_pc(),data);

	coin_counter_w(0, ~data & 0x0001);
	coin_counter_w(1, ~data & 0x0002);

	cpunum_set_input_line(1, INPUT_LINE_RESET, (data & 0x0400) ? ASSERT_LINE : CLEAR_LINE);
}



/*************************************
 *
 *  Cool Pool IOP communications
 *  (from TMS34010 side)
 *
 *************************************/

static void deferred_iop_w(int data)
{
	iop_cmd = data;
	cmd_pending = 1;
	cpunum_set_input_line(1, 0, HOLD_LINE);	/* ???  I have no idea who should generate this! */
										/* the DSP polls the status bit so it isn't strictly */
										/* necessary to also have an IRQ */
	cpu_boost_interleave(0, TIME_IN_USEC(50));
}


static WRITE16_HANDLER( coolpool_iop_w )
{
	logerror("%08x:IOP write %04x\n", activecpu_get_pc(), data);
	timer_set(TIME_NOW, data, deferred_iop_w);
}


static READ16_HANDLER( coolpool_iop_r )
{
	logerror("%08x:IOP read %04x\n",activecpu_get_pc(),iop_answer);
	cpunum_set_input_line(0, 1, CLEAR_LINE);

	return iop_answer;
}



/*************************************
 *
 *  Cool Pool IOP communications
 *  (from IOP side)
 *
 *************************************/

static READ16_HANDLER( dsp_cmd_r )
{
	cmd_pending = 0;
	logerror("%08x:IOP cmd_r %04x\n",activecpu_get_pc(),iop_cmd);
	return iop_cmd;
}


static WRITE16_HANDLER( dsp_answer_w )
{
	logerror("%08x:IOP answer %04x\n",activecpu_get_pc(),data);
	iop_answer = data;
	cpunum_set_input_line(0, 1, ASSERT_LINE);
}


static READ16_HANDLER( dsp_bio_line_r )
{
	return cmd_pending ? CLEAR_LINE : ASSERT_LINE;
}


static READ16_HANDLER( dsp_hold_line_r )
{
	return CLEAR_LINE;	/* ??? */
}



/*************************************
 *
 *  IOP ROM and DAC access
 *
 *************************************/

static READ16_HANDLER( dsp_rom_r )
{
	UINT8 *rom = memory_region(REGION_USER2);
	return rom[iop_romaddr & (memory_region_length(REGION_USER2) - 1)];
}


static WRITE16_HANDLER( dsp_romaddr_w )
{
	switch (offset)
	{
		case 0:
			iop_romaddr = (iop_romaddr & 0xffff00) | (data >> 8);
			break;

		case 1:
			iop_romaddr = (iop_romaddr & 0x0000ff) | (data << 8);
			break;
	}
}


WRITE16_HANDLER( dsp_dac_w )
{
	DAC_signed_data_16_w(0,(INT16)(data << 4) + 0x8000);
}



/*************************************
 *
 *  Cool Pool trackball inputs
 *
 *************************************/

static READ16_HANDLER( coolpool_input_r )
{
	static UINT8 oldx, oldy;
	static UINT16 lastresult;

	int result = (readinputportbytag("IN1") & 0x00ff) | (lastresult & 0xff00);
	UINT8 newx = readinputportbytag("XAXIS");
	UINT8 newy = readinputportbytag("YAXIS");
	int dx = (INT8)(newx - oldx);
	int dy = (INT8)(newy - oldy);

	if (dx < 0)
	{
		oldx--;
		switch (result & 0x300)
		{
			case 0x000:	result ^= 0x200;	break;
			case 0x100:	result ^= 0x100;	break;
			case 0x200:	result ^= 0x100;	break;
			case 0x300:	result ^= 0x200;	break;
		}
	}
	if (dx > 0)
	{
		oldx++;
		switch (result & 0x300)
		{
			case 0x000:	result ^= 0x100;	break;
			case 0x100:	result ^= 0x200;	break;
			case 0x200:	result ^= 0x200;	break;
			case 0x300:	result ^= 0x100;	break;
		}
	}

	if (dy < 0)
	{
		oldy--;
		switch (result & 0xc00)
		{
			case 0x000:	result ^= 0x800;	break;
			case 0x400:	result ^= 0x400;	break;
			case 0x800:	result ^= 0x400;	break;
			case 0xc00:	result ^= 0x800;	break;
		}
	}
	if (dy > 0)
	{
		oldy++;
		switch (result & 0xc00)
		{
			case 0x000:	result ^= 0x400;	break;
			case 0x400:	result ^= 0x800;	break;
			case 0x800:	result ^= 0x800;	break;
			case 0xc00:	result ^= 0x400;	break;
		}
	}

//  logerror("%08X:read port 7 (X=%02X Y=%02X oldX=%02X oldY=%02X res=%04X)\n", activecpu_get_pc(),
//      newx, newy, oldx, oldy, result);
	lastresult = result;
	return result;
}



/*************************************
 *
 *  Main Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( amerdart_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x000fffff) AM_RAM AM_BASE(&vram_base)
	AM_RANGE(0x04000000, 0x0400000f) AM_WRITE(amerdart_misc_w)
	AM_RANGE(0x05000000, 0x0500000f) AM_READWRITE(coolpool_iop_r, amerdart_iop_w)
	AM_RANGE(0x06000000, 0x06007fff) AM_READWRITE(MRA16_RAM, nvram_thrash_data_w) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, coolpool_34010_io_register_w)
	AM_RANGE(0xffb00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( coolpool_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&vram_base)
	AM_RANGE(0x01000000, 0x010000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)	// IMSG176P-40
	AM_RANGE(0x02000000, 0x020000ff) AM_READWRITE(coolpool_iop_r, coolpool_iop_w)
	AM_RANGE(0x03000000, 0x0300000f) AM_WRITE(coolpool_misc_w)
	AM_RANGE(0x03000000, 0x03ffffff) AM_ROM AM_REGION(REGION_GFX1, 0)
	AM_RANGE(0x06000000, 0x06007fff) AM_READWRITE(MRA16_RAM, nvram_thrash_data_w) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, coolpool_34010_io_register_w)
	AM_RANGE(0xffe00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END


static ADDRESS_MAP_START( nballsht_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000000, 0x001fffff) AM_RAM AM_BASE(&vram_base)
	AM_RANGE(0x02000000, 0x020000ff) AM_READWRITE(coolpool_iop_r, coolpool_iop_w)
	AM_RANGE(0x03000000, 0x0300000f) AM_WRITE(coolpool_misc_w)
	AM_RANGE(0x04000000, 0x040000ff) AM_READWRITE(tlc34076_lsb_r, tlc34076_lsb_w)	// IMSG176P-40
	AM_RANGE(0x06000000, 0x0601ffff) AM_MIRROR(0x00020000) AM_READWRITE(MRA16_RAM, nvram_thrash_data_w) AM_BASE(&generic_nvram16) AM_SIZE(&generic_nvram_size)
	AM_RANGE(0xc0000000, 0xc00001ff) AM_READWRITE(tms34010_io_register_r, coolpool_34010_io_register_w)
	AM_RANGE(0xff000000, 0xff7fffff) AM_ROM AM_REGION(REGION_GFX1, 0)
	AM_RANGE(0xffc00000, 0xffffffff) AM_ROM AM_REGION(REGION_USER1, 0)
ADDRESS_MAP_END



/*************************************
 *
 *  DSP Memory maps
 *
 *************************************/

static ADDRESS_MAP_START( amerdart_dsp_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x011f) AM_RAM
	AM_RANGE(0x8000, 0xffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( dsp_program_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START( dsp_io_map, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00, 0x01) AM_WRITE(dsp_romaddr_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(dsp_cmd_r, dsp_answer_w)
	AM_RANGE(0x03, 0x03) AM_WRITE(dsp_dac_w)
	AM_RANGE(0x04, 0x04) AM_READ(dsp_rom_r)
	AM_RANGE(0x05, 0x05) AM_READ(input_port_0_word_r)
	AM_RANGE(0x07, 0x07) AM_READ(input_port_1_word_r)
	AM_RANGE(TMS32025_BIO, TMS32025_BIO) AM_READ(dsp_bio_line_r)
	AM_RANGE(TMS32025_HOLD, TMS32025_HOLD) AM_READ(dsp_hold_line_r)
//  AM_RANGE(TMS32025_HOLDA, TMS32025_HOLDA) AM_WRITE(dsp_HOLDA_signal_w)
ADDRESS_MAP_END



/*************************************
 *
 *  Input ports
 *
 *************************************/

INPUT_PORTS_START( amerdart )
	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_BUTTON1 )	PORT_PLAYER(1)
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_HIGH )
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0xff00, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START_TAG("XAXIS1")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(1)

	PORT_START_TAG("YAXIS1")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(1)

	PORT_START_TAG("XAXIS2")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(2)

	PORT_START_TAG("YAXIS2")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_RESET PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( 9ballsht )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x0300, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_LOW )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1)
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1)
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2)
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2)
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
INPUT_PORTS_END


INPUT_PORTS_START( coolpool )
	PORT_START_TAG("IN0")
    PORT_BIT( 0x00ff, IP_ACTIVE_HIGH, IPT_UNKNOWN )
    PORT_BIT( 0x0700, IP_ACTIVE_LOW, IPT_UNKNOWN )
    PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) //P2 English
    PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2) //P2 Lock
    PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1) //P1 Lock
    PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) //P1 English
    PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_UNKNOWN )
	

	PORT_START_TAG("IN1")
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_SERVICE_NO_TOGGLE( 0x0010, IP_ACTIVE_LOW )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0xff00, IP_ACTIVE_LOW, IPT_SPECIAL )

	PORT_START_TAG("XAXIS")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10)

	PORT_START_TAG("YAXIS")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(10) PORT_REVERSE
INPUT_PORTS_END



/*************************************
 *
 *  34010 configuration
 *
 *************************************/

static struct tms34010_config tms_config =
{
	0,								/* halt on reset */
	NULL,							/* generate interrupt */
	coolpool_to_shiftreg,			/* write to shiftreg function */
	coolpool_from_shiftreg,			/* read from shiftreg function */
	NULL,							/* display address changed */
	coolpool_dpyint_callback		/* display interrupt callback */
};



/*************************************
 *
 *  Machine drivers
 *
 *************************************/

MACHINE_DRIVER_START( amerdart )

	/* basic machine hardware */
	MDRV_CPU_ADD(TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(amerdart_map,0)

	MDRV_CPU_ADD(TMS32010, 15000000/8)
	MDRV_CPU_FLAGS(CPU_DISABLE)
	MDRV_CPU_PROGRAM_MAP(amerdart_dsp_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_MACHINE_RESET(amerdart)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(320, 261)	/* ??? */
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_PALETTE_LENGTH(16)

	MDRV_VIDEO_UPDATE(amerdart)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( coolpool )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", TMS34010, 40000000/TMS34010_CLOCK_DIVIDER)
	MDRV_CPU_CONFIG(tms_config)
	MDRV_CPU_PROGRAM_MAP(coolpool_map,0)

	MDRV_CPU_ADD(TMS32026,40000000)
	MDRV_CPU_PROGRAM_MAP(dsp_program_map,0)
	MDRV_CPU_IO_MAP(dsp_io_map,0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(1000000 * (261 - 240) / (261 * 60))
	MDRV_MACHINE_RESET(coolpool)
	MDRV_NVRAM_HANDLER(generic_0fill)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN)
	MDRV_SCREEN_SIZE(320, 261)	/* ??? */
	MDRV_VISIBLE_AREA(0, 320-1, 0, 240-1)
	MDRV_PALETTE_LENGTH(256)

	MDRV_VIDEO_UPDATE(coolpool)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(DAC, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( 9ballsht )
	MDRV_IMPORT_FROM(coolpool)

	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(nballsht_map,0)
MACHINE_DRIVER_END



/*************************************
 *
 *  ROM definitions
 *
 *************************************/

ROM_START( amerdart )
	ROM_REGION16_LE( 0x0a0000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "u31",  0x000001, 0x10000, CRC(9628c422) SHA1(46b71acc746760962e34e9d7876f9499ea7d5c7c) )
	ROM_LOAD16_BYTE( "u32",  0x000000, 0x10000, CRC(2d651ed0) SHA1(e2da2c3d8f25c17e26fd435c75983b2db8691993) )
	ROM_LOAD16_BYTE( "u38",  0x020001, 0x10000, CRC(1eb8c887) SHA1(220f566043535c54ad1cf2216966c7f42099e50b) )
	ROM_LOAD16_BYTE( "u39",  0x020000, 0x10000, CRC(2ab1ea68) SHA1(4e29a274c5c62b6ca92119eb320200beb784ca55) )
	ROM_LOAD16_BYTE( "u45",  0x040001, 0x10000, CRC(74394375) SHA1(ceb7ae4e3253351da362cd0ada87702164005d17) )
	ROM_LOAD16_BYTE( "u46",  0x040000, 0x10000, CRC(1188047e) SHA1(249f25582ab72eeee37798418460de312053660e) )
	ROM_LOAD16_BYTE( "u52",  0x060001, 0x10000, CRC(5ac2f06d) SHA1(b3a5d0cd94bdffdbf5bd17dbb30c07bfad3fa5d0) )
	ROM_LOAD16_BYTE( "u53",  0x060000, 0x10000, CRC(4bd25cf0) SHA1(d1092cc3b6172d6567acd21f79b22043380102b7) )
	ROM_LOAD16_BYTE( "u57",  0x080001, 0x10000, CRC(f620f935) SHA1(bf891fce1f04f3ad5b8b72d43d041ceacb0b65bc) )
	ROM_LOAD16_BYTE( "u58",  0x080000, 0x10000, CRC(f1b3d7c4) SHA1(7b897230d110be7a5eb05eda927d00561ebb9ce3) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 32015 code (missing) */
	ROM_LOAD16_BYTE( "dspl",         0x00000, 0x08000, NO_DUMP )
	ROM_LOAD16_BYTE( "dsph",         0x00001, 0x08000, NO_DUMP )

	ROM_REGION( 0x100000, REGION_USER2, 0 )				/* 32015 data? (incl. samples?) */
	ROM_LOAD16_WORD( "u1",   0x000000, 0x10000, CRC(3f459482) SHA1(d9d489efd0d9217fceb3bf1a3b37a78d6823b4d9) )
	ROM_LOAD16_WORD( "u2",   0x010000, 0x10000, CRC(a587fffd) SHA1(f33f511d1bf1d6eb3c42535593a9718571174c4b) )
	ROM_LOAD16_WORD( "u3",   0x020000, 0x10000, CRC(984d343a) SHA1(ee214830de4cb22d2d8e9d3ca335eff05af4abb6) )
	ROM_LOAD16_WORD( "u4",   0x030000, 0x10000, CRC(c4765ff6) SHA1(7dca61d32300047ca1c089057e617553d60a0995) )
	ROM_LOAD16_WORD( "u5",   0x040000, 0x10000, CRC(3b63b890) SHA1(a1223cb8884d5365af7d3f607657efff877f8845) )
	ROM_LOAD16_WORD( "u6",   0x050000, 0x10000, CRC(5cdb9aa9) SHA1(fae5d2c7f649bcba8068c8bc8266ee411258535e) )
	ROM_LOAD16_WORD( "u7",   0x060000, 0x10000, CRC(147083a2) SHA1(c04c38145ab159bd519e6325477a3f7d0eebbda1) )
	ROM_LOAD16_WORD( "u8",   0x070000, 0x10000, CRC(975b368c) SHA1(1d637ce8c5d60833bb25aab2610e1a856720235e) )
	ROM_LOAD16_WORD( "u16",  0x080000, 0x10000, CRC(7437e8bf) SHA1(754be4822cd586590f09e706d7eb48e5ba8c8817) )
	ROM_LOAD16_WORD( "u17",  0x090000, 0x10000, CRC(e32bdd0f) SHA1(0662abbe84f0bad2631566b506ef016fcd79b9ee) )
	ROM_LOAD16_WORD( "u18",  0x0a0000, 0x10000, CRC(de3b4d7c) SHA1(68e7ffe2d84aef7c24d1787c4f9b6950c0107741) )
	ROM_LOAD16_WORD( "u19",  0x0b0000, 0x10000, CRC(7109247c) SHA1(201809ec6599b30c26823bde6851b6eaa2589710) )
	ROM_LOAD16_WORD( "u20",  0x0c0000, 0x10000, CRC(038b7d2d) SHA1(80bab18ca36d2bc101da7f3f6e1c82d8a802c14c) )
	ROM_LOAD16_WORD( "u21",  0x0d0000, 0x10000, CRC(9b0b8978) SHA1(b31d0451ecd7085c191d20b2b41d0e8fe551996c) )
	ROM_LOAD16_WORD( "u22",  0x0e0000, 0x10000, CRC(4b92588a) SHA1(eea262c1a122015364a0046ff2bc7816f5f6821d) )
	ROM_LOAD16_WORD( "u23",  0x0f0000, 0x10000, CRC(d7c2b13b) SHA1(3561e08011f649e4d0c47792745b2a014167e816) )
ROM_END

ROM_START( amerdar2 )
	ROM_REGION16_LE( 0x0a0000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "u31",     0x000001, 0x10000, CRC(9628c422) SHA1(46b71acc746760962e34e9d7876f9499ea7d5c7c) )
	ROM_LOAD16_BYTE( "u32",     0x000000, 0x10000, CRC(2d651ed0) SHA1(e2da2c3d8f25c17e26fd435c75983b2db8691993) )
	ROM_LOAD16_BYTE( "u38",     0x020001, 0x10000, CRC(1eb8c887) SHA1(220f566043535c54ad1cf2216966c7f42099e50b) )
	ROM_LOAD16_BYTE( "u39",     0x020000, 0x10000, CRC(2ab1ea68) SHA1(4e29a274c5c62b6ca92119eb320200beb784ca55) )
	ROM_LOAD16_BYTE( "u45",     0x040001, 0x10000, CRC(74394375) SHA1(ceb7ae4e3253351da362cd0ada87702164005d17) )
	ROM_LOAD16_BYTE( "u46",     0x040000, 0x10000, CRC(1188047e) SHA1(249f25582ab72eeee37798418460de312053660e) )
	ROM_LOAD16_BYTE( "u52",     0x060001, 0x10000, CRC(5ac2f06d) SHA1(b3a5d0cd94bdffdbf5bd17dbb30c07bfad3fa5d0) )
	ROM_LOAD16_BYTE( "u53",     0x060000, 0x10000, CRC(4bd25cf0) SHA1(d1092cc3b6172d6567acd21f79b22043380102b7) )
	ROM_LOAD16_BYTE( "u57.bin", 0x080001, 0x10000, CRC(8a70f849) SHA1(dfd4cf90de2ab8cbeff458f0fd20110c1ed009e9) )
	ROM_LOAD16_BYTE( "u58.bin", 0x080000, 0x10000, CRC(8bb81975) SHA1(b7666572ab543991c7deaa0ebefb8b4526a7e386) )

	ROM_REGION( 0x18000, REGION_CPU2, 0 )	/* 32015 code (missing) */
	ROM_LOAD16_BYTE( "dspl",         0x00000, 0x08000, NO_DUMP )
	ROM_LOAD16_BYTE( "dsph",         0x00001, 0x08000, NO_DUMP )

	ROM_REGION( 0x100000, REGION_USER2, 0 )				/* 32015 data? (incl. samples?) */
	ROM_LOAD16_WORD( "u1.bin",   0x000000, 0x10000, CRC(e2bb7f54) SHA1(39eeb61a852b93331f445cc1c993727e52959660) )
	ROM_LOAD16_WORD( "u2",      0x010000, 0x10000, CRC(a587fffd) SHA1(f33f511d1bf1d6eb3c42535593a9718571174c4b) )
	ROM_LOAD16_WORD( "u3",      0x020000, 0x10000, CRC(984d343a) SHA1(ee214830de4cb22d2d8e9d3ca335eff05af4abb6) )
	ROM_LOAD16_WORD( "u4",      0x030000, 0x10000, CRC(c4765ff6) SHA1(7dca61d32300047ca1c089057e617553d60a0995) )
	ROM_LOAD16_WORD( "u5",      0x040000, 0x10000, CRC(3b63b890) SHA1(a1223cb8884d5365af7d3f607657efff877f8845) )
	ROM_LOAD16_WORD( "u6",      0x050000, 0x10000, CRC(5cdb9aa9) SHA1(fae5d2c7f649bcba8068c8bc8266ee411258535e) )
	ROM_LOAD16_WORD( "u7",      0x060000, 0x10000, CRC(147083a2) SHA1(c04c38145ab159bd519e6325477a3f7d0eebbda1) )
	ROM_LOAD16_WORD( "u8",      0x070000, 0x10000, CRC(975b368c) SHA1(1d637ce8c5d60833bb25aab2610e1a856720235e) )
	ROM_LOAD16_WORD( "u16",     0x080000, 0x10000, CRC(7437e8bf) SHA1(754be4822cd586590f09e706d7eb48e5ba8c8817) )
	ROM_LOAD16_WORD( "u17",     0x090000, 0x10000, CRC(e32bdd0f) SHA1(0662abbe84f0bad2631566b506ef016fcd79b9ee) )
	ROM_LOAD16_WORD( "u18",     0x0a0000, 0x10000, CRC(de3b4d7c) SHA1(68e7ffe2d84aef7c24d1787c4f9b6950c0107741) )
	ROM_LOAD16_WORD( "u19",     0x0b0000, 0x10000, CRC(7109247c) SHA1(201809ec6599b30c26823bde6851b6eaa2589710) )
	ROM_LOAD16_WORD( "u20",     0x0c0000, 0x10000, CRC(038b7d2d) SHA1(80bab18ca36d2bc101da7f3f6e1c82d8a802c14c) )
	ROM_LOAD16_WORD( "u21",     0x0d0000, 0x10000, CRC(9b0b8978) SHA1(b31d0451ecd7085c191d20b2b41d0e8fe551996c) )
	ROM_LOAD16_WORD( "u22",     0x0e0000, 0x10000, CRC(4b92588a) SHA1(eea262c1a122015364a0046ff2bc7816f5f6821d) )
	ROM_LOAD16_WORD( "u23.bin", 0x0f0000, 0x10000, CRC(7c1e6f2e) SHA1(21ae530e4bd7c0c9f1a84f01f136c71952c8adc4) )
ROM_END


ROM_START( coolpool )
	ROM_REGION16_LE( 0x40000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "u112b",        0x00000, 0x20000, CRC(aa227769) SHA1(488e357a7aad07369cade3110cde14ba8562c66c) )
	ROM_LOAD16_BYTE( "u113b",        0x00001, 0x20000, CRC(5b5f82f1) SHA1(82afb6a8d94cf09960b962d5208aab451b56feae) )

	ROM_REGION16_LE( 0x200000, REGION_GFX1, 0 )	/* gfx data read by main CPU */
	ROM_LOAD16_BYTE( "u04",          0x000000, 0x20000, CRC(66a9940e) SHA1(7fa587280ecfad6b06194868de09cbdd57cf517f) )
	ROM_CONTINUE(                    0x100000, 0x20000 )
	ROM_LOAD16_BYTE( "u08",          0x000001, 0x20000, CRC(56789cf4) SHA1(5ad867d5029fdac9dccd01a6979171aa30d9a6eb) )
	ROM_CONTINUE(                    0x100001, 0x20000 )
	ROM_LOAD16_BYTE( "u03",          0x040000, 0x20000, CRC(02bc792a) SHA1(8085cff38868a307d6d29a7aadf3d6a99cbe85bb) )
	ROM_CONTINUE(                    0x140000, 0x20000 )
	ROM_LOAD16_BYTE( "u07",          0x040001, 0x20000, CRC(7b2fcb9f) SHA1(fa912663891bac6ba78519f030ba2c718e3514c3) )
	ROM_CONTINUE(                    0x140001, 0x20000 )
	ROM_LOAD16_BYTE( "u02",          0x080000, 0x20000, CRC(3b7d757d) SHA1(8737721764b181b050d776b2d2e1208419f8e5eb) )
	ROM_CONTINUE(                    0x180000, 0x20000 )
	ROM_LOAD16_BYTE( "u06",          0x080001, 0x20000, CRC(c09353a2) SHA1(f3588ec75b757232bdaa40d055e171a501122bfa) )
	ROM_CONTINUE(                    0x180001, 0x20000 )
	ROM_LOAD16_BYTE( "u01",          0x0c0000, 0x20000, CRC(948a5faf) SHA1(186ab3ab0ede168beaa4dae0cba753df10cdac46) )
	ROM_CONTINUE(                    0x1c0000, 0x20000 )
	ROM_LOAD16_BYTE( "u05",          0x0c0001, 0x20000, CRC(616965e2) SHA1(588ea3c5c7838c50b2157ff1074f629d9d85791c) )
	ROM_CONTINUE(                    0x1c0001, 0x20000 )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* TMS320C26 */
	ROM_LOAD16_BYTE( "u34",          0x00000, 0x08000, CRC(dc1df70b) SHA1(e42fa7e34e50e0bd2aaeea5c55d750ed3286610d) )
	ROM_LOAD16_BYTE( "u35",          0x00001, 0x08000, CRC(ac999431) SHA1(7e4c2dcaedcb7e7c67072a179e4b8488d2bbdac7) )

	ROM_REGION( 0x200000, REGION_USER2, 0 )	/* TMS32026 data */
	ROM_LOAD( "u17c",         0x000000, 0x40000, CRC(ea3cc41d) SHA1(e703e789dfbcfaec878a990031ce839164c51253) )
	ROM_LOAD( "u16c",         0x040000, 0x40000, CRC(2e6680ea) SHA1(cb30dc789039aab491428d075fee9e0bc04fd2ce) )
	ROM_LOAD( "u15c",         0x080000, 0x40000, CRC(8e5f248e) SHA1(a954d3c20dc0b70f83c4c238db30a33285fcb353) )
	ROM_LOAD( "u14c",         0x0c0000, 0x40000, CRC(dcd6cf71) SHA1(b1f53bffdd19f5da1d8664765d504568d1f5867c) )
	ROM_LOAD( "u13c",         0x100000, 0x40000, CRC(5a7fe750) SHA1(bbbd45380545cb0f17d9f6811b2a7300fa3b682d) )
	ROM_LOAD( "u12c",         0x140000, 0x40000, CRC(4f246958) SHA1(ee4446159635b6c44d88d8f6aac52787a89403c1) )
	ROM_LOAD( "u11c",         0x180000, 0x40000, CRC(92cd2b03) SHA1(e80df65f8ec5ed2178f623bdd975e2b01a12a184) )
	ROM_LOAD( "u10c",         0x1c0000, 0x40000, CRC(a3dbcae3) SHA1(af997f3f56f406d5eb9fa415e1672b2d129815b8) )
ROM_END


ROM_START( 9ballsht )
	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "u112",         0x00000, 0x40000, CRC(b3855e59) SHA1(c3175df24b85897783169bcaccd61630e512f7f6) )
	ROM_LOAD16_BYTE( "u113",         0x00001, 0x40000, CRC(30cbf462) SHA1(64b2e2d40c2a92c4f4823dc866e5464792954ac3) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )	/* gfx data read by main CPU */
	ROM_LOAD16_BYTE( "u110",         0x00000, 0x80000, CRC(890ed5c0) SHA1(eaf06ee5b6c5ed0103b535396b4517012818a416) )
	ROM_LOAD16_BYTE( "u111",         0x00001, 0x80000, CRC(1a9f1145) SHA1(ba52a6d1aca26484c320518f69c66ce3ceb4adcf) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* TMS320C26 */
	ROM_LOAD16_BYTE( "u34",          0x00000, 0x08000, CRC(dc1df70b) SHA1(e42fa7e34e50e0bd2aaeea5c55d750ed3286610d) )
	ROM_LOAD16_BYTE( "u35",          0x00001, 0x08000, CRC(ac999431) SHA1(7e4c2dcaedcb7e7c67072a179e4b8488d2bbdac7) )

	ROM_REGION( 0x100000, REGION_USER2, 0 )	/* TMS32026 data */
	ROM_LOAD( "u54",          0x00000, 0x80000, CRC(1be5819c) SHA1(308b5b1fe05634419d03956ae1b2e5a61206900f) )
	ROM_LOAD( "u53",          0x80000, 0x80000, CRC(d401805d) SHA1(f4bcb2bdc45c3bc5ca423e518cdea8b3a7e8d60e) )
ROM_END

/*
  all ROMs for this set were missing except for the main program,
  I assume the others are the same.
 */
ROM_START( 9ballsh2 )
	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "e-scape.112",  0x00000, 0x40000, CRC(aee8114f) SHA1(a0d0e9e3a879393585b85ac6d04e31a7d4221179) )
	ROM_LOAD16_BYTE( "e-scape.113",  0x00001, 0x40000, CRC(ccd472a7) SHA1(d074080e987c233b26b3c72248411c575f7a2293) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )	/* gfx data read by main CPU */
	ROM_LOAD16_BYTE( "u110",         0x00000, 0x80000, CRC(890ed5c0) SHA1(eaf06ee5b6c5ed0103b535396b4517012818a416) )
	ROM_LOAD16_BYTE( "u111",         0x00001, 0x80000, CRC(1a9f1145) SHA1(ba52a6d1aca26484c320518f69c66ce3ceb4adcf) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* TMS320C26 */
	ROM_LOAD16_BYTE( "u34",          0x00000, 0x08000, CRC(dc1df70b) SHA1(e42fa7e34e50e0bd2aaeea5c55d750ed3286610d) )
	ROM_LOAD16_BYTE( "u35",          0x00001, 0x08000, CRC(ac999431) SHA1(7e4c2dcaedcb7e7c67072a179e4b8488d2bbdac7) )

	ROM_REGION( 0x100000, REGION_USER2, 0 )	/* TMS32026 data */
	ROM_LOAD( "u54",          0x00000, 0x80000, CRC(1be5819c) SHA1(308b5b1fe05634419d03956ae1b2e5a61206900f) )
	ROM_LOAD( "u53",          0x80000, 0x80000, CRC(d401805d) SHA1(f4bcb2bdc45c3bc5ca423e518cdea8b3a7e8d60e) )
ROM_END

ROM_START( 9ballsh3 )
	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "8e_1826.112",  0x00000, 0x40000, CRC(486f7a8b) SHA1(635e3b1e7a21a86dd3d0ea994e9b923b06df587e) )
	ROM_LOAD16_BYTE( "8e_6166.113",  0x00001, 0x40000, CRC(c41db70a) SHA1(162112f9f5bb6345920a45c41da6a249796bd21f) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )	/* gfx data read by main CPU */
	ROM_LOAD16_BYTE( "u110",         0x00000, 0x80000, CRC(890ed5c0) SHA1(eaf06ee5b6c5ed0103b535396b4517012818a416) )
	ROM_LOAD16_BYTE( "u111",         0x00001, 0x80000, CRC(1a9f1145) SHA1(ba52a6d1aca26484c320518f69c66ce3ceb4adcf) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* TMS320C26 */
	ROM_LOAD16_BYTE( "u34",          0x00000, 0x08000, CRC(dc1df70b) SHA1(e42fa7e34e50e0bd2aaeea5c55d750ed3286610d) )
	ROM_LOAD16_BYTE( "u35",          0x00001, 0x08000, CRC(ac999431) SHA1(7e4c2dcaedcb7e7c67072a179e4b8488d2bbdac7) )

	ROM_REGION( 0x100000, REGION_USER2, 0 )	/* TMS32026 data */
	ROM_LOAD( "u54",          0x00000, 0x80000, CRC(1be5819c) SHA1(308b5b1fe05634419d03956ae1b2e5a61206900f) )
	ROM_LOAD( "u53",          0x80000, 0x80000, CRC(d401805d) SHA1(f4bcb2bdc45c3bc5ca423e518cdea8b3a7e8d60e) )
ROM_END

ROM_START( 9ballshtc )
	ROM_REGION16_LE( 0x80000, REGION_USER1, 0 )	/* 34010 code */
	ROM_LOAD16_BYTE( "3990.u112",  0x00000, 0x40000, CRC(7ba2749a) SHA1(e2ddc2600234dbebbb423f201cc4061fd0b9911a) )
	ROM_LOAD16_BYTE( "b72f.u113",  0x00001, 0x40000, CRC(1e0f3c62) SHA1(3c24a38dcb553fd84b0b44a5a8d93a14435e22b0) )

	ROM_REGION16_LE( 0x100000, REGION_GFX1, 0 )	/* gfx data read by main CPU */
	ROM_LOAD16_BYTE( "u110",         0x00000, 0x80000, CRC(890ed5c0) SHA1(eaf06ee5b6c5ed0103b535396b4517012818a416) )
	ROM_LOAD16_BYTE( "u111",         0x00001, 0x80000, CRC(1a9f1145) SHA1(ba52a6d1aca26484c320518f69c66ce3ceb4adcf) )

	ROM_REGION( 0x40000, REGION_CPU2, 0 )	/* TMS320C26 */
	ROM_LOAD16_BYTE( "u34",          0x00000, 0x08000, CRC(dc1df70b) SHA1(e42fa7e34e50e0bd2aaeea5c55d750ed3286610d) )
	ROM_LOAD16_BYTE( "u35",          0x00001, 0x08000, CRC(ac999431) SHA1(7e4c2dcaedcb7e7c67072a179e4b8488d2bbdac7) )

	ROM_REGION( 0x100000, REGION_USER2, 0 )	/* TMS32026 data */
	ROM_LOAD( "0000.u54",          0x00000, 0x80000, CRC(04b509a0) SHA1(093343741a3d8d0786fd443e68dd85b414c6cf9e) )
	ROM_LOAD( "2df8.u53",          0x80000, 0x80000, CRC(c8a7b576) SHA1(7eb71dd791fdcbfe71764a454f0a1d3130d8a57e) )
ROM_END



/*************************************
 *
 *  Driver init
 *
 *************************************/

static DRIVER_INIT( coolpool )
{
	memory_install_read16_handler(1, ADDRESS_SPACE_IO, 0x07, 0x07, 0, 0, coolpool_input_r);
}


static DRIVER_INIT( 9ballsht )
{
	int a;
	UINT16 *rom;

	/* decrypt the main program ROMs */
	rom = (UINT16 *)memory_region(REGION_USER1);
	for (a = 0;a < memory_region_length(REGION_USER1)/2;a++)
	{
		int hi,lo,nhi,nlo;

		hi = rom[a] >> 8;
		lo = rom[a] & 0xff;

		nhi = BITSWAP8(hi,5,2,0,7,6,4,3,1) ^ 0x29;
		if (hi & 0x01) nhi ^= 0x03;
		if (hi & 0x10) nhi ^= 0xc1;
		if (hi & 0x20) nhi ^= 0x40;
		if (hi & 0x40) nhi ^= 0x12;

		nlo = BITSWAP8(lo,5,3,4,6,7,1,2,0) ^ 0x80;
		if ((lo & 0x02) && (lo & 0x04)) nlo ^= 0x01;
		if (lo & 0x04) nlo ^= 0x0c;
		if (lo & 0x08) nlo ^= 0x10;

		rom[a] = (nhi << 8) | nlo;
	}

	/* decrypt the sub data ROMs */
	rom = (UINT16 *)memory_region(REGION_USER2);
	for (a = 1;a < memory_region_length(REGION_USER2)/2;a+=4)
	{
		/* just swap bits 1 and 2 of the address */
		UINT16 tmp = rom[a];
		rom[a] = rom[a+1];
		rom[a+1] = tmp;
	}
}



/*************************************
 *
 *  Game drivers
 *
 *************************************/

GAME( 1989, amerdart, 0,        amerdart, amerdart, 0,        ROT0, "Ameri",   "AmeriDarts (set 1)", GAME_NO_SOUND )
GAME( 1989, amerdar2, amerdart, amerdart, amerdart, 0,        ROT0, "Ameri",   "AmeriDarts (set 2)", GAME_NO_SOUND )
GAME( 1992, coolpool, 0,        coolpool, coolpool, coolpool, ROT0, "Catalina", "Cool Pool", 0 )
GAME( 1993, 9ballsht, 0,        9ballsht, 9ballsht, 9ballsht, ROT0, "E-Scape EnterMedia (Bundra license)", "9-Ball Shootout (set 1)", 0 )
GAME( 1993, 9ballsh2, 9ballsht, 9ballsht, 9ballsht, 9ballsht, ROT0, "E-Scape EnterMedia (Bundra license)", "9-Ball Shootout (set 2)", 0 )
GAME( 1993, 9ballsh3, 9ballsht, 9ballsht, 9ballsht, 9ballsht, ROT0, "E-Scape EnterMedia (Bundra license)", "9-Ball Shootout (set 3)", 0 )
GAME( 1993, 9ballshtc,9ballsht, 9ballsht, 9ballsht, 9ballsht, ROT0, "E-Scape EnterMedia (Bundra license)", "9-Ball Shootout Championship", 0)
