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

#include "portable.h"

#include "event.h"
#include "common.h"
#include "play.h"
#include "log.h"

#include "target.h"
#include "keydrv.h"
#include "joydrv.h"
#include "mousedrv.h"

#include <deque>
#include <sstream>

using namespace std;

static deque<int> event_queue;
static int event_last_push = EVENT_NONE;
static target_clock_t event_last_time;
static unsigned event_last_counter;
static bool event_alpha_mode;
static bool event_unassigned_mode;
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
	bool simulate;
};

#define OP_NONE 0
#define OP_OR 1
#define OP_NOT 2
#define OP_HIDDEN 3 /* Event disabled and hidden in the menus */

#define OP_KEY (1 << 8)
#define OP_JOY (2 << 8)
#define OP_MOUSE (3 << 8)

#define K(a) (OP_KEY + KEYB_ ## a)
#define J(a) (OP_JOY + JOYB_ ## a)
#define M(a) (OP_MOUSE + MOUSEB_ ## a)

static struct event_item EVENT_TAB[] = {
	{ "up", EVENT_UP, { K(UP), OP_OR, K(8_PAD), OP_NONE } },
	{ "down", EVENT_DOWN, { K(DOWN), OP_OR, K(2_PAD), OP_NONE } },
	{ "left", EVENT_LEFT, { K(LEFT), OP_OR, K(4_PAD), OP_NONE } },
	{ "right", EVENT_RIGHT, { K(RIGHT), OP_OR, K(6_PAD), OP_NONE } },
	{ "enter", EVENT_ENTER, { K(ENTER), OP_OR, K(ENTER_PAD), OP_OR, J(0), OP_OR, M(0), OP_NONE } },
	{ "esc", EVENT_ESC, { K(ESC), OP_OR, J(1), OP_OR, M(1), OP_NONE } },
	{ "space", EVENT_SPACE, { K(SPACE), OP_OR, J(3), OP_NONE } },
	{ "home", EVENT_HOME, { K(HOME), OP_NONE } },
	{ "end", EVENT_END, { K(END), OP_NONE } },
	{ "pgup", EVENT_PGUP, { K(PGUP), OP_OR, J(6), OP_NONE } },
	{ "pgdn", EVENT_PGDN, { K(PGDN), OP_OR, J(7), OP_NONE } },
	{ "del", EVENT_DEL, { K(DEL), OP_NONE } },
	{ "ins", EVENT_INS, { K(INSERT), OP_NONE } },
	{ "exit", EVENT_EXIT, { K(ALT), K(X), OP_NONE } },
	{ "shutdown", EVENT_OFF, { K(LCONTROL), K(ESC), OP_NONE } },
	{ "mode", EVENT_MODE, { K(TAB), OP_OR, J(4), OP_NONE } },
	{ "calib", EVENT_CALIBRATION, { K(PLUS_PAD), OP_NONE } },
	{ "help", EVENT_HELP, { K(F1), OP_NONE } },
	{ "group", EVENT_GROUP, { K(F2), OP_NONE } },
	{ "type", EVENT_TYPE, { K(F3), OP_NONE } },
	{ "exclude", EVENT_ATTRIB, { K(F4), OP_NONE } },
	{ "sort", EVENT_SORT, { K(F5), OP_NONE } },
	{ "setgroup", EVENT_SETGROUP, { K(F9), OP_NONE } },
	{ "settype", EVENT_SETTYPE, { K(F10), OP_NONE } },
	{ "runclone", EVENT_CLONE, { K(F12), OP_NONE } },
	{ "command", EVENT_COMMAND, { K(F8), OP_NONE } },
	{ "menu", EVENT_MENU, { K(BACKQUOTE), OP_OR, K(BACKSLASH), OP_OR, J(2), OP_OR, M(2), OP_NONE } },
	{ "emulator", EVENT_EMU, { K(F6), OP_NONE } },
	{ "rotate", EVENT_ROTATE, { K(0_PAD), OP_NONE } },
	{ "lock", EVENT_LOCK, { K(SCRLOCK), OP_NONE } },
	{ "preview", EVENT_PREVIEW, { K(SPACE), OP_OR, J(5), OP_NONE } },
	{ "mute", EVENT_MUTE, { K(PERIOD_PAD), OP_NONE } },
	{ "volume", EVENT_VOLUME, { OP_NONE } },
	{ "difficulty", EVENT_DIFFICULTY, { OP_NONE } },
	{ 0, 0, { 0 } }
};

static int seq_pressed(const unsigned* code)
{
	int j;
	int res = 1;
	int invert = 0;
	int count = 0;

	for (j = 0; j < SEQ_MAX; ++j) {
		switch (code[j]) {
		case OP_HIDDEN:
			return 0;
		case OP_NONE:
			return res && count;
		case OP_OR:
			if (res && count)
				return 1;
			res = 1;
			count = 0;
			break;
		case OP_NOT:
			invert = !invert;
			break;
		default:
			if (res) {
				adv_bool pressed = 0;
				unsigned m = code[j] & 0xFF00;
				unsigned c = code[j] & 0xFF;
				switch (m) {
				case OP_KEY:
					for (unsigned k = 0; k < keyb_count_get(); ++k)
						if (keyb_get(k, c))
							pressed = 1;
					break;
				case OP_JOY:
					for (unsigned i = 0; i < joystickb_count_get(); ++i) {
						int b = joystickb_bind(i, c);
						if (b >= 0 && joystickb_button_get(i, b))
							pressed = 1;
					}
					break;
				case OP_MOUSE:
					for (unsigned i = 0; i < joystickb_count_get(); ++i) {
						int b = mouseb_bind(i, c);
						if (b >= 0 && mouseb_button_get(i, b))
							pressed = 1;
					}
					break;
				}
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

	// the only possible use of the hidden event
	if (seq[0] == OP_HIDDEN && seq[1] == OP_NONE)
		return 1;

	for (j = 0; j < SEQ_MAX; ++j) {
		switch (seq[j]) {
		case OP_HIDDEN:
			return 0;
		case OP_NONE:
			return positive && operand;
		case OP_OR:
			if (!operand || !positive)
				return 0;
			pred_not = 0;
			positive = 0;
			operand = 0;
			break;
		case OP_NOT:
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

	for (i = 0; EVENT_TAB[i].name && EVENT_TAB[i].event != event; ++i) ;

	if (EVENT_TAB[i].name) {
		unsigned j;
		for (j = 0; seq[j] != OP_NONE; ++j)
			EVENT_TAB[i].seq[j] = seq[j];
		EVENT_TAB[i].seq[j] = OP_NONE;
	}
}

static unsigned string2event(const string& s)
{
	unsigned i;

	for (i = 0; EVENT_TAB[i].name && EVENT_TAB[i].name != s; ++i) ;

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

	if (sevent == "snapshot") // LEGACY 2.3.7
		return true; // ignore

	unsigned event = string2event(sevent);
	if (event == EVENT_NONE)
		return false;

	seq_count = 0;
	while (pos < s.length()) {
		if (seq_count + 1 >= SEQ_MAX)
			return false;

		string skey = arg_get(s, pos);
		if (skey == "or") {
			seq[seq_count++] = OP_OR;
		} else if (skey == "not") {
			seq[seq_count++] = OP_NOT;
		} else if (skey == "hidden") {
			seq[seq_count++] = OP_HIDDEN;
		} else if (skey == "and") {
			/* nothing */
		} else if (skey.substr(0, 4) == "joy_") {
			string sbutton = skey.substr(4);
			unsigned button = joy_button_code(sbutton.c_str());
			if (button >= JOYB_MAX)
				return false;
			seq[seq_count++] = OP_JOY + button;
		} else if (skey.substr(0, 6) == "mouse_") {
			string sbutton = skey.substr(6);
			unsigned button = mouse_button_code(sbutton.c_str());
			if (button >= MOUSEB_MAX)
				return false;
			seq[seq_count++] = OP_MOUSE + button;
		} else {
			unsigned key;
			key = key_code(skey.c_str());
			// LEGACY support the old scan code format only numeric
			if (key == KEYB_MAX && skey.find_first_not_of("0123456789") == string::npos)
				key = atoi(skey.c_str());
			if (key >= KEYB_MAX)
				return false;
			seq[seq_count++] = OP_KEY + key;
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
	for (unsigned i = 0; EVENT_TAB[i].name; ++i) {
		unsigned j;
		string s;
		s += EVENT_TAB[i].name;
		s += " ";
		for (j = 0; EVENT_TAB[i].seq[j] != OP_NONE && j < SEQ_MAX; ++j) {
			unsigned m = EVENT_TAB[i].seq[j] & 0xFF00;
			unsigned c = EVENT_TAB[i].seq[j] & 0xFF;
			ostringstream os;
			const char* name;

			if (j != 0)
				s += " ";

			switch (m) {
			case OP_NONE:
				switch (c) {
				case OP_OR: s += "or"; break;
				case OP_NOT: s += "not"; break;
				case OP_HIDDEN: s += "hidden"; break;
				}
				break;
			case OP_KEY:
				s += key_name(c);
				break;
			case OP_JOY:
				os << "joy_" << joy_button_name(c);
				s += os.str();
				break;
			case OP_MOUSE:
				os << "mouse_" << mouse_button_name(c);
				s += os.str();
				break;
			}
		}

		if (j == 0)
			s += "none";

		conf_set(config_context, "", tag, s.c_str());
	}
}

bool event_is_visible(unsigned event)
{
	unsigned i;
	for (i = 0; EVENT_TAB[i].name; ++i) {
		if (EVENT_TAB[i].event == event)
			break;
	}

	if (!EVENT_TAB[i].name)
		return 1;

	return EVENT_TAB[i].seq[0] != OP_HIDDEN;
}

std::string event_name(unsigned event)
{
	string s;

	unsigned i;
	for (i = 0; EVENT_TAB[i].name; ++i) {
		if (EVENT_TAB[i].event == event)
			break;
	}

	if (!EVENT_TAB[i].name)
		return s;

	bool first = true;
	for (unsigned j = 0; EVENT_TAB[i].seq[j] != OP_NONE && j < SEQ_MAX; ++j) {
		unsigned m = EVENT_TAB[i].seq[j] & 0xFF00;
		unsigned c = EVENT_TAB[i].seq[j] & 0xFF;
		const char* name;

		switch (m) {
		case OP_NONE:
			switch (c) {
			case OP_OR: j = SEQ_MAX; break; // print only the first sequence
			case OP_NOT: s += "!"; break;
			case OP_HIDDEN: break;
			}
			first = true;
			break;
		case OP_KEY:
			if (!first)
				s += "+";
			first = false;
			s += case_upper(key_name(c));
			break;
		case OP_JOY:
			if (!first)
				s += "+";
			first = false;
			s += "j";
			s += case_upper(joy_button_name(c));
			break;
		case OP_MOUSE:
			if (!first)
				s += "+";
			first = false;
			s += "m";
			s += case_upper(mouse_button_name(c));
			break;
		}
	}

	return s;
}

void event_push_repeat(int event)
{
	if (event == EVENT_NONE)
		return;

	event_last_push = event;

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
	case EVENT_UP:
	case EVENT_DOWN:
	case EVENT_LEFT:
	case EVENT_RIGHT:
	case EVENT_PGUP:
	case EVENT_PGDN:
	case EVENT_IDLE_0:
	case EVENT_IDLE_1:
	case EVENT_IDLE_2:
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

static void event_push_filter(int event)
{
	if (event == EVENT_NONE)
		return;

	int last = event_last_push;

	event_last_push = event;

	struct event_item* i;
	for (i = EVENT_TAB; i->name; ++i) {
		if (i->event == event)
			break;
	}

	if (i->name) {
		event_push_norepeat(event, i->time, i->counter);
	} else {
		// for alphanumeric keys
		if (last != event) {
			event_last_time = 0;
			event_last_counter = 0;
		}
		event_push_norepeat(event, event_last_time, event_last_counter);
	}
}

void event_poll()
{
	for (struct event_item* i = EVENT_TAB; i->name; ++i) {
		if (seq_pressed(i->seq) || i->simulate) {
			event_push_filter(i->event);
			i->simulate = false;
		} else {
			i->counter = 0;
			i->time = 0;
		}
	}

	if (event_alpha_mode) {
		bool found = false;
		for (unsigned i = 0; EVENT_CONV[i].code != KEYB_MAX; ++i) {
			for (unsigned k = 0; k < keyb_count_get(); ++k) {
				if (keyb_get(k, EVENT_CONV[i].code)) {
					found = true;
					event_push_filter(EVENT_CONV[i].c);
				}
			}
		}
		if (!found)
			event_last_push = -1;
	}

	if (event_unassigned_mode) {
		// if anything is pressed report a generic event
		bool pressed = false;
		for (int j = 0; j < joystickb_count_get(); ++j)
			for (int b = 0; b < joystickb_button_count_get(j); ++b)
				if (joystickb_button_get(j, b))
					pressed = true;
		if (pressed)
			event_push_filter(EVENT_UNASSIGNED);
	}
}

void event_push(int event)
{
	struct event_item* i;
	for (i = EVENT_TAB; i->name; ++i) {
		if (i->event == event) {
			i->simulate = true;
			break;
		}
	}

	if (!i->name) {
		log_std(("ERROR:event: unknown event %d\n", event));
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

void event_unassigned(bool unassigned_mode)
{
	event_unassigned_mode = unassigned_mode;
}

