/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002 Ian Patterson
 * Copyright (C) 1999-2002 Andrea Mazzoleni
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
#include "mame2.h"

#include <ctype.h>

#define MAX_ACTIONS 64

enum {
	kEvent_ZeroCoin = 0,
	kEvent_DemoMode
};

enum {
	kAction_Match = 0,
	kAction_NoMatch = 1,
	kAction_Match_Master = 10,
	kAction_NoMatch_Master = 11
};

struct ActionEntry {
	UINT8 event;
	UINT8 cpu;
	UINT32 address;
	UINT8 action;
	UINT8 mask;
	UINT8 result;

	UINT32 frameCount;
};

typedef struct ActionEntry ActionEntry;

static ActionEntry entryList[MAX_ACTIONS + 1];
static UINT32 entryListLength;
static UINT32 safeQuitStatus;

static void AddDatabaseEntry(char * buf)
{
	char event[4096];
	int cpu;
	unsigned address;
	int action;
	unsigned mask;
	unsigned result;

	if(entryListLength >= MAX_ACTIONS)
		return;

	if (!buf[0])
		return;

	if (sscanf(buf, "%[^:]:%d:%x:%d:%x:%x", event, &cpu, &address, &action, &mask, &result) != 6)
		return;

	if (strcmp(event, "zero_coin") == 0) {
		entryList[entryListLength].event = kEvent_ZeroCoin;
	} else if (strcmp(event, "demo_mode") == 0) {
		entryList[entryListLength].event = kEvent_DemoMode;
	} else {
		return;
	}

	entryList[entryListLength].cpu = cpu;
	entryList[entryListLength].address = address;
	entryList[entryListLength].action = action;
	entryList[entryListLength].mask = mask;
	entryList[entryListLength].result = result;

	logerror("%s:%d:%x:%d:%x:%x\n", event, cpu, address, action, mask, result);

	entryListLength++;
}

static void LoadSafeQuitDatabase(const char* file, const char* game_name)
{
	mame_file* theFile;
	char buffer[2048];
	char game_name_buffer[32];
	UINT8 foundGameName = 0;

	theFile = mame_fopen(NULL, file, FILETYPE_HISTORY, 0);

	if (!theFile)
		return;

	snprintf(game_name_buffer, sizeof(game_name_buffer), "%s:", game_name);

	while ((mame_fgets(buffer, 2048, theFile) != NULL) && (entryListLength < MAX_ACTIONS))
	{
		unsigned len = strlen(buffer);

		/* remove spaces at the end */
		while (len>0 && isspace(buffer[len-1]))
			buffer[--len] = 0;

		if (foundGameName) {
			/* terminate on empty line */
			if(!buffer[0])
				goto done;
			AddDatabaseEntry(buffer);
		} else {
			if (strcmp(buffer, game_name_buffer) == 0) {
				foundGameName = 1;
			}
		}
	}

	done:

	mame_fclose(theFile);
}

static int ConditionSatisfied(ActionEntry * action)
{
	int data = cpunum_read_byte(action->cpu, action->address);

	switch (action->action) {
		case kAction_Match:
			if ((data & action->mask) == action->result) {
				return 1;
			}
			break;
		case kAction_NoMatch:
			if ((data & action->mask) != action->result) {
				return 1;
			}
			break;
		case kAction_Match_Master:
			break;
		case kAction_NoMatch_Master:
			break;
	}

	return 0;
}

adv_error advance_safequit_init(struct advance_safequit_context* context, adv_conf* cfg_context)
{
	conf_bool_register_default(cfg_context, "misc_safequit", 1);
	conf_bool_register_default(cfg_context, "misc_safequitdebug", 0);
	conf_string_register_default(cfg_context, "misc_safequitfile", "safequit.dat");
	return 0;
}

void advance_safequit_done(struct advance_safequit_context* context)
{
}

adv_error advance_safequit_inner_init(struct advance_safequit_context* context, struct mame_option* option)
{
	if (!context->config.safe_exit_flag)
		return 0;

	entryListLength = 0;
	safeQuitStatus = 0;
	memset(entryList, 0, sizeof(ActionEntry) * MAX_ACTIONS);

	LoadSafeQuitDatabase(context->config.file_buffer, mame_game_name(option->game));

	return 0;
}

void advance_safequit_inner_done(struct advance_safequit_context* context)
{
}

adv_error advance_safequit_config_load(struct advance_safequit_context* context, adv_conf* cfg_context)
{
	context->config.safe_exit_flag = conf_bool_get_default(cfg_context, "misc_safequit");
	context->config.debug_flag = conf_bool_get_default(cfg_context, "misc_safequitdebug");
	snprintf(context->config.file_buffer, sizeof(context->config.file_buffer), "%s", conf_string_get_default(cfg_context, "misc_safequitfile"));

	return 0;
}

void advance_safequit_update(struct advance_safequit_context* context)
{
	int i;
	int good = 3;

	if (!context->config.safe_exit_flag)
		return;

	for(i=0;i<entryListLength;++i) {
		if (ConditionSatisfied(&entryList[i])) {
			if (entryList[i].frameCount < Machine->drv->frames_per_second) {
				entryList[i].frameCount++;
				good &= ~(1 << entryList[i].event);
			}
		} else {
			entryList[i].frameCount = 0;
			good &= ~(1 << entryList[i].event);
		}
	}

	safeQuitStatus = good;

	if (context->config.debug_flag) {
		char buffer[4];

		buffer[0] = (safeQuitStatus & 1) ? '1' : '0';
		buffer[1] = (safeQuitStatus & 2) ? '1' : '0';
		buffer[2] = 0;

		mame_ui_text(buffer, 0, 0);
	}
}

adv_bool advance_safequit_can_exit(struct advance_safequit_context* context)
{
	if (!context->config.safe_exit_flag)
		return 1;

	return entryListLength == 0 || safeQuitStatus == 3;
}

