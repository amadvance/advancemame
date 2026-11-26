// license:BSD-3-Clause
// copyright-holders:Ville Linde
/*
    Konami 'ZR107' Hardware
    Konami, 1995-1996

    Driver by Ville Linde

    Hardware overview:

    ZR107 CPU board:
    ----------------
        IBM PowerPC 403GA at 32MHz (main CPU)
        Motorola MC68EC000 at 8MHz (sound CPU)
        Konami K056800 (MIRAC), sound system interface
        Konami K056230 (LANC), LAN interface
        Konami K058141 sound chip (same as 2x K054539)

    ZR107 GFX board:
    ----------------
        Analog Devices ADSP-21062 SHARC DSP at 36MHz
        Konami K056832 tilemap chip
        KS10071 (custom 3D pixel unit)
        KS10081 (custom 3D texel unit)

    GN678 GFX board (same as in gticlub.c):
    ----------------
        Analog Devices ADSP-21062 SHARC DSP at 36MHz
        Konami K001604 (2D tilemaps + 2x ROZ)
        2x KS10071 (custom 3D pixel unit)
        2x KS10081 (custom 3D texel unit)

    Known games on this hardware:

    Game                     | Year | ID    | CPU PCB | CG Board(s)
    --------------------------------------------------------------------------
    Midnight Run             | 1995 | GX476 | ZR107   | ZR107
    Winding Heat             | 1996 | GX677 | ZR107   | ZR107
    Jet Wave / Wave Shark    | 1996 | GX678 | ZR107   | GN678

    Jet Wave uses the lower board from GTI Club (GN678) and a ZR107 top board.
*/

/*
    Status:

    - The self test passes and midnrun reaches the attract mode, the course
      selection and the start of the race, with the 2D layers and the HUD
      drawn correctly.
    - The 3D polygons are never drawn during the game: at about frame 480 the
      PowerPC detects a problem in the DSP handshake and resets the SHARC. After
      that reboot the SHARC no longer sends display lists to the K001005.
      The cause is not found yet.
    - Sometimes the game stops at the "please adjust the steering wheel
      mechanically" message, depending on the analog input value at boot.
    - jetwave is untested and the LAN, force feedback and sensors are not
      emulated.
*/

#include "driver.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/sharc/sharc.h"
#include "machine/eeprom.h"
#include "machine/adc083x.h"
#include "vidhrdw/konamiic.h"
#include "vidhrdw/k001005.h"
#include "vidhrdw/k001604.h"
#include "sound/k054539.h"
#include "machine/konppc.h"

#define PPC_CPU		0
#define SOUND_CPU	1
#define DSP_CPU		2

static UINT8 led_reg0 = 0x7f, led_reg1 = 0x7f;

static UINT32 *sharc_dataram;
static UINT32 lanc_ram[0x800];

static UINT8 lanc_status;
static int lanc_irq;
static int ccu_vcth, ccu_vctl;

/* K056800 (MIRAC) */
static UINT8 host_to_snd_regs[4];
static UINT8 snd_to_host_regs[2];
static int snd_int_pending, snd_int_enabled;

/* the 68000 core keeps a single interrupt level, so the pending levels are tracked here */
static UINT8 sound_ctrl;
static int sound_irq_pending;

static WRITE32_HANDLER( paletteram32_w )
{
	int r,g,b;
	int r2,g2,b2;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 16) & 0x1f);
	g = ((data >> 21) & 0x1f);
	r = ((data >> 26) & 0x1f);
	b2 = ((data >> 0) & 0x1f);
	g2 = ((data >> 5) & 0x1f);
	r2 = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);
	b2 = (b2 << 3) | (b2 >> 2);
	g2 = (g2 << 3) | (g2 >> 2);
	r2 = (r2 << 3) | (r2 >> 2);

	palette_set_color((offset * 2) + 0, r, g, b);
	palette_set_color((offset * 2) + 1, r2, g2, b2);
}

static WRITE32_HANDLER( paletteram32_single_w )
{
	int r,g,b;

	COMBINE_DATA(&paletteram32[offset]);
	data = paletteram32[offset];

	b = ((data >> 0) & 0x1f);
	g = ((data >> 5) & 0x1f);
	r = ((data >> 10) & 0x1f);

	b = (b << 3) | (b >> 2);
	g = (g << 3) | (g >> 2);
	r = (r << 3) | (r >> 2);

	palette_set_color(offset, r, g, b);
}

/******************************************************************/

static void midnrun_tile_callback(int layer, int *code, int *color)
{
	*color += layer * 0x40;
}

VIDEO_START( midnrun )
{
	static int scrolld[2][4][2] = {
		{{ 0, 0}, {0, 0}, {0, 0}, {0, 0}},
		{{ 0, 0}, {0, 0}, {0, 0}, {0, 0}}
	};
	int i;

	if (k001006_start())
		return 1;

	if (k001005_start(DSP_CPU, Machine->drv->screen_width, Machine->drv->screen_height))
		return 1;

	if (K056832_vh_start(REGION_GFX2, K056832_BPP_8, 1, scrolld, midnrun_tile_callback, 0))
		return 1;

	K056832_set_LayerOffset(0, -29, -27);
	K056832_set_LayerOffset(1, -25, -27);
	for (i = 2; i < 4; ++i)
		K056832_set_LayerOffset(i, -29, -27);

	return 0;
}

VIDEO_UPDATE( midnrun )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	K056832_tilemap_draw_dj(bitmap, cliprect, 1, 0, 0);

	k001005_draw(bitmap, cliprect);

	K056832_tilemap_draw_dj(bitmap, cliprect, 0, 0, 0);

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
}

VIDEO_START( jetwave )
{
	if (k001006_start())
		return 1;

	if (k001005_start(DSP_CPU, 512, 384))
		return 1;

	return k001604_start(0, 0);
}

VIDEO_UPDATE( jetwave )
{
	fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	k001604_draw_back_layer(0, bitmap, cliprect);

	k001005_draw(bitmap, cliprect);

	k001604_draw_front_layer(0, bitmap, cliprect);

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);
}

/******************************************************************/

static READ32_HANDLER( ccu_r )
{
	UINT32 r = 0;

	/* Midnight Run polls the vertical counter in vblank */
	if (offset == 0x1c/4)
	{
		if (!(mem_mask & 0xff000000))
		{
			ccu_vcth ^= 0xff;
			r |= ccu_vcth << 24;
		}
		if (!(mem_mask & 0x0000ff00))
		{
			ccu_vctl = (ccu_vctl + 1) & 0x1ff;
			r |= (ccu_vctl >> 2) << 8;
		}
	}

	return r;
}

static WRITE32_HANDLER( ccu_w )
{
}

static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;

	if (offset == 0)
	{
		r |= readinputport(0) << 24;
		r |= readinputport(1) << 16;
		r |= (readinputport(2) | (adc083x_do_read(0) << 7)) << 8;
		r |= readinputport(3) << 0;
	}
	else if (offset == 1)
	{
		UINT32 in4 = (adc083x_sars_read(0) << 5) | (EEPROM_read_bit() << 4);
		r |= in4 << 24;
	}

	return r;
}

static WRITE32_HANDLER( sysreg_w )
{
	if (offset == 0)
	{
		if (!(mem_mask & 0xff000000))
		{
			led_reg0 = (data >> 24) & 0xff;
		}
		if (!(mem_mask & 0x00ff0000))
		{
			led_reg1 = (data >> 16) & 0xff;
		}
		if (!(mem_mask & 0x000000ff))
		{
			/* 0x10 = SNDRES, 0x04 = EEPCS, 0x02 = EEPCLK, 0x01 = EEPDI */
			EEPROM_write_bit(data & 0x01);
			EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			/* the EEPROM core treats CLEAR_LINE as chip selected */
			EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
			cpunum_set_input_line(SOUND_CPU, INPUT_LINE_RESET, (data & 0x10) ? CLEAR_LINE : ASSERT_LINE);
		}
	}
	else if (offset == 1)
	{
		if (!(mem_mask & 0xff000000))
		{
			if (data & 0x80000000)	/* CG Board 1 IRQ Ack */
			{
				cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ1, CLEAR_LINE);
			}
			if (data & 0x40000000)	/* CG Board 0 IRQ Ack */
			{
				cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ0, CLEAR_LINE);
			}
			/* 0x04 = ADCS, 0x02 = ADDI, 0x01 = ADDSCLK */
			adc083x_clk_write(0, (data >> 24) & 1);
			adc083x_di_write(0, (data >> 25) & 1);
			adc083x_cs_write(0, (data >> 26) & 1);
		}
	}
}

static void sound_update_irq(void)
{
	if (sound_irq_pending & 2)
		cpunum_set_input_line(SOUND_CPU, 2, ASSERT_LINE);
	else if (sound_irq_pending & 1)
		cpunum_set_input_line(SOUND_CPU, 1, ASSERT_LINE);
	else
		cpunum_set_input_line(SOUND_CPU, 1, CLEAR_LINE);
}

/* K056800 host side */

static READ32_HANDLER( ppc_sound_r )
{
	UINT32 rv = 0;
	int lane;

	for (lane = 0; lane < 4; ++lane)
	{
		int reg = ((offset * 4) + lane) & 7;

		if (!(mem_mask & (0xff000000 >> (lane * 8))) && reg < 2)
			rv |= snd_to_host_regs[reg] << (24 - lane * 8);
	}

	return rv;
}

static WRITE32_HANDLER( ppc_sound_w )
{
	int lane;

	for (lane = 0; lane < 4; ++lane)
	{
		int reg = ((offset * 4) + lane) & 7;
		int val = (data >> (24 - lane * 8)) & 0xff;

		if (mem_mask & (0xff000000 >> (lane * 8)))
			continue;

		if (reg < 4)
		{
			host_to_snd_regs[reg] = val;
		}
		else if (reg == 7)
		{
			/* sound interrupt */
			if (snd_int_enabled)
			{
				snd_int_pending = 1;
				sound_irq_pending |= 1;
				sound_update_irq();
			}
		}
	}
}

/* K056230 (LANC), the network is not emulated */

static void lanc_update_irq(int state)
{
	if (lanc_irq != state)
	{
		lanc_irq = state;
		cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ2, state);
	}
}

static READ32_HANDLER( lanc_r )
{
	UINT32 r = 0;

	if (offset == 0 && !(mem_mask & 0xff000000))
		r |= lanc_status << 24;

	return r;
}

static WRITE32_HANDLER( lanc_w )
{
	if (offset == 0 && !(mem_mask & 0x00ff0000))
	{
		UINT8 value = (data >> 16) & 0xff;

		if (value & 0x20)
			lanc_update_irq(ASSERT_LINE);
		if (!(value & 0x01))
			lanc_update_irq(CLEAR_LINE);
	}
}

static READ32_HANDLER( lanc_ram_r )
{
	return lanc_ram[offset & 0x7ff];
}

static WRITE32_HANDLER( lanc_ram_w )
{
	COMBINE_DATA(&lanc_ram[offset & 0x7ff]);
}

/******************************************************************/

static ADDRESS_MAP_START( midnrun_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_MIRROR(0x80000000) AM_RAM		/* Work RAM */
	AM_RANGE(0x74000000, 0x74001fff) AM_MIRROR(0x80002000) AM_READWRITE(K056832_ram_long_r, K056832_ram_long_w)
	AM_RANGE(0x74020000, 0x7402003f) AM_MIRROR(0x80000000) AM_READWRITE(K056832_long_r, K056832_long_w)
	AM_RANGE(0x74060000, 0x7406003f) AM_MIRROR(0x80000000) AM_READWRITE(ccu_r, ccu_w)
	AM_RANGE(0x74080000, 0x74081fff) AM_MIRROR(0x80000000) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x740a0000, 0x740a3fff) AM_MIRROR(0x80000000) AM_READ(K056832_rom_long_r)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)		/* 21N 21K 23N 23K */
	AM_RANGE(0x78010000, 0x7801ffff) AM_MIRROR(0x80000000) AM_WRITE(cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x78040000, 0x7804000f) AM_MIRROR(0x80000000) AM_READWRITE(k001006_0_r, k001006_0_w)
	AM_RANGE(0x780c0000, 0x780c0007) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_MIRROR(0x80000000) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e008000, 0x7e009fff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_r, lanc_w)	/* LANC registers */
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_ram_r, lanc_ram_w)	/* LANC Buffer RAM (27E) */
	AM_RANGE(0x7e00c000, 0x7e00c00f) AM_MIRROR(0x80000000) AM_READWRITE(ppc_sound_r, ppc_sound_w)
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

static ADDRESS_MAP_START( jetwave_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_MIRROR(0x80000000) AM_RAM		/* Work RAM */
	AM_RANGE(0x74000000, 0x740000ff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_reg_r, k001604_0_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_MIRROR(0x80000000) AM_READWRITE(paletteram32_r, paletteram32_single_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_tile_r, k001604_0_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_MIRROR(0x80000000) AM_READWRITE(k001604_0_char_r, k001604_0_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)		/* 21N 21K 23N 23K */
	AM_RANGE(0x78010000, 0x7801ffff) AM_MIRROR(0x80000000) AM_WRITE(cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x78040000, 0x7804000f) AM_MIRROR(0x80000000) AM_READWRITE(k001006_0_r, k001006_0_w)
	AM_RANGE(0x78080000, 0x7808000f) AM_MIRROR(0x80000000) AM_READWRITE(k001006_1_r, k001006_1_w)
	AM_RANGE(0x780c0000, 0x780c0007) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_MIRROR(0x80000000) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e008000, 0x7e009fff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_r, lanc_w)	/* LANC registers */
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_ram_r, lanc_ram_w)	/* LANC Buffer RAM (27E) */
	AM_RANGE(0x7e00c000, 0x7e00c00f) AM_MIRROR(0x80000000) AM_READWRITE(ppc_sound_r, ppc_sound_w)
	AM_RANGE(0x7f000000, 0x7f3fffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

/**********************************************************************/

static READ16_HANDLER( dual539_r )
{
	UINT16 ret = 0;

	if (ACCESSING_LSB16)
		ret |= K054539_1_r(offset);
	if (ACCESSING_MSB16)
		ret |= K054539_0_r(offset)<<8;

	return ret;
}

static WRITE16_HANDLER( dual539_w )
{
	if (ACCESSING_LSB16)
		K054539_1_w(offset, data);
	if (ACCESSING_MSB16)
		K054539_0_w(offset, data>>8);
}

/* K056800 sound side */

static READ16_HANDLER( sndcomm68k_r )
{
	if (ACCESSING_LSB16 && (offset & 7) < 4)
		return host_to_snd_regs[offset & 7];

	return 0;
}

static WRITE16_HANDLER( sndcomm68k_w )
{
	if (!ACCESSING_LSB16)
		return;

	switch (offset & 7)
	{
		case 0:
		case 1:
			snd_to_host_regs[offset & 7] = data;
			break;

		case 4:
			/* sound CPU interrupt control */
			snd_int_enabled = data & 1;

			if (snd_int_enabled)
			{
				if (snd_int_pending)
					sound_irq_pending |= 1;
			}
			else
			{
				snd_int_pending = 0;
				sound_irq_pending &= ~1;
			}
			sound_update_irq();
			break;
	}
}

static WRITE16_HANDLER( sound_ctrl_w )
{
	if (ACCESSING_LSB16)
	{
		if (!(data & 1))
		{
			sound_irq_pending &= ~2;
			sound_update_irq();
		}

		sound_ctrl = data;
	}
}

static void k054539_irq_gen(void)
{
	if (sound_ctrl & 1)
	{
		sound_irq_pending |= 2;
		sound_update_irq();
	}
}

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x01ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM		/* Work RAM */
	AM_RANGE(0x200000, 0x2004ff) AM_READWRITE(dual539_r, dual539_w)
	AM_RANGE(0x400000, 0x40001f) AM_READWRITE(sndcomm68k_r, sndcomm68k_w)
	AM_RANGE(0x500000, 0x500001) AM_WRITE(sound_ctrl_w)
	AM_RANGE(0x580000, 0x580001) AM_WRITENOP
ADDRESS_MAP_END

static struct K054539interface k054539_interface_1 =
{
	REGION_SOUND1,
	0,
	k054539_irq_gen
};

static struct K054539interface k054539_interface_2 =
{
	REGION_SOUND1
};

/*****************************************************************************/

static READ32_HANDLER( dsp_dataram_r )
{
	return sharc_dataram[offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram_w )
{
	sharc_dataram[offset] = data;
}

static ADDRESS_MAP_START( sharc_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x400000, 0x41ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x500000, 0x5fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(k001005_r, k001005_w)
	AM_RANGE(0x700000, 0x7000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
ADDRESS_MAP_END

/*****************************************************************************/

#define ZR107_IN1_IN2_PORTS \
	PORT_START_TAG("IN1") \
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
\
	PORT_START_TAG("IN2") \
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNKNOWN )

INPUT_PORTS_START( midnrun )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Start/View")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_NAME("Shift Up") PORT_4WAY
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_NAME("Shift Down") PORT_4WAY
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_NAME("AT/MT Switch")
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x0b, IP_ACTIVE_LOW, IPT_UNKNOWN )

	ZR107_IN1_IN2_PORTS

	PORT_START_TAG("IN3")
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0c, 0x0c, "Network ID" ) PORT_DIPLOCATION("SW:4,3")
	PORT_DIPSETTING( 0x0c, "1" )
	PORT_DIPSETTING( 0x08, "2" )
	PORT_DIPSETTING( 0x04, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Transmission Type" ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING( 0x02, "'T'Gate" )
	PORT_DIPSETTING( 0x00, "Button" )
	PORT_DIPNAME( 0x01, 0x01, "CG Board Type" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING( 0x01, "Single" )
	PORT_DIPSETTING( 0x00, "Twin" )

	PORT_START_TAG("IN4")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("ANALOG1")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_NAME("Steering Wheel") PORT_MINMAX(0x00,0xff) PORT_SENSITIVITY(25) PORT_KEYDELTA(16)

	PORT_START_TAG("ANALOG2")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_NAME("Gas Pedal") PORT_MINMAX(0x00,0x80) PORT_SENSITIVITY(25) PORT_KEYDELTA(16)

	PORT_START_TAG("ANALOG3")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL2 ) PORT_NAME("Brake Pedal") PORT_MINMAX(0x00,0x80) PORT_SENSITIVITY(25) PORT_KEYDELTA(16)
INPUT_PORTS_END

INPUT_PORTS_START( jetwave )
	PORT_START_TAG("IN0")
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 ) PORT_NAME("Start/View")
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_NAME("Angle")
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_NAME("Tilt Left") PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_NAME("Tilt Right") PORT_PLAYER(2)
	PORT_BIT( 0x07, IP_ACTIVE_LOW, IPT_UNKNOWN )

	ZR107_IN1_IN2_PORTS

	PORT_START_TAG("IN3")
	PORT_SERVICE_NO_TOGGLE( 0x80, IP_ACTIVE_LOW )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_DIPNAME( 0x0c, 0x0c, "Network ID" ) PORT_DIPLOCATION("SW:4,3")
	PORT_DIPSETTING( 0x0c, "1" )
	PORT_DIPSETTING( 0x08, "2" )
	PORT_DIPSETTING( 0x04, "3" )
	PORT_DIPSETTING( 0x00, "4" )
	PORT_DIPNAME( 0x02, 0x00, "Drive System" ) PORT_DIPLOCATION("SW:2")
	PORT_DIPSETTING( 0x00, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x02, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "Running Mode" ) PORT_DIPLOCATION("SW:1")
	PORT_DIPSETTING( 0x01, "Production" )
	PORT_DIPSETTING( 0x00, "Developer" )

	PORT_START_TAG("IN4")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START_TAG("ANALOG1")
	PORT_BIT( 0xff, 0x80, IPT_PADDLE ) PORT_NAME("Steering Handle") PORT_MINMAX(0x70,0x90) PORT_SENSITIVITY(25) PORT_KEYDELTA(4) PORT_REVERSE

	PORT_START_TAG("ANALOG2")
	PORT_BIT( 0xff, 0x00, IPT_PEDAL ) PORT_NAME("Accelerator") PORT_MINMAX(0x00,0x20) PORT_SENSITIVITY(25) PORT_KEYDELTA(4)

	PORT_START_TAG("ANALOG3")
	PORT_BIT( 0xff, 0x00, IPT_UNUSED )
INPUT_PORTS_END

/* ADC0838 interface */

static double adc0838_callback(int input)
{
	switch (input)
	{
		case ADC083X_CH0:
			return (double)(5 * readinputportbytag("ANALOG1")) / 255.0;
		case ADC083X_CH1:
			return (double)(5 * readinputportbytag("ANALOG2")) / 255.0;
		case ADC083X_CH2:
			return (double)(5 * readinputportbytag("ANALOG3")) / 255.0;
		case ADC083X_VREF:
			return 5.0;
	}
	return 0.0;
}

static ppc_config zr107_ppc_cfg =
{
	PPC_MODEL_403GA
};

static sharc_config sharc_cfg =
{
	BOOT_MODE_EPROM
};

/* PowerPC interrupts

    IRQ0:  Vblank
    IRQ2:  LANC
    DMA0

*/
static INTERRUPT_GEN( zr107_vblank )
{
	cpunum_set_input_line(PPC_CPU, INPUT_LINE_IRQ0, ASSERT_LINE);
	cpunum_set_input_line(DSP_CPU, SHARC_INPUT_FLAG1, ASSERT_LINE);
}

static MACHINE_RESET( zr107 )
{
	lanc_status = 0x08;
	lanc_irq = CLEAR_LINE;
	ccu_vcth = ccu_vctl = 0;
	sound_ctrl = 0;
	sound_irq_pending = 0;
	snd_int_pending = snd_int_enabled = 0;
	memset(host_to_snd_regs, 0, sizeof(host_to_snd_regs));
	memset(snd_to_host_regs, 0, sizeof(snd_to_host_regs));

	cpunum_set_input_line(DSP_CPU, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_DRIVER_START( zr107 )

	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(zr107_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(midnrun_map, 0)
	MDRV_CPU_VBLANK_INT(zr107_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/8)	/* 8MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(93C46)
	MDRV_MACHINE_RESET(zr107)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(midnrun)
	MDRV_VIDEO_UPDATE(midnrun)

	MDRV_SPEAKER_STANDARD_STEREO("left", "right")

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface_1)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)

	MDRV_SOUND_ADD(K054539, 48000)
	MDRV_SOUND_CONFIG(k054539_interface_2)
	MDRV_SOUND_ROUTE(0, "left", 0.75)
	MDRV_SOUND_ROUTE(1, "right", 0.75)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( jetwave )
	MDRV_IMPORT_FROM(zr107)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_PROGRAM_MAP(jetwave_map, 0)

	/* video hardware */
	MDRV_SCREEN_SIZE(1024, 1024)
	MDRV_VISIBLE_AREA(40, 511+40, 27, 383+27)

	MDRV_VIDEO_START(jetwave)
	MDRV_VIDEO_UPDATE(jetwave)
MACHINE_DRIVER_END

/*****************************************************************************/

static DRIVER_INIT(zr107)
{
	init_konami_cgboard(0, CGBOARD_TYPE_ZR107);
	sharc_dataram = auto_malloc(0x100000);
	adc083x_init(0, ADC0838, adc0838_callback);
}

static DRIVER_INIT(jetwave)
{
	init_konami_cgboard(0, CGBOARD_TYPE_GTICLUB);
	sharc_dataram = auto_malloc(0x100000);
	adc083x_init(0, ADC0838, adc0838_callback);
}

/*****************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,hash)      ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START(midnrun)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("476ea1a01.20u", 0x000003, 0x80000, CRC(ea70edf2) SHA1(51c882383a150ba118ccd39eb869525fcf5eee3c))
	ROM_LOAD32_BYTE("476ea1a02.17u", 0x000002, 0x80000, CRC(1462994f) SHA1(c8614c6c416f81737cc77c46eea6d8d440bc8cf3))
	ROM_LOAD32_BYTE("476ea1a03.15u", 0x000001, 0x80000, CRC(b770ae46) SHA1(c61daa8353802957eb1c2e2c6204c3a98569627e))
	ROM_LOAD32_BYTE("476ea1a04.13u", 0x000000, 0x80000, CRC(9644b277) SHA1(b9cb812b6035dfd93032d277c8aa0037cf6b3dbe))

	ROM_REGION(0x20000, REGION_CPU2, 0)		/* M68K program */
	ROM_LOAD16_WORD_SWAP("477a07.19l", 0x000000, 0x20000, CRC(a82c0ba1) SHA1(dad69f2e5e75009d70cc2748477248ec47627c30))

	ROM_REGION(0x100000, REGION_GFX2, 0)	/* Tilemap */
	ROM_LOAD16_BYTE("477a11.35b", 0x000000, 0x80000, CRC(85eef04b) SHA1(02e26d2d4a8b29894370f28d2a49fdf5c7d23f95))
	ROM_LOAD16_BYTE("477a12.35a", 0x000001, 0x80000, CRC(451d7777) SHA1(0bf280ca475100778bbfd3f023547bf0413fc8b7))

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* Texture data */
	ROM_LOAD64_WORD("477a13.9h", 0x000000, 0x200000, CRC(b1ee901d) SHA1(b1432cb1379b35d99d3f2b7f6409db6f7e88121d))
	ROM_LOAD64_WORD("477a14.7h", 0x000002, 0x200000, CRC(9ffa8cc5) SHA1(eaa19e26df721bec281444ca1c5ccc9e48df1b0b))
	ROM_LOAD64_WORD("477a15.5h", 0x000004, 0x200000, CRC(e337fce7) SHA1(c84875f3275efd47273508b340231721f5a631d2))
	ROM_LOAD64_WORD("477a16.2h", 0x000006, 0x200000, CRC(2c03ee63) SHA1(6b74d340dddf92bb4e4b1e037f003d58c65d8d9b))

	ROM_REGION(0x600000, REGION_SOUND1, 0)	/* Sound data */
	ROM_LOAD("477a08.5r", 0x000000, 0x200000, CRC(d320dbde) SHA1(eb602cad6ac7c7151c9f29d39b10041d5a354164))
	ROM_LOAD("477a09.3r", 0x200000, 0x200000, CRC(f431e29f) SHA1(e6082d88f86abb63d02ac34e70873b58f88b0ddc))
	ROM_LOAD("477a10.5n", 0x400000, 0x200000, CRC(8db31bd4) SHA1(d662d3bb6e8b44a01ffa158f5d7425454aad49a3))
ROM_END

ROM_START(windheat)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("677ubc01.20u", 0x000003, 0x080000, CRC(63198721) SHA1(7f34131bf51d573d0c683b28df2567a0b911c98c))
	ROM_LOAD32_BYTE("677ubc02.17u", 0x000002, 0x080000, CRC(bdb00e2d) SHA1(c54b2250047576e12e9936300989e40494b4659d))
	ROM_LOAD32_BYTE("677ubc03.15u", 0x000001, 0x080000, CRC(0f7d8c1f) SHA1(63de03c7be794b6dae8d0af69e894ac573dbbc11))
	ROM_LOAD32_BYTE("677ubc04.13u", 0x000000, 0x080000, CRC(4e42791c) SHA1(a53c6374c6b46db578be4ced2ee7c2af7062d961))

	ROM_REGION(0x20000, REGION_CPU2, 0)		/* M68K program */
	ROM_LOAD16_WORD_SWAP("677a07.19l", 0x000000, 0x020000, CRC(05b14f2d) SHA1(3753f71173594ee741980e08eed0f7c3fc3588c9))

	ROM_REGION(0x100000, REGION_GFX2, 0)	/* Tilemap */
	ROM_LOAD16_BYTE("677a11.35b", 0x000000, 0x080000, CRC(bf34f00f) SHA1(ca0d390c8b30d0cfdad4cfe5a601cc1f6e8c263d))
	ROM_LOAD16_BYTE("677a12.35a", 0x000001, 0x080000, CRC(458f0b1d) SHA1(8e11023c75c80b496dfc62b6645cfedcf2a80db4))

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* Texture data */
	ROM_LOAD64_WORD("677a13.9h", 0x000000, 0x200000, CRC(7937d226) SHA1(c2ba777292c293e31068eeb3a27353ad2595b413))
	ROM_LOAD64_WORD("677a14.7h", 0x000002, 0x200000, CRC(2568cf41) SHA1(6ed01922943486dafbdc863b76b2036c1fbe5281))
	ROM_LOAD64_WORD("677a15.5h", 0x000004, 0x200000, CRC(62e2c3dd) SHA1(c9127ed70bdff947c3da2908a08974091615a685))
	ROM_LOAD64_WORD("677a16.2h", 0x000006, 0x200000, CRC(7cc75539) SHA1(4bd8d88debf7489f30008bd4cbded67cb1a20ab0))

	ROM_REGION(0x600000, REGION_SOUND1, 0)	/* Sound data */
	ROM_LOAD("677a08.5r", 0x000000, 0x200000, CRC(bde38850) SHA1(aaf1bdfc25ecdffc1f6076c9c1b2edbe263171d2))
	ROM_LOAD("677a09.3r", 0x200000, 0x200000, CRC(4dfc1ea9) SHA1(4ab264c1902b522bc0589766e42f2b6ca276808d))
	ROM_LOAD("677a10.5n", 0x400000, 0x200000, CRC(d8f77a68) SHA1(ff251863ef096f0864f6cbe6caa43b0aa299d9ee))
ROM_END

ROM_START(jetwave)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
	ROM_LOAD32_BYTE("678eab01.20u", 0x000003, 0x080000, CRC(bc657198) SHA1(e521bb2c1b1a3ae934c98ce1656d35821fc287c9))
	ROM_LOAD32_BYTE("678eab02.17u", 0x000002, 0x080000, CRC(a9a57090) SHA1(ae0273b00c64687f8f835aba531580654edd1097))
	ROM_LOAD32_BYTE("678eab03.15u", 0x000001, 0x080000, CRC(483aaff0) SHA1(86e011337532f6ff0174393758784b276143ba10))
	ROM_LOAD32_BYTE("678eab04.13u", 0x000000, 0x080000, CRC(c7580d72) SHA1(6a5652365a85917ac48b0f1ced70b9c311e89a4f))

	ROM_REGION(0x20000, REGION_CPU2, 0)		/* M68K program */
	ROM_LOAD16_WORD_SWAP("678a07.19l", 0x000000, 0x020000, CRC(bb3f5875) SHA1(97f80d9b55d4177217b7cd1ba14e8ed2d64376bb))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
	ROM_LOAD32_WORD_SWAP("685a05.10u", 0x000000, 0x200000, CRC(00e59741) SHA1(d799910d4e85482b0e92a3cc9043f81d97b2fb02))
	ROM_LOAD32_WORD_SWAP("685a06.8u",  0x000002, 0x200000, CRC(fc98c6a5) SHA1(a84583bb7296fa9e0c284b2ac59e2dc7b2689eee))

	ROM_REGION(0x800000, REGION_GFX1, 0)	/* Texture data */
	ROM_LOAD64_WORD("678a13.18d", 0x000000, 0x200000, CRC(ccf75722) SHA1(f48d21dfc4f82adbb4c9c841a809267cfd028a3d))
	ROM_LOAD64_WORD("678a14.13d", 0x000002, 0x200000, CRC(333a1ab4) SHA1(79df4a98b7871eba4157307a7709da8f8b5da39b))
	ROM_LOAD64_WORD("678a15.9d",  0x000004, 0x200000, CRC(58b670f8) SHA1(5d4facb00e34de3ad11ed60c19835918a9cf6cb9))
	ROM_LOAD64_WORD("678a16.4d",  0x000006, 0x200000, CRC(137b9bff) SHA1(5052c1fa30cc1d6affd78f48d483415dca89d10b))

	ROM_REGION(0x600000, REGION_SOUND1, 0)	/* Sound data */
	ROM_LOAD("678a08.5r", 0x000000, 0x200000, CRC(4aeb61ad) SHA1(ec6872cb2e4776849963f48c1c245ca7697849e0))
	ROM_LOAD("678a09.3r", 0x200000, 0x200000, CRC(39baef23) SHA1(9f7bda0f9c06eee94703f9ceb06975c8e28338cc))
	ROM_LOAD("678a10.5n", 0x400000, 0x200000, CRC(0508280e) SHA1(a36c5dc377b0ba597f131bd9dfc6019e7fc2d243))
ROM_END

/*****************************************************************************/

GAME( 1995, midnrun,	0,		zr107,	midnrun,	zr107,		ROT0,	"Konami",	"Midnight Run: Road Fighter 2 (EAA, Euro v1.11)", GAME_NOT_WORKING )
GAME( 1996, windheat,	0,		zr107,	midnrun,	zr107,		ROT0,	"Konami",	"Winding Heat", GAME_NOT_WORKING )
GAME( 1996, jetwave,	0,		jetwave,	jetwave,	jetwave,	ROT0,	"Konami",	"Jet Wave (EAB, Euro v1.04)", GAME_NOT_WORKING )
