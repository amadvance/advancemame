// license:BSD-3-Clause
// copyright-holders:Charles MacDonald, David Haywood
#include "driver.h"
#include "calcune.h"
#include "sound/sn76496.h"

#define BITMAP_WIDTH		320

#define VRAM_SIZE			0x10000
#define VRAM_MASK			(VRAM_SIZE - 1)
#define VSRAM_SIZE			0x80
#define VSRAM_MASK			(VSRAM_SIZE - 1)
#define CRAM_SIZE			0x40

#define VDP_VRAM_BYTE(x)	(vdp_vram[(x) & VRAM_MASK])
#define VDP_VSRAM_BYTE(x)	(vdp_vsram[(x) & VSRAM_MASK])
#define VDP_VRAM_WORD(x)	((VDP_VRAM_BYTE(x) << 8) | VDP_VRAM_BYTE((x) + 1))
#define VDP_VSRAM_WORD(x)	((VDP_VSRAM_BYTE(x) << 8) | VDP_VSRAM_BYTE((x) + 1))

#ifdef LSB_FIRST
#define EXTRACT_PIXEL(x,i)	(((x) >> (((i) ^ 1) * 4)) & 0x0f)
#else
#define EXTRACT_PIXEL(x,i)	(((x) >> (((i) ^ 7) * 4)) & 0x0f)
#endif

static int  vdp_data_r(void);
static void vdp_data_w(int data);
static int  vdp_control_r(void);
static void vdp_control_w(int data);
static void vdp_register_w(int data, int vblank);
static void vdp_control_dma(int data);
static void vdp_dma_68k(void);
static void vdp_dma_fill(int);
static void vdp_dma_copy(void);

static void drawline(UINT16 *bitmap, int line, int bgfill);
static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, int *offset);
static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles);
static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri, int offset, int lclip, int rclip);
static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase);

struct calcune_vdp {
	UINT16 genesis_bg_pal_lookup[4];
	UINT16 genesis_sp_pal_lookup[4];
	UINT8 regs[32];
	UINT16 scanbase;

	UINT8 * vdp_vram;
	UINT8 * vdp_vsram;
	UINT8 display_enable;
	int genesis_palette_base;

	UINT16 * transparent_lookup;

	UINT32 vdp_scrollabase;
	UINT32 vdp_scrollbbase;
	UINT32 vdp_windowbase;
	UINT32 vdp_spritebase;
	UINT32 vdp_hscrollbase;

	INT32 vdp_hscrollmask;
	UINT32 vdp_hscrollsize;
	UINT8 vdp_vscrollmode;

	UINT8 vdp_cmdpart;
	UINT8 vdp_code;
	UINT32 vdp_address;
	UINT8 vdp_dmafill;
	UINT8 scrollheight;
	UINT8 scrollwidth;
	UINT8 bgcol;
	UINT8 window_down;
	UINT32 window_vpos;
	UINT8 window_right;
	UINT32 window_hpos;
	UINT8 window_width;

	UINT16 cram[64];
};

/* Keep memory, register latches and DMA state separate for the two VDPs. */
static struct calcune_vdp chips[2];
static struct calcune_vdp *vdp;
static int selected_vdp;
#define genesis_bg_pal_lookup (vdp->genesis_bg_pal_lookup)
#define genesis_sp_pal_lookup (vdp->genesis_sp_pal_lookup)
#define genesis_vdp_regs (vdp->regs)
#define scanbase (vdp->scanbase)
#define vdp_vram (vdp->vdp_vram)
#define vdp_vsram (vdp->vdp_vsram)
#define display_enable (vdp->display_enable)
#define genesis_palette_base (vdp->genesis_palette_base)
#define transparent_lookup (vdp->transparent_lookup)
#define vdp_scrollabase (vdp->vdp_scrollabase)
#define vdp_scrollbbase (vdp->vdp_scrollbbase)
#define vdp_windowbase (vdp->vdp_windowbase)
#define vdp_spritebase (vdp->vdp_spritebase)
#define vdp_hscrollbase (vdp->vdp_hscrollbase)
#define vdp_hscrollmask (vdp->vdp_hscrollmask)
#define vdp_hscrollsize (vdp->vdp_hscrollsize)
#define vdp_vscrollmode (vdp->vdp_vscrollmode)
#define vdp_cmdpart (vdp->vdp_cmdpart)
#define vdp_code (vdp->vdp_code)
#define vdp_address (vdp->vdp_address)
#define vdp_dmafill (vdp->vdp_dmafill)
#define scrollheight (vdp->scrollheight)
#define scrollwidth (vdp->scrollwidth)
#define bgcol (vdp->bgcol)
#define window_down (vdp->window_down)
#define window_vpos (vdp->window_vpos)
#define window_right (vdp->window_right)
#define window_hpos (vdp->window_hpos)
#define window_width (vdp->window_width)

static int start_vdp(void)
{
	static const UINT8 vdp_init[24] =
	{
		0x04, 0x44, 0x30, 0x3C, 0x07, 0x6C, 0x00, 0x00,
		0x00, 0x00, 0xFF, 0x00, 0x01, 0x37, 0x00, 0x02,
		0x01, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x80,
	};
	int i;

	vdp_vram			= auto_malloc(VRAM_SIZE);
	vdp_vsram			= auto_malloc(VSRAM_SIZE);
	transparent_lookup	= auto_malloc(0x1000 * sizeof(UINT16));

	memset(vdp_vram, 0, VRAM_SIZE);
	memset(vdp_vsram, 0, VSRAM_SIZE);

	for (i = 0; i < 0x1000; i++)
	{
		int orig_color = i & 0x7ff;
		int half_bright = i & 0x800;

		if (orig_color & 0x100)
			transparent_lookup[i] = orig_color;
		else if (half_bright)
			transparent_lookup[i] = orig_color | 0x800;
		else
			transparent_lookup[i] = orig_color | 0x1000;
	}

	genesis_palette_base = selected_vdp * 64;
	genesis_bg_pal_lookup[0] = genesis_sp_pal_lookup[0] = genesis_palette_base + 0x00;
	genesis_bg_pal_lookup[1] = genesis_sp_pal_lookup[1] = genesis_palette_base + 0x10;
	genesis_bg_pal_lookup[2] = genesis_sp_pal_lookup[2] = genesis_palette_base + 0x20;
	genesis_bg_pal_lookup[3] = genesis_sp_pal_lookup[3] = genesis_palette_base + 0x30;

	scanbase = 0;

	for (i = 0; i < 24; i++)
		vdp_register_w(0x8000 | (i << 8) | vdp_init[i], 1);
	vdp_cmdpart = 0;
	vdp_code    = 0;
	vdp_address = 0;

	return 0;
}

static READ16_HANDLER( vdp_r )
{
	switch (offset)
	{
		case 0x00:
		case 0x01:
			return vdp_data_r();

		case 0x02:
		case 0x03:
			return vdp_control_r();

		case 0x04:
		case 0x05:
		case 0x06:
		case 0x07:
		{
			int xpos = cpu_gethorzbeampos();
			int ypos = cpu_getscanline();

			if (xpos > 0xe9) xpos -= (342 - 0x100);
			if (ypos > 0xea) ypos -= (262 - 0x100);

			if (ypos > 0) ypos -= 2;
			return (ypos << 8) | xpos;
		}
	}
	return 0;
}

static WRITE16_HANDLER( vdp_w )
{
	switch (offset)
	{
		case 0x00:
		case 0x01:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
					data |= data >> 8;
				 else
					data |= data << 8;
			}
			vdp_data_w(data);
			break;

		case 0x02:
		case 0x03:
			if (mem_mask)
			{
				data &= ~mem_mask;
				 if (ACCESSING_MSB)
					data |= data >> 8;
				 else
					data |= data << 8;
			}
			vdp_control_w(data);
			break;

		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0b:
			if (ACCESSING_LSB && sndti_to_sndnum(SOUND_SN76496, 0) != -1)
				SN76496_0_w(0, data & 0xff);
			break;
	}
}

static int vdp_data_r(void)
{
	int read = 0;

	vdp_cmdpart = 0;

	switch (vdp_code & 0x0f)
	{
		case 0x00:
			read = VDP_VRAM_WORD(vdp_address & ~1);
			break;

		case 0x08:
			read = vdp->cram[(vdp_address >> 1) & 63];
			break;

		case 0x04:
			read = VDP_VSRAM_WORD(vdp_address & ~1);
			break;

		default:
			logerror("%06x: VDP illegal read type %02x\n", activecpu_get_previouspc(), vdp_code);
			read = 0x00;
			break;
	}

	vdp_address += genesis_vdp_regs[15];
	return read;
}

static void vdp_data_w(int data)
{

	vdp_cmdpart = 0;

	if (vdp_dmafill)
	{
		vdp_dma_fill(data);
		vdp_dmafill = 0;
		return;
	}

	switch (vdp_code & 0x0f)
	{
		case 0x01:

			if (!cpu_getvblank() &&
				vdp_address >= vdp_hscrollbase &&
				vdp_address < vdp_hscrollbase + vdp_hscrollsize)
				force_partial_update(cpu_getscanline() + scanbase);

			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VRAM_BYTE(vdp_address |  1) = data;
			break;

		case 0x03:
			{
				int offset = (vdp_address/2) % CRAM_SIZE;
				int r = (data >> 1) & 0x07;
				int g = (data >> 5) & 0x07;
				int b = (data >> 9) & 0x07;

				r = (r << 5) | (r << 2) | (r >> 1);
				g = (g << 5) | (g << 2) | (g >> 1);
				b = (b << 5) | (b << 2) | (b >> 1);

				if (!cpu_getvblank())
					force_partial_update(cpu_getscanline());
				vdp->cram[offset] = data;
				palette_set_color(offset + genesis_palette_base, r, g, b);
				palette_set_color(offset + genesis_palette_base + 0x800, r / 2, g / 2, b / 2);
				palette_set_color(offset + genesis_palette_base + 0x1000, 128 + r / 2, 128 + g / 2, 128 + b / 2);
			}
			break;

		case 0x05:

			if (!cpu_getvblank())
				force_partial_update(cpu_getscanline() + scanbase);

			if (vdp_address & 1)
				data = ((data & 0xff) << 8) | ((data >> 8) & 0xff);
			VDP_VSRAM_BYTE(vdp_address & ~1) = data >> 8;
			VDP_VSRAM_BYTE(vdp_address |  1) = data;
			break;

		default:
			logerror("PC:%06x: VDP illegal write type %02x data %04x\n", activecpu_get_previouspc(), vdp_code, data);
			break;
	}

	vdp_address += genesis_vdp_regs[15];
}

static int vdp_control_r(void)
{
	int beampos = cpu_gethorzbeampos();
	int status = 0x3600;

	vdp_cmdpart = 0;

	if (cpu_getvblank())
		status |= 0x0008;

	if (beampos >= (Machine->drv->screen_width * 4) / 5)
		status |= 0x0004;

	return (status);
}

static void vdp_control_w(int data)
{

	if (!vdp_cmdpart)
	{

		if ((data & 0xc000) == 0x8000)
			vdp_register_w(data, cpu_getvblank());

		else
		{
			vdp_code    = (vdp_code & 0x3c) | ((data >> 14) & 0x03);
			vdp_address = (vdp_address & 0xc000) | (data & 0x3fff);
			vdp_cmdpart = 1;
		}
	}

	else
	{
		vdp_code    = (vdp_code & 0x03) | ((data >> 2) & 0x3c);
		vdp_address = (vdp_address & 0x3fff) | ((data << 14) & 0xc000);
		vdp_cmdpart = 0;
		vdp_control_dma(data);
	}
}

static void vdp_register_w(int data, int vblank)
{
	static const UINT8 is_important[32] = { 0,0,1,1,1,1,0,1,0,0,0,1,0,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	UINT8 regnum = (data & 0x1f00) >> 8;
	UINT8 regdat = (data & 0x00ff);

	if (!vblank && is_important[regnum])
		force_partial_update(cpu_getscanline() + scanbase);

	genesis_vdp_regs[regnum] = regdat;

	switch (regnum)
	{
		case 0x02:
			vdp_scrollabase = (regdat & 0x38) << 10;
			break;

		case 0x03:
			vdp_windowbase = (regdat & 0x3e) << 10;
			break;

		case 0x04:
			vdp_scrollbbase = (regdat & 0x07) << 13;
			break;

		case 0x05:
			vdp_spritebase = (regdat & ((genesis_vdp_regs[12] & 0x01) ? 0x7e : 0x7f)) << 9;
			break;

		case 0x07:
			bgcol = regdat & 0x3f;
			break;

		case 0x0b:
		{
			static const UINT16 mask_table[4] = { 0x000, 0x007, 0xff8, 0xfff };
			vdp_vscrollmode = (regdat & 0x04) >> 2;

			vdp_hscrollmask = mask_table[regdat & 3];
			vdp_hscrollsize = 4 * ((vdp_hscrollmask < 224) ? (vdp_hscrollmask + 1) : 224);
			break;
		}

		case 0x0c:

			switch (regdat&0x81)
			{
				case 0x00:

					window_width=32;
				break;
				case 0x01:

					window_width=64;
				break;
				case 0x80:

					window_width=64;
				break;
				case 0x81:

					window_width=64;
				break;
			}

			vdp_spritebase = (genesis_vdp_regs[5] & ((regdat & 0x01) ? 0x7e : 0x7f)) << 9;
			break;

		case 0x0d:
			vdp_hscrollbase = (regdat & 0x3f) << 10;
			break;

		case 0x10:
		{
			static const UINT8 size_table[4] = { 32, 64, 128, 128 };
			scrollwidth = size_table[regdat & 0x03];
			scrollheight = size_table[(regdat & 0x30) >> 4];
			break;
		}

		case 0x11:
			window_right = regdat & 0x80;
			window_hpos = (regdat & 0x1f) << 4;
			break;

		case 0x12:
			window_down = regdat & 0x80;
			window_vpos = (regdat & 0x1f) << 3;
			break;
	}
}

static void vdp_control_dma(int data)
{
	if ((vdp_code & 0x20) && (genesis_vdp_regs[1] & 0x10))
	{
		switch(genesis_vdp_regs[23] & 0xc0)
		{
			case 0x00:
			case 0x40:
				vdp_dma_68k();
				break;

			case 0x80:
				vdp_dmafill = 1;
				break;

			case 0xC0:
				vdp_dma_copy();
				break;
		}
	}
}

static void vdp_dma_68k(void)
{
	int length = genesis_vdp_regs[19] | (genesis_vdp_regs[20] << 8);
	int source = (genesis_vdp_regs[21] << 1) | (genesis_vdp_regs[22] << 9) | ((genesis_vdp_regs[23] & 0x7f) << 17);
	int count;

	if (!length)
		length = 0xffff;

	for (count = 0; count < length; count++)
	{
		vdp_data_w(program_read_word(source));
		source += 2;
	}
}

static void vdp_dma_copy(void)
{
	int length = genesis_vdp_regs[19] | (genesis_vdp_regs[20] << 8);
	int source = genesis_vdp_regs[21] | (genesis_vdp_regs[22] << 8);
	int count;

	if (!length)
		length = 0xffff;

	for (count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address) = VDP_VRAM_BYTE(source++);
		vdp_address += genesis_vdp_regs[15];
	}
}

static void vdp_dma_fill(int data)
{
	int length = genesis_vdp_regs[19] | (genesis_vdp_regs[20] << 8);
	int count;

	if (!length)
		length = 0xffff;

	VDP_VRAM_BYTE(vdp_address) = data;
	data >>= 8;
	for(count = 0; count < length; count++)
	{
		VDP_VRAM_BYTE(vdp_address ^ 1) = data;
		vdp_address += genesis_vdp_regs[15];
	}
}

INLINE int vdp_gethscroll(int plane, int line)
{
	line &= vdp_hscrollmask;
	return 0x400 - (VDP_VRAM_WORD(vdp_hscrollbase + (4 * line) + plane) & 0x3ff);
}

static int vdp_getvscroll(int plane, int column)
{
	UINT32 vsramoffset;

	switch (vdp_vscrollmode)
	{
		case 0x00:
			return VDP_VSRAM_WORD(plane) & 0x7ff;

		case 0x01:
			if (column == 40) column = 39;
			vsramoffset = (4 * (column >> 1)) + plane;
			return VDP_VSRAM_WORD(vsramoffset) & 0x7ff;
	}
	return 0;
}

static void drawline(UINT16 *bitmap, int line, int bgfill)
{
	int lowsprites, highsprites, link;
	UINT32 scrolla_tiles[41], scrollb_tiles[41], window_tiles[41];
	int scrolla_offset, scrollb_offset;
	UINT8 *lowlist[81], *highlist[81];
	int bgcolor = bgfill ? bgfill : genesis_palette_base + bgcol;
	int window_lclip, window_rclip;
	int scrolla_lclip, scrolla_rclip;
	int column, sprite;

	for (column = 0; column < BITMAP_WIDTH; column++)
		bitmap[column] = bgcolor;

	if (!(genesis_vdp_regs[1] & 0x40))
		return;

	link = lowsprites = highsprites = 0;
	for (sprite = 0; sprite < 80; sprite++)
	{
		UINT8 *spritebase = &VDP_VRAM_BYTE(vdp_spritebase + 8 * link);

		if (spritebase[4] & 0x0080)
			highlist[++highsprites] = spritebase;
		else
			lowlist[++lowsprites] = spritebase;

		link = spritebase[3] & 0x7F;
		if (!link)
			break;
	}

	get_scroll_tiles(line, 2, vdp_scrollbbase, scrollb_tiles, &scrollb_offset);

	get_scroll_tiles(line, 0, vdp_scrollabase, scrolla_tiles, &scrolla_offset);

	get_window_tiles(line, vdp_windowbase, window_tiles);

	if ((window_down && line >= window_vpos) || (!window_down && line < window_vpos))
		window_lclip = 0, window_rclip = BITMAP_WIDTH - 1;
	else if (window_right)
		window_lclip = window_hpos, window_rclip = BITMAP_WIDTH - 1;
	else
		window_lclip = 0, window_rclip = window_hpos - 1;

	if (window_lclip == 0)
	{
		scrolla_lclip = window_rclip + 1;
		scrolla_rclip = BITMAP_WIDTH - 1;
	}
	else
	{
		scrolla_lclip = 0;
		scrolla_rclip = window_lclip - 1;
	}

	drawline_tiles(scrollb_tiles, bitmap, 0, scrollb_offset, 0, BITMAP_WIDTH - 1);

	drawline_tiles(scrolla_tiles, bitmap, 0, scrolla_offset, scrolla_lclip, scrolla_rclip);

	drawline_tiles(window_tiles, bitmap, 0, 0, window_lclip, window_rclip);

	for (sprite = lowsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 0, lowlist[sprite]);

	drawline_tiles(scrollb_tiles, bitmap, 1, scrollb_offset, 0, BITMAP_WIDTH - 1);

	drawline_tiles(scrolla_tiles, bitmap, 1, scrolla_offset, scrolla_lclip, scrolla_rclip);

	drawline_tiles(window_tiles, bitmap, 1, 0, window_lclip, window_rclip);

	for (sprite = highsprites; sprite > 0; sprite--)
		drawline_sprite(line, bitmap, 1, highlist[sprite]);
}

static void get_scroll_tiles(int line, int scrollnum, UINT32 scrollbase, UINT32 *tiles, int *offset)
{
	int linehscroll = vdp_gethscroll(scrollnum, line);
	int column;

	*offset = -(linehscroll % 8);
	linehscroll /= 8;

	for (column = 0; column < 41; column++)
	{
		int columnvscroll = vdp_getvscroll(scrollnum, (column - (linehscroll & 1)) & 0x3f) + line;

		int temp = ((columnvscroll / 8) & (scrollheight - 1)) * scrollwidth;
		int tilebase = scrollbase + 2 * temp;

		temp = (linehscroll + column) & (scrollwidth - 1);
		tilebase += 2 * temp;

		*tiles++ = ((columnvscroll % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}

static void get_window_tiles(int line, UINT32 scrollbase, UINT32 *tiles)
{
	int column;

	for (column = 0; column < 40; column++)
	{

		int temp = (line / 8) * window_width + column;
		int tilebase = scrollbase + 2 * temp;

		*tiles++ = ((line % 8) << 16) | VDP_VRAM_WORD(tilebase);
	}
}

static void drawline_tiles(UINT32 *tiles, UINT16 *bmap, int pri, int offset, int lclip, int rclip)
{

	bmap += offset;
	if (lclip > rclip)
		return;

	for ( ; offset < BITMAP_WIDTH; offset += 8, bmap += 8)
	{
		UINT32 tile = *tiles++;

		if (((tile >> 15) & 1) == pri && offset < BITMAP_WIDTH)
		{
			int colbase = genesis_bg_pal_lookup[(tile & 0x6000) >> 13];
			UINT32 *tp = (UINT32 *)&VDP_VRAM_BYTE((tile & 0x7ff) * 32);
			UINT32 mytile;
			int col;

			if (!(tile & 0x1000))
				mytile = tp[tile >> 16];
			else
				mytile = tp[(tile >> 16) ^ 7];

			if (!mytile)
				continue;

			if (offset >= lclip && offset <= rclip - 7)
			{

				if (!(tile & 0x0800))
				{
					col = EXTRACT_PIXEL(mytile, 0); if (col) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 7); if (col) bmap[7] = colbase + col;
				}

				else
				{
					col = EXTRACT_PIXEL(mytile, 7); if (col) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 0); if (col) bmap[7] = colbase + col;
				}
			}

			else if (offset >= lclip - 8 && offset <= rclip)
			{

				if (!(tile & 0x0800))
				{
					col = EXTRACT_PIXEL(mytile, 0); if (col && (offset + 0) >= lclip && (offset + 0) <= rclip) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col && (offset + 1) >= lclip && (offset + 1) <= rclip) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col && (offset + 2) >= lclip && (offset + 2) <= rclip) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col && (offset + 3) >= lclip && (offset + 3) <= rclip) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col && (offset + 4) >= lclip && (offset + 4) <= rclip) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col && (offset + 5) >= lclip && (offset + 5) <= rclip) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col && (offset + 6) >= lclip && (offset + 6) <= rclip) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 7); if (col && (offset + 7) >= lclip && (offset + 7) <= rclip) bmap[7] = colbase + col;
				}

				else
				{
					col = EXTRACT_PIXEL(mytile, 7); if (col && (offset + 0) >= lclip && (offset + 0) <= rclip) bmap[0] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 6); if (col && (offset + 1) >= lclip && (offset + 1) <= rclip) bmap[1] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 5); if (col && (offset + 2) >= lclip && (offset + 2) <= rclip) bmap[2] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 4); if (col && (offset + 3) >= lclip && (offset + 3) <= rclip) bmap[3] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 3); if (col && (offset + 4) >= lclip && (offset + 4) <= rclip) bmap[4] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 2); if (col && (offset + 5) >= lclip && (offset + 5) <= rclip) bmap[5] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 1); if (col && (offset + 6) >= lclip && (offset + 6) <= rclip) bmap[6] = colbase + col;
					col = EXTRACT_PIXEL(mytile, 0); if (col && (offset + 7) >= lclip && (offset + 7) <= rclip) bmap[7] = colbase + col;
				}
			}
		}
	}
}

INLINE void draw8pixs(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	if (!tile)
		return;

	if ((colbase & 0x30) != 0x30 || !(genesis_vdp_regs[12] & 0x08))
	{
		col = EXTRACT_PIXEL(tile, 0); if (col) bmap[0] = colbase + col;
		col = EXTRACT_PIXEL(tile, 1); if (col) bmap[1] = colbase + col;
		col = EXTRACT_PIXEL(tile, 2); if (col) bmap[2] = colbase + col;
		col = EXTRACT_PIXEL(tile, 3); if (col) bmap[3] = colbase + col;
		col = EXTRACT_PIXEL(tile, 4); if (col) bmap[4] = colbase + col;
		col = EXTRACT_PIXEL(tile, 5); if (col) bmap[5] = colbase + col;
		col = EXTRACT_PIXEL(tile, 6); if (col) bmap[6] = colbase + col;
		col = EXTRACT_PIXEL(tile, 7); if (col) bmap[7] = colbase + col;
	}

	else
	{
		col = EXTRACT_PIXEL(tile, 0);
		if (col)
		{
			if (col < 0x0e) bmap[0] = colbase + col;
			else bmap[0] = transparent_lookup[((col & 1) << 11) | (bmap[0] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 1);
		if (col)
		{
			if (col < 0x0e) bmap[1] = colbase + col;
			else bmap[1] = transparent_lookup[((col & 1) << 11) | (bmap[1] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 2);
		if (col)
		{
			if (col < 0x0e) bmap[2] = colbase + col;
			else bmap[2] = transparent_lookup[((col & 1) << 11) | (bmap[2] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 3);
		if (col)
		{
			if (col < 0x0e) bmap[3] = colbase + col;
			else bmap[3] = transparent_lookup[((col & 1) << 11) | (bmap[3] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 4);
		if (col)
		{
			if (col < 0x0e) bmap[4] = colbase + col;
			else bmap[4] = transparent_lookup[((col & 1) << 11) | (bmap[4] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 5);
		if (col)
		{
			if (col < 0x0e) bmap[5] = colbase + col;
			else bmap[5] = transparent_lookup[((col & 1) << 11) | (bmap[5] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 6);
		if (col)
		{
			if (col < 0x0e) bmap[6] = colbase + col;
			else bmap[6] = transparent_lookup[((col & 1) << 11) | (bmap[6] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 7);
		if (col)
		{
			if (col < 0x0e) bmap[7] = colbase + col;
			else bmap[7] = transparent_lookup[((col & 1) << 11) | (bmap[7] & 0x7ff)];
		}
	}
}

INLINE void draw8pixs_hflip(UINT16 *bmap, int patno, int priority, int colbase, int patline)
{
	UINT32 tile = *(UINT32 *)&VDP_VRAM_BYTE(patno * 32 + 4 * patline);
	int col;

	if (!tile)
		return;

	if ((colbase & 0x30) != 0x30 || !(genesis_vdp_regs[12] & 0x08))
	{
		col = EXTRACT_PIXEL(tile, 7); if (col) bmap[0] = colbase + col;
		col = EXTRACT_PIXEL(tile, 6); if (col) bmap[1] = colbase + col;
		col = EXTRACT_PIXEL(tile, 5); if (col) bmap[2] = colbase + col;
		col = EXTRACT_PIXEL(tile, 4); if (col) bmap[3] = colbase + col;
		col = EXTRACT_PIXEL(tile, 3); if (col) bmap[4] = colbase + col;
		col = EXTRACT_PIXEL(tile, 2); if (col) bmap[5] = colbase + col;
		col = EXTRACT_PIXEL(tile, 1); if (col) bmap[6] = colbase + col;
		col = EXTRACT_PIXEL(tile, 0); if (col) bmap[7] = colbase + col;
	}

	else
	{
		col = EXTRACT_PIXEL(tile, 7);
		if (col)
		{
			if (col < 0x0e) bmap[0] = colbase + col;
			else bmap[0] = transparent_lookup[((col & 1) << 11) | (bmap[0] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 6);
		if (col)
		{
			if (col < 0x0e) bmap[1] = colbase + col;
			else bmap[1] = transparent_lookup[((col & 1) << 11) | (bmap[1] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 5);
		if (col)
		{
			if (col < 0x0e) bmap[2] = colbase + col;
			else bmap[2] = transparent_lookup[((col & 1) << 11) | (bmap[2] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 4);
		if (col)
		{
			if (col < 0x0e) bmap[3] = colbase + col;
			else bmap[3] = transparent_lookup[((col & 1) << 11) | (bmap[3] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 3);
		if (col)
		{
			if (col < 0x0e) bmap[4] = colbase + col;
			else bmap[4] = transparent_lookup[((col & 1) << 11) | (bmap[4] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 2);
		if (col)
		{
			if (col < 0x0e) bmap[5] = colbase + col;
			else bmap[5] = transparent_lookup[((col & 1) << 11) | (bmap[5] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 1);
		if (col)
		{
			if (col < 0x0e) bmap[6] = colbase + col;
			else bmap[6] = transparent_lookup[((col & 1) << 11) | (bmap[6] & 0x7ff)];
		}
		col = EXTRACT_PIXEL(tile, 0);
		if (col)
		{
			if (col < 0x0e) bmap[7] = colbase + col;
			else bmap[7] = transparent_lookup[((col & 1) << 11) | (bmap[7] & 0x7ff)];
		}
	}
}

static void drawline_sprite(int line, UINT16 *bmap, int priority, UINT8 *spritebase)
{
	int spriteypos   = (((spritebase[0] & 0x01) << 8) | spritebase[1]) - 0x80;
	int spritexpos   = (((spritebase[6] & 0x01) << 8) | spritebase[7]) - 0x80;
	int spriteheight = ((spritebase[2] & 0x03) + 1) * 8;
	int spritewidth  = (((spritebase[2] & 0x0c) >> 2) + 1) * 8;
	int spriteattr, patno, patflip, patline, colbase, x;

	if (line < spriteypos || line >= spriteypos + spriteheight)
		return;
	if (spritexpos + spritewidth < 0 || spritexpos >= BITMAP_WIDTH)
		return;

	spriteattr = (spritebase[4] << 8) | spritebase[5];
	patno      = spriteattr & 0x07FF;
	patflip    = (spriteattr & 0x1800) >> 11;
	patline    = line - spriteypos;

	colbase = genesis_sp_pal_lookup[(spriteattr & 0x6000) >> 13];

	spritewidth >>= 3;
	spriteheight >>= 3;

	bmap += spritexpos;
	switch (patflip)
	{
		case 0x00:
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x01:
			patno += spriteheight * (spritewidth - 1);
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;

		case 0x02:
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno += spriteheight;
			}
			break;

		case 0x03:
			patno += spriteheight * (spritewidth - 1);
			patline = 8 * spriteheight - patline - 1;
			for (x = 0; x < spritewidth; x++, bmap += 8)
			{
				if (spritexpos >= -7 && spritexpos < BITMAP_WIDTH)
					draw8pixs_hflip(bmap, patno, priority, colbase, patline);
				spritexpos += 8;
				patno -= spriteheight;
			}
			break;
	}
}

VIDEO_START(calcune)
{
	int i;
	memset(chips, 0, sizeof(chips));
	for (i = 0; i < 2; ++i) {
		selected_vdp = i;
		vdp = &chips[i];
		if (start_vdp())
			return 1;
	}
	return 0;
}

READ16_HANDLER(calcune_vdp_r)
{
	vdp = &chips[selected_vdp];
	return vdp_r(offset, mem_mask);
}

WRITE16_HANDLER(calcune_vdp_w)
{
	vdp = &chips[selected_vdp];
	if (offset >= 8 && offset <= 11) {
		if (ACCESSING_LSB) {
			if (selected_vdp)
				SN76496_1_w(0, data & 0xff);
			else
				SN76496_0_w(0, data & 0xff);
		}
	} else
		vdp_w(offset, data, mem_mask);
}

void calcune_vdp_select(int chip)
{
	selected_vdp = chip != 0;
}

void calcune_vdp_interrupt(int line)
{
	static int hint_count;
	if (line == 0)
		hint_count = chips[0].regs[10];
	if (line == 224 && (chips[0].regs[1] & 0x20))
		cpunum_set_input_line(0, 6, HOLD_LINE);
	if (line < 224 && --hint_count < 0) {
		hint_count = chips[0].regs[10];
		if (chips[0].regs[0] & 0x10)
			cpunum_set_input_line(0, 4, HOLD_LINE);
	}
}

VIDEO_UPDATE(calcune)
{
	UINT16 bottom[320], top[320];
	int x, y;
	/* Each VDP owns its palette. Transparent foreground pixels expose VDP 2. */
	for (y = cliprect->min_y; y <= cliprect->max_y; ++y) {
		vdp = &chips[1];
		drawline(bottom, y, 0);
		vdp = &chips[0];
		drawline(top, y, 0xffff);
		for (x = cliprect->min_x; x <= cliprect->max_x; ++x) {
			int pen = bottom[x];
			if (top[x] != 0xffff && (top[x] & 15))
				pen = top[x];
			plot_pixel(bitmap, x, y, Machine->pens[pen]);
		}
	}
	vdp = &chips[selected_vdp];
}
