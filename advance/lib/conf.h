/*
 * This file is part of the Advance project.
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

/** \file
 * Configuration.
 */

#ifndef __CONF_H
#define __CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "extra.h"

/**
 * Type of a configuration item.
 */
enum conf_type {
	conf_type_bool,
	conf_type_int,
	conf_type_float,
	conf_type_string
};

struct conf_option {
	enum conf_type type; /**< Type of the configuration item */

	boolean is_multi; /**< Support multi definition */

	char* tag; /**< Name of the option. [heap] */

	union {
		struct {
			boolean has_def;
			boolean def;
		} base_bool;
		struct {
			boolean has_def;
			int def;
			boolean has_limit;
			int limit_low;
			int limit_high;
			boolean has_enum;
			struct conf_enum_int* enum_map;
			unsigned enum_mac;
		} base_int;
		struct {
			boolean has_def;
			double def;
			boolean has_limit;
			double limit_low;
			double limit_high;
		} base_float;
		struct {
			boolean has_def;
			char* def; /**< [heap] */
			boolean has_enum;
			struct conf_enum_string* enum_map;
			unsigned enum_mac;
		} base_string;
	} data;

	struct conf_option* pred; /**< Pred entry on the list. */
	struct conf_option* next; /**< Next entry on the list. */
};

struct conf_input {
	char* file_in; /**< File path for input. [heap] */
	char* file_out; /**< File path for output. [heap]. ==0 for readonly. */

	struct conf_conv* conv_map; /**< Vector of conversion */
	unsigned conv_mac; /** Number of conversion */

	boolean ignore_unknown_flag; /**< If the unknown options must be ignored */

	int priority; /**< Priority of the file. Bigger is more */

	struct conf_input* pred; /**< Pred entry on the list. */
	struct conf_input* next; /**< Next entry on the list. */
};

struct conf_value {
	struct conf_option* option; /**< Registration entry of the value. */
	struct conf_input* input; /**< File where the value lives. */

	char* section; /**< Section of the value */
	char* comment; /**< Comment before the value */
	char* format; /**< Formatted text of the value */

	union {
		boolean bool_value;
		int int_value;
		double float_value;
		char* string_value; /* [heap] */
		const char* enum_string_value;
		int enum_int_value;
	} data;

	struct conf_value* pred; /**< Pred entry on the list. */
	struct conf_value* next; /**< Next entry on the list. */
};

/**
 * Configuration context.
 * This struct contains the status of the configuration system.
 */
struct conf_context {
	struct conf_option* option_list; /**< List of option. */
	struct conf_input* input_list; /**< List of input. */
	struct conf_value* value_list; /**< List of value. */

	char** section_map; /**< Vector of section to search. [heap] */
	unsigned section_mac; /**< Size of the vector of sections */

	boolean is_modified; /**< If the configuration need to be saved */
};

/** \addtogroup Configuration */
/*@{*/

struct conf_context* conf_init(void);
void conf_done(struct conf_context* context);

void conf_sort(struct conf_context* context);
void conf_uncomment(struct conf_context* context);

error conf_save(struct conf_context* context, boolean force);

/**
 * Type of error reading a configuration file.
 */
enum conf_callback_error {
	conf_error_unknown, /**< Unknown option. */
	conf_error_invalid, /**< Invalid argument. */
	conf_error_missing /**< Missing argument. */
};

/**
 * Callback used to notify errors.
 * \param context Context originally passed at the conf_() function.
 * \param error Type of the error.
 * \param file File containing the error.
 * \param tag Tag containing the error.
 * \param valid Description of the valid arguments. To print at the user. It's null if the error type isn't conf_error_invalid.
 * \param desc Description of the error. To print at the user. It's always present.
 */
typedef void conf_error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...);

/**
 * Conversion specification.
 * This struct can be used to specify a conversion filter reading the options.
 */
struct conf_conv {
	char* section_glob; /**< Option to recognize. * any char. */
	char* tag_glob; /**< Option to recognize. * any char. */
	char* value_glob; /**< Value to recognize. * any char. */
	char* section_result; /**< Conversion. %s for the whole original record. */
	char* tag_result; /**< Conversion. %s for the whole original record. Setting an empty tag automatically ignore the option. */
	char* value_result; /**< Conversion. %s for the whole original record. */
	boolean autoreg; /**< Auto registration. */
};

error conf_input_file_load(struct conf_context* context, int priority, const char* file, conf_error_callback* error, void* error_context);
error conf_input_file_load_adv(struct conf_context* context, int priority, const char* file_in, const char* file_out, boolean ignore_unknown, boolean multi_line, const struct conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context);
error conf_input_args_load(struct conf_context* context, int priority, const char* section, int* argc, char* argv[], conf_error_callback* error, void* error_context);

#define conf_size(v) sizeof(v)/sizeof(v[0])
#define conf_enum(v) v, sizeof(v)/sizeof(v[0])

/**
 * Specify a string for an enumeration value.
 * The string tag assumes only the specified string value.
 */
struct conf_enum_string {
	const char* value; /** String to print in the configuration file and value to use. */
};

/**
 * Specify a string-int pair for an enumeration value.
 * The int tag assumes the specified int value when is set to the specified string value.
 */
struct conf_enum_int {
	const char* value; /**< String to print in the configuration file. */
	int map; /**< Value to use. */
};

boolean conf_is_registered(struct conf_context* context, const char* tag);
void conf_bool_register(struct conf_context* context, const char* tag);
void conf_bool_register_default(struct conf_context* context, const char* tag, boolean def);
void conf_int_register(struct conf_context* context, const char* tag);
void conf_int_register_default(struct conf_context* context, const char* tag, int def);
void conf_int_register_limit(struct conf_context* context, const char* tag, int limit_low, int limit_high);
void conf_int_register_limit_default(struct conf_context* context, const char* tag, int limit_low, int limit_high, int def);
void conf_int_register_enum_default(struct conf_context* context, const char* tag, struct conf_enum_int* enum_map, unsigned enum_mac, int def);
void conf_float_register(struct conf_context* context, const char* tag);
void conf_float_register_default(struct conf_context* context, const char* tag, double def);
void conf_float_register_limit_default(struct conf_context* context, const char* tag, double limit_low, double limit_high, double def);
void conf_string_register(struct conf_context* context, const char* tag);
void conf_string_register_multi(struct conf_context* context, const char* tag);
void conf_string_register_default(struct conf_context* context, const char* tag, const char* def);
void conf_string_register_enum_default(struct conf_context* context, const char* tag, struct conf_enum_string* enum_map, unsigned enum_mac, const char* def);

/**
 * Iterator for multi value options.
 */
typedef struct conf_iterator_struct {
	struct conf_context* context; /**< Parent context. */
	struct conf_value* value; /**< Value. */
} conf_iterator;

void conf_iterator_begin(conf_iterator* i, struct conf_context* context, const char* tag);
void conf_iterator_next(conf_iterator* i);
boolean conf_iterator_is_end(const conf_iterator* i);
const char* conf_iterator_string_get(const conf_iterator* i);

void conf_section_set(struct conf_context* context, const char** section_map, unsigned section_mac);

boolean conf_bool_get_default(struct conf_context* context, const char* tag);
int conf_int_get_default(struct conf_context* context, const char* tag);
double conf_float_get_default(struct conf_context* context, const char* tag);
const char* conf_string_get_default(struct conf_context* context, const char* tag);
error conf_bool_get(struct conf_context* context, const char* tag, boolean* result);
error conf_int_get(struct conf_context* context, const char* tag, int* result);
error conf_float_get(struct conf_context* context, const char* tag, double* result);
error conf_string_get(struct conf_context* context, const char* tag, const char** result);
error conf_string_section_get(struct conf_context* context, const char* section, const char* tag, const char** result);

error conf_bool_set(struct conf_context* context, const char* section, const char* tag, boolean value);
error conf_int_set(struct conf_context* context, const char* section, const char* tag, int value);
error conf_float_set(struct conf_context* context, const char* section, const char* tag, double value);
error conf_string_set(struct conf_context* context, const char* section, const char* tag, const char* value);
error conf_set(struct conf_context* context, const char* section, const char* tag, const char* value);
error conf_set_default(struct conf_context* context, const char* section, const char* tag);
void conf_set_default_if_missing(struct conf_context* context, const char* section);

void conf_remove_if_default(struct conf_context* context, const char* section);
error conf_remove(struct conf_context* context, const char* section, const char* tag);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


