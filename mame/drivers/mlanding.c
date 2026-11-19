// license:BSD-3-Clause
// copyright-holders:Angelo Salese, Tomasz Slanina, Philip Bennett
/***************************************************************************

    Midnight Landing

    driver by Tomasz Slanina, Phil Bennett & Angelo Salese
    Based on early work by David Haywood

    CPU Board quick layout:
    |----------------------------------------|
    |    68000CP8        PAL     DSW  DSW    |
    |    *   *                             J|--|
    |    22  23   5165              uPD4701 |--|
    |   6264 6264 5165     TMS32020 uPD4701 |--|
    |                                       |--|
    |                              PC050CM  |--|
    |A                          MB3731 x 3  |--|
    |    5165 5165              VR1 VR2 VR3 |--|
    | PAL                                   |--|
    | PAL               PAL                 |--|
    |                                        |
    | 16.000MHz   30   YM2151              R |
    |B    *   *   31    384KHz               |
    |     24  25  32   5205 5205             |
    | #   26  27  33                         |
    |     28  29  34                         |
    |                              36        |
    |                PAL           PC060HA   |
    |PAL             PAL     35    Z80 CTC   |
    |    68000CP8    PAL     Z80             |
    |----------------------------------------|

        A, B, R are flatcable connectors, and J is for Jamma
        All numbered roms are prefixed with B09
        XTALs are 16.0000MHz, 384KHz
        VR1, VR2 & VR3 are sound pots
        # denotes a MB3771 Power Supply Monitor
        * denotes unpopulated ROM sockets

****************************************************************************/

#include "driver.h"
#include "cpu/tms32025/tms32025.h"
#include "machine/z80ctc.h"
#include "sndhrdw/taitosnd.h"
#include "vidhrdw/generic.h"
#include "sound/2151intf.h"
#include "sound/msm5205.h"

#define DMA_BANK_WORDS 0x2000

static UINT16 *g_ram;
static UINT16 *cha_ram;
static UINT8 *power_ram;
static UINT16 *dma_ram;
static UINT8 dma_cpu_bank;
static UINT8 dma_busy;
static UINT16 dsp_hold_signal;
static mame_timer *dma_done_timer;

static UINT32 msm_pos[2];
static UINT8 msm_reset[2];
static UINT8 msm_nibble[2];
static UINT8 msm2_vck;
static UINT8 msm2_vck2;

/*************************************
 *
 *  Video update
 *
 *************************************/

VIDEO_START( mlanding )
{
	return 0;
}

VIDEO_UPDATE( mlanding )
{
	int x, y;

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		const UINT16 *src = &g_ram[(112 + y) * 512 + cliprect->min_x];
		UINT16 *dst = (UINT16 *)bitmap->line[y] + cliprect->min_x;

		for (x = cliprect->min_x; x <= cliprect->max_x; ++x)
			*dst++ = Machine->pens[*src++ & 0x3fff];
	}

}

/*************************************
 *
 *  Video DMA
 *
 *************************************/

static UINT32 exec_dma(void)
{
	UINT32 pixcnt = 0;
	const UINT16 *bank = &dma_ram[(dma_cpu_bank ^ 1) * DMA_BANK_WORDS];
	UINT32 offs;

	for (offs = 0; offs < DMA_BANK_WORDS; offs += 4)
	{
		const UINT16 attr = bank[offs];
		UINT16 code;
		UINT16 xword, yword;
		UINT16 x, y, sx, sy;
		UINT8 colour;
		UINT32 j, k, y1, x1;

		if (attr == 0)
			continue;

		code = attr & 0x1fff;

		xword = bank[offs + 1];
		yword = bank[offs + 2];

		x = xword & 0x1ff;
		y = yword & 0x1ff;
		sx = ((xword >> 11) & 0x1f) + 1;
		sy = ((yword >> 11) & 0x1f) + 1;

		colour = bank[offs + 3] & 0xff;

		if ((attr & 0x2000) == 0)
		{
			/* normal draw mode */
			UINT8 basepix = colour << 4;

			for (j = 0; j < sx; ++j)
			{
				for (k = 0; k < sy; ++k)
				{
					/* draw an 8x8 tile */
					for (y1 = 0; y1 < 8; ++y1)
					{
						const UINT16 *src = &cha_ram[(code * 2 * 8) + y1 * 2];
						UINT32 addr = (y + k * 8 + y1) * 512 + (j * 8 + x);

						UINT8 p2 = src[0] & 0xff;
						UINT8 p1 = src[0] >> 8;
						UINT8 p4 = src[1] & 0xff;
						UINT8 p3 = src[1] >> 8;

						for (x1 = 0; x1 < 8; ++x1)
						{
							UINT16 pix = (((p4 >> x1) & 1) << 3) | (((p3 >> x1) & 1) << 2) | (((p2 >> x1) & 1) << 1) | ((p1 >> x1) & 1);

							/* bit 15 selects opaque mode */
							if ((attr & 0x8000) != 0 || pix)
							{
								UINT16 *dst = &g_ram[addr & 0x3ffff];
								*dst = (*dst & 0xff00) | (basepix | pix);
							}

							++addr;
							++pixcnt;
						}
					}
					++code;
				}
			}
		}
		else
		{
			/* set pixel or palette data */
			for (y1 = 0; y1 < sy * 8U; ++y1)
			{
				UINT32 addr = (y + y1) * 512 + x;

				if ((attr & 0x4000) == 0)
				{
					/* clear pixel data */
					for (x1 = 0; x1 < sx * 8U; ++x1)
					{
						UINT16 *dst = &g_ram[addr & 0x3ffff];
						*dst = (*dst & 0xff00) | colour;
						++addr;
						++pixcnt;
					}
				}
				else
				{
					/* clear palette data */
					for (x1 = 0; x1 < sx * 8U; ++x1)
					{
						UINT16 *dst = &g_ram[addr & 0x3ffff];
						*dst = (*dst & 0x00ff) | (colour << 8);
						++addr;
						++pixcnt;
					}
				}
			}
		}
	}

	return pixcnt;
}

static void dma_complete(int param)
{
	dma_busy = 0;
}

static WRITE16_HANDLER( dma_start_w )
{
	UINT32 pixels;

	dma_cpu_bank ^= 1;
	memory_set_bank(1, dma_cpu_bank);

	pixels = exec_dma();

	if (pixels)
	{
		dma_busy = 1;

		timer_adjust(dma_done_timer, TIME_IN_HZ(16000000) * pixels, 0, 0);
	}
}

static WRITE16_HANDLER( dma_stop_w )
{
	dma_busy = 0;
	timer_adjust(dma_done_timer, TIME_NEVER, 0, 0);
}

/*************************************
 *
 *  I/O
 *
 *************************************/

static READ16_HANDLER( input_r )
{
	UINT8 dswa = readinputportbytag("DSWA");
	UINT8 dswb = readinputportbytag("DSWB") & 0x7f;

	return (dma_busy << 15) | (dswb << 8) | dswa;
}

static READ16_HANDLER( in0_r )
{
	return readinputportbytag("IN0");
}

static READ16_HANDLER( in1_r )
{
	return readinputportbytag("IN1");
}

static WRITE16_HANDLER( output_w )
{
	if (ACCESSING_LSB)
	{
		cpunum_set_input_line(2, INPUT_LINE_RESET, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		cpunum_set_input_line(3, INPUT_LINE_RESET, (data & 0x40) ? CLEAR_LINE : ASSERT_LINE);
		coin_counter_w(0, data & 4);
		coin_counter_w(1, data & 8);
	}
}

/*************************************
 *
 *  Analog controls
 *
 *************************************/

static READ16_HANDLER( analog1_msb_r )
{
	return (readinputportbytag("THROTTLE") >> 4) & 0xff;
}

static READ16_HANDLER( analog2_msb_r )
{
	return (readinputportbytag("STICK_X") >> 4) & 0xff;
}

static READ16_HANDLER( analog3_msb_r )
{
	return (readinputportbytag("STICK_Y") >> 4) & 0xff;
}

static READ16_HANDLER( analog1_lsb_r )
{
	UINT16 throttle = readinputportbytag("THROTTLE");
	UINT16 x = readinputportbytag("STICK_X");

	/* the limit contacts are active low */
	return (throttle & 0xf)
		| ((x & 0xe00) != 0x600 ? 0x10 : 0)
		| ((throttle & 0xe00) != 0x800 ? 0x20 : 0)
		| ((throttle & 0xe00) != 0x600 ? 0x40 : 0);
}

static READ16_HANDLER( analog2_lsb_r )
{
	return readinputportbytag("STICK_X") & 0x0f;
}

static READ16_HANDLER( analog3_lsb_r )
{
	UINT16 x = readinputportbytag("STICK_X");
	UINT16 y = readinputportbytag("STICK_Y");

	/* the limit contacts are active low */
	return (y & 0xf)
		| ((y & 0xe00) != 0x600 ? 0x10 : 0)
		| ((x & 0xe00) != 0x800 ? 0x20 : 0)
		| ((y & 0xe00) != 0x800 ? 0x40 : 0);
}

/*************************************
 *
 *  DSP control
 *
 *************************************/

static READ16_HANDLER( dsp_hold_signal_r )
{
	return dsp_hold_signal;
}

static WRITE16_HANDLER( dsp_hold_ack_w )
{
}

static WRITE16_HANDLER( dsp_control_w )
{
	if (ACCESSING_LSB)
		cpunum_set_input_line(4, INPUT_LINE_RESET, (data & 0x2) ? CLEAR_LINE : ASSERT_LINE);
}

/*************************************
 *
 *  Sound
 *
 *************************************/

static WRITE8_HANDLER( sound_bankswitch_w )
{
}

static void msm5205_update(int chip)
{
	UINT8 data;

	if (msm_reset[chip])
		return;

	data = memory_region(REGION_SOUND1 + chip)[msm_pos[chip]];

	MSM5205_data_w(chip, (msm_nibble[chip] ? data : data >> 4) & 0xf);

	if (msm_nibble[chip])
		++msm_pos[chip];

	msm_nibble[chip] ^= 1;
}

static void msm5205_1_vck(int chip)
{
	msm5205_update(0);
}

static WRITE8_HANDLER( z80ctc_to0 )
{
	if (msm2_vck2 && !data)
	{
		/* the CTC output is divided by 2 */
		if (msm2_vck)
		{
			MSM5205_vclk_w(1, 1);
		}
		else
		{
			/* update on falling edge of /VCK */
			msm5205_update(1);

			/* handle looping */
			if (msm_pos[1] == 0x2000)
			{
				msm_pos[1] = 0;
				MSM5205_reset_w(1, 1);
				MSM5205_vclk_w(1, 0);
				MSM5205_reset_w(1, 0);
			}
			else
			{
				MSM5205_vclk_w(1, 0);
			}
		}

		msm2_vck ^= 1;
	}
	msm2_vck2 = data;
}

static WRITE8_HANDLER( msm5205_1_start_w )
{
	msm_reset[0] = 0;
	MSM5205_reset_w(0, 0);
}

static WRITE8_HANDLER( msm5205_1_stop_w )
{
	msm_reset[0] = 1;
	msm_nibble[0] = 0;
	msm_pos[0] &= ~0xff;
	MSM5205_reset_w(0, 1);
}

static WRITE8_HANDLER( msm5205_1_addr_lo_w )
{
	msm_pos[0] &= ~0x0ff00;
	msm_pos[0] |= data << 8;
}

static WRITE8_HANDLER( msm5205_1_addr_hi_w )
{
	msm_pos[0] &= ~0x70000;
	msm_pos[0] |= (data & 7) << 16;
}

static WRITE8_HANDLER( msm5205_2_start_w )
{
	msm_reset[1] = 0;
	MSM5205_reset_w(1, 0);
}

static WRITE8_HANDLER( msm5205_2_stop_w )
{
	msm_reset[1] = 1;
	msm_nibble[1] = 0;
	MSM5205_reset_w(1, 1);
}

static void irq_handler(int irq)
{
	cpunum_set_input_line(1, 0, irq ? ASSERT_LINE : CLEAR_LINE);
}

/*************************************
 *
 *  Mecha drive (motorized cabinet)
 *
 *************************************/

static READ16_HANDLER( power_ram_r )
{
	return power_ram[offset];
}

static WRITE16_HANDLER( power_ram_w )
{
	if (ACCESSING_LSB)
		power_ram[offset] = data & 0xff;
}

static READ8_HANDLER( motor_r )
{
	return 0x10;
}

/*************************************
 *
 *  Initialization
 *
 *************************************/

static const z80ctc_interface ctc_intf =
{
	16000000 / 4,
	0,
	0,
	z80ctc_to0,
	0,
	0
};

static MACHINE_START( mlanding )
{
	/* allocate two DMA RAM banks */
	dma_ram = auto_malloc(DMA_BANK_WORDS * 2 * sizeof(UINT16));
	memset(dma_ram, 0, DMA_BANK_WORDS * 2 * sizeof(UINT16));
	memory_configure_bank(1, 0, 2, dma_ram, DMA_BANK_WORDS * sizeof(UINT16));

	dma_done_timer = timer_alloc(dma_complete);

	state_save_register_global_pointer(dma_ram, DMA_BANK_WORDS * 2);
	state_save_register_global(dma_cpu_bank);
	state_save_register_global(dma_busy);
	state_save_register_global(dsp_hold_signal);
	state_save_register_global_array(msm_pos);
	state_save_register_global_array(msm_reset);
	state_save_register_global_array(msm_nibble);
	state_save_register_global(msm2_vck);
	state_save_register_global(msm2_vck2);

	z80ctc_init(0, (z80ctc_interface *)&ctc_intf);
	return 0;
}

static MACHINE_RESET( mlanding )
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(4, INPUT_LINE_RESET, ASSERT_LINE);

	dma_cpu_bank = 0;
	dma_busy = 0;
	memory_set_bank(1, dma_cpu_bank);

	dsp_hold_signal = 0;

	z80ctc_reset(0);

	msm_pos[0] = 0;
	msm_pos[1] = 0;
	msm_reset[0] = 0;
	msm_reset[1] = 0;
	msm_nibble[0] = 0;
	msm_nibble[1] = 0;
	MSM5205_reset_w(0, 1);
	MSM5205_reset_w(1, 1);
	msm2_vck = 0;
	msm2_vck2 = 0;
}

/*************************************
 *
 *  Main CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x05ffff) AM_ROM
	AM_RANGE(0x080000, 0x08ffff) AM_RAM
	AM_RANGE(0x100000, 0x17ffff) AM_RAM AM_BASE(&g_ram)
	AM_RANGE(0x180000, 0x1bffff) AM_RAM AM_BASE(&cha_ram)
	AM_RANGE(0x1c0000, 0x1c3fff) AM_RAMBANK(1)
	AM_RANGE(0x1c4000, 0x1cffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x1d0000, 0x1d0001) AM_WRITE(dma_start_w)
	AM_RANGE(0x1d0002, 0x1d0003) AM_WRITE(dma_stop_w)
	AM_RANGE(0x200000, 0x20ffff) AM_RAM AM_WRITE(paletteram16_xBBBBBGGGGGRRRRR_word_w) AM_BASE(&paletteram16)
	AM_RANGE(0x240004, 0x240005) AM_READNOP /* watchdog */
	AM_RANGE(0x240006, 0x240007) AM_READ(input_r)
	AM_RANGE(0x280000, 0x280fff) AM_READWRITE(power_ram_r, power_ram_w)
	AM_RANGE(0x290000, 0x290001) AM_READ(in1_r)
	AM_RANGE(0x290002, 0x290003) AM_READ(in0_r)
	AM_RANGE(0x2a0000, 0x2a0001) AM_WRITE(output_w)
	AM_RANGE(0x2b0000, 0x2b0001) AM_READ(analog1_msb_r)
	AM_RANGE(0x2b0002, 0x2b0003) AM_READ(analog1_lsb_r)
	AM_RANGE(0x2b0004, 0x2b0005) AM_READ(analog2_msb_r)
	AM_RANGE(0x2b0006, 0x2b0007) AM_READ(analog2_lsb_r)
	AM_RANGE(0x2c0000, 0x2c0001) AM_READ(analog3_msb_r)
	AM_RANGE(0x2c0002, 0x2c0003) AM_READ(analog3_lsb_r)
	AM_RANGE(0x2d0000, 0x2d0001) AM_READNOP AM_WRITE(taitosound_port16_lsb_w)
	AM_RANGE(0x2d0002, 0x2d0003) AM_READWRITE(taitosound_comm16_lsb_r, taitosound_comm16_lsb_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Audio CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( audio_map_prog, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x8fff) AM_RAM
	AM_RANGE(0x9000, 0x9000) AM_WRITE(YM2151_register_port_0_w)
	AM_RANGE(0x9001, 0x9001) AM_READWRITE(YM2151_status_port_0_r, YM2151_data_port_0_w)
	AM_RANGE(0xa000, 0xa000) AM_WRITE(taitosound_slave_port_w)
	AM_RANGE(0xa001, 0xa001) AM_READWRITE(taitosound_slave_comm_r, taitosound_slave_comm_w)
	AM_RANGE(0xb000, 0xb000) AM_WRITE(msm5205_2_start_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(msm5205_2_stop_w)
	AM_RANGE(0xd000, 0xd000) AM_WRITE(msm5205_1_start_w)
	AM_RANGE(0xe000, 0xe000) AM_WRITE(msm5205_1_stop_w)
	AM_RANGE(0xf000, 0xf000) AM_WRITE(msm5205_1_addr_lo_w)
	AM_RANGE(0xf200, 0xf200) AM_WRITE(msm5205_1_addr_hi_w)
	AM_RANGE(0xf400, 0xf400) AM_WRITENOP
	AM_RANGE(0xf600, 0xf600) AM_WRITENOP
	AM_RANGE(0xf800, 0xf800) AM_WRITENOP
	AM_RANGE(0xfa00, 0xfa00) AM_WRITENOP
ADDRESS_MAP_END

static ADDRESS_MAP_START( audio_map_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS( AMEF_ABITS(8) )
	AM_RANGE(0x00, 0x00) AM_READWRITE(z80ctc_0_r, z80ctc_0_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Sub CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( sub_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x040000, 0x043fff) AM_RAM
	AM_RANGE(0x050000, 0x0507ff) AM_RAM AM_SHARE(2)
	AM_RANGE(0x060000, 0x060001) AM_WRITE(dsp_control_w)
	AM_RANGE(0x1c0000, 0x1c3fff) AM_RAMBANK(1)
	AM_RANGE(0x1c4000, 0x1cffff) AM_RAM AM_SHARE(1)
	AM_RANGE(0x200000, 0x2007ff) AM_RAM
	AM_RANGE(0x200800, 0x203fff) AM_RAM AM_SHARE(3)
ADDRESS_MAP_END

/*************************************
 *
 *  Mecha CPU memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( mecha_map_prog, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x7fff) AM_ROM
	AM_RANGE(0x8000, 0x87ff) AM_RAM
	AM_RANGE(0x8800, 0x8fff) AM_RAM AM_BASE(&power_ram)
	AM_RANGE(0x9000, 0x9003) AM_WRITENOP
	AM_RANGE(0x9800, 0x9805) AM_READ(motor_r)
ADDRESS_MAP_END

/*************************************
 *
 *  DSP memory handlers
 *
 *************************************/

static ADDRESS_MAP_START( dsp_map_prog, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x03ff) AM_RAM AM_SHARE(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_map_data, ADDRESS_SPACE_DATA, 16 )
	AM_RANGE(0x0400, 0x1fff) AM_RAM AM_SHARE(3)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_map_io_read, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(TMS32025_HOLD, TMS32025_HOLD) AM_READ(dsp_hold_signal_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp_map_io_write, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(TMS32025_HOLDA, TMS32025_HOLDA) AM_WRITE(dsp_hold_ack_w)
ADDRESS_MAP_END

/*************************************
 *
 *  Port definitions
 *
 *************************************/

INPUT_PORTS_START( mlanding )
	PORT_START_TAG("DSWA")
	PORT_DIPNAME( 0x01, 0x01, DEF_STR( Cabinet ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Standard ) )
	PORT_DIPSETTING(    0x00, "Deluxe" )
	PORT_DIPNAME( 0x02, 0x02, "Coin Mode" )
	PORT_DIPSETTING(    0x02, "Mode A (Japan)" )
	PORT_DIPSETTING(    0x00, "Mode B (World)" )
	PORT_SERVICE( 0x04, IP_ACTIVE_LOW )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x08, DEF_STR( On ) )
	PORT_DIPNAME( 0x30, 0x30, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(    0x00, DEF_STR( 4C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 3C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x10, DEF_STR( 2C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x20, DEF_STR( 2C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x30, DEF_STR( 1C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x20, DEF_STR( 1C_2C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPNAME( 0xc0, 0xc0, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(    0x40, DEF_STR( 2C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_1C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x00, DEF_STR( 2C_3C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_2C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x02)
	PORT_DIPSETTING(    0xc0, DEF_STR( 1C_2C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x80, DEF_STR( 1C_3C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x40, DEF_STR( 1C_4C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)
	PORT_DIPSETTING(    0x00, DEF_STR( 1C_6C ) ) PORT_CONDITION("DSWA", 0x02, PORTCOND_EQUALS, 0x00)

	PORT_START_TAG("DSWB")
	PORT_DIPNAME( 0x03, 0x03, DEF_STR( Difficulty ) )
	PORT_DIPSETTING(    0x02, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x03, DEF_STR( Medium ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Hard ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Hardest ) )
	PORT_DIPNAME( 0x04, 0x04, DEF_STR( Allow_Continue ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x08, 0x08, DEF_STR( Unknown ) )
	PORT_DIPSETTING(    0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Test Mode 2" )
	PORT_DIPSETTING(    0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x00, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
	PORT_DIPNAME( 0x80, 0x80, DEF_STR( Unused ) )
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START_TAG("IN0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_SERVICE2 ) PORT_NAME("Door") PORT_TOGGLE
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START_TAG("IN1")
	PORT_DIPNAME( 0x01, 0x01, "Coin A Enable" )
	PORT_DIPSETTING(    0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "Coin B Enable" )
	PORT_DIPSETTING(    0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	/* the input scaler needs a representable positive magnitude for the negative
       endpoint; -2048 overflows its signed 12-bit intermediate, so use -2047 */

	PORT_START_TAG("STICK_X")
	PORT_BIT( 0x0fff, 0x0000, IPT_AD_STICK_X ) PORT_MINMAX(0x0801,0x7ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_NAME("Yoke X")

	PORT_START_TAG("STICK_Y")
	PORT_BIT( 0x0fff, 0x0000, IPT_AD_STICK_Y ) PORT_MINMAX(0x0801,0x7ff) PORT_SENSITIVITY(100) PORT_KEYDELTA(20) PORT_NAME("Yoke Y")

	PORT_START_TAG("THROTTLE")
	PORT_BIT( 0x0fff, 0x0000, IPT_AD_STICK_Z ) PORT_MINMAX(0x0801,0x7ff) PORT_SENSITIVITY(30) PORT_KEYDELTA(40) PORT_CENTERDELTA(0) PORT_CODE_DEC(KEYCODE_Z) PORT_CODE_INC(KEYCODE_A) PORT_NAME("Throttle Lever")
INPUT_PORTS_END

INPUT_PORTS_START( mlandingj )
	PORT_INCLUDE(mlanding)

	PORT_MODIFY("DSWB")
	PORT_DIPNAME( 0x40, 0x40, DEF_STR( Language ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Japanese ) )
	PORT_DIPSETTING(    0x00, DEF_STR( English ) )
INPUT_PORTS_END

/*************************************
 *
 *  Machine driver
 *
 *************************************/

static struct YM2151interface ym2151_interface =
{
	irq_handler,
	sound_bankswitch_w
};

static const struct MSM5205interface msm5205_1_interface =
{
	msm5205_1_vck,	/* VCK function */
	MSM5205_S48_4B	/* 8 kHz, 4-bit */
};

static const struct MSM5205interface msm5205_2_interface =
{
	0,
	MSM5205_SEX_4B	/* slave mode, 4-bit */
};

static MACHINE_DRIVER_START( mlanding )

	/* basic machine hardware */
	MDRV_CPU_ADD(M68000, 16000000 / 2)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(irq6_line_hold, 1)

	MDRV_CPU_ADD(Z80, 16000000 / 4)
	MDRV_CPU_PROGRAM_MAP(audio_map_prog, 0)
	MDRV_CPU_IO_MAP(audio_map_io, 0)

	MDRV_CPU_ADD(M68000, 16000000 / 2)
	MDRV_CPU_PROGRAM_MAP(sub_map, 0)

	MDRV_CPU_ADD(Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(mecha_map_prog, 0)
	MDRV_CPU_VBLANK_INT(irq0_line_hold, 1)

	MDRV_CPU_ADD(TMS32025, 16000000)
	MDRV_CPU_PROGRAM_MAP(dsp_map_prog, 0)
	MDRV_CPU_DATA_MAP(dsp_map_data, 0)
	MDRV_CPU_IO_MAP(dsp_map_io_read, dsp_map_io_write)

	MDRV_FRAMES_PER_SECOND(16000000.0 / 640 / 462)
	MDRV_VBLANK_DURATION(DEFAULT_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(12)

	MDRV_MACHINE_START(mlanding)
	MDRV_MACHINE_RESET(mlanding)

	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 400)
	MDRV_VISIBLE_AREA(0, 511, 0, 399)
	MDRV_PALETTE_LENGTH(32768)

	MDRV_VIDEO_START(mlanding)
	MDRV_VIDEO_UPDATE(mlanding)

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")

	MDRV_SOUND_ADD(YM2151, 16000000 / 4)
	MDRV_SOUND_CONFIG(ym2151_interface)
	MDRV_SOUND_ROUTE(0, "mono", 0.50)
	MDRV_SOUND_ROUTE(1, "mono", 0.50)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_1_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.80)

	MDRV_SOUND_ADD(MSM5205, 384000)
	MDRV_SOUND_CONFIG(msm5205_2_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.10)
MACHINE_DRIVER_END

/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( mlanding )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b09_29.ic44", 0x00000, 0x10000, CRC(ab3f38f3) SHA1(4357112ca11a8e7bfe08ba99ac3bddac046c230a) )
	ROM_LOAD16_BYTE( "b09_28.ic27", 0x00001, 0x10000, CRC(21e7a8f6) SHA1(860d3861d4375866cd27d426d546ddb2894a6629) )
	ROM_LOAD16_BYTE( "b09_27.ic45", 0x20000, 0x10000, CRC(b02f1805) SHA1(b8050f955c7070dc9b962db329b5b0ee8b2acb70) )
	ROM_LOAD16_BYTE( "b09_26.ic28", 0x20001, 0x10000, CRC(d57ff428) SHA1(8ff1ab666b06fb873f1ba9b25edf4cd49b9861a1) )
	ROM_LOAD16_BYTE( "b09_25.ic46", 0x40000, 0x10000, CRC(ff59f049) SHA1(aba490a28aba03728415f34d321fd599c31a5fde) )
	ROM_LOAD16_BYTE( "b09_24.ic29", 0x40001, 0x10000, CRC(9bc3e1b0) SHA1(6d86804327df11a513a0f06dceb57b83b34ac007) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b09_35.ic80", 0x00000, 0x08000, CRC(b85915c5) SHA1(656e97035ae304f84e90758d0dd6f0616c40f1db) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "b09_23.ic56", 0x00000, 0x10000, CRC(81b2c871) SHA1(a085bc528c63834079469db6ae263a5b9b984a7c) )
	ROM_LOAD16_BYTE( "b09_22.ic39", 0x00001, 0x10000, CRC(36923b42) SHA1(c31d7c45a563cfc4533379f69f32889c79562534) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "b09_37.epr", 0x00000, 0x08000, CRC(4bdf15ed) SHA1(b960208e63cede116925e064279a6cf107aef81c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b09_34.ic62", 0x00000, 0x10000, CRC(0899666f) SHA1(032e3ddd4caa48f82592570616e16c084de91f3e) )
	ROM_LOAD( "b09_33.ic63", 0x10000, 0x10000, CRC(f5cac954) SHA1(71abdc545e0196ad4d357af22dd6312d10a1323f) )
	ROM_LOAD( "b09_32.ic64", 0x20000, 0x10000, CRC(4721dc59) SHA1(faad75d577344e9ba495059040a2cf0647567426) )
	ROM_LOAD( "b09_31.ic65", 0x30000, 0x10000, CRC(9c4a82bf) SHA1(daeac620c636013a36595ce9f37e84e807f88977) )
	ROM_LOAD( "b09_30.ic66", 0x40000, 0x10000, CRC(214a30e2) SHA1(3dcc3a89ed52e4dbf232d2a92a3e64975b46c2dd) )

	ROM_REGION( 0x2000, REGION_SOUND2, 0 )
	ROM_LOAD( "b09_36.ic111", 0x00000, 0x02000, CRC(51fd3a77) SHA1(1fcbadf1877e25848a1d1017322751560a4823c0) )
ROM_END

ROM_START( mlandingj )
	ROM_REGION( 0x60000, REGION_CPU1, 0 )
	ROM_LOAD16_BYTE( "b09_29-2.ic44", 0x00000, 0x10000, CRC(c78d55cd) SHA1(0126524310048a55f4c0c361a1673cc259e461ba) )
	ROM_LOAD16_BYTE( "b09_28-3.ic27", 0x00001, 0x10000, CRC(7098c18d) SHA1(ee5b386fed9df5edfd19cb967fd01e9e7fd9ad08) )
	ROM_LOAD16_BYTE( "b09_27.ic45",   0x20000, 0x10000, CRC(b02f1805) SHA1(b8050f955c7070dc9b962db329b5b0ee8b2acb70) )
	ROM_LOAD16_BYTE( "b09_26.ic28",   0x20001, 0x10000, CRC(d57ff428) SHA1(8ff1ab666b06fb873f1ba9b25edf4cd49b9861a1) )
	ROM_LOAD16_BYTE( "b09_25.ic46",   0x40000, 0x10000, CRC(ff59f049) SHA1(aba490a28aba03728415f34d321fd599c31a5fde) )
	ROM_LOAD16_BYTE( "b09_24.ic29",   0x40001, 0x10000, CRC(9bc3e1b0) SHA1(6d86804327df11a513a0f06dceb57b83b34ac007) )

	ROM_REGION( 0x10000, REGION_CPU2, 0 )
	ROM_LOAD( "b09_35.ic80", 0x00000, 0x08000, CRC(b85915c5) SHA1(656e97035ae304f84e90758d0dd6f0616c40f1db) )

	ROM_REGION( 0x20000, REGION_CPU3, 0 )
	ROM_LOAD16_BYTE( "b09_23.ic56", 0x00000, 0x10000, CRC(81b2c871) SHA1(a085bc528c63834079469db6ae263a5b9b984a7c) )
	ROM_LOAD16_BYTE( "b09_22.ic39", 0x00001, 0x10000, CRC(36923b42) SHA1(c31d7c45a563cfc4533379f69f32889c79562534) )

	ROM_REGION( 0x10000, REGION_CPU4, 0 )
	ROM_LOAD( "b09_37.epr", 0x00000, 0x08000, CRC(4bdf15ed) SHA1(b960208e63cede116925e064279a6cf107aef81c) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "b09_34.ic62", 0x00000, 0x10000, CRC(0899666f) SHA1(032e3ddd4caa48f82592570616e16c084de91f3e) )
	ROM_LOAD( "b09_33.ic63", 0x10000, 0x10000, CRC(f5cac954) SHA1(71abdc545e0196ad4d357af22dd6312d10a1323f) )
	ROM_LOAD( "b09_32.ic64", 0x20000, 0x10000, CRC(4721dc59) SHA1(faad75d577344e9ba495059040a2cf0647567426) )
	ROM_LOAD( "b09_31.ic65", 0x30000, 0x10000, CRC(9c4a82bf) SHA1(daeac620c636013a36595ce9f37e84e807f88977) )
	ROM_LOAD( "b09_30.ic66", 0x40000, 0x10000, CRC(214a30e2) SHA1(3dcc3a89ed52e4dbf232d2a92a3e64975b46c2dd) )

	ROM_REGION( 0x2000, REGION_SOUND2, 0 )
	ROM_LOAD( "b09_36.ic111", 0x00000, 0x02000, CRC(51fd3a77) SHA1(1fcbadf1877e25848a1d1017322751560a4823c0) )
ROM_END

/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1987, mlanding,  0,        mlanding, mlanding,  0, ROT0, "Taito America Corporation", "Midnight Landing (Germany)", 0 )
GAME( 1987, mlandingj, mlanding, mlanding, mlandingj, 0, ROT0, "Taito Corporation", "Midnight Landing (Japan, rev 3)", 0 )
