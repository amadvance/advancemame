/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2004 Andrea Mazzoleni
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
 */

#include "event.h"
#include "common.h"
#include "play.h"

#include "target.h"
#include "keydrv.h"

#include <deque>

using namespace std;

static deque<int> event_queue;
static int event_last_push = EVENT_NONE;
static int event_last_code = EVENT_NONE;
static target_clock_t event_last_time;
static unsigned event_last_counter;
static bool event_alpha_mode;
static double event_delay_repeat_ms;
static double event_delay_repeat_next_ms;
static string event_press_sound;

#define SEQ_MAX 256

struct event_item {
	const char* name;
	unsigned event;
	unsigned seq[SEQ_MAX];
	target_clock_t time;
	unsigned counter;
};

#define OP_NONE KEYB_MAX
#define OP_OR (KEYB_MAX + 1)
#define OP_NOT (KEYB_MAX + 2)

static struct event_item EVENT_TAB[] = {
{"up", EVENT_UP, { KEYB_UP, OP_OR, KEYB_8_PAD, KEYB_MAX } },
{"down", EVENT_DOWN, { KEYB_DOWN, OP_OR, KEYB_2_PAD, KEYB_MAX } },
{"left", EVENT_LEFT, { KEYB_LEFT, OP_OR, KEYB_4_PAD, KEYB_MAX } },
{"right", EVENT_RIGHT, { KEYB_RIGHT, OP_OR, KEYB_6_PAD, KEYB_MAX } },
{"enter", EVENT_ENTER, { KEYB_ENTER, OP_OR, KEYB_ENTER_PAD, KEYB_MAX } },
{"esc", EVENT_ESC, { KEYB_ESC, KEYB_MAX } },
{"space", EVENT_SPACE, { KEYB_SPACE, KEYB_MAX } },
{"home", EVENT_HOME, { KEYB_HOME, KEYB_MAX } },
{"end", EVENT_END, { KEYB_END, KEYB_MAX } },
{"pgup", EVENT_PGUP, { KEYB_PGUP, KEYB_MAX } },
{"pgdn", EVENT_PGDN, { KEYB_PGDN, KEYB_MAX } },
{"del", EVENT_DEL, { KEYB_DEL, KEYB_MAX } },
{"ins", EVENT_INS, { KEYB_INSERT, KEYB_MAX } },

{"shutdown", EVENT_OFF, { KEYB_LCONTROL, KEYB_ESC, KEYB_MAX } },
{"mode", EVENT_MODE, { KEYB_TAB, KEYB_MAX } },
{"help", EVENT_HELP, { KEYB_F1, KEYB_MAX } },
{"group", EVENT_GROUP, { KEYB_F2, KEYB_MAX } },
{"type", EVENT_TYPE, { KEYB_F3, KEYB_MAX } },
{"exclude", EVENT_ATTRIB, { KEYB_F4, KEYB_MAX } },
{"sort", EVENT_SORT, { KEYB_F5, KEYB_MAX } },
{"setgroup", EVENT_SETGROUP, { KEYB_F9, KEYB_MAX } },
{"settype", EVENT_SETTYPE, { KEYB_F10, KEYB_MAX } },
{"runclone", EVENT_RUN_CLONE, { KEYB_F12, KEYB_MAX } },
{"command", EVENT_COMMAND, { KEYB_F8, KEYB_MAX } },
{"menu", EVENT_MENU, { KEYB_BACKQUOTE, OP_OR, KEYB_BACKSLASH, KEYB_MAX } },
{"emulator", EVENT_EMU, { KEYB_F6, KEYB_MAX } },
{"rotate", EVENT_ROTATE, { KEYB_0_PAD, KEYB_MAX } },
{"lock", EVENT_LOCK, { KEYB_SCRLOCK, KEYB_MAX } },
{"preview", EVENT_PREVIEW, { KEYB_SPACE, KEYB_MAX } },
{ 0, 0, { 0 } }
};

static int seq_pressed(const unsigned* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for(j=0;j<SEQ_MAX;++j) {
		switch (code[j]) {
			case OP_NONE :
				return res && count;
			case OP_OR :
				if (res && count)
					return 1;
				res = 1;
				count = 0;
				break;
			case OP_NOT :
				invert = !invert;
				break;
			default:
				if (res) {
					adv_bool pressed = 0;
					for(unsigned k=0;k<keyb_count_get();++k)
						pressed = pressed || (keyb_get(k, code[j]) != 0);
					if ((pressed != 0) == invert)
						res = 0;
				}
				invert = 0;
				++count;
		}
	}
	return res && count;
}

static int seq_valid(const unsigned* seq)
{
	int j;
	int positive = 0; // if isn't a completly negative sequence
	int pred_not = 0;
	int operand = 0;
	for(j=0;j<SEQ_MAX;++j)
	{
		switch (seq[j])
		{
			case OP_NONE :
				return positive && operand;
			case OP_OR :
				if (!operand || !positive)
					return 0;
				pred_not = 0;
				positive = 0;
				operand = 0;
				break;
			case OP_NOT :
				if (pred_not)
					return 0;
				pred_not = !pred_not;
				operand = 0;
				break;
			default:
				if (!pred_not)
					positive = 1;
				pred_not = 0;
				operand = 1;
				break;
		}
	}

	return positive && operand;
}

static struct event_conv {
	int code;
	char c;
} EVENT_CONV[] = {
{ KEYB_A, 'a' },
{ KEYB_B, 'b' },
{ KEYB_C, 'c' },
{ KEYB_D, 'd' },
{ KEYB_E, 'e' },
{ KEYB_F, 'f' },
{ KEYB_G, 'g' },
{ KEYB_H, 'h' },
{ KEYB_I, 'i' },
{ KEYB_J, 'j' },
{ KEYB_K, 'k' },
{ KEYB_L, 'l' },
{ KEYB_M, 'm' },
{ KEYB_N, 'n' },
{ KEYB_O, 'o' },
{ KEYB_P, 'p' },
{ KEYB_Q, 'q' },
{ KEYB_R, 'r' },
{ KEYB_S, 's' },
{ KEYB_T, 't' },
{ KEYB_U, 'u' },
{ KEYB_V, 'v' },
{ KEYB_W, 'w' },
{ KEYB_X, 'x' },
{ KEYB_Y, 'y' },
{ KEYB_Z, 'z' },
{ KEYB_0, '0' },
{ KEYB_1, '1' },
{ KEYB_2, '2' },
{ KEYB_3, '3' },
{ KEYB_4, '4' },
{ KEYB_5, '5' },
{ KEYB_6, '6' },
{ KEYB_7, '7' },
{ KEYB_8, '8' },
{ KEYB_9, '9' },
{ KEYB_MAX, ' ' },
};

static void event_insert(unsigned event, unsigned* seq)
{
	unsigned i;

	for(i=0;EVENT_TAB[i].name && EVENT_TAB[i].event != event;++i);

	if (EVENT_TAB[i].name) {
		unsigned j;
		for(j=0;seq[j]!=OP_NONE;++j)
			EVENT_TAB[i].seq[j] = seq[j];
		EVENT_TAB[i].seq[j] = OP_NONE;
	}
}

static unsigned string2event(const string& s)
{
	unsigned i;

	for(i=0;EVENT_TAB[i].name && EVENT_TAB[i].name != s;++i);

	if (EVENT_TAB[i].name)
		return EVENT_TAB[i].event;
	else
		return EVENT_NONE;
}

bool event_in(const string& s)
{
	string sevent;
	unsigned seq[SEQ_MAX];
	unsigned seq_count;
	int pos = 0;

	sevent = arg_get(s, pos);

	if (sevent == "snapshot") /* LEGACY 2.3.7 */
		return true; /* ignore */

	unsigned event = string2event(sevent);
	if (event == EVENT_NONE)
		return false;

	seq_count = 0;
	while (pos < s.length()) {
		if (seq_count+1 >= SEQ_MAX)
			return false;

		string skey = arg_get(s, pos);
		if (skey == "or") {
			seq[seq_count++] = OP_OR;
		} else if (skey == "not") {
			seq[seq_count++] = OP_NOT;
		} else if (skey == "and") {
			/* nothing */
		} else {
			unsigned key;
			key = key_code(skey.c_str());
			if (key == KEYB_MAX) {
				// support the old scan code format only numeric
				if (skey.find_first_not_of("0123456789") == string::npos)
					key = atoi(skey.c_str());
				if (key >= KEYB_MAX) {
					return false;
				}
			}
			seq[seq_count++] = key;
		}
	}

	seq[seq_count] = OP_NONE;

	if (!seq_valid(seq))
		return false;

	event_insert(event, seq);

	return true;
}

void event_out(adv_conf* config_context, const char* tag)
{
	for(unsigned i=0;EVENT_TAB[i].name;++i) {
		string s;
		s += EVENT_TAB[i].name;
		s += " ";
		for(unsigned j=0;EVENT_TAB[i].seq[j] != KEYB_MAX && j<SEQ_MAX;++j) {
			unsigned k = EVENT_TAB[i].seq[j];
			if (j != 0)
				s += " ";
			if (k == OP_OR)
				s += "or";
			else if (k == OP_NOT)
				s += "and";
			else {
				s += key_name(k);
			}
		}

		conf_set(config_context, "", tag, s.c_str());
	}
}

std::string event_tag(const std::string& s, unsigned key)
{
	unsigned i;
	for(i=0;EVENT_TAB[i].name;++i) {
		if (EVENT_TAB[i].event == key)
			break;
	}

	if (!EVENT_TAB[i].name || EVENT_TAB[i].seq[0] == KEYB_MAX || EVENT_TAB[i].seq[1] != KEYB_MAX)
		return s;

	string n = key_name(EVENT_TAB[i].seq[0]);

	for(unsigned j=0;j<n.length();++j)
		n[j] = toupper(n[j]);

	return s + "^" + n;
}

void event_poll()
{
	for(struct event_item* i=EVENT_TAB;i->name;++i) {
		if (seq_pressed(i->seq)) {
			event_push(i->event);
		} else {
			i->counter = 0;
			i->time = 0;
		}
	}

	if (event_alpha_mode) {
		for(unsigned i=0;EVENT_CONV[i].code != KEYB_MAX;++i) {
			for(unsigned k=0;k<keyb_count_get();++k) {
				if (keyb_get(k, EVENT_CONV[i].code)) {
					event_push(EVENT_CONV[i].c);
				}
			}
		}
	}
}

void event_push_repeat(int event)
{
	if (event == EVENT_NONE)
		return;

	event_queue.push_front(event);
}

static void event_signal()
{
	play_foreground_effect_key(event_press_sound);
}

static void event_push_norepeat(int event, target_clock_t& time, unsigned& counter)
{
	double delay_ms = (target_clock() - time) / (double)TARGET_CLOCKS_PER_SEC * 1000;

	bool is_repeat;

	switch (event) {
	case EVENT_UP :
	case EVENT_DOWN :
	case EVENT_LEFT :
	case EVENT_RIGHT :
	case EVENT_PGUP :
	case EVENT_PGDN :
		is_repeat = true;
		break;
	default:
		is_repeat = false;
		break;
	}

	if (counter == 0) {
		time = target_clock();
		++counter;
		event_queue.push_front(event);
		event_signal();
	} else if (is_repeat && counter == 1 && delay_ms > event_delay_repeat_ms) {
		time = target_clock();
		++counter;
		event_queue.push_front(event);
	} else if (is_repeat && counter > 1 && delay_ms > event_delay_repeat_next_ms) {
		time = target_clock();
		++counter;
		event_queue.push_front(event);
	}
}

void event_push(int event)
{
	if (event == EVENT_NONE)
		return;

	event_last_push = event;

	struct event_item* i;
	for(i=EVENT_TAB;i->name;++i) {
		if (i->event == event)
			break;
	}

	if (i->name) {
		event_push_norepeat(event, i->time, i->counter);
	} else {
		if (event_last_code != event) {
			event_last_time = 0;
			event_last_counter = 0;
		}
		event_push_norepeat(event, event_last_time, event_last_counter);
		event_last_code = event;
	}
}

int event_pop()
{
	int event = event_queue.back();

	event_queue.pop_back();

	return event;
}

int event_peek()
{
	if (event_queue.size() == 0)
		return EVENT_NONE;
	else
		return event_queue.back();
}

void event_setup(const string& press_sound, double delay_repeat_ms, double delay_repeat_next_ms, bool alpha_mode)
{
	event_press_sound = press_sound;
	event_alpha_mode = alpha_mode;
	event_delay_repeat_ms = delay_repeat_ms;
	event_delay_repeat_next_ms = delay_repeat_next_ms;
}

int event_last()
{
	return event_last_push;
}

void event_forget()
{
	event_last_push = EVENT_NONE;
}

