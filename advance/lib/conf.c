/*
 * This file is part of the AdvanceMAME project.
 *
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

#include "conf.h"
#include "incstr.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

/** Buffer used for int/float values */
#define CONF_NUM_BUFFER_MAX 48

/***************************************************************************/
/* Local */

static conf_bool partial_match(const char* s, const char* partial) {
	const char* p = s;
	while (1) {
		if (memcmp(p, partial, strlen(partial))==0)
			return 1;

		p = strchr(p,'_');
		if (!p)
			return 0;

		++p; /* skip the '_' */
	}

	return 0;
}

static conf_bool glob_match(const char* s, const char* glob) {
	while (*s && *glob) {
		if (*glob=='*') {
			if (glob_match(s,glob+1))
				return 1;
			++s;
		} else if (*glob != *s) {
			return 0;
		} else {
			++glob;
			++s;
		}
	}
	while (*glob == '*')
		++glob;
	return !*s && !*glob;
}

static char* glob_subst(const char* format, char* own_s) {
	if (strcmp(format,"%s")==0) {
		return own_s;
	} else {
		char* token = strstr(format,"%s");
		/* assume no more than one %s token */
		if (token) {
			char* n = malloc(strlen(format) + strlen(own_s) - 1);
			sprintf(n,format,own_s);
			free(own_s);
			return n;
		} else {
			free(own_s);
			return strdup(format);
		}
	}
}

static void option_insert(struct conf_context* context, struct conf_option* option) {
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

static struct conf_option* option_alloc(void) {
	struct conf_option* option;

	option = (struct conf_option*)malloc(sizeof(struct conf_option));

	option->tag = 0;
	option->type = -1;

	return option;
}

static void option_free(struct conf_option* option) {
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

static struct conf_option* option_search_tag(struct conf_context* context, const char* tag) {
	if (context->option_list) {
		struct conf_option* option = context->option_list;
		do {
			if (strcmp(option->tag,tag)==0)
				return option;
			option = option->next;
		} while (option != context->option_list);
	}
	return 0;
}

/**
 * Search a option for partial match.
 * The search complete with success only if an unique option is found.
 */
static struct conf_option* option_search_tag_partial(struct conf_context* context, const char* tag) {
	struct conf_option* found = 0;

	if (context->option_list) {
		struct conf_option* option = context->option_list;
		do {
			if (partial_match(option->tag,tag)) {
				if (found) {
					/* printf("conf: multiple match %s and %s for %s\n",found->tag,option->tag,tag); */
					return 0; /* multiple match */
				}
				found = option;
			}
			option = option->next;
		} while (option != context->option_list);
	}


	return found;
}

static struct conf_input* input_alloc(void) {
	struct conf_input* input = (struct conf_input*)malloc(sizeof(struct conf_input));
	input->file_in = 0;
	input->file_out = 0;
	input->conv_map = 0;
	input->conv_mac = 0;
	return input;
}

static void input_free(struct conf_input* input) {
	unsigned i;
	free(input->file_in);
	free(input->file_out);
	for(i=0;i<input->conv_mac;++i) {
		free(input->conv_map[i].section_glob);
		free(input->conv_map[i].section_result);
		free(input->conv_map[i].tag_glob);
		free(input->conv_map[i].tag_result);
		free(input->conv_map[i].value_glob);
		free(input->conv_map[i].value_result);
	}
	free(input->conv_map);
	free(input);
}

static void input_insert(struct conf_context* context, struct conf_input* input) {
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

static struct conf_input* input_searchbest_writable(struct conf_context* context) {
	if (context->input_list) {
		struct conf_input* best_input = 0;
		struct conf_input* input = context->input_list;

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

static struct conf_value* value_alloc(struct conf_option* option) {
	struct conf_value* value = (struct conf_value*)malloc(sizeof(struct conf_value));

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

static void value_free(struct conf_value* value) {
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

static void value_insert(struct conf_context* context, struct conf_value* value) {
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

static int value_cmp(struct conf_value* a, struct conf_value* b) {
	int r;

	r = strcmp(a->section,b->section);
	if (r)
		return r;

	r = strcmp(a->option->tag,b->option->tag);
	return r;
}

static void value_insert_sort(struct conf_context* context, struct conf_value* value) {
	if (context->value_list) {
		struct conf_value* i = context->value_list;
		if (value_cmp(i,value) <= 0) {
			do {
				i = i->next;
			} while (i != context->value_list && value_cmp(i,value) <= 0);
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

static void value_remove(struct conf_context* context, struct conf_value* value) {
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

static struct conf_value* value_searchbest_sectiontag(struct conf_context* context, const char* section, const char* tag) {
	if (context->value_list) {
		struct conf_value* best_value = 0;
		struct conf_value* value = context->value_list;

		do {
			if (strcmp(value->section,section)==0
				&& strcmp(value->option->tag,tag)==0) {
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

static struct conf_value* value_search_inputsectiontag(struct conf_context* context, struct conf_input* input, const char* section, const char* tag) {
	if (context->value_list) {
		struct conf_value* value = context->value_list;

		do {
			if (value->input == input
				&& strcmp(value->section,section)==0
				&& strcmp(value->option->tag,tag)==0) {
				return value;
			}
			value = value->next;
		} while (value != context->value_list);
	}

	return 0;
}

static struct conf_value* value_searchbest_tag(struct conf_context* context, const char** section_map, unsigned section_mac, const char* tag) {
	unsigned i;

	for(i=0;i<section_mac;++i) {
		struct conf_value* value;
		value = value_searchbest_sectiontag(context, section_map[i], tag);
		if (value)
			return value;
	}

	return 0;
}

static struct conf_value* value_searchbest_from(struct conf_context* context, struct conf_value* like_value) {
	struct conf_value* value = like_value->next;

	while (value != context->value_list) {

		if (value->option == like_value->option
			&& value->input == like_value->input
			&& strcmp(value->section,like_value->section)==0) {
			return value;
		}

		value = value->next;
	}

	return 0;
}

/***************************************************************************/
/* Init/Done */

struct conf_context* conf_init(void) {
	struct conf_context* context = malloc(sizeof(struct conf_context));

	context->option_list = 0;
	context->input_list = 0;
	context->value_list = 0;

	context->section_mac = 0;
	context->section_map = 0;

	context->is_modified = 0;

	return context;
}

void conf_done(struct conf_context* context) {
	unsigned i;

	if (context->value_list) {
		struct conf_value* value = context->value_list;
		do {
			struct conf_value* value_next = value->next;
			value_free(value);
			value = value_next;
		} while (value != context->value_list);
	}

	if (context->option_list) {
		struct conf_option* option = context->option_list;
		do {
			struct conf_option* option_next = option->next;
			option_free(option);
			option = option_next;
		} while (option != context->option_list);
	}

	if (context->input_list) {
		struct conf_input* input = context->input_list;
		do {
			struct conf_input* input_next = input->next;
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

conf_bool conf_is_registered(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_search_tag(context, tag);
	return option != 0;
}

void conf_bool_register(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_bool;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_bool.has_def = 0;

	option_insert(context, option);
}

void conf_bool_register_default(struct conf_context* context, const char* tag, conf_bool def) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_bool;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_bool.has_def = 1;
	option->data.base_bool.def = def;

	option_insert(context, option);
}

void conf_int_register(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_int;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_int.has_def = 0;
	option->data.base_int.has_limit = 0;
	option->data.base_int.has_enum = 0;

	option_insert(context, option);
}

void conf_int_register_default(struct conf_context* context, const char* tag, int def) {
	struct conf_option* option = option_alloc();

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

void conf_int_register_limit(struct conf_context* context, const char* tag, int limit_low, int limit_high) {
	struct conf_option* option = option_alloc();

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

void conf_int_register_limit_default(struct conf_context* context, const char* tag, int limit_low, int limit_high, int def) {
	struct conf_option* option = option_alloc();

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

void conf_float_register(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_float;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_float.has_def = 0;
	option->data.base_float.has_limit = 0;

	option_insert(context, option);
}

void conf_float_register_default(struct conf_context* context, const char* tag, double def) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_float;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_float.has_def = 1;
	option->data.base_float.def = def;
	option->data.base_float.has_limit = 0;

	option_insert(context, option);
}

void conf_float_register_limit_default(struct conf_context* context, const char* tag, double limit_low, double limit_high, double def) {
	struct conf_option* option = option_alloc();

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

void conf_string_register(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 0;
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

void conf_string_register_multi(struct conf_context* context, const char* tag) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 1;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 0;
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

void conf_string_register_default(struct conf_context* context, const char* tag, const char* def) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

	option->type = conf_type_string;
	option->is_multi = 0;
	option->tag = strdup(tag);
	option->data.base_string.has_def = 1;
	option->data.base_string.def = strdup(def);
	option->data.base_string.has_enum = 0;

	option_insert(context, option);
}

void conf_string_register_enum_default(struct conf_context* context, const char* tag, struct conf_enum_string* enum_map, unsigned enum_mac, const char* def) {
	struct conf_option* option = option_alloc();

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

void conf_int_register_enum_default(struct conf_context* context, const char* tag, struct conf_enum_int* enum_map, unsigned enum_mac, int def) {
	struct conf_option* option = option_alloc();

	assert(option_search_tag(context, tag) == 0);

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

static conf_error import_bool(struct conf_value* value, char* own_value, conf_error_callback* error, void* error_context) {
	if (strcmp(own_value,"yes")==0) {
		value->data.bool_value = 1;
		free(value->format);
		value->format = own_value;
		return 0;
	} else if (strcmp(own_value,"no")==0) {
		value->data.bool_value = 0;
		free(value->format);
		value->format = own_value;
		return 0;
	} else {
		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, "Valid arguments are 'yes' and 'no'", "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_value);
		return -1;
	}
}

static conf_error import_int(struct conf_value* value, char* own_value, conf_error_callback* error, void* error_context) {
	if (value->option->data.base_int.has_enum) {
		unsigned i;
		struct inc_str valid;
		char* own_valid;
		char* endp;
		int r;

		/* try comparing the string */
		for(i=0;i<value->option->data.base_int.enum_mac;++i) {
			if (strcmp(value->option->data.base_int.enum_map[i].value,own_value)==0) {
				value->data.int_value = value->option->data.base_int.enum_map[i].map;
				free(value->format);
				value->format = own_value;
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
					free(value->format);
					value->format = own_value;
					return 0;
				}
			}
		}

		inc_str_init(&valid);
		inc_str_cat(&valid,"Valid values are ");
		for(i=0;i<value->option->data.base_int.enum_mac;++i) {
			if (i)
				inc_str_cat(&valid,", ");
			inc_str_cat(&valid,"'");
			inc_str_cat(&valid,value->option->data.base_int.enum_map[i].value);
			inc_str_cat(&valid,"'");
		}
		own_valid = inc_str_alloc(&valid);
		inc_str_done(&valid);

		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, own_valid, "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_valid);
		free(own_value);
		return -1;
	} else {
		char* endp;
		int r = strtol(own_value, &endp, 10);

		if (*endp != 0
			|| *own_value == 0
			|| (value->option->data.base_int.has_limit
				&& (r < value->option->data.base_int.limit_low || r > value->option->data.base_int.limit_high))
			) {
				char valid[128];
				sprintf(valid,"Valid arguments are int from %d to %d", value->option->data.base_int.limit_low, value->option->data.base_int.limit_high);
				if (error)
					error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, valid, "Invalid argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
				free(own_value);
				return -1;
		}

		value->data.int_value = r;
		free(value->format);
		value->format = own_value;

		return 0;
	}
}

static conf_error import_float(struct conf_value* value, char* own_value, conf_error_callback* error, void* error_context) {
	char* endp;

	double r = strtod(own_value, &endp);

	if (*endp != 0
		|| *own_value == 0
		|| (value->option->data.base_float.has_limit
			&& (r < value->option->data.base_float.limit_low || r > value->option->data.base_float.limit_high)
		)) {
		char valid[128];
		sprintf(valid,"Valid arguments are float from %g to %g", value->option->data.base_float.limit_low, value->option->data.base_float.limit_high);
		if (error)
			error(error_context, conf_error_invalid, value->input->file_in, value->option->tag, valid, "Out of range argument '%s' for option '%s' in file '%s'", own_value, value->option->tag, value->input->file_in);
		free(own_value);
		return -1;
	}

	value->data.float_value = r;
	free(value->format);
	value->format = own_value;

	return 0;
}

static conf_error import_string(struct conf_value* value, char* own_value, conf_error_callback* error, void* error_context) {
	unsigned i;
	char* own_valid;
	struct inc_str valid;

	if (!value->option->data.base_string.has_enum) {
		free(value->data.string_value);
		value->data.string_value = own_value;
		free(value->format);
		value->format = strdup(own_value);
		return 0;
	}

	/* try comparing the string */
	for(i=0;i<value->option->data.base_string.enum_mac;++i) {
		if (strcmp(value->option->data.base_string.enum_map[i].value,own_value)==0) {
			free(value->data.string_value);
			value->data.string_value = own_value;
			free(value->format);
			value->format = strdup(own_value);
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
	return -1;
}

/**
 * Import a value. The value item is NOT initialized before the call.
 */
static conf_error value_import(struct conf_value* value, char* own_value, conf_error_callback* error, void* error_context) {
	switch (value->option->type) {
		case conf_type_bool : return import_bool(value, own_value, error, error_context);
		case conf_type_int : return import_int(value, own_value, error, error_context);
		case conf_type_float : return import_float(value, own_value, error, error_context);
		case conf_type_string : return import_string(value, own_value, error, error_context);
		default:
			assert(0);
			return -1;
	}
}

/***************************************************************************/
/* Load */

static conf_error value_make_raw(struct conf_context* context, struct conf_input* input, struct conf_option* option, char* own_section, char* own_comment, char* own_value, char* own_format, conf_error_callback* error, void* error_context) {
	struct conf_value* value;

	value = value_alloc(option);

	value->input = input;
	value->section = own_section;
	value->comment = own_comment;
	value->format = own_format;

	if (value_import(value, own_value, error, error_context) != 0) {
		value_free(value);
		return -1;
	}

	value_insert(context, value);

	return 0;
}

static conf_error value_make_own(struct conf_context* context, struct conf_input* input, struct conf_option* option, char* own_section, char* own_comment, char* own_value, char* own_format, conf_error_callback* error, void* error_context) {
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

static conf_error value_make_dup(struct conf_context* context, struct conf_input* input, struct conf_option* option, const char* section, const char* comment, const char* value, const char* format, conf_error_callback* error, void* error_context) {
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

/** Insert if it doesn't exist whithout error checks. Return the existing value if any. */
static conf_error value_set_dup(struct conf_context* context, struct conf_input* input, struct conf_option* option, const char* section, const char* comment, const char* value, const char* format, conf_error_callback* error, void* error_context) {
	/* check for not multi options */
	if (!option->is_multi) {
		struct conf_value* value_exist = value_search_inputsectiontag(context, input, section, option->tag);
		if (value_exist) {
			context->is_modified = 1;
			return value_import(value_exist, strdup(value), error, error_context);
		}
	}

	return value_make_dup(context, input, option, section, comment, value, format, error, error_context);
}

static conf_error input_value_insert(struct conf_context* context, struct conf_input* input, char** global_section, char* own_comment, char* own_sectiontag, char* own_value, char* own_format, conf_error_callback* error, void* error_context) {
	struct conf_option* option;
	char* slash;
	char* own_section;
	char* own_tag;
	conf_bool autoreg;

	slash = strrchr(own_sectiontag,'/');
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

	autoreg = 0;
	if (input->conv_mac) {
		unsigned conv;
		for(conv=0;conv<input->conv_mac;++conv)
			if (glob_match(own_section,input->conv_map[conv].section_glob)
				&& glob_match(own_tag,input->conv_map[conv].tag_glob)
				&& glob_match(own_value,input->conv_map[conv].value_glob)
			)
				break;
		if (conv < input->conv_mac) {
			autoreg = input->conv_map[conv].autoreg;
			own_section = glob_subst(input->conv_map[conv].section_result,own_section);
			own_tag = glob_subst(input->conv_map[conv].tag_result,own_tag);
			own_value = glob_subst(input->conv_map[conv].value_result,own_value);
			free(own_format);
			own_format = strdup(own_value);
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

		if (autoreg) {
			/* auto register the option */
			option = option_alloc();
			option->type = conf_type_string;
			option->is_multi = 1;
			option->tag = own_tag;
			option->data.base_string.has_def = 0;
			option->data.base_string.has_enum = 0;
			option_insert(context, option);

			if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
				goto err;

			return 0;
		}

		if (input->ignore_unknow_flag) {
			/* ignore */
			free(own_section);
			free(own_tag);
			free(own_comment);
			free(own_value);
			free(own_format);
			return 0;
		}

		error(error_context, conf_error_unknow, input->file_in, own_tag, 0, "Unknow option '%s' in file '%s'", own_tag, input->file_in);
		goto err_free;
	} else {
		free(own_tag);

		if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
			goto err;

		return 0;
	}

	assert(0);

err_free:
	free(own_section);
	free(own_tag);
	free(own_comment);
	free(own_value);
	free(own_format);

err:
	return -1;
}

static conf_error input_section_insert(struct conf_context* context, struct conf_input* input, char** global_section, char* own_comment, char* own_tag, char* own_value, char* own_format) {
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

static conf_error input_value_load(struct conf_context* context, struct conf_input* input, FILE* f, char** global_section, conf_bool multi_line, conf_error_callback* error, void* error_context) {
	int c;

	enum state_type {
		state_comment,
		state_comment_line,
		state_tag,
		state_before_equal,
		state_after_equal,
		state_value,
		state_value_line,
		state_eof
	};

	enum state_type state;

	struct inc_str icomment;
	struct inc_str itag;
	struct inc_str ivalue;
	struct inc_str iformat;

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
					state = 1;
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
					copy |= copy_in_format;
				} else {
					ungetc(c,f);
					state = state_value;
					c = '\\';
					copy |= copy_in_value;
				}
				break;
			case state_eof:
				break;
		}

		if ((copy & copy_in_comment) != 0) {
			if (inc_str_catc(&icomment,c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_tag) != 0) {
			if (inc_str_catc(&itag,c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_value) != 0) {
			if (inc_str_catc(&ivalue,c) != 0)
				goto err_done;
		}

		if ((copy & copy_in_format) != 0) {
			if (inc_str_catc(&iformat,c) != 0)
				goto err_done;
		}

	} while (state != state_eof);

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

static conf_error input_load(struct conf_context* context, struct conf_input* input, conf_bool multi_line, conf_error_callback* error, void* error_context) {
	FILE* f;

	char* global_section;

	f = fopen(input->file_in,"rt");
	if (!f)
		goto err;

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

conf_error conf_input_file_load_adv(struct conf_context* context, int priority, const char* file_in, const char* file_out, conf_bool ignore_unknow_flag, conf_bool multi_line, const struct conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context) {
	struct conf_input* input;

	conf_bool is_file_in_exist = file_in != 0 && access(file_in,F_OK) == 0;

	/* ignore if don't exist and is not writable */
	if (!is_file_in_exist && !file_out)
		return 0;

	input = input_alloc();

	input->priority = priority;
	input->ignore_unknow_flag = ignore_unknow_flag;

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
		input->conv_map = malloc(conv_mac * sizeof(struct conf_conv));
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

conf_error conf_input_file_load(struct conf_context* context, int priority, const char* file, conf_error_callback* error, void* error_context) {
	return conf_input_file_load_adv(context, priority, file, file, 0, 1, 0, 0, error, error_context);
}

conf_error conf_input_args_load(struct conf_context* context, int priority, const char* section, int* argc, char* argv[], conf_error_callback* error, void* error_context) {
	int i;
	struct conf_input* input = input_alloc();

	input->priority = priority;
	input->ignore_unknow_flag = 1;
	input->file_in = strdup("commandline");
	input->file_out = 0;

	input_insert(context,input);

	i = 0;
	while (i<*argc) {
		conf_bool noformat;
		struct conf_option* option;
		const char* tag = argv[i];
		unsigned used;

		if (tag[0]!='-') {
			++i;
			continue;
		}

		++tag;

		noformat = 0;

		/* exact search */
		option = option_search_tag(context, tag);
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

		/* partial search */
		if (!option) {
			option = option_search_tag_partial(context, tag);
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
			own_format = strdup(own_value);

			if (value_make_own(context, input, option, own_section, own_comment, own_value, own_format, error, error_context) != 0)
				return -1;

			used = 1;
		} else {
			char* own_section = strdup(section);
			char* own_comment = strdup("");
			char* own_value = strdup(argv[i+1]);
			char* own_format = strdup(own_value);

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

static void value_save(struct conf_value* value, const char** global_section, FILE* f) {
	if (strcmp(*global_section,value->section) != 0) {
#if 0
		/* print an extra newline before a section change */
		if (*value->comment == 0)
			fprintf(f,"\n");
#endif
		*global_section = value->section;
	}
	if (*value->section)
		fprintf(f,"%s%s/%s %s\n",value->comment,value->section,value->option->tag,value->format);
	else
		fprintf(f,"%s%s %s\n",value->comment,value->option->tag,value->format);
}

static conf_error input_save(struct conf_context* context, struct conf_input* input) {
	FILE* f;
	const char* global_section;

	/* if not writable skip */
	if (!input->file_out)
		return 0;

	global_section = "";

	f = fopen(input->file_out,"wt");
	if (!f)
		goto err;

	if (context->value_list) {
		struct conf_value* value = context->value_list;
		do {
			if (value->input == input) {
				value_save(value,&global_section,f);
			}
			value = value->next;
		} while (value != context->value_list);
	}

	fclose(f);
	return 0;

err:
	return -1;
}

conf_error conf_save(struct conf_context* context, conf_bool force) {

	/* only if necessary */
	if (!force && !context->is_modified)
		return 0;

	if (context->input_list) {
		struct conf_input* input = context->input_list;
		do {
			if (input_save(context, input)!=0)
				return -1;
			input = input->next;
		} while (input != context->input_list);
	}

	context->is_modified = 0;

	return 0;
}

void conf_uncomment(struct conf_context* context) {
	struct conf_value* value = context->value_list;

	if (value) {
		do {
			free(value->comment);
			value->comment = strdup("");

			value = value->next;
		} while (value != context->value_list);
	}
}

void conf_sort(struct conf_context* context) {
	struct conf_value* value_list = context->value_list;
	struct conf_value* value = value_list;

	context->value_list = 0;

	if (value_list) {
		do {
			struct conf_value* value_next = value->next;
			value_insert_sort(context,value);
			value = value_next;
		} while (value != value_list);
	}

	context->is_modified = 0;
}

/***************************************************************************/
/* Get */

void conf_section_set(struct conf_context* context, const char** section_map, unsigned section_mac) {
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
#define assert_option(context,tag,type) \
	do { } while (0)
#define assert_option_def(context,tag,type,has_def) \
	do { } while (0)
#else
static void assert_option(struct conf_context* context, const char* tag, enum conf_type type) {
	struct conf_option* option = option_search_tag(context, tag);

	assert(option);
	assert(option->type == type);
}

static void assert_option_def(struct conf_context* context, const char* tag, enum conf_type type, conf_bool has_def) {
	struct conf_option* option = option_search_tag(context, tag);

	assert(option);
	assert(option->type == type);

	switch (option->type) {
		case conf_type_bool : assert(option->data.base_bool.has_def == has_def); break;
		case conf_type_int : assert(option->data.base_int.has_def == has_def); break;
		case conf_type_float : assert(option->data.base_float.has_def == has_def); break;
		case conf_type_string : assert(option->data.base_string.has_def == has_def); break;
		default:
			assert(0);
			return;
	}
}
#endif

conf_bool conf_bool_get_default(struct conf_context* context, const char* tag) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_bool,1);

	if (!value)
		return option_search_tag(context, tag)->data.base_bool.def;

	return value->data.bool_value;
}

conf_error conf_bool_get(struct conf_context* context, const char* tag, conf_bool* result) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_bool,0);

	if (!value)
		return -1;

	*result = value->data.bool_value;
	return 0;
}

conf_bool conf_int_get_default(struct conf_context* context, const char* tag) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_int,1);

	if (!value)
		return option_search_tag(context, tag)->data.base_int.def;

	return value->data.int_value;
}

conf_error conf_int_get(struct conf_context* context, const char* tag, int* result) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_int,0);

	if (!value)
		return -1;

	*result = value->data.int_value;
	return 0;
}

double conf_float_get_default(struct conf_context* context, const char* tag) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_float,1);

	if (!value)
		return option_search_tag(context, tag)->data.base_float.def;

	return value->data.float_value;
}

conf_error conf_float_get(struct conf_context* context, const char* tag, double* result) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_float,0);

	if (!value)
		return -1;

	*result = value->data.float_value;
	return 0;
}

const char* conf_string_get_default(struct conf_context* context, const char* tag) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag,conf_type_string,1);

	if (!value)
		return option_search_tag(context, tag)->data.base_string.def;

	return value->data.string_value;
}

conf_error conf_string_get(struct conf_context* context, const char* tag, const char** result) {
	struct conf_value* value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);

	assert_option_def(context, tag, conf_type_string, 0);

	if (!value)
		return -1;

	*result = value->data.string_value;
	return 0;
}

conf_error conf_string_section_get(struct conf_context* context, const char* section, const char* tag, const char** result) {
	struct conf_value* value = value_searchbest_sectiontag(context, section, tag);

	assert_option_def(context, tag, conf_type_string, 0);

	if (!value)
		return -1;

	*result = value->data.string_value;
	return 0;
}

/***************************************************************************/
/* Iterator */

void conf_iterator_begin(conf_iterator* i, struct conf_context* context, const char* tag) {
	i->context = context;
	i->value = value_searchbest_tag(context, (const char**)context->section_map, context->section_mac, tag);
}

void conf_iterator_next(conf_iterator* i) {
	assert(i && i->value);

	i->value = value_searchbest_from(i->context, i->value);
}

conf_bool conf_iterator_is_end(const conf_iterator* i) {
	assert(i);

	return i->value == 0;
}

const char* conf_iterator_string_get(const conf_iterator* i) {
	assert(i && i->value && i->value->option->type == conf_type_string);

	return i->value->data.string_value;
}

/***************************************************************************/
/* Set */

conf_error conf_set(struct conf_context* context, const char* section, const char* tag, const char* result) {
	struct conf_input* input;
	struct conf_option* option;

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	return value_set_dup(context, input, option, section, "", result, result, 0, 0);
}

conf_error conf_bool_set(struct conf_context* context, const char* section, const char* tag, conf_bool result) {
	const char* result_string;

	assert_option(context, tag, conf_type_bool);

	result_string = result ? "yes" : "no";

	return conf_set(context, section, tag, result_string);
}

conf_error conf_int_set(struct conf_context* context, const char* section, const char* tag, int result) {
	const char* result_string;
	char result_buffer[CONF_NUM_BUFFER_MAX];
	struct conf_option* option;

	assert_option(context, tag, conf_type_int);

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	if (!option->data.base_int.has_enum) {
		sprintf(result_buffer,"%d",(int)result);
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

conf_error conf_float_set(struct conf_context* context, const char* section, const char* tag, double result) {
	char result_buffer[CONF_NUM_BUFFER_MAX];
	const char* result_string;

	assert_option(context, tag, conf_type_float);

	sprintf(result_buffer,"%g",(double)result);
        result_string = result_buffer;

	return conf_set(context, section, tag, result_string);
}

conf_error conf_string_set(struct conf_context* context, const char* section, const char* tag, const char* result) {
	const char* result_string;

	assert_option(context, tag, conf_type_string);

	result_string = result;

	return conf_set(context, section, tag, result_string);
}

conf_error conf_remove(struct conf_context* context, const char* section, const char* tag) {
	struct conf_value* value;
	struct conf_input* input;

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
 * \param buffer destination buffer used for int/float values
 * \return 0 if the option has not a default.
 */
static const char* option_default_get(struct conf_option* option, char* buffer) {
	switch (option->type) {
		case conf_type_bool :
			if (!option->data.base_bool.has_def)
				return 0;
			return option->data.base_bool.def ? "yes" : "no";
		case conf_type_int :
			if (!option->data.base_int.has_def)
				return 0;
			if (!option->data.base_int.has_enum) {
				sprintf(buffer,"%d",(int)option->data.base_int.def);
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
			sprintf(buffer,"%g",(double)option->data.base_float.def);
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

static const char* value_get(struct conf_value* value, char* buffer) {
	switch (value->option->type) {
		case conf_type_bool :
			return value->data.bool_value ? "yes" : "no";
		case conf_type_int :
			if (!value->option->data.base_int.has_enum) {
				sprintf(buffer,"%d",(int)value->data.int_value);
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
			sprintf(buffer,"%g",(double)value->data.float_value);
			return buffer;
		case conf_type_string :
			return value->data.string_value;
		default:
			assert(0);
			return 0;
	}
}

conf_error conf_set_default(struct conf_context* context, const char* section, const char* tag) {
	struct conf_input* input;
	struct conf_option* option;
	const char* result_string;
	char result_buffer[CONF_NUM_BUFFER_MAX];

	input = input_searchbest_writable(context);
	if (!input)
		return -1;

	option = option_search_tag(context, tag);
	if (!option)
		return -1;

	result_string = option_default_get(option, result_buffer);
	if (!result_string)
		return -1;

	return value_set_dup(context, input, option, section, "", result_string, result_string, 0, 0);
}

void conf_set_default_if_missing(struct conf_context* context, const char* section) {

	if (context->option_list) {
		struct conf_option* option = context->option_list;
		do {
			struct conf_value* value = value_searchbest_sectiontag(context,section,option->tag);
			if (!value)
				conf_set_default(context,section,option->tag);
			option = option->next;
		} while (option != context->option_list);
	}
}

void conf_remove_if_default(struct conf_context* context, const char* section)
{
	if (context->option_list) {
		struct conf_option* option = context->option_list;
		do {
			struct conf_value* value = value_searchbest_sectiontag(context,section,option->tag);
			if (value) {
				char result_buffer[CONF_NUM_BUFFER_MAX];
				char default_buffer[CONF_NUM_BUFFER_MAX];
				const char* result_string = value_get(value,result_buffer);
				const char* default_string = option_default_get(option,default_buffer);
				assert(result_string);
				if (default_string && strcmp(result_string,default_string)==0) {
					value_remove(context, value);
				}
			}
			option = option->next;
		} while (option != context->option_list);
	}
}


