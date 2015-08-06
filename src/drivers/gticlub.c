/*  Konami GTI Club System

    Driver by Ville Linde
*/

#include "driver.h"
#include "machine/eeprom.h"
#include "cpu/powerpc/ppc.h"
#include "cpu/sharc/sharc.h"
#include "machine/konppc.h"
#include "vidhrdw/poly.h"

static UINT32 *work_ram;

static UINT8 led_reg0 = 0x7f, led_reg1 = 0x7f;

int K001604_vh_start(void);
void K001604_tile_update(void);
void K001604_tile_draw(mame_bitmap *bitmap, const rectangle *cliprect);
READ32_HANDLER(K001604_tile_r);
READ32_HANDLER(K001604_char_r);
WRITE32_HANDLER(K001604_tile_w);
WRITE32_HANDLER(K001604_char_w);
WRITE32_HANDLER(K001604_reg_w);
READ32_HANDLER(K001604_reg_r);

void K001005_render(mame_bitmap *bitmap, const rectangle *cliprect);
void K001005_draw(void);

typedef struct {
	float x,y,z;
	UINT32 u,v;
} VERTEX;

UINT32 K001005_status = 0;

mame_bitmap *K001005_bitmap;
rectangle K001005_cliprect;

static WRITE32_HANDLER( paletteram32_w )
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

#define MAX_K001006_CHIPS		2

static UINT16 K001006_pal_ram[MAX_K001006_CHIPS][0x800];
static UINT16 K001006_unknown_ram[MAX_K001006_CHIPS][0x1000];
static UINT32 K001006_addr[MAX_K001006_CHIPS] = { 0, 0 };
static int K001006_device_sel[MAX_K001006_CHIPS] = { 0, 0 };

static UINT32 K001006_r(int chip, int offset, UINT32 mem_mask)
{
	if (offset == 1)
	{
		switch (K001006_device_sel[chip])
		{
			case 0x0b:		// CG Board ROM read
			{
				UINT16 *rom = (UINT16*)memory_region(REGION_GFX1);
				return rom[K001006_addr[chip] / 2] << 16;
			}
			case 0x0d:		// Palette RAM read
			{
				return K001006_pal_ram[chip][K001006_addr[chip]++];
			}
			case 0x0f:		// Unknown RAM read
			{
				return K001006_unknown_ram[chip][K001006_addr[chip]++];
			}
			default:
			{
				fatalerror("K001006_r chip %d, unknown device %02X", chip, K001006_device_sel[chip]);
			}
		}
	}
	return 0;
}

static void K001006_w(int chip, int offset, UINT32 data, UINT32 mem_mask)
{
	if (offset == 0)
	{
		COMBINE_DATA(&K001006_addr[chip]);
	}
	else if (offset == 1)
	{
		switch (K001006_device_sel[chip])
		{
			case 0xd:	// Palette RAM write
			{
				K001006_pal_ram[chip][K001006_addr[chip]++] = data & 0xffff;
				break;
			}
			case 0xf:	// Unknown RAM write
			{
			//  printf("Unknown RAM %08X = %04X\n", K001006_addr[chip], data & 0xffff);
				K001006_unknown_ram[chip][K001006_addr[chip]++] = data & 0xffff;
				break;
			}
			default:
			{
				printf("K001006_w: chip %d, device %02X, write %04X to %08X\n", chip, K001006_device_sel[chip], data & 0xffff, K001006_addr[0]++);
			}
		}
	}
	else if (offset == 2)
	{
		if (!(mem_mask & 0xffff0000))
		{
			K001006_device_sel[chip] = (data >> 16) & 0xf;
		}
	}
}

READ32_HANDLER(K001006_0_r)
{
	return K001006_r(0, offset, mem_mask);
}

WRITE32_HANDLER(K001006_0_w)
{
	K001006_w(0, offset, data, mem_mask);
}

READ32_HANDLER(K001006_1_r)
{
	return K001006_r(1, offset, mem_mask);
}

WRITE32_HANDLER(K001006_1_w)
{
	K001006_w(1, offset, data, mem_mask);
}



VIDEO_START( gticlub )
{
	K001005_bitmap = auto_bitmap_alloc(Machine->drv->screen_width, Machine->drv->screen_height);
	if (!K001005_bitmap)
		return 1;

	return K001604_vh_start();
}

static const int decode_x[8] = {  0, 16, 2, 18, 4, 20, 6, 22 };
static const int decode_y[16] = {  0, 8, 32, 40, 1, 9, 33, 41, 64, 72, 96, 104, 65, 73, 97, 105 };

static const int texture_decode_x[64] =
{
	0x000, 0x010, 0x002, 0x012, 0x004, 0x014, 0x006, 0x016,
	0x080, 0x090, 0x082, 0x092, 0x084, 0x094, 0x086, 0x096,
	0x200, 0x210, 0x202, 0x212, 0x204, 0x214, 0x206, 0x216,
	0x280, 0x290, 0x282, 0x292, 0x284, 0x294, 0x286, 0x296,
	0x800, 0x810, 0x802, 0x812, 0x804, 0x814, 0x806, 0x816,
	0x880, 0x890, 0x882, 0x892, 0x884, 0x894, 0x886, 0x896,
	0xa00, 0xa10, 0xa02, 0xa12, 0xa04, 0xa14, 0xa06, 0xa16,
	0xa80, 0xa90, 0xa82, 0xa92, 0xa84, 0xa94, 0xa86, 0xa96
};
static const int texture_decode_y[64] =
{
	0x000, 0x008, 0x020, 0x028, 0x001, 0x009, 0x021, 0x029,
	0x040, 0x048, 0x060, 0x068, 0x041, 0x049, 0x061, 0x069,
	0x100, 0x108, 0x120, 0x128, 0x101, 0x109, 0x121, 0x129,
	0x140, 0x148, 0x160, 0x168, 0x141, 0x149, 0x161, 0x169,
	0x400, 0x408, 0x420, 0x428, 0x401, 0x409, 0x421, 0x429,
	0x440, 0x448, 0x460, 0x468, 0x441, 0x449, 0x461, 0x469,
	0x500, 0x508, 0x520, 0x528, 0x501, 0x509, 0x521, 0x529,
	0x540, 0x548, 0x560, 0x568, 0x541, 0x549, 0x561, 0x569
};

static int tick = 0;
static int texture_page = 0;
static int texture_palette = 0;

VIDEO_UPDATE( gticlub )
{
	//fillbitmap(bitmap, Machine->remapped_colortable[0], cliprect);

	memcpy(&K001005_cliprect, &cliprect, sizeof(rectangle));

	copybitmap(bitmap, K001005_bitmap, 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);

	if (K001005_status == 2)
	{
		fillbitmap(K001005_bitmap, Machine->remapped_colortable[0], cliprect);
	}

	K001604_tile_update();
	K001604_tile_draw(bitmap, cliprect);

	//K001005_render(bitmap, cliprect);

	tick++;
	if( tick >= 5 ) {
		tick = 0;

		if( code_pressed(KEYCODE_O) )
			texture_page++;

		if( code_pressed(KEYCODE_I) )
			texture_page--;

		if (code_pressed(KEYCODE_U))
			texture_palette++;
		if (code_pressed(KEYCODE_Y))
			texture_palette--;

		if (texture_page < 0)
			texture_page = 32;
		if (texture_page > 32)
			texture_page = 0;

		if (texture_palette < 0)
			texture_palette = 15;
		if (texture_palette > 15)
			texture_palette = 0;
	}

	if (texture_page > 0)
	{
		char string[200];
		int i,x,y;
		int index = (texture_page - 1) * 0x40000;
		int pal = texture_palette & 7;
		int tp = (texture_palette >> 3) & 1;
		UINT8 *rom = memory_region(REGION_GFX1);
		for (i=0; i < 0x800; i++)
		{
			int tx = ((i & 0x400) >> 5) | ((i & 0x100) >> 4) | ((i & 0x40) >> 3) | ((i & 0x10) >> 2) | ((i & 0x4) >> 1) | (i & 0x1);
			int ty = ((i & 0x200) >> 5) | ((i & 0x80) >> 4) | ((i & 0x20) >> 3) | ((i & 0x8) >> 2) | ((i & 0x2) >> 1);

			tx <<= 3;
			ty <<= 4;

			//if (ty < 384)
			{
				for (y=0; y < 16; y++)
				{
					for (x=0; x < 8; x++)
					{
						//int x2 = x ^ ((x & 2) ? 0 : 1);
						UINT8 pixel = rom[index + decode_y[y] + decode_x[x]];
						UINT16 color = K001006_pal_ram[tp][(pal * 256) + pixel];
						int b = ((color >> 10) & 0x1f) << 3;
						int g = ((color >> 5) & 0x1f) << 3;
						int r = ((color >> 0) & 0x1f) << 3;
						//int sx = (((x & 1) ^ 1) << 2) | ((x & 6) >> 1);
						//int sy = ((y & 1) << 3) | ((y & 0xe) >> 1);
						plot_pixel(bitmap, (tx+x), ty+y, (r << 16) | (g << 8) | b);
					}
				}
			}
			index += 128;
			//pal++;
			//if (pal >= 8) pal = 0;
		}

		sprintf(string, "Texture page %d\nPalette %d", texture_page, texture_palette);
		ui_draw_message_window(string);
	}

	draw_7segment_led(bitmap, 3, 3, led_reg0);
	draw_7segment_led(bitmap, 9, 3, led_reg1);

	cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
}

/******************************************************************/

/* 93C56 EEPROM */
static struct EEPROM_interface eeprom_interface =
{
	8,				/* address bits */
	16,				/* data bits */
	"*110",			/*  read command */
	"*101",			/* write command */
	"*111",			/* erase command */
	"*10000xxxxxx",	/* lock command */
	"*10011xxxxxx",	/* unlock command */
	1,				/* enable_multi_read */
	0				/* reset_delay */
};

static void eeprom_handler(mame_file *file,int read_or_write)
{
	if (read_or_write)
		EEPROM_save(file);
	else
	{
		EEPROM_init(&eeprom_interface);
		if (file)
		{
			EEPROM_load(file);
		}
		else
		{
			// set default eeprom
			UINT8 eepdata[0x200];
			memset(eepdata, 0xff, 0x200);

			if (mame_stricmp(Machine->gamedrv->name, "slrasslt") == 0)
			{
				// magic number
				eepdata[0x4] = 0x96;
				eepdata[0x5] = 0x72;
			}

			EEPROM_set_data(eepdata, 0x200);
		}
	}
}

static UINT8 inputport2 = 0xff;

//UINT32 eeprom_bit = 0;
static READ32_HANDLER( sysreg_r )
{
	UINT32 r = 0;
	if (offset == 0)
	{
		if (!(mem_mask & 0xff000000))
		{
			r |= readinputport(0) << 24;
		}
		if (!(mem_mask & 0x00ff0000))
		{
			r |= readinputport(1) << 16;
		}
		if (!(mem_mask & 0x0000ff00))
		{
			//r |= readinputport(2) << 8;
			inputport2 ^= 0x80;
			r |= inputport2 << 8;
		}
		if (!(mem_mask & 0x000000ff))
		{
			r |= readinputport(3) << 0;
		}
	}
	else if (offset == 1) {
		if (!(mem_mask & 0xff000000) )
		{
			UINT32 eeprom_bit = (EEPROM_read_bit() << 1);
			r |= eeprom_bit << 24;
		}
		return r;
	}
	return r;
}

static WRITE32_HANDLER( sysreg_w )
{
	if (offset == 0) {
		if( !(mem_mask & 0xff000000) )
		{
			led_reg0 = (data >> 24) & 0xff;
		}
		if( !(mem_mask & 0x00ff0000) )
		{
			led_reg1 = (data >> 16) & 0xff;
		}
		if( !(mem_mask & 0x000000ff) )
		{
			EEPROM_write_bit((data & 0x01) ? 1 : 0);
			EEPROM_set_clock_line((data & 0x02) ? ASSERT_LINE : CLEAR_LINE);
			EEPROM_set_cs_line((data & 0x04) ? CLEAR_LINE : ASSERT_LINE);
		}
	}
	if( offset == 1 )
	{
		if (!(mem_mask & 0xff000000))
		{
			if (data & 0x80)	/* CG Board 1 IRQ Ack */
			{
				cpunum_set_input_line(0, INPUT_LINE_IRQ1, CLEAR_LINE);
			}
			if (data & 0x40)	/* CG Board 0 IRQ Ack */
			{
				cpunum_set_input_line(0, INPUT_LINE_IRQ0, CLEAR_LINE);
			}
			//set_cgboard_id((data >> 4) & 0x3);
		}
		return;
	}
}

static UINT8 sndto68k[16], sndtoppc[16];	/* read/write split mapping */

static int soundb1 = 0;
static READ32_HANDLER( ppc_sound_r )
{
	UINT32 reg, w[4], rv = 0;

	reg = offset * 4;

	if (!(mem_mask & 0xff000000))
	{
		w[0] = sndtoppc[reg];
		if (reg == 2) w[0] &= ~3; // supress VOLWR busy flags
		//rv |= w[0]<<24;
		soundb1 ^= 0xff;
		rv |= soundb1 << 24;
	}

	if (!(mem_mask & 0x00ff0000))
	{
		w[1] = sndtoppc[reg+1];
		//rv |= w[1]<<16;
		rv |= 0xff << 16;
	}

	if (!(mem_mask & 0x0000ff00))
	{
		w[2] = sndtoppc[reg+2];
		rv |= w[2]<<8;
	}

	if (!(mem_mask & 0x000000ff))
	{
		w[3] = sndtoppc[reg+3];
		rv |= w[3]<<0;
	}

	printf("ppc_sound_r: %08X, %08X at %08X\n", offset, mem_mask, activecpu_get_pc());

	return(rv);
}

INLINE void write_snd_ppc(int reg, int val)
{
	sndto68k[reg] = val;

	if (reg == 7)
	{
		cpunum_set_input_line(1, 1, HOLD_LINE);
	}
}

static WRITE32_HANDLER( ppc_sound_w )
{
	int reg=0, val=0;

	if (!(mem_mask & 0xff000000))
	{
		reg = offset * 4;
		val = data >> 24;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x00ff0000))
	{
		reg = (offset * 4) + 1;
		val = (data >> 16) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x0000ff00))
	{
		reg = (offset * 4) + 2;
		val = (data >> 8) & 0xff;
		write_snd_ppc(reg, val);
	}

	if (!(mem_mask & 0x000000ff))
	{
		reg = (offset * 4) + 3;
		val = (data >> 0) & 0xff;
		write_snd_ppc(reg, val);
	}
}

static READ32_HANDLER( lanc_r )
{
	return 0x08000000;
}

static WRITE32_HANDLER( lanc_w )
{
}

/******************************************************************/

static ADDRESS_MAP_START( gticlub_map, ADDRESS_SPACE_PROGRAM, 32 )
	AM_RANGE(0x00000000, 0x000fffff) AM_MIRROR(0x80000000) AM_RAM AM_BASE(&work_ram)		/* Work RAM */
	AM_RANGE(0x74000000, 0x740000ff) AM_MIRROR(0x80000000) AM_READWRITE(K001604_reg_r, K001604_reg_w)
	AM_RANGE(0x74010000, 0x7401ffff) AM_MIRROR(0x80000000) AM_READWRITE(paletteram32_r, paletteram32_w) AM_BASE(&paletteram32)
	AM_RANGE(0x74020000, 0x7403ffff) AM_MIRROR(0x80000000) AM_READWRITE(K001604_tile_r, K001604_tile_w)
	AM_RANGE(0x74040000, 0x7407ffff) AM_MIRROR(0x80000000) AM_READWRITE(K001604_char_r, K001604_char_w)
	AM_RANGE(0x78000000, 0x7800ffff) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_shared_r_ppc, cgboard_dsp_shared_w_ppc)
	AM_RANGE(0x78040000, 0x7804000f) AM_MIRROR(0x80000000) AM_READWRITE(K001006_0_r, K001006_0_w)
	AM_RANGE(0x78080000, 0x7808000f) AM_MIRROR(0x80000000) AM_READWRITE(K001006_1_r, K001006_1_w)
	AM_RANGE(0x780c0000, 0x780c0003) AM_MIRROR(0x80000000) AM_READWRITE(cgboard_dsp_comm_r_ppc, cgboard_dsp_comm_w_ppc)
	AM_RANGE(0x7e000000, 0x7e003fff) AM_MIRROR(0x80000000) AM_READWRITE(sysreg_r, sysreg_w)
	AM_RANGE(0x7e008000, 0x7e009fff) AM_MIRROR(0x80000000) AM_READWRITE(lanc_r, lanc_w)
	AM_RANGE(0x7e00a000, 0x7e00bfff) AM_MIRROR(0x80000000) AM_RAM		// LANC RAM
	AM_RANGE(0x7e00c000, 0x7e00c007) AM_MIRROR(0x80000000) AM_WRITE(ppc_sound_w)
	AM_RANGE(0x7e00c000, 0x7e00c007) AM_MIRROR(0x80000000) AM_READ(ppc_sound_r)
	AM_RANGE(0x7e00c008, 0x7e00c00f) AM_MIRROR(0x80000000) AM_READ(ppc_sound_r)
	AM_RANGE(0x7f000000, 0x7f3fffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER2, 0)	/* Data ROM */
	AM_RANGE(0x7f800000, 0x7f9fffff) AM_MIRROR(0x80000000) AM_ROM AM_SHARE(2)
	AM_RANGE(0x7fe00000, 0x7fffffff) AM_MIRROR(0x80000000) AM_ROM AM_REGION(REGION_USER1, 0) AM_SHARE(2)	/* Program ROM */
ADDRESS_MAP_END

/**********************************************************************/

static READ16_HANDLER( sndcomm68k_r )
{
	return sndto68k[offset];
}

static WRITE16_HANDLER( sndcomm68k_w )
{
//  logerror("68K: write %x to %x\n", data, offset);
	sndtoppc[offset] = data;
}

static ADDRESS_MAP_START( sound_memmap, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x000000, 0x07ffff) AM_ROM
	AM_RANGE(0x100000, 0x103fff) AM_RAM		/* Work RAM */
	AM_RANGE(0x200000, 0x20ffff) AM_RAM
	AM_RANGE(0x400000, 0x40000f) AM_WRITE(sndcomm68k_w)
	AM_RANGE(0x400010, 0x40001f) AM_READ(sndcomm68k_r)
	AM_RANGE(0x400020, 0x4fffff) AM_RAM
	AM_RANGE(0x580000, 0x580001) AM_WRITENOP
ADDRESS_MAP_END

/*****************************************************************************/

static UINT32 *sharc_dataram;

static READ32_HANDLER( dsp_dataram_r )
{
	return sharc_dataram[offset] & 0xffff;
}

static WRITE32_HANDLER( dsp_dataram_w )
{
	sharc_dataram[offset] = data;
}

/* Konami K001005 Custom 3D Pixel Renderer chip (KS10071) */
UINT32 K001005_ram[2][0x140000];
int K001005_ram_ptr = 0;
UINT32 K001005_fifo[0x800];
int K001005_fifo_read_ptr = 0;
int K001005_fifo_write_ptr = 0;

UINT32 K001005_3d_fifo[0x10000];
int K001005_3d_fifo_ptr = 0;

READ32_HANDLER( K001005_r )
{
	switch(offset)
	{
		case 0x000:			// FIFO read, high 16 bits
		{
			UINT16 value = K001005_fifo[K001005_fifo_read_ptr] >> 16;
		//  printf("FIFO_r0: %08X\n", K001005_fifo_ptr);
			return value;
		}

		case 0x001:			// FIFO read, low 16 bits
		{
			UINT16 value = K001005_fifo[K001005_fifo_read_ptr] & 0xffff;
		//  printf("FIFO_r1: %08X\n", K001005_fifo_ptr);

			if (K001005_status != 1 && K001005_status != 2)
			{
				if (K001005_fifo_read_ptr < 0x3ff)
				{
					cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
				}
				else
				{
					cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
				}
			}
			else
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
			}

			K001005_fifo_read_ptr++;
			K001005_fifo_read_ptr &= 0x7ff;
			return value;
		}

		case 0x11b:			// status ?
			return 0x8002;

		case 0x11c:			// slave status ?
			return 0x8000;

		case 0x11f:
			if (K001005_ram_ptr >= 0x400000)
			{
				return K001005_ram[1][(K001005_ram_ptr++) & 0x3fffff];
			}
			else
			{
				return K001005_ram[0][(K001005_ram_ptr++) & 0x3fffff];
			}

		default:
			printf("K001005_r: %08X, %08X\n", offset, mem_mask);
			break;
	}
	return 0;
}

WRITE32_HANDLER( K001005_w )
{
	switch(offset)
	{
		case 0x000:			// FIFO write
		{
			if (K001005_status != 1 && K001005_status != 2)
			{
				if (K001005_fifo_write_ptr < 0x400)
				{
					cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
				}
				else
				{
					cpunum_set_input_line(2, SHARC_INPUT_FLAG1, CLEAR_LINE);
				}
			}
			else
			{
				cpunum_set_input_line(2, SHARC_INPUT_FLAG1, ASSERT_LINE);
			}

	//      printf("K001005 FIFO write: %08X at %08X\n", data, activecpu_get_pc());
			K001005_fifo[K001005_fifo_write_ptr] = data;
			K001005_fifo_write_ptr++;
			K001005_fifo_write_ptr &= 0x7ff;

			K001005_3d_fifo[K001005_3d_fifo_ptr++] = data;

			// !!! HACK to get past the FIFO B test (GTI Club & Thunder Hurricane) !!!
			if (activecpu_get_pc() == 0x201ee)
			{
				// This is used to make the SHARC timeout
				cpu_spinuntil_trigger(10000);
			}
			// !!! HACK to get past the FIFO B test (Winding Heat & Midnight Run) !!!
			if (activecpu_get_pc() == 0x201e6)
			{
				// This is used to make the SHARC timeout
				cpu_spinuntil_trigger(10000);
			}

			break;
		}

		case 0x11a:
			K001005_status = data;
			K001005_fifo_write_ptr = 0;
			K001005_fifo_read_ptr = 0;

			K001005_draw();
			K001005_3d_fifo_ptr = 0;

			break;

		case 0x11d:
			K001005_fifo_write_ptr = 0;
			K001005_fifo_read_ptr = 0;
			break;

		case 0x11e:
			K001005_ram_ptr = data;
			break;

		case 0x11f:
			if (K001005_ram_ptr >= 0x400000)
			{
				K001005_ram[1][(K001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			}
			else
			{
				K001005_ram[0][(K001005_ram_ptr++) & 0x3fffff] = data & 0xffff;
			}
			break;

		default:
			printf("K001005_w: %08X, %08X, %08X\n", data, offset, mem_mask);
			break;
	}

}

static void draw_triangle(mame_bitmap *bitmap, VERTEX v1, VERTEX v2, VERTEX v3, UINT32 color)
{
	int x, y;
	struct poly_vertex vert[3];
	const struct poly_scanline_data *scans;

	rectangle cliprect;
	cliprect.min_x = 0;
	cliprect.min_y = 0;
	cliprect.max_x = Machine->drv->screen_width-1;
	cliprect.max_y = Machine->drv->screen_height-1;

	vert[0].x = v1.x;	vert[0].y = v1.y;
	vert[1].x = v2.x;	vert[1].y = v2.y;
	vert[2].x = v3.x;	vert[2].y = v3.y;

	scans = setup_triangle_1(&vert[0], &vert[1], &vert[2], &cliprect);

	if(scans)
	{
		for(y = scans->sy; y <= scans->ey; y++) {
			int x1, x2;
			const struct poly_scanline *scan = &scans->scanline[y - scans->sy];
			UINT32 *p = (UINT32*)bitmap->line[y];

			x1 = scan->sx;
			x2 = scan->ex;
			if(x1 < cliprect.min_x)
				x1 = cliprect.min_x;
			if(x2 > cliprect.max_x)
				x2 = cliprect.max_x;

			if(x1 < cliprect.max_x && x2 > cliprect.min_x) {
				for(x = x1; x <= x2; x++) {
					p[x] = color;
				}
			}
		}
	}
}

static VERTEX prev_v[4];

void K001005_draw(void)
{
	int i, j;

//  printf("K001005_fifo_ptr = %08X\n", K001005_3d_fifo_ptr);

	for (i=0; i < K001005_3d_fifo_ptr; i++)
	{
		if (K001005_3d_fifo[i] == 0x80000003)
		{
			VERTEX v[4];
			int x[4], y[4];
			int r = K001005_3d_fifo[i+6] & 0xff;
			int g = (K001005_3d_fifo[i+6] >> 8) & 0xff;
			int b = (K001005_3d_fifo[i+6] >> 16) & 0xff;
			UINT32 color = (r << 16) | (g << 8) | (b);

			for (j=0; j < 4; j++)
			{
				x[j] = ((K001005_3d_fifo[i+1+j] >> 0) & 0x1fff);
				y[j] = ((K001005_3d_fifo[i+1+j] >> 16) & 0x1fff);

				if (x[j] & 0x1000)
					x[j] |= 0xffffe000;
				if (y[j] & 0x1000)
					y[j] |= 0xffffe000;

				y[j] = -y[j];

				x[j] += 4096;
				y[j] += 3072;

				x[j] /= 16;
				y[j] /= 16;

				v[j].x = x[j];
				v[j].y = y[j];
			}

			draw_triangle(K001005_bitmap, v[0], v[1], v[2], color);
			draw_triangle(K001005_bitmap, v[0], v[2], v[3], color);

			//plot_box(K001005_bitmap, x[0], y[1], x[2]-x[0], y[0]-y[1], color);
		}
		else if (K001005_3d_fifo[i] == 0x800000ae || K001005_3d_fifo[i] == 0x8000008e ||
				 K001005_3d_fifo[i] == 0x80000096 || K001005_3d_fifo[i] == 0x800000b6)
		{
			VERTEX v[4];
			int x[4], y[4];
			int num_verts = 0;

	//      UINT32 texbase = (K001005_3d_fifo[i+1] & 0x7fffff);
	//      int pal = (K001005_3d_fifo[i+1] >> 28) & 0x7;
	//      int width = (((K001005_3d_fifo[i+1] >> 20) & 0x7) + 1) * 8;
	//      int height = (((K001005_3d_fifo[i+1] >> 23) & 0x7) + 1) * 8;

			for (j=0; j < 4; j++)
			{
				x[j] = ((K001005_3d_fifo[i+2+(j*3)] >> 0) & 0x1fff);
				y[j] = ((K001005_3d_fifo[i+2+(j*3)] >> 16) & 0x1fff);

				if (x[j] & 0x1000)
					x[j] |= 0xffffe000;
				if (y[j] & 0x1000)
					y[j] |= 0xffffe000;

				y[j] = -y[j];

				x[j] += 4096;
				y[j] += 3072;

				x[j] /= 16;
				y[j] /= 16;

				v[j].x = x[j];
				v[j].y = y[j];

				num_verts++;

				if (K001005_3d_fifo[i+2+(j*3)] & 0x8000)
				{
					break;
				}
			}

			for (j=0; j < num_verts-1; j++)
			{
				UINT16 u2 = (K001005_3d_fifo[i+4+(j*3)] >> 16) & 0xfff;
				UINT16 v2 = (K001005_3d_fifo[i+4+(j*3)] >> 0) & 0xfff;

				v[j].u = u2;
				v[j].v = v2;
			}

			{
				UINT16 u2 = (K001005_3d_fifo[i+4+((num_verts-1)*3)] >> 16) & 0xfff;
				UINT16 v2 = (K001005_3d_fifo[i+4+((num_verts-1)*3)] >> 0) & 0xfff;
				v[num_verts-1].u = u2;
				v[num_verts-1].v = v2;
			}


			if (num_verts < 3)
			{
				//draw_triangle_tex(K001005_bitmap, prev_v[2], prev_v[3], v[0], texbase, pal, width, height);
				//draw_triangle_tex(K001005_bitmap, prev_v[2], v[0], v[1], texbase, pal, width, height);
				draw_triangle(K001005_bitmap, prev_v[2], prev_v[3], v[0], 0xffff);
				draw_triangle(K001005_bitmap, prev_v[2], v[0], v[1], 0xffff);
			}
			else
			{
				//draw_triangle_tex(K001005_bitmap, v[0], v[1], v[2], texbase, pal, width, height);
				draw_triangle(K001005_bitmap, v[0], v[1], v[2], 0xffff);
				if (num_verts > 3)
				{
					//draw_triangle_tex(K001005_bitmap, v[2], v[3], v[0], texbase, pal, width, height);
					draw_triangle(K001005_bitmap, v[2], v[3], v[0], 0xffff);
				}
			}

			memcpy(prev_v, v, sizeof(VERTEX) * 4);
		}
		else if (K001005_3d_fifo[i] == 0x80000006/* || K001005_3d_fifo[i] == 0x80000026*/)
		{
			VERTEX v[4];
			int x[4], y[4];
			int r,g,b;
			UINT32 color;

			int num_verts = 0;

			for (j=0; j < 4; j++)
			{
				x[j] = ((K001005_3d_fifo[i+1+(j*2)] >> 0) & 0x1fff);
				y[j] = ((K001005_3d_fifo[i+1+(j*2)] >> 16) & 0x1fff);

				if (x[j] & 0x1000)
					x[j] |= 0xffffe000;
				if (y[j] & 0x1000)
					y[j] |= 0xffffe000;

				y[j] = -y[j];

				x[j] += 4096;
				y[j] += 3072;

				x[j] /= 16;
				y[j] /= 16;

				v[j].x = x[j];
				v[j].y = y[j];
				num_verts++;

				if (K001005_3d_fifo[i+1+(j*2)] & 0x8000)
				{
					break;
				}
			}

			if (num_verts > 3)
			{
				r = K001005_3d_fifo[i+9] & 0xff;
				g = (K001005_3d_fifo[i+9] >> 8) & 0xff;
				b = (K001005_3d_fifo[i+9] >> 16) & 0xff;
				color = (r << 16) | (g << 8) | (b);
			}
			else
			{
				r = K001005_3d_fifo[i+7] & 0xff;
				g = (K001005_3d_fifo[i+7] >> 8) & 0xff;
				b = (K001005_3d_fifo[i+7] >> 16) & 0xff;
				color = (r << 16) | (g << 8) | (b);
			}

			draw_triangle(K001005_bitmap, v[0], v[1], v[2], color);
			if (num_verts > 3)
			{
				draw_triangle(K001005_bitmap, v[2], v[3], v[0], color);
			}
		}
		else if (K001005_3d_fifo[i] == 0x80000026)
		{
			VERTEX v[6];
			int x[4], y[4];
			int r,g,b;
			UINT32 color;

			int num_verts = 0;

			for (j=0; j < 4; j++)
			{
				x[j] = ((K001005_3d_fifo[i+1+(j*2)] >> 0) & 0x1fff);
				y[j] = ((K001005_3d_fifo[i+1+(j*2)] >> 16) & 0x1fff);

				if (x[j] & 0x1000)
					x[j] |= 0xffffe000;
				if (y[j] & 0x1000)
					y[j] |= 0xffffe000;

				y[j] = -y[j];

				x[j] += 4096;
				y[j] += 3072;

				x[j] /= 16;
				y[j] /= 16;

				v[j].x = x[j];
				v[j].y = y[j];
				num_verts++;

				if (K001005_3d_fifo[i+1+(j*2)] & 0x8000)
				{
					break;
				}
			}

			if (num_verts > 3)
			{
				r = K001005_3d_fifo[i+9] & 0xff;
				g = (K001005_3d_fifo[i+9] >> 8) & 0xff;
				b = (K001005_3d_fifo[i+9] >> 16) & 0xff;
				color = (r << 16) | (g << 8) | (b);
			}
			else
			{
				r = K001005_3d_fifo[i+7] & 0xff;
				g = (K001005_3d_fifo[i+7] >> 8) & 0xff;
				b = (K001005_3d_fifo[i+7] >> 16) & 0xff;
				color = (r << 16) | (g << 8) | (b);
			}

			draw_triangle(K001005_bitmap, v[0], v[1], v[2], color);
			if (num_verts > 3)
			{
				draw_triangle(K001005_bitmap, v[2], v[3], v[0], color);
			}

			if ((K001005_3d_fifo[i+2+(num_verts*2)] & 0xffffff00) != 0x80000000)
			{
				for (j=0; j < 2; j++)
				{
					x[j] = ((K001005_3d_fifo[i+2+(num_verts*2)+(j*2)] >> 0) & 0x1fff);
					y[j] = ((K001005_3d_fifo[i+2+(num_verts*2)+(j*2)] >> 16) & 0x1fff);

					if (x[j] & 0x1000)
						x[j] |= 0xffffe000;
					if (y[j] & 0x1000)
						y[j] |= 0xffffe000;

					y[j] = -y[j];

					x[j] += 4096;
					y[j] += 3072;

					x[j] /= 16;
					y[j] /= 16;

					v[j+4].x = x[j];
					v[j+4].y = y[j];

					if (K001005_3d_fifo[i+2+(num_verts*2)+(j*2)] & 0x8000)
					{
						break;
					}
				}

				r = K001005_3d_fifo[i+2+(num_verts*2)+4] & 0xff;
				g = (K001005_3d_fifo[i+2+(num_verts*2)+4] >> 8) & 0xff;
				b = (K001005_3d_fifo[i+2+(num_verts*2)+4] >> 16) & 0xff;
				color = (r << 16) | (g << 8) | (b);

				draw_triangle(K001005_bitmap, v[2], v[3], v[4], color);
				draw_triangle(K001005_bitmap, v[2], v[4], v[5], color);
			}
		}
		else if (K001005_3d_fifo[i] == 0x80000000)
		{

		}
		else if ((K001005_3d_fifo[i] & 0xffffff00) == 0x80000000)
		{
			/*printf("Unknown polygon type %08X:\n", K001005_3d_fifo[i]);
            for (j=0; j < 0x20; j++)
            {
                printf("  %02X: %08X\n", j, K001005_3d_fifo[i+1+j]);
            }
            printf("\n");
            */
		}
	}
}


static ADDRESS_MAP_START( sharc_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x400000, 0x41ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x500000, 0x5fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x600000, 0x6fffff) AM_READWRITE(K001005_r, K001005_w)
	AM_RANGE(0x700000, 0x7000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
ADDRESS_MAP_END

static ADDRESS_MAP_START( sharc_hangplt_map, ADDRESS_SPACE_DATA, 32 )
	AM_RANGE(0x0400000, 0x041ffff) AM_READWRITE(cgboard_dsp_shared_r_sharc, cgboard_dsp_shared_w_sharc)
	AM_RANGE(0x0500000, 0x05fffff) AM_READWRITE(dsp_dataram_r, dsp_dataram_w)
	AM_RANGE(0x1400000, 0x14fffff) AM_RAM
	//AM_RANGE(0x2400000, 0x27fffff) AM_READWRITE(voodoo_0_r, voodoo_0_w)
	AM_RANGE(0x3400000, 0x34000ff) AM_READWRITE(cgboard_dsp_comm_r_sharc, cgboard_dsp_comm_w_sharc)
	//AM_RANGE(0x3500000, 0x35000ff) AM_READWRITE(pci_3dfx_r, pci_3dfx_w)
	AM_RANGE(0x3500000, 0x3507fff) AM_RAM
	AM_RANGE(0x3600000, 0x37fffff) AM_ROMBANK(5)
ADDRESS_MAP_END

/*****************************************************************************/

static NVRAM_HANDLER(gticlub)
{
	eeprom_handler(file, read_or_write);
}


INPUT_PORTS_START( gticlub )
	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(1)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1)

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_8WAY PORT_PLAYER(2)
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2)
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2)

	PORT_START
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Test Button") PORT_CODE(KEYCODE_7)
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_SERVICE ) PORT_NAME("Service Button") PORT_CODE(KEYCODE_8)
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_DIPNAME( 0x08, 0x08, "DIP3" )
	PORT_DIPSETTING( 0x08, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "DIP2" )
	PORT_DIPSETTING( 0x04, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "DIP1" )
	PORT_DIPSETTING( 0x02, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "DIP0" )
	PORT_DIPSETTING( 0x01, DEF_STR( Off ) )
	PORT_DIPSETTING( 0x00, DEF_STR( On ) )
INPUT_PORTS_END

/* PowerPC interrupts

    IRQ0:  Vblank
    IRQ2:  LANC
    DMA0

*/
static INTERRUPT_GEN( gticlub_vblank )
{
	// !!! GTI Club LANC Hack !!!
	if (work_ram[0x81090/4] == 0xff000000)
	{
		work_ram[0x81094/4] = 0x01000000;
	}

	cpunum_set_input_line(0, INPUT_LINE_IRQ0, ASSERT_LINE);
}


static ppc_config gticlub_ppc_cfg =
{
	PPC_MODEL_403GA
};

static sharc_config sharc_cfg =
{
	BOOT_MODE_EPROM
};

static MACHINE_RESET( gticlub )
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_DRIVER_START( gticlub )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(gticlub_map, 0)
	MDRV_CPU_VBLANK_INT(gticlub_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)
	MDRV_MACHINE_RESET(gticlub)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(gticlub)
	MDRV_VIDEO_UPDATE(gticlub)
MACHINE_DRIVER_END

static MACHINE_RESET( hangplt )
{
	cpunum_set_input_line(2, INPUT_LINE_RESET, ASSERT_LINE);
	cpunum_set_input_line(3, INPUT_LINE_RESET, ASSERT_LINE);
}

static MACHINE_DRIVER_START( hangplt )

	/* basic machine hardware */
	MDRV_CPU_ADD(PPC403, 64000000/2)	/* PowerPC 403GA 32MHz */
	MDRV_CPU_CONFIG(gticlub_ppc_cfg)
	MDRV_CPU_PROGRAM_MAP(gticlub_map, 0)
	MDRV_CPU_VBLANK_INT(gticlub_vblank, 1)

	MDRV_CPU_ADD(M68000, 64000000/4)	/* 16MHz */
	MDRV_CPU_PROGRAM_MAP(sound_memmap, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_hangplt_map, 0)

	MDRV_CPU_ADD(ADSP21062, 36000000)
	MDRV_CPU_CONFIG(sharc_cfg)
	MDRV_CPU_DATA_MAP(sharc_hangplt_map, 0)

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)

	MDRV_NVRAM_HANDLER(gticlub)
	MDRV_MACHINE_RESET(hangplt)

 	/* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER | VIDEO_NEEDS_6BITS_PER_GUN | VIDEO_RGB_DIRECT)
	MDRV_SCREEN_SIZE(64*8, 48*8)
	MDRV_VISIBLE_AREA(0*8, 64*8-1, 0*8, 48*8-1)
	MDRV_PALETTE_LENGTH(65536)

	MDRV_VIDEO_START(gticlub)
	MDRV_VIDEO_UPDATE(gticlub)
MACHINE_DRIVER_END

/*************************************************************************/

#define ROM_LOAD64_WORD(name,offset,length,hash)      ROMX_LOAD(name, offset, length, hash, ROM_GROUPWORD | ROM_SKIP(6))

ROM_START(gticlub)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
		ROM_LOAD32_BYTE("688aaa01.21u", 0x000003, 0x80000, CRC(06a56474) SHA1(3a457b885a35e3ee030fd51d847bcf75fce46208))
		ROM_LOAD32_BYTE("688aaa02.19u", 0x000002, 0x80000, CRC(3c1e714a) SHA1(557f8542b855b2b35f242c8db7396017aca6dbd8))
		ROM_LOAD32_BYTE("688aaa03.21r", 0x000001, 0x80000, CRC(e060580b) SHA1(50242f3f3b949cc03082e4e75d9dcc89e17f0a75))
		ROM_LOAD32_BYTE("688aaa04.19r", 0x000000, 0x80000, CRC(928c23cd) SHA1(cce54398e1e5b98bfb717839cc422f1f60502788))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
		ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d))
		ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program */
		ROM_LOAD16_WORD_SWAP( "688a07.13k",   0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
		ROM_LOAD( "688a09.9s",    0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
		ROM_LOAD( "688a10.7s",    0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
		ROM_LOAD( "688a11.5s",    0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
		ROM_LOAD( "688a12.2s",    0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

    ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
		ROM_LOAD64_WORD( "688a13.18d",   0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
		ROM_LOAD64_WORD( "688a14.13d",   0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
		ROM_LOAD64_WORD( "688a15.9d",    0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
		ROM_LOAD64_WORD( "688a16.4d",    0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )
ROM_END

ROM_START(gticlubj)
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
		ROM_LOAD32_BYTE("688jaa01.bin", 0x000003, 0x80000, CRC(1492059c) SHA1(176dbd87f23f4cd8e1397e67da501738e20e5a57))
		ROM_LOAD32_BYTE("688jaa02.bin", 0x000002, 0x80000, CRC(7896dd69) SHA1(a3ab7b872132a5e66238e414f4b497cf7beb8b1c))
		ROM_LOAD32_BYTE("688jaa03.bin", 0x000001, 0x80000, CRC(94e2be50) SHA1(f206ac201903f3aae29196ab6fccdef104859346))
		ROM_LOAD32_BYTE("688jaa04.bin", 0x000000, 0x80000, CRC(ff539bb6) SHA1(1a225eca4377d82a2b6cb99c1d16580b9ccf2f08))

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
		ROM_LOAD32_WORD_SWAP("688a05.14u", 0x000000, 0x200000, CRC(7caa3f80) SHA1(28409dc17c4e010173396fdc069a409fbea0d58d))
		ROM_LOAD32_WORD_SWAP("688a06.12u", 0x000002, 0x200000, CRC(83e7ce0a) SHA1(afe185f6ed700baaf4c8affddc29f8afdfec4423))

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program */
        ROM_LOAD16_WORD_SWAP( "688a07.13k",   0x000000, 0x040000, CRC(f0805f06) SHA1(4b87e02b89e7ea812454498603767668e4619025) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
        ROM_LOAD( "688a09.9s",    0x000000, 0x200000, CRC(fb582963) SHA1(ce8fe6a4d7ac7d7f4b6591f9150b1d351e636354) )
        ROM_LOAD( "688a10.7s",    0x200000, 0x200000, CRC(b3ddc5f1) SHA1(a3f76c86e85eb17f20efb037c1ad64e9cb8566c8) )
        ROM_LOAD( "688a11.5s",    0x400000, 0x200000, CRC(fc706183) SHA1(c8ce6de0588be1023ef48577bc88a4e5effdcd25) )
        ROM_LOAD( "688a12.2s",    0x600000, 0x200000, CRC(510c70e3) SHA1(5af77bc98772ab7961308c3af0a80cb1bca690e3) )

    ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
        ROM_LOAD64_WORD( "688a13.18d",   0x000000, 0x200000, CRC(c8f04f91) SHA1(9da8cc3a94dbf0a1fce87c2bc9249df712ae0b0d) )
        ROM_LOAD64_WORD( "688a14.13d",   0x000002, 0x200000, CRC(b9932735) SHA1(2492244d2acb350974202a6718bc7121325d2121) )
        ROM_LOAD64_WORD( "688a15.9d",    0x000004, 0x200000, CRC(8aadee51) SHA1(be9020a47583da9d4ff586d227836dc5b7dc31f0) )
        ROM_LOAD64_WORD( "688a16.4d",    0x000006, 0x200000, CRC(7f4e1893) SHA1(585be7b31ab7a48300c22b00443b00d631f4c49d) )
ROM_END

ROM_START( thunderh )
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
        ROM_LOAD32_BYTE( "680uaa01.21u", 0x000003, 0x080000, CRC(f2bb2ba1) SHA1(311e88d63179486014376c4af4ff0ef28673ee5a) )
        ROM_LOAD32_BYTE( "680uaa02.19u", 0x000002, 0x080000, CRC(52f617b5) SHA1(fda3133d3a7e04eb4432c69becdcf1872b3660d9) )
        ROM_LOAD32_BYTE( "680uaa03.21r", 0x000001, 0x080000, CRC(086a0574) SHA1(32fb93dbb93d2fe6af743ea4310b50a6cd03647d) )
        ROM_LOAD32_BYTE( "680uaa04.19r", 0x000000, 0x080000, CRC(85e1f8e3) SHA1(9172c54b6663f1bf390795068271198083a6860d) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
        ROM_LOAD32_WORD_SWAP( "680a05.14u",   0x000000, 0x200000, CRC(0c9f334d) SHA1(99ac622a04a7140244d81031df69a796b6fd2657) )
        ROM_LOAD32_WORD_SWAP( "680a06.12u",   0x000002, 0x200000, CRC(83074217) SHA1(bbf782ac125cd98d9147ef4e0373bf61f74726f7) )

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program */
        ROM_LOAD16_WORD_SWAP( "680a07.13k",   0x000000, 0x080000, CRC(12247a3e) SHA1(846cd9423efd3c9b17fce08393c6c83307d72f92) )

	ROM_REGION(0x20000, REGION_CPU3, 0)		/* 68k program for outboard sound? network? board */
        ROM_LOAD16_WORD_SWAP( "680c22.20k",   0x000000, 0x020000, CRC(d93c0ee2) SHA1(4b58418cbb01b51e12d6e7c86b2c81cd35d86248) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
        ROM_LOAD( "680a09.9s",    0x000000, 0x200000, CRC(71c2b049) SHA1(ce360172c8774b31edf16a80104c35b1caf26cd9) )
        ROM_LOAD( "680a10.7s",    0x200000, 0x200000, CRC(19882bf3) SHA1(7287da58853c84cbadbfb42bed37f2b0032c4b4d) )
        ROM_LOAD( "680a11.5s",    0x400000, 0x200000, CRC(0c74fe3f) SHA1(2e69f8d37552a74bbda65b134f747b4380ed33b0) )
        ROM_LOAD( "680a12.2s",    0x600000, 0x200000, CRC(b052919d) SHA1(a61c8eaf378ab7d780478db61217302d1b9f8f73) )

    ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
        ROM_LOAD64_WORD( "680a13.18d",   0x000000, 0x200000, CRC(233f9074) SHA1(78ce42c35407d61df37cc0d16cdee84f8de968fa) )
        ROM_LOAD64_WORD( "680a14.13d",   0x000002, 0x200000, CRC(9ae15033) SHA1(12e291114629632b81f53811a6c8666aff4e92f3) )
        ROM_LOAD64_WORD( "680a15.9d",    0x000004, 0x200000, CRC(dc47c86f) SHA1(71af9b21f1ecc063135f501b1561869ee910c236) )
        ROM_LOAD64_WORD( "680a16.4d",    0x000006, 0x200000, CRC(ea388143) SHA1(3de5314a009d702186d5e285c8edefdd48139eab) )
ROM_END

ROM_START( slrasslt )
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
        ROM_LOAD32_BYTE( "792uaa01.21u", 0x000003, 0x080000, CRC(c73bf7fb) SHA1(ffe0fea155473827929339a9261a158287ce30a8) )
        ROM_LOAD32_BYTE( "792uaa02.19u", 0x000002, 0x080000, CRC(a940bb9b) SHA1(65a60157697a21cc2485c02c689c9addb3ac91f1) )
        ROM_LOAD32_BYTE( "792uaa03.21r", 0x000001, 0x080000, CRC(363e8411) SHA1(b9c70033d8e3de4b339b61a66172bfecb7c2b3ab) )
        ROM_LOAD32_BYTE( "792uaa04.19r", 0x000000, 0x080000, CRC(7910d99c) SHA1(e2114d369060528998b58331d590c086d306f541) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
        ROM_LOAD32_WORD_SWAP( "792a05.14u",   0x000000, 0x200000, CRC(9a27edfc) SHA1(c028b6440eb1b0c814c4db45918e580662ac2d9a) )
        ROM_LOAD32_WORD_SWAP( "792a06.12u",   0x000002, 0x200000, CRC(c272f171) SHA1(df492287eadc5e8668fe46cfa3ed3ca77c57feca) )

	ROM_REGION(0x80000, REGION_CPU2, 0)		/* 68k program */
        ROM_LOAD16_WORD_SWAP( "792a07.10k",   0x000000, 0x080000, CRC(89a65ad1) SHA1(d814ef0b560c8e68da57ad5c6096e4fc05e9913e) )

	ROM_REGION(0x800000, REGION_SOUND1, 0)	/* sound roms */
        ROM_LOAD( "792a09.9s",    0x000000, 0x200000, CRC(7d7ea427) SHA1(a9a311a7c17223cc87140fe2890e20a321464831) )
        ROM_LOAD( "792a10.7s",    0x200000, 0x200000, CRC(e585e5d9) SHA1(ec44ad324a66eeea4c45933dda5a8a9a4398879d) )
        ROM_LOAD( "792a11.5s",    0x400000, 0x200000, CRC(c9c3a04c) SHA1(f834659f67712c9fcd93b7407669d7f35517b790) )
        ROM_LOAD( "792a12.2s",    0x600000, 0x200000, CRC(da8fcdd5) SHA1(daa7b3a086ada69e93c3d7cd9130befc79e422dc) )

    ROM_REGION(0x800000, REGION_GFX1, 0)	/* texture roms */
        ROM_LOAD64_WORD( "792a13.18d",   0x000000, 0x200000, CRC(16d6a134) SHA1(3f53f3c6759d7c5f40aa25a598df899fbac35a60) )
        ROM_LOAD64_WORD( "792a14.13d",   0x000002, 0x200000, CRC(cf57e830) SHA1(607b4dec3b8180a63e29d9dab1ca28d7226dda1e) )
        ROM_LOAD64_WORD( "792a15.9d",    0x000004, 0x200000, CRC(1c5531cb) SHA1(1b514f181c92e16d07bfe4719604f1e4caf15377) )
        ROM_LOAD64_WORD( "792a16.4d",    0x000006, 0x200000, CRC(df89e392) SHA1(af37c5460d43bf8d8a1ab4213c4528083a7363c2) )
ROM_END

ROM_START( hangplt )
	ROM_REGION(0x200000, REGION_USER1, 0)	/* PowerPC program roms */
        ROM_LOAD32_BYTE( "685jab01.21u", 0x000003, 0x080000, CRC(f98a3e82) SHA1(94ebaa172b0e98c5cd08efaea5f56e707e5032b4) )
        ROM_LOAD32_BYTE( "685jab02.19u", 0x000002, 0x080000, CRC(20730cdc) SHA1(71b2cf7077ab7db875f9030e21afd05905f57ce5) )
        ROM_LOAD32_BYTE( "685jab03.21r", 0x000001, 0x080000, CRC(77fa2248) SHA1(a662b84945b3d268fed15952cc793d821233735e) )
        ROM_LOAD32_BYTE( "685jab04.19r", 0x000000, 0x080000, CRC(ab6773df) SHA1(91d3f849a1cc5fa4b2fbd876d53402a548198c41) )

	ROM_REGION32_BE(0x400000, REGION_USER2, 0)	/* data roms */
        ROM_LOAD32_WORD_SWAP( "685a05.14u",   0x000000, 0x200000, CRC(ba1c8f40) SHA1(ce4ed641c1d6d44447eaaada16f305f1d7fb9ee2) )
        ROM_LOAD32_WORD_SWAP( "685a06.12u",   0x000002, 0x200000, CRC(2429935c) SHA1(4da9e169adcac81ea1bc135d727c2bd13ad372fa) )

	ROM_REGION(0x80000, REGION_CPU2, 0)	/* 68k program */
        ROM_LOAD16_WORD_SWAP( "685a07.13k",   0x000000, 0x080000, CRC(5b72fd80) SHA1(a150837fa0d66dc0c3832495a4c8ce4f9b92cd98) )

	ROM_REGION(0x1000000, REGION_USER3, 0)	/* other roms */
        ROM_LOAD( "685a09.9s",    0x000000, 0x200000, CRC(653821cf) SHA1(625abb7769a52c9ac61cfddaa084b9c9539f3b15) )
        ROM_LOAD( "685a10.7s",    0x200000, 0x200000, CRC(71eb06e5) SHA1(3c5953e87df63fb7680d7a04267ff2208f49838f) )
        ROM_LOAD( "685a13.4w",    0x800000, 0x400000, CRC(06329af4) SHA1(76cad9db604751ce48bb67bfd29e57bac0ee9a16) )
        ROM_LOAD( "685a14.12w",   0xc00000, 0x400000, CRC(87437739) SHA1(0d45637af40938a54d5efd29c125b0fafd55f9a4) )
ROM_END

static DRIVER_INIT(gticlub)
{
	init_konami_cgboard(0, CGBOARD_TYPE_GTICLUB);
	sharc_dataram = auto_malloc(0x100000);
}

static DRIVER_INIT(hangplt)
{
	init_konami_cgboard(0, CGBOARD_TYPE_GTICLUB);
	init_konami_cgboard(1, CGBOARD_TYPE_GTICLUB);
	set_cgboard_texture_bank(5);
	sharc_dataram = auto_malloc(0x100000);
}

/*************************************************************************/

GAME( 1996, gticlub,	0,		 gticlub, gticlub, gticlub,	ROT0,	"Konami",	"GTI Club (ver AAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1996, gticlubj,	gticlub, gticlub, gticlub, gticlub,	ROT0,	"Konami",	"GTI Club (ver JAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1996, thunderh,	0,		 gticlub, gticlub, gticlub,	ROT0,	"Konami",	"Thunder Hurricane (ver UAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1997, slrasslt,	0,		 gticlub, gticlub, gticlub,	ROT0,	"Konami",	"Solar Assault DR2 (ver UAA)", GAME_NOT_WORKING|GAME_NO_SOUND )
GAME( 1997, hangplt,	0,		 hangplt, gticlub, hangplt, ROT0,	"Konami",	"Hang Pilot", GAME_NOT_WORKING|GAME_NO_SOUND )
