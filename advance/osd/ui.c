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
#include "glue.h"
#include "input.h"

#include "advance.h"

/**************************************************************************/
/* Internal */

struct ui_color {
	adv_pixel p;
	adv_color_rgb c;
};

static inline adv_bool ui_alpha(adv_color_def color_def)
{
	return color_def_type_get(color_def) != adv_color_type_palette;
}

static void ui_text_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_color_def def)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x / 2;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_alpha(context->state.ui_font, dst, pos_x, pos_y, begin, end, &cf.c, &cb.c, def);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_text_left(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_color_def def)
{
	int pos_x, pos_y;

	pos_x = x;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_alpha(context->state.ui_font, dst, pos_x, pos_y, begin, end, &cf.c, &cb.c, def);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_text_right(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_color_def def)
{
	int size_x;
	int pos_x, pos_y;

	size_x = adv_font_sizex_string(context->state.ui_font, begin, end);

	pos_x = x - size_x;
	pos_y = y;

	if (ui_alpha(def))
		adv_font_put_string_alpha(context->state.ui_font, dst, pos_x, pos_y, begin, end, &cf.c, &cb.c, def);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x, pos_y, begin, end, cf.p, cb.p);
}

static void ui_messagebox_center(struct advance_ui_context* context, adv_bitmap* dst, int x, int y, const char* begin, const char* end, struct ui_color cf, struct ui_color cb, adv_color_def def)
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

	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, cf.p);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, cb.p);

	if (ui_alpha(def))
		adv_font_put_string_alpha(context->state.ui_font, dst, pos_x + border_x, pos_y + border_y, begin, end, &cf.c, &cb.c, def);
	else
		adv_font_put_string(context->state.ui_font, dst, pos_x + border_x, pos_y + border_y, begin, end, cf.p, cb.p);
}

static void ui_menu(struct advance_ui_context* context, adv_bitmap* dst, const char* menu_title, struct ui_menu_entry* menu_map, unsigned menu_mac, int menu_sel, struct ui_color entry_f, struct ui_color entry_b, struct ui_color select_f, struct ui_color select_b, struct ui_color title_f, struct ui_color title_b, adv_color_def def)
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
	if (menu_title)
		size_y += step_y;
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
	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, entry_f.p);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, entry_b.p);

	if (menu_title) {
		const char* begin = menu_title;
		const char* end = begin + strlen(begin);
		unsigned width = size_x - 2 * border_x;

		end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

		ui_text_center(context, dst, pos_x + size_x / 2, pos_y + border_y, begin, end, title_f, title_b, def);
	}

	for(i=0;i<size_r;++i) {
		struct ui_menu_entry* e = &menu_map[i + posr];
		struct ui_color cef;
		struct ui_color ceb;
		struct ui_color ctf;
		struct ui_color ctb;
		unsigned width = size_x - 2 * border_x;
		unsigned y = pos_y + border_y + step_y * i;
		if (menu_title)
			y += step_y;
		
		if (i + posr == menu_sel) {
			cef = select_f;
			ceb = select_b;
			ctf = select_f;
			ctb = select_b;
		} else {
			cef = entry_f;
			ceb = entry_b;
			ctf = title_f;
			ctb = title_b;
		}

		adv_bitmap_clear(dst, pos_x + border_x / 2, y, size_x - border_x, step_y, ceb.p);

		if (e->option_buffer[0]) {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_left(context, dst, pos_x + border_x, y, begin, end, cef, ceb, def);

			width -= adv_font_sizex_string(context->state.ui_font, begin, end);

			begin = e->option_buffer;
			end = e->option_buffer + strlen(e->option_buffer);

			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);
			
			ui_text_right(context, dst, pos_x + size_x - border_x, y, begin, end, ctf, ctb, def);
		} else {
			const char* begin = e->text_buffer;
			const char* end = e->text_buffer + strlen(e->text_buffer);
			
			end = adv_font_sizex_limit(context->state.ui_font, begin, end, width);

			ui_text_center(context, dst, pos_x + size_x / 2, y, begin, end, cef, ceb, def);
		}
	}
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

static void ui_scroll(struct advance_ui_context* context, adv_bitmap* dst, char* begin, char* end, unsigned pos, struct ui_color text_f, struct ui_color text_b, struct ui_color title_f, struct ui_color title_b, adv_color_def def)
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

	/* put */
	adv_bitmap_box(dst, pos_x, pos_y, size_x, size_y, 1, text_f.p);
	adv_bitmap_clear(dst, pos_x + 1, pos_y + 1, size_x - 2, size_y - 2, text_b.p);

	n = 0;
	i = start;
	while (i != end && n < size_v) {
		struct ui_color f;
		struct ui_color b;
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
		} else {
			f = text_f;
			b = text_b;
		}

		ui_text_left(context, dst, pos_x + border_x, pos_y + border_y + n * step_y, i, je, f, b, def);

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
void advance_ui_menu(struct advance_ui_context* context, const char* menu_title, struct ui_menu_entry* menu_map, unsigned menu_mac, unsigned menu_sel)
{
	context->state.ui_menu_flag = 1;
	free(context->state.ui_menu_map);
	context->state.ui_menu_title = menu_title;
	context->state.ui_menu_map = menu_map;
	context->state.ui_menu_mac = menu_mac;
	context->state.ui_menu_sel = menu_sel;
}

void advance_ui_menu_vect(struct advance_ui_context* context, const char* title, const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem)
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

	advance_ui_menu(context, title, menu_map, menu_mac, menu_sel);
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

struct ui_color_set {
	adv_color_def def;
	struct ui_color ui_f; /**< User interface foreground. */
	struct ui_color ui_b; /**< User interface bacground. */
	struct ui_color title_f; /**< Title foreground. */
	struct ui_color title_b; /**< Title bacground. */
	struct ui_color select_f; /**< Selected menu entry foreground. */
	struct ui_color select_b; /**< Selected menu entry bacground. */
	struct ui_color help_p1; /**< Help player 1 foreground. */
	struct ui_color help_p2; /**< Help player 2 foreground. */
	struct ui_color help_p3; /**< Help player 3 foreground. */
	struct ui_color help_p4; /**< Help player 4 foreground. */
	struct ui_color help_u; /**< Help unassigned foreground. */
};

static void ui_message_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	if (context->state.ui_message_stop_time < advance_timer()) {
		context->state.ui_message_flag = 0;
	}

	ui_messagebox_center(context, dst, dst->size_x / 2, dst->size_y / 2, context->state.ui_message_buffer, context->state.ui_message_buffer + strlen(context->state.ui_message_buffer), color->ui_f, color->ui_b, color->def);
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
			adv_pixel c;
			if ((adv_bitmap_pixel_get(context->state.help_image, cx, cy)) != pb) {
				c = color->ui_f.p;
			} else {
				c = color->ui_b.p;
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
				case 1 : ckb = color->help_p1.p; break;
				case 2 : ckb = color->help_p2.p; break;
				case 3 : ckb = color->help_p3.p; break;
				case 4 : ckb = color->help_p4.p; break;
				default : ckb = color->help_u.p; break;
				}
				ckf = color->ui_f.p;

				if (digital_map[i].port_state) {
					ckf = color->ui_f.p;
					ckb = color->ui_f.p;
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
		ui_messagebox_center(context, dst, dst->size_x / 2, pos_y + size_y + adv_font_sizey(context->state.ui_font) * 2, msg_buffer, msg_buffer + strlen(msg_buffer), color->ui_f, color->ui_b, color->def);
}

static void ui_menu_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	ui_menu(context, dst, context->state.ui_menu_title, context->state.ui_menu_map, context->state.ui_menu_mac, context->state.ui_menu_sel, color->ui_f, color->ui_b, color->select_f, color->select_b, color->title_f, color->title_b, color->def);

	context->state.ui_menu_title = 0;
	free(context->state.ui_menu_map);
	context->state.ui_menu_map = 0;
	context->state.ui_menu_flag = 0;
}

static void ui_osd_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	unsigned pos_x, pos_y;

	pos_x = dst->size_x / 2;
	pos_y = dst->size_y * 7 / 8;

	ui_messagebox_center(context, dst, pos_x, pos_y, context->state.ui_osd_buffer, context->state.ui_osd_buffer + strlen(context->state.ui_osd_buffer), color->ui_f, color->ui_b, color->def);

	context->state.ui_osd_flag = 0;
}

static void ui_scroll_update(struct advance_ui_context* context, adv_bitmap* dst, struct ui_color_set* color)
{
	ui_scroll(context, dst, context->state.ui_scroll_begin, context->state.ui_scroll_end, context->state.ui_scroll_pos, color->ui_f, color->ui_b, color->title_f, color->title_b, color->def);

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

static void ui_color_rgb_set(struct ui_color* color, const adv_color_rgb* c, adv_color_def color_def)
{
	color->c = *c;
	color->p = pixel_make_from_def(c->red, c->green, c->blue, color_def);
}

static void ui_color_palette_set(struct ui_color* color, const adv_color_rgb* c, adv_color_rgb* palette_map, unsigned palette_max)
{
	color->c = *c;
	color->p = video_color_find(c->red, c->green, c->blue, palette_map, palette_max);
}

static void ui_setup_color(struct advance_ui_context* context, struct ui_color_set* color, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	const adv_color_rgb* map;

	map = context->config.ui_color;
	color->def = color_def;

	switch (color_def_type_get(color_def)) {
	case adv_color_type_rgb :
	case adv_color_type_yuy2 :
		ui_color_rgb_set(&color->ui_f, &map[UI_COLOR_INTERFACE_F], color_def);
		ui_color_rgb_set(&color->ui_b, &map[UI_COLOR_INTERFACE_B], color_def);
		ui_color_rgb_set(&color->title_f, &map[UI_COLOR_TAG_F], color_def);
		ui_color_rgb_set(&color->title_b, &map[UI_COLOR_TAG_B], color_def);
		ui_color_rgb_set(&color->select_f, &map[UI_COLOR_SELECT_F], color_def);
		ui_color_rgb_set(&color->select_b, &map[UI_COLOR_SELECT_B], color_def);
		ui_color_rgb_set(&color->help_p1, &map[UI_COLOR_HELP_P1], color_def);
		ui_color_rgb_set(&color->help_p2, &map[UI_COLOR_HELP_P2], color_def);
		ui_color_rgb_set(&color->help_p3, &map[UI_COLOR_HELP_P3], color_def);
		ui_color_rgb_set(&color->help_p4, &map[UI_COLOR_HELP_P4], color_def);
		ui_color_rgb_set(&color->help_u, &map[UI_COLOR_HELP_OTHER], color_def);
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
}

void advance_ui_buffer_update(struct advance_ui_context* context, void* ptr, unsigned dx, unsigned dy, unsigned dw, adv_color_def color_def, adv_color_rgb* palette_map, unsigned palette_max)
{
	adv_bitmap* dst;
	struct ui_color_set color;

	ui_setup_color(context, &color, color_def, palette_map, palette_max);

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
	context->state.ui_menu_title = 0;
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

	conf_bool_register_default(cfg_context, "ui_speedmark", 0);
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

	context->config.ui_speedmark_flag = conf_bool_get_default(cfg_context, "ui_speedmark");

	sncpy(context->config.help_image_buffer, sizeof(context->config.help_image_buffer), conf_string_get_default(cfg_context, "ui_helpimage"));
	sncpy(context->config.ui_font_buffer, sizeof(context->config.ui_font_buffer), conf_string_get_default(cfg_context, "ui_font"));

	s = conf_string_get_default(cfg_context, "ui_fontsize");

	context->config.ui_font_sizey = strtol(s, &e, 10);
	if (*e) {
		context->config.ui_font_sizex = strtol(s, &e, 10);
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

	if (context->config.ui_font_sizey >= 5 && context->config.ui_font_sizey <= 100)
		sizey = screen_height / context->config.ui_font_sizey;
	else
		sizey = screen_height / 30;

	if (context->config.ui_font_sizex >= 5 && context->config.ui_font_sizex <= 200)
		sizex = screen_width / context->config.ui_font_sizex;
	else
		sizex = sizey * (screen_width*aspect_y) / (screen_height*aspect_x);

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

void osd_ui_menu(const char** items, const char** subitems, char* flag, int selected, int arrowize_subitem)
{
	unsigned menu_mac;
	struct ui_menu_entry* menu_map;
	unsigned menu_sel;
	unsigned i;
	const char* menu_title;

	struct advance_ui_context* context = &CONTEXT.ui;

	/* count elements */
	menu_mac = 0;
	while (items[menu_mac])
		++menu_mac;

	menu_map = malloc(menu_mac * sizeof(struct ui_menu_entry));

	for(i=0;i<menu_mac;++i) {
		sncpy(menu_map[i].text_buffer, sizeof(menu_map[i].text_buffer), items[i]);
		if (subitems && subitems[i]) {

			/* HACK: Recongnize options "None" and "n/a" */
			if (strcmp(subitems[i], "None") == 0
				|| strcmp(subitems[i], "n/a") == 0)
				sncpy(menu_map[i].option_buffer, sizeof(menu_map[i].option_buffer), "<none>");
			else
				sncpy(menu_map[i].option_buffer, sizeof(menu_map[i].option_buffer), subitems[i]);

			if (flag && flag[i]) {
				menu_map[i].flag = flag[i] != 0;
			} else {
				menu_map[i].flag = 0;
			}
		} else {
			menu_map[i].option_buffer[0] = 0;
			menu_map[i].flag = 0;
		}

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

	/* HACK: Recongnize main menu */
	if (menu_mac > 0 && strstr(menu_map[0].text_buffer, "Input") != 0)
		menu_title = ADVANCE_TITLE;
	else
		menu_title = 0;

	advance_ui_menu(context, menu_title, menu_map, menu_mac, menu_sel);
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

