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
#include "glue.h"

#include "mame2.h"

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
int osd_display_loading_rom_message(const char* name, struct rom_load_data* romdata)
{
	struct advance_global_context* context = &CONTEXT.global;

	log_std(("osd: osd_display_loading_rom_message(name:%s)\n", name));

	if (!romdata && name) {
		/* it's a message */
		if (!context->config.quiet_flag || strstr(name,"ERROR")!=0) {
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
static void customize_language(struct advance_global_context* context, struct InputPort* current)
{
	struct InputPort* i;
	adv_bool at_least_one_found = 0;
	adv_bool at_least_one_set = 0;

	if (context->config.lang < 0)
		return;

	i = current;
	while ((i->type & ~IPF_MASK) != IPT_END) {
		if ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME
			&& (strstr(i->name, "Language")!=0 || strstr(i->name, "Territory")!=0 || strstr(i->name, "Country")!=0)) {
			struct InputPort* j;
			struct InputPort* best;
			unsigned best_value;
			struct InputPort* begin;
			struct InputPort* end;
			struct InputPort* value;

			at_least_one_found = 1;

			/* the value is stored in the NAME item */
			value = i;

			begin = ++i;

			/* read the value */
			while ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING) {
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
static void customize_difficulty(struct advance_global_context* context, struct InputPort* current)
{
	const char** names;
	const char** names_secondary;
	struct InputPort* value;
	struct InputPort* level;
	struct InputPort* begin;
	struct InputPort* end;
	struct InputPort* i;

	names = 0;
	names_secondary = 0;
	value = 0;
	begin = 0;
	end = 0;

	i = current;
	while ((i->type & ~IPF_MASK) != IPT_END) {
		if ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME
			&& strcmp(i->name,"Difficulty")==0) {

			/* the value is stored in the NAME item */
			value = i;

			begin = ++i;

			/* read the value */
			while ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING) {
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
				if (strcmp(names[j],i->name)==0) {
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
				if (strcmp(names_secondary[j],i->name)==0) {
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
static void customize_freeplay(struct advance_global_context* context, struct InputPort* current)
{
	struct InputPort* i;
	adv_bool atleastone;

	if (!context->config.freeplay_flag)
		return;

	atleastone = 0;

	i = current;
	while ((i->type & ~IPF_MASK) != IPT_END) {
		if ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME) {
			struct InputPort* value;
			struct InputPort* freeplay_exact;
			struct InputPort* freeplay_in;

			/* the value is stored in the NAME item */
			value = i;

			freeplay_exact = 0;
			freeplay_in = 0;

			++i;

			/* read the value */
			while ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING) {
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
static void customize_mutedemo(struct advance_global_context* context, struct InputPort* current)
{
	struct InputPort* i;
	adv_bool atleastone;

	if (!context->config.mutedemo_flag)
		return;

	atleastone = 0;

	i = current;
	while ((i->type & ~IPF_MASK) != IPT_END) {
		if ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_NAME) {
			struct InputPort* value;
			struct InputPort* exact;

			adv_bool match_name = strcmp(i->name, "Demo Sounds") == 0
				|| strcmp(i->name, "Demo Music") == 0;

			/* the value is stored in the NAME item */
			value = i;

			exact = 0;

			++i;

			/* read the value */
			while ((i->type & ~IPF_MASK) == IPT_DIPSWITCH_SETTING) {
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
static void customize_switch(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, struct InputPort* current, const char* tag, unsigned ipt_name, unsigned ipt_setting)
{
	struct InputPort* i;

	i = current;
	while (i->type != IPT_END) {
		if ((i->type & ~IPF_MASK) == ipt_name) {
			char name_buffer[256];
			char tag_buffer[256];
			const char* value;
			mame_name_adjust(name_buffer, sizeof(name_buffer), i->name);
			snprintf(tag_buffer, sizeof(tag_buffer), "%s[%s]", tag, name_buffer);

			if (conf_autoreg_string_get(cfg_context, tag_buffer, &value) == 0) {
				struct InputPort* j;
				j = i + 1;
				while ((j->type & ~IPF_MASK) == ipt_setting) {
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
static void customize_analog(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, struct InputPort* current)
{
	struct InputPort* i;

	i = current;
	while (i->type != IPT_END) {
		char name_buffer[256];

		if (advance_input_print_analogname(name_buffer, sizeof(name_buffer), i->type) == 0) {
			char tag_buffer[256];
			const char* value;
			snprintf(tag_buffer, sizeof(tag_buffer), "input_setting[%s]", name_buffer);

			if (conf_autoreg_string_get(cfg_context, tag_buffer, &value) == 0) {
				int delta;
				int sensitivity;
				int reverse;
				int center;
				char* d;

				d = strdup(value);
				if (advance_input_parse_analogvalue(&delta, &sensitivity, &reverse, &center, d) == 0) {
					IP_SET_DELTA(i, delta);
					IP_SET_SENSITIVITY(i, sensitivity);
					if (reverse)
						i->type |= IPF_REVERSE;
					else
						i->type &= ~IPF_REVERSE;
					if (center)
						i->type |= IPF_CENTER;
					else
						i->type &= ~IPF_CENTER;
					log_std(("emu:global: input set '%s %s'\n", name_buffer, value));
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
static void customize_input(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, struct InputPort* current)
{
	struct InputPort* i;

	i = current;
	while (i->type != IPT_END) {
		struct mame_port* p;

		p = mame_port_find(glue_port_convert(&i[-1].type, i[0].type, i[0].name));
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
						glue_seq_convertback(seq, INPUT_MAP_MAX, i->seq, SEQ_MAX);
					}
				} else {
					log_std(("ERROR:emu:global: error parsing '%s %s'\n", tag_buffer, value));
				}

				free(d);
			}
		}

		++i;
	}
}

/**
 * Customization of the GLOBAL ports.
 * These are system depended customization and are used to
 * to change the defaults values.
 */
void osd_customize_inputport_defaults(struct ipd* defaults)
{
	log_std(("emu:glue: osd_customize_inputport_defaults()\n"));

	/* no specific OS customization */
}

/**
 * User customization of the GLOBAL ports.
 * These customizations don't change the defaults values,
 * to allow the user to restore the original values
 * if he want.
 */
void osd_customize_inputport_pre_defaults(struct ipd* defaults)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	struct ipd* i = defaults;

	log_std(("emu:global: osd_customize_inputport_pre_defaults()\n"));

	while (i->type != IPT_END) {
		struct mame_port* p;

		p = mame_port_find(glue_port_convert(&i[-1].type, i[0].type, i[0].name));
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
						glue_seq_convertback(seq, INPUT_MAP_MAX, i->seq, SEQ_MAX);
					}
				} else {
					log_std(("ERROR:emu:global: error parsing '%s %s'\n", tag_buffer, value));
				}

				free(d);
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
void osd_customize_inputport_pre_game(struct InputPort* current)
{
	struct advance_global_context* context = &CONTEXT.global;
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;

	log_std(("emu:global: osd_customize_inputport_pre_game()\n"));

	customize_language(context, current);
	customize_difficulty(context, current);
	customize_freeplay(context, current);
	customize_mutedemo(context, current);

	customize_switch(context, cfg_context, game, current, "input_dipswitch", IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
#ifdef MESS
	customize_switch(context, cfg_context, game, current, "input_configswitch", IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
#endif

	customize_analog(context, cfg_context, game, current);
	customize_input(context, cfg_context, game, current);
}

/**
 * Called after a user customization of a GAME port.
 * Used for both dipswitch and analog ports.
 * Called with a null value to delete the customization.
 */
void osd2_customize_genericport_post_game(const char* tag, const char* value)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;

	log_std(("emu:global: osd2_customize_port_post_game(%s,%s)\n", tag, value));

	if (value) {
		conf_autoreg_string_set(cfg_context, mame_game_name(game), tag, value);
	} else {
		conf_autoreg_remove(cfg_context, mame_game_name(game), tag);
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

	advance_global_lcd(context, 0, "");
	advance_global_lcd(context, 1, "riga 1");

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

