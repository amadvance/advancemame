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

#include "portable.h"

#include "emu.h"
#include "thread.h"
#include "input.h"

#include "glueint.h"

#include "advance.h"

/**************************************************************************/
/* Internal */

static inline adv_bool ui_alpha(adv_color_def color_def)
{
	return color_def_type_get(color_def) == adv_color_type_rgb;
}

static void ui_text_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_pixel* map, adv_color_def def)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x / 2;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_map(context->state.ui_font, dst, pos_x, pos_y, begin, end, map);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_text_left(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_pixel* map, adv_color_def def)
{
	int pos_x, pos_y;

	pos_x = x;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_map(context->state.ui_font, dst, pos_x, pos_y, begin, end, map);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_text_right(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_pixel* map, adv_color_def def)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_map(context->state.ui_font, dst, pos_x, pos_y, begin, end, map);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_dft(struct advance_ui_context* context, adv_bitmap* dst, unsigned x, unsigned y, unsigned dx, unsigned dy, double* m, unsigned n, double cut1, double cut2, struct ui_color cf, struct ui_color cb, struct ui_color ct, adv_color_def def)
{
	unsigned i;
	char buffer[32];
	const char* begin;
	const char* end;
	int xv1, xv2;

	adv_bitmap_box(dst, x, y, dx, dy, 1, ct.f);

	++x;
	++y;
	dx -= 2;
	dy -= 2;

	adv_bitmap_clear(dst, x, y, dx, dy, cf.b);

	for(i=0;i<dx;++i) {
		int vi;
		double v;

		v = m[i * (n/2) /(dx-1)];

		v /= 10;

		if (v < 1E-4)
			v = 1E-4;
		if (v > 1)
			v = 1;

		v = 20.0 * log10( v );

		vi = v * dy / -80.0;
		if (vi < 0)
			vi = 0;
		if (vi > dy)
			vi = dy;

		adv_bitmap_clear(dst, x + i, y + vi, 1, dy - vi, cb.b);
	}

	xv1 = x+cut1*2*dx;
	xv2 = x+cut2*2*dx;

	adv_bitmap_clear(dst, xv1, y, 1, dy, ct.f);
	adv_bitmap_clear(dst, xv2, y, 1, dy, ct.f);

	adv_bitmap_clear(dst, x, y+dy/4, dx, 1, ct.f);
	adv_bitmap_clear(dst, x, y+dy/2, dx, 1, ct.f);
	adv_bitmap_clear(dst, x, y+dy*3/4, dx, 1, ct.f);
}

static void ui_menu(struct advance_ui_context* context, adv_bitmap* dst, struct ui_menu_entry* menu_map, unsigned menu_mac, int menu_sel, struct ui_color entry_f, struct ui_color entry_b, adv_pixel* entry_map, struct ui_color select_f, struct ui_color select_b, adv_pixel* select_map, struct ui_color title_f, struct ui_color title_b, adv_pixel* title_map, adv_color_def def)
{
	int step_x, step_y;
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;
	int pos_r;
	int pos_s;
	int size_r;
	unsigned i;
	unsigned y;
	unsigned dft_dx;
	unsigned dft_dy;
	int up;
	int down;
	adv_bitmap* flat;

	step_x = adv_font_sizex(context->state.ui_font);
	step_y = adv_font_sizey(context->state.ui_font);

	dft_dx = dst->size_x * 3 / 4;
	dft_dy = dst->size_y * 2 / 5;

	border_x = step_x;
	border_y = step_y / 2;

	/* compute the size of the menu */

	size_x = border_x * 2;
	size_y = border_y * 2;

	/* search the selected position */
	pos_r = 0;
	pos_s = 0;
	for(i=0;i<menu_mac;++i) {
		switch (menu_map[i].type) {
		case ui_menu_text :
		case ui_menu_option :
			if (menu_sel == pos_r) {
				pos_s = i;
			}
			++pos_r;
			break;
		default:
			break;
		}
	}

	up = 0;
	down = 0;
	size_r = 0;
	pos_r = pos_s;

	while (1) {
		unsigned width;
		unsigned height;

		if (size_r == 0) {
			i = pos_r;
		} else if (up <= down && pos_r > 0) {
			i = pos_r - 1;
		} else if (up > down && pos_r + size_r < menu_mac) {
			i = pos_r + size_r;
		} else if (pos_r > 0) {
			i = pos_r - 1;
		} else if (pos_r + size_r < menu_mac) {
			i = pos_r + size_r;
		} else {
			break;
		}

		switch (menu_map[i].type) {
		case ui_menu_title :
			width = border_x * 2 + adv_font_sizex_string(context->state.ui_font, menu_map[i].text_buffer, menu_map[i].text_buffer + strlen(menu_map[i].text_buffer));
			height = step_y;
			break;
		case ui_menu_text :
			width = border_x * 2 + adv_font_sizex_string(context->state.ui_font, menu_map[i].text_buffer, menu_map[i].text_buffer + strlen(menu_map[i].text_buffer));
			height = step_y;
			break;
		case ui_menu_option :
			width = border_x * 2 + adv_font_sizex_string(context->state.ui_font, menu_map[i].text_buffer, menu_map[i].text_buffer + strlen(menu_map[i].text_buffer));
			width += border_x * 3 + adv_font_sizex_string(context->state.ui_font, menu_map[i].option_buffer, menu_map[i].option_buffer + strlen(menu_map[i].option_buffer));
			height = step_y;
			break;
		case ui_menu_dft :
			width = dft_dx;
			height = dft_dy;
			break;
		default:
			width = 0;
			height = 0;
			break;
		}

		if (size_y + height > dst->size_y)
			break;

		size_y += height;

		if (size_x < width)
			size_x = width;

		if (i == pos_r) {
			++size_r;
		} else if (i < pos_r) {
			up += height;
			--pos_r;
			++size_r;
		} else if (i > pos_r) {
			down += height;
			++size_r;
		}
	}

	if (size_x > dst->size_x)
		size_x = dst->size_x;

	/* position */
	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 2 - size_y / 2;

	if (ui_alpha(def))
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(context->state.buffer_def));
	else
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(def));

	/* put */
	adv_bitmap_box(flat, 0, 0, size_x, size_y, 1, entry_f.f);
	adv_bitmap_clear(flat, 1, 1, size_x - 2, size_y - 2, entry_b.b);

	y = border_y;

	for(i=0;i<size_r;++i) {
		struct ui_menu_entry* e = &menu_map[i + pos_r];
		struct ui_color cef;
		struct ui_color ceb;
		struct ui_color ctf;
		struct ui_color ctb;
		adv_pixel* cem;
		adv_pixel* ctm;

		unsigned height = e->type == ui_menu_dft ? dft_dy : step_y;
		unsigned width = size_x - 2 * border_x;

		if (i + pos_r == pos_s) {
			cef = select_f;
			ceb = select_b;
			cem = select_map;
			ctf = select_f;
			ctb = select_b;
			ctm = select_map;
		} else {
			cef = entry_f;
			ceb = entry_b;
			cem = entry_map;
			ctf = title_f;
			ctb = title_b;
			ctm = title_map;
		}

		adv_bitmap_clear(flat, border_x/2, y, size_x - (border_x/2)*2, height, ceb.b);

		if (e->type == ui_menu_option) {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_left(context, flat, border_x, y, begin, end, cef, ceb, cem, def);

			width -= adv_font_sizex_string(context->state.ui_font, begin, end);

			begin = e->option_buffer;
			end = e->option_buffer + strlen(e->option_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_right(context, flat, size_x - border_x, y, begin, end, ctf, ctb, ctm, def);
		} else if (e->type == ui_menu_title || e->type == ui_menu_text) {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);

			adv_bool title = e->type == ui_menu_title;

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_center(context, flat, size_x / 2, y, begin, end, title ? ctf : cef, title ? ctb : ceb, title ? ctm : cem, def);
		} else if (e->type == ui_menu_dft) {
			ui_dft(context, flat, border_x, y + border_y, size_x - border_x * 2, height - border_y * 2, e->m, e->n, e->cut1, e->cut2, cef, ceb, ctf, def);
		}

		y += height;
	}

	if (ui_alpha(def))
		adv_bitmap_put_alpha(dst, pos_x, pos_y, def, flat, 0, 0, flat->size_x, flat->size_y, context->state.buffer_def);
	else
		adv_bitmap_put(dst, pos_x, pos_y, flat, 0, 0, flat->size_x, flat->size_y);

	adv_bitmap_free(flat);
}

static adv_bool ui_recognize_title(const char* begin, const char* end)
{
	const char* i;

	while (begin != end && isspace(end[-1]))
		--end;

	if (begin == end)
		return 0;

	if (end[-1] == ':' && isupper(begin[0]))
		return 1;

	if (begin[0] == '-' && end[-1] == '-')
		return 1;

	for(i=begin;i!=end;++i)
		if (islower(*i))
			break;
	if (i == end && end[-1] != '.' && isupper(begin[0]))
		return 1;

	return 0;
}

static void ui_scroll(struct advance_ui_context* context, adv_bitmap* dst, char* begin, char* end, unsigned pos, struct ui_color text_f, struct ui_color text_b, adv_pixel* text_map, struct ui_color title_f, struct ui_color title_b, adv_pixel* title_map, adv_color_def def)
{
	unsigned size_r;
	unsigned size_v;
	int step_x, step_y;
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;
	int limit_x;
	char* i;
	char* start;
	unsigned n;
	adv_bitmap* flat;

	step_x = adv_font_sizex(context->state.ui_font);
	step_y = adv_font_sizey(context->state.ui_font);

	border_x = step_x;
	border_y = step_y / 2;

	/* remove unprintable chars */
	i = begin;
	while (i != end) {
		if (!((*i >= ' ' && *i <='~') || *i =='\n'))
			*i = ' ';
		++i;
	}

	/* count size_r and compute width and wrap long line */
	limit_x = dst->size_x - 6 * border_x;
	size_r = 0;
	size_x = 0;
	i = begin;
	start = begin;
	while (i != end) {
		unsigned width;
		char* j = i;

		width = 0;
		while (j != end && *j != '\n') {
			unsigned char_width;

			char_width = adv_font_sizex_char(context->state.ui_font, *j);
			if (width + char_width > limit_x) {
				char* b = j;
				/* search first space backward */
				while (b != i && !isspace(*b))
					--b;
				if (b != i) {
					/* adjust the position */
					if (pos > size_r)
						++pos;
					/* insert a break */
					*b = '\n';
					j = b;
					break;
				}
			} else {
				width += char_width;
			}
			++j;
		}

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
	size_v = (dst->size_y - 4 * border_y) / step_y;
	if (size_v > size_r)
		size_v = size_r;
	size_y = size_v * step_y + 2 * border_y;

	/* position */
	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 2 - size_y / 2;

	if (ui_alpha(def))
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(context->state.buffer_def));
	else
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(def));

	/* put */
	adv_bitmap_box(flat, 0, 0, size_x, size_y, 1, text_f.f);
	adv_bitmap_clear(flat, 1, 1, size_x - 2, size_y - 2, text_b.b);

	n = 0;
	i = start;
	while (i != end && n < size_v) {
		struct ui_color f;
		struct ui_color b;
		adv_pixel* m;
		char* k;
		char* j = i;
		const char* je;
		while (j != end && *j != '\n')
			++j;

		je = adv_font_sizex_limit(context->state.ui_font, i, j, size_x - 2 * border_x);

		for(k=i;k!=j;++k) {
			if (islower(*k))
				break;
		}
		if (ui_recognize_title(i, j)) {
			f = title_f;
			b = title_b;
			m = title_map;
		} else {
			f = text_f;
			b = text_b;
			m = text_map;
		}

		ui_text_left(context, flat, border_x, border_y + n * step_y, i, je, f, b, m, def);

		if (j != end)
			i = j + 1;
		else
			i = j;
		++n;
	}

	if (ui_alpha(def))
		adv_bitmap_put_alpha(dst, pos_x, pos_y, def, flat, 0, 0, flat->size_x, flat->size_y, context->state.buffer_def);
	else
		adv_bitmap_put(dst, pos_x, pos_y, flat, 0, 0, flat->size_x, flat->size_y);

	adv_bitmap_free(flat);
}

static void ui_messagebox_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_pixel* map, adv_color_def def)
{
	int border_x, border_y;
	int size_x, size_y;
	int pos_x, pos_y;
	adv_bitmap* flat;

	border_x = adv_font_sizex(context->state.ui_font);
	border_y = adv_font_sizey(context->state.ui_font) / 2;

	end = adv_font_sizex_limit(context->state.ui_font, begin, end, dst->size_x - border_x * 2);

	size_x = border_x * 2 + adv_font_sizex_string(context->state.ui_font, begin, end);
	size_y = border_y * 2 + adv_font_sizey(context->state.ui_font);

	pos_x = x - size_x / 2;
	pos_y = y - size_y / 2;

	if (ui_alpha(def))
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(context->state.buffer_def));
	else
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(def));

	adv_bitmap_box(flat, 0, 0, size_x, size_y, 1, cf.f);
	adv_bitmap_clear(flat, 1, 1, size_x - 2, size_y - 2, cb.b);

	if (ui_alpha(def))
		adv_font_put_string_map(context->state.ui_font, flat, border_x, border_y, begin, end, map);
	else
		adv_font_put_string(context->state.ui_font, flat, border_x, border_y, begin, end, cf.p, cb.p);

	if (ui_alpha(def))
		adv_bitmap_put_alpha(dst, pos_x, pos_y, def, flat, 0, 0, flat->size_x, flat->size_y, context->state.buffer_def);
	else
		adv_bitmap_put(dst, pos_x, pos_y, flat, 0, 0, flat->size_x, flat->size_y);

	adv_bitmap_free(flat);
}

/**************************************************************************/
/* Commands */

void advance_ui_menu_init(struct ui_menu* menu)
{
	menu->mac = 0;
	menu->max = 256;
	menu->map = (struct ui_menu_entry*)malloc(menu->max * sizeof(struct ui_menu_entry));
}

static unsigned ui_menu_select(struct ui_menu* menu)
{
	unsigned i, count;

	count = 0;
	for(i=0;i<menu->mac;++i) {
		switch (menu->map[i].type) {
		case ui_menu_text :
		case ui_menu_option :
			++count;
			break;
		default:
			break;
		}
	}

	return count;
}

void advance_ui_menu_dft_insert(struct ui_menu* menu, double* m, unsigned n, double cut1, double cut2)
{
	struct ui_menu_entry* entry = &menu->map[menu->mac];

	assert(menu->mac < menu->max);
	if (menu->mac == menu->max)
		return;

	entry->type = ui_menu_dft;
	entry->m = m;
	entry->n = n;
	entry->cut1 = cut1;
	entry->cut2 = cut2;

	++menu->mac;
}

void advance_ui_menu_title_insert(struct ui_menu* menu, const char* title)
{
	struct ui_menu_entry* entry = &menu->map[menu->mac];

	assert(menu->mac < menu->max);
	if (menu->mac == menu->max)
		return;

	entry->type = ui_menu_title;
	sncpy(entry->text_buffer, sizeof(entry->text_buffer), title);

	++menu->mac;
}

unsigned advance_ui_menu_text_insert(struct ui_menu* menu, const char* text)
{
	struct ui_menu_entry* entry = &menu->map[menu->mac];

	assert(menu->mac < menu->max);
	if (menu->mac == menu->max)
		return -1;

	entry->type = ui_menu_text;
	sncpy(entry->text_buffer, sizeof(entry->text_buffer), text);

	++menu->mac;

	return ui_menu_select(menu) - 1;
}

unsigned advance_ui_menu_option_insert(struct ui_menu* menu, const char* text, const char* option)
{
	struct ui_menu_entry* entry = &menu->map[menu->mac];

	assert(menu->mac < menu->max);
	if (menu->mac == menu->max)
		return -1;

	entry->type = ui_menu_option;
	sncpy(entry->text_buffer, sizeof(entry->text_buffer), text);
	sncpy(entry->option_buffer, sizeof(entry->option_buffer), option);
	++menu->mac;

	return ui_menu_select(menu) - 1;
}

/**
 * Display a menu.
 */
unsigned advance_ui_menu_done(struct ui_menu* menu, struct advance_ui_context* context, unsigned menu_sel)
{
	unsigned count, i;

	context->state.ui_menu_flag = 1;
	free(context->state.ui_menu_map);
	context->state.ui_menu_map = menu->map;
	context->state.ui_menu_mac = menu->mac;
	context->state.ui_menu_sel = menu_sel;

	return ui_menu_select(menu);
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

void advance_ui_direct_text(struct advance_ui_context* context, const char* text)
{
	context->state.ui_direct_text_flag = 1;
	sncpy(context->state.ui_direct_buffer, sizeof(context->state.ui_direct_buffer), text);
}

void advance_ui_direct_slow(struct advance_ui_context* context, int flag)
{
	if (context->config.ui_speedmark_flag)
		context->state.ui_direct_slow_flag = flag;
}

void advance_ui_direct_fast(struct advance_ui_context* context, int flag)
{
	if (context->config.ui_speedmark_flag)
		context->state.ui_direct_fast_flag = flag;
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
	return context->state.ui_direct_text_flag
		|| context->state.ui_direct_slow_flag
		|| context->state.ui_direct_fast_flag;
}

/**************************************************************************/
/* Update */

static void ui_message_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	if (context->state.ui_message_stop_time < advance_timer()) {
		context->state.ui_message_flag = 0;
	}

	ui_messagebox_center(context, dst, dst->size_x / 2, dst->size_y / 2, context->state.ui_message_buffer, context->state.ui_message_buffer + strlen(context->state.ui_message_buffer), color->ui_f, color->ui_b, color->ui_alpha, color->def);
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

static void ui_help_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
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
	adv_bitmap* flat;
	adv_color_def def = color->def;

	struct mame_digital_map_entry digital_map[UI_MAP_MAX];
	unsigned digital_mac;

	mame_ui_input_map(&digital_mac, digital_map, UI_MAP_MAX);

	size_x = context->state.help_image->size_x;
	size_y = context->state.help_image->size_y;

	pos_x = dst->size_x / 2 - size_x / 2;
	pos_y = dst->size_y / 8;

	if (ui_alpha(def))
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(context->state.buffer_def));
	else
		flat = adv_bitmap_alloc(size_x, size_y, color_def_bytes_per_pixel_get(def));

	pb = 0; /* black on RGB format */

	for(cy=0;cy<context->state.help_image->size_y;++cy) {
		for(cx=0;cx<context->state.help_image->size_x;++cx) {
			adv_pixel c;
			if ((adv_bitmap_pixel_get(context->state.help_image, cx, cy)) != pb) {
				c = color->ui_f.f;
			} else {
				c = color->ui_b.b;
			}
			adv_bitmap_pixel_put(flat, cx, cy, c);
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
				case 1 : ckb = color->help_p1.b; break;
				case 2 : ckb = color->help_p2.b; break;
				case 3 : ckb = color->help_p3.b; break;
				case 4 : ckb = color->help_p4.b; break;
				default : ckb = color->help_u.b; break;
				}
				ckf = color->ui_f.f;

				if (digital_map[i].port_state) {
					ckf = color->ui_f.f;
					ckb = color->ui_f.b;
				}

				for(k=0;k<context->config.help_mac;++k) {
					if (context->config.help_map[k].code == digital_map[i].seq[j]) {
						struct help_entry* h = context->config.help_map + k;
						ui_help_update_key(flat, context->state.help_image, 0, 0, h->x, h->y, h->dx, h->dy, ckf, ckb, pb);
					}
				}
			}

			pred_not = digital_map[i].seq[j] == DIGITAL_SPECIAL_NOT;
		}
	}

	if (ui_alpha(def))
		adv_bitmap_put_alpha(dst, pos_x, pos_y, def, flat, 0, 0, flat->size_x, flat->size_y, context->state.buffer_def);
	else
		adv_bitmap_put(dst, pos_x, pos_y, flat, 0, 0, flat->size_x, flat->size_y);

	adv_bitmap_free(flat);

	if (msg_buffer[0])
		ui_messagebox_center(context, dst, dst->size_x / 2, pos_y + size_y + adv_font_sizey(context->state.ui_font) * 2, msg_buffer, msg_buffer + strlen(msg_buffer), color->ui_f, color->ui_b, color->ui_alpha, color->def);
}

static void ui_menu_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	ui_menu(context, dst, context->state.ui_menu_map, context->state.ui_menu_mac, context->state.ui_menu_sel, color->ui_f, color->ui_b, color->ui_alpha, color->select_f, color->select_b, color->select_alpha, color->title_f, color->title_b, color->title_alpha, color->def);

	free(context->state.ui_menu_map);
	context->state.ui_menu_map = 0;
	context->state.ui_menu_flag = 0;
}

static void ui_osd_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	unsigned pos_x, pos_y;

	pos_x = dst->size_x / 2;
	pos_y = dst->size_y * 7 / 8;

	ui_messagebox_center(context, dst, pos_x, pos_y, context->state.ui_osd_buffer, context->state.ui_osd_buffer + strlen(context->state.ui_osd_buffer), color->ui_f, color->ui_b, color->ui_alpha, color->def);

	context->state.ui_osd_flag = 0;
}

static void ui_scroll_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	ui_scroll(context, dst, context->state.ui_scroll_begin, context->state.ui_scroll_end, context->state.ui_scroll_pos, color->ui_f, color->ui_b, color->ui_alpha, color->title_f, color->title_b, color->title_alpha, color->def);

	context->state.ui_scroll_flag = 0;
}

static void ui_direct_text_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
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

	adv_font_put_string_oriented(context->state.ui_font_oriented, dst, pos_x, pos_y, begin, end, color->ui_f.p, color->ui_b.p, context->config.ui_font_orientation);

	context->state.ui_direct_text_flag = 0;
}

static void ui_direct_slow_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	int pos_x, pos_y;
	int size_x, size_y;

	size_x = dst->size_x / 20;
	size_y = dst->size_y * 4 / 3 / 20;

	pos_x = dst->size_x - 1 - size_x - size_x / 4;
	pos_y = size_y / 4;

	adv_bitmap_clear(dst, pos_x, pos_y, size_x, size_y, color->help_p3.p);

	context->state.ui_direct_slow_flag = 0;
}

static void ui_direct_fast_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	int pos_x, pos_y;
	int size_x, size_y;
	int i;
	unsigned m;

	size_x = dst->size_x / 20;
	size_y = dst->size_y * 4 / 3 / 20;
	if (size_y % 2 == 0)
		++size_y; /* make it odd */

	pos_x = dst->size_x - 1 - size_x - size_x / 4;
	pos_y = size_y / 4;

	m = size_y/2;
	if (!m)
		m = 1;
	for(i=0;i<=m;++i) {
		unsigned l = (size_x * i + m - 1) / m + 1;
		adv_bitmap_clear(dst, pos_x, pos_y + i, l, 1, color->help_p3.p);
		adv_bitmap_clear(dst, pos_x, pos_y + size_y - 1 - i, l, 1, color->help_p3.p);
	}

	context->state.ui_direct_slow_flag = 0;
}

static void ui_color_rgb_set(struct ui_color* color, const adv_color_rgb* c, adv_color_def color_def, adv_color_def buffer_def, unsigned translucency, adv_pixel* background)
{
	color->c = *c;
	color->p = pixel_make_from_def(c->red, c->green, c->blue, color_def);
	color->f = alpha_make_from_def(c->red, c->green, c->blue, 255, buffer_def);
	if (background)
		color->b = *background;
	else
		color->b = alpha_make_from_def(c->red, c->green, c->blue, translucency, buffer_def);
}

static void ui_color_alpha_set(adv_pixel* map, const adv_color_rgb* fore, const adv_color_rgb* back, adv_color_def buffer_def, unsigned translucency)
{
	unsigned i;
	int t;

	int red_shift, green_shift, blue_shift, alpha_shift;
	adv_pixel red_mask, green_mask, blue_mask, alpha_mask;
	union adv_color_def_union udef;

	udef.ordinal = buffer_def;
	rgb_shiftmask_get(&red_shift, &red_mask, udef.nibble.red_len, udef.nibble.red_pos);
	rgb_shiftmask_get(&green_shift, &green_mask, udef.nibble.green_len, udef.nibble.green_pos);
	rgb_shiftmask_get(&blue_shift, &blue_mask, udef.nibble.blue_len, udef.nibble.blue_pos);
	alpha_shiftmask_get(&alpha_shift, &alpha_mask, buffer_def);

	t = translucency;

	for(i=0;i<256;++i) {
		int cr, cg, cb, ca;
		double F, B, A;
		double I, T;
		double a, b, c;

		/*
			I = ui foreground/background alpha channel
			T = ui/game alpha channel

			a = ui foreground component
			b = ui background component
			c = game component

			the color is computed with

			a * UI_FOREGROUND + b * UI_BACKGROUND + c * GAME

			with the corner conditions :

			T=1 -> a=I,b=1-I,c=0
			T=0 -> a=I,b=0,c=1-I
			I=0 -> a=0,b=T,c=1-T
			I=1 -> a=1,b=0,c=0

			where always a+b+c = 1

			the following relations are arbitrarily chosen :

			a = I
			b = T * (1-I)
			c = (1-T) * (1-I)
		*/

		I = i / 255.0;
		T = t / 255.0;

		a = I;
		b = T * (1-I);
		c = (1-T) * (1-I);

		A = 1 - c; /* alpha channel */
		if (A == 0) {
			F = 0;
			B = 0;
		} else {
			/* the alpha component is implicitely applied */
			F = a / A;
			B = b / A;
		}

		cr = fore->red * F + back->red * B;
		cg = fore->green * F + back->green * B;
		cb = fore->blue * F + back->blue * B;
		ca = 255 * A;

		if (cr > 255)
			cr = 255;
		if (cg > 255)
			cg = 255;
		if (cb > 255)
			cb = 255;
		if (ca > 255)
			ca = 255;

		map[i] = rgb_nibble_insert(cr, red_shift, red_mask)
			| rgb_nibble_insert(cg, green_shift, green_mask)
			| rgb_nibble_insert(cb, blue_shift, blue_mask)
			| rgb_nibble_insert(ca, alpha_shift, alpha_mask);
	}
}

static void ui_color_palette_set(struct ui_color* color, const adv_color_rgb* c, adv_color_rgb* palette_map, unsigned palette_max)
{
	color->c = *c;
	color->p = video_color_find(c->red, c->green, c->blue, palette_map, palette_max);
	color->f = color->p;
	color->b = color->p;
}

static void ui_setup_color(struct advance_ui_context* context, struct ui_color_set* color, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	const adv_color_rgb* map = context->config.ui_color;
	adv_color_def buffer_def = context->state.buffer_def;
	unsigned translucency = context->config.ui_translucency;

	switch (color_def_type_get(color_def)) {
	case adv_color_type_rgb :
	case adv_color_type_yuy2 :
		if (color->def != color_def) {
			ui_color_alpha_set(color->ui_alpha, &map[UI_COLOR_INTERFACE_F], &map[UI_COLOR_INTERFACE_B], buffer_def, translucency);
			ui_color_alpha_set(color->title_alpha, &map[UI_COLOR_TAG_F], &map[UI_COLOR_TAG_B], buffer_def, translucency);
			ui_color_alpha_set(color->select_alpha, &map[UI_COLOR_SELECT_F], &map[UI_COLOR_SELECT_B], buffer_def, translucency);
			ui_color_rgb_set(&color->ui_f, &map[UI_COLOR_INTERFACE_F], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->ui_b, &map[UI_COLOR_INTERFACE_B], color_def, buffer_def, translucency, color->ui_alpha);
			ui_color_rgb_set(&color->title_f, &map[UI_COLOR_TAG_F], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->title_b, &map[UI_COLOR_TAG_B], color_def, buffer_def, translucency, color->title_alpha);
			ui_color_rgb_set(&color->select_f, &map[UI_COLOR_SELECT_F], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->select_b, &map[UI_COLOR_SELECT_B], color_def, buffer_def, translucency, color->select_alpha);
			ui_color_rgb_set(&color->help_p1, &map[UI_COLOR_HELP_P1], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->help_p2, &map[UI_COLOR_HELP_P2], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->help_p3, &map[UI_COLOR_HELP_P3], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->help_p4, &map[UI_COLOR_HELP_P4], color_def, buffer_def, translucency, 0);
			ui_color_rgb_set(&color->help_u, &map[UI_COLOR_HELP_OTHER], color_def, buffer_def, translucency, 0);
		}
		break;
	case adv_color_type_palette :
		ui_color_palette_set(&color->ui_f, &map[UI_COLOR_INTERFACE_F], palette_map, palette_max);
		ui_color_palette_set(&color->ui_b, &map[UI_COLOR_INTERFACE_B], palette_map, palette_max);
		color->title_f = color->ui_f;
		color->title_b = color->ui_b;
		color->select_f = color->ui_b;
		color->select_b = color->ui_f;
		ui_color_palette_set(&color->help_p1, &map[UI_COLOR_HELP_P1], palette_map, palette_max);
		ui_color_palette_set(&color->help_p2, &map[UI_COLOR_HELP_P2], palette_map, palette_max);
		ui_color_palette_set(&color->help_p3, &map[UI_COLOR_HELP_P3], palette_map, palette_max);
		ui_color_palette_set(&color->help_p4, &map[UI_COLOR_HELP_P4], palette_map, palette_max);
		ui_color_palette_set(&color->help_u, &map[UI_COLOR_HELP_OTHER], palette_map, palette_max);
		break;
	default:
		assert(0);
		break;
	}

	color->def = color_def;
}

void advance_ui_buffer_update(struct advance_ui_context* context, void* ptr, unsigned dx, unsigned dy, unsigned dw, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	adv_bitmap* dst;
	struct ui_color_set* color = &context->state.color_map;

	ui_setup_color(context, color, color_def, palette_map, palette_max);

	dst = adv_bitmap_import_rgb(dx, dy, color_def_bytes_per_pixel_get(color_def), 0, 0, ptr, dw);

	context->state.ui_extra_flag = 0;

	if (context->state.ui_help_flag) {
		ui_help_update(context, dst, color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_menu_flag) {
		ui_menu_update(context, dst, color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_message_flag) {
		ui_message_update(context, dst, color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_osd_flag) {
		ui_osd_update(context, dst, color);
		context->state.ui_extra_flag = 1;
	}
	if (context->state.ui_scroll_flag) {
		ui_scroll_update(context, dst, color);
		context->state.ui_extra_flag = 1;
	}

	adv_bitmap_free(dst);
}

void advance_ui_direct_update(struct advance_ui_context* context, void* ptr, unsigned dx, unsigned dy, unsigned dw, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	adv_bitmap* dst;
	struct ui_color_set color;

	ui_setup_color(context, &color, color_def, palette_map, palette_max);

	dst = adv_bitmap_import_rgb(dx, dy, color_def_bytes_per_pixel_get(color_def), 0, 0, ptr, dw);

	if (context->state.ui_direct_slow_flag) {
		ui_direct_slow_update(context, dst, &color);
	}

	if (context->state.ui_direct_fast_flag) {
		ui_direct_fast_update(context, dst, &color);
	}

	if (context->state.ui_direct_text_flag) {
		ui_direct_text_update(context, dst, &color);
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
	context->state.ui_direct_text_flag = 0;
	context->state.ui_direct_slow_flag = 0;
	context->state.ui_direct_fast_flag = 0;
	context->state.ui_font = 0;
	context->state.ui_font_oriented = 0;
	context->state.buffer_def = color_def_make_rgb_from_sizelenpos(4, 8, 16, 8, 8, 8, 0); /* BGRA */
	context->state.color_map.def = 0; /* invalidate the color map */

	conf_bool_register_default(cfg_context, "debug_speedmark", 0);
	conf_string_register_multi(cfg_context, "ui_helptag");
	conf_string_register_default(cfg_context, "ui_helpimage", "auto");
	conf_string_register_default(cfg_context, "ui_font", "auto");
	conf_string_register_default(cfg_context, "ui_fontsize", "auto");
	conf_string_register_default(cfg_context, "ui_color[interface]", "000000 ffffff");
	conf_string_register_default(cfg_context, "ui_color[tag]", "247ef0 ffffff");
	conf_string_register_default(cfg_context, "ui_color[select]", "000000 afffff");
	conf_string_register_default(cfg_context, "ui_color[help_p1]", "000000 ffff00");
	conf_string_register_default(cfg_context, "ui_color[help_p2]", "000000 00ff00");
	conf_string_register_default(cfg_context, "ui_color[help_p3]", "000000 ff0000");
	conf_string_register_default(cfg_context, "ui_color[help_p4]", "000000 00ffff");
	conf_string_register_default(cfg_context, "ui_color[help_other]", "000000 808080");
	conf_float_register_limit_default(cfg_context, "ui_translucency", 0, 1, 0.80);

	return 0;
}

static int ui_color_hex(adv_color_rgb* rgb, const char* s)
{
	unsigned v;

	if (strspn(s,"0123456789abcdefABCDEF") != strlen(s))
		return -1;
	if (strlen(s) != 6)
		return -1;

	v = strtol(s, 0, 16);

	rgb->red = (v >> 16) & 0xFF;
	rgb->green = (v >> 8) & 0xFF;
	rgb->blue = v & 0xFF;
	rgb->alpha = 0;

	return 0;
}

static int ui_color_load(struct advance_ui_context* context, adv_conf* cfg_context, int f, int b, const char* tag)
{
	const char* fs;
	const char* bs;
	const char* s;
	int p;
	char c;
	char* d;

	s = conf_string_get_default(cfg_context, tag);

	d = strdup(s);

	p = 0;

	fs = stoken(&c, &p, d, " ", "");
	if (c != ' ') {
		target_err("Error in option %s\n", tag);
		return -1;
	}
	sskip(&p, d, " ");
	bs = stoken(&c, &p, d, " ", "");
	if (c != 0) {
		target_err("Error in option %s\n", tag);
		return -1;
	}

	if (f >= 0) {
		if (ui_color_hex(&context->config.ui_color[f], fs) != 0) {
			target_err("Error in option %s\n", tag);
			return -1;
		}
	}

	if (b >= 0) {
		if (ui_color_hex(&context->config.ui_color[b], bs) != 0) {
			target_err("Error in option %s\n", tag);
			return -1;
		}
	}

	free(d);

	return 0;
}

adv_error advance_ui_config_load(struct advance_ui_context* context, adv_conf* cfg_context, struct mame_option* option)
{
	const char* s;
	char* e;

	context->config.ui_speedmark_flag = conf_bool_get_default(cfg_context, "debug_speedmark");

	sncpy(context->config.help_image_buffer, sizeof(context->config.help_image_buffer), conf_string_get_default(cfg_context, "ui_helpimage"));
	sncpy(context->config.ui_font_buffer, sizeof(context->config.ui_font_buffer), conf_string_get_default(cfg_context, "ui_font"));

	context->config.ui_translucency = conf_float_get_default(cfg_context, "ui_translucency") * 255;
	if (context->config.ui_translucency > 255)
		context->config.ui_translucency = 255;

	s = conf_string_get_default(cfg_context, "ui_fontsize");

	context->config.ui_font_sizey = strtol(s, &e, 10);
	while (*e && isspace(*e))
		++e;
	if (*e) {
		context->config.ui_font_sizex = strtol(e, &e, 10);
	} else {
		context->config.ui_font_sizex = 0;
	}

	context->config.ui_font_orientation = option->direct_orientation;

	if (ui_color_load(context, cfg_context, UI_COLOR_INTERFACE_F, UI_COLOR_INTERFACE_B, "ui_color[interface]") != 0
		|| ui_color_load(context, cfg_context, UI_COLOR_TAG_F, UI_COLOR_TAG_B, "ui_color[tag]") != 0
		|| ui_color_load(context, cfg_context, UI_COLOR_SELECT_F, UI_COLOR_SELECT_B, "ui_color[select]") != 0
		|| ui_color_load(context, cfg_context, -1, UI_COLOR_HELP_P1, "ui_color[help_p1]") != 0
		|| ui_color_load(context, cfg_context, -1, UI_COLOR_HELP_P2, "ui_color[help_p2]") != 0
		|| ui_color_load(context, cfg_context, -1, UI_COLOR_HELP_P3, "ui_color[help_p3]") != 0
		|| ui_color_load(context, cfg_context, -1, UI_COLOR_HELP_P4, "ui_color[help_p4]") != 0
		|| ui_color_load(context, cfg_context, -1, UI_COLOR_HELP_OTHER, "ui_color[help_other]") != 0)
		return -1;

	return 0;
}

void advance_ui_done(struct advance_ui_context* context)
{
}

#include "help.dat"

void advance_ui_changefont(struct advance_ui_context* context, unsigned screen_width, unsigned screen_height, unsigned aspect_x, unsigned aspect_y)
{
	unsigned sizex;
	unsigned sizey;

	adv_font_free(context->state.ui_font);
	adv_font_free(context->state.ui_font_oriented);

	context->state.ui_font = 0;
	context->state.ui_font_oriented = 0;

	if ((context->config.ui_font_orientation & OSD_ORIENTATION_SWAP_XY) != 0) {
		unsigned t = screen_width;
		screen_width = screen_height;
		screen_height = t;
		t = aspect_x;
		aspect_x = aspect_y;
		aspect_y = t;
	}

	if (context->config.ui_font_sizey >= 5 && context->config.ui_font_sizey <= 100) {
		sizey = screen_height / context->config.ui_font_sizey;
	} else {
		sizey = screen_height / 30;
	}

	if (context->config.ui_font_sizex >= 5 && context->config.ui_font_sizex <= 200) {
		sizex = screen_width / context->config.ui_font_sizex;
	} else {
		log_std(("emu:ui: font computation: screen %dx%d, aspect %dx%d\n", screen_width, screen_height, aspect_x, aspect_y));
		sizex = sizey * (screen_width * aspect_y) / (screen_height * aspect_x);
	}

	log_std(("emu:ui: font pixel size %dx%d\n", sizex, sizey));

	if (strcmp(context->config.ui_font_buffer, "auto") != 0) {
		adv_fz* f;
		const char* file = file_config_file_home(context->config.ui_font_buffer);

		log_std(("emu:ui: load font '%s'\n", file));

		f = fzopen(file, "rb");
		if (f) { /* ignore error */
			context->state.ui_font = adv_font_load(f, sizex, sizey);
			/* ignore error */

			fzclose(f);
		}
	}

	/* use default font otherwise */
	if (!context->state.ui_font)
		context->state.ui_font = adv_font_default(sizex, sizey, 0);

	if (!context->state.ui_font_oriented)
		context->state.ui_font_oriented = adv_font_default(13, 13, 1);

	adv_font_orientation(context->state.ui_font_oriented, context->config.ui_font_orientation);
}

adv_error advance_ui_inner_init(struct advance_ui_context* context, adv_conf* cfg_context)
{
	adv_conf_iterator k;
	adv_color_def def;

	context->state.ui_font = 0;
	context->state.ui_font_oriented = 0;

	if (strcmp(context->config.ui_font_buffer, "auto") != 0) {
		/* try reading the font, the real font is loaded later */
		adv_font* font;
		adv_fz* f;

		const char* file = file_config_file_home(context->config.ui_font_buffer);

		log_std(("emu:ui: font '%s'\n", file));

		f = fzopen(file, "rb");
		if (!f) {
			target_err("Error opening the font %s\n", file);
			return -1;
		}

		font = adv_font_load(f, 16, 16);
		if (!font) {
			target_err("Error reading the font %s\n%s\n", file, error_get());
			return -1;
		}

		adv_font_free(font);

		fzclose(f);
	}

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
	adv_font_free(context->state.ui_font_oriented);
	context->state.ui_font_oriented = 0;

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

void osd_ui_menu(const ui_menu_item *items, int numitems, int selected)
{
	unsigned menu_mac;
	struct ui_menu menu;
	unsigned menu_sel;
	unsigned i;

	struct advance_ui_context* context = &CONTEXT.ui;

	/* count elements */
	menu_mac = numitems;

	advance_ui_menu_init(&menu);

	/* HACK: Recognize main menu */
	if (items && strstr(items[0].text, "Input") != 0)
		advance_ui_menu_title_insert(&menu, ADV_TITLE);

	for(i=0;i<menu_mac;++i) {
		if (items[i].subtext) {
			/* HACK: Recongnize options "None" and "n/a" */
			if (strcmp(items[i].subtext, "None") == 0
				|| strcmp(items[i].subtext, "n/a") == 0) {
				advance_ui_menu_option_insert(&menu, items[i].text, "<none>");
			} else {
				advance_ui_menu_option_insert(&menu, items[i].text, items[i].subtext);
			}
		} else {
			advance_ui_menu_text_insert(&menu, items[i].text);
		}
	}

	if (selected >= 0 && selected < menu_mac)
		menu_sel = selected;
	else
		menu_sel = menu_mac;

	advance_ui_menu_done(&menu, context, menu_sel);
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

