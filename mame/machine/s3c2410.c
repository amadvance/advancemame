// license:BSD-3-Clause
// copyright-holders:Tim Schuerewegen
/* Samsung S3C2410 peripherals. */
#include "driver.h"
#include "machine/s3c2410.h"
#include "sound/dac.h"
#include "cpu/arm920t/arm920t.h"
#include "machine/generic.h"
static int lcd_height = 262;
static rgb_t soc_palette_color(int pen)
{
	UINT8 r, g, b;
	palette_get_color(pen, &r, &g, &b);
	return MAKE_RGB(r, g, b);
}

#define CLOCK_MULTIPLIER 1

#define BIT(x,n) (((x)>>(n))&1)
#define BITS(x,m,n) (((x)>>(n))&(((UINT32)1<<((m)-(n)+1))-1))
#define CLR_BITS(x,m,n) ((x) & ~((((UINT32)1 << ((m) - (n) + 1)) - 1) << n))

#define S3C24XX_TPAL_GET_TPALEN(x)  BIT(x,24)
#define S3C24XX_TPAL_GET_TPALVAL(x) BITS(x,23,0)

#define S3C24XX_DCON_GET_TC(x)      BITS(x,19,0)
#define S3C24XX_DCON_GET_DSZ(x)     BITS(x,21,20)
#define S3C24XX_DCON_GET_RELOAD(x)  BIT(x,22)
#define S3C24XX_DCON_GET_SWHWSEL(x) BIT(x,23)

#define S3C24XX_DSTAT_GET_CURR_TC(x)   BITS(x,19,0)
#define S3C24XX_DSTAT_SET_CURR_TC(x,m) (CLR_BITS(x,19,0) | m)

#define S3C24XX_DMASKTRIG_GET_ON_OFF(x) BIT(x,1)

#define S3C24XX_DCON_GET_HWSRCSEL(x) BITS(x,26,24)
#define S3C24XX_DCON_GET_SERVMODE(x) BIT(x,27)
#define S3C24XX_DCON_GET_TSZ(x)      BIT(x,28)
#define S3C24XX_DCON_GET_INT(x)      BIT(x,29)

#define S3C24XX_DISRC_GET_SADDR(x) BITS(x,30,0)

#define S3C24XX_DIDST_GET_DADDR(x) BITS(x,30,0)

#define S3C24XX_DCSRC_GET_CURR_SRC(x)   BITS(x,30,0)
#define S3C24XX_DCSRC_SET_CURR_SRC(x,m) (CLR_BITS(x,30,0) | m)

#define S3C24XX_DCDST_GET_CURR_DST(x)   BITS(x,30,0)
#define S3C24XX_DCDST_SET_CURR_DST(x,m) (CLR_BITS(x,30,0) | m)

typedef s3c2410_interface s3c24xx_interface;

static UINT32 s3c24xx_get_fclk( int device);
static UINT32 s3c24xx_get_hclk( int device);
static UINT32 s3c24xx_get_pclk( int device);

static void s3c24xx_dma_request_iis( int device);
static void s3c24xx_dma_request_pwm( int device);

static s3c24xx_t soc;
static s3c24xx_t *get_token(int device) { return &soc; }

static void s3c24xx_reset( int device)
{
	mame_schedule_soft_reset();
}

static rgb_t s3c24xx_get_color_tft_16( int device, UINT16 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if ((s3c24xx->lcd.regs.lcdcon5 & (1 << 11)) == 0)
	{
		UINT8 r, g, b, i;
		r = (BITS( data, 15, 11) << 3);
		g = (BITS( data, 10, 6) << 3);
		b = (BITS( data, 5, 1) << 3);
		i = BIT( data, 1) << 2;
		return MAKE_RGB( r | i, g | i, b | i);
	}
	else
	{
		UINT8 r, g, b;
		r = BITS( data, 15, 11) << 3;
		g = BITS( data, 10, 5) << 2;
		b = BITS( data, 4, 0) << 3;
		return MAKE_RGB( r, g, b);
	}
}

static rgb_t s3c24xx_get_color_tft_24( int device, UINT32 data)
{
	UINT8 r, g, b;
	r = BITS( data, 23, 16);
	g = BITS( data, 15, 8);
	b = BITS( data, 7, 0);
	return MAKE_RGB( r, g, b);
}

static rgb_t s3c24xx_get_color_stn_12( int device, UINT16 data)
{
	UINT8 r, g, b;
	r = BITS( data, 11, 8) << 4;
	g = BITS( data, 7, 4) << 4;
	b = BITS( data, 3, 0) << 4;
	return MAKE_RGB( r, g, b);
}

static rgb_t s3c24xx_get_color_stn_08( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT8 r, g, b;
	r = ((s3c24xx->lcd.regs.redlut   >> (BITS( data, 7, 5) << 2)) & 0xF) << 4;
	g = ((s3c24xx->lcd.regs.greenlut >> (BITS( data, 4, 2) << 2)) & 0xF) << 4;
	b = ((s3c24xx->lcd.regs.bluelut  >> (BITS( data, 1, 0) << 2)) & 0xF) << 4;
	return MAKE_RGB( r, g, b);
}

static rgb_t s3c24xx_get_color_stn_01( int device, UINT8 data)
{
	if ((data & 1) == 0)
	{
		return RGB_BLACK;
	}
	else
	{
		return RGB_WHITE;
	}
}

static rgb_t s3c24xx_get_color_stn_02( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT8 r, g, b;
	r = g = b = ((s3c24xx->lcd.regs.bluelut >> (BITS( data, 1, 0) << 2)) & 0xF) << 4;
	return MAKE_RGB( r, g, b);
}

static rgb_t s3c24xx_get_color_stn_04( int device, UINT8 data)
{
	UINT8 r, g, b;
	r = g = b = BITS( data, 3, 0) << 4;
	return MAKE_RGB( r, g, b);
}

static rgb_t s3c24xx_get_color_tpal( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	return s3c24xx_get_color_tft_24( device, S3C24XX_TPAL_GET_TPALVAL( s3c24xx->lcd.tpal));
}

static void s3c24xx_lcd_dma_reload( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->lcd.vramaddr_cur = s3c24xx->lcd.regs.lcdsaddr1 << 1;
	s3c24xx->lcd.vramaddr_max = ((s3c24xx->lcd.regs.lcdsaddr1 & 0xFFE00000) | s3c24xx->lcd.regs.lcdsaddr2) << 1;
	s3c24xx->lcd.offsize = BITS( s3c24xx->lcd.regs.lcdsaddr3, 21, 11);
	s3c24xx->lcd.pagewidth_cur = 0;
	s3c24xx->lcd.pagewidth_max = BITS( s3c24xx->lcd.regs.lcdsaddr3, 10, 0);
	s3c24xx->lcd.dma_data = 0;
	s3c24xx->lcd.dma_bits = 0;
}

static void s3c24xx_lcd_dma_init( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx_lcd_dma_reload( device);
	s3c24xx->lcd.bppmode = BITS( s3c24xx->lcd.regs.lcdcon1, 4, 1);
	s3c24xx->lcd.bswp = BIT( s3c24xx->lcd.regs.lcdcon5, 1);
	s3c24xx->lcd.hwswp = BIT( s3c24xx->lcd.regs.lcdcon5, 0);
	s3c24xx->lcd.tpal = s3c24xx->lcd.regs.tpal;
	s3c24xx->lcd.dma_data = 0;
	s3c24xx->lcd.dma_bits = 0;
}

static UINT32 s3c24xx_lcd_dma_read( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT8 *vram, data[4];
	vram = (UINT8 *)memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM,  s3c24xx->lcd.vramaddr_cur);
	for (int i = 0; i < 2; i++)
	{
		if (s3c24xx->lcd.hwswp == 0)
		{
			if (s3c24xx->lcd.bswp == 0)
			{
				if ((s3c24xx->lcd.vramaddr_cur & 2) == 0)
				{
					data[i*2+0] = *(vram + 3);
					data[i*2+1] = *(vram + 2);
				}
				else
				{
					data[i*2+0] = *(vram - 1);
					data[i*2+1] = *(vram - 2);
				}
			}
			else
			{
				data[i*2+0] = *(vram + 0);
				data[i*2+1] = *(vram + 1);
			}
		}
		else
		{
			if (s3c24xx->lcd.bswp == 0)
			{
				data[i*2+0] = *(vram + 1);
				data[i*2+1] = *(vram + 0);
			}
			else
			{
				if ((s3c24xx->lcd.vramaddr_cur & 2) == 0)
				{
					data[i*2+0] = *(vram + 2);
					data[i*2+1] = *(vram + 3);
				}
				else
				{
					data[i*2+0] = *(vram - 2);
					data[i*2+1] = *(vram - 1);
				}
			}
		}
		s3c24xx->lcd.vramaddr_cur += 2;
		s3c24xx->lcd.pagewidth_cur++;
		if (s3c24xx->lcd.pagewidth_cur >= s3c24xx->lcd.pagewidth_max)
		{
			s3c24xx->lcd.vramaddr_cur += s3c24xx->lcd.offsize << 1;
			s3c24xx->lcd.pagewidth_cur = 0;
			vram = (UINT8 *)memory_get_read_ptr(0, ADDRESS_SPACE_PROGRAM,  s3c24xx->lcd.vramaddr_cur);
		}
		else
		{
			vram += 2;
		}
	}
	return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | (data[3] << 0);
}

static UINT32 s3c24xx_lcd_dma_read_bits( int device, int count)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data;
	if (count <= s3c24xx->lcd.dma_bits)
	{
		s3c24xx->lcd.dma_bits -= count;
		data = BITS( s3c24xx->lcd.dma_data, 31, 32 - count);
		s3c24xx->lcd.dma_data = s3c24xx->lcd.dma_data << count;
	}
	else
	{
		if (s3c24xx->lcd.dma_bits == 0)
		{
			if (count == 32)
			{
				data = s3c24xx_lcd_dma_read( device);
			}
			else
			{
				UINT32 temp = s3c24xx_lcd_dma_read( device);
				data = BITS( temp, 31, 32 - count);
				s3c24xx->lcd.dma_data = temp << count;
				s3c24xx->lcd.dma_bits = 32 - count;
			}
		}
		else
		{
			UINT32 temp = s3c24xx_lcd_dma_read( device);
			data = (s3c24xx->lcd.dma_data >> (32 - count)) | BITS( temp, 31, 32 - (count - s3c24xx->lcd.dma_bits));
			s3c24xx->lcd.dma_data = temp << (count - s3c24xx->lcd.dma_bits);
			s3c24xx->lcd.dma_bits = 32 - (count - s3c24xx->lcd.dma_bits);
		}
	}
	return data;
}

static void s3c24xx_lcd_render_tpal( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 color = s3c24xx_get_color_tpal( device);
	for (int y = s3c24xx->lcd.vpos_min; y <= s3c24xx->lcd.vpos_max; y++)
	{
		UINT32 *scanline = ((UINT32 *)bitmap->line[y] + (s3c24xx->lcd.hpos_min));
		for (int x = s3c24xx->lcd.hpos_min; x <= s3c24xx->lcd.hpos_max; x++)
		{
			*scanline++ = color;
		}
	}
}

static void s3c24xx_lcd_render_stn_01( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 32; j++)
		{
			*scanline++ = s3c24xx_get_color_stn_01( device, (data >> 31) & 0x01);
			data = data << 1;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 4))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_stn_02( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 16; j++)
		{
			*scanline++ = s3c24xx_get_color_stn_02( device, (data >> 30) & 0x03);
			data = data << 2;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 3))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_stn_04( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 8; j++)
		{
			*scanline++ = s3c24xx_get_color_stn_04( device, (data >> 28) & 0x0F);
			data = data << 4;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 2))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_stn_08( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 4; j++)
		{
			*scanline++ = s3c24xx_get_color_stn_08( device, (data >> 24) & 0xFF);
			data = data << 8;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 1))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_stn_12_p( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 16; i++)
	{
		*scanline++ = s3c24xx_get_color_stn_12( device, s3c24xx_lcd_dma_read_bits( device, 12));
		s3c24xx->lcd.hpos++;
		if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max * 16 / 12))
		{
			s3c24xx->lcd.vpos++;
			if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
			s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
			scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
		}
	}
}

static void s3c24xx_lcd_render_tft_01( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 32; j++)
		{
			*scanline++ = soc_palette_color( (data >> 31) & 0x01);
			data = data << 1;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 4))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_tft_02( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 16; j++)
		{
			*scanline++ = soc_palette_color( (data >> 30) & 0x03);
			data = data << 2;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 3))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_tft_04( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 8; j++)
		{
			*scanline++ = soc_palette_color( (data >> 28) & 0x0F);
			data = data << 4;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 2))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_tft_08( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 4; j++)
		{
			*scanline++ = soc_palette_color( (data >> 24) & 0xFF);
			data = data << 8;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 1))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_render_tft_16( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	mame_bitmap *bitmap = s3c24xx->lcd.bitmap[0];
	UINT32 *scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
	for (int i = 0; i < 4; i++)
	{
		UINT32 data = s3c24xx_lcd_dma_read( device);
		for (int j = 0; j < 2; j++)
		{
			*scanline++ = s3c24xx_get_color_tft_16( device, (data >> 16) & 0xFFFF);
			data = data << 16;
			s3c24xx->lcd.hpos++;
			if (s3c24xx->lcd.hpos >= s3c24xx->lcd.hpos_min + (s3c24xx->lcd.pagewidth_max << 0))
			{
				s3c24xx->lcd.vpos++;
				if (s3c24xx->lcd.vpos > s3c24xx->lcd.vpos_max) s3c24xx->lcd.vpos = s3c24xx->lcd.vpos_min;
				s3c24xx->lcd.hpos = s3c24xx->lcd.hpos_min;
				scanline = ((UINT32 *)bitmap->line[s3c24xx->lcd.vpos] + (s3c24xx->lcd.hpos));
			}
		}
	}
}

static void s3c24xx_lcd_timer_exp(int param)
{
	int device = 0;
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 tpalen;
	s3c24xx->lcd.vpos = soc.lcd.vpos_min;
	s3c24xx->lcd.hpos = soc.lcd.hpos_min;
	tpalen = S3C24XX_TPAL_GET_TPALEN( s3c24xx->lcd.tpal);
	if (tpalen == 0)
	{
		if (s3c24xx->lcd.vramaddr_cur >= s3c24xx->lcd.vramaddr_max)
		{
			s3c24xx_lcd_dma_reload( device);
		}
		while (s3c24xx->lcd.vramaddr_cur < s3c24xx->lcd.vramaddr_max)
		{
			switch (s3c24xx->lcd.bppmode)
			{
				case S3C24XX_BPPMODE_STN_01   : s3c24xx_lcd_render_stn_01( device); break;
				case S3C24XX_BPPMODE_STN_02   : s3c24xx_lcd_render_stn_02( device); break;
				case S3C24XX_BPPMODE_STN_04   : s3c24xx_lcd_render_stn_04( device); break;
				case S3C24XX_BPPMODE_STN_08   : s3c24xx_lcd_render_stn_08( device); break;
				case S3C24XX_BPPMODE_STN_12_P : s3c24xx_lcd_render_stn_12_p( device); break;
				case S3C24XX_BPPMODE_TFT_01   : s3c24xx_lcd_render_tft_01( device); break;
				case S3C24XX_BPPMODE_TFT_02   : s3c24xx_lcd_render_tft_02( device); break;
				case S3C24XX_BPPMODE_TFT_04   : s3c24xx_lcd_render_tft_04( device); break;
				case S3C24XX_BPPMODE_TFT_08   : s3c24xx_lcd_render_tft_08( device); break;
				case S3C24XX_BPPMODE_TFT_16   : s3c24xx_lcd_render_tft_16( device); break;
				default : break;
			}
			if ((s3c24xx->lcd.vpos == s3c24xx->lcd.vpos_min) && (s3c24xx->lcd.hpos == s3c24xx->lcd.hpos_min)) break;
		}
	}
	else
	{
		s3c24xx_lcd_render_tpal( device);
	}

}

static void s3c24xx_video_start( int device, int machine)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->lcd.bitmap[0] = auto_bitmap_alloc_depth(1024, 1024, 32);
	s3c24xx->lcd.bitmap[1] = auto_bitmap_alloc_depth(1024, 1024, 32);
}

static void bitmap_blend( mame_bitmap *bitmap_dst, mame_bitmap *bitmap_src_1, mame_bitmap *bitmap_src_2)
{
	for (int y = 0; y < bitmap_dst->height; y++)
	{
		UINT32 *line0 = ((UINT32 *)bitmap_src_1->line[y] + (0));
		UINT32 *line1 = ((UINT32 *)bitmap_src_2->line[y] + (0));
		UINT32 *line2 = ((UINT32 *)bitmap_dst->line[y] + (0));
		for (int x = 0; x < bitmap_dst->width; x++)
		{
				UINT32 color0 = line0[x];
				UINT32 color1 = line1[x];
				UINT16 r0 = (color0 >> 16) & 0x000000ff;
				UINT16 g0 = (color0 >>  8) & 0x000000ff;
				UINT16 b0 = (color0 >>  0) & 0x000000ff;
				UINT16 r1 = (color1 >> 16) & 0x000000ff;
				UINT16 g1 = (color1 >>  8) & 0x000000ff;
				UINT16 b1 = (color1 >>  0) & 0x000000ff;
				UINT8 r = (UINT8)((r0 + r1) >> 1);
				UINT8 g = (UINT8)((g0 + g1) >> 1);
				UINT8 b = (UINT8)((b0 + b1) >> 1);
				line2[x] = (r << 16) | (g << 8) | b;
			}
		}
}

static UINT32 s3c24xx_video_update( int device, int screen, mame_bitmap *bitmap, const rectangle *cliprect)
{
	s3c24xx_t *s3c24xx = get_token( device);
	/* Render one DMA pass per frame; LCD phase reads still follow the emulated beam. */
	if (soc.lcd.regs.lcdcon1 & 1)
	{
		s3c24xx_lcd_dma_init(0);
		s3c24xx_lcd_timer_exp(0);
	}
	if (s3c24xx->lcd.framerate >= 1195)
	{
		bitmap_blend( bitmap, s3c24xx->lcd.bitmap[0], s3c24xx->lcd.bitmap[1]);
		copybitmap( s3c24xx->lcd.bitmap[1], s3c24xx->lcd.bitmap[0], 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	}
	else
	{
		copybitmap( bitmap, s3c24xx->lcd.bitmap[0], 0, 0, 0, 0, cliprect, TRANSPARENCY_NONE, 0);
	}
	s3c24xx_lcd_dma_init( device);
	return 0;
}

UINT32 s3c2410_lcd_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->lcd.regs)[offset];
	switch (offset)
	{
		case S3C24XX_LCDCON1 :
		{

			UINT32 vpos = (cpu_getscanline() * lcd_height / Machine->drv->screen_height);
			if (vpos < s3c24xx->lcd.vpos_min) vpos = s3c24xx->lcd.vpos_min;
			if (vpos > s3c24xx->lcd.vpos_max) vpos = s3c24xx->lcd.vpos_max;
			data = (data & ~0xFFFC0000) | ((s3c24xx->lcd.vpos_max - vpos) << 18);
		}
		break;
		case S3C24XX_LCDCON5 :
		{
			UINT32 vpos = (cpu_getscanline() * lcd_height / Machine->drv->screen_height);
			data = data & ~0x00018000;
			if (vpos < s3c24xx->lcd.vpos_min) data = data | 0x00000000;
			if (vpos > s3c24xx->lcd.vpos_max) data = data | 0x00018000;

		}
		break;
	}
	return data;
}

static void s3c24xx_lcd_configure_tft( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 vspw, vbpd, lineval, vfpd, hspw, hbpd, hfpd, hozval, clkval, hclk;
	double framerate, vclk;
	UINT32 width, height;
	rectangle visarea;
	vspw = BITS( s3c24xx->lcd.regs.lcdcon2, 5, 0);
	vbpd = BITS( s3c24xx->lcd.regs.lcdcon2, 31, 24);
	lineval = BITS( s3c24xx->lcd.regs.lcdcon2, 23, 14);
	vfpd = BITS( s3c24xx->lcd.regs.lcdcon2, 13, 6);
	hspw = BITS( s3c24xx->lcd.regs.lcdcon4, 7, 0);
	hbpd = BITS( s3c24xx->lcd.regs.lcdcon3, 25, 19);
	hfpd = BITS( s3c24xx->lcd.regs.lcdcon3, 7, 0);
	hozval = BITS( s3c24xx->lcd.regs.lcdcon3, 18, 8);
	clkval = BITS( s3c24xx->lcd.regs.lcdcon1, 17, 8);
	hclk = s3c24xx_get_hclk( device);
	vclk = (double)(hclk / ((clkval + 1) * 2));
	framerate = vclk / (((vspw + 1) + (vbpd + 1) + (lineval + 1) + (vfpd + 1)) * ((hspw + 1) + (hbpd + 1) + (hozval + 1) + (hfpd + 1)));
	s3c24xx->lcd.framerate = framerate;
	width = (hspw + 1) + (hbpd + 1) + (hozval + 1) + (hfpd + 1);
	height = (vspw + 1) + (vbpd + 1) + (lineval + 1) + (vfpd + 1);
	visarea.min_x = (hspw + 1) + (hbpd + 1);
	visarea.min_y = (vspw + 1) + (vbpd + 1);
	visarea.max_x = visarea.min_x + (hozval + 1) - 1;
	visarea.max_y = visarea.min_y + (lineval + 1) - 1;
	s3c24xx->lcd.hpos_min = (hspw + 1) + (hbpd + 1);
	s3c24xx->lcd.hpos_max = s3c24xx->lcd.hpos_min + (hozval + 1) - 1;
	s3c24xx->lcd.vpos_min = (vspw + 1) + (vbpd + 1);
	s3c24xx->lcd.vpos_max = s3c24xx->lcd.vpos_min + (lineval + 1) - 1;
	lcd_height = height;
	set_visible_area(visarea.min_x, visarea.max_x, visarea.min_y, visarea.max_y);
	set_refresh_rate(framerate);
}

static void s3c24xx_lcd_configure_stn( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 pnrmode, bppmode, clkval, lineval, wdly, hozval, lineblank, wlh, hclk;
	double vclk, framerate;
	UINT32 width, height;
	rectangle visarea;
	pnrmode = BITS( s3c24xx->lcd.regs.lcdcon1, 6, 5);
	bppmode = BITS( s3c24xx->lcd.regs.lcdcon1, 4, 1);
	clkval = BITS( s3c24xx->lcd.regs.lcdcon1, 17, 8);
	lineval = BITS( s3c24xx->lcd.regs.lcdcon2, 23, 14);
	wdly = BITS( s3c24xx->lcd.regs.lcdcon3, 20, 19);
	hozval = BITS( s3c24xx->lcd.regs.lcdcon3, 18, 8);
	lineblank = BITS( s3c24xx->lcd.regs.lcdcon3, 7, 0);
	wlh = BITS( s3c24xx->lcd.regs.lcdcon4, 1, 0);
	hclk = s3c24xx_get_hclk( device);
	vclk = (double)(hclk / ((clkval + 0) * 2));
	framerate = 1 / (((1 / vclk) * (hozval + 1) + (1 / hclk) * ((1 << (4 + wlh)) + (1 << (4 + wdly)) + (lineblank * 8))) * (lineval + 1));
	switch (pnrmode)
	{
		case S3C24XX_PNRMODE_STN_04_SS : width = ((hozval + 1) * 4); break;
		case S3C24XX_PNRMODE_STN_04_DS : width = ((hozval + 1) * 4); break;
		case S3C24XX_PNRMODE_STN_08_SS : width = ((hozval + 1) * 8 / 3); break;
		default : width = 0; break;
	}
	height = lineval + 1;
	s3c24xx->lcd.framerate = framerate;
	visarea.min_x = 0;
	visarea.min_y = 0;
	visarea.max_x = width - 1;
	visarea.max_y = height - 1;
	s3c24xx->lcd.hpos_min = 0;
	s3c24xx->lcd.hpos_max = width - 1;
	s3c24xx->lcd.vpos_min = 0;
	s3c24xx->lcd.vpos_max = height - 1;
	lcd_height = height;
	set_visible_area(visarea.min_x, visarea.max_x, visarea.min_y, visarea.max_y);
	set_refresh_rate(framerate);
}

static void s3c24xx_lcd_configure( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 bppmode;
	bppmode = BITS( s3c24xx->lcd.regs.lcdcon1, 4, 1);
	if ((bppmode & (1 << 3)) == 0)
	{
		s3c24xx_lcd_configure_stn( device);
	}
	else
	{
		s3c24xx_lcd_configure_tft( device);
	}
}

static void s3c24xx_lcd_start( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx_lcd_configure( device);
	s3c24xx_lcd_dma_init( device);

}

static void s3c24xx_lcd_stop( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	timer_adjust( s3c24xx->lcd.timer, TIME_NEVER, 0, 0);
}

static void s3c24xx_lcd_recalc( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->lcd.regs.lcdcon1 & (1 << 0))
	{
		s3c24xx_lcd_start( device);
	}
	else
	{
		s3c24xx_lcd_stop( device);
	}
}

static void s3c24xx_lcd_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->lcd.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->lcd.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_LCDCON1 :
		{
			if ((old_value & (1 << 0)) != (data & (1 << 0)))
			{
				s3c24xx_lcd_recalc( device);
			}
		}
		break;
	}
}

static UINT32 s3c24xx_lcd_palette_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = s3c24xx->lcdpal.regs.data[offset];
	return data;
}

static void s3c24xx_lcd_palette_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&s3c24xx->lcdpal.regs.data[offset]);
	if (mem_mask != 0)
	{
	}
	palette_set_color(offset, (data >> 8) & 0xf8, (data >> 3) & 0xfc, (data << 3) & 0xf8);
}

static UINT32 s3c24xx_get_fclk( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data, mdiv, pdiv, sdiv;
	data = s3c24xx->clkpow.regs.mpllcon;
	mdiv = BITS( data, 19, 12);
	pdiv = BITS( data, 9, 4);
	sdiv = BITS( data, 1, 0);
	return (UINT32)((double)(1 * (mdiv + 8) * 12000000) / (double)((pdiv + 2) * (1 << sdiv)));
}

static UINT32 s3c24xx_get_hclk( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	return s3c24xx_get_fclk( device) / (BIT( s3c24xx->clkpow.regs.clkdivn, 1) + 1);
}

static UINT32 s3c24xx_get_pclk( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	return s3c24xx_get_hclk( device) / (1 << BIT( s3c24xx->clkpow.regs.clkdivn, 0));
}

static UINT32 s3c24xx_clkpow_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->clkpow.regs)[offset];
	return data;
}

static void s3c24xx_clkpow_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&((UINT32*)&s3c24xx->clkpow.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_MPLLCON :
		{
			cpunum_set_clock(0, s3c24xx_get_fclk( device) * CLOCK_MULTIPLIER);
		}
		break;
	}
}

static void s3c24xx_check_pending_irq( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 temp = s3c24xx->irq.regs.srcpnd & ~s3c24xx->irq.regs.intmsk;
	if (temp != 0)
	{
		UINT32 int_type = 0;
		while ((temp & 1) == 0)
		{
			int_type++;
			temp = temp >> 1;
		}
		s3c24xx->irq.regs.intpnd |= (1 << int_type);
		s3c24xx->irq.regs.intoffset = int_type;
		cpunum_set_input_line(0, ARM7_IRQ_LINE, ASSERT_LINE);
	}
	else
	{
		cpunum_set_input_line(0, ARM7_IRQ_LINE, CLEAR_LINE);
	}
}

static void s3c24xx_request_irq( int device, UINT32 int_type)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->irq.regs.srcpnd |= (1 << int_type);
	s3c24xx_check_pending_irq( device);
}

static void s3c24xx_check_pending_subirq( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 temp = s3c24xx->irq.regs.subsrcpnd & ~s3c24xx->irq.regs.intsubmsk;
	if (temp != 0)
	{
		UINT32 int_type = 0;
		while ((temp & 1) == 0)
		{
			int_type++;
			temp = temp >> 1;
		}
		s3c24xx_request_irq( device, MAP_SUBINT_TO_INT[int_type]);
	}
}

static void s3c24xx_request_subirq( int device, UINT32 int_type)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->irq.regs.subsrcpnd |= (1 << int_type);
	s3c24xx_check_pending_subirq( device);
}

static void s3c24xx_check_pending_eint( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 temp = s3c24xx->gpio.regs.eintpend & ~s3c24xx->gpio.regs.eintmask;
	if (temp != 0)
	{
		UINT32 int_type = 0;
		while ((temp & 1) == 0)
		{
			int_type++;
			temp = temp >> 1;
		}
		if (int_type < 8)
		{
			s3c24xx_request_irq( device, S3C24XX_INT_EINT4_7);
		}
		else
		{
			s3c24xx_request_irq( device, S3C24XX_INT_EINT8_23);
		}
	}
}

static void s3c24xx_request_eint( int device, UINT32 number)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (number < 4)
	{
		s3c24xx_request_irq( device, S3C24XX_INT_EINT0 + number);
	}
	else
	{
		s3c24xx->gpio.regs.eintpend |= (1 << number);
		s3c24xx_check_pending_eint( device);
	}
}

static UINT32 s3c24xx_irq_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->irq.regs)[offset];
	return data;
}

static void s3c24xx_irq_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->irq.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->irq.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_SRCPND :
		{
			s3c24xx->irq.regs.srcpnd = (old_value & ~data);
			s3c24xx_check_pending_irq( device);
		}
		break;
		case S3C24XX_INTMSK :
		{
			s3c24xx_check_pending_irq( device);
		}
		break;
		case S3C24XX_INTPND :
		{
			s3c24xx->irq.regs.intpnd = (old_value & ~data);
		}
		break;
		case S3C24XX_SUBSRCPND :
		{
			s3c24xx->irq.regs.subsrcpnd = (old_value & ~data);
			s3c24xx_check_pending_subirq( device);
		}
		break;
		case S3C24XX_INTSUBMSK :
		{
			s3c24xx_check_pending_subirq( device);
		}
		break;
	}
}

static UINT16 s3c24xx_pwm_calc_observation( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	double timeleft, x1, x2;
	UINT32 cnto;
	timeleft = timer_timeleft(s3c24xx->pwm.timer[ch]);

	x1 = 1 / ((double)s3c24xx->pwm.freq[ch] / (s3c24xx->pwm.cnt[ch]- s3c24xx->pwm.cmp[ch] + 1));
	x2 = x1 / timeleft;

	cnto = s3c24xx->pwm.cmp[ch] + ((s3c24xx->pwm.cnt[ch]- s3c24xx->pwm.cmp[ch]) / x2);

	return cnto;
}

static UINT32 s3c24xx_pwm_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->pwm.regs)[offset];
	switch (offset)
	{
		case S3C24XX_TCNTO0 :
		{
			data = (data & ~0x0000FFFF) | s3c24xx_pwm_calc_observation( device, 0);
		}
		break;
		case S3C24XX_TCNTO1 :
		{
			data = (data & ~0x0000FFFF) | s3c24xx_pwm_calc_observation( device, 1);
		}
		break;
		case S3C24XX_TCNTO2 :
		{
			data = (data & ~0x0000FFFF) | s3c24xx_pwm_calc_observation( device, 2);
		}
		break;
		case S3C24XX_TCNTO3 :
		{
			data = (data & ~0x0000FFFF) | s3c24xx_pwm_calc_observation( device, 3);
		}
		break;
		case S3C24XX_TCNTO4 :
		{
			data = (data & ~0x0000FFFF) | s3c24xx_pwm_calc_observation( device, 4);
		}
		break;
	}
	return data;
}

static void s3c24xx_pwm_start( int device, int timer)
{
	s3c24xx_t *s3c24xx = get_token( device);
	const int mux_table[] = { 2, 4, 8, 16};
	const int prescaler_shift[] = { 0, 0, 8, 8, 8};
	const int mux_shift[] = { 0, 4, 8, 12, 16};
	UINT32 pclk, prescaler, mux, cnt, cmp, auto_reload;
	double freq, hz;
	pclk = s3c24xx_get_pclk( device);
	prescaler = (s3c24xx->pwm.regs.tcfg0 >> prescaler_shift[timer]) & 0xFF;
	mux = (s3c24xx->pwm.regs.tcfg1 >> mux_shift[timer]) & 0x0F;
	if (mux < 4)
	{
		freq = (double)pclk / (prescaler + 1) / mux_table[mux];
	}
	else
	{

		freq = (double)pclk / (prescaler + 1) / 1;
	}
	switch (timer)
	{
		case 0 :
		{
			cnt = BITS( s3c24xx->pwm.regs.tcntb0, 15, 0);
			cmp = BITS( s3c24xx->pwm.regs.tcmpb0, 15, 0);
			auto_reload = BIT( s3c24xx->pwm.regs.tcon, 3);
		}
		break;
		case 1 :
		{
			cnt = BITS( s3c24xx->pwm.regs.tcntb1, 15, 0);
			cmp = BITS( s3c24xx->pwm.regs.tcmpb1, 15, 0);
			auto_reload = BIT( s3c24xx->pwm.regs.tcon, 11);
		}
		break;
		case 2 :
		{
			cnt = BITS( s3c24xx->pwm.regs.tcntb2, 15, 0);
			cmp = BITS( s3c24xx->pwm.regs.tcmpb2, 15, 0);
			auto_reload = BIT( s3c24xx->pwm.regs.tcon, 15);
		}
		break;
		case 3 :
		{
			cnt = BITS( s3c24xx->pwm.regs.tcntb3, 15, 0);
			cmp = BITS( s3c24xx->pwm.regs.tcmpb3, 15, 0);
			auto_reload = BIT( s3c24xx->pwm.regs.tcon, 19);
		}
		break;
		case 4 :
		{
			cnt = BITS( s3c24xx->pwm.regs.tcntb4, 15, 0);
			cmp = 0;
			auto_reload = BIT( s3c24xx->pwm.regs.tcon, 22);
		}
		break;
		default :
		{
			cnt = cmp = auto_reload = 0;
		}
		break;
	}

	if (cnt < 2)
	{
		hz = freq;
	}
	else
	{
		hz = freq / cnt;
	}
	s3c24xx->pwm.cnt[timer] = cnt;
	s3c24xx->pwm.cmp[timer] = cmp;
	s3c24xx->pwm.freq[timer] = freq;
	if (auto_reload)
	{
		timer_adjust( s3c24xx->pwm.timer[timer], TIME_IN_HZ( hz), timer, TIME_IN_HZ( hz));
	}
	else
	{
		timer_adjust( s3c24xx->pwm.timer[timer], TIME_IN_HZ( hz), timer, 0);
	}
}

static void s3c24xx_pwm_stop( int device, int timer)
{
	s3c24xx_t *s3c24xx = get_token( device);
	timer_adjust( s3c24xx->pwm.timer[timer], TIME_NEVER, 0, 0);
}

static void s3c24xx_pwm_recalc( int device, int timer)
{
	s3c24xx_t *s3c24xx = get_token( device);
	const int tcon_shift[] = { 0, 8, 12, 16, 20};
	if (s3c24xx->pwm.regs.tcon & (1 << tcon_shift[timer]))
	{
		s3c24xx_pwm_start( device, timer);
	}
	else
	{
		s3c24xx_pwm_stop( device, timer);
	}
}

static void s3c24xx_pwm_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->pwm.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->pwm.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_TCON :
		{
			if ((data & (1 << 0)) != (old_value & (1 << 0)))
			{
				s3c24xx_pwm_recalc( device, 0);
			}
			if ((data & (1 << 8)) != (old_value & (1 << 8)))
			{
				s3c24xx_pwm_recalc( device, 1);
			}
			if ((data & (1 << 12)) != (old_value & (1 << 12)))
			{
				s3c24xx_pwm_recalc( device, 2);
			}
			if ((data & (1 << 16)) != (old_value & (1 << 16)))
			{
				s3c24xx_pwm_recalc( device, 3);
			}
			if ((data & (1 << 20)) != (old_value & (1 << 20)))
			{
				s3c24xx_pwm_recalc( device, 4);
			}
		}
		break;
	}
}

static void s3c24xx_pwm_timer_exp(int param)
{
	int device = 0;
	s3c24xx_t *s3c24xx = get_token( device);
	int ch = param;
	const int ch_int[] = { S3C24XX_INT_TIMER0, S3C24XX_INT_TIMER1, S3C24XX_INT_TIMER2, S3C24XX_INT_TIMER3, S3C24XX_INT_TIMER4 };
	if (BITS( s3c24xx->pwm.regs.tcfg1, 23, 20) == (ch + 1))
	{
		s3c24xx_dma_request_pwm( device);
	}
	else
	{
		s3c24xx_request_irq( device, ch_int[ch]);
	}
}

static void s3c24xx_dma_reload( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx_dma_regs_t *regs = &s3c24xx->dma[ch].regs;
	regs->dstat = S3C24XX_DSTAT_SET_CURR_TC( regs->dstat, S3C24XX_DCON_GET_TC( regs->dcon));
	regs->dcsrc = S3C24XX_DCSRC_SET_CURR_SRC( regs->dcsrc, S3C24XX_DISRC_GET_SADDR( regs->disrc));
	regs->dcdst = S3C24XX_DCDST_SET_CURR_DST( regs->dcdst, S3C24XX_DIDST_GET_DADDR( regs->didst));
}

static void s3c24xx_dma_trigger( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx_dma_regs_t *regs = &s3c24xx->dma[ch].regs;
	UINT32 curr_tc, curr_src, curr_dst;
	int dsz, inc_src, inc_dst, servmode, tsz;
	const UINT32 ch_int[] = { S3C24XX_INT_DMA0, S3C24XX_INT_DMA1, S3C24XX_INT_DMA2, S3C24XX_INT_DMA3};
	curr_tc = S3C24XX_DSTAT_GET_CURR_TC( regs->dstat);
	dsz = S3C24XX_DCON_GET_DSZ( regs->dcon);
	curr_src = S3C24XX_DCSRC_GET_CURR_SRC( regs->dcsrc);
	curr_dst = S3C24XX_DCDST_GET_CURR_DST( regs->dcdst);
	servmode = S3C24XX_DCON_GET_SERVMODE( regs->dcon);
	tsz = S3C24XX_DCON_GET_TSZ( regs->dcon);
	inc_src = BIT( regs->disrcc, 0);
	inc_dst = BIT( regs->didstc, 0);
	while (curr_tc > 0)
	{
		curr_tc--;
		for (int i = 0; i < 1 << (tsz << 1); i++)
		{
			switch (dsz)
			{
				case 0 : program_write_byte_32le( curr_dst, program_read_byte_32le( curr_src)); break;
				case 1 : program_write_word_32le( curr_dst, program_read_word_32le( curr_src)); break;
				case 2 : program_write_dword_32le( curr_dst, program_read_dword_32le( curr_src)); break;
			}
			if (inc_src == 0) curr_src += (1 << dsz);
			if (inc_dst == 0) curr_dst += (1 << dsz);
		}
		if (servmode == 0) break;
	}
	regs->dcsrc = S3C24XX_DCSRC_SET_CURR_SRC( regs->dcsrc, curr_src);
	regs->dcdst = S3C24XX_DCDST_SET_CURR_DST( regs->dcdst, curr_dst);
	regs->dstat = S3C24XX_DSTAT_SET_CURR_TC( regs->dstat, curr_tc);
	if (curr_tc == 0)
	{
		if (S3C24XX_DCON_GET_RELOAD( regs->dcon) == 0)
		{
			s3c24xx_dma_reload( device, ch);
		}
		else
		{
			regs->dmasktrig &= ~(1 << 1);
		}
		if (S3C24XX_DCON_GET_INT( regs->dcon) != 0)
		{
			s3c24xx_request_irq( device, ch_int[ch]);
		}
	}
}

static void s3c24xx_dma_request_iis( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx_dma_regs_t *regs = &s3c24xx->dma[2].regs;
	if ((S3C24XX_DMASKTRIG_GET_ON_OFF( regs->dmasktrig) != 0) && (S3C24XX_DCON_GET_SWHWSEL( regs->dcon) != 0) && (S3C24XX_DCON_GET_HWSRCSEL( regs->dcon) == 0))
	{
		s3c24xx_dma_trigger( device, 2);
	}
}

static void s3c24xx_dma_request_pwm( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	for (int i = 0; i < 4; i++)
	{
		if (i != 1)
		{
			s3c24xx_dma_regs_t *regs = &s3c24xx->dma[i].regs;
			if ((S3C24XX_DMASKTRIG_GET_ON_OFF( regs->dmasktrig) != 0) && (S3C24XX_DCON_GET_SWHWSEL( regs->dcon) != 0) && (S3C24XX_DCON_GET_HWSRCSEL( regs->dcon) == 3))
			{
				s3c24xx_dma_trigger( device, i);
			}
		}
	}
}

static void s3c24xx_dma_start( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 addr_src, addr_dst, tc;
	s3c24xx_dma_regs_t *regs = &s3c24xx->dma[ch].regs;
	UINT32 dsz, tsz, reload;
	int inc_src, inc_dst, _int, servmode, swhwsel, hwsrcsel;
	addr_src = S3C24XX_DISRC_GET_SADDR( regs->disrc);
	addr_dst = S3C24XX_DIDST_GET_DADDR( regs->didst);
	tc = S3C24XX_DCON_GET_TC( regs->dcon);
	_int = S3C24XX_DCON_GET_INT( regs->dcon);
	servmode = S3C24XX_DCON_GET_SERVMODE( regs->dcon);
	hwsrcsel = S3C24XX_DCON_GET_HWSRCSEL( regs->dcon);
	swhwsel = S3C24XX_DCON_GET_SWHWSEL( regs->dcon);
	reload = S3C24XX_DCON_GET_RELOAD( regs->dcon);
	dsz = S3C24XX_DCON_GET_DSZ( regs->dcon);
	tsz = S3C24XX_DCON_GET_TSZ( regs->dcon);
	inc_src = BIT( regs->disrcc, 0);
	inc_dst = BIT( regs->didstc, 0);
	s3c24xx_dma_reload( device, ch);
	if (swhwsel == 0)
	{
		s3c24xx_dma_trigger( device, ch);
	}
}

static void s3c24xx_dma_stop( int device, int ch)
{
}

static void s3c24xx_dma_recalc( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if ((s3c24xx->dma[ch].regs.dmasktrig & (1 << 1)) != 0)
	{
		s3c24xx_dma_start( device, ch);
	}
	else
	{
		s3c24xx_dma_stop( device, ch);
	}
}

static UINT32 s3c24xx_dma_r( int device, UINT32 ch, UINT32 offset)
{
	s3c24xx_t *s3c24xx = get_token( device);
	return ((UINT32*)&s3c24xx->dma[ch].regs)[offset];
}

static void s3c24xx_dma_w( int device, UINT32 ch, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->dma[ch].regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->dma[ch].regs)[offset]);
	switch (offset)
	{
		case S3C24XX_DCON :
		{

		}
		break;
		case S3C24XX_DMASKTRIG :
		{
			if ((old_value & (1 << 1)) != (data & (1 << 1)))
			{
				s3c24xx_dma_recalc( device, ch);
			}
		}
		break;
	}
}

static UINT32 s3c24xx_dma_0_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_dma_r( device, 0, offset);
	return data;
}

static UINT32 s3c24xx_dma_1_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_dma_r( device, 1, offset);
	return data;
}

static UINT32 s3c24xx_dma_2_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_dma_r( device, 2, offset);
	return data;
}

static UINT32 s3c24xx_dma_3_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_dma_r( device, 3, offset);
	return data;
}

static void s3c24xx_dma_0_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_dma_w( device, 0, offset, data, mem_mask);
}

static void s3c24xx_dma_1_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_dma_w( device, 1, offset, data, mem_mask);
}

static void s3c24xx_dma_2_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_dma_w( device, 2, offset, data, mem_mask);
}

static void s3c24xx_dma_3_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_dma_w( device, 3, offset, data, mem_mask);
}

static void s3c24xx_dma_timer_exp(int param)
{
	int ch = param;
}

INLINE UINT32 iface_gpio_port_r( int device, int port)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->gpio.port_r)
	{
		return (s3c24xx->iface->gpio.port_r)( device, port);
	}
	else
	{
		return 0;
	}
}

INLINE void iface_gpio_port_w( int device, int port, UINT32 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->gpio.port_w)
	{
		(s3c24xx->iface->gpio.port_w)( device, port, data);
	}
}

static UINT32 s3c24xx_gpio_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->gpio.regs)[offset];
	switch (offset)
	{
		case S3C24XX_GPADAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_A) & S3C24XX_GPADAT_MASK;
		}
		break;
		case S3C24XX_GPBDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_B) & S3C24XX_GPBDAT_MASK;
		}
		break;
		case S3C24XX_GPCDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_C) & S3C24XX_GPCDAT_MASK;
		}
		break;
		case S3C24XX_GPDDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_D) & S3C24XX_GPDDAT_MASK;
		}
		break;
		case S3C24XX_GPEDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_E) & S3C24XX_GPEDAT_MASK;
		}
		break;
		case S3C24XX_GPFDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_F) & S3C24XX_GPFDAT_MASK;
		}
		break;
		case S3C24XX_GPGDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_G) & S3C24XX_GPGDAT_MASK;
		}
		break;
		case S3C24XX_GPHDAT :
		{
			data = iface_gpio_port_r( device, S3C24XX_GPIO_PORT_H) & S3C24XX_GPHDAT_MASK;
		}
		break;
	}
	return data;
}

static void s3c24xx_gpio_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->gpio.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->gpio.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_GPADAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_A, data & S3C24XX_GPADAT_MASK);
		}
		break;
		case S3C24XX_GPBDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_B, data & S3C24XX_GPBDAT_MASK);
		}
		break;
		case S3C24XX_GPCDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_C, data & S3C24XX_GPCDAT_MASK);
		}
		break;
		case S3C24XX_GPDDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_D, data & S3C24XX_GPDDAT_MASK);
		}
		break;
		case S3C24XX_GPEDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_E, data & S3C24XX_GPEDAT_MASK);
		}
		break;
		case S3C24XX_GPFDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_F, data & S3C24XX_GPFDAT_MASK);
		}
		break;
		case S3C24XX_GPGDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_G, data & S3C24XX_GPGDAT_MASK);
		}
		break;
		case S3C24XX_GPHDAT :
		{
			 iface_gpio_port_w( device, S3C24XX_GPIO_PORT_H, data & S3C24XX_GPHDAT_MASK);
		}
		break;
		case S3C24XX_EINTPEND :
		{
			s3c24xx->gpio.regs.eintpend = (old_value & ~data);
			s3c24xx_check_pending_eint( device);
		}
		break;
		case S3C24XX_EINTMASK :
		{
			s3c24xx_check_pending_eint( device);
		}
		break;
	}
}

static UINT32 s3c24xx_memcon_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = s3c24xx->memcon.regs.data[offset];
	return data;
}

static void s3c24xx_memcon_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&s3c24xx->memcon.regs.data[offset]);
}

static UINT32 s3c24xx_usb_host_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = s3c24xx->usbhost.regs.data[offset];
	switch (offset)
	{

		case 0x08 / 4 :
		{
			data = data & ~(1 << 0);
		}
		break;
	}
	return data;
}

static void s3c24xx_usb_host_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&s3c24xx->usbhost.regs.data[offset]);
}

static UINT32 s3c24xx_uart_r( int device, UINT32 ch, UINT32 offset)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->uart[ch].regs)[offset];
	switch (offset)
	{
		case S3C24XX_UTRSTAT :
		{
			data = (data & ~0x00000006) | 0x00000004 | 0x00000002;
		}
		break;
		case S3C24XX_URXH :
		{
			UINT8 rxdata = data & 0xFF;
			s3c24xx->uart[ch].regs.utrstat &= ~1;
		}
		break;
	}
	return data;
}

static void s3c24xx_uart_w( int device, UINT32 ch, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&((UINT32*)&s3c24xx->uart[ch].regs)[offset]);
	switch (offset)
	{
		case S3C24XX_UTXH :
		{
			UINT8 txdata = data & 0xFF;
		}
		break;
	}
}

static UINT32 s3c24xx_uart_0_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_uart_r( device, 0, offset);

	return data;
}

static UINT32 s3c24xx_uart_1_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_uart_r( device, 1, offset);

	return data;
}

static UINT32 s3c24xx_uart_2_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_uart_r( device, 2, offset);

	return data;
}

static void s3c24xx_uart_0_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{

	s3c24xx_uart_w( device, 0, offset, data, mem_mask);
}

static void s3c24xx_uart_1_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{

	s3c24xx_uart_w( device, 1, offset, data, mem_mask);
}

static void s3c24xx_uart_2_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{

	s3c24xx_uart_w( device, 2, offset, data, mem_mask);
}

static void s3c24xx_uart_fifo_w( int device, int uart, UINT8 data)
{

	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->uart[uart].regs.urxh = data;
	s3c24xx->uart[uart].regs.utrstat |= 1;
}

static UINT32 s3c24xx_usb_device_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = s3c24xx->usbdev.regs.data[offset];
	return data;
}

static void s3c24xx_usb_device_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&s3c24xx->usbdev.regs.data[offset]);
}

static UINT32 s3c24xx_wdt_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->wdt.regs)[offset];
	switch (offset)
	{
		case S3C24XX_WTCNT :
		{
			data = 0;
		}
		break;
	}
	return data;
}

static void s3c24xx_wdt_start( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 pclk, prescaler, clock;
	double freq, hz;
	pclk = s3c24xx_get_pclk( device);
	prescaler = BITS( s3c24xx->wdt.regs.wtcon, 15, 8);
	clock = 16 << BITS( s3c24xx->wdt.regs.wtcon, 4, 3);
	freq = (double)pclk / (prescaler + 1) / clock;
	hz = freq / s3c24xx->wdt.regs.wtcnt;
	timer_adjust( s3c24xx->wdt.timer, TIME_IN_HZ( hz), 0, TIME_IN_HZ( hz));
}

static void s3c24xx_wdt_stop( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	timer_adjust( s3c24xx->wdt.timer, TIME_NEVER, 0, 0);
}

static void s3c24xx_wdt_recalc( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if ((s3c24xx->wdt.regs.wtcon & (1 << 5)) != 0)
	{
		s3c24xx_wdt_start( device);
	}
	else
	{
		s3c24xx_wdt_stop( device);
	}
}

static void s3c24xx_wdt_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->wdt.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->wdt.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_WTCON :
		{
			if ((data & (1 << 5)) != (old_value & (1 << 5)))
			{
				s3c24xx_wdt_recalc( device);
			}
		}
		break;
	}
}

static void s3c24xx_wdt_timer_exp(int param)
{
	int device = 0;
	s3c24xx_t *s3c24xx = get_token( device);
	if ((s3c24xx->wdt.regs.wtcon & (1 << 2)) != 0)
	{
		s3c24xx_request_irq( device, S3C24XX_INT_WDT);
	}
	if ((s3c24xx->wdt.regs.wtcon & (1 << 0)) != 0)
	{
		s3c24xx_reset( device);
	}
}

INLINE void iface_i2c_scl_w( int device, int state)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->i2c.scl_w)
	{
		(s3c24xx->iface->i2c.scl_w)( device, state);
	}
}

INLINE void iface_i2c_sda_w( int device, int state)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->i2c.sda_w)
	{
		(s3c24xx->iface->i2c.sda_w)( device, state);
	}
}

INLINE int iface_i2c_sda_r( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->i2c.sda_r)
	{
		return (s3c24xx->iface->i2c.sda_r)( device);
	}
	else
	{
		return 0;
	}
}

static void i2c_send_start( int device)
{
	iface_i2c_sda_w( device, 1);
	iface_i2c_scl_w( device, 1);
	iface_i2c_sda_w( device, 0);
	iface_i2c_scl_w( device, 0);
}

static void i2c_send_stop( int device)
{
	iface_i2c_sda_w( device, 0);
	iface_i2c_scl_w( device, 1);
	iface_i2c_sda_w( device, 1);
	iface_i2c_scl_w( device, 0);
}

static UINT8 i2c_receive_byte( int device, int ack)
{
	UINT8 data = 0;
	iface_i2c_sda_w( device, 1);
	for (int i = 0; i < 8; i++)
	{
		iface_i2c_scl_w( device, 1);
		data = (data << 1) + (iface_i2c_sda_r( device) ? 1 : 0);
		iface_i2c_scl_w( device, 0);
	}
	iface_i2c_sda_w( device, ack ? 0 : 1);
	iface_i2c_scl_w( device, 1);
	iface_i2c_scl_w( device, 0);
	return data;
}

static int i2c_send_byte( int device, UINT8 data)
{
	int ack;
	for (int i = 0; i < 8; i++)
	{
		iface_i2c_sda_w( device, (data & 0x80) ? 1 : 0);
		data = data << 1;
		iface_i2c_scl_w( device, 1);
		iface_i2c_scl_w( device, 0);
	}
	iface_i2c_sda_w( device, 1);
	iface_i2c_scl_w( device, 1);
	ack = iface_i2c_sda_r( device);
	iface_i2c_scl_w( device, 0);
	return ack;
}

static void iic_start( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	int mode_selection;
	i2c_send_start( device);
	mode_selection = BITS( s3c24xx->iic.regs.iicstat, 7, 6);
	switch (mode_selection)
	{
		case 2 : i2c_send_byte( device, s3c24xx->iic.regs.iicds | 0x01); break;
		case 3 : i2c_send_byte( device, s3c24xx->iic.regs.iicds & 0xFE); break;
	}
	timer_adjust( s3c24xx->iic.timer, TIME_IN_USEC( 1), 0, 0);
}

static void iic_stop( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	i2c_send_stop( device);
	timer_adjust( s3c24xx->iic.timer, TIME_NEVER, 0, 0);
}

static void iic_resume( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	int mode_selection;
	mode_selection = BITS( s3c24xx->iic.regs.iicstat, 7, 6);
	switch (mode_selection)
	{
		case 2 : s3c24xx->iic.regs.iicds = i2c_receive_byte( device, BIT( s3c24xx->iic.regs.iiccon, 7)); break;
		case 3 : i2c_send_byte( device, s3c24xx->iic.regs.iicds & 0xFF); break;
	}
	timer_adjust( s3c24xx->iic.timer, TIME_IN_USEC( 1), 0, 0);
}

static UINT32 s3c24xx_iic_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->iic.regs)[offset];
	switch (offset)
	{
		case S3C24XX_IICSTAT :
		{
			data = data & ~0x0000000F;
		}
		break;
	}
	return data;
}

static void s3c24xx_iic_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->iic.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->iic.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_IICCON :
		{
			int interrupt_pending_flag;
			interrupt_pending_flag = BIT( old_value, 4);
			if (interrupt_pending_flag != 0)
			{
				interrupt_pending_flag = BIT( data, 4);
				if (interrupt_pending_flag == 0)
				{
					int start_stop_condition;
					start_stop_condition = BIT( s3c24xx->iic.regs.iicstat, 5);
					if (start_stop_condition != 0)
					{
						if (s3c24xx->iic.count == 0)
						{
							iic_start( device);

						}
						else
						{
							iic_resume( device);
						}
					}
					else
					{
						iic_stop( device);
					}
				}
			}
		}
		break;
		case  S3C24XX_IICSTAT :
		{
			int interrupt_pending_flag;
			s3c24xx->iic.count = 0;
			interrupt_pending_flag = BIT( s3c24xx->iic.regs.iiccon, 4);
			if (interrupt_pending_flag == 0)
			{
				int start_stop_condition;
				start_stop_condition = BIT( data, 5);
				if (start_stop_condition != 0)
				{
					if (s3c24xx->iic.count == 0)
					{
						iic_start( device);

					}
					else
					{
						iic_resume( device);
					}
				}
				else
				{
					iic_stop( device);
				}
			}
		}
		break;
	}
}

static void s3c24xx_iic_timer_exp(int param)
{
	int device = 0;
	s3c24xx_t *s3c24xx = get_token( device);
	int enable_interrupt;
	s3c24xx->iic.count++;
	enable_interrupt = BIT( s3c24xx->iic.regs.iiccon, 5);
	if (enable_interrupt)
	{
		s3c24xx->iic.regs.iiccon |= (1 << 4);
		s3c24xx_request_irq( device, S3C24XX_INT_IIC);
	}
}

INLINE void iface_i2s_data_w( int device, int ch, UINT16 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->i2s.data_w)
	{
		(s3c24xx->iface->i2s.data_w)( device, ch, data, 0);
	}
}

static void s3c24xx_iis_start( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	const UINT32 codeclk_table[] = { 256, 384};
	double freq;
	int pclk, prescaler_enable, prescaler_control_a, prescaler_control_b, codeclk;
	prescaler_enable = BIT( s3c24xx->iis.regs.iiscon, 1);
	prescaler_control_a = BITS( s3c24xx->iis.regs.iispsr, 9, 5);
	prescaler_control_b = BITS( s3c24xx->iis.regs.iispsr, 4, 0);
	codeclk = BIT( s3c24xx->iis.regs.iismod, 2);
	pclk = s3c24xx_get_pclk( device);
	freq = ((double)pclk / (prescaler_control_a + 1) / codeclk_table[codeclk]) * 2;
	timer_adjust( s3c24xx->iis.timer, TIME_IN_HZ( freq), 0, TIME_IN_HZ( freq));
}

static void s3c24xx_iis_stop( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	timer_adjust( s3c24xx->iis.timer, TIME_NEVER, 0, 0);
}

static void s3c24xx_iis_recalc( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if ((s3c24xx->iis.regs.iiscon & (1 << 0)) != 0)
	{
		s3c24xx_iis_start( device);
	}
	else
	{
		s3c24xx_iis_stop( device);
	}
}

static UINT32 s3c24xx_iis_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->iis.regs)[offset];
	return data;
}

static void s3c24xx_iis_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->iis.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->iis.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_IISCON :
		{
			if ((old_value & (1 << 0)) != (data & (1 << 0)))
			{
				s3c24xx_iis_recalc( device);
			}
		}
		break;
		case S3C24XX_IISFIFO :
		{
			if (ACCESSING_MSW32)
			{
				s3c24xx->iis.fifo[s3c24xx->iis.fifo_index++] = BITS( data, 31, 16);
			}
			if (ACCESSING_LSW32)
			{
				s3c24xx->iis.fifo[s3c24xx->iis.fifo_index++] = BITS( data, 15, 0);
			}
			if (s3c24xx->iis.fifo_index == 2)
			{
				s3c24xx->iis.fifo_index = 0;
				iface_i2s_data_w( device, 0, s3c24xx->iis.fifo[0]);
				iface_i2s_data_w( device, 1, s3c24xx->iis.fifo[1]);
			}
		}
		break;
	}
}

static void s3c24xx_iis_timer_exp(int param)
{
	int device = 0;
	s3c24xx_dma_request_iis( device);
}

static void s3c24xx_rtc_init( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->rtc.regs.bcdsec = dec_2_bcd( 0);
	s3c24xx->rtc.regs.bcdmin = dec_2_bcd( 0);
	s3c24xx->rtc.regs.bcdhour = dec_2_bcd( 0);
	s3c24xx->rtc.regs.bcdday = dec_2_bcd( 1);
	s3c24xx->rtc.regs.bcddow = dec_2_bcd( 1);
	s3c24xx->rtc.regs.bcdmon = dec_2_bcd( 1);
	s3c24xx->rtc.regs.bcdyear = dec_2_bcd( 4);
}

static UINT32 s3c24xx_rtc_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->rtc.regs)[offset];
	return data;
}

static void s3c24xx_rtc_recalc( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->rtc.regs.ticnt & (1 << 7))
	{
		UINT32 ttc;
		double freq;
		ttc = BITS( s3c24xx->rtc.regs.ticnt, 6, 0);
		freq = 128 / (ttc + 1);

		timer_adjust( s3c24xx->rtc.timer_tick_count, TIME_IN_HZ( freq), 0, TIME_IN_HZ( freq));
	}
	else
	{
		timer_adjust( s3c24xx->rtc.timer_tick_count, TIME_NEVER, 0, 0);
	}
}

static void s3c24xx_rtc_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&((UINT32*)&s3c24xx->rtc.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_TICNT :
		{
			s3c24xx_rtc_recalc( device);
		}
		break;
	}
}

static void s3c24xx_rtc_timer_tick_count_exp(int param)
{
	int device = 0;
	s3c24xx_request_irq( device, S3C24XX_INT_TICK);
}

static void s3c24xx_rtc_update( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 bcdday_max;

	s3c24xx->rtc.regs.bcdsec = bcd_adjust( s3c24xx->rtc.regs.bcdsec + 1);
	if (s3c24xx->rtc.regs.bcdsec >= 0x60)
	{
		s3c24xx->rtc.regs.bcdsec = 0;

		s3c24xx->rtc.regs.bcdmin = bcd_adjust( s3c24xx->rtc.regs.bcdmin + 1);
		if (s3c24xx->rtc.regs.bcdmin >= 0x60)
		{
			s3c24xx->rtc.regs.bcdmin = 0;

			s3c24xx->rtc.regs.bcdhour = bcd_adjust( s3c24xx->rtc.regs.bcdhour + 1);
			if (s3c24xx->rtc.regs.bcdhour >= 0x24)
			{
				s3c24xx->rtc.regs.bcdhour = 0;

				s3c24xx->rtc.regs.bcddow = (s3c24xx->rtc.regs.bcddow % 7) + 1;

				s3c24xx->rtc.regs.bcdday = bcd_adjust( s3c24xx->rtc.regs.bcdday + 1);
				bcdday_max = dec_2_bcd( gregorian_days_in_month( bcd_2_dec( s3c24xx->rtc.regs.bcdmon), bcd_2_dec( s3c24xx->rtc.regs.bcdyear) + 2000));
				if (s3c24xx->rtc.regs.bcdday > bcdday_max)
				{
					s3c24xx->rtc.regs.bcdday = 1;

					s3c24xx->rtc.regs.bcdmon = bcd_adjust( s3c24xx->rtc.regs.bcdmon + 1);
					if (s3c24xx->rtc.regs.bcdmon >= 0x12)
					{
						s3c24xx->rtc.regs.bcdmon = 1;

						s3c24xx->rtc.regs.bcdyear = bcd_adjust( s3c24xx->rtc.regs.bcdyear + 1);
						if (s3c24xx->rtc.regs.bcdyear >= 0x100)
						{
							s3c24xx->rtc.regs.bcdyear = 0;
						}
					}
				}
			}
		}
	}
}

static void s3c24xx_rtc_check_alarm( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->rtc.regs.rtcalm & 0x40)
	{
		int isalarm = 1;
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x20) == 0) || (s3c24xx->rtc.regs.almyear == s3c24xx->rtc.regs.bcdyear));
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x10) == 0) || (s3c24xx->rtc.regs.almmon == s3c24xx->rtc.regs.bcdmon));
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x08) == 0) || (s3c24xx->rtc.regs.almday == s3c24xx->rtc.regs.bcdday));
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x04) == 0) || (s3c24xx->rtc.regs.almhour == s3c24xx->rtc.regs.bcdhour));
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x02) == 0) || (s3c24xx->rtc.regs.almmin == s3c24xx->rtc.regs.bcdmin));
		isalarm = isalarm && (((s3c24xx->rtc.regs.rtcalm & 0x01) == 0) || (s3c24xx->rtc.regs.almsec == s3c24xx->rtc.regs.bcdsec));
		if (isalarm != 0)
		{
			s3c24xx_request_irq( device, S3C24XX_INT_RTC);
		}
	}
}

static void s3c24xx_rtc_timer_update_exp(int param)
{
	int device = 0;
	s3c24xx_rtc_update( device);
	s3c24xx_rtc_check_alarm( device);
}

static UINT32 iface_adc_data_r( int device, int ch)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->adc.data_r)
	{
		return (s3c24xx->iface->adc.data_r)( device, ch, 0);
	}
	else
	{
		return 0;
	}
}

static UINT32 s3c24xx_adc_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->adc.regs)[offset];
	switch (offset)
	{
		case S3C24XX_ADCDAT0 :
		{
			data = (data & ~0x3FF) | (iface_adc_data_r( device, 0) & 0x3FF);
		}
		break;
		case S3C24XX_ADCDAT1 :
		{
			data = (data & ~0x3FF) | (iface_adc_data_r( device, 1) & 0x3FF);
		}
		break;
	}
	return data;
}

static void s3c24xx_adc_start( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->adc.regs.adccon &= ~(1 << 0);
	s3c24xx->adc.regs.adccon |= (1 << 15);
}

static void s3c24xx_adc_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->adc.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->adc.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_ADCCON :
		{
			if (((old_value & (1 << 0)) == 0) && ((data & (1 << 0)) != 0))
			{
				s3c24xx_adc_start( device);
			}
		}
		break;
	}
}

static void s3c24xx_touch_screen( int device, int state)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->adc.regs.adcdat0 = ((state ? 0 : 1) << 15);
	s3c24xx->adc.regs.adcdat1 = ((state ? 0 : 1) << 15);
	s3c24xx_request_subirq( device, S3C24XX_SUBINT_TC);
}

static UINT32 s3c24xx_spi_r( int device, UINT32 ch, UINT32 offset)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->spi[ch].regs)[offset];
	switch (offset)
	{
		case S3C24XX_SPSTA :
		{
			data = data | (1 << 0);
		}
		break;
	}
	return data;
}

static void s3c24xx_spi_w( int device, UINT32 ch, UINT32 offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&((UINT32*)&s3c24xx->spi[ch].regs)[offset]);
}

static UINT32 s3c24xx_spi_0_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_spi_r( device, 0, offset);
	return data;
}

static UINT32 s3c24xx_spi_1_r(int device, offs_t offset, UINT32 mem_mask)
{
	UINT32 data = s3c24xx_spi_r( device, 1, offset);
	return data;
}

static void s3c24xx_spi_0_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_spi_w( device, 0, offset, data, mem_mask);
}

static void s3c24xx_spi_1_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_spi_w( device, 1, offset, data, mem_mask);
}

static UINT32 s3c24xx_sdi_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = s3c24xx->sdi.regs.data[offset];
	return data;
}

static void s3c24xx_sdi_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	COMBINE_DATA(&s3c24xx->sdi.regs.data[offset]);
}

INLINE void iface_nand_command_w( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->nand.command_w)
	{
		(s3c24xx->iface->nand.command_w)( device, 0, data);
	}
}

INLINE void iface_nand_address_w( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->nand.address_w)
	{
		(s3c24xx->iface->nand.address_w)( device, 0, data);
	}
}

INLINE UINT8 iface_nand_data_r( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->nand.data_r)
	{
		return (s3c24xx->iface->nand.data_r)( device, 0);
	}
	else
	{
		return 0;
	}
}

INLINE void iface_nand_data_w( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (s3c24xx->iface->nand.data_w)
	{
		(s3c24xx->iface->nand.data_w)( device, 0, data);
	}
}

static void nand_update_mecc( UINT8 *ecc, int pos, UINT8 data)
{
	int bit[8];
	UINT8 temp;
	bit[0] = (data >> 0) & 1;
	bit[1] = (data >> 1) & 1;
	bit[2] = (data >> 2) & 1;
	bit[3] = (data >> 3) & 1;
	bit[4] = (data >> 4) & 1;
	bit[5] = (data >> 5) & 1;
	bit[6] = (data >> 6) & 1;
	bit[7] = (data >> 7) & 1;

	ecc[2] ^= ((bit[6] ^ bit[4] ^ bit[2] ^ bit[0]) << 2);
	ecc[2] ^= ((bit[7] ^ bit[5] ^ bit[3] ^ bit[1]) << 3);
	ecc[2] ^= ((bit[5] ^ bit[4] ^ bit[1] ^ bit[0]) << 4);
	ecc[2] ^= ((bit[7] ^ bit[6] ^ bit[3] ^ bit[2]) << 5);
	ecc[2] ^= ((bit[3] ^ bit[2] ^ bit[1] ^ bit[0]) << 6);
	ecc[2] ^= ((bit[7] ^ bit[6] ^ bit[5] ^ bit[4]) << 7);

	temp = bit[7] ^ bit[6] ^ bit[5] ^ bit[4] ^ bit[3] ^ bit[2] ^ bit[1] ^ bit[0];
	if (pos & 0x001) ecc[0] ^= (temp << 1); else ecc[0] ^= (temp << 0);
	if (pos & 0x002) ecc[0] ^= (temp << 3); else ecc[0] ^= (temp << 2);
	if (pos & 0x004) ecc[0] ^= (temp << 5); else ecc[0] ^= (temp << 4);
	if (pos & 0x008) ecc[0] ^= (temp << 7); else ecc[0] ^= (temp << 6);
	if (pos & 0x010) ecc[1] ^= (temp << 1); else ecc[1] ^= (temp << 0);
	if (pos & 0x020) ecc[1] ^= (temp << 3); else ecc[1] ^= (temp << 2);
	if (pos & 0x040) ecc[1] ^= (temp << 5); else ecc[1] ^= (temp << 4);
	if (pos & 0x080) ecc[1] ^= (temp << 7); else ecc[1] ^= (temp << 6);
	if (pos & 0x100) ecc[2] ^= (temp << 1); else ecc[2] ^= (temp << 0);
	if (pos & 0x200) ecc[3] ^= (temp << 5); else ecc[3] ^= (temp << 4);
	if (pos & 0x400) ecc[3] ^= (temp << 7); else ecc[3] ^= (temp << 6);
}

static void s3c24xx_nand_update_ecc( int device, UINT8 data)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT8 temp[4];
	temp[0] = s3c24xx->nand.mecc[0];
	temp[1] = s3c24xx->nand.mecc[1];
	temp[2] = s3c24xx->nand.mecc[2];
	nand_update_mecc( s3c24xx->nand.mecc, s3c24xx->nand.pos++, data);
	if (s3c24xx->nand.pos == 512) s3c24xx->nand.pos = 0;
}

static void s3c24xx_nand_command_w( int device, UINT8 data)
{
	iface_nand_command_w( device, data);
}

static void s3c24xx_nand_address_w( int device, UINT8 data)
{
	iface_nand_address_w( device, data);
}

static UINT8 s3c24xx_nand_data_r( int device)
{
	UINT8 data = iface_nand_data_r( device);
	s3c24xx_nand_update_ecc( device, data);
	return data;
}

static void s3c24xx_nand_data_w( int device, UINT8 data)
{
	iface_nand_data_w( device, data);
	s3c24xx_nand_update_ecc( device, data);
}

static UINT32 s3c24xx_nand_r(int device, offs_t offset, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 data = ((UINT32*)&s3c24xx->nand.regs)[offset];
	switch (offset)
	{
		case S3C24XX_NFDATA :
		{
			data = 0;
			if ((~mem_mask & 0x000000FF) != 0) data = data | (s3c24xx_nand_data_r( device) <<  0);
			if ((~mem_mask & 0x0000FF00) != 0) data = data | (s3c24xx_nand_data_r( device) <<  8);
			if ((~mem_mask & 0x00FF0000) != 0) data = data | (s3c24xx_nand_data_r( device) << 16);
			if ((~mem_mask & 0xFF000000) != 0) data = data | (s3c24xx_nand_data_r( device) << 24);
		}
		break;
		case S3C24XX_NFECC :
		{
			data = (s3c24xx->nand.mecc[2] << 16) | (s3c24xx->nand.mecc[1] << 8) | (s3c24xx->nand.mecc[0] << 0);

		}
		break;
	}
	return data;
}

static void s3c24xx_nand_init_ecc( int device)
{
	s3c24xx_t *s3c24xx = get_token( device);
	s3c24xx->nand.mecc[0] = 0xff;
	s3c24xx->nand.mecc[1] = 0xff;
	s3c24xx->nand.mecc[2] = 0xff;
	s3c24xx->nand.pos = 0;
}

static void s3c24xx_nand_w(int device, offs_t offset, UINT32 data, UINT32 mem_mask)
{
	s3c24xx_t *s3c24xx = get_token( device);
	UINT32 old_value = ((UINT32*)&s3c24xx->nand.regs)[offset];
	COMBINE_DATA(&((UINT32*)&s3c24xx->nand.regs)[offset]);
	switch (offset)
	{
		case S3C24XX_NFCONF :
		{
			if ((data & (1 << 12)) != 0)
			{
				s3c24xx_nand_init_ecc( device);
			}
		}
		break;
		case S3C24XX_NFSTAT :
		{
			s3c24xx->nand.regs.nfstat = (s3c24xx->nand.regs.nfstat & ~0x03) | (old_value & 0x03);
		}
		break;
		case S3C24XX_NFCMD :
		{
			s3c24xx_nand_command_w( device, data);
		}
		break;
		case S3C24XX_NFADDR :
		{
			s3c24xx_nand_address_w( device, data);
		}
		break;
		case S3C24XX_NFDATA :
		{
			if ((~mem_mask & 0x000000FF) != 0) s3c24xx_nand_data_w( device, (data >>  0) & 0xFF);
			if ((~mem_mask & 0x0000FF00) != 0) s3c24xx_nand_data_w( device, (data >>  8) & 0xFF);
			if ((~mem_mask & 0x00FF0000) != 0) s3c24xx_nand_data_w( device, (data >> 16) & 0xFF);
			if ((~mem_mask & 0xFF000000) != 0) s3c24xx_nand_data_w( device, (data >> 24) & 0xFF);
		}
		break;
	}
}

static void s3c24xx_pin_frnb_w(int device, int state)
{
	s3c24xx_t *s3c24xx = get_token( device);
	if (state == 0)
	{
		s3c24xx->nand.regs.nfstat &= ~(1 << 0);
	}
	else
	{
		s3c24xx->nand.regs.nfstat |= (1 << 0);
	}
}

READ32_HANDLER( s3c2410_r )
{
	UINT32 address = 0x48000000 + offset * 4;
	if (address >= 0x48000000 && address <= 0x4800003b) return s3c24xx_memcon_r(0, (address - 0x48000000) / 4, mem_mask);
	if (address >= 0x49000000 && address <= 0x4900005b) return s3c24xx_usb_host_r(0, (address - 0x49000000) / 4, mem_mask);
	if (address >= 0x4a000000 && address <= 0x4a00001f) return s3c24xx_irq_r(0, (address - 0x4a000000) / 4, mem_mask);
	if (address >= 0x4b000000 && address <= 0x4b000023) return s3c24xx_dma_0_r(0, (address - 0x4b000000) / 4, mem_mask);
	if (address >= 0x4b000040 && address <= 0x4b000063) return s3c24xx_dma_1_r(0, (address - 0x4b000040) / 4, mem_mask);
	if (address >= 0x4b000080 && address <= 0x4b0000a3) return s3c24xx_dma_2_r(0, (address - 0x4b000080) / 4, mem_mask);
	if (address >= 0x4b0000c0 && address <= 0x4b0000e3) return s3c24xx_dma_3_r(0, (address - 0x4b0000c0) / 4, mem_mask);
	if (address >= 0x4c000000 && address <= 0x4c000017) return s3c24xx_clkpow_r(0, (address - 0x4c000000) / 4, mem_mask);
	if (address >= 0x4d000000 && address <= 0x4d000063) return s3c2410_lcd_r(0, (address - 0x4d000000) / 4, mem_mask);
	if (address >= 0x4d000400 && address <= 0x4d0007ff) return s3c24xx_lcd_palette_r(0, (address - 0x4d000400) / 4, mem_mask);
	if (address >= 0x4e000000 && address <= 0x4e000017) return s3c24xx_nand_r(0, (address - 0x4e000000) / 4, mem_mask);
	if (address >= 0x50000000 && address <= 0x5000002b) return s3c24xx_uart_0_r(0, (address - 0x50000000) / 4, mem_mask);
	if (address >= 0x50004000 && address <= 0x5000402b) return s3c24xx_uart_1_r(0, (address - 0x50004000) / 4, mem_mask);
	if (address >= 0x50008000 && address <= 0x5000802b) return s3c24xx_uart_2_r(0, (address - 0x50008000) / 4, mem_mask);
	if (address >= 0x51000000 && address <= 0x51000043) return s3c24xx_pwm_r(0, (address - 0x51000000) / 4, mem_mask);
	if (address >= 0x52000140 && address <= 0x5200026f) return s3c24xx_usb_device_r(0, (address - 0x52000140) / 4, mem_mask);
	if (address >= 0x53000000 && address <= 0x5300000b) return s3c24xx_wdt_r(0, (address - 0x53000000) / 4, mem_mask);
	if (address >= 0x54000000 && address <= 0x5400000f) return s3c24xx_iic_r(0, (address - 0x54000000) / 4, mem_mask);
	if (address >= 0x55000000 && address <= 0x55000013) return s3c24xx_iis_r(0, (address - 0x55000000) / 4, mem_mask);
	if (address >= 0x56000000 && address <= 0x560000bf) return s3c24xx_gpio_r(0, (address - 0x56000000) / 4, mem_mask);
	if (address >= 0x57000040 && address <= 0x5700008b) return s3c24xx_rtc_r(0, (address - 0x57000040) / 4, mem_mask);
	if (address >= 0x58000000 && address <= 0x58000013) return s3c24xx_adc_r(0, (address - 0x58000000) / 4, mem_mask);
	if (address >= 0x59000000 && address <= 0x59000017) return s3c24xx_spi_0_r(0, (address - 0x59000000) / 4, mem_mask);
	if (address >= 0x59000020 && address <= 0x59000037) return s3c24xx_spi_1_r(0, (address - 0x59000020) / 4, mem_mask);
	if (address >= 0x5a000000 && address <= 0x5a000043) return s3c24xx_sdi_r(0, (address - 0x5a000000) / 4, mem_mask);
	return 0;
}

WRITE32_HANDLER( s3c2410_w )
{
	UINT32 address = 0x48000000 + offset * 4;
	if (address >= 0x48000000 && address <= 0x4800003b) { s3c24xx_memcon_w(0, (address - 0x48000000) / 4, data, mem_mask); return; }
	if (address >= 0x49000000 && address <= 0x4900005b) { s3c24xx_usb_host_w(0, (address - 0x49000000) / 4, data, mem_mask); return; }
	if (address >= 0x4a000000 && address <= 0x4a00001f) { s3c24xx_irq_w(0, (address - 0x4a000000) / 4, data, mem_mask); return; }
	if (address >= 0x4b000000 && address <= 0x4b000023) { s3c24xx_dma_0_w(0, (address - 0x4b000000) / 4, data, mem_mask); return; }
	if (address >= 0x4b000040 && address <= 0x4b000063) { s3c24xx_dma_1_w(0, (address - 0x4b000040) / 4, data, mem_mask); return; }
	if (address >= 0x4b000080 && address <= 0x4b0000a3) { s3c24xx_dma_2_w(0, (address - 0x4b000080) / 4, data, mem_mask); return; }
	if (address >= 0x4b0000c0 && address <= 0x4b0000e3) { s3c24xx_dma_3_w(0, (address - 0x4b0000c0) / 4, data, mem_mask); return; }
	if (address >= 0x4c000000 && address <= 0x4c000017) { s3c24xx_clkpow_w(0, (address - 0x4c000000) / 4, data, mem_mask); return; }
	if (address >= 0x4d000000 && address <= 0x4d000063) { s3c24xx_lcd_w(0, (address - 0x4d000000) / 4, data, mem_mask); return; }
	if (address >= 0x4d000400 && address <= 0x4d0007ff) { s3c24xx_lcd_palette_w(0, (address - 0x4d000400) / 4, data, mem_mask); return; }
	if (address >= 0x4e000000 && address <= 0x4e000017) { s3c24xx_nand_w(0, (address - 0x4e000000) / 4, data, mem_mask); return; }
	if (address >= 0x50000000 && address <= 0x5000002b) { s3c24xx_uart_0_w(0, (address - 0x50000000) / 4, data, mem_mask); return; }
	if (address >= 0x50004000 && address <= 0x5000402b) { s3c24xx_uart_1_w(0, (address - 0x50004000) / 4, data, mem_mask); return; }
	if (address >= 0x50008000 && address <= 0x5000802b) { s3c24xx_uart_2_w(0, (address - 0x50008000) / 4, data, mem_mask); return; }
	if (address >= 0x51000000 && address <= 0x51000043) { s3c24xx_pwm_w(0, (address - 0x51000000) / 4, data, mem_mask); return; }
	if (address >= 0x52000140 && address <= 0x5200026f) { s3c24xx_usb_device_w(0, (address - 0x52000140) / 4, data, mem_mask); return; }
	if (address >= 0x53000000 && address <= 0x5300000b) { s3c24xx_wdt_w(0, (address - 0x53000000) / 4, data, mem_mask); return; }
	if (address >= 0x54000000 && address <= 0x5400000f) { s3c24xx_iic_w(0, (address - 0x54000000) / 4, data, mem_mask); return; }
	if (address >= 0x55000000 && address <= 0x55000013) { s3c24xx_iis_w(0, (address - 0x55000000) / 4, data, mem_mask); return; }
	if (address >= 0x56000000 && address <= 0x560000bf) { s3c24xx_gpio_w(0, (address - 0x56000000) / 4, data, mem_mask); return; }
	if (address >= 0x57000040 && address <= 0x5700008b) { s3c24xx_rtc_w(0, (address - 0x57000040) / 4, data, mem_mask); return; }
	if (address >= 0x58000000 && address <= 0x58000013) { s3c24xx_adc_w(0, (address - 0x58000000) / 4, data, mem_mask); return; }
	if (address >= 0x59000000 && address <= 0x59000017) { s3c24xx_spi_0_w(0, (address - 0x59000000) / 4, data, mem_mask); return; }
	if (address >= 0x59000020 && address <= 0x59000037) { s3c24xx_spi_1_w(0, (address - 0x59000020) / 4, data, mem_mask); return; }
	if (address >= 0x5a000000 && address <= 0x5a000043) { s3c24xx_sdi_w(0, (address - 0x5a000000) / 4, data, mem_mask); return; }
}

void s3c2410_init(const s3c2410_interface *iface)
{
	int i;
	memset(&soc, 0, sizeof(soc));
	soc.iface = iface;
	for (i = 0; i < 5; ++i) soc.pwm.timer[i] = timer_alloc(s3c24xx_pwm_timer_exp);
	for (i = 0; i < 4; ++i) soc.dma[i].timer = timer_alloc(s3c24xx_dma_timer_exp);
	soc.iic.timer = timer_alloc(s3c24xx_iic_timer_exp);
	soc.iis.timer = timer_alloc(s3c24xx_iis_timer_exp);
	soc.lcd.timer = timer_alloc(s3c24xx_lcd_timer_exp);
	soc.rtc.timer_tick_count = timer_alloc(s3c24xx_rtc_timer_tick_count_exp);
	soc.rtc.timer_update = timer_alloc(s3c24xx_rtc_timer_update_exp);
	soc.wdt.timer = timer_alloc(s3c24xx_wdt_timer_exp);
	timer_adjust(soc.rtc.timer_update, 1, 0, 1);
	s3c24xx_rtc_init(0);
}
void s3c2410_reset(void)
{
	int i;
	/* Keep allocated timers and bitmaps, but discard pending peripheral work on reset. */
	for (i = 0; i < 5; ++i) timer_adjust(soc.pwm.timer[i], TIME_NEVER, 0, 0);
	for (i = 0; i < 4; ++i)
	{
		timer_adjust(soc.dma[i].timer, TIME_NEVER, 0, 0);
		memset(&soc.dma[i].regs, 0, sizeof(soc.dma[i].regs));
	}
	timer_adjust(soc.iic.timer, TIME_NEVER, 0, 0);
	timer_adjust(soc.iis.timer, TIME_NEVER, 0, 0);
	timer_adjust(soc.lcd.timer, TIME_NEVER, 0, 0);
	timer_adjust(soc.wdt.timer, TIME_NEVER, 0, 0);
	timer_adjust(soc.rtc.timer_tick_count, TIME_NEVER, 0, 0);
#define RESET_REGS(name) memset(&soc.name.regs, 0, sizeof(soc.name.regs))
	RESET_REGS(memcon);
	RESET_REGS(irq);
	RESET_REGS(clkpow);
	RESET_REGS(lcd);
	RESET_REGS(pwm);
	RESET_REGS(wdt);
	RESET_REGS(iic);
	RESET_REGS(iis);
	RESET_REGS(gpio);
#undef RESET_REGS
	memset(&soc.nand, 0, sizeof(soc.nand));

	soc.iis.fifo_index = 0;
	soc.irq.regs.intmsk = 0xffffffff;
	soc.irq.regs.intsubmsk = 0x7ff;
	soc.nand.regs.nfstat = 1;
}
void s3c2410_frnb_w(int state) { s3c24xx_pin_frnb_w(0, state); }
VIDEO_START( s3c2410 ) { s3c24xx_video_start(0, 0); return 0; }
VIDEO_UPDATE( s3c2410 ) { s3c24xx_video_update(0, screen, bitmap, cliprect); }

void s3c2410_nand_calculate_mecc(UINT8 *data, UINT32 size, UINT8 *mecc)
{
	UINT32 i;
	memset(mecc, 0xff, 4);
	for (i = 0; i < size; ++i) nand_update_mecc(mecc, i, data[i]);
}
