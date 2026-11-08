// license:BSD-3-Clause
// copyright-holders:Philip Bennett, Paul Arnold, James Wallace, smf
/*
    Bell-Fruit Cobra II hardware
    Break Ball (BFM/ATOD, 1994)
*/

#include "driver.h"
#include "cpu/z180/z180.h"
#include "cpu/m6809/m6809.h"
#include "sound/2413intf.h"
#include "sound/upd7759.h"

union ADDR_REG
{
#ifdef LSB_FIRST
	struct { UINT16 loword, hiword; } as16bit;
	struct { UINT8 addr0, addr1, addr2; } as8bit;
#else
	struct { UINT16 hiword, loword; } as16bit;
	struct { UINT8 addr2, addr1, addr0; } as8bit;
#endif
	UINT32 addr;
};

static const UINT8 CMD_RUN         = 0x01;
static const UINT8 CMD_COLST       = 0x02;
static const UINT8 CMD_PARRD       = 0x04;
static const UINT8 CMD_SRCUP       = 0x08;
static const UINT8 CMD_DSTUP       = 0x10;
static const UINT8 CMD_LT0         = 0x20;
static const UINT8 CMD_LT1         = 0x40;
static const UINT8 CMD_LINEDRAW    = 0x80;

#define SRCDST_A_1      0x80

static const UINT8 MODE_SSIGN      = 0x80;
static const UINT8 MODE_DSIGN      = 0x40;
static const UINT8 MODE_YFRAC      = 0x20;
static const UINT8 MODE_BITTOBYTE  = 0x04;
static const UINT8 MODE_PALREMAP   = 0x10;

static const UINT8 CMPFUNC_LT      = 0x01;
static const UINT8 CMPFUNC_EQ      = 0x02;
static const UINT8 CMPFUNC_GT      = 0x04;
static const UINT8 CMPFUNC_BEQ     = 0x08;
static const UINT8 CMPFUNC_LOG0    = 0x10;
static const UINT8 CMPFUNC_LOG1    = 0x20;
static const UINT8 CMPFUNC_LOG2    = 0x40;
static const UINT8 CMPFUNC_LOG3    = 0x80;

struct bf_blitter_t
{
	union ADDR_REG    program;

	UINT8     control;
	UINT8     status;

	UINT8     command;
	union ADDR_REG    source;
	union ADDR_REG    dest;
	UINT8     modectl;
	UINT8     compfunc;
	UINT8     outercnt;

	UINT8     innercnt;
	UINT8     step;
	UINT8     pattern;
};

#define LOOPTYPE ( ( blitter.command&0x60 ) >> 5 )

static struct bf_blitter_t blitter;
static UINT8 *m_work_ram, *m_video_ram;
static UINT8 m_bank_data[4], m_rompage, m_h_scroll, m_v_scroll;
static UINT8 m_flip_8, m_flip_22, m_videomode, m_port_0, m_genio;
static UINT8 m_col4bit[16], m_col3bit[16], m_col8bit[256], m_col7bit[256], m_col6bit[256];
static int m_scanline_irq, m_global_volume;
static void *scanline_timer;
static UINT8 palram[768], pal_index[2], pal_component[2], pal_mask;
static UINT8 eeprom[1024];
static int ee_scl, ee_sda, ee_out, ee_state, ee_bits, ee_shift, ee_select, ee_addr;
static UINT8 dm_control, dm_scanline[9], dm_pixels[65 * 21], dm_data, dm_shift_data, dm_last_data;
static int dm_x, dm_shift, dm_data_avail;

static void update_irqs(void)
{
	cpunum_set_input_line(0, 0, m_scanline_irq ? ASSERT_LINE : CLEAR_LINE);
}

static void scanline_callback(int param)
{
	m_scanline_irq = 1;
	update_irqs();
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);
}

static UINT8 *blitter_get_addr(UINT32 addr)
{
	addr &= 0xfffff;
	if (addr < 0x10000)
		return memory_region(REGION_USER1) + addr;
	if (addr < 0x20000)
		return memory_region(REGION_USER1) + (m_rompage & 15) * 0x10000 + (addr & 0xffff);
	if (addr < 0x40000)
		return m_video_ram + addr - 0x20000;
	return m_work_ram + addr - 0x40000;
}

static void z8s180_bank(int num, int data)
{
	UINT8 *base;
	if (data < 4)
		base = memory_region(REGION_USER1) + data * 0x4000;
	else if (data < 8)
		base = memory_region(REGION_USER1) + (m_rompage & 15) * 0x10000 + (data - 4) * 0x4000;
	else if (data < 16)
		base = m_video_ram + (data - 8) * 0x4000;
	else
		base = m_work_ram + (data - 16) * 0x4000;
	memory_set_bankptr(num + 1, base);
}

#define BLUE_0 0
#define BLUE_1 1
#define BLUE_2 2
#define BLUE_3 3
#define GREEN_0 ( 0 << 2 )
#define GREEN_1 ( 1 << 2 )
#define GREEN_2 ( 2 << 2 )
#define GREEN_3 ( 3 << 2 )
#define GREEN_4 ( 4 << 2 )
#define GREEN_5 ( 5 << 2 )
#define GREEN_6 ( 6 << 2 )
#define GREEN_7 ( 7 << 2 )
#define RED_0 ( 0 << 5 )
#define RED_1 ( 1 << 5 )
#define RED_2 ( 2 << 5 )
#define RED_3 ( 3 << 5 )
#define RED_4 ( 4 << 5 )
#define RED_5 ( 5 << 5 )
#define RED_6 ( 6 << 5 )
#define RED_7 ( 7 << 5 )
static const UINT8 col4bit_default[16]=
{
	BLUE_0 | GREEN_0 | RED_0,
	BLUE_1,
	GREEN_2,
	BLUE_1 | GREEN_2,
	RED_2,
	RED_2 | BLUE_1,
	RED_2 | GREEN_2,
	RED_2 | GREEN_2 | BLUE_1,
	BLUE_2 | GREEN_5 | RED_5,
	BLUE_3,
	GREEN_7,
	BLUE_3 | GREEN_7,
	RED_7,
	RED_7 | BLUE_3,
	RED_7 | GREEN_7,
	RED_7 | GREEN_7 | BLUE_3
};

static const UINT8 col3bit_default[16]=
{
	0,
	BLUE_3,
	GREEN_7,
	BLUE_3 | GREEN_7,
	RED_7,
	RED_7 | BLUE_3,
	RED_7 | GREEN_7,
	RED_7 | GREEN_7 | BLUE_3,
	0,
	BLUE_3,
	GREEN_7,
	BLUE_3 | GREEN_7,
	RED_7,
	RED_7 | BLUE_3,
	RED_7 | GREEN_7,
	RED_7 | GREEN_7 | BLUE_3
};

static const UINT8 col76index[] = {0, 2, 4, 7};

static void RunBlit(void)
{
#define BLITPRG_READ(x)     blitter.x = *(blitter_get_addr(blitter.program.addr++))

	 int cycles_used = 0;

	do
	{
		UINT8 srcdata = 0;
		UINT8 dstdata = 0;

		BLITPRG_READ(source.as8bit.addr0);
		BLITPRG_READ(source.as8bit.addr1);
		BLITPRG_READ(source.as8bit.addr2);
		BLITPRG_READ(dest.as8bit.addr0);
		BLITPRG_READ(dest.as8bit.addr1);
		BLITPRG_READ(dest.as8bit.addr2);
		BLITPRG_READ(modectl);
		BLITPRG_READ(compfunc);
		BLITPRG_READ(outercnt);
		BLITPRG_READ(innercnt);
		BLITPRG_READ(step);
		BLITPRG_READ(pattern);

		if (blitter.dest.addr == 0)
			return;

		for (;;)
		{
			UINT8 innercnt = blitter.innercnt;
			dstdata = blitter.pattern;

			if (blitter.command & CMD_LINEDRAW)
			{
				do
				{
					if (blitter.modectl & MODE_YFRAC)
					{
						if (blitter.modectl & MODE_SSIGN )
							blitter.dest.as8bit.addr0--;
						else
							blitter.dest.as8bit.addr0++;
					}
					else
					{
						if (blitter.modectl & MODE_DSIGN )
							blitter.dest.as8bit.addr1--;
						else
							blitter.dest.as8bit.addr1++;
					}
					if( blitter.source.as8bit.addr0 < blitter.step )
					{
						blitter.source.as8bit.addr0 -= blitter.step;
						blitter.source.as8bit.addr0 += blitter.source.as8bit.addr1;

						if ( blitter.modectl & MODE_YFRAC )
						{
							if (blitter.modectl & MODE_DSIGN )
								blitter.dest.as8bit.addr1--;
							else
								blitter.dest.as8bit.addr1++;
						}
						else
						{
							if (blitter.modectl & MODE_SSIGN )
								blitter.dest.as8bit.addr0--;
							else
								blitter.dest.as8bit.addr0++;
						}
					}
					else
					{
						blitter.source.as8bit.addr0 -= blitter.step;
					}

					*blitter_get_addr( blitter.dest.addr) = blitter.pattern;

				} while (--innercnt);
			}
			else do
			{
				UINT8   inhibit = 0;

				UINT8   result = blitter.pattern;

				if (LOOPTYPE == 3 && innercnt == blitter.innercnt)
				{
					srcdata = *(blitter_get_addr( blitter.source.addr & 0xfffff));
					blitter.source.as16bit.loword++;

				}

				if (!(blitter.modectl & (MODE_BITTOBYTE | MODE_PALREMAP)))
				{
					if (LOOPTYPE == 0 || LOOPTYPE == 1)
					{
						srcdata = *(blitter_get_addr( blitter.source.addr & 0xfffff));

						if (blitter.modectl & MODE_SSIGN)
							blitter.source.as16bit.loword--;
						else
							blitter.source.as16bit.loword++;

						result = srcdata;
					}
				}

				if (LOOPTYPE == 0)
				{
					dstdata = *blitter_get_addr( blitter.dest.addr & 0xfffff);

				}

				if (blitter.modectl & MODE_BITTOBYTE)
				{
					inhibit = !(srcdata & (1 << (8 - innercnt)));
				}

				if (blitter.compfunc & CMPFUNC_BEQ)
				{
					if (srcdata == blitter.pattern)
					{
						inhibit = 1;

						if (blitter.command & CMD_COLST)
							return;
					}
				}
				if (blitter.compfunc & CMPFUNC_LT)
				{
					if ((srcdata & 0xc0) > (dstdata & 0xc0))
					{
						inhibit = 1;

						if (blitter.command & CMD_COLST)
							return;
					}
				}
				if (blitter.compfunc & CMPFUNC_EQ)
				{
					if ((srcdata & 0xc0) == (dstdata & 0xc0))
					{
						inhibit = 1;

						if (blitter.command & CMD_COLST)
							return;
					}
				}
				if (blitter.compfunc & CMPFUNC_GT)
				{
					if ((srcdata & 0xc0) < (dstdata & 0xc0))
					{
						inhibit = 1;

						if (blitter.command & CMD_COLST)
							return;
					}
				}

				if (!inhibit)
				{
					if (blitter.modectl == MODE_PALREMAP)
					{

						UINT8 dest = *blitter_get_addr( blitter.dest.addr);
						UINT8 newcol = *(blitter_get_addr( (blitter.source.addr + dest) & 0xfffff));

						*blitter_get_addr( blitter.dest.addr) = newcol;

					}
					else
					{
						UINT8 final_result = 0;

						if (blitter.compfunc & CMPFUNC_LOG3)
							final_result |= result & dstdata;

						if (blitter.compfunc & CMPFUNC_LOG2)
							final_result |= result & ~dstdata;

						if (blitter.compfunc & CMPFUNC_LOG1)
							final_result |= ~result & dstdata;

						if (blitter.compfunc & CMPFUNC_LOG0)
							final_result |= ~result & ~dstdata;

						*blitter_get_addr( blitter.dest.addr) = final_result;

					}
				}

				if (blitter.modectl & MODE_DSIGN)
					blitter.dest.as16bit.loword--;
				else
					blitter.dest.as16bit.loword++;

			} while (--innercnt);

			if (!--blitter.outercnt)
			{
				break;
			}
			else
			{
				if (blitter.command & CMD_DSTUP)
					blitter.dest.as16bit.loword += blitter.step;

				if (blitter.command & CMD_SRCUP)
					blitter.source.as16bit.loword += blitter.step;

				if (blitter.command & CMD_PARRD)
				{
					BLITPRG_READ(innercnt);
					BLITPRG_READ(step);
					BLITPRG_READ(pattern);
				}
			}
		}

		BLITPRG_READ(command);

	} while (blitter.command  & CMD_RUN);

}

/* State changes on the falling edge keep the ninth clock reserved for ACK. */
static void eeprom_sda(int level)
{
	if (ee_sda != level && ee_scl && ee_out)
	{
		ee_state = level ? 0 : 1;
		ee_bits = 0;
	}
	ee_sda = level;
}

static void eeprom_scl(int level)
{
	if (ee_scl == level)
		return;
	ee_scl = level;
	if (ee_state >= 1 && ee_state <= 3)
	{
		if (ee_bits < 8)
		{
			if (level) { ee_shift = ((ee_shift << 1) | ee_sda) & 255; ++ee_bits; }
		}
		else if (level)
			++ee_bits;
		else if (ee_bits == 8)
		{
			if (ee_state == 1)
			{
				ee_select = ee_shift;
				if ((ee_select & 0xf8) != 0xa0) ee_state = 0;
				else ee_state = (ee_select & 1) ? 4 : 2;
			}
			else if (ee_state == 2)
			{
				ee_addr = ((ee_select & 6) << 7) | ee_shift;
				ee_state = 3;
			}
			else
			{
				eeprom[ee_addr] = ee_shift;
				ee_addr = (ee_addr & ~15) | ((ee_addr + 1) & 15);
			}
			ee_out = ee_state == 0;
		}
		else { ee_bits = 0; ee_out = 1; }
	}
	else if (ee_state == 4)
	{
		ee_bits = 0;
		ee_state = 5;
	}
	else if (ee_state == 5)
	{
		if (ee_bits < 8)
		{
			if (level) ++ee_bits;
			else
			{
				if (!ee_bits) { ee_shift = eeprom[ee_addr]; ee_addr = (ee_addr + 1) & 1023; }
				ee_out = (ee_shift >> 7) & 1;
				ee_shift = (ee_shift << 1) & 255;
			}
		}
		else if (level) { if (ee_sda) ee_state = 0; ee_bits = 0; }
		else ee_out = 1;
	}
}

static NVRAM_HANDLER( bfcobra )
{
	if (read_or_write) mame_fwrite(file, eeprom, sizeof(eeprom));
	else if (file) mame_fread(file, eeprom, sizeof(eeprom));
	else memset(eeprom, 0xff, sizeof(eeprom));
}

static void dotmatrix_w(UINT8 data)
{
	data &= 0x1c;
	if (!(data & 0x10))
	{
		/* This line resets the display interface, leaving its CPU running. */
		dm_data_avail = 0;
		dm_shift = dm_shift_data = 0;
		dm_control = dm_x = 0;
	}
	else
	{
		if (!(data & 4) && (dm_last_data & 4))
		{
			dm_shift_data = (dm_shift_data << 1) | !(data & 8);
			if (++dm_shift == 8)
			{
				dm_shift = 0;
				dm_data = dm_shift_data;
				dm_data_avail = 1;
				cpunum_set_input_line(1, M6809_IRQ_LINE, HOLD_LINE);
			}
		}
	}
	dm_last_data = data;
}

static WRITE8_HANDLER( output0_w )
{
	UINT8 changed = (m_port_0 ^ data) & 0x60;
	m_port_0 = data;
	dotmatrix_w(data);
	/* Change SDA first when lowering SCL to avoid a spurious stop condition. */
	if (changed == 0x60 && !(data & 0x40))
	{
		eeprom_sda(!(data & 0x20));
		eeprom_scl(!(data & 0x40));
	}
	else
	{
		eeprom_scl(!(data & 0x40));
		eeprom_sda(!(data & 0x20));
	}
}

static READ8_HANDLER( input0_r )
{
	return readinputport(0) | ((ee_state ? ee_out : ee_sda) ? 0x80 : 0);
}

static READ8_HANDLER( input1_r ) { return readinputport(1); }

static WRITE8_HANDLER( rombank_w )
{
	m_rompage = data & 15;
	memory_set_bankptr(5, memory_region(REGION_USER1) + m_rompage * 0x10000);
}

static WRITE8_HANDLER( aux_upd7759_w )
{
	upd7759_set_bank_base(1, ((data >> 2) & 3) * 0x20000);
	upd7759_port_w(1, data >> 4);
	upd7759_reset_w(1, data & 1);
	upd7759_start_w(1, data & 2);
}

static READ8_HANDLER( aux_upd7759_r ) { return upd7759_busy_r(1); }

static void genio_w(UINT8 data)
{
	if ((m_genio & 0x10) && !(data & 0x10))
	{
		if (data & 0x20) { if (m_global_volume < 31) ++m_global_volume; }
		else if (m_global_volume) --m_global_volume;
		sndti_set_output_gain(SOUND_UPD7759, 0, 0, (32 - m_global_volume) / 32.0);
		sndti_set_output_gain(SOUND_UPD7759, 1, 0, (32 - m_global_volume) / 32.0);
		sndti_set_output_gain(SOUND_YM2413, 0, 0, (32 - m_global_volume) / 32.0);
		sndti_set_output_gain(SOUND_YM2413, 0, 1, (32 - m_global_volume) / 32.0);
	}
	upd7759_reset_w(0, !(data & 4));
	/* The slave chip starts when MD falls while it is out of reset. */
	if (!(m_genio & 2) && (data & 2))
	{
		upd7759_start_w(0, 0);
		upd7759_start_w(0, 1);
	}
	m_genio = data;
}

static void upd7759_dreq(int state)
{
	cpuintrf_push_context(0);
	activecpu_set_reg(Z180_IOLINES, state ? 0x800 : 0);
	cpuintrf_pop_context();
}

static READ8_HANDLER( chipset_r )
{
	UINT8 val = 0xff;

	switch(offset)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		{
			val = m_bank_data[offset];
			break;
		}
		case 6:
		{

			val = m_scanline_irq << 4;
			break;
		}
		case 7:
		{
			m_scanline_irq = 0;
			val = 0x1;

			update_irqs();
			break;
		}
		case 0x1C:
		{

			val = readinputport(3);
			break;
		}
		case 0x20:
		{

			val = blitter.dest.as8bit.addr0;
			break;
		}
		case 0x22:
		{
			val = 0x40 | readinputport(2) | ( upd7759_busy_r(0) ? 0x20 : 0 );
			break;
		}
		default:
		{
		}
	}

	return val;
}

static WRITE8_HANDLER( chipset_w )
{
	switch (offset)
	{
		case 0x00:
		case 0x01:
		case 0x02:
		case 0x03:
		{

			data &= 0x3f;
			m_bank_data[offset] = data;
			z8s180_bank(offset, data);
			break;
		}

		case 0x07:
			timer_adjust(scanline_timer, cpu_getscanlinetime(data), 0, 0);
			break;

		case 0x08:
		{
			m_flip_8 = data;
			break;
		}
		case 9:
			m_videomode = data;
			break;

		case 0x0B:
		{
			m_h_scroll = data;
			break;
		}
		case 0x0C:
		{
			m_v_scroll = data;
			break;
		}
		case 0x0E:
		{
			m_col4bit[5] = data;
			m_col3bit[5] = data;
			m_col3bit[5 + 8] = data;
			break;
		}
		case 0x0f:
		{
			m_col4bit[6] = data;
			m_col3bit[6] = data;
			m_col3bit[6 + 8] = data;
			break;
		}
		case 0x18:
		{
			blitter.program.as8bit.addr0 = data;
			break;
		}
		case 0x19:
		{
			blitter.program.as8bit.addr1 = data;
			break;
		}
		case 0x1A:
		{
			blitter.program.as8bit.addr2 = data;
			break;
		}
		case 0x20:
		{
			blitter.command = data;

			if (data & CMD_RUN)
				RunBlit();

			break;
		}
		case 0x22:
		{
			m_flip_22 = data;
			genio_w( data );
			break;
		}
		default:
		{
		}
	}
}

static WRITE8_HANDLER( ramdac_w )
{
	if (offset == 0) { pal_index[0] = data; pal_component[0] = 0; }
	else if (offset == 3) { pal_index[1] = data; pal_component[1] = 0; }
	else if (offset == 2) pal_mask = data;
	else
	{
		int index = pal_index[0];
		palram[index * 3 + pal_component[0]] = data & 63;
		palette_set_color(index, pal6bit(palram[index * 3]), pal6bit(palram[index * 3 + 1]), pal6bit(palram[index * 3 + 2]));
		if (++pal_component[0] == 3) { pal_component[0] = 0; ++pal_index[0]; }
	}
}

static READ8_HANDLER( ramdac_r )
{
	UINT8 data = palram[pal_index[1] * 3 + pal_component[1]];
	if (++pal_component[1] == 3) { pal_component[1] = 0; ++pal_index[1]; }
	return data;
}

static WRITE8_HANDLER( ym2413_w )
{
	if (offset & 1) YM2413_data_port_0_w(0, data);
	else YM2413_register_port_0_w(0, data);
}

/* The unconnected serial port keeps its transmit register empty. */
static READ8_HANDLER( acia_r ) { return offset ? 0 : 2; }

static ADDRESS_MAP_START( cobra_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x03fff) AM_RAMBANK(1)
	AM_RANGE(0x04000, 0x07fff) AM_RAMBANK(2)
	AM_RANGE(0x08000, 0x0bfff) AM_RAMBANK(3)
	AM_RANGE(0x0c000, 0x0ffff) AM_RAMBANK(4)
	AM_RANGE(0x10000, 0x1ffff) AM_ROMBANK(5)
	AM_RANGE(0x20000, 0x3ffff) AM_RAM AM_BASE(&m_video_ram)
	AM_RANGE(0x40000, 0xfffff) AM_RAM AM_BASE(&m_work_ram)
ADDRESS_MAP_END

static ADDRESS_MAP_START( cobra_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_FLAGS(AMEF_ABITS(8))
	AM_RANGE(0x00, 0x23) AM_READWRITE(chipset_r, chipset_w)
	AM_RANGE(0x24, 0x25) AM_WRITENOP
	AM_RANGE(0x26, 0x27) AM_READ(acia_r)
	AM_RANGE(0x2c, 0x2c) AM_WRITE(rombank_w)
	AM_RANGE(0x30, 0x30) AM_WRITE(upd7759_0_port_w)
	AM_RANGE(0x50, 0x53) AM_WRITE(ramdac_w)
	AM_RANGE(0x51, 0x51) AM_READ(ramdac_r)
	AM_RANGE(0x80, 0x87) AM_READ(input1_r)
	AM_RANGE(0x88, 0x8f) AM_READWRITE(input0_r, output0_w)
	AM_RANGE(0x90, 0x97) AM_READWRITE(aux_upd7759_r, ym2413_w)
	AM_RANGE(0x98, 0x9f) AM_WRITE(aux_upd7759_w)
ADDRESS_MAP_END

static WRITE8_HANDLER( dm_control_w )
{
	if ((dm_control & 2) && !(data & 2)) dm_x = 0;
	dm_control = data;
}

static WRITE8_HANDLER( dm_mux_w )
{
	if (dm_x < 9) dm_scanline[dm_x++] = data;
	if (dm_x == 9)
	{
		int row = ((data ^ 255) & 0x7c) >> 2;
		int x;
		if (row < 21)
			for (x = 0; x < 65; ++x)
				dm_pixels[row * 65 + x] = (dm_scanline[x / 8] >> (7 - (x & 7))) & 1;
	}
}

static READ8_HANDLER( dm_zero_r ) { return 0; }

static READ8_HANDLER( dm_comm_r )
{
	if (!dm_data_avail) return 0;
	dm_data_avail = 0;
	return dm_data;
}
static WRITE8_HANDLER( dm_nmi_w ) { cpunum_set_input_line(1, INPUT_LINE_NMI, CLEAR_LINE); }
static INTERRUPT_GEN( dm_nmi ) { cpunum_set_input_line(1, INPUT_LINE_NMI, ASSERT_LINE); }

static ADDRESS_MAP_START( dm_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x2000) AM_READWRITE(dm_zero_r, dm_control_w)
	AM_RANGE(0x2800, 0x2800) AM_READWRITE(dm_zero_r, dm_mux_w)
	AM_RANGE(0x3000, 0x3000) AM_READWRITE(dm_comm_r, MWA8_NOP)
	AM_RANGE(0x3800, 0x3800) AM_READWRITE(dm_zero_r, dm_nmi_w)
	AM_RANGE(0x4000, 0xffff) AM_ROM
ADDRESS_MAP_END

INPUT_PORTS_START( brkball )
	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON4 ) PORT_PLAYER(2) PORT_NAME("P2 FIRE C")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_COIN1 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_NAME("P1 Left Flip")
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_NAME("P1 Right Flip")
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2) PORT_NAME("P2 Left Flip")
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_BUTTON2 ) PORT_PLAYER(2) PORT_NAME("P2 Right Flip")
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_SERVICE1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_COIN2 )

	PORT_START
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_TILT ) PORT_NAME("TEST") PORT_CODE(KEYCODE_F1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )
INPUT_PORTS_END

static VIDEO_START( bfcobra )
{
	int i;
	memcpy(m_col4bit, col4bit_default, 16);
	memcpy(m_col3bit, col3bit_default, 16);
	for (i = 0; i < 256; ++i)
	{
		UINT8 col = i & 0x7f;
		m_col8bit[i] = i;
		col = (col & 0x1f) | (col76index[(col >> 5) & 3] << 5);
		m_col7bit[i] = col;
		m_col6bit[i] = (col & 3) | (col76index[(col >> 2) & 3] << 2) | (col76index[(col >> 4) & 3] << 5);
	}
	return 0;
}

static VIDEO_UPDATE( bfcobra )
{
	int x, y;
	int page = (m_flip_8 & m_flip_22 & 0x40) ? 0x10000 : 0;
	const UINT8 *hi = (m_videomode & 0x20) ? m_col3bit : m_col4bit;
	const UINT8 *lo = (m_videomode & 0x20) ? m_col7bit : (m_videomode & 0x40) ? m_col6bit : m_col8bit;
	for (y = 0; y < 256; ++y)
		for (x = 0; x < 256; ++x)
		{
			UINT8 pen = m_video_ram[page + (UINT16)((y + m_v_scroll) * 256) + (UINT8)(x + m_h_scroll)];
			int hires = ((m_videomode & 0x81) == 1) || ((m_videomode & 0x80) && (pen & 0x80));
			plot_pixel(bitmap, x * 2, y, Machine->pens[(hires ? hi[pen & 15] : lo[pen]) & pal_mask]);
			plot_pixel(bitmap, x * 2 + 1, y, Machine->pens[(hires ? hi[pen >> 4] : lo[pen]) & pal_mask]);
		}
	/* The separate score display occupies the hidden rows below the monitor. */
	for (y = 256; y < 304; ++y)
		for (x = 0; x < 512; ++x)
		{
			int dx = (x - 126) / 4, dy = (y - 259) / 2;
			int on = x >= 126 && x < 386 && y >= 259 && y < 301 && dm_pixels[dy * 65 + dx];
			plot_pixel(bitmap, x, y, Machine->pens[256 + on]);
		}
}

static MACHINE_RESET( bfcobra )
{
	int i;
	memset(&blitter, 0, sizeof(blitter));
	memset(dm_pixels, 0, sizeof(dm_pixels));
	for (i = 0; i < 4; ++i) { m_bank_data[i] = i; z8s180_bank(i, i); }
	m_rompage = 0;
	memory_set_bankptr(5, memory_region(REGION_USER1) + 0x10000);
	m_scanline_irq = m_genio = m_global_volume = m_port_0 = 0;
	m_h_scroll = m_v_scroll = m_flip_8 = m_flip_22 = m_videomode = 0;
	dm_data_avail = dm_x = dm_shift = dm_shift_data = dm_last_data = dm_control = dm_data = 0;
	ee_scl = ee_sda = ee_state = ee_bits = ee_shift = ee_addr = 0;
	ee_out = 1;
	pal_mask = 255;
	memset(pal_index, 0, sizeof(pal_index));
	memset(pal_component, 0, sizeof(pal_component));
	for (i = 0; i < 256; ++i)
		palette_set_color(i, pal3bit(i >> 5), pal3bit((i >> 2) & 7), pal2bit(i & 3));
	palette_set_color(256, 0, 0, 0);
	palette_set_color(257, 255, 64, 0);
	update_irqs();
	timer_adjust(scanline_timer, cpu_getscanlinetime(0), 0, 0);
}

static DRIVER_INIT( bfcobra ) { scanline_timer = timer_alloc(scanline_callback); }
static struct upd7759_interface upd_int = { 0, upd7759_dreq };
static struct upd7759_interface upd_aux = { REGION_SOUND1, 0 };

static MACHINE_DRIVER_START( bfcobra )
	MDRV_CPU_ADD(Z180, 24000000 / 2)
	MDRV_CPU_PROGRAM_MAP(cobra_mem, 0)
	MDRV_CPU_IO_MAP(cobra_io, 0)
	MDRV_CPU_ADD(M6809, 8000000 / 4)
	MDRV_CPU_PROGRAM_MAP(dm_mem, 0)
	MDRV_CPU_PERIODIC_INT(dm_nmi, TIME_IN_HZ(1500))
	MDRV_INTERLEAVE(16)
	MDRV_MACHINE_RESET(bfcobra)
	MDRV_NVRAM_HANDLER(bfcobra)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(2500)
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(512, 304)
	MDRV_VISIBLE_AREA(0, 511, 0, 255)
	MDRV_PALETTE_LENGTH(258)
	MDRV_VIDEO_START(bfcobra)
	MDRV_VIDEO_UPDATE(bfcobra)
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd_int)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.4)
	MDRV_SOUND_ADD(UPD7759, UPD7759_STANDARD_CLOCK)
	MDRV_SOUND_CONFIG(upd_aux)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.4)
	MDRV_SOUND_ADD(YM2413, 3579545)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_DRIVER_END

ROM_START( brkball )
	ROM_REGION( 0x100000, REGION_CPU1, ROMREGION_ERASEFF )
	ROM_REGION( 0x200000, REGION_USER1, 0 )
	ROM_LOAD( "95000352.bin", 0x000000, 0x80000, CRC(8daad60a) SHA1(10ac31b7791c2215d0561972ae63e1405361b908) )
	ROM_LOAD( "95000353.bin", 0x080000, 0x80000, CRC(8e7b84ed) SHA1(b5e9ffe08eabe1446aa583fb85548451d944e811) )

	ROM_REGION( 0x80000, REGION_SOUND1, 0 )
	ROM_LOAD( "sounds.bin", 0x00000, 0x80000, CRC(4e84402d) SHA1(f6056669f005d8790331be5b0f34d9441190b120) )

	ROM_REGION( 0x20000, REGION_CPU2, 0 )
	ROM_LOAD("ledv1.bin",  0x00000, 0x10000, CRC(ea918cb9) SHA1(9e7047613cf1cb4b9a7fefb8a02d8479a7b09e6a))
ROM_END

GAME( 1994, brkball, 0, bfcobra, brkball, bfcobra, ROT0, "BFM/ATOD", "Break Ball", GAME_IMPERFECT_GRAPHICS )
