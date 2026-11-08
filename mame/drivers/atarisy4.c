// license:BSD-3-Clause
// copyright-holders:Philip Bennett
/* Atari System IV hardware */

#include "driver.h"
#include "cpu/m68000/m68000.h"
#include "cpu/tms32010/tms32010.h"
#include "vidhrdw/poly.h"

static struct _gpu_
{

	UINT16 gr[8];

	UINT16 bcrw;
	UINT16 bcrx;
	UINT16 bcry;
	UINT16 bcrz;
	UINT16 psrw;
	UINT16 psrx;
	UINT16 psry;
	UINT16 psrz;

	UINT16 dpr;
	UINT16 ctr;
	UINT16 lfr;
	UINT16 ifr;
	UINT16 ecr;
	UINT16 far;
	UINT16 mcr;
	UINT16 qlr;
	UINT16 qar;

	UINT16 dhr;
	UINT16 dlr;

	UINT16 idr;
	UINT16 icd;

	UINT8  transpose;
	UINT8  vblank_wait;

	struct
	{
		INT16 x;
		INT16 y;
	} points[16];

	UINT16 pt_idx;
	int   poly_open;

	UINT16 clip_min_x;
	UINT16 clip_max_x;
	UINT16 clip_min_y;
	UINT16 clip_max_y;
} gpu;

static UINT8 r_color_table[256];
static UINT8 g_color_table[256];
static UINT8 b_color_table[256];

static UINT16 dsp_bank[2];
static UINT8 csr[2];

static UINT16 *m68k_ram;
static UINT16 *shared_ram[2];
static UINT16 *screen_ram;

static READ16_HANDLER( analog_r );
static MACHINE_RESET( atarisy4 );
static MACHINE_RESET( airrace );

static VIDEO_UPDATE( atarisy4 )
{
	int y;
	UINT32 offset = 0;

	if (gpu.bcrw & 0x80)
	{
		offset = 0;
	}
	else if (gpu.bcrx & 0x80)
	{
		offset = 0x10 << 5;
	}

	for (y = cliprect->min_y; y <= cliprect->max_y; ++y)
	{
		UINT16 *src = &screen_ram[(offset + (4096 * y)) / 2];
		UINT16 *dest = (UINT16 *)bitmap->line[y] + cliprect->min_x;
		int x;

		for (x = cliprect->min_x; x < cliprect->max_x; x += 2)
		{
			UINT16 data = *src++;

			*dest++ = Machine->pens[data & 0xff];
			*dest++ = Machine->pens[data >> 8];
		}
	}
}

static inline UINT32 xy_to_screen_addr(UINT32 x, UINT32 y)
{

	UINT32 offset = 0;

	if (~gpu.bcrw & 0x80)
	{
		offset = 0;
	}
	else if (~gpu.bcrx & 0x80)
	{
		offset = 0x10 << 5;
	}

	return (y * 4096) + offset + x;
}

static void image_mem_to_screen(int clip)
{
	INT16 y = gpu.gr[1] - 0x200;
	UINT16 h = gpu.gr[3];

	if (h & 0x8000)
		h = -h;

	while (h--)
	{
		UINT16 w = gpu.gr[2];
		INT16 x = gpu.gr[0] - 0x400;

		if (w & 0x8000)
			w = -w;

		if (y >= 0 && y <= 511)
		{
			while (w--)
			{
				if (x >= 0 && x <= 511)
				{
					UINT16 pix = screen_ram[xy_to_screen_addr(x,y) >> 1];

					if (x & 1)
						pix = (pix & (0x00ff)) | gpu.idr << 8;
					else
						pix = (pix & (0xff00)) | gpu.idr;

					screen_ram[xy_to_screen_addr(x,y) >> 1] = pix;
				}
				++x;
			}
		}
		++y;
	}
}

static void draw_polygon(UINT16 color)
{
	struct poly_vertex v[3];
	const rectangle clip = { 0, 511, 0, 511 };
	int i;
	memset(v, 0, sizeof(v));
	v[0].x = gpu.points[0].x;
	v[0].y = gpu.points[0].y;
	v[1].x = gpu.points[1].x;
	v[1].y = gpu.points[1].y;
	for (i = 2; i <= gpu.pt_idx; ++i)
	{
		const struct poly_scanline_data *scans;
		int y, x;
		v[2].x = gpu.points[i].x;
		v[2].y = gpu.points[i].y;
		scans = setup_triangle_0(&v[0], &v[1], &v[2], &clip);
		if (scans)
			for (y = scans->sy; y <= scans->ey; ++y)
			{
				const struct poly_scanline *line = &scans->scanline[y - scans->sy];
				for (x = line->sx; x <= line->ex; ++x)
				{
					UINT32 addr = xy_to_screen_addr(x, y);
					UINT16 pix = screen_ram[addr >> 1];
					screen_ram[addr >> 1] = (x & 1) ? ((pix & 0xff) | (color << 8)) : ((pix & 0xff00) | color);
				}
			}
		v[1] = v[2];
	}
}

static void execute_gpu_command(void)
{
	switch (gpu.ecr)
	{
		case 0x04:
		{
			gpu.transpose = 0;
			break;
		}
		case 0x05:
		{
			gpu.transpose = 1;
			break;
		}
		case 0x06:
		{
			gpu.idr = gpu.gr[0];
			break;
		}
		case 0x07:
		{
			gpu.icd = gpu.gr[0];
			break;
		}
		case 0x09:
		{
			gpu.clip_max_x = gpu.gr[0];
			gpu.clip_min_x = gpu.gr[1];
			gpu.clip_max_y = gpu.gr[2];
			gpu.clip_min_y = gpu.gr[3];
			break;
		}
		case 0x0b:
		{

			gpu.vblank_wait = 1;
			break;
		}
		case 0x16:
		{

			int i;
			int offset = xy_to_screen_addr(gpu.gr[0] - 0x400, gpu.gr[1] - 0x200);
			int table_offs = gpu.gr[2];

			for (i = 0; i < gpu.gr[3]; ++i)
			{
				UINT16 val = screen_ram[offset >> 1];
				val >>= (~offset & 1) << 3;

				if (gpu.gr[4] & 0x10)
					r_color_table[table_offs] = val;
				if (gpu.gr[4] & 0x20)
					g_color_table[table_offs] = val;
				if (gpu.gr[4] & 0x40)
					b_color_table[table_offs] = val;

				palette_set_color(table_offs, r_color_table[table_offs], g_color_table[table_offs], b_color_table[table_offs]);

				++table_offs;
				++offset;
			}

			break;
		}
		case 0x20:
		{
			image_mem_to_screen(0);
			break;
		}
		case 0x21:
		{
			image_mem_to_screen(1);
			break;
		}
		case 0x28:
		{
			gpu.points[0].x = gpu.gr[0] - 0x400;
			gpu.points[0].y = gpu.gr[1] - 0x200;
			gpu.pt_idx = 0;
			break;
		}
		case 0x29:
		{
			gpu.points[0].x = gpu.points[gpu.pt_idx].x + gpu.gr[0];
			gpu.points[0].y = gpu.points[gpu.pt_idx].y + gpu.gr[1];
			gpu.pt_idx = 0;
			break;
		}
		case 0x2a:
		{
			++gpu.pt_idx;
			gpu.points[gpu.pt_idx].x = gpu.gr[0] - 0x400;
			gpu.points[gpu.pt_idx].y = gpu.gr[1] - 0x200;
			break;
		}
		case 0x2b:
		{
			UINT16 x = gpu.points[gpu.pt_idx].x + gpu.gr[0];
			UINT16 y = gpu.points[gpu.pt_idx].y + gpu.gr[1];
			++gpu.pt_idx;
			gpu.points[gpu.pt_idx].x = x;
			gpu.points[gpu.pt_idx].y = y;
			break;
		}
		case 0x2c:
		{
			draw_polygon(gpu.gr[2]);
			break;
		}
		default:
			logerror("GPU COMMAND: %x\n", gpu.ecr);
	}
}

static WRITE16_HANDLER( gpu_w )
{
	switch (offset)
	{
		case 0x00:	gpu.gr[0] = data;	break;
		case 0x01:	gpu.gr[1] = data;	break;
		case 0x02:	gpu.gr[2] = data;	break;
		case 0x03:	gpu.gr[3] = data;	break;
		case 0x04:	gpu.gr[4] = data;	break;
		case 0x05:	gpu.gr[5] = data;	break;
		case 0x06:	gpu.gr[6] = data;	break;
		case 0x07:	gpu.gr[7] = data;	break;

		case 0x08:	gpu.bcrw = data;	break;
		case 0x09:	gpu.bcrx = data;	break;
		case 0x0a:	gpu.bcry = data;	break;
		case 0x0b:	gpu.bcrz = data;	break;
		case 0x0c:	gpu.psrw = data;	break;
		case 0x0d:	gpu.psrx = data;	break;
		case 0x0e:	gpu.psry = data;	break;
		case 0x0f:	gpu.psrz = data;	break;

		case 0x14:	gpu.dpr = data;		break;
		case 0x15:	gpu.ctr = data;		break;
		case 0x16:	gpu.ifr = data;		break;
		case 0x17:
		{
			gpu.ecr = data;
			execute_gpu_command();
			break;
		}
		case 0x1a:	gpu.far = data;		break;
		case 0x20:
		{
			gpu.mcr = data;

			if (~data & 0x08)
				cpunum_set_input_line(0, 6, CLEAR_LINE);

			break;
		}

		case 0x21:	gpu.qlr = data;		break;
		case 0x22:	gpu.qar = data;		break;
	}
}

static READ16_HANDLER( gpu_r )
{
	UINT16 res = 0;

	switch (offset)
	{
		case 0x08:	res = gpu.bcrw;		break;
		case 0x09:	res = gpu.bcrx;		break;
		case 0x0a:	res = gpu.bcry;		break;
		case 0x0b:	res = gpu.bcrz;		break;

		case 0x20:	res = gpu.mcr;		break;

		case 0x400:	res = 5;			break;
		case 0x420:	res = 5;			break;

		default:	logerror("GPU_R[%x]\n", offset);
	}

	return res;
}

static INTERRUPT_GEN( vblank_int )
{
	if (gpu.mcr & 0x08)
		cpunum_set_input_line(0, 6, ASSERT_LINE);
}

static READ16_HANDLER( m68k_shared_0_r )
{
	if (!BIT(csr[0], 3))
		return (shared_ram[0][offset]);
	else
		return 0xffff;
}

static WRITE16_HANDLER( m68k_shared_0_w )
{
	if (!BIT(csr[0], 3))
		COMBINE_DATA(&shared_ram[0][offset]);
}

static READ16_HANDLER( m68k_shared_1_r )
{
	if (!BIT(csr[1], 3))
		return (shared_ram[1][offset]);
	else
		return 0xffff;
}

static WRITE16_HANDLER( m68k_shared_1_w )
{
	if (!BIT(csr[1], 3))
		COMBINE_DATA(&shared_ram[1][offset]);
}

static READ16_HANDLER( dsp0_status_r )
{
	return csr[0];
}

static WRITE16_HANDLER( dsp0_control_w )
{
	cpunum_set_input_line(1, INPUT_LINE_RESET, data & 0x01 ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(1, 0, data & 0x02 ? ASSERT_LINE : CLEAR_LINE);

	csr[0] = data;
}

static READ16_HANDLER( dsp0_bio_r )
{
	return BIT(csr[0], 2);
}

static WRITE16_HANDLER( dsp0_bank_w )
{
	if (data & 0x4000)
	{

		csr[0] |= 0x10;

		if (BIT(csr[0], 5) == 1)
			fatalerror("68000 interrupt enable was set!");
	}

	data &= 0x3800;
	memory_set_bankptr(2, &shared_ram[0][data]);
	dsp_bank[0] = data;
}

static READ16_HANDLER( dsp1_status_r )
{
	return csr[1];
}

static WRITE16_HANDLER( dsp1_control_w )
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, data & 0x01 ? CLEAR_LINE : ASSERT_LINE);
	cpunum_set_input_line(2, 0, data & 0x02 ? ASSERT_LINE : CLEAR_LINE);

	csr[1] = data;
}

static READ16_HANDLER( dsp1_bio_r )
{
	return BIT(csr[1], 2);
}

static WRITE16_HANDLER( dsp1_bank_w )
{
	if (data & 0x4000)
	{

		csr[1] |= 0x10;

		if (BIT(csr[1], 5) == 1)
			fatalerror("68000 interrupt enable was set!");
	}

	data &= 0x3800;
	memory_set_bankptr(4, &shared_ram[1][data]);
	dsp_bank[1] = data;
}

static ADDRESS_MAP_START( main_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x00ffff) AM_RAM AM_BASE(&m68k_ram)
	AM_RANGE(0x010000, 0x01ffff) AM_RAM
	AM_RANGE(0x580000, 0x580001) AM_READ(input_port_0_word_r)
	AM_RANGE(0x588000, 0x588001) AM_READ(analog_r)
	AM_RANGE(0x598000, 0x598001) AM_NOP
	AM_RANGE(0x7c0000, 0x7c4fff) AM_READWRITE(m68k_shared_1_r, m68k_shared_1_w)
	AM_RANGE(0x7f0000, 0x7f4fff) AM_READWRITE(m68k_shared_0_r, m68k_shared_0_w)
	AM_RANGE(0x7f6000, 0x7f6001) AM_READWRITE(dsp0_status_r, dsp0_control_w)
	AM_RANGE(0xa00400, 0xbfffff) AM_RAM AM_BASE(&screen_ram)
	AM_RANGE(0xff8000, 0xff8fff) AM_READWRITE(gpu_r, gpu_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp0_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x07ff) AM_READWRITE(MRA16_BANK1, MWA16_BANK1)
	AM_RANGE(0x0800, 0x0fff) AM_READWRITE(MRA16_BANK2, MWA16_BANK2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp0_io_map, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00, 0x01) AM_WRITE(dsp0_bank_w)
	AM_RANGE(TMS32010_BIO, TMS32010_BIO) AM_READ(dsp0_bio_r)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp1_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x0000, 0x07ff) AM_READWRITE(MRA16_BANK3, MWA16_BANK3)
	AM_RANGE(0x0800, 0x0fff) AM_READWRITE(MRA16_BANK4, MWA16_BANK4)
ADDRESS_MAP_END

static ADDRESS_MAP_START( dsp1_io_map, ADDRESS_SPACE_IO, 16 )
	AM_RANGE(0x00, 0x01) AM_WRITE(dsp1_bank_w)
	AM_RANGE(TMS32010_BIO, TMS32010_BIO) AM_READ(dsp1_bio_r)
ADDRESS_MAP_END

 static READ16_HANDLER( analog_r )
{
	return (readinputport(2) << 8) | readinputport(1);
}

static INPUT_PORTS_START( atarisy4 )
	PORT_START
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_LEFT )
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_RIGHT )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_DOWN )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICKRIGHT_UP )
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_LEFT )
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_RIGHT )
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_DOWN )
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_JOYSTICKLEFT_UP )
	PORT_BIT( 0x0100, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x0200, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x0400, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x0800, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x1000, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x2000, IP_ACTIVE_LOW, IPT_BUTTON4 )
	PORT_BIT( 0x4000, IP_ACTIVE_LOW, IPT_BUTTON5 )
	PORT_BIT( 0x8000, IP_ACTIVE_LOW, IPT_BUTTON6 )

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_Y ) PORT_SENSITIVITY(50) PORT_KEYDELTA(5)

	PORT_START
	PORT_BIT( 0xff, 0x80, IPT_AD_STICK_X ) PORT_SENSITIVITY(50) PORT_KEYDELTA(5)
INPUT_PORTS_END

static MACHINE_DRIVER_START( atarisy4 )
	MDRV_CPU_ADD(M68000, 8000000)
	MDRV_CPU_PROGRAM_MAP(main_map, 0)
	MDRV_CPU_VBLANK_INT(vblank_int, 1)
	/* This core takes instruction clocks rather than the external DSP clock. */
	MDRV_CPU_ADD(TMS32010, 16000000/TMS32010_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(dsp0_map, 0)
	MDRV_CPU_IO_MAP(dsp0_io_map, 0)
	MDRV_MACHINE_RESET(atarisy4)
	MDRV_FRAMES_PER_SECOND(16000000.0 / (660 * 404))
	MDRV_VBLANK_DURATION((int)(1000000.0 * 660 * 20 / 16000000))
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_UPDATE_AFTER_VBLANK)
	MDRV_SCREEN_SIZE(512, 404)
	MDRV_VISIBLE_AREA(0, 511, 0, 383)
	MDRV_PALETTE_LENGTH(256)
	MDRV_VIDEO_UPDATE(atarisy4)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( airrace )
	MDRV_IMPORT_FROM(atarisy4)
	MDRV_CPU_ADD(TMS32010, 16000000/TMS32010_CLOCK_DIVIDER)
	MDRV_CPU_PROGRAM_MAP(dsp1_map, 0)
	MDRV_CPU_IO_MAP(dsp1_io_map, 0)
	MDRV_MACHINE_RESET(airrace)
MACHINE_DRIVER_END

ROM_START( laststar )
	ROM_REGION( 0x20000, REGION_USER1, 0 )
	ROM_LOAD( "demo.hex",  0x00000, 0x1c2c4, CRC(7f9e344e) SHA1(ff1462f4f3fa01c47b74d365c240b1c3fdd36755) )

	ROM_REGION( 0x20000, REGION_USER2, 0 )
	ROM_LOAD( "data1.hex", 0x00000, 0x077cb, CRC(0e75efc0) SHA1(badfcadd92625637bb3223bee986a29428c8d35a) )

	ROM_REGION( 0x20000, REGION_USER3, 0 )
	ROM_LOAD( "mathbs.lda", 0x00000, 0x01e00, CRC(4378739a) SHA1(7c360da99a1366a3315f4846244ed3b2514ef7ec) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "2d_data", 0x00000, 0x077cb, NO_DUMP )
ROM_END

ROM_START( airrace )
	ROM_REGION( 0x30000, REGION_USER1, 0 )
	ROM_LOAD( "demo.hex",  0x00000, 0x25d23, CRC(937fa025) SHA1(c393a05645ad5df9268f03fffad294477e6f090b) )

	ROM_REGION( 0x20000, REGION_USER3, 0 )
	ROM_LOAD( "mathbs.lda", 0x00000, 0x01e00, CRC(d215acd4) SHA1(853e792b343dc6bcb5b11d455af478a79edfd740) )

	ROM_REGION( 0x20000, REGION_GFX1, 0 )
	ROM_LOAD( "2d_data", 0x00000, 0x077cb, NO_DUMP )
ROM_END

static inline UINT8 hex_to_ascii(UINT8 in)
{
	if (in < 0x3a)
		return in - 0x30;
	else if (in < 0x47)
		return in - 0x37;
	else
		return in;
}

static void load_ldafile(const UINT8 *file)
{
#define READ_CHAR()     file[i++]
	int i = 0;

	while (1)
	{
		UINT8 c;
		UINT8 sum = 1;
		UINT16 len;
		UINT16 addr;
		UINT8 pval = 0;

		if (READ_CHAR() != 0x01)
			fatalerror("Bad .LDA file\n");

		if (READ_CHAR() != 0x00)
			fatalerror("Bad .LDA file\n");

		len = READ_CHAR();
		sum += len;

		c = READ_CHAR();
		len |= c << 8;
		sum += c;

		if (len == 6)
			break;

		addr = READ_CHAR();
		sum += addr;

		c = READ_CHAR();
		addr |= c << 8;
		sum += c;

		len -= 6;

		addr <<= 1;
		do
		{
			UINT8 data = READ_CHAR();
			sum += data;
			/* Assemble DSP words explicitly so host endianness cannot swap opcodes. */
			if (addr & 1)
				program_write_word_16be(addr - 1, (pval << 8) | data);
			else
				pval = data;
			addr ++;
		} while (--len);

		sum += READ_CHAR();

		if (sum != 0)
			fatalerror(".LDA checksum failure\n");
	}
}

static void load_hexfile(const UINT8 *file)
{
#define READ_HEX_CHAR()		hex_to_ascii(file[i++])

	UINT32 i = 0;
	UINT32 line = 1;
	int end = 0;

	while (1)
	{
		UINT8 len;
		UINT8 record;
		UINT8 checksum;
		UINT8 sum = 0;
		UINT8 addr_len;
		UINT32 addr = 0;

		while (file[i] == '\n') i++;

		if (file[i++] != '%')
			fatalerror("Error on line %d - invalid line start character", line);

		len = READ_HEX_CHAR() << 4;
		len |= READ_HEX_CHAR();

		sum += len & 0xf;
		sum += len >> 4;

		record = READ_HEX_CHAR();
		sum += record;

		if (record == 8)
		{
			end = 1;
		}
		else if (record == 3)
		{

			i += len - 2;
			goto next_line;
		}
		else if (record != 6)
		{
			fatalerror("Error on line %d - Invalid record type %d\n", line, record);
		}

		checksum = READ_HEX_CHAR() << 4;
		checksum |= READ_HEX_CHAR();

		addr_len = READ_HEX_CHAR();
		sum += addr_len;

		len = len - 6 - addr_len;

		while (addr_len)
		{
			UINT8 digit;

			addr <<= 4;
			digit = READ_HEX_CHAR();
			sum += digit;
			addr |= digit;
			--addr_len;
		}

		while (len)
		{
			UINT8 data;

			data = READ_HEX_CHAR() << 4;
			data |= READ_HEX_CHAR();
			sum += data >> 4;
			sum += data & 0xf;

			if (record == 6)
				program_write_byte_16be(addr++, data);

			len -= 2;
		}

		++i;

		if (sum != checksum)
			fatalerror("Checksum mismatch on line %d (Found 0x%.2x but expected 0x%.2x)\n", line, sum, checksum);

next_line:
		++line;

		if (end == 1)
			break;
	}
}

static void load_program(int airrace)
{
	int dsp;
	/* Every selected DSP bank must refer to allocated shared memory. */
	shared_ram[0] = auto_malloc(0x8000);
	shared_ram[1] = auto_malloc(0x8000);
	memset(shared_ram[0], 0, 0x8000);
	memset(shared_ram[1], 0, 0x8000);
	cpuintrf_push_context(0);
	load_hexfile(memory_region(REGION_USER1));
	if (!airrace)
		load_hexfile(memory_region(REGION_USER2));
	cpuintrf_pop_context();
	for (dsp = 0; dsp < (airrace ? 2 : 1); ++dsp)
	{
		memory_set_bankptr(1 + dsp * 2, shared_ram[dsp]);
		memory_set_bankptr(2 + dsp * 2, shared_ram[dsp] + 0x800);
		cpuintrf_push_context(dsp + 1);
		load_ldafile(memory_region(REGION_USER3));
		cpuintrf_pop_context();
	}
}

static DRIVER_INIT( laststar )
{
	load_program(0);
}

static DRIVER_INIT( airrace )
{
	/* Only Air Race has a second DSP and its control register. */
	memory_install_read16_handler(0, ADDRESS_SPACE_PROGRAM, 0x7c6000, 0x7c6001, 0, 0, dsp1_status_r);
	memory_install_write16_handler(0, ADDRESS_SPACE_PROGRAM, 0x7c6000, 0x7c6001, 0, 0, dsp1_control_w);
	load_program(1);
}

static MACHINE_RESET( atarisy4 )
{
	gpu.vblank_wait = 0;
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_RESET( airrace )
{
	gpu.vblank_wait = 0;
	cpunum_set_input_line(1, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
}

GAME( 1984, laststar, 0, atarisy4, atarisy4, laststar, ROT0, "Atari Games", "The Last Starfighter (prototype)", GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
GAME( 1985, airrace,  0, airrace,  atarisy4, airrace,  ROT0, "Atari Games", "Air Race (prototype)",             GAME_IMPERFECT_GRAPHICS | GAME_NO_SOUND )
