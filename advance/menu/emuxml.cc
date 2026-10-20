/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2008 Andrea Mazzoleni
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

#include "emulator.h"
#include "game.h"

#include <expat.h>

#include "advance.h"

#include <string>
#include <iostream>

using namespace std;

/****************************************************************************/
/* Parser */

/**
 * Max depth checked.
 */
#define DEPTH_MAX 5

enum token_t {
	token_open,
	token_close,
	token_data
};

struct state_t;

typedef void (process_t)(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes);

/**
 * State for every depth level.
 */
struct level_t {
	const char* tag; /**< Tag name. */
	char local_buf[512]; /**< Small inline buffer to avoid heap allocations. */
	char* data; /**< Accumulative data. */
	unsigned len; /**< Length of the data. */
	unsigned cap; /**< Allocated capacity. */
	process_t* process; /**< Processing function. */
};

/**
 * Global parsing state.
 */
struct state_t {
	XML_Parser parser; /**< Parser. */
	int depth; /**< Current depth. */
	int ignore_depth; /**< Depth of ignored subtrees. */
	struct level_t level[DEPTH_MAX]; /**< Level state. */
	int error; /**< Error flag. */
	emulator* e; /**< Current parent emulator. */
	game* g; /**< Curren game in the loading process. */
	game_set* a; /**< Game archive. */
	machinedevice* m; /**< Machine device. */
	unsigned rom_size; /**< Size of the current rom. */
	bool rom_merge; /**< Merge of the current rom. */
	string user_prefix; /**< Precomputed emulator user prefix. */
	game_set::iterator last_inserted; /**< Hint iterator for faster sorted insertion. */
};

/****************************************************************************/
/* Processing */

static void process_error(struct state_t* state, const char* tag, const char* msg)
{
	int line = XML_GetCurrentLineNumber(state->parser);
	if (tag)
		target_err("Error reading at line %d for element/attribute `%s' for %s.\n", line, tag, msg);
	else
		target_err("Error reading at line %d for %s.\n", line, msg);
	state->error = 1;
}

static void process_game(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		state->g = new game;
		state->g->emulator_set(state->e);
	} else if (t == token_close) {
		state->last_inserted = state->a->insert(state->last_inserted, *state->g);
		delete state->g;
		state->g = 0;
	}
}

static void process_runnable(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		if (len == 2 && s[0] == 'n' && s[1] == 'o')
			state->g->flag_set(true, emulator::flag_derived_resource);
	}
}

static void process_isbios(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		if (len == 3 && s[0] == 'y' && s[1] == 'e' && s[2] == 's')
			state->g->flag_set(true, emulator::flag_derived_resource);
	}
}

static void process_isdevice(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		if (len == 3 && s[0] == 'y' && s[1] == 'e' && s[2] == 's')
			state->g->flag_set(true, emulator::flag_derived_resource);
	}
}

static void process_name(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		string n = state->user_prefix;
		n.append(s, len);
		state->g->name_set(n);
	}
}

static void process_description(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->auto_description_set(string(s, len));
	}
}

static void process_manufacturer(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->manufacturer_set(string(s, len));
	}
}

static void process_year(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->year_set(string(s, len));
	}
}

static void process_cloneof(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		string n = state->user_prefix;
		n.append(s, len);
		state->g->cloneof_set(n);
	}
}

static void process_romof(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		string n = state->user_prefix;
		n.append(s, len);
		state->g->romof_set(n);
	}
}

static void process_driverstatus(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		if (len == 11 && memcmp(s, "preliminary", 11) == 0)
			state->g->play_set(play_preliminary);
		if (len == 9 && memcmp(s, "imperfect", 9) == 0 && state->g->play_get() < play_imperfect)
			state->g->play_set(play_imperfect);
	}
}

static void process_rom(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		state->rom_merge = false;
		state->rom_size = 0;
	} else if (t == token_close) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		if (!state->rom_merge)
			state->g->size_set(state->g->size_get() + state->rom_size);
	}
}

static void process_romsize(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->rom_size = strtoul(s, 0, 10);
	}
}

static void process_rommerge(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->rom_merge = 1;
	}
}

static void process_device(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		state->m = new machinedevice;
	} else if (t == token_close) {
		if (!state->g || !state->m) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->machinedevice_bag_get().insert(state->g->machinedevice_bag_get().end(), *state->m);
		delete state->m;
		state->m = 0;
	}
}

static void process_deviceextensionname(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->m) {
			process_error(state, 0, "invalid state");
			return;
		}
		string v = string(s, len);
		state->m->ext_bag.insert(state->m->ext_bag.end(), v);
	}
}

static void process_devicename(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->m) {
			process_error(state, 0, "invalid state");
			return;
		}
		string v = string(s, len);
		state->m->name = v;
	}
}

static void process_videoscreen(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->flag_set(len == 6 && memcmp(s, "vector", 6) == 0, emulator::flag_derived_vector);
	}
}

static void process_videoorientation(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->flag_set(len == 8 && memcmp(s, "vertical", 8) == 0, emulator::flag_derived_vertical);
	}
}

static void process_videowidth(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->sizex_set(strtoul(s, 0, 10));
	}
}

static void process_videoheight(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->sizey_set(strtoul(s, 0, 10));
	}
}

static void process_videoaspectx(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->aspectx_set(strtoul(s, 0, 10));
	}
}

static void process_videoaspecty(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		if (!state->g) {
			process_error(state, 0, "invalid state");
			return;
		}
		state->g->aspecty_set(strtoul(s, 0, 10));
	}
}

static const char* match_mamemessraine = "mame|mess|raine";
static const char* match_gamemachine = "game|machine";

/**
 * Conversion table.
 * Any element/attribute not in this table is ignored.
 */
struct conversion_t {
	unsigned depth;
	const char* name[DEPTH_MAX];
	process_t* process;
};

static struct conversion_t CONV1[] = {
	{ 1, { match_mamemessraine, match_gamemachine, 0, 0, 0 }, process_game },
	{ 0, { 0, 0, 0, 0, 0 }, 0 }
};

static struct conversion_t CONV2[] = {
	{ 2, { match_mamemessraine, match_gamemachine, "runnable", 0, 0 }, process_runnable },
	{ 2, { match_mamemessraine, match_gamemachine, "isbios", 0, 0 }, process_isbios },
	{ 2, { match_mamemessraine, match_gamemachine, "isdevice", 0, 0 }, process_isdevice },
	{ 2, { match_mamemessraine, match_gamemachine, "name", 0, 0 }, process_name },
	{ 2, { match_mamemessraine, match_gamemachine, "description", 0, 0 }, process_description },
	{ 2, { match_mamemessraine, match_gamemachine, "manufacturer", 0, 0 }, process_manufacturer },
	{ 2, { match_mamemessraine, match_gamemachine, "year", 0, 0 }, process_year },
	{ 2, { match_mamemessraine, match_gamemachine, "cloneof", 0, 0 }, process_cloneof },
	{ 2, { match_mamemessraine, match_gamemachine, "romof", 0, 0 }, process_romof },
	{ 2, { match_mamemessraine, match_gamemachine, "rom", 0, 0 }, process_rom },
	{ 2, { match_mamemessraine, match_gamemachine, "device", 0, 0 }, process_device },
	{ 2, { match_mamemessraine, match_gamemachine, "driver", 0, 0 }, 0 },
	{ 2, { match_mamemessraine, match_gamemachine, "video", 0, 0 }, 0 },
	{ 0, { 0, 0, 0, 0, 0 }, 0 }
};

static struct conversion_t CONV3[] = {
	{ 3, { match_mamemessraine, match_gamemachine, "rom", "merge", 0 }, process_rommerge },
	{ 3, { match_mamemessraine, match_gamemachine, "rom", "size", 0 }, process_romsize },
	{ 3, { match_mamemessraine, match_gamemachine, "device", "name", 0 }, process_devicename },
	{ 3, { match_mamemessraine, match_gamemachine, "device", "type", 0 }, process_devicename }, /* from MESS 0.106, instead of "name" */
	{ 3, { match_mamemessraine, match_gamemachine, "device", "extension", 0 }, 0 },
	{ 3, { match_mamemessraine, match_gamemachine, "driver", "status", 0 }, process_driverstatus },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "screen", 0 }, process_videoscreen },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "orientation", 0 }, process_videoorientation },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "width", 0 }, process_videowidth },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "height", 0 }, process_videoheight },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "aspectx", 0 }, process_videoaspectx },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "aspecty", 0 }, process_videoaspecty },
	{ 0, { 0, 0, 0, 0, 0 }, 0 }
};

static struct conversion_t CONV4[] = {
	{ 4, { match_mamemessraine, match_gamemachine, "device", "extension", "name" }, process_deviceextensionname },
	{ 0, { 0, 0, 0, 0, 0 }, 0 }
};

/**
 * Check if an XML tag is a relevant child element of <game> or <machine>.
 *
 * Quickly filters out unsupported or unwanted XML elements at depth 1
 * (such as dipswitch, configuration, port, chip, softwarelist, slot),
 * allowing the parser to skip entire subtrees early.
 */
static inline bool is_game_child_tag(const char* name)
{
	switch (name[0]) {
	case 'd':
		return strcmp(name, "description") == 0
			|| strcmp(name, "driver") == 0
			|| strcmp(name, "device") == 0;
	case 'y':
		return strcmp(name, "year") == 0;
	case 'm':
		return strcmp(name, "manufacturer") == 0;
	case 'r':
		return strcmp(name, "rom") == 0
			|| strcmp(name, "romof") == 0
			|| strcmp(name, "runnable") == 0;
	case 'v':
		return strcmp(name, "video") == 0;
	case 'n':
		return strcmp(name, "name") == 0;
	case 'c':
		return strcmp(name, "cloneof") == 0;
	case 'i':
		return strcmp(name, "isbios") == 0
			|| strcmp(name, "isdevice") == 0;
	default:
		return false;
	}
}

/**
 * Direct processing of XML element attributes.
 *
 * Bypasses the generic hierarchical conversion table to quickly extract
 * game metadata stored directly as XML attributes in modern MAME XML formats.
 */
static inline void process_attributes_direct(struct state_t* state, const XML_Char* tag, const XML_Char** attributes)
{
	if (!attributes || !attributes[0])
		return;

	if (strcmp(tag, "game") == 0 || strcmp(tag, "machine") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			const char* v = attributes[i + 1];
			unsigned l = strlen(v);
			if (strcmp(k, "name") == 0)
				process_name(state, token_data, v, l, 0);
			else if (strcmp(k, "cloneof") == 0)
				process_cloneof(state, token_data, v, l, 0);
			else if (strcmp(k, "romof") == 0)
				process_romof(state, token_data, v, l, 0);
			else if (strcmp(k, "runnable") == 0)
				process_runnable(state, token_data, v, l, 0);
			else if (strcmp(k, "isbios") == 0)
				process_isbios(state, token_data, v, l, 0);
			else if (strcmp(k, "isdevice") == 0)
				process_isdevice(state, token_data, v, l, 0);
		}
	} else if (strcmp(tag, "rom") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			const char* v = attributes[i + 1];
			unsigned l = strlen(v);
			if (strcmp(k, "size") == 0)
				process_romsize(state, token_data, v, l, 0);
			else if (strcmp(k, "merge") == 0)
				process_rommerge(state, token_data, v, l, 0);
		}
	} else if (strcmp(tag, "driver") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			const char* v = attributes[i + 1];
			if (strcmp(k, "status") == 0)
				process_driverstatus(state, token_data, v, strlen(v), 0);
		}
	} else if (strcmp(tag, "video") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			const char* v = attributes[i + 1];
			unsigned l = strlen(v);
			if (strcmp(k, "screen") == 0)
				process_videoscreen(state, token_data, v, l, 0);
			else if (strcmp(k, "orientation") == 0)
				process_videoorientation(state, token_data, v, l, 0);
			else if (strcmp(k, "width") == 0)
				process_videowidth(state, token_data, v, l, 0);
			else if (strcmp(k, "height") == 0)
				process_videoheight(state, token_data, v, l, 0);
			else if (strcmp(k, "aspectx") == 0)
				process_videoaspectx(state, token_data, v, l, 0);
			else if (strcmp(k, "aspecty") == 0)
				process_videoaspecty(state, token_data, v, l, 0);
		}
	} else if (strcmp(tag, "device") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			if (strcmp(k, "name") == 0 || strcmp(k, "type") == 0)
				process_devicename(state, token_data, attributes[i + 1], strlen(attributes[i + 1]), 0);
		}
	} else if (strcmp(tag, "extension") == 0) {
		for (int i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			if (strcmp(k, "name") == 0)
				process_deviceextensionname(state, token_data, attributes[i + 1], strlen(attributes[i + 1]), 0);
		}
	}
}

/**
 * Identify the conversion rule for the current XML tag hierarchy.
 *
 * Finds the matching rule in conversion tables (CONV1..CONV4) corresponding
 * to the current element hierarchy in the XML tree.
 */
static struct conversion_t* identify(unsigned depth, const struct level_t* level)
{
	struct conversion_t* conv;

	switch (depth) {
	case 1: conv = CONV1; break;
	case 2: conv = CONV2; break;
	case 3: conv = CONV3; break;
	case 4: conv = CONV4; break;
	default:
		return 0;
	}

	for (unsigned i = 0; conv[i].name[0]; ++i) {
		bool equal = true;

		// check all the item, backward
		for (int j = depth; equal && j >= 0; --j) {
			if (conv[i].name[j] == match_mamemessraine) {
				if (strcmp(level[j].tag, "mame") != 0 && strcmp(level[j].tag, "mess") != 0 && strcmp(level[j].tag, "raine") != 0)
					equal = false;
			} else if (conv[i].name[j] == match_gamemachine) {
				if (strcmp(level[j].tag, "game") != 0 && strcmp(level[j].tag, "machine") != 0)
					equal = false;
			} else {
				if (strcmp(level[j].tag, conv[i].name[j]) != 0)
					equal = false;
			}
		}

		// if match return
		if (equal)
			return &conv[i];
	}

	return 0;
}

/**
 * End element callback for the Expat parser.
 *
 * Handles closing XML tags (</tag>), finalizing data processing and cleaning up
 * per-level state and buffers.
 */
static void end_handler(void* data, const XML_Char* name)
{
	struct state_t* state = (struct state_t*)data;

	if (state->ignore_depth > 0) {
		--state->ignore_depth;
		return;
	}

	if (state->depth < DEPTH_MAX && state->depth >= 0) {
		if (state->error == 0) {
			if (state->level[state->depth].process) {
				const char* d = state->level[state->depth].data ? state->level[state->depth].data : "";
				unsigned l = state->level[state->depth].len;
				state->level[state->depth].process(state, token_data, d, l, 0);
				state->level[state->depth].process(state, token_close, 0, 0, 0);
			}
		}
		if (state->level[state->depth].data && state->level[state->depth].data != state->level[state->depth].local_buf) {
			free(state->level[state->depth].data);
		}
		state->level[state->depth].data = 0;
		state->level[state->depth].len = 0;
		state->level[state->depth].cap = 0;
	}

	--state->depth;
}

/**
 * Character data callback for the Expat parser.
 *
 * Accumulates text content enclosed inside XML elements.
 */
void data_handler(void* data, const XML_Char* s, int len)
{
	struct state_t* state = (struct state_t*)data;

	if (state->ignore_depth > 0 || state->depth >= DEPTH_MAX || state->depth < 0)
		return;
	if (state->error != 0 || state->level[state->depth].process == 0)
		return;

	struct level_t* lvl = &state->level[state->depth];
	unsigned required = lvl->len + len + 1;
	if (lvl->data == lvl->local_buf) {
		if (required <= sizeof(lvl->local_buf)) {
			memcpy(lvl->data + lvl->len, s, len);
			lvl->len += len;
			lvl->data[lvl->len] = 0;
			return;
		}
		lvl->cap = required * 2;
		lvl->data = (char*)malloc(lvl->cap);
		if (!lvl->data) {
			process_error(state, lvl->tag, "low memory");
			return;
		}
		memcpy(lvl->data, lvl->local_buf, lvl->len);
	} else if (!lvl->data) {
		if (required <= sizeof(lvl->local_buf)) {
			lvl->data = lvl->local_buf;
			memcpy(lvl->data, s, len);
			lvl->len = len;
			lvl->data[len] = 0;
			return;
		}
		lvl->cap = required * 2;
		lvl->data = (char*)malloc(lvl->cap);
		if (!lvl->data) {
			process_error(state, lvl->tag, "low memory");
			return;
		}
	} else if (required > lvl->cap) {
		lvl->cap = required * 2;
		char* newdata = (char*)realloc(lvl->data, lvl->cap);
		if (!newdata) {
			free(lvl->data);
			lvl->data = 0;
			process_error(state, lvl->tag, "low memory");
			return;
		}
		lvl->data = newdata;
	}
	memcpy(lvl->data + lvl->len, s, len);
	lvl->len += len;
	lvl->data[lvl->len] = 0;
}

/**
 * Start element callback for the Expat parser.
 *
 * Handles opening XML tags (<tag ...>), managing subtree skipping, tracking
 * parser hierarchy depth, and dispatching element and attribute handlers.
 */
static void start_handler(void* data, const XML_Char* name, const XML_Char** attributes)
{
	struct state_t* state = (struct state_t*)data;

	if (state->ignore_depth > 0) {
		++state->ignore_depth;
		return;
	}

	if (state->depth == 1) {
		// Children of game/machine (depth 1)
		if (!is_game_child_tag(name)) {
			state->ignore_depth = 1;
			return;
		}
	} else if (state->depth == 2) {
		if (state->level[2].tag && strcmp(state->level[2].tag, "device") == 0) {
			if (strcmp(name, "extension") != 0 && strcmp(name, "name") != 0 && strcmp(name, "type") != 0) {
				state->ignore_depth = 1;
				return;
			}
		}
	}

	if (state->depth + 1 >= DEPTH_MAX) {
		state->ignore_depth = 1;
		return;
	}

	++state->depth;

	state->level[state->depth].tag = name;
	state->level[state->depth].data = 0;
	state->level[state->depth].len = 0;
	state->level[state->depth].cap = 0;
	state->level[state->depth].process = 0;

	if (state->error == 0) {
		struct conversion_t* c = identify(state->depth, state->level);
		if (c) {
			state->level[state->depth].process = c->process;
			if (state->level[state->depth].process)
				state->level[state->depth].process(state, token_open, 0, 0, attributes);
		}
		process_attributes_direct(state, name, attributes);
	}
}

bool mame_info::load_xml(istream& is, game_set& gar)
{
	struct state_t state;
	const int BUF_SIZE = 64 * 1024;

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser) {
		return false;
	}

	state.depth = -1;
	state.ignore_depth = 0;
	state.error = 0;
	state.e = this;
	state.g = 0;
	state.a = &gar;
	state.m = 0;
	state.user_prefix = user_name_get() + "/";
	state.last_inserted = gar.end();

	for (int i = 0; i < DEPTH_MAX; ++i) {
		state.level[i].tag = 0;
		state.level[i].data = 0;
		state.level[i].len = 0;
		state.level[i].cap = 0;
		state.level[i].process = 0;
	}

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);
	XML_SetParamEntityParsing(state.parser, XML_PARAM_ENTITY_PARSING_NEVER);

	while (1) {
		void* buf = XML_GetBuffer(state.parser, BUF_SIZE);
		if (!buf) {
			process_error(&state, "", "out of memory");
			break;
		}

		is.read((char*)buf, BUF_SIZE);
		if (is.bad()) {
			process_error(&state, "", "read error");
			break;
		}

		int len = is.gcount();
		bool done = is.eof();

		if (XML_ParseBuffer(state.parser, len, done) == XML_STATUS_ERROR) {
			process_error(&state, "", XML_ErrorString(XML_GetErrorCode(state.parser)));
			break;
		}

		if (done)
			break;
	}

	XML_ParserFree(state.parser);

	if (state.g) {
		delete state.g;
		state.g = 0;
	}
	if (state.m) {
		delete state.m;
		state.m = 0;
	}

	if (state.error) {
		return false;
	}

	return true;
}

