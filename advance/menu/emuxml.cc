#include "emulator.h"
#include "game.h"
#include "target.h"

// Include the internal copy of the libexpat library
#include "../expat/expat.h"

#include <string>
#include <iostream>

using namespace std;

/****************************************************************************/
/* Parser */

/**
 * Max depth checked.
 */
#define DEPTH_MAX 4

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
	char* data; /**< Accumulative data. */
	unsigned len; /**< Length of the data. */
	process_t* process; /**< Processing function. */
};

/**
 * Global parsing state.
 */
struct state_t {
	XML_Parser parser; /**< Parser. */
	int depth; /**< Current depth. */
	struct level_t level[DEPTH_MAX]; /**< Level state. */
	int error; /**< Error flag. */
	emulator* e; /**< Current parent emulator. */
	game* g; /**< Curren game in the loading process. */
	game_set* a; /**< Game archive. */
	unsigned rom_size; /**< Size of the current rom. */
	bool rom_merge; /**< Merge of the current rom. */

};

/****************************************************************************/
/* Processing */

static void process_error(struct state_t* state, const char* tag, const char* msg)
{
	if (*tag)
		target_err("Error reading at line %d for element/attribute `%s' for %s\n", XML_GetCurrentLineNumber(state->parser), tag, msg);
	else
		target_err("Error reading at line %d for %s\n", XML_GetCurrentLineNumber(state->parser), msg);
	state->error = 1;
}

static void process_game(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		state->g = new game;
		state->g->emulator_set(state->e);
	} else if (t == token_close) {
		state->a->insert( *state->g );
		delete state->g;
		state->g = 0;
	}
}

static void process_runnable(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->flag_set(v == "no", emulator::flag_derived_resource);
	}
}

static void process_name(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->name_set( state->e->user_name_get() + "/" + string(s, len) );
	}
}

static void process_description(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->auto_description_set( string(s, len) );
	}
}

static void process_manufacturer(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->manufacturer_set( string(s, len) );
	}
}

static void process_year(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->year_set( string(s, len) );
	}
}

static void process_cloneof(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->cloneof_set( state->e->user_name_get() + "/" + string(s, len) );
	}
}

static void process_romof(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->g->romof_set( state->e->user_name_get() + "/" + string(s, len) );
	}
}

static void process_driverstatus(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		if (v == "preliminary")
			state->g->play_set(play_not);
	}
}

static void process_drivercolor(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		if (v == "preliminary")
			if (state->g->play_get() < play_major)
				state->g->play_set(play_major);
	}
}

static void process_driversound(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		if (v == "preliminary")
			if (state->g->play_get() < play_minor)
				state->g->play_set(play_minor);
	}
}

static void process_rom(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_open) {
		state->rom_merge = false;
		state->rom_size = 0;
	} else if (t == token_close) {
		if (!state->rom_merge)
			state->g->size_set( state->g->size_get() + state->rom_size );
	}
}

static void process_romsize(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->rom_size = atoi(v.c_str());
	}
}

static void process_rommerge(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		state->rom_merge = 1;
	}
}

static void process_videoscreen(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->flag_set(v ==  "vector", emulator::flag_derived_vector );
	}
}

static void process_videoorientation(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->flag_set(v == "vertical", emulator::flag_derived_vertical);
	}
}

static void process_videowidth(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->sizex_set( atoi(v.c_str()) );
	}
}

static void process_videoheight(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->sizey_set( atoi(v.c_str()) );
	}
}

static void process_videoaspectx(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->aspectx_set( atoi(v.c_str()) );
	}
}

static void process_videoaspecty(struct state_t* state, enum token_t t, const char* s, unsigned len, const char** attributes)
{
	if (t == token_data) {
		string v = string(s, len);
		state->g->aspecty_set( atoi(v.c_str()) );
	}
}

static const char* match_mamemessraine = "mame|mess|raine";
static const char* match_gamemachine = "game|machine";

/**
 * Conversion table.
 * Any element/attribute not in this table is ignored.
 */
static struct conversion_t {
	unsigned depth;
	const char* name[DEPTH_MAX];
	process_t* process;
} CONV[] = {
	{ 1, { match_mamemessraine, match_gamemachine, 0, 0 }, process_game },
	{ 2, { match_mamemessraine, match_gamemachine, "runnable", 0 }, process_runnable },
	{ 2, { match_mamemessraine, match_gamemachine, "name", 0 }, process_name },
	{ 2, { match_mamemessraine, match_gamemachine, "description", 0 }, process_description },
	{ 2, { match_mamemessraine, match_gamemachine, "manufacturer", 0 }, process_manufacturer },
	{ 2, { match_mamemessraine, match_gamemachine, "year", 0 }, process_year },
	{ 2, { match_mamemessraine, match_gamemachine, "cloneof", 0 }, process_cloneof },
	{ 2, { match_mamemessraine, match_gamemachine, "romof", 0 }, process_romof },
	{ 2, { match_mamemessraine, match_gamemachine, "rom", 0 }, process_rom },
	{ 3, { match_mamemessraine, match_gamemachine, "rom", "merge" }, process_rommerge },
	{ 3, { match_mamemessraine, match_gamemachine, "rom", "size" }, process_romsize },
	{ 3, { match_mamemessraine, match_gamemachine, "driver", "status" }, process_driverstatus },
	{ 3, { match_mamemessraine, match_gamemachine, "driver", "color" }, process_drivercolor },
	{ 3, { match_mamemessraine, match_gamemachine, "driver", "sound" }, process_driversound },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "screen" }, process_videoscreen },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "orientation" }, process_videoorientation },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "width" }, process_videowidth },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "height" }, process_videoheight },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "aspectx" }, process_videoaspectx },
	{ 3, { match_mamemessraine, match_gamemachine, "video", "aspecty" }, process_videoaspecty },
	{ 0, { 0, 0, 0, 0 }, 0 }
};

/**
 * Identify the specified element/attribute.
 */
static struct conversion_t* identify(unsigned depth, const struct level_t* level)
{
	unsigned i, j;

	if (depth < DEPTH_MAX) {
		for(i=0;CONV[i].name[0];++i) {
			if (CONV[i].depth != depth)
				continue;
			for(j=0;j<=depth;++j) {
				if (CONV[i].name[j] == match_mamemessraine) {
					if (strcmp(level[j].tag, "mame") != 0 && strcmp(level[j].tag, "mess") != 0 && strcmp(level[j].tag, "raine") != 0)
						break;
				} else if (CONV[i].name[j] == match_gamemachine) {
					if (strcmp(level[j].tag, "game") != 0 && strcmp(level[j].tag, "machine") != 0)
						break;
				} else {
					if (strcmp(level[j].tag, CONV[i].name[j]) != 0)
						break;
				}
			}
			if (j > depth)
				break;
		}
		if (CONV[i].name[0])
			return &CONV[i];
	}

	return 0;
}

/**
 * End Handler for the Expat parser.
 */
static void end_handler(void* data, const XML_Char* name)
{
	struct state_t* state = (struct state_t*)data;

	if (state->depth < DEPTH_MAX) {
		if (state->error == 0) {
			if (state->level[state->depth].process) {
				state->level[state->depth].process(state, token_data, state->level[state->depth].data, state->level[state->depth].len, 0);
				state->level[state->depth].process(state, token_close, 0, 0, 0);
			}
		}
		free(state->level[state->depth].data);
	}

	--state->depth;
}

/**
 * Data Handler for the Expat parser.
 */
void data_handler(void* data, const XML_Char* s, int len)
{
	struct state_t* state = (struct state_t*)data;

	if (state->depth < DEPTH_MAX) {
		if (state->error == 0) {
			/* accumulate the data */
			unsigned new_len = state->level[state->depth].len + len;
			state->level[state->depth].data = (char*)realloc(state->level[state->depth].data, new_len);
			if (!state->level[state->depth].data) {
				process_error(state, state->level[state->depth].tag, "low memory");
				return;
			}
			memcpy(state->level[state->depth].data + state->level[state->depth].len, s, len);
			state->level[state->depth].len += len;
		}
	}
}

/**
 * Start Handler for the Expat parser.
 */
static void start_handler(void* data, const XML_Char* name, const XML_Char** attributes)
{
	struct state_t* state = (struct state_t*)data;
	struct conversion_t* c;
	unsigned i;

	++state->depth;

	if (state->depth < DEPTH_MAX) {
		state->level[state->depth].tag = name;
		state->level[state->depth].data = 0;
		state->level[state->depth].len = 0;

		if (state->error == 0) {
			c = identify(state->depth, state->level);
			if (c) {
				state->level[state->depth].process = c->process;
				state->level[state->depth].process(state, token_open, 0, 0, attributes);
			} else {
				state->level[state->depth].process = 0;
			}

			for(i=0;attributes[i];i+=2) {
				const char* null_atts[1] = { 0 };
				start_handler(data, attributes[i], null_atts);
				data_handler(data, attributes[i+1], strlen(attributes[i+1]));
				end_handler(data, attributes[i]);
			}
		} else {
			state->level[state->depth].process = 0;
		}
	}
}

bool mame_info::load_xml(istream& is, game_set& gar)
{
	struct state_t state;
	char buf[4096];

	state.parser = XML_ParserCreate(NULL);
	if (!state.parser) {
		return false;
	}

	state.depth = -1;
	state.error = 0;
	state.e = this;
	state.g = 0;
	state.a = &gar;

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);

	while (1) {
		int done;
		int len;

		is.read(buf, sizeof(buf));
		if (is.bad()) {
			process_error(&state, "", "read error");
			break;
		}

		len = is.gcount();

		done = is.eof();

		if (XML_Parse(state.parser, buf, len, done) == XML_STATUS_ERROR) {
			process_error(&state, "", XML_ErrorString(XML_GetErrorCode(state.parser)));
			break;
		}

		if (done)
			break;
	}

	XML_ParserFree(state.parser);

	if (state.error) {
		return false;
	}

	return true;
}
