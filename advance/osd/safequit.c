/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Ian Patterson
 * Copyright (C) 1999-2003 Andrea Mazzoleni
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
#include "log.h"

#include "mame2.h"

#include <ctype.h>


static adv_error advance_safequit_insert_database(struct advance_safequit_context* context, char* buf, unsigned line, const char* game, adv_bool insert)
{
	char c;
	int i;
	const char* t;
	char* e;
	struct safequit_entry* entry = &context->state.entry_map[context->state.entry_mac];

	if (context->state.entry_mac >= SAFEQUIT_ENTRY_MAX) {
		target_err("Too many entries on the safequit database.\n");
		return -1;
	}

	i = 0;

	/* event (decimal 1-8 or nominal) */
	t = stoken(&c, &i, buf, ":", " \t");
	if (strcmp(t, "zerocoin") == 0) {
		entry->event = safequit_event_zerocoin;
	} else if (strcmp(t, "demomode") == 0) {
		entry->event = safequit_event_demomode;
	} else if (strcmp(t, "event1") == 0) {
		entry->event = safequit_event_event1;
	} else if (strcmp(t, "event2") == 0) {
		entry->event = safequit_event_event2;
	} else if (strcmp(t, "event3") == 0) {
		entry->event = safequit_event_event3;
	} else if (strcmp(t, "event4") == 0) {
		entry->event = safequit_event_event4;
	} else if (strcmp(t, "event5") == 0) {
		entry->event = safequit_event_event5;
	} else if (strcmp(t, "event6") == 0) {
		entry->event = safequit_event_event6;
	} else {
		goto err;
	}
	if (c != ':')
		goto err;

	/* cpu (decimal) */
	t = stoken(&c, &i, buf, ":", " \t");
	entry->cpu = strtol(t,&e,10);
	if (*e != 0 || e == t)
		goto err;
	if (c != ':')
		goto err;

	/* address (hex) */
	t = stoken(&c, &i, buf, ":", " \t");
	entry->address = strtol(t, &e, 16);
	if (*e != 0 || e == t)
		goto err;
	if (c != ':')
		goto err;

	/* action (nominal) */
	t = stoken(&c, &i, buf, ":", " \t");
	if (strcmp(t, "match") == 0) {
		entry->action = safequit_action_match;
	} else if (strcmp(t, "nomatch") == 0) {
		entry->action = safequit_action_nomatch;
	} else if (strcmp(t, "on") == 0) {
		entry->action = safequit_action_on;
	} else if (strcmp(t, "off") == 0) {
		entry->action = safequit_action_off;
	} else {
		goto err;
	}
	if (c != ':')
		goto err;

	/* mask (hex) */
	t = stoken(&c, &i, buf, ":", " \t");
	entry->mask = strtol(t, &e, 16);
	if (*e != 0 || e == t)
		goto err;
	if (c != ':')
		goto err;

	/* result (hex) */
	t = stoken(&c, &i, buf, "", " \t");
	entry->result = strtol(t, &e, 16);
	if (*e != 0 || e == t)
		goto err;
	if (c != 0)
		goto err;

	entry->frame_count = 0;

	if (insert) {
		log_std(("advance:safequit: entry %d:%d:%x:%d:%x:%x\n", (unsigned)entry->event, (unsigned)entry->cpu, (unsigned)entry->address, (unsigned)entry->action, (unsigned)entry->mask, (unsigned)entry->result));
		++context->state.entry_mac;
	}

	return 0;

err:
	target_err("Error parsing the safequit line %d for game '%s'.\n", line, game);
	return -1;
}

static adv_error advance_safequit_load_database(struct advance_safequit_context* context, const char* file, const char* game_name)
{
	mame_file* f;
	char buffer[2048];
	char game_name_buffer[32];
	unsigned line = 1;
	adv_bool match;
	adv_bool def;

	f = mame_fopen(NULL, file, FILETYPE_HISTORY, 0);
	if (!f) {
		target_err("Error opening the safequit database %s.\n", file);
		goto err;
	}

	match = 0;
	def = 0;

	while (mame_fgets(buffer, sizeof(buffer), f) != NULL)
	{
		unsigned len = strlen(buffer);

		/* remove spaces at the end */
		while (len>0 && isspace(buffer[len-1]))
			buffer[--len] = 0;

		if (len>0 && (buffer[0]==';' || buffer[0] == '#')) {
			/* ignore comment line */
		} else if (len>0 && strchr(buffer,':') == &buffer[len-1]) {
			buffer[len-1] = 0; /* remove the : */
			sncpy(game_name_buffer, sizeof(game_name_buffer), buffer);
			if (!def)
				match = 0;
			if (strcmp(game_name_buffer, game_name) == 0)
				match = 1;
			def = 1;
		} else if (len > 0) {
			/* entry def */
			if (advance_safequit_insert_database(context, buffer, line, game_name_buffer, match) != 0)
				goto err;
			def = 0;
		}

		++line;
	}

done:
	mame_fclose(f);
	return 0;

err_close:
	mame_fclose(f);
err:
	return -1;
}

static adv_bool advance_safequit_is_entry_set(struct safequit_entry* entry, unsigned char result)
{
	switch (entry->action) {
	case safequit_action_match :
		if ((result & entry->mask) == entry->result) {
			return 1;
		}
		break;
	case safequit_action_nomatch :
		if ((result & entry->mask) != entry->result) {
			return 1;
		}
		break;
	case safequit_action_on :
		return 1;
	case safequit_action_off :
		return 0;
	}

	return 0;
}

adv_error advance_safequit_init(struct advance_safequit_context* context, adv_conf* cfg_context)
{
	conf_bool_register_default(cfg_context, "misc_safequit", 1);
	conf_bool_register_default(cfg_context, "misc_eventdebug", 0);
	conf_string_register_default(cfg_context, "misc_eventfile", "event.dat");
	return 0;
}

void advance_safequit_done(struct advance_safequit_context* context)
{
}

adv_error advance_safequit_inner_init(struct advance_safequit_context* context, struct mame_option* option)
{
	context->state.entry_mac = 0;
	context->state.status = 0;
	context->state.coin_set = 0;
	context->state.coin_format = safequit_format_bcd;

	if (!context->config.safe_exit_flag)
		return 0;

	if (advance_safequit_load_database(context, context->config.file_buffer, mame_game_name(option->game)) != 0)
		return -1;

	return 0;
}

void advance_safequit_inner_done(struct advance_safequit_context* context)
{
}

adv_error advance_safequit_config_load(struct advance_safequit_context* context, adv_conf* cfg_context)
{
	context->config.safe_exit_flag = conf_bool_get_default(cfg_context, "misc_safequit");
	context->config.debug_flag = conf_bool_get_default(cfg_context, "misc_eventdebug");
	sncpy(context->config.file_buffer, sizeof(context->config.file_buffer), conf_string_get_default(cfg_context, "misc_eventfile"));

	return 0;
}

void advance_safequit_event(struct advance_safequit_context* context, struct safequit_entry* entry, unsigned char result)
{
	if (advance_safequit_is_entry_set(entry, result)) {
		if (entry->frame_count < Machine->drv->frames_per_second) {
			entry->frame_count++;
			context->state.status &= ~(1 << entry->event);
		}
	} else {
		entry->frame_count = 0;
		context->state.status &= ~(1 << entry->event);
	}
}

void advance_safequit_coin(struct advance_safequit_context* context, struct safequit_entry* entry, unsigned char result)
{
	/* try to use the zero_coin rules to detect the number of coins */
	if (entry->event == safequit_event_zerocoin
		&& entry->action == safequit_action_match) {
		if (entry->result == 0 && (entry->mask == 0xf || entry->mask == 0xff)) {
			unsigned v = result & entry->mask;
			if (context->state.coin_format == safequit_format_bcd) {
				/* check if the value is in the bcd range */
				if ((v & 0xf) > 9 || ((v & 0xf0) >> 4) > 9) {
					context->state.coin_format = safequit_format_byte;
				} else {
					v = ((v & 0xf0) >> 4) * 10 + (v & 0xF);
				}
				if (context->state.coin_set)
					context->state.coin += v;
				else
					context->state.coin = v;
				context->state.coin_set = 1;
			} else if (context->state.coin_format == safequit_format_byte) {
				if (context->state.coin_set)
					context->state.coin += v;
				else
					context->state.coin = v;
				context->state.coin_set = 1;
			}
		} else if ((result & entry->mask) == entry->result) {
			/* unknown */
			context->state.coin_set = 1;
			context->state.coin = 0;
		}
	}
}

void advance_safequit_update(struct advance_safequit_context* context)
{
	unsigned i;

	context->state.coin_set = 0;
	context->state.status = 0xff;

	for(i=0;i<context->state.entry_mac;++i) {
		struct safequit_entry* entry = &context->state.entry_map[i];
		unsigned char result = cpunum_read_byte(entry->cpu, entry->address);

		advance_safequit_event(context, entry, result);
		advance_safequit_coin(context, entry, result);
	}

	if (context->config.debug_flag) {
		char buffer[16];

		buffer[0] = (context->state.status & 0x01) ? 'Z' : '_';
		buffer[1] = (context->state.status & 0x02) ? 'D' : '_';
		buffer[2] = (context->state.status & 0x04) ? '1' : '_';
		buffer[3] = (context->state.status & 0x08) ? '2' : '_';
		buffer[4] = (context->state.status & 0x10) ? '3' : '_';
		buffer[5] = (context->state.status & 0x20) ? '4' : '_';
		buffer[6] = (context->state.status & 0x40) ? '5' : '_';
		buffer[7] = (context->state.status & 0x80) ? '6' : '_';
		buffer[8] = 0;

		if (context->state.coin_set) {
			sncatf(buffer, sizeof(buffer), "-%d", context->state.coin);
		}

		mame_ui_text(buffer, 0, 0);
	}
}

adv_bool advance_safequit_can_exit(struct advance_safequit_context* context)
{
	if (!context->config.safe_exit_flag)
		return 1;

	return context->state.entry_mac == 0 || (context->state.status & 0x3) == 3;
}

adv_bool advance_safequit_event_mask(struct advance_safequit_context* context)
{
	if (context->state.entry_mac == 0)
		return 0;

	return context->state.status;
}
