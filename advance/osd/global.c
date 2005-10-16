/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2002, 2003, 2004 Andrea Mazzoleni
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
#include "input.h"

#include "glueint.h"

#include "advance.h"

#ifdef USE_LCD
#include "lcd.h"
#endif

/***************************************************************************/
/* Advance */

/**
 * Display a message.
 * This function can be called also from the video thread.
 */
void advance_global_message(struct advance_global_context* context, const char* text, ...)
{
	va_list arg;
	va_start(arg, text);
	advance_global_message_va(context, text, arg);
	va_end(arg);
}

/**
 * Display a message on the screen.
 * This function can be called also from the video thread.
 */
void advance_global_message_va(struct advance_global_context* context, const char* text, va_list arg)
{
	vsnprintf(context->state.message_buffer, sizeof(context->state.message_buffer), text, arg);
	log_std(("advance:global: set msg %s\n", context->state.message_buffer));
}

/**
 * Display a message in the LCD.
 */
void advance_global_lcd(struct advance_global_context* context, unsigned row, const char* text)
{
#ifdef USE_LCD
	if (context->state.lcd) {
		adv_lcd_display(context->state.lcd, row, text, context->config.lcd_speed);
	}
#endif
}

/**
 * Execute a shell script.
 */
int advance_global_script(struct advance_global_context* context, const char* command)
{
	return target_script(command);
}

/***************************************************************************/
/* Language */

#define LANG_TAG_MAX 16

struct language {
	struct language* fallback;
	const char* name;
	const char* tag[LANG_TAG_MAX];
};

static struct language LANG_USA =
{ 0, 0, { "USA", "US", "America", "American", "World", "England", "British" } };

static struct language LANG_EUROPE =
{ 0, 0, { "Europe", "Euro", "World", "England", "British", "USA", "US", "America", "American" } };

static struct language LANG_ASIA =
{ 0, 0, { "Asia", "Japan", "Japanese" } };

static struct language LANG[] = {
{ &LANG_USA, "usa", { "USA", "US", "America", "American" } },
{ &LANG_USA, "canada", { "Canada" } },
{ &LANG_EUROPE, "england", { "England", "British" } },
{ &LANG_EUROPE, "italy", { "Italy", "Italian" } },
{ &LANG_EUROPE, "germany", { "Germany", "German" } },
{ &LANG_EUROPE, "spain", { "Spain", "Hispanic", "Spanish" } },
{ &LANG_EUROPE, "austria", { "Austria" } },
{ &LANG_EUROPE, "norway", { "Norway", "Norwegian" } },
{ &LANG_EUROPE, "france", { "France", "French" } },
{ &LANG_EUROPE, "denmark", { "Denmark" } },
{ &LANG_ASIA, "japan", { "Japan", "Japanese" } },
{ &LANG_ASIA, "korea", { "Korea" } },
{ &LANG_ASIA, "china", { "China" } },
{ &LANG_ASIA, "hongkong", { "Hong Kong", "Hong-Kong" } },
{ &LANG_ASIA, "taiwan", { "Taiwan" } },
{ 0, 0 }
};

static adv_bool lang_match(const struct language* id, const char* text)
{
	unsigned i;

	for(i=0;id->tag[i];++i) {
		const char* j = strstr(text, id->tag[i]);
		if (j != 0) {
			if (j == text || !isalpha(j[-1])) {
				const char* e = j + strlen(id->tag[i]);
				if (!isalpha(*e))
					return 1;
			}
		}
	}

	return 0;
}

/**
 * Identify a language definition in a text.
 * \param id Language id to search.
 * \param text Text to analyze.
 * \return Match value. 0 for no match or a comparing score.
 */
unsigned lang_identify_text(int lang, const char* text)
{
	const struct language* id;
	unsigned score = 100;

	if (lang < 0)
		return 0;

	id = &LANG[lang];

	while (id != 0 && score > 0) {
		if (lang_match(id, text))
			return score;

		--score;
		id = id->fallback;
	}

	return 0;
}

/***************************************************************************/
/* OSD */

/**
 * Function called while loading ROMs.
 * It's called with romdata == 0 to print the message in name.
 * \return
 *  - !=0 to abort loading
 *  - ==0 on success
 */
int osd_display_loading_rom_message(const char* name, rom_load_data* romdata)
{
	struct advance_global_context* context = &CONTEXT.global;

	log_std(("osd: osd_display_loading_rom_message(name:%s)\n", name));

	if (!romdata && name) {
		/* it's a message */
		if (!context->config.quiet_flag || strstr(name, "ERROR")!=0) {
			target_err("%s", name);
		}
	}

	return 0;
}

/**
 * Display the stored message.
 */
void osd2_message(void)
{
	struct advance_global_context* context = &CONTEXT.global;

	/* display the stored message */
	if (context->state.message_buffer[0]) {
		log_std(("advance:global: display msg %s\n", context->state.message_buffer));
		advance_ui_message(&CONTEXT.ui, "%s", context->state.message_buffer);
		context->state.message_buffer[0] = 0;
	}
}

/**
 * User customization of the language dipswitch.
 */
static void config_customize_language(struct advance_global_context* context, input_port_entry* current)
{
	input_port_entry* i;
	adv_bool at_least_one_found = 0;
	adv_bool at_least_one_set = 0;

	if (context->config.lang < 0)
		return;

	i = current;
	while (i->type != IPT_END) {
		if (i->type == IPT_DIPSWITCH_NAME
			&& (strstr(i->name, "Language")!=0 || strstr(i->name, "Territory")!=0 || strstr(i->name, "Country")!=0)) {
			input_port_entry* j;
			input_port_entry* best;
			unsigned best_value;
			input_port_entry* begin;
			input_port_entry* end;
			input_port_entry* value;

			at_least_one_found = 1;

			/* the value is stored in the NAME item */
			value = i;

			begin = ++i;

			/* read the value */
			while (i->type == IPT_DIPSWITCH_SETTING) {
				++i;
			}

			end = i;

			/* search the best */
			best = begin;
			best_value = lang_identify_text(context->config.lang, best->name);
			for(j=begin;j!=end;++j) {
				unsigned value = lang_identify_text(context->config.lang, j->name);
				if (value > best_value) {
					best_value = value;
					best = j;
				}
			}

			/* set the value */
			if (best_value) {
				at_least_one_set = 1;

				value->default_value = best->default_value & value->mask;

				log_std(("emu:global: language dip switch '%s' set to '%s'\n", value->name, best->name));
			}
		} else {
			++i;
		}
	}

	if (!at_least_one_found) {
		log_std(("emu:global: language dip switch not found\n"));
		return;
	}

	if (!at_least_one_set) {
		log_std(("emu:global: language dip switch unknown\n"));
		return;
	}
}

const char* NAME_EASIEST[] = { "Easiest", "Very Easy", 0 };
const char* NAME_EASY[] = { "Easy", "Easier", "Easy?", 0 };
const char* NAME_MEDIUM[] = { "Medium", "Normal", "Normal?", 0 };
const char* NAME_HARD[] = { "Hard", "Harder", "Difficult", "Hard?", 0 };
const char* NAME_HARDEST[] = { "Hardest", "Very Hard", "Very Difficult", 0 };

/**
 * User customization of the difficulty dipswitch.
 */
static void config_customize_difficulty(struct advance_global_context* context, input_port_entry* current)
{
	const char** names;
	const char** names_secondary;
	input_port_entry* value;
	input_port_entry* level;
	input_port_entry* begin;
	input_port_entry* end;
	input_port_entry* i;

	names = 0;
	names_secondary = 0;
	value = 0;
	begin = 0;
	end = 0;

	i = current;
	while (i->type != IPT_END) {
		if (i->type == IPT_DIPSWITCH_NAME
			&& strcmp(i->name, "Difficulty")==0) {

			/* the value is stored in the NAME item */
			value = i;

			begin = ++i;

			/* read the value */
			while (i->type == IPT_DIPSWITCH_SETTING) {
				++i;
			}

			end = i;

			/* set only one */
			break;
		} else {
			++i;
		}
	}

	if (!value) {
		log_std(("emu:global: difficulty dip switch not found\n"));
		return;
	}

	if (begin == end) {
		log_std(("emu:global: difficulty dip switch empty\n"));
		return;
	}

	/* start the search */
	level = 0;

	/* get the list of names */
	switch (context->config.difficulty) {
	case DIFFICULTY_NONE :
		/* nothing to do */
		return;
	case DIFFICULTY_EASIEST :
		names = NAME_EASIEST;
		names_secondary = NAME_EASY;
		break;
	case DIFFICULTY_EASY :
		names = NAME_EASY;
		names_secondary = 0;
		break;
	case DIFFICULTY_MEDIUM :
		names = NAME_MEDIUM;
		names_secondary = NAME_EASY;
		break;
	case DIFFICULTY_HARD :
		names = NAME_HARD;
		names_secondary = 0;
		break;
	case DIFFICULTY_HARDEST :
		names = NAME_HARDEST;
		names_secondary = NAME_HARD;
		break;
	}

	/* search an exact match */
	if (!level) {
		unsigned j;
		for(j=0;names[j] && !level;++j) {
			for(i=begin;i!=end && !level;++i) {
				if (strcmp(names[j], i->name)==0) {
					level = i;
					break;
				}
			}
		}
	}

	/* search a secondary match */
	if (!level && names_secondary) {
		unsigned j;
		for(j=0;names[j] && !level;++j) {
			for(i=begin;i!=end && !level;++i) {
				if (strcmp(names_secondary[j], i->name)==0) {
					level = i;
					break;
				}
			}
		}
	}

	/* interpolate */
	if (!level) {
		unsigned n = end - begin;
		switch (context->config.difficulty) {
		case DIFFICULTY_EASIEST :
			level = begin;
			break;
		case DIFFICULTY_EASY :
			level = begin + n / 4;
				break;
		case DIFFICULTY_MEDIUM :
			level = begin + n * 2 / 4;
			break;
		case DIFFICULTY_HARD :
			level = begin + n * 3 / 4;
			break;
		case DIFFICULTY_HARDEST :
			level = end - 1;
			break;
		}
	}

	if (!level) {
		log_std(("emu:global: difficulty dip switch unknown\n"));
		return;
	}

	/* set the difficulty */
	value->default_value = level->default_value & value->mask;

	log_std(("emu:global: difficulty dip switch '%s' set to '%s'\n", value->name, level->name));
}

/**
 * User customization of the freeplay dipswitch.
 */
static void config_customize_freeplay(struct advance_global_context* context, input_port_entry* current)
{
	input_port_entry* i;
	adv_bool atleastone;

	if (!context->config.freeplay_flag)
		return;

	atleastone = 0;

	i = current;
	while (i->type != IPT_END) {
		if (i->type == IPT_DIPSWITCH_NAME) {
			input_port_entry* value;
			input_port_entry* freeplay_exact;
			input_port_entry* freeplay_in;

			/* the value is stored in the NAME item */
			value = i;

			freeplay_exact = 0;
			freeplay_in = 0;

			++i;

			/* read the value */
			while (i->type == IPT_DIPSWITCH_SETTING) {
				if (!freeplay_exact && sglob(i->name, "Free?Play")) {
					freeplay_exact = i;
				} else if (!freeplay_in && sglob(i->name, "*Free?Play*")) {
					freeplay_in = i;
				}
				++i;
			}

			if (freeplay_exact == 0 && freeplay_in != 0) {
				freeplay_exact = freeplay_in;
			}

			if (freeplay_exact) {
				/* set the difficulty */
				value->default_value = freeplay_exact->default_value & value->mask;
				atleastone = 1;
			}
		} else {
			++i;
		}
	}

	if (atleastone) {
		log_std(("emu:global: freeplay dip switch set\n"));
	}
}

/**
 * User customization of the mutedemo dipswitch.
 */
static void config_customize_mutedemo(struct advance_global_context* context, input_port_entry* current)
{
	input_port_entry* i;
	adv_bool atleastone;

	if (!context->config.mutedemo_flag)
		return;

	atleastone = 0;

	i = current;
	while (i->type != IPT_END) {
		if (i->type == IPT_DIPSWITCH_NAME) {
			input_port_entry* value;
			input_port_entry* exact;

			adv_bool match_name = strcmp(i->name, "Demo Sounds") == 0
				|| strcmp(i->name, "Demo Music") == 0;

			/* the value is stored in the NAME item */
			value = i;

			exact = 0;

			++i;

			/* read the value */
			while (i->type == IPT_DIPSWITCH_SETTING) {
				if (match_name) {
					if (!exact && strcmp(i->name, "Off")==0) {
						exact = i;
					}
				} else {
					if (!exact && strcmp(i->name, "Demo Sounds Off")==0) {
						exact = i;
					}
				}
				++i;
			}

			if (exact) {
				/* set the difficulty */
				value->default_value = exact->default_value & value->mask;
				atleastone = 1;
			}
		} else {
			++i;
		}
	}

	if (atleastone) {
		log_std(("emu:global: mutedemo dip switch set\n"));
	}
}

/**
 * User customization of the generic dipswitches.
 */
static void config_customize_switch(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, input_port_entry* current, const char* tag, unsigned ipt_name, unsigned ipt_setting)
{
	input_port_entry* i;

	i = current;
	while (i->type != IPT_END) {
		if (i->type == ipt_name) {
			char name_buffer[256];
			char tag_buffer[256];
			const char* value;
			mame_name_adjust(name_buffer, sizeof(name_buffer), i->name);
			snprintf(tag_buffer, sizeof(tag_buffer), "%s[%s]", tag, name_buffer);

			if (conf_autoreg_string_get(cfg_context, tag_buffer, &value) == 0) {
				input_port_entry* j;
				j = i + 1;
				while (j->type == ipt_setting) {
					char value_buffer[256];
					mame_name_adjust(value_buffer, sizeof(value_buffer), j->name);
					if (strcmp(value_buffer, value) == 0) {
						/* set the port */
						i->default_value = j->default_value & i->mask;
						log_std(("emu:global: dip switch set '%s' to '%s'\n", tag_buffer, value_buffer));
						break;
					}
					++j;
				}
			}
		}
		++i;
	}
}

/**
 * User customization of the analog ports.
 */
static void config_customize_analog(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, input_port_entry* current)
{
	input_port_entry* i;

	i = current;
	while (i->type != IPT_END) {
		struct mame_analog* a;
		unsigned port;

		port = glue_port_convert(i->type, i->player, SEQ_TYPE_STANDARD, 0);

		a = mame_analog_find(port);
		if (a) {
			char tag_buffer[256];
			const char* value;
			snprintf(tag_buffer, sizeof(tag_buffer), "input_setting[%s]", a->name);

			if (conf_autoreg_string_get(cfg_context, tag_buffer, &value) == 0) {
				int delta;
				int sensitivity;
				int reverse;
				int centerdelta;
				char* d;

				d = strdup(value);
				delta = i->analog.delta;
				sensitivity = i->analog.sensitivity;
				reverse = i->analog.reverse;
				centerdelta = i->analog.centerdelta;
				if (advance_input_parse_analogvalue(&delta, &sensitivity, &reverse, &centerdelta, d) == 0) {
					i->analog.delta = delta;
					i->analog.sensitivity = sensitivity;
					i->analog.reverse = reverse;
					i->analog.centerdelta = centerdelta;
					log_std(("emu:global: input set '%s %s'\n", a->name, value));
				} else {
					log_std(("ERROR:emu:global: unknown '%s %s'\n", tag_buffer, value));
				}
				free(d);
			}
		}

		++i;
	}
}

/**
 * User customization of the input code sequences.
 */
static void config_customize_input(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, input_port_entry* current)
{
	input_port_entry* i;

	i = current;
	while (i->type != IPT_END) {
		unsigned n;
		for(n=glue_port_seq_begin(i->type);n<glue_port_seq_end(i->type);++n) {
			struct mame_port* p;
			p = mame_port_find(glue_port_convert(i->type, i->player, glue_port_seqtype(i->type, n), i->name));
			if (p != 0) {
				char tag_buffer[64];
				const char* value;

				snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

				if (conf_string_get(cfg_context, tag_buffer, &value) == 0) {
					char* d = strdup(value);
					unsigned seq[INPUT_MAP_MAX];

					if (advance_input_parse_digital(seq, INPUT_MAP_MAX, d) == 0) {
						if (seq[0] != DIGITAL_SPECIAL_AUTO) {
							log_std(("emu:global: input game seq '%s %s'\n", p->name, value));
							glue_seq_convertback(seq, INPUT_MAP_MAX, glue_port_seq_get(i, glue_port_seqtype(i->type, n))->code, SEQ_MAX);
						}
					} else {
						log_std(("ERROR:emu:global: error parsing '%s %s'\n", tag_buffer, value));
					}

					free(d);
				}
			}
		}

		++i;
	}
}

static input_port_default_entry* config_portdef_find(input_port_default_entry* list, unsigned type)
{
	while (list->type != IPT_END && list->type != type)
		++list;

	assert(list->type != IPT_END);

	if (list->type == IPT_END)
		return 0;

	return list;
}

/**
 * Customization of the GLOBAL ports.
 * These are system depended customization and are used to
 * to change the defaults values.
 */
void osd_customize_inputport_list(input_port_default_entry* defaults)
{
	input_port_default_entry* i;

	log_std(("emu:global: osd_customize_inputport_list()\n"));

	i = config_portdef_find(defaults, IPT_UI_HELP);
	seq_set_1(&i->defaultseq, KEYCODE_F1_REAL);
	i->name = "Help";

	i = config_portdef_find(defaults, IPT_UI_RECORD_START);
	seq_set_2(&i->defaultseq, KEYCODE_ENTER, KEYCODE_LCONTROL);
	i->name = "Record Start";

	i = config_portdef_find(defaults, IPT_UI_RECORD_STOP);
	seq_set_3(&i->defaultseq, KEYCODE_ENTER, CODE_NOT, KEYCODE_LCONTROL);
	i->name = "Record Stop";

	i = config_portdef_find(defaults, IPT_UI_TURBO);
	seq_set_1(&i->defaultseq, KEYCODE_ASTERISK);
	i->name = "Turbo";

	i = config_portdef_find(defaults, IPT_UI_COCKTAIL);
	seq_set_1(&i->defaultseq, KEYCODE_SLASH_PAD);
	i->name = "Cocktail";

	i = config_portdef_find(defaults, IPT_UI_STARTUP_END);
	seq_set_1(&i->defaultseq, KEYCODE_MINUS_PAD);
	i->name = "Startup End";

	i = config_portdef_find(defaults, IPT_UI_MODE_NEXT);
	seq_set_1(&i->defaultseq, KEYCODE_STOP);
	i->name = "Mode Next";

	i = config_portdef_find(defaults, IPT_UI_MODE_PRED);
	seq_set_1(&i->defaultseq, KEYCODE_COMMA);
	i->name = "Mode Pred";
}

/**
 * User customization of the GLOBAL ports.
 * These customizations don't change the defaults values,
 * to allow the user to restore the original values
 * if he want.
 */
void osd_config_load_default(input_port_default_entry* backup, input_port_default_entry* list)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	input_port_default_entry* i;

	log_std(("emu:global: osd_config_load_default()\n"));

	i = list;
	while (i->type != IPT_END) {
		unsigned n;
		for(n=glue_port_seq_begin(i->type);n<glue_port_seq_end(i->type);++n) {
			struct mame_port* p;
			p = mame_port_find(glue_port_convert(i->type, i->player, glue_port_seqtype(i->type, n), i->name));
			if (p != 0) {
				char tag_buffer[64];
				const char* value;

				snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

				if (conf_string_section_get(cfg_context, "", tag_buffer, &value) == 0) {
					char* d = strdup(value);
					unsigned seq[INPUT_MAP_MAX];

					if (advance_input_parse_digital(seq, INPUT_MAP_MAX, d) == 0) {
						if (seq[0] != DIGITAL_SPECIAL_AUTO) {
							log_std(("emu:global: input default seq '%s %s'\n", p->name, value));
							glue_seq_convertback(seq, INPUT_MAP_MAX, glue_portdef_seq_get(i, glue_port_seqtype(i->type, n))->code, SEQ_MAX);
						}
					} else {
						log_std(("ERROR:emu:global: error parsing '%s %s'\n", tag_buffer, value));
					}

					free(d);
				}
			}
		}

		++i;
	}
}

/**
 * User customization of the GAME ports.
 * These customizations don't change the defaults values,
 * to allow the user to restore the original values
 * if he want.
 */
void osd_config_load(input_port_entry* backup, input_port_entry* list)
{
	struct advance_global_context* context = &CONTEXT.global;
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;

	log_std(("emu:global: osd_config_load()\n"));

	config_customize_language(context, list);
	config_customize_difficulty(context, list);
	config_customize_freeplay(context, list);
	config_customize_mutedemo(context, list);

	config_customize_switch(context, cfg_context, game, list, "input_dipswitch", IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
#ifdef MESS
	config_customize_switch(context, cfg_context, game, list, "input_configswitch", IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
#endif

	config_customize_analog(context, cfg_context, game, list);
	config_customize_input(context, cfg_context, game, list);
}

/**
 * Called after a user customization of a GAME port.
 * Used for both dipswitch and analog ports.
 * Called with a null value to delete the customization.
 */
static void config_save_generic(const char* tag, const char* value)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;

	log_debug(("emu:global: config_save_generic(%s,%s)\n", tag, value));

	if (value) {
		conf_autoreg_string_set(cfg_context, mame_game_name(game), tag, value);
	} else {
		conf_autoreg_remove(cfg_context, mame_game_name(game), tag);
	}
}

/**
 * Called after a user customization of a GLOBAL input code combination.
 * Called with an null/empty sequence to delete the customization.
 */
static void config_save_seq_default(unsigned port, unsigned* seq, unsigned seq_max)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const struct mame_port* p;

	if (seq)
		log_debug(("global: config_save_seq_default(%d, set)\n", port));
	else
		log_debug(("global: config_save_seq_default(%d, clear)\n", port));

	p = mame_port_find(port);
	if (p) {
		char tag_buffer[64];
		char value_buffer[512];

		snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

		if (!seq || seq[0] == DIGITAL_SPECIAL_AUTO) {
			log_std(("global: customize port default %s\n", tag_buffer));

			conf_remove(cfg_context, "", tag_buffer);
		} else {
			advance_input_print_digital(value_buffer, sizeof(value_buffer), seq, seq_max);

			log_std(("global: customize port %s %s\n", tag_buffer, value_buffer));

			conf_string_set(cfg_context, "", tag_buffer, value_buffer);
		}
	} else {
		log_debug(("WARNING:global: customization for unknown port %d not saved\n", port));
	}
}

/**
 * Called after a user customization of a GAME input code combination.
 * Called with an null/empty sequence to delete the customization.
 */
static void config_save_seq(unsigned port, unsigned* seq, unsigned seq_max)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;
	const struct mame_port* p;

	if (seq)
		log_debug(("global: config_save_seq(%d, set)\n", port));
	else
		log_debug(("global: config_save_seq(%d, clear)\n", port));

	p = mame_port_find(port);
	if (p) {
		char tag_buffer[64];
		char value_buffer[512];

		log_std(("global: setup port %s\n", p->name));

		snprintf(tag_buffer, sizeof(tag_buffer), "input_map[%s]", p->name);

		if (!seq || seq[0] == DIGITAL_SPECIAL_AUTO) {
			log_std(("global: customize port default %s/%s\n", mame_section_name(game, cfg_context), tag_buffer));

			conf_remove(cfg_context, mame_section_name(game, cfg_context), tag_buffer);
		} else {
			advance_input_print_digital(value_buffer, sizeof(value_buffer), seq, seq_max);

			log_std(("global: customize port %s/%s %s\n", mame_section_name(game, cfg_context), tag_buffer, value_buffer));

			conf_set_if_different(cfg_context, mame_section_name(game, cfg_context), tag_buffer, value_buffer);
		}
	} else {
		log_debug(("WARNING:global: customization for unknown port %d not saved\n", port));
	}
}

/**
 * Called after a user customization of a GAME input code combination.
 * \param def Default value of the input code combination.
 * \param current User chosen value of the input code combination.
 */
static void config_save_seqport(input_port_entry* def, input_port_entry* current, int seqtype)
{
	unsigned seq[MAME_INPUT_MAP_MAX];
	unsigned def_seq[MAME_INPUT_MAP_MAX];
	unsigned port;

	log_debug(("global: config_save_seqport()\n"));

	glue_seq_convert(glue_port_seq_get(current, seqtype)->code, SEQ_MAX, seq, MAME_INPUT_MAP_MAX);

	/* the evaluated sequence must be used because the default */
	/* configuration value is copied also in the specific game */
	/* configuration in the osd_config_load() function. */
	glue_seq_convert(glue_port_seqeval_get(def, seqtype)->code, SEQ_MAX, def_seq, MAME_INPUT_MAP_MAX);

	port = glue_port_convert(current->type, current->player, seqtype, current->name);

	if (seq[0] != DIGITAL_SPECIAL_AUTO
		&& memcmp(def_seq, seq, sizeof(def_seq)) != 0) {
		config_save_seq(port, seq, MAME_INPUT_MAP_MAX);
	} else {
		config_save_seq(port, 0, 0);
	}
}

/**
 * Called after a user customization of a GLOBAL input code combination.
 * \param def Default value of the input code combination.
 * \param current User chosen value of the input code combination.
 */
static void config_save_seqport_default(input_port_default_entry* def, input_port_default_entry* current, int seqtype)
{
	unsigned def_seq[MAME_INPUT_MAP_MAX];
	unsigned seq[MAME_INPUT_MAP_MAX];
	unsigned port;

	log_debug(("global: config_save_seqport_default()\n"));

	glue_seq_convert(glue_portdef_seq_get(current, seqtype)->code, SEQ_MAX, seq, MAME_INPUT_MAP_MAX);
	glue_seq_convert(glue_portdef_seq_get(def, seqtype)->code, SEQ_MAX, def_seq, MAME_INPUT_MAP_MAX);

	port = glue_port_convert(current->type, current->player, seqtype, current->name);

	assert(seq[0] != DIGITAL_SPECIAL_AUTO);
	assert(def_seq[0] != DIGITAL_SPECIAL_AUTO);

	if (memcmp(def_seq, seq, sizeof(def_seq)) != 0) {
		config_save_seq_default(port, seq, MAME_INPUT_MAP_MAX);
	} else {
		config_save_seq_default(port, 0, 0);
	}
}

/**
 * Called after a user customization of a dipswitch.
 * \param def Default value of the dipswitch.
 * \param current User chosen value of the dipswitch.
 */
static void config_save_switchport(input_port_entry* def, input_port_entry* current)
{
	char name_buffer[256];
	char tag_buffer[256];
	char value_buffer[256];
	input_port_entry* v;
	const char* tag;
	unsigned type;

	log_debug(("global: config_save_switchport()\n"));

	switch (current->type) {
	case IPT_DIPSWITCH_NAME :
		tag = "input_dipswitch";
		type = IPT_DIPSWITCH_SETTING;
		break;
#ifdef MESS
	case IPT_CONFIG_NAME :
		tag = "input_configswitch";
		type = IPT_CONFIG_SETTING;
		break;
#endif
	default:
		log_std(("WARNING:global: unknown switchport %d\n", current->type));
		return;
	}

	if (strcmp(current->name, DEF_STR(Unused))==0 || strcmp(current->name, DEF_STR(Unknown))==0) {
		log_std(("WARNING:global: ignoring named Unknown/Unused switchport %d\n", current->type));
		return;
	}

	mame_name_adjust(name_buffer, sizeof(name_buffer), current->name);

	v = current + 1;
	while (v->type == type) {
		if ((v->default_value & current->mask) == (current->default_value & current->mask)) {
			break;
		}
		++v;
	}

	if (v->type != type) {
		log_std(("ERROR:global: Unknown switchport %s value %d\n", name_buffer, current->default_value));
		return;
	}

	snprintf(tag_buffer, sizeof(tag_buffer), "%s[%s]", tag, name_buffer);

	if ((def->default_value & current->mask) != (current->default_value & current->mask)) {
		mame_name_adjust(value_buffer, sizeof(value_buffer), v->name);
		config_save_generic(tag_buffer, value_buffer);
	} else {
		config_save_generic(tag_buffer, 0);
	}
}

/**
 * Called after a user customization of GAME analog port.
 * \param def Default value of the analog port.
 * \param current User chosen value of the analog port.
 */
static void config_save_analogport(input_port_entry* def, input_port_entry* current)
{
	char value_buffer[256];
	char default_buffer[256];
	char tag_buffer[256];
	unsigned port;
	struct mame_analog* a;

	log_debug(("global: config_save_analogport()\n"));

	port = glue_port_convert(current->type, current->player, SEQ_TYPE_STANDARD, 0);

	a = mame_analog_find(port);
	if (!a) {
		log_debug(("WARNING:global: unknown analog port %d\n", port));
		return;
	}

	advance_input_print_analogvalue(value_buffer, sizeof(value_buffer), current->analog.delta, current->analog.sensitivity, current->analog.reverse != 0, current->analog.centerdelta);
	advance_input_print_analogvalue(default_buffer, sizeof(default_buffer), def->analog.delta, def->analog.sensitivity, def->analog.reverse != 0, def->analog.centerdelta);

	snprintf(tag_buffer, sizeof(tag_buffer), "input_setting[%s]", a->name);

	if (strcmp(value_buffer, default_buffer) != 0) {
		config_save_generic(tag_buffer, value_buffer);
	} else {
		config_save_generic(tag_buffer, 0);
	}
}

void osd_config_save_default(input_port_default_entry* backup, input_port_default_entry* list)
{
	input_port_default_entry* i;
	input_port_default_entry* j;

	i = list;
	j = backup;
	while (i->type != IPT_END) {
		unsigned type = i->type;
		unsigned k;

		if (i->name != 0) {
			switch (type) {
			default:
				for(k=glue_port_seq_begin(type);k!=glue_port_seq_end(type);++k) {
					config_save_seqport_default(j, i, glue_port_seqtype(type, k));
				}
				break;
			}
		}

		++i;
		++j;
	}
}

void osd_config_save(input_port_entry* backup, input_port_entry* list)
{
	input_port_entry* i;
	input_port_entry* j;

	i = list;
	j = backup;
	while (i->type != IPT_END) {
		unsigned type = i->type;
		adv_bool analog = port_type_is_analog(type);
		unsigned k;

		if (input_port_active(i)) {
			switch (type) {
			case IPT_DIPSWITCH_NAME :
			case IPT_CONFIG_NAME :
				config_save_switchport(j, i);
				break;
			default:
				if (analog) {
					config_save_analogport(j, i);
				}
				for(k=glue_port_seq_begin(type);k!=glue_port_seq_end(type);++k) {
					config_save_seqport(j, i, glue_port_seqtype(type, k));
				}
				break;
			}
		}

		++i;
		++j;
	}
}

/***************************************************************************/
/* Initialization */

static adv_conf_enum_int OPTION_DIFFICULTY[] = {
{ "none", DIFFICULTY_NONE },
{ "easiest", DIFFICULTY_EASIEST },
{ "easy", DIFFICULTY_EASY },
{ "normal", DIFFICULTY_MEDIUM },
{ "hard", DIFFICULTY_HARD },
{ "hardest", DIFFICULTY_HARDEST }
};

#define OPTION_LANG_MAX 64
static adv_conf_enum_int OPTION_LANG[OPTION_LANG_MAX];

adv_error advance_global_init(struct advance_global_context* context, adv_conf* cfg_context)
{
	unsigned i;

	conf_bool_register_default(cfg_context, "misc_quiet", 0);
	conf_int_register_enum_default(cfg_context, "misc_difficulty", conf_enum(OPTION_DIFFICULTY), DIFFICULTY_NONE);

	OPTION_LANG[0].value = "none";
	OPTION_LANG[0].map = -1;
	for(i=0;LANG[i].name && i+1<OPTION_LANG_MAX;++i) {
		OPTION_LANG[i+1].value = LANG[i].name;
		OPTION_LANG[i+1].map = i;
	}

	conf_int_register_enum_default(cfg_context, "misc_lang", OPTION_LANG, i+1, -1);
	conf_bool_register_default(cfg_context, "misc_freeplay", 0);
	conf_bool_register_default(cfg_context, "misc_mutedemo", 0);
	conf_float_register_limit_default(cfg_context, "display_pausebrightness", 0.0, 1.0, 1.0);

#ifdef USE_LCD
	conf_string_register_default(cfg_context, "lcd_server", "none");
	conf_int_register_limit_default(cfg_context, "lcd_timeout", 100, 60000, 500);
	conf_int_register_limit_default(cfg_context, "lcd_speed", -16, 16, 4);
#endif

	return 0;
}

adv_error advance_global_config_load(struct advance_global_context* context, adv_conf* cfg_context)
{
	const char* s;

	context->config.quiet_flag = conf_bool_get_default(cfg_context, "misc_quiet");
	context->config.difficulty = conf_int_get_default(cfg_context, "misc_difficulty");

	context->config.lang = conf_int_get_default(cfg_context, "misc_lang");

	context->config.freeplay_flag = conf_bool_get_default(cfg_context, "misc_freeplay");
	context->config.mutedemo_flag = conf_bool_get_default(cfg_context, "misc_mutedemo");
	context->config.pause_brightness = conf_float_get_default(cfg_context, "display_pausebrightness");

#ifdef USE_LCD
	s = conf_string_get_default(cfg_context, "lcd_server");
	sncpy(context->config.lcd_server_buffer, sizeof(context->config.lcd_server_buffer), s);
	context->config.lcd_timeout = conf_int_get_default(cfg_context, "lcd_timeout");
	context->config.lcd_speed = conf_int_get_default(cfg_context, "lcd_speed");
#endif

	return 0;
}

adv_error advance_global_inner_init(struct advance_global_context* context)
{
#ifdef USE_LCD
	if (strcmp(context->config.lcd_server_buffer, "none") != 0) {
		log_std(("global: initializing lcd at '%s' with timeout %d\n", context->config.lcd_server_buffer, context->config.lcd_timeout));

		context->state.lcd = adv_lcd_init(context->config.lcd_server_buffer, context->config.lcd_timeout);
		if (!context->state.lcd) {
			log_std(("ERROR:global: lcd not initialized\n"));
			/* ignore error and continue without using display */
			context->state.lcd = 0;
		}
	} else {
		context->state.lcd = 0;
	}
#endif

	return 0;
}

void advance_global_inner_done(struct advance_global_context* context)
{
#ifdef USE_LCD
	if (context->state.lcd) {
		adv_lcd_done(context->state.lcd);
	}
#endif
}

void advance_global_done(struct advance_global_context* context)
{
}

