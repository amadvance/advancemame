/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2003, 2004 Andrea Mazzoleni
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

#include "emu.h"
#include "thread.h"
#include "glue.h"
#include "input.h"
#include "portable.h"

#include "os.h"
#include "log.h"
#include "target.h"
#include "snstring.h"
#include "error.h"
#include "fontdef.h"
#include "font.h"

#include <string.h>
#include <stdlib.h>

/**************************************************************************/
/* Internal */

static void ui_text_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned cf, unsigned cb)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x / 2;
	pos_y = y;

	adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf, cb);
}

static void ui_text_left(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned cf, unsigned cb)
{
	int pos_x, pos_y;

	pos_x = x;
	pos_y = y;

	adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf, cb);
}

static void ui_text_right(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned cf, unsigned cb)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x;
	pos_y = y;

	adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf, cb);
}

static void ui_messagebox_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, unsigned cf, unsigned cb)
{
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;

	border_x = adv_font_sizex(context->state.ui_font);
	border_y = adv_font_sizey(context->state.ui_font) / 2;

	end = adv_font_sizex_limit(context->state.ui_font, begin, end, dst->size_x - border_x * 2);

	size_x = border_x * 2 + adv_font_sizex_string(context->state.ui_font, begin, end);
	size_y = border_y * 2 + adv_font_sizey(context->state.ui_font);

	pos_x = x - size_x / 2;
	pos_y = y - size_y / 2;

	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, cf);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, cb);

	adv_font_put_string(context->state.ui_font, dst, pos_x + border_x, pos_y + border_y, begin, end, cf, cb);
}

static void ui_menu(struct advance_ui_context* context, adv_bitmap* dst, struct ui_menu_entry* menu_map, unsigned menu_mac, int menu_sel, unsigned cf, unsigned cb)
{
	int step_x, step_y;
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;
	int posr;
	int size_r;
	unsigned i;

	step_x = adv_font_sizex(context->state.ui_font);
	step_y = adv_font_sizey(context->state.ui_font);

	border_x = step_x;
	border_y = step_y / 2;

	/* compute the size of the menu */

	size_x = border_x * 2;
	size_y = border_y * 2;
	size_r = 0;
	for(i=0;i<menu_mac;++i) {
		unsigned width;

		if (size_y + step_y < dst->size_y) {
			size_y += step_y;
			++size_r;
		}

		width = border_x * 2 + adv_font_sizex_string(context->state.ui_font, menu_map[i].text_buffer, menu_map[i].text_buffer + strlen(menu_map[i].text_buffer));
		if (*menu_map[i].option_buffer) {
			width += border_x * 3 + adv_font_sizex_string(context->state.ui_font, menu_map[i].option_buffer, menu_map[i].option_buffer + strlen(menu_map[i].option_buffer));
		}

		if (size_x < width)
			size_x = width;
	}

	if (size_x > dst->size_x)
		size_x = dst->size_x;

	/* position */
	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 2 - size_y / 2;

	/* position in the menu */
	if (size_r < menu_mac) {
		posr = menu_sel - size_r / 2;
		if (posr < 0)
			posr = 0;
		if (posr + size_r > menu_mac)
			posr = menu_mac - size_r;
	} else {
		posr = 0;
	}

	/* put */
	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, cf);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, cb);

	for(i=0;i<size_r;++i) {
		struct ui_menu_entry* e = &menu_map[i + posr];
		unsigned cef;
		unsigned ceb;
		unsigned width = size_x - 2 * border_x;
		unsigned y = pos_y + border_y + step_y * i;
		
		if (i + posr == menu_sel) {
			cef = cb;
			ceb = cf;
		} else {
			cef = cf;
			ceb = cb;
		}

		adv_bitmap_clear(dst, pos_x + border_x / 2, y, size_x - border_x, step_y, ceb);

		if (e->option_buffer[0]) {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_left(context, dst, pos_x + border_x, y, begin, end, cef, ceb);

			width -= adv_font_sizex_string(context->state.ui_font, begin, end);

			begin = e->option_buffer;
			end = e->option_buffer + strlen(e->option_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);
			
			ui_text_right(context, dst, pos_x + size_x - border_x, y, begin, end, cef, ceb);
		} else {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);
			
			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_center(context, dst, pos_x + size_x / 2, y, begin, end, cef, ceb);
		}
	}
}

static void ui_scroll(struct advance_ui_context* context, adv_bitmap* dst, const char* begin, const char* end, unsigned pos, unsigned cf, unsigned cb)
{
	unsigned size_r;
	unsigned sizew;
	int step_x, step_y;
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;
	const char* i;
	const char* start;
	unsigned n;

	step_x = adv_font_sizex(context->state.ui_font);
	step_y = adv_font_sizey(context->state.ui_font);

	border_x = step_x;
	border_y = step_y / 2;

	/* count size_r and compute width */
	size_r = 0;
	size_x = 0;
	i = begin;
	start = begin;
	while (i != end) {
		unsigned width;
		const char* j = i;
		while (j != end && *j != '\n')
			++j;

		if (size_r == pos)
			start = i;

		++size_r;
		width = adv_font_sizex_string(context->state.ui_font, i, j);
		if (width > size_x)
			size_x = width;

		if (j != end)
			i = j + 1;
		else
			i = j;
	}

	/* size */
	size_x += 2 * border_x;
	if (size_x > dst->size_x)
		size_x = dst->size_x;
	sizew = (dst->size_y - 2 * border_y) / step_y;
	if (sizew > size_r)
		sizew = size_r;
	size_y = sizew * step_y + 2 * border_y;

	/* position */
	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 2 - size_y / 2;

	/* put */
	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, cf);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, cb);

	n = 0;
	i = start;
	while (i != end && n < sizew) {
		const char* j = i;
		const char* je;
		while (j != end && *j != '\n')
			++j;

		je = adv_font_sizex_limit(context->state.ui_font, i, j, size_x - 2 * border_x);

		ui_text_left(context, dst, pos_x + border_x, pos_y + border_y + n * step_y, i, je, cf, cb);

		if (j != end)
			i = j + 1;
		else
			i = j;
		++n;
	}
}

/**************************************************************************/
/* Commands */

/**
 * Display a menu.
 */
void advance_ui_menu(struct advance_ui_context* context, struct ui_menu_entry* menu_map, unsigned menu_mac, unsigned menu_sel)
{
	context->state.ui_menu_flag = 1;
	free(context->state.ui_menu_map);
	context->state.ui_menu_map = menu_map;
	context->state.ui_menu_mac = menu_mac;
	context->state.ui_menu_sel = menu_sel;
}

void advance_ui_menu_vect(struct advance_ui_context* context, const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem)
{
	unsigned menu_mac;
	struct ui_menu_entry* menu_map;
	unsigned menu_sel;
	unsigned i;

	/* count elements */
	menu_mac = 0;
	while (items[menu_mac])
		++menu_mac;

	menu_map = malloc(menu_mac * sizeof(struct ui_menu_entry));

	for(i=0;i<menu_mac;++i) {
		sncpy(menu_map[i].text_buffer, sizeof(menu_map[i].text_buffer), items[i]);
		if (subitems && subitems[i])
			sncpy(menu_map[i].option_buffer, sizeof(menu_map[i].option_buffer), subitems[i]);
		else
			menu_map[i].option_buffer[0] = 0;
		if (flag && flag[i])
			menu_map[i].flag = flag[i] != 0;
		else
			menu_map[i].flag = 0;
		if (i == selected) {
			menu_map[i].arrow_left_flag = (arrowize_subitem & 1) != 0;
			menu_map[i].arrow_right_flag = (arrowize_subitem & 2) != 0 ;
		} else {
			menu_map[i].arrow_left_flag = 0;
			menu_map[i].arrow_right_flag = 0;
		}
	}

	if (selected >= 0 && selected < menu_mac)
		menu_sel = selected;
	else
		menu_sel = menu_mac;

	advance_ui_menu(context, menu_map, menu_mac, menu_sel);
}

/**
 * Display a message.
 * This function cannot be called from the video thread.
 */
void advance_ui_message(struct advance_ui_context* context, const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	advance_ui_message_va(context, text, arg);
	va_end(arg);
}

/**
 * Display a message.
 * This function cannot be called from the video thread.
 */
void advance_ui_message_va(struct advance_ui_context* context, const char* text, va_list arg)
{
	context->state.ui_message_flag = 1;
	context->state.ui_message_stop_time = advance_timer() + 2; /* 2 seconds */
	vsnprintf(context->state.ui_message_buffer, sizeof(context->state.ui_message_buffer), text, arg);
}

void advance_ui_direct(struct advance_ui_context* context, const char* text)
{
	context->state.ui_direct_flag = 1;
	sncpy(context->state.ui_direct_buffer, sizeof(context->state.ui_direct_buffer), text);
}

void advance_ui_help(struct advance_ui_context* context)
{
	context->state.ui_help_flag = !context->state.ui_help_flag;
	if (!context->state.help_image)
		context->state.ui_help_flag = 0;
}

void advance_ui_osd(struct advance_ui_context* context, const char* text, int val, int def, int min, int max)
{
	context->state.ui_osd_flag = 1;
	context->state.ui_osd_value = val;
	context->state.ui_osd_def = val;
	context->state.ui_osd_min = min;
	context->state.ui_osd_max = max;
	sncpy(context->state.ui_osd_buffer, sizeof(context->state.ui_osd_buffer), text);
}

void advance_ui_scroll(struct advance_ui_context* context, const char* begin, const char* end, int* scroll)
{
	int pos;
	unsigned size_r;
	const char* i;

	/* count size_r */
	size_r = 0;
	i = begin;
	while (i != end) {
		const char* j = i;
		while (j != end && *j != '\n')
			++j;
		++size_r;
		if (j != end)
			i = j + 1;
		else
			i = j;
	}

	if (scroll) {
		pos = *scroll;
		if (pos >= size_r)
			pos = size_r - 1;
		if (pos < 0)
			pos = 0;
		*scroll = pos;
	} else {
		pos = 0;
	}

	context->state.ui_scroll_flag = 1;
	free(context->state.ui_scroll_begin);
	context->state.ui_scroll_begin = malloc(end - begin);
	memcpy(context->state.ui_scroll_begin, begin, end - begin);
	context->state.ui_scroll_end = context->state.ui_scroll_begin + (end - begin);
	context->state.ui_scroll_pos = pos;
}

adv_bool advance_ui_buffer_active(struct advance_ui_context* context)
{
	return context->state.ui_extra_flag
		|| context->state.ui_message_flag
		|| context->state.ui_help_flag
		|| context->state.ui_menu_flag
		|| context->state.ui_osd_flag
		|| context->state.ui_scroll_flag;
}

adv_bool advance_ui_direct_active(struct advance_ui_context* context)
{
	return context->state.ui_direct_flag;
}

/**************************************************************************/
/* Update */

struct ui_colors {
	adv_color_def def;
	unsigned f;
	unsigned b;
	unsigned p1;
	unsigned p2;
	unsigned p3;
	unsigned p4;
	unsigned u;
};

static void ui_message_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	if (context->state.ui_message_stop_time < advance_timer()) {
		context->state.ui_message_flag = 0;
	}

	ui_messagebox_center(context, dst, dst->size_x / 2, dst->size_x / 2, context->state.ui_message_buffer, context->state.ui_message_buffer + strlen(context->state.ui_message_buffer), color->f, color->b);
}

#define UI_MAP_MAX 256

static void ui_help_update_key(adv_bitmap* dst, adv_bitmap* src, int x, int y, int kx, int ky, int kdx, int kdy, unsigned cf, unsigned cb, unsigned pb)
{
	unsigned cy, cx;

	for(cy=0;cy<kdy;++cy) {
		for(cx=0;cx<kdx;++cx) {
			unsigned c;
			if ((adv_bitmap_pixel_get(src, kx + cx, ky + cy)) != pb) {
				c = cf;
			} else {
				c = cb;
			}
			adv_bitmap_pixel_put(dst, x + kx + cx, y + ky + cy, c);
		}
	}
}

static void ui_help_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	int size_x;
	int size_y;
	int pos_x;
	int pos_y;
	unsigned cx;
	unsigned cy;
	unsigned i;
	unsigned pb;
	char msg_buffer[256];

	struct mame_digital_map_entry digital_map[UI_MAP_MAX];
	unsigned digital_mac;

	mame_ui_input_map(&digital_mac, digital_map, UI_MAP_MAX);

	size_x = context->state.help_image->size_x;
	size_y = context->state.help_image->size_y;

	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 8;

	pb = 0; /* black on RGB format */

	for(cy=0;cy<context->state.help_image->size_y;++cy) {
		for(cx=0;cx<context->state.help_image->size_x;++cx) {
			unsigned c;
			if ((adv_bitmap_pixel_get(context->state.help_image, cx, cy)) != pb) {
				c = color->f;
			} else {
				c = color->b;
			}
			adv_bitmap_pixel_put(dst, pos_x + cx, pos_y + cy, c);
		}
	}

	msg_buffer[0] = 0;

	for(i=0;i<digital_mac;++i) {
		unsigned j;
		adv_bool pred_not = 0;

		if (digital_map[i].port_state) {
			struct mame_port* p;
			p = mame_port_find(digital_map[i].port);
			if (p) {
				unsigned k;

				/* add the port name only one time */
				/* if the port list is broken, a port name may appers more than one time */

				for(k=0;k<i;++k)
					if (digital_map[k].port == digital_map[i].port)
						break;

				if (k == i) {
					if (msg_buffer[0])
						sncat(msg_buffer, sizeof(msg_buffer), ", ");
					sncat(msg_buffer, sizeof(msg_buffer), p->desc);
				}
			}
		}

		for(j=0;j<MAME_INPUT_MAP_MAX && digital_map[i].seq[j] != DIGITAL_SPECIAL_NONE;++j) {
			if (!pred_not) {
				unsigned k;
				unsigned ckf;
				unsigned ckb;

				switch (mame_port_player(digital_map[i].port)) {
				case 1 : ckb = color->p1; break;
				case 2 : ckb = color->p2; break;
				case 3 : ckb = color->p3; break;
				case 4 : ckb = color->p4; break;
				default : ckb = color->u; break;
				}
				ckf = color->f;

				if (digital_map[i].port_state) {
					ckf = color->f;
					ckb = color->f;
				}
			
				for(k=0;k<context->config.help_mac;++k) {
					if (context->config.help_map[k].code == digital_map[i].seq[j]) {
						struct help_entry* h = context->config.help_map + k;
						ui_help_update_key(dst, context->state.help_image, pos_x, pos_y, h->x, h->y, h->dx, h->dy, ckf, ckb, pb);
					}
				}
			}

			pred_not = digital_map[i].seq[j] == DIGITAL_SPECIAL_NOT;
		}
	}

	if (msg_buffer[0])
		ui_messagebox_center(context, dst, dst->size_x / 2, pos_y + size_y + adv_font_sizey(context->state.ui_font) * 2, msg_buffer, msg_buffer + strlen(msg_buffer), color->f, color->b);
}

static void ui_menu_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	ui_menu(context, dst, context->state.ui_menu_map, context->state.ui_menu_mac, context->state.ui_menu_sel, color->f, color->b);

	free(context->state.ui_menu_map);
	context->state.ui_menu_map = 0;
	context->state.ui_menu_flag = 0;
}

static void ui_osd_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	unsigned pos_x, pos_y;

	pos_x = dst->size_x / 2;
	pos_y = dst->size_y * 7 / 8;

	ui_messagebox_center(context, dst, pos_x, pos_y, context->state.ui_osd_buffer, context->state.ui_osd_buffer + strlen(context->state.ui_osd_buffer), color->f, color->b);

	context->state.ui_osd_flag = 0;
}

static void ui_scroll_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	ui_scroll(context, dst, context->state.ui_scroll_begin, context->state.ui_scroll_end, context->state.ui_scroll_pos, color->f, color->b);

	context->state.ui_scroll_flag = 0;
}

static void ui_direct_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_colors* color)
{
	const char* begin = context->state.ui_direct_buffer;
	const char* end = context->state.ui_direct_buffer + strlen(context->state.ui_direct_buffer);
	int pos_x, pos_y;
	int size_x, size_y;
	unsigned orientation;

	orientation = context->config.ui_font_orientation;

	if ((orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
		end = adv_font_sizey_limit(context->state.ui_font_oriented, begin, end, dst->size_y);
		size_x = adv_font_sizex(context->state.ui_font_oriented);
		size_y = adv_font_sizey_string(context->state.ui_font_oriented, begin, end);

		pos_x = 0;
		pos_y = dst->size_y - size_y;
	} else {
		end = adv_font_sizex_limit(context->state.ui_font_oriented, begin, end, dst->size_x);
		size_x = adv_font_sizex_string(context->state.ui_font_oriented, begin, end);
		size_y = adv_font_sizey(context->state.ui_font_oriented);

		pos_x = dst->size_x - size_x;
		pos_y = 0;
	}

	if ((orientation & OSD_ORIENTATION_FLIP_X) != 0) {
		pos_x = dst->size_x - 1 - pos_x;
	}
	if ((orientation & OSD_ORIENTATION_FLIP_Y) != 0) {
		pos_y = dst->size_y - 1 - pos_y;
	}

	adv_font_put_string_oriented(context->state.ui_font_oriented, dst, pos_x, pos_y, begin, end, color->f, color->b, context->config.ui_font_orientation);

	context->state.ui_direct_flag = 0;
}

static void ui_setup_color(struct ui_colors* color, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	color->def = color_def;
	switch (color_def_type_get(color_def)) {
	case adv_color_type_rgb :
	case adv_color_type_yuy2 :
		color->b = pixel_make_from_def(0xff, 0xff, 0xff, color_def);
		color->f = pixel_make_from_def(0x00, 0x00, 0x00, color_def);
		color->p1 = pixel_make_from_def(0xff, 0xff, 0x00, color_def);
		color->p2 = pixel_make_from_def(0x00, 0xff, 0x00, color_def);
		color->p3 = pixel_make_from_def(0xff, 0x00, 0x00, color_def);
		color->p4 = pixel_make_from_def(0x00, 0xff, 0xff, color_def);
		color->u = pixel_make_from_def(0x80, 0x80, 0x80, color_def);
		break;
	case adv_color_type_palette :
		color->b = video_color_find(0xff, 0xff, 0xff, palette_map, palette_max);
		color->f = video_color_find(0x00, 0x00, 0x00, palette_map, palette_max);
		color->p1 = video_color_find(0xff, 0xff, 0x00, palette_map, palette_max);
		color->p2 = video_color_find(0x00, 0xff, 0x00, palette_map, palette_max);
		color->p3 = video_color_find(0xff, 0x00, 0x00, palette_map, palette_max);
		color->p4 = video_color_find(0x00, 0xff, 0xff, palette_map, palette_max);
		color->u = video_color_find(0x80, 0x80, 0x80, palette_map, palette_max);
		break;
	default:
		color->b = 0xffffffff;
		color->f = 0x0;
		color->p1 = color->b;
		color->p2 = color->b;
		color->p3 = color->b;
		color->p4 = color->b;
		color->u = color->b;
		break;
	}
}

void advance_ui_buffer_update(struct advance_ui_context* context, void* ptr, unsigned dx, unsigned dy, unsigned dw, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	adv_bitmap* dst;
	struct ui_colors color;

	ui_setup_color(&color, color_def, palette_map, palette_max);

	dst = adv_bitmap_import_rgb(dx, dy, color_def_bytes_per_pixel_get(color_def), 0, 0, ptr, dw);

	context->state.ui_extra_flag = 0;

	if (context->state.ui_help_flag) {
		ui_help_update(context, dst, &color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_menu_flag) {
		ui_menu_update(context, dst, &color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_message_flag) {
		ui_message_update(context, dst, &color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_osd_flag) {
		ui_osd_update(context, dst, &color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_scroll_flag) {
		ui_scroll_update(context, dst, &color);
		context->state.ui_extra_flag = 1;
	}

	adv_bitmap_free(dst);
}

void advance_ui_direct_update(struct advance_ui_context* context, void* ptr, unsigned dx, unsigned dy, unsigned dw, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	adv_bitmap* dst;
	struct ui_colors color;

	ui_setup_color(&color, color_def, palette_map, palette_max);

	dst = adv_bitmap_import_rgb(dx, dy, color_def_bytes_per_pixel_get(color_def), 0, 0, ptr, dw);

	if (context->state.ui_direct_flag) {
		ui_direct_update(context, dst, &color);
	}

	adv_bitmap_free(dst);
}

/**************************************************************************/
/* Interface */

adv_error advance_ui_init(struct advance_ui_context* context, adv_conf* cfg_context)
{
	context->state.ui_extra_flag = 0;
	context->state.ui_message_flag = 0;
	context->state.ui_help_flag = 0;
	context->state.ui_menu_map = 0;
	context->state.ui_osd_flag = 0;
	context->state.ui_scroll_flag = 0;
	context->state.ui_scroll_begin = 0;
	context->state.ui_scroll_end = 0;
	context->state.ui_direct_flag = 0;
	context->state.ui_font = 0;
	context->state.ui_font_oriented = 0;

	conf_string_register_multi(cfg_context, "ui_helptag");
	conf_string_register_default(cfg_context, "ui_helpimage", "auto");
	conf_string_register_default(cfg_context, "ui_font", "auto");

	return 0;
}

adv_error advance_ui_config_load(struct advance_ui_context* context, adv_conf* cfg_context, struct mame_option* option)
{
	sncpy(context->config.help_image_buffer, sizeof(context->config.help_image_buffer), conf_string_get_default(cfg_context, "ui_helpimage"));
	sncpy(context->config.ui_font_buffer, sizeof(context->config.ui_font_buffer), conf_string_get_default(cfg_context, "ui_font"));
	context->config.ui_font_orientation = option->direct_orientation;

	return 0;
}

void advance_ui_done(struct advance_ui_context* context)
{
}

#include "help.dat"

adv_error advance_ui_inner_init(struct advance_ui_context* context, adv_conf* cfg_context)
{
	adv_conf_iterator k;
	adv_color_def def;
	adv_bitmap* bitmap;

	if (strcmp(context->config.ui_font_buffer, "auto") == 0) {
		context->state.ui_font = adv_font_default(13);
		context->state.ui_font_oriented = adv_font_default(13);
	} else {
		adv_fz* f;
		const char* file = file_config_file_home(context->config.ui_font_buffer);

		log_std(("emu:ui: font '%s'\n", file));

		f = fzopen(file, "rb");
		if (!f) {
			target_err("Error opening the font %s\n", file);
			return -1;
		}

		context->state.ui_font = adv_font_load(f);
		if (!context->state.ui_font) {
			target_err("Error reading the font %s\n%s\n", file, error_get());
			return -1;
		}

		if (fzseek(f, 0, SEEK_SET) != 0) {
			target_err("Error reading the font %s\n%s\n", file, error_get());
			return -1;
		}

		context->state.ui_font_oriented = adv_font_load(f);
		if (!context->state.ui_font_oriented) {
			target_err("Error reading the font %s\n%s\n", file, error_get());
			return -1;
		}

		fzclose(f);
	}

	bitmap = adv_bitmap_alloc( adv_font_sizex_char(context->state.ui_font, '1'), adv_font_sizey_char(context->state.ui_font, '1'), 8);
	adv_bitmap_clear(bitmap, 0, 0, bitmap->size_x, bitmap->size_y, 0);
	adv_font_set_char(context->state.ui_font, 255, adv_bitmap_dup(bitmap));
	adv_font_set_char(context->state.ui_font_oriented, 255, adv_bitmap_dup(bitmap));
	adv_bitmap_free(bitmap);

	adv_font_orientation(context->state.ui_font_oriented, context->config.ui_font_orientation);

	def = color_def_make_rgb_from_sizelenpos(3, 8, 0, 8, 8, 8, 16);

	if (strcmp(context->config.help_image_buffer, "auto") == 0) {
		adv_fz* f;
		unsigned i;

		log_std(("emu:ui: helpimage auto\n"));

		f = fzopenmemory(HELPIMAGE, HELPIMAGE_SIZE);

		context->state.help_image = adv_bitmap_load_png_rgb(f, def);
		if (!context->state.help_image) {
			target_err("Error reading the internal help image\n");
			return -1;
		}

		fzclose(f);

		log_std(("emu:ui: helptag auto\n"));
		i = 0;
		while (HELPTAG[i]) {
			char* d = strdup(HELPTAG[i]);

			log_std(("emu:ui: helptag '%s'\n", d));

			if (advance_ui_parse_help(context, d) != 0) {
				free(d);
				target_err("Invalid 'ui_helptag' option.\n%s\n", error_get());
				return -1;
			}

			free(d);

			++i;
		}

	} else {
		if (strcmp(context->config.help_image_buffer, "none") != 0) {
			adv_fz* f;
			const char* file = file_config_file_home(context->config.help_image_buffer);

			log_std(("emu:ui: helpimage '%s'\n", file));

			f = fzopen(file, "rb");
			if (!f) {
				target_err("Error opening the help image %s\n", file);
				return -1;
			}
			
			context->state.help_image = adv_bitmap_load_png_rgb(f, def);
			if (!context->state.help_image) {
				target_err("Error reading the help image %s\n%s\n", file, error_get());
				return -1;
			}
			
			fzclose(f);
		} else {
			context->state.help_image = 0;
		}

		log_std(("emu:ui: helptag start\n"));
		for (conf_iterator_begin(&k, cfg_context, "ui_helptag");!conf_iterator_is_end(&k);conf_iterator_next(&k)) {
			char* d = strdup(conf_iterator_string_get(&k));

			log_std(("emu:ui: helptag '%s'\n", d));

			if (advance_ui_parse_help(context, d) != 0) {
				free(d);
				target_err("Invalid 'ui_helptag' option.\n%s\n", error_get());
				return -1;
			}

			free(d);
		}
	}

	return 0;
}

void advance_ui_inner_done(struct advance_ui_context* context)
{
	adv_font_free(context->state.ui_font);
	context->state.ui_font = 0;
	if (context->state.help_image) {
		adv_bitmap_free(context->state.help_image);
		context->state.help_image = 0;
	}
}

/**************************************************************************/
/* OSD interface */

void osd_ui_message(const char* s, int second)
{
	struct advance_ui_context* context = &CONTEXT.ui;

	advance_ui_message(context, "%s", s);
}

void osd_ui_menu(const char** items,const char** subitems, char* flag, int selected, int arrowize_subitem)
{
	unsigned menu_mac;
	struct ui_menu_entry* menu_map;
	unsigned menu_sel;
	unsigned i;

	struct advance_ui_context* context = &CONTEXT.ui;

	/* count elements */
	menu_mac = 0;
	while (items[menu_mac])
		++menu_mac;

	menu_map = malloc(menu_mac * sizeof(struct ui_menu_entry));

	for(i=0;i<menu_mac;++i) {
		sncpy(menu_map[i].text_buffer, sizeof(menu_map[i].text_buffer), items[i]);
		if (subitems && subitems[i])
			sncpy(menu_map[i].option_buffer, sizeof(menu_map[i].option_buffer), subitems[i]);
		else
			menu_map[i].option_buffer[0] = 0;
		if (flag && flag[i])
			menu_map[i].flag = flag[i] != 0;
		else
			menu_map[i].flag = 0;
		if (i == selected) {
			menu_map[i].arrow_left_flag = (arrowize_subitem & 1) != 0;
			menu_map[i].arrow_right_flag = (arrowize_subitem & 2) != 0 ;
		} else {
			menu_map[i].arrow_left_flag = 0;
			menu_map[i].arrow_right_flag = 0;
		}
	}

	if (selected >= 0 && selected < menu_mac)
		menu_sel = selected;
	else
		menu_sel = menu_mac;

	advance_ui_menu(context, menu_map, menu_mac, menu_sel);
}

void osd_ui_osd(const char *text, int percentage, int default_percentage)
{
	struct advance_ui_context* context = &CONTEXT.ui;

	advance_ui_osd(context, text, percentage, default_percentage, 0, 100);
}

void osd_ui_scroll(const char* text, int* pos)
{
	struct advance_ui_context* context = &CONTEXT.ui;

	advance_ui_scroll(context, text, text + strlen(text), pos);
}
