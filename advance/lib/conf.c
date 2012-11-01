/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004, 2005 Andrea Mazzoleni
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

#include "conf.h"
#include "incstr.h"
#include "snstring.h"
#include "log.h"
#include "target.h"

/** Buffer used for int/float values */
#define CONF_NUM_BUFFER_MAX 48

/***************************************************************************/
/* Local */

static adv_bool partial_match_whole(const char* s, const char* partial)
{
	const char* p = s;
	unsigned l = strlen(partial);

	/* skip composite option strings */
	if (strchr(s, '[') != 0)
		return 0;

	while (1) {
		if (memcmp(p, partial, l)==0 && (p[l]==0 || p[l]=='_'))
			return 1;

		p = strchr(p, '_');
		if (!p)
			return 0;

		++p; /* skip the '_' */
	}
}

static adv_bool partial_match(const char* s, const char* partial)
{
	const char* p = s;
	unsigned l = strlen(partial);

	/* skip composite option strings */
	if (strchr(s, '[') != 0)
		return 0;

	while (1) {
		if (memcmp(p, partial, l)==0)
			return 1;

		p = strchr(p, '_');
		if (!p)
			return 0;

		++p; /* skip the '_' */
	}
}

static char* glob_subst(const char* format, char* own_s)
{
	if (strcmp(format, "%s")==0) {
		return own_s;
	} else {
		char* token = strstr(format, "%s");
		/* assume no more than one %s token */
		if (token) {
			unsigned l = strlen(format) + strlen(own_s) - 1;
			char* n = malloc(l);
			snprintf(n, l, format, own_s);
			free(own_s);
			return n;
		} else {
			free(own_s);
			return strdup(format);
		}
	}
}

static void option_insert(adv_conf* context, struct adv_conf_option_struct* option)
{
	if (context->option_list) {
		option->pred = context->option_list->pred;
		option->next = context->option_list;
		option->pred->next = option;
		option->next->pred = option;
	} else {
		context->option_list = option;
		option->next = option;
		option->pred = option;
	}
}

static struct adv_conf_option_struct* option_alloc(void)
{
	struct adv_conf_option_struct* option;

	option = (struct adv_conf_option_struct*)malloc(sizeof(struct adv_conf_option_struct));

	option->tag = 0;
	option->type = -1; /* invalid value */

	return option;
}

static void option_free(struct adv_conf_option_struct* option)
{
	free(option->tag);
	switch (option->type) {
		case conf_type_string :
			if (option->data.base_string.has_def)
				free(option->data.base_string.def);
			break;
		case conf_type_bool :
		case conf_type_int :
		case conf_type_float :
			break;
	}
	free(option);
}

static struct adv_conf_option_struct* option_search_tag(adv_conf* context, const char* tag)
{
	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			if (strcmp(option->tag, tag)==0)
				return option;
			option = option->next;
		} while (option != context->option_list);
	}
	return 0;
}

/**
 * Search a option for partial match for a whole subtag.
 * The search complete with success only if an unique option is found.
 */
static struct adv_conf_option_struct* option_search_tag_partial_whole(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* found = 0;

	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			if (partial_match_whole(option->tag, tag)) {
				if (found) {
					log_std(("conf: multiple match %s and %s for %s\n", found->tag, option->tag, tag));
					return 0; /* multiple match */
				}
				found = option;
			}
			option = option->next;
		} while (option != context->option_list);
	}


	return found;
}

/**
 * Search a option for partial match.
 * The search complete with success only if an unique option is found.
 */
static struct adv_conf_option_struct* option_search_tag_partial(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* found = 0;

	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			if (partial_match(option->tag, tag)) {
				if (found) {
					log_std(("conf: multiple match %s and %s for %s\n", found->tag, option->tag, tag));
					return 0; /* multiple match */
				}
				found = option;
			}
			option = option->next;
		} while (option != context->option_list);
	}


	return found;
}

static struct adv_conf_input_struct* input_alloc(void)
{
	struct adv_conf_input_struct* input = (struct adv_conf_input_struct*)malloc(sizeof(struct adv_conf_input_struct));
	input->file_in = 0;
	input->file_out = 0;
	input->conv_map = 0;
	input->conv_mac = 0;
	return input;
}

static void input_free(struct adv_conf_input_struct* input)
{
	unsigned i;
	free(input->file_in);
	free(input->file_out);
	for(i=0;i<input->conv_mac;++i) {
		free((void*)input->conv_map[i].section_glob);
		free((void*)input->conv_map[i].section_result);
		free((void*)input->conv_map[i].tag_glob);
		free((void*)input->conv_map[i].tag_result);
		free((void*)input->conv_map[i].value_glob);
		free((void*)input->conv_map[i].value_result);
	}
	free(input->conv_map);
	free(input);
}

static void input_insert(adv_conf* context, struct adv_conf_input_struct* input)
{
	if (context->input_list) {
		input->pred = context->input_list->pred;
		input->next = context->input_list;
		input->pred->next = input;
		input->next->pred = input;
	} else {
		context->input_list = input;
		input->next = input;
		input->pred = input;
	}
}

static struct adv_conf_input_struct* input_searchbest_writable(adv_conf* context)
{
	if (context->input_list) {
		struct adv_conf_input_struct* best_input = 0;
		struct adv_conf_input_struct* input = context->input_list;

		do {
			if (input->file_out) {
				if (!best_input || best_input->priority < input->priority) {
					best_input = input;
				}
			}
			input = input->next;
		} while (input != context->input_list);

		return best_input;
	}

	return 0;
}

static struct adv_conf_value_struct* value_alloc(struct adv_conf_option_struct* option)
{
	struct adv_conf_value_struct* value = (struct adv_conf_value_struct*)malloc(sizeof(struct adv_conf_value_struct));

	value->option = option;

	value->section = 0;
	value->comment = 0;
	value->format = 0;

	switch (option->type) {
		case conf_type_string : value->data.string_value = 0; break;
		default: break;
	}

	return value;
}

static void value_free(struct adv_conf_value_struct* value)
{
	if (value->option) {
		switch (value->option->type) {
			case conf_type_string : free(value->data.string_value); break;
			default: break;
		}
	}
	free(value->section);
	free(value->comment);
	free(value->format);
	free(value);
}

static void value_insert(adv_conf* context, struct adv_conf_value_struct* value)
{
	if (context->value_list) {
		value->pred = context->value_list->pred;
		value->next = context->value_list;
		value->pred->next = value;
		value->next->pred = value;
	} else {
		context->value_list = value;
		value->next = value;
		value->pred = value;
	}

	context->is_modified = 1;
}

static int value_cmp(struct adv_conf_value_struct* a, struct adv_conf_value_struct* b)
{
	int r;

	r = strcmp(a->section, b->section);
	if (r)
		return r;

	r = strcmp(a->option->tag, b->option->tag);
	return r;
}

static void value_insert_sort(adv_conf* context, struct adv_conf_value_struct* value)
{
	if (context->value_list) {
		struct adv_conf_value_struct* i = context->value_list;
		if (value_cmp(i, value) <= 0) {
			do {
				i = i->next;
			} while (i != context->value_list && value_cmp(i, value) <= 0);
		} else {
			context->value_list = value;
		}
		value->pred = i->pred;
		value->next = i;
		value->pred->next = value;
		value->next->pred = value;
	} else {
		context->value_list = value;
		value->next = value;
		value->pred = value;
	}

	context->is_modified = 1;
}

static void value_remove(adv_conf* context, struct adv_conf_value_struct* value)
{
	if (context->value_list == value) {
		context->value_list = value->next;
	}
	if (context->value_list == value) {
		context->value_list = 0;
	} else {
		value->next->pred = value->pred;
		value->pred->next = value->next;
	}

	context->is_modified = 1;

	value_free(value);
}

static struct adv_conf_value_struct* value_searchbest_sectiontag(adv_conf* context, const char* section, const char* tag)
{
	if (context->value_list) {
		struct adv_conf_value_struct* best_value = 0;
		struct adv_conf_value_struct* value = context->value_list;

		do {
			if (strcmp(value->section, section)==0
				&& strcmp(value->option->tag, tag)==0) {
				if (!best_value || best_value->input->priority < value->input->priority) {
					best_value = value;
				}
			}
			value = value->next;
		} while (value != context->value_list);

		return best_value;
	}

	return 0;
}

static struct adv_conf_value_struct* value_search_inputsectiontag(adv_conf* context, struct adv_conf_input_struct* input, const char* section, const char* tag)
{
	if (context->value_list) {
		struct adv_conf_value_struct* value = context->value_list;

		do {
			if (value->input == input
				&& strcmp(value->section, section)==0
				&& strcmp(value->option->tag, tag)==0) {
				return value;
			}
			value = value->next;
		} while (value != context->value_list);
	}

	return 0;
}

static struct adv_conf_value_struct* value_searchbest_tag(adv_conf* context, const char** section_map, unsigned section_mac, const char* tag)
{
	unsigned i;

	for(i=0;i<section_mac;++i) {
		struct adv_conf_value_struct* value;
		value = value_searchbest_sectiontag(context, section_map[i], tag);
		if (value)
			return value;
	}

	return 0;
}

static struct adv_conf_value_struct* value_searchbest_from(adv_conf* context, struct adv_conf_value_struct* like_value)
{
	struct adv_conf_value_struct* value = like_value->next;

	while (value != context->value_list) {

		if (value->option == like_value->option
			&& value->input == like_value->input
			&& strcmp(value->section, like_value->section)==0) {
			return value;
		}

		value = value->next;
	}

	return 0;
}

/***************************************************************************/
/* Init/Done */

/**
 * Initialization of the configuration system.
 * You can have more than one configuration context active at the same time.
 * \return A new configuration context. It must be deallocated calling conf_done().
 */
adv_conf* conf_init(void)
{
	adv_conf* context = malloc(sizeof(adv_conf));

	context->option_list = 0;
	context->input_list = 0;
	context->value_list = 0;

	context->section_mac = 0;
	context->section_map = 0;

	context->is_modified = 0;

	return context;
}

/**
 * Deinitialization of the configuration system.
 * Free all the memory and updates the configuration files.
 * After this function you can reinitialize calling the conf_init() function.
 * \param context Configuration context to deallocate.
 */
void conf_done(adv_conf* context)
{
	unsigned i;

	if (context->value_list) {
		struct adv_conf_value_struct* value = context->value_list;
		do {
			struct adv_conf_value_struct* value_next = value->next;
			value_free(value);
			value = value_next;
		} while (value != context->value_list);
	}

	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			struct adv_conf_option_struct* option_next = option->next;
			option_free(option);
			option = option_next;
		} while (option != context->option_list);
	}

	if (context->input_list) {
		struct adv_conf_input_struct* input = context->input_list;
		do {
			struct adv_conf_input_struct* input_next = input->next;
			input_free(input);
			input = input_next;
		} while (input != context->input_list);
	}

	for(i=0;i<context->section_mac;++i)
		free(context->section_map[i]);
	free(context->section_map);

	free(context);
}

/***************************************************************************/
/* Register */

/**
 * Check if a tag is already registered.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 */
adv_bool conf_is_registered(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_search_tag(context, tag);
	return option != 0;
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 */
void conf_bool_register(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_bool;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_bool.has_def = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 */
void conf_bool_register_default(adv_conf* context, const char* tag, adv_bool def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_bool;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_bool.has_def = 1;
	option->data.base_bool.def = def;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 */
void conf_int_register(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 0;
	option->data.base_int.has_limit = 0;
	option->data.base_int.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 */
void conf_int_register_default(adv_conf* context, const char* tag, int def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 1;
	option->data.base_int.def = def;
	option->data.base_int.has_limit = 0;
	option->data.base_int.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param limit_low Low limit of the numerical value.
 * \param limit_high High limit of the numerical value.
 */
void conf_int_register_limit(adv_conf* context, const char* tag, int limit_low, int limit_high)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 0;
	option->data.base_int.has_limit = 1;
	option->data.base_int.limit_low = limit_low;
	option->data.base_int.limit_high = limit_high;
	option->data.base_int.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 * \param limit_low Low limit of the numerical value.
 * \param limit_high High limit of the numerical value.
 */
void conf_int_register_limit_default(adv_conf* context, const char* tag, int limit_low, int limit_high, int def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 1;
	option->data.base_int.def = def;
	option->data.base_int.has_limit = 1;
	option->data.base_int.limit_low = limit_low;
	option->data.base_int.limit_high = limit_high;
	option->data.base_int.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 */
void conf_float_register(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_float;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_float.has_def = 0;
	option->data.base_float.has_limit = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 */
void conf_float_register_default(adv_conf* context, const char* tag, double def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_float;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_float.has_def = 1;
	option->data.base_float.def = def;
	option->data.base_float.has_limit = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 * \param limit_low Low limit of the numerical value.
 * \param limit_high High limit of the numerical value.
 */
void conf_float_register_limit_default(adv_conf* context, const char* tag, double limit_low, double limit_high, double def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_float;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_float.has_def = 1;
	option->data.base_float.def = def;
	option->data.base_float.has_limit = 1;
	option->data.base_float.limit_low = limit_low;
	option->data.base_float.limit_high = limit_high;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 */
void conf_string_register(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 0;
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 */
void conf_string_register_multi(adv_conf* context, const char* tag)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 1;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 0;
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 */
void conf_string_register_default(adv_conf* context, const char* tag, const char* def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 1;
	option->data.base_string.def = strdup(def);
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 * \param enum_map Vector for the enumeration value. The tag can assume only one of these values.
 * \param enum_mac Elements in the enumeration vector.
 */
void conf_string_register_enum_default(adv_conf* context, const char* tag, adv_conf_enum_string* enum_map, unsigned enum_mac, const char* def)
{
	struct adv_conf_option_struct* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 1;
	option->data.base_string.def = strdup(def);
	option->data.base_string.has_enum = 1;
	option->data.base_string.enum_map = enum_map;
	option->data.base_string.enum_mac = enum_mac;

	option_insert(context, option);
}

/**
 * Register a tag.
 * A tag can be registered only one time.
 * \note All the pointer arguments must be keep valid until the conf_done() call.
 * \param context Configuration context to use.
 * \param tag Tag of the value.
 * \param def Default value.
 * \param enum_map Vector for the enumeration value. The tag can assume only one of these values.
 * \param enum_mac Elements in the enumeration vector.
 */
void conf_int_register_enum_default(adv_conf* context, const char* tag, adv_conf_enum_int* enum_map, unsigned enum_mac, int def)
{
	struct adv_conf_option_struct* option = option_alloc();
	unsigned i;

	assert(option_search_tag(context, tag) == 0);

	for(i=0;i<enum_mac;++i)
		if (enum_map[i].map == def)
			break;

	assert(i != enum_mac);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 1;
	option->data.base_int.def = def;
	option->data.base_int.has_limit = 0;
	option->data.base_int.has_enum = 1;
	option->data.base_int.enum_map = enum_map;
	option->data.base_int.enum_mac = enum_mac;

	option_insert(context, option);
}

/***************************************************************************/
/* Import */

/**
 * Import of a new value.
 * On error the value struct is not modified.
 */

static adv_error import_bool(struct adv_conf_value_struct* value, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	if (strcmp(own_value, "yes")==0) {
		value->data.bool_value = 1;
		free(own_value);
		free(value->format);
		value->format = own_format;
		return 0;
	} else if (strcmp(own_value, "no")==0) {
		value->data.bool_value = 0;
		free(own_value);
		free(value->format);
		value->format = own_format;
		return 0;
	} else {
		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, "Valid arguments are 'yes' and 'no'", "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_value);
		free(own_format);
		return -1;
	}
}

static adv_error import_int(struct adv_conf_value_struct* value, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	if (value->option->data.base_int.has_enum) {
		unsigned i;
		adv_string valid;
		char* own_valid;
		char* endp;
		int r;

		/* try comparing the string */
		for(i=0;i<value->option->data.base_int.enum_mac;++i) {
			if (strcmp(value->option->data.base_int.enum_map[i].value, own_value)==0) {
				value->data.int_value = value->option->data.base_int.enum_map[i].map;
				free(own_value);
				free(value->format);
				value->format = own_format;
				return 0;
			}
		}

		/* try converting the real value */
		r = strtol(own_value, &endp, 10);
		if (*endp == 0 && *own_value != 0) {
			/* check if is an allowed value */
			for(i=0;i<value->option->data.base_int.enum_mac;++i) {
				if (value->option->data.base_int.enum_map[i].map == r) {
					value->data.int_value = value->option->data.base_int.enum_map[i].map;
					free(own_value);
					free(value->format);
					value->format = own_format;
					return 0;
				}
			}
		}

		inc_str_init(&valid);
		inc_str_cat(&valid, "Valid values are ");
		for(i=0;i<value->option->data.base_int.enum_mac;++i) {
			if (i)
				inc_str_cat(&valid, ", ");
			inc_str_cat(&valid, "'");
			inc_str_cat(&valid, value->option->data.base_int.enum_map[i].value);
			inc_str_cat(&valid, "'");
		}
		own_valid = inc_str_alloc(&valid);
		inc_str_done(&valid);

		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, own_valid, "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_valid);
		free(own_value);
		free(own_format);
		return -1;
	} else {
		char* endp;
		int r = strtol(own_value, &endp, 10);

		if (*endp != 0
			|| *own_value == 0
			|| (value->option->data.base_int.has_limit
				&& (r < value->option->data.base_int.limit_low || r > value->option->data.base_int.limit_high))
			) {
				char valid_buffer[128];
				snprintf(valid_buffer, sizeof(valid_buffer), "Valid arguments are int from %d to %d", value->option->data.base_int.limit_low, value->option->data.base_int.limit_high);
				if (error)
					error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, valid_buffer, "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
				free(own_value);
				free(own_format);
				return -1;
		}

		value->data.int_value = r;
		free(own_value);
		free(value->format);
		value->format = own_format;

		return 0;
	}
}

static adv_error import_float(struct adv_conf_value_struct* value, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	char* endp;

	double r = strtod(own_value, &endp);

	if (*endp != 0
		|| *own_value == 0
		|| (value->option->data.base_float.has_limit
			&& (r < value->option->data.base_float.limit_low || r > value->option->data.base_float.limit_high)
		)) {
		char valid_buffer[128];
		snprintf(valid_buffer, sizeof(valid_buffer), "Valid arguments are float from %g to %g", value->option->data.base_float.limit_low, value->option->data.base_float.limit_high);
		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, valid_buffer, "Out of range argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_value);
		free(own_format);
		return -1;
	}

	value->data.float_value = r;
	free(own_value);
	free(value->format);
	value->format = own_format;

	return 0;
}

static adv_error import_string(struct adv_conf_value_struct* value, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	unsigned i;
	char* own_valid;
	adv_string valid;

	if (!value->option->data.base_string.has_enum) {
		free(value->data.string_value);
		value->data.string_value = own_value;
		free(value->format);
		value->format = own_format;
		return 0;
	}

	/* try comparing the string */
	for(i=0;i<value->option->data.base_string.enum_mac;++i) {
		if (strcmp(value->option->data.base_string.enum_map[i].value, own_value)==0) {
			free(value->data.string_value);
			value->data.string_value = own_value;
			free(value->format);
			value->format = own_format;
			return 0;
		}
	}

	inc_str_init(&valid);
	inc_str_cat(&valid, "Valid values are ");
	for(i=0;i<value->option->data.base_string.enum_mac;++i) {
		if (i)
			inc_str_cat(&valid, ", ");
		inc_str_cat(&valid, "'");
		inc_str_cat(&valid, value->option->data.base_string.enum_map[i].value);
		inc_str_cat(&valid, "'");
	}
	own_valid = inc_str_alloc(&valid);
	inc_str_done(&valid);

	if (error)
		error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, own_valid, "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);

	free(own_valid);
	free(own_value);
	free(own_format);
	return -1;
}

/**
 * Import a value. The value item is NOT initialized before the call.
 */
static adv_error value_import(struct adv_conf_value_struct* value, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	switch (value->option->type) {
		case conf_type_bool : return import_bool(value, own_value, own_format, error, error_context);
		case conf_type_int : return import_int(value, own_value, own_format, error, error_context);
		case conf_type_float : return import_float(value, own_value, own_format, error, error_context);
		case conf_type_string : return import_string(value, own_value, own_format, error, error_context);
		default:
			assert(0);
			return -1;
	}
}

/***************************************************************************/
/* Load */

static adv_error value_make_raw(adv_conf* context, struct adv_conf_input_struct* input, struct adv_conf_option_struct* option, char* own_section, char* own_comment, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	struct adv_conf_value_struct* value;

	value = value_alloc(option);

	value->input = input;
	value->section = own_section;
	value->comment = own_comment;
	value->format = 0;

	if (value_import(value, own_value, own_format, error, error_context) != 0) {
		value_free(value);
		return -1;
	}

	value_insert(context, value);

	return 0;
}

static adv_error value_make_own(adv_conf* context, struct adv_conf_input_struct* input, struct adv_conf_option_struct* option, char* own_section, char* own_comment, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	/* check for not multi options */
	if (!option->is_multi) {
		if (value_search_inputsectiontag(context, input, own_section, option->tag)) {
			if (error)
				error(error_context, conf_error_invalid, input->file_in, option->tag, 0, "Duplicate specification of the option '%s' in file '%s'", option->tag, input->file_in);
			goto err_free;
		}
	}

	return value_make_raw(context, input, option, own_section, own_comment, own_value, own_format, error, error_context);

err_free:
	free(own_section);
	free(own_comment);
	free(own_format);
	free(own_value);
	return -1;
}

static adv_error value_make_dup(adv_conf* context, struct adv_conf_input_struct* input, struct adv_conf_option_struct* option, const char* section, const char* comment, const char* value, const char* format, conf_error_callback* error, void* error_context)
{
	char* own_section;
	char* own_comment;
	char* own_value;
	char* own_format;

	/* check for not multi options */
	if (!option->is_multi) {
		if (value_search_inputsectiontag(context, input, section, option->tag)) {
			if (error)
				error(error_context, conf_error_invalid, input->file_in, option->tag, 0, "Duplicate specification of the option '%s' in file '%s'", option->tag, input->file_in);
			goto err;
		}
	}

	own_section = strdup(section);
	own_comment = strdup(comment);
	own_value = strdup(value);
	own_format = strdup(format);

	return value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context);

err:
	return -1;
}

static char* format_alloc(const char* value)
{
	unsigned eol;
	const char* s;
	char* d;
	char* r;

	eol = 0;
	s = value;
	while (*s) {
		if (*s == '\n')
			++eol;
		++s;
	}

	r = malloc(strlen(value) + eol + 1);

	s = value;
	d = r;
	while (*s) {
		if (*s == '\n')
			*d++ = '\\';
		*d++ = *s;
		++s;
	}

	*d = 0;

	return r;
}

/**
 * Insert if it doesn't exist without error checks.
 * Return the existing value if any.
 */
static adv_error value_set_dup(adv_conf* context, struct adv_conf_input_struct* input, struct adv_conf_option_struct* option, const char* section, const char* comment, const char* value, const char* format, conf_error_callback* error, void* error_context)
{
	/* check for not multi options */
	if (!option->is_multi) {
		struct adv_conf_value_struct* value_exist = value_search_inputsectiontag(context, input, section, option->tag);
		if (value_exist) {
			context->is_modified = 1;
			return value_import(value_exist, strdup(value), format_alloc(value), error, error_context);
		}
	}

	return value_make_dup(context, input, option, section, comment, value, format, error, error_context);
}

static adv_error input_value_insert(adv_conf* context, struct adv_conf_input_struct* input, char** global_section, char* own_comment, char* own_sectiontag, char* own_value, char* own_format, conf_error_callback* error, void* error_context)
{
	struct adv_conf_option_struct* option;
	char* slash;
	char* own_section;
	char* own_tag;
	int autoreg;

	slash = strrchr(own_sectiontag, '/');
	if (slash!=0) {
		*slash = 0;
		own_section = strdup(own_sectiontag);
		own_tag = strdup(slash + 1);
		free(own_sectiontag);
		/* clear the global section is another section is present */
		if (*global_section) {
			free(*global_section);
			*global_section = 0;
		}
	} else {
		if (*global_section)
			own_section = strdup(*global_section);
		else
			own_section = strdup("");
		own_tag = own_sectiontag;
	}

	autoreg = ADV_CONF_CONV_AUTOREG_NO;
	if (input->conv_mac) {
		unsigned conv;
		/* use the conversion map to substitute options */
		for(conv=0;conv<input->conv_mac;++conv) {
			if (sglob(own_section, input->conv_map[conv].section_glob)
				&& sglob(own_tag, input->conv_map[conv].tag_glob)
				&& sglob(own_value, input->conv_map[conv].value_glob)
			) {
				autoreg = input->conv_map[conv].autoreg;
				own_section = glob_subst(input->conv_map[conv].section_result, own_section);
				own_tag = glob_subst(input->conv_map[conv].tag_result, own_tag);
				own_value = glob_subst(input->conv_map[conv].value_result, own_value);
				free(own_format);
				own_format = format_alloc(own_value);

				/* don't stop and continue on the next conversion */
			}
		}
	}

	if (!*own_tag) {
		/* empty tag, ignore it */
		free(own_section);
		free(own_tag);
		free(own_comment);
		free(own_value);
		free(own_format);
		return 0;
	}

	option = option_search_tag(context, own_tag);
	if (!option) {

		if (autoreg != ADV_CONF_CONV_AUTOREG_NO) {
			/* auto register the option */
			option = option_alloc();
			option->type = conf_type_string;
			option->is_multi = autoreg == ADV_CONF_CONV_AUTOREG_MULTI;
			option->tag = own_tag;
			option->data.base_string.has_def = 0;
			option->data.base_string.has_enum = 0;
			option_insert(context, option);

			if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
				goto err;

			return 0;
		}

		if (input->ignore_unknown_flag) {
			/* ignore */
			free(own_section);
			free(own_tag);
			free(own_comment);
			free(own_value);
			free(own_format);
			return 0;
		}

		error(error_context, conf_error_unknown, input->file_in, own_tag, 0, "Unknown option '%s' in file '%s'", own_tag, input->file_in);
		goto err_free;
	}

	free(own_tag);

	if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
		goto err;

	return 0;

err_free:
	free(own_section);
	free(own_tag);
	free(own_comment);
	free(own_value);
	free(own_format);

err:
	return -1;
}

static adv_error input_section_insert(adv_conf* context, struct adv_conf_input_struct* input, char** global_section, char* own_comment, char* own_tag, char* own_value, char* own_format)
{
	char* own_section;

	/* remove [] */
	own_tag[strlen(own_tag)-1] = 0;
	own_section = strdup(own_tag + 1);

	free(*global_section);
	*global_section = own_section;

	free(own_comment);
	free(own_tag);
	free(own_value);
	free(own_format);

	return 0;
}

static adv_error input_value_load(adv_conf* context, struct adv_conf_input_struct* input, FILE* f, char** global_section, adv_bool multi_line, conf_error_callback* error, void* error_context)
{
	int c;

	enum state_type {
		state_comment,
		state_comment_line,
		state_tag,
		state_before_equal, /* conversion from the old = option format */
		state_after_equal,
		state_value,
		state_value_line,
		state_eof
	};

	enum state_type state;

	adv_string icomment;
	adv_string itag;
	adv_string ivalue;
	adv_string iformat;

	char* comment;
	char* tag;
	char* value;
	char* format;

	inc_str_init(&icomment);
	inc_str_init(&itag);
	inc_str_init(&iformat);
	inc_str_init(&ivalue);

	/* read the comment */
	state = state_comment;
	do {
		unsigned copy = 0;

		enum copy_type {
			copy_in_comment = 0x1,
			copy_in_tag = 0x2,
			copy_in_value = 0x4,
			copy_in_format = 0x8
		};

		c = fgetc(f);

		switch (state) {
			case state_comment :
				if (c == EOF) {
					state = state_eof;
				} else if (c == '#') {
					state = state_comment_line;
					copy |= copy_in_comment;
				} else if (!isspace(c)) {
					state = state_tag;
					copy |= copy_in_tag;
				} else {
					copy |= copy_in_comment;
				}
				break;
			case state_comment_line :
				if (c == EOF) {
					state = state_eof;
				} else if (c == '\n') {
					state = state_comment;
					copy |= copy_in_comment;
				} else {
					copy |= copy_in_comment;
				}
				break;
			case state_tag : /* tag */
				if (c == EOF) {
					state = state_eof;
				} else if (c == '\n') {
					state = state_eof;
				} else if (c == '=') {
					state = state_after_equal;
				} else if (isspace(c)) {
					state = state_before_equal;
				} else {
					copy |= copy_in_tag;
				}
				break;
			case state_before_equal :
				if (c == EOF) {
					state = state_eof;
				} else if (c == '\n') {
					state = state_eof;
				} else if (c == '\\' && multi_line) {
					state = state_value_line;
					copy |= copy_in_format;
				} else if (c == '=') {
					state = state_after_equal;
				} else if (!isspace(c)) {
					state = state_value;
					copy |= copy_in_value | copy_in_format;
				}
				break;
			case state_after_equal :
				if (c == EOF) {
					state = state_eof;
				} else if (c == '\n') {
					state = state_eof;
				} else if (c == '\\' && multi_line) {
					state = state_value_line;
					copy |= copy_in_format;
				} else if (!isspace(c)) {
					state = state_value;
					copy |= copy_in_value | copy_in_format;
				}
				break;
			case state_value : /* effective value */
				if (c == EOF) {
					state = state_eof;
				} else if (c == '\n') {
					state = state_eof;
				} else if (c == '\\' && multi_line) {
					state = state_value_line;
					copy |= copy_in_format;
				} else {
					copy |= copy_in_value | copy_in_format;
				}
				break;
			case state_value_line :
				if (c == EOF) {
					state = state_eof;
					c = '\\';
					copy |= copy_in_value;
				} else if (c == '\n') {
					state = state_value;
					copy |= copy_in_value | copy_in_format;
				} else {
					ungetc(c, f);
					state = state_value;
					c = '\\';
					copy |= copy_in_value;
				}
				break;
			case state_eof:
				break;
		}

		if ((copy & copy_in_comment) != 0) {
			if (inc_str_catc(&icomment, c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_tag) != 0) {
			if (inc_str_catc(&itag, c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_value) != 0) {
			if (inc_str_catc(&ivalue, c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_format) != 0) {
			if (inc_str_catc(&iformat, c) != 0)
				goto err_done;
		}

	} while (state != state_eof);

	if (state == state_eof && ferror(f)) {
		if (error)
			error(error_context, conf_error_failure, input->file_in, 0, 0, "Error reading the file %s, %s.", input->file_in, strerror(errno));
		goto err_done;
	}

	if (inc_str_len(&itag) != 0) {
		unsigned i;

		comment = inc_str_alloc(&icomment);
		tag = inc_str_alloc(&itag);
		value = inc_str_alloc(&ivalue);
		format = inc_str_alloc(&iformat);

		/* remove any space at the end of the value */
		i = inc_str_len(&ivalue);
		while (i && isspace(value[i-1]))
			value[--i] = 0;

		if (!comment || !tag || !value || !format)
			goto err_free;

		/* conversion from the old [] section format */
		if (tag[0] == '[' && tag[strlen(tag)-1]==']') {
			if (input_section_insert(context, input, global_section, comment, tag, value, format)!=0)
				goto err_done;
		} else {
			if (input_value_insert(context, input, global_section, comment, tag, value, format, error, error_context) != 0)
				goto err_done;
		}
	}

	inc_str_done(&icomment);
	inc_str_done(&itag);
	inc_str_done(&ivalue);
	inc_str_done(&iformat);

	return 0;

err_free:
	free(comment);
	free(tag);
	free(value);
	free(format);

err_done:
	inc_str_done(&icomment);
	inc_str_done(&itag);
	inc_str_done(&ivalue);
	inc_str_done(&iformat);

	return -1;
}

static adv_error input_load(adv_conf* context, struct adv_conf_input_struct* input, adv_bool multi_line, conf_error_callback* error, void* error_context)
{
	FILE* f;

	char* global_section;

	f = fopen(input->file_in, "rt");
	if (!f) {
		if (error)
			error(error_context, conf_error_failure, input->file_in, 0, 0, "Error opening the file %s for reading, %s.", input->file_in, strerror(errno));
		goto err;
	}

	global_section = 0;

	while (!feof(f)) {
		if (input_value_load(context, input, f, &global_section, multi_line, error, error_context) != 0) {
			goto err_free;
		}
	}

	free(global_section);

	fclose(f);
	return 0;
err_free:
	free(global_section);
	fclose(f);
err:
	return -1;
}

/**
 * Load an input file and store it in memory.
 * The old format file (Windows .INI like) is only read, the file is written
 * with the new .RC format.
 * Any unknown option in the old format is ignored.
 * \param context Configuration context to use.
 * \param priority Priority of the file. When you get an option the files with
 * an higher priority are searched first.
 * \param file_in Path of the configuration file in the old format.
 * \param file_out Path of the output configuration, use 0 for a readonly file.
 * \param ignore_unknown Ignore any unknown option.
 * \param multi_line Support the multi line values terminating with a backslash.
 * \param conv_map Vector of conversion elements. Use 0 for none. All the elements are
 * applied in order. If an element match the substitution is done and the conversion
 * continues checking the next element and so on.
 * \param conv_mac Number of elements in the conversion vector. Use 0 for none.
 * \param error Callback called for every error.
 * \param error_context Argument for the error callback.
 * \return
 *   - ==0 if ok
 *   - !=0 if not ok and error callback called
 */
adv_error conf_input_file_load_adv(adv_conf* context, int priority, const char* file_in, const char* file_out, adv_bool ignore_unknown, adv_bool multi_line, const adv_conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context)
{
	struct adv_conf_input_struct* input;

	adv_bool is_file_in_exist = file_in != 0 && access(file_in, F_OK) == 0;

	/* ignore if it doesn't exist and isn't writable */
	if (!is_file_in_exist && !file_out)
		return 0;

	input = input_alloc();

	input->priority = priority;
	input->ignore_unknown_flag = ignore_unknown;

	if (file_in)
		input->file_in = strdup(file_in);
	else
		input->file_in = 0;

	if (file_out)
		input->file_out = strdup(file_out);
	else
		input->file_out = 0;

	if (conv_mac) {
		unsigned i;
		input->conv_map = malloc(conv_mac * sizeof(adv_conf_conv));
		for(i=0;i<conv_mac;++i) {
			input->conv_map[i].section_glob = strdup(conv_map[i].section_glob);
			input->conv_map[i].section_result = strdup(conv_map[i].section_result);
			input->conv_map[i].tag_glob = strdup(conv_map[i].tag_glob);
			input->conv_map[i].tag_result = strdup(conv_map[i].tag_result);
			input->conv_map[i].value_glob = strdup(conv_map[i].value_glob);
			input->conv_map[i].value_result = strdup(conv_map[i].value_result);
			input->conv_map[i].autoreg = conv_map[i].autoreg;
		}
		input->conv_mac = conv_mac;
	} else {
		input->conv_map = 0;
		input->conv_mac = 0;
	}

	input_insert(context, input);

	if (is_file_in_exist) {
		if (input_load(context, input, multi_line, error, error_context) != 0)
			return -1;
	}

	return 0;
}

/**
 * Load an input file and store it in memory.
 * \param context Configuration context to use.
 * \param priority Priority of the file. When you get an option the files with
 * an higher priority are searched first.
 * \param file Path of the configuration file.
 * \param error Callback called for every error.
 * \param error_context Argument for the error callback.
 * \return
 *   - ==0 if ok
 *   - !=0 if not ok and error callback called
 */
adv_error conf_input_file_load(adv_conf* context, int priority, const char* file, conf_error_callback* error, void* error_context)
{
	return conf_input_file_load_adv(context, priority, file, file, 0, 1, 0, 0, error, error_context);
}

/**
 * Set the list of input arguments.
 * After the call the argc and argv arguments contain only the unused options.
 * \param context Configuration context to use.
 * \param priority Priority of the file. When you get an option the files with
 * an higher priority are searched first.
 * \param section Section to use for the command line arguments.
 * \param argc Pointer at the number of arguments.
 * \param argv Pointer at the arguments.
 * \param error Callback called for every error.
 * \param error_context Argument for the error callback.
 */
adv_error conf_input_args_load(adv_conf* context, int priority, const char* section, int* argc, char* argv[], conf_error_callback* error, void* error_context)
{
	int i;
	struct adv_conf_input_struct* input = input_alloc();

	input->priority = priority;
	input->ignore_unknown_flag = 1;
	input->file_in = strdup("commandline");
	input->file_out = 0;

	input_insert(context, input);

	i = 0;
	while (i<*argc) {
		adv_bool noformat;
		struct adv_conf_option_struct* option;
		const char* tag;
		unsigned used;

		tag = target_option_extract(argv[i]);

		if (!tag) {
			++i;
			continue;
		}

		noformat = 0;
		option = 0;

		/* exact search */
		if (!option) {
			option = option_search_tag(context, tag);
		}

		if (!option) {
			if (tag[0]=='n' && tag[1]=='o') {
				option = option_search_tag(context, tag + 2);
				if (option && option->type == conf_type_bool) {
					noformat = 1;
				} else {
					option = 0;
				}
			}
		}

		/* partial search whole */
		if (!option) {
			option = option_search_tag_partial_whole(context, tag);
		}

		if (!option) {
			if (tag[0]=='n' && tag[1]=='o') {
				option = option_search_tag_partial_whole(context, tag + 2);
				if (option && option->type == conf_type_bool) {
					noformat = 1;
				} else {
					option = 0;
				}
			}
		}

		/* partial search */
		if (!option) {
			option = option_search_tag_partial(context, tag);
		}

		if (!option) {
			if (tag[0]=='n' && tag[1]=='o') {
				option = option_search_tag_partial(context, tag + 2);
				if (option && option->type == conf_type_bool) {
					noformat = 1;
				} else {
					option = 0;
				}
			}
		}

		if (!option
			|| (option->type != conf_type_bool && i+1>=*argc)) {
			++i;
			continue;
		}

		if (option->type == conf_type_bool) {
			char* own_section = strdup(section);
			char* own_comment = strdup("");
			char* own_value;
			char* own_format;
			if (noformat)
				own_value = strdup("no");
			else
				own_value = strdup("yes");
			own_format = format_alloc(own_value);

			if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
				return -1;

			used = 1;
		} else {
			char* own_section = strdup(section);
			char* own_comment = strdup("");
			char* own_value = strdup(argv[i+1]);
			char* own_format = format_alloc(own_value);

			if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
				return -1;

			used = 2;
		}

		while (used) {
			int j;
			for(j=i;j<*argc;++j) {
				argv[j] = argv[j+1];
			}
			--*argc;
			--used;
		}
	}

	return 0;
}

/***************************************************************************/
/* Save */

static adv_error value_save(struct adv_conf_input_struct* input, struct adv_conf_value_struct* value, const char** global_section, FILE* f, conf_error_callback* error, void* error_context)
{
	if (strcmp(*global_section, value->section) != 0) {
		*global_section = value->section;
	}
	if (*value->section)
		fprintf(f, "%s%s/%s %s\n", value->comment, value->section, value->option->tag, value->format);
	else
		fprintf(f, "%s%s %s\n", value->comment, value->option->tag, value->format);

	if (ferror(f)) {
		if (error)
			error(error_context, conf_error_failure, input->file_out, 0, 0, "Error writing the file %s, %s.", input->file_out, strerror(errno));
		goto err;
	}

	return 0;

err:
	return -1;
}

static adv_error input_save(adv_conf* context, struct adv_conf_input_struct* input, adv_bool quiet, conf_error_callback* error, void* error_context)
{
	FILE* f;
	const char* global_section;

	/* if not writable skip */
	if (!input->file_out)
		return 0;

	global_section = "";

	f = fopen(input->file_out, "wt");
	if (!f) {
		if (!quiet || errno != EACCES) {
			if (error)
				error(error_context, conf_error_failure, input->file_out, 0, 0, "Error opening the file %s for writing, %s.", input->file_out, strerror(errno));
		}
		goto err;
	}

	if (context->value_list) {
		struct adv_conf_value_struct* value = context->value_list;
		do {
			if (value->input == input) {
				if (value_save(input, value, &global_section, f, error, error_context) != 0) {
					goto err_close;
				}
			}
			value = value->next;
		} while (value != context->value_list);
	}

	fclose(f);
	return 0;

err_close:
	fclose(f);
err:
	return -1;
}

/**
 * Updates all the writable configuration files.
 * \param context Configuration context to use.
 * \param force Force the rewrite also if the configuration file are unchanged.
 * \param quiet Quiet on normal error. For example if the file is not writable.
 * \param error Callback called for every error.
 * \param error_context Argument for the error callback.
 */
adv_error conf_save(adv_conf* context, adv_bool force, adv_bool quiet, conf_error_callback* error, void* error_context)
{
	/* only if necessary */
	if (!force && !context->is_modified)
		return 0;

	if (context->input_list) {
		struct adv_conf_input_struct* input = context->input_list;
		do {
			if (input_save(context, input, quiet, error, error_context)!=0)
				return -1;
			input = input->next;
		} while (input != context->input_list);
	}

	context->is_modified = 0;

	return 0;
}

/**
 * Remove all the comments.
 * All the comments present are removed.
 * The files must be explicitly saved calling conf_save().
 * \param context Configuration context to use.
 */
void conf_uncomment(adv_conf* context)
{
	struct adv_conf_value_struct* value = context->value_list;

	if (value) {
		do {
			free(value->comment);
			value->comment = strdup("");

			value = value->next;
		} while (value != context->value_list);
	}
}

/**
 * Sort all the configuration options.
 * All the option in all the loaded files are ordered by their name.
 * The files must be explicitly saved calling conf_save().
 * \param context Configuration context to use.
 */
void conf_sort(adv_conf* context)
{
	struct adv_conf_value_struct* value_list = context->value_list;
	struct adv_conf_value_struct* value = value_list;

	context->value_list = 0;

	if (value_list) {
		do {
			struct adv_conf_value_struct* value_next = value->next;
			value_insert_sort(context, value);
			value = value_next;
		} while (value != value_list);
	}

	context->is_modified = 0;
}

/***************************************************************************/
/* Get */

/**
 * Set the list of sections to try for every get.
 * Every conf_get() operation search on this list of section from the first to
 * the latter for the specified tag.
 * You can change this list at every time.
 * \param context Configuration context to use.
 * \param section_map Section vector.
 * \param section_mac Elements in the section vector.
 */
void conf_section_set(adv_conf* context, const char** section_map, unsigned section_mac)
{
	unsigned i;

	for(i=0;i<context->section_mac;++i)
		free(context->section_map[i]);
	free(context->section_map);

	context->section_mac = section_mac;
	context->section_map = malloc(context->section_mac * sizeof(char*));
	for(i=0;i<context->section_mac;++i)
		context->section_map[i] = strdup(section_map[i]);
}

#ifdef NDEBUG
#define assert_option(context, tag, type) \
	do { } while (0)
#define assert_option_def(context, tag, type, has_def) \
	do { } while (0)
#else
static void assert_option(adv_conf* context, const char* tag, enum adv_conf_enum type)
{
	struct adv_conf_option_struct* option = option_search_tag(context, tag);

	assert(option);
	assert(option->type == type);
}

static void assert_option_def(adv_conf* context, const char* tag, enum adv_conf_enum type, adv_bool has_def)
{
	struct adv_conf_option_struct* option = option_search_tag(context, tag);

	assert(option);
	assert(option->type == type);

	switch (option->type) {
		case conf_type_bool : assert(option->data.base_bool.has_def || !has_def); break;
		case conf_type_int : assert(option->data.base_int.has_def || !has_def); break;
		case conf_type_float : assert(option->data.base_float.has_def || !has_def); break;
		case conf_type_string : assert(option->data.base_string.has_def || !has_def); break;
		default:
			assert(0);
			return;
	}
}
#endif

/**
 * Get a value with a default.
 * This function get a configuration value which as a default defined.
 * Because of the specification of the default value this function never fails.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \return The value got.
 */
adv_bool conf_bool_get_default(adv_conf* context, const char* tag)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_bool, 1);

	if (!value)
		return option_search_tag(context, tag)->data.base_bool.def;

	return value->data.bool_value;
}

/**
 * Get a value.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_bool_get(adv_conf* context, const char* tag, adv_bool* result)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_bool, 0);

	if (!value)
		return -1;

	*result = value->data.bool_value;
	return 0;
}

/**
 * Get a value in a specified section.
 * The sections specified with of conf_section_set() are temporarely ignored.
 * \param context Configuration context to use.
 * \param section Section to search.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_bool_section_get(adv_conf* context, const char* section, const char* tag, adv_bool* result)
{
	struct adv_conf_value_struct* value = value_searchbest_sectiontag(context, section, tag);

	assert_option_def(context, tag, conf_type_bool, 0);

	if (!value)
		return -1;

	*result = value->data.bool_value;
	return 0;
}

/**
 * Get a value with a default.
 * This function get a configuration value which as a default defined.
 * Because of the specification of the default value this function never fails.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \return The value got.
 */
adv_bool conf_int_get_default(adv_conf* context, const char* tag)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_int, 1);

	if (!value)
		return option_search_tag(context, tag)->data.base_int.def;

	return value->data.int_value;
}

/**
 * Get a value.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_int_get(adv_conf* context, const char* tag, int* result)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_int, 0);

	if (!value)
		return -1;

	*result = value->data.int_value;
	return 0;
}

/**
 * Get a value in a specified section.
 * The sections specified with of conf_section_set() are temporarely ignored.
 * \param context Configuration context to use.
 * \param section Section to search.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_int_section_get(adv_conf* context, const char* section, const char* tag, int* result)
{
	struct adv_conf_value_struct* value = value_searchbest_sectiontag(context, section, tag);

	assert_option_def(context, tag, conf_type_int, 0);

	if (!value)
		return -1;

	*result = value->data.int_value;
	return 0;
}

/**
 * Get a value with a default.
 * This function get a configuration value which as a default defined.
 * Because of the specification of the default value this function never fails.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \return The value got.
 */
double conf_float_get_default(adv_conf* context, const char* tag)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_float, 1);

	if (!value)
		return option_search_tag(context, tag)->data.base_float.def;

	return value->data.float_value;
}

/**
 * Get a value.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_float_get(adv_conf* context, const char* tag, double* result)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_float, 0);

	if (!value)
		return -1;

	*result = value->data.float_value;
	return 0;
}

/**
 * Get a value with a default.
 * This function get a configuration value which as a default defined.
 * Because of the specification of the default value this function never fails.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \return The value got.
 */
const char* conf_string_get_default(adv_conf* context, const char* tag)
{
	struct adv_conf_value_struct* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_string, 1);

	if (!value)
		return option_search_tag(context, tag)->data.base_string.def;

	return value->data.string_value;
}

/**
 * Get a value.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_string_get(adv_conf* context, const char* tag, const char** result)
{
	adv_conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_string, 0);

	if (!value)
		return -1;

	*result = value->data.string_value;
	return 0;
}

/**
 * Get a value in raw internal format.
 * The value is searched in the list of section specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 * \return
 *   - ==0 if not found
 *   - !=0 value pointer
 */
adv_conf_value* conf_value_get(adv_conf* context, const char* tag)
{
	adv_conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	return value;
}

/**
 * Get the bool value from a raw value.
 */
adv_bool conf_value_bool_get(const adv_conf_value* value)
{
	return value->data.bool_value;
}

/**
 * Get the int value from a raw value.
 */
int conf_value_int_get(const adv_conf_value* value)
{
	return value->data.int_value;
}

/**
 * Get the flat value from a raw value.
 */
double conf_value_float_get(const adv_conf_value* value)
{
	return value->data.float_value;
}

/**
 * Get the string value from a raw value.
 */
const char* conf_value_string_get(const adv_conf_value* value)
{
	return value->data.string_value;
}

/**
 * Get the section from a raw value.
 */
const char* conf_value_section_get(const adv_conf_value* value)
{
	return value->section;
}

/**
 * Get the comment from a raw value.
 */
const char* conf_value_comment_get(const adv_conf_value* value)
{
	return value->comment;
}

/**
 * Get a value in a specified section.
 * The sections specified with conf_section_set() are temporarely ignored.
 * \param context Configuration context to use.
 * \param section Section to search.
 * \param tag Tag to search.
 * \param result Where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
adv_error conf_string_section_get(adv_conf* context, const char* section, const char* tag, const char** result)
{
	struct adv_conf_value_struct* value = value_searchbest_sectiontag(context, section, tag);

	assert_option_def(context, tag, conf_type_string, 0);

	if (!value)
		return -1;

	*result = value->data.string_value;
	return 0;
}

/***************************************************************************/
/* Iterator */

/**
 * Initialize an iterator for a multi value option.
 * The value is searched in the list of sections specified by the previous call
 * of conf_section_set(). If no sections are specified calling conf_section_set()
 * no value is found.
 * \param i Iterator to initialize.
 * \param context Configuration context to use.
 * \param tag Tag to search.
 */
void conf_iterator_begin(adv_conf_iterator* i, adv_conf* context, const char* tag)
{
	i->context = context;
	i->value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);
}

/**
 * Like conf_iterator_begin() but only in the specified section.
 * \param i Iterator to initialize.
 * \param context Configuration context to use.
 * \param section Section to search.
 * \param tag Tag to search.
 */
void conf_iterator_section_begin(adv_conf_iterator* i, adv_conf* context, const char* section, const char* tag)
{
	i->context = context;
	i->value = value_searchbest_tag(context, &section, 1, tag);
}

/**
 * Move the iterator to the next position.
 * You can call this function only if conf_iterator_is_end() return false.
 * The next value is searched only on the same file and section of the first value found.
 * \param i Iterator to use.
 */
void conf_iterator_next(adv_conf_iterator* i)
{
	assert(i && i->value);

	i->value = value_searchbest_from(i->context, i->value);
}

/**
 * Move the iterator to the next position and delete the current element.
 * You can call this function only if conf_iterator_is_end() return false.
 * The next value is searched only on the same file and section of the first value found.
 * \param i Iterator to use.
 */
void conf_iterator_remove(adv_conf_iterator* i)
{
	struct adv_conf_value_struct* value_current;

	assert(i && i->value);

	value_current = i->value;

	i->value = value_searchbest_from(i->context, i->value);

	value_remove(i->context, value_current);
}

/**
 * Check if the iterator is at the end.
 * \param i Iterator to use.
 * \return
 *  - == 0 The iterator is not at the end.
 *  - != 0 The iterator is at the end.
 */
adv_bool conf_iterator_is_end(const adv_conf_iterator* i)
{
	assert(i);

	return i->value == 0;
}

/**
 * Get the value at the iterator position.
 * You can call this function only if conf_iterator_is_end() return false.
 * \param i Iterator to use.
 * \return Pointer at the value.
 */
const char* conf_iterator_string_get(const adv_conf_iterator* i)
{
	assert(i && i->value && i->value->option->type == conf_type_string);

	return i->value->data.string_value;
}

/***************************************************************************/
/* Set */

/**
 * Set a value of any format.
 * This function can set a value of any type, eventually the value is converted from
 * the string passed.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_set(adv_conf* context, const char* section, const char* tag, const char* result)
{
	struct adv_conf_input_struct* input;
	struct adv_conf_option_struct* option;

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	return value_set_dup(context, input, option, section, "", result, result, 0, 0);
}

/**
 * Set a value.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_bool_set(adv_conf* context, const char* section, const char* tag, adv_bool result)
{
	const char* buffer;

	assert_option(context, tag, conf_type_bool);

	buffer = result ? "yes" : "no";

	return conf_set(context, section, tag, buffer);
}

/**
 * Set a value.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_int_set(adv_conf* context, const char* section, const char* tag, int result)
{
	const char* result_string;
	char result_buffer[CONF_NUM_BUFFER_MAX];
	struct adv_conf_option_struct* option;

	assert_option(context, tag, conf_type_int);

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	if (!option->data.base_int.has_enum) {
		snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
		result_string = result_buffer;
	} else {
		unsigned i;
		result_string = 0;
		for(i=0;i<option->data.base_int.enum_mac;++i) {
			if (option->data.base_int.enum_map[i].map == result) {
				result_string = option->data.base_int.enum_map[i].value;
				break;
			}
		}
		if (i == option->data.base_int.enum_mac)
			return -1;
	}

	return conf_set(context, section, tag, result_string);
}

/**
 * Set a value.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_float_set(adv_conf* context, const char* section, const char* tag, double result)
{
	char buffer[CONF_NUM_BUFFER_MAX];

	assert_option(context, tag, conf_type_float);

	snprintf(buffer, sizeof(buffer), "%g", (double)result);

	return conf_set(context, section, tag, buffer);
}

/**
 * Set a value.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_string_set(adv_conf* context, const char* section, const char* tag, const char* result)
{
	const char* result_string;

	assert_option(context, tag, conf_type_string);

	result_string = result;

	return conf_set(context, section, tag, result_string);
}

/**
 * Remove an option.
 * In case of multi options, all the options are cleared.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \return
 *   - ==0 if a value is found and removed
 *   - !=0 if not found
 */
adv_error conf_remove(adv_conf* context, const char* section, const char* tag)
{
	struct adv_conf_value_struct* value;
	struct adv_conf_input_struct* input;

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	do {
		value = value_search_inputsectiontag(context, input, section, tag);
		if (value) {
			value_remove(context, value);
		}
	} while (value);

	return 0;
}

/***************************************************************************/
/* Special set/remove */

/**
 * Get the default value.
 * \param option Option to scan.
 * \param buffer Destination buffer used for int/float values.
 * \param size Size of the destination buffer.
 * \return 0 if the option has not a default.
 */
static const char* option_default_get(struct adv_conf_option_struct* option, char* buffer, unsigned size)
{
	switch (option->type) {
		case conf_type_bool :
			if (!option->data.base_bool.has_def)
				return 0;
			return option->data.base_bool.def ? "yes" : "no";
		case conf_type_int :
			if (!option->data.base_int.has_def)
				return 0;
			if (!option->data.base_int.has_enum) {
				snprintf(buffer, size, "%d", (int)option->data.base_int.def);
				return buffer;
			} else {
				unsigned i;
				for(i=0;i<option->data.base_int.enum_mac;++i) {
					if (option->data.base_int.enum_map[i].map == option->data.base_int.def) {
						return option->data.base_int.enum_map[i].value;
					}
				}
				return 0;
			}
		case conf_type_float :
			if (!option->data.base_float.has_def)
				return 0;
			snprintf(buffer, size, "%g", (double)option->data.base_float.def);
			return buffer;
		case conf_type_string :
			if (!option->data.base_string.has_def)
				return 0;
			return option->data.base_string.def;
		default:
			assert(0);
			return 0;
	}
}

static const char* value_get(struct adv_conf_value_struct* value, char* buffer, unsigned size)
{
	switch (value->option->type) {
		case conf_type_bool :
			return value->data.bool_value ? "yes" : "no";
		case conf_type_int :
			if (!value->option->data.base_int.has_enum) {
				snprintf(buffer, size, "%d", (int)value->data.int_value);
				return buffer;
			} else {
				unsigned i;
				for(i=0;i<value->option->data.base_int.enum_mac;++i) {
					if (value->option->data.base_int.enum_map[i].map == value->data.int_value) {
						return value->option->data.base_int.enum_map[i].value;
					}
				}
				return 0;
			}
		case conf_type_float :
			snprintf(buffer, size, "%g", (double)value->data.float_value);
			return buffer;
		case conf_type_string :
			return value->data.string_value;
		default:
			assert(0);
			return 0;
	}
}
/**
 * Set the value of a option only if it change something.
 * The value is set only if the current value is different. Otherwise the
 * option is removed.
 * The current value is checked reading all the section specified
 * with conf_section_set() which have the precedence on the
 * save section specified.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 * \param result Value to save.
 */
adv_error conf_set_if_different(adv_conf* context, const char* section, const char* tag, const char* result)
{
	adv_conf_value* value;
	struct adv_conf_option_struct* option;
	const char* result_current;
	char result_buffer[CONF_NUM_BUFFER_MAX];
	unsigned i;

	/* limits the search only after the specified section */
	for(i=0;i<context->section_mac;++i) {
		if (strcmp(section, context->section_map[i]) == 0) {
			++i;
			break;
		}
	}

	value = value_searchbest_tag(context, (const char**)(context->section_map + i), context->section_mac - i, tag);

	result_current = 0;

	if (!value) {
		/* get the default value if any */
		option = option_search_tag(context, tag);
		if (option) {
			result_current = option_default_get(option, result_buffer, sizeof(result_buffer));
		}
	} else {
		/* get the value */
		result_current = value_get(value, result_buffer, sizeof(result_buffer));
	}

	if (!result_current || strcmp(result_current, result) != 0)
		return conf_set(context, section, tag, result);
	else
		return conf_remove(context, section, tag);
}

/**
 * Like conf_set_if_different() but operates on bool values.
 */
adv_error conf_bool_set_if_different(adv_conf* context, const char* section, const char* tag, adv_bool result)
{
	const char* buffer;

	assert_option(context, tag, conf_type_bool);

	buffer = result ? "yes" : "no";

	return conf_set_if_different(context, section, tag, buffer);
}

/**
 * Like conf_set_if_different() but operates on int values.
 */
adv_error conf_int_set_if_different(adv_conf* context, const char* section, const char* tag, int result)
{
	const char* result_string;
	char result_buffer[CONF_NUM_BUFFER_MAX];
	struct adv_conf_option_struct* option;

	assert_option(context, tag, conf_type_int);

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	if (!option->data.base_int.has_enum) {
		snprintf(result_buffer, sizeof(result_buffer), "%d", (int)result);
		result_string = result_buffer;
	} else {
		unsigned i;
		result_string = 0;
		for(i=0;i<option->data.base_int.enum_mac;++i) {
			if (option->data.base_int.enum_map[i].map == result) {
				result_string = option->data.base_int.enum_map[i].value;
				break;
			}
		}
		if (i == option->data.base_int.enum_mac)
			return -1;
	}

	return conf_set_if_different(context, section, tag, result_string);
}

/**
 * Like conf_set_if_different() but operates on float values.
 */
adv_error conf_float_set_if_different(adv_conf* context, const char* section, const char* tag, double result)
{
	char buffer[CONF_NUM_BUFFER_MAX];

	assert_option(context, tag, conf_type_float);

	snprintf(buffer, sizeof(buffer), "%g", result);

	return conf_set_if_different(context, section, tag, buffer);
}

/**
 * Like conf_set_if_different() but operates on string values.
 */
adv_error conf_string_set_if_different(adv_conf* context, const char* section, const char* tag, const char* value)
{
	assert_option(context, tag, conf_type_string);

	return conf_set_if_different(context, section, tag, value);
}

/**
 * Set the default value of a option.
 * The option is added in the last writable file.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 */
adv_error conf_setdefault(adv_conf* context, const char* section, const char* tag)
{
	struct adv_conf_input_struct* input;
	struct adv_conf_option_struct* option;
	const char* result_string;
	char result_buffer[CONF_NUM_BUFFER_MAX];

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	result_string = option_default_get(option, result_buffer, sizeof(result_buffer));
	if (!result_string)
		return -1;

	return value_set_dup(context, input, option, section, "", result_string, result_string, 0, 0);
}

/**
 * Set the default value of all the options if they are not defined.
 * If the option is missing in all the input files is added at the last writable.
 * \param context Configuration context to use.
 * \param section Section of the options.
 */
void conf_setdefault_all_if_missing(adv_conf* context, const char* section)
{
	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			struct adv_conf_value_struct* value = value_searchbest_sectiontag(context, section, option->tag);
			if (!value)
				conf_setdefault(context, section, option->tag);
			option = option->next;
		} while (option != context->option_list);
	}
}

/**
 * Remove all the options if is they are set with the default values.
 * \param context Configuration context to use.
 * \param section Section of the options.
 */
void conf_remove_all_if_default(adv_conf* context, const char* section)
{
	if (context->option_list) {
		struct adv_conf_option_struct* option = context->option_list;
		do {
			struct adv_conf_value_struct* value = value_searchbest_sectiontag(context, section, option->tag);
			if (value) {
				char result_buffer[CONF_NUM_BUFFER_MAX];
				char default_buffer[CONF_NUM_BUFFER_MAX];
				const char* result_string = value_get(value, result_buffer, sizeof(result_buffer));
				const char* default_string = option_default_get(option, default_buffer, sizeof(default_buffer));
				assert(result_string);
				if (default_string && strcmp(result_string, default_string)==0) {
					value_remove(context, value);
				}
			}
			option = option->next;
		} while (option != context->option_list);
	}
}

/**
 * Remove the specified tag if it has a value different from the default.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 */
adv_error conf_remove_if_notdefault(adv_conf* context, const char* section, const char* tag)
{
	struct adv_conf_value_struct* value;
	struct adv_conf_input_struct* input;

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	value = value_search_inputsectiontag(context, input, section, tag);
	if (value) {
		char result_buffer[CONF_NUM_BUFFER_MAX];
		char default_buffer[CONF_NUM_BUFFER_MAX];
		const char* result_string = value_get(value, result_buffer, sizeof(result_buffer));
		const char* default_string = option_default_get(value->option, default_buffer, sizeof(default_buffer));
		assert(result_string);

		if (default_string && strcmp(result_string, default_string) != 0) {
			value_remove(context, value);
		}
	}

	return 0;
}

/**
 * Remove the specified tag if it has a value equal at the default.
 * \param context Configuration context to use.
 * \param section Section of the option.
 * \param tag Tag of the option.
 */
adv_error conf_remove_if_default(adv_conf* context, const char* section, const char* tag)
{
	struct adv_conf_value_struct* value;
	struct adv_conf_input_struct* input;

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	value = value_search_inputsectiontag(context, input, section, tag);
	if (value) {
		char result_buffer[CONF_NUM_BUFFER_MAX];
		char default_buffer[CONF_NUM_BUFFER_MAX];
		const char* result_string = value_get(value, result_buffer, sizeof(result_buffer));
		const char* default_string = option_default_get(value->option, default_buffer, sizeof(default_buffer));
		assert(result_string);

		if (default_string && strcmp(result_string, default_string) == 0) {
			value_remove(context, value);
		}
	}

	return 0;
}

/***************************************************************************/
/* Autoregistration */

/**
 * Autoregistration version of conf_string_set().
 * The tag is automatically registered as a string (without default) if
 * it isn't already registered.
 */
adv_error conf_autoreg_string_set(adv_conf* context, const char* section, const char* tag, const char* value)
{
	if (!conf_is_registered(context, tag)) {
		conf_string_register(context, tag);
	}
	return conf_string_set(context, section, tag, value);
}

/**
 * Autoregistration version of conf_string_get().
 * The tag is automatically registered as a string (without default) if
 * it isn't already registered.
 */
adv_error conf_autoreg_string_get(adv_conf* context, const char* tag, const char** result)
{
	if (!conf_is_registered(context, tag)) {
		return -1;
	}
	return conf_string_get(context, tag, result);
}

/**
 * Autoregistration version of conf_remove().
 * If the tag isn't registred no action is taken.
 */
adv_error conf_autoreg_remove(adv_conf* context, const char* section, const char* tag)
{
	if (conf_is_registered(context, tag)) {
		return conf_remove(context, section, tag);
	} else {
		/* if it isn't registered it's surely missing */
		return 0;
	}
}

