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

#include "emu.h"
#include "input.h"
#include "glue.h"

#include "log.h"
#include "snstring.h"
#include "target.h"

#include "mame2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Function called while loading ROMs.
 * It's called with romdata == 0 to print a message.
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
 * Display a message.
 * This function can be called also from the video thread.
 */
void advance_global_message_va(struct advance_global_context* context, const char* text, va_list arg)
{
	vsnprintf(context->state.message_buffer, sizeof(context->state.message_buffer), text, arg);
	log_std(("advance:global: set msg %s\n", context->state.message_buffer));
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

const char* NAME_EASIEST[] = { "Easiest", "Very Easy", 0 };
const char* NAME_EASY[] = { "Easy", "Easier", "Easy?", 0 };
const char* NAME_MEDIUM[] = { "Medium", "Normal", "Normal?", 0 };
const char* NAME_HARD[] = { "Hard", "Harder", "Difficult", "Hard?", 0 };
const char* NAME_HARDEST[] = { "Hardest", "Very Hard", "Very Difficult", 0 };

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

			break;
		}
		++i;
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

	log_std(("emu:global: difficulty dip switch set to '%s'\n", level->name));
}

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

static void customize_input(struct advance_global_context* context, adv_conf* cfg_context, const mame_game* game, struct InputPort* current)
{
	struct InputPort* i;

	i = current;
	while (i->type != IPT_END) {
		struct mame_port* p;

		p = mame_port_find(glue_port_convert(&i[-1].type, i[0].type));
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

void osd_customize_inputport_pre_game(struct InputPort* current)
{
	struct advance_global_context* context = &CONTEXT.global;
	adv_conf* cfg_context = CONTEXT.cfg;
	const mame_game* game = CONTEXT.game;

	log_std(("emu:global: osd_customize_inputport_pre_game()\n"));

	customize_switch(context, cfg_context, game, current, "input_dipswitch", IPT_DIPSWITCH_NAME, IPT_DIPSWITCH_SETTING);
#ifdef MESS
	customize_switch(context, cfg_context, game, current, "input_configswitch", IPT_CONFIG_NAME, IPT_CONFIG_SETTING);
#endif
	customize_difficulty(context, current);
	customize_analog(context, cfg_context, game, current);
	customize_input(context, cfg_context, game, current);
}

void osd_customize_inputport_pre_defaults(struct ipd* defaults)
{
	adv_conf* cfg_context = CONTEXT.cfg;
	struct ipd* i = defaults;

	log_std(("emu:global: osd_customize_inputport_pre_defaults()\n"));

	while (i->type != IPT_END) {
		struct mame_port* p;

		p = mame_port_find(glue_port_convert(&i[-1].type, i[0].type));
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

void osd2_customize_port_post_game(const char* tag, const char* value)
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

static adv_conf_enum_int OPTION_DIFFICULTY[] = {
{ "none", DIFFICULTY_NONE },
{ "easiest", DIFFICULTY_EASIEST },
{ "easy", DIFFICULTY_EASY },
{ "normal", DIFFICULTY_MEDIUM },
{ "hard", DIFFICULTY_HARD },
{ "hardest", DIFFICULTY_HARDEST }
};

adv_error advance_global_init(struct advance_global_context* context, adv_conf* cfg_context)
{
	conf_bool_register_default(cfg_context, "misc_quiet", 0);
	conf_int_register_enum_default(cfg_context, "misc_difficulty", conf_enum(OPTION_DIFFICULTY), DIFFICULTY_NONE);
	conf_float_register_limit_default(cfg_context, "display_pausebrightness", 0.0, 1.0, 1.0);

	return 0;
}

adv_error advance_global_config_load(struct advance_global_context* context, adv_conf* cfg_context)
{
	context->config.quiet_flag = conf_bool_get_default(cfg_context, "misc_quiet");
	context->config.difficulty = conf_int_get_default(cfg_context, "misc_difficulty");
	context->config.pause_brightness = conf_float_get_default(cfg_context, "display_pausebrightness");

	return 0;
}

void advance_global_done(struct advance_global_context* context)
{
}

