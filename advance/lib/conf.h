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
 * Specify a string for an enumeration value.
 * The string tag assumes only the specified string value.
 */
typedef struct adv_conf_enum_string_struct {
	const char* value; /** String to print in the configuration file and value to use. */
} adv_conf_enum_string;

/**
 * Specify a string-int pair for an enumeration value.
 * The int tag assumes the specified int value when is set to the specified string value.
 */
typedef struct adv_conf_enum_int_struct {
	const char* value; /**< String to print in the configuration file. */
	int map; /**< Value to use. */
} adv_conf_enum_int;

/**
 * Type of a configuration item.
 */
enum adv_conf_enum {
	conf_type_bool,
	conf_type_int,
	conf_type_float,
	conf_type_string
};

/**
 * Configuration input.
 * This struct contains a single configuration option.
 */
struct adv_conf_option_struct {
	enum adv_conf_enum type; /**< Type of the configuration item */

	adv_bool is_multi; /**< Support multi definition */

	char* tag; /**< Name of the option. [heap] */

	union {
		struct {
			adv_bool has_def;
			adv_bool def;
		} base_bool;
		struct {
			adv_bool has_def;
			int def;
			adv_bool has_limit;
			int limit_low;
			int limit_high;
			adv_bool has_enum;
			adv_conf_enum_int* enum_map;
			unsigned enum_mac;
		} base_int;
		struct {
			adv_bool has_def;
			double def;
			adv_bool has_limit;
			double limit_low;
			double limit_high;
		} base_float;
		struct {
			adv_bool has_def;
			char* def; /**< [heap] */
			adv_bool has_enum;
			adv_conf_enum_string* enum_map;
			unsigned enum_mac;
		} base_string;
	} data;

	struct adv_conf_option_struct* pred; /**< Pred entry on the list. */
	struct adv_conf_option_struct* next; /**< Next entry on the list. */
};

/**
 * Configuration input.
 * This struct contains a single configuration file.
 */
struct adv_conf_input_struct {
	char* file_in; /**< File path for input. [heap] */
	char* file_out; /**< File path for output. [heap]. ==0 for readonly. */

	struct adv_conf_conv_struct* conv_map; /**< Vector of conversion */
	unsigned conv_mac; /** Number of conversion */

	adv_bool ignore_unknown_flag; /**< If the unknown options must be ignored */

	int priority; /**< Priority of the file. Bigger is more */

	struct adv_conf_input_struct* pred; /**< Pred entry on the list. */
	struct adv_conf_input_struct* next; /**< Next entry on the list. */
};

/**
 * Configuration value.
 * This struct contains a single configuration value.
 */
typedef struct adv_conf_value_struct {
	struct adv_conf_option_struct* option; /**< Registration entry of the value. */
	struct adv_conf_input_struct* input; /**< File where the value lives. */

	char* section; /**< Section of the value */
	char* comment; /**< Comment before the value */
	char* format; /**< Formatted text of the value */

	union {
		adv_bool bool_value;
		int int_value;
		double float_value;
		char* string_value; /* [heap] */
		const char* enum_string_value;
		int enum_int_value;
	} data;

	struct adv_conf_value_struct* pred; /**< Pred entry on the list. */
	struct adv_conf_value_struct* next; /**< Next entry on the list. */
} adv_conf_value;

/**
 * Configuration context.
 * This struct contains the status of the configuration system.
 */
typedef struct adv_conf_struct {
	struct adv_conf_option_struct* option_list; /**< List of option. */
	struct adv_conf_input_struct* input_list; /**< List of input. */
	struct adv_conf_value_struct* value_list; /**< List of value. */

	char** section_map; /**< Vector of section to search. [heap] */
	unsigned section_mac; /**< Size of the vector of sections */

	adv_bool is_modified; /**< If the configuration need to be saved */
} adv_conf;

/** \addtogroup Configuration */
/*@{*/

adv_conf* conf_init(void);
void conf_done(adv_conf* context);

void conf_sort(adv_conf* context);
void conf_uncomment(adv_conf* context);

/**
 * Type of error reading a configuration file.
 */
enum conf_callback_error {
	conf_error_unknown, /**< Unknown option. */
	conf_error_invalid, /**< Invalid argument. */
	conf_error_missing, /**< Missing argument. */
	conf_error_failure /**< Input failure. */
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

adv_error conf_save(adv_conf* context, adv_bool force, adv_bool quiet, conf_error_callback* error, void* error_context);


#define ADV_CONF_CONV_AUTOREG_NO 0 /**< Conversion without autoregistration. */
#define ADV_CONF_CONV_AUTOREG_MULTI 1 /**< Conversion with autoregistration in multi mode. */
#define ADV_CONF_CONV_AUTOREG_SINGLE 2 /**< Conversion with autoregistration in single mode. */

/**
 * Conversion specification.
 * This struct can be used to specify a conversion filter reading the options.
 */
typedef struct adv_conf_conv_struct {
	const char* section_glob; /**< Option to recognize. * any char. */
	const char* tag_glob; /**< Option to recognize. * any char. */
	const char* value_glob; /**< Value to recognize. * any char. */
	const char* section_result; /**< Conversion. %s for the whole original record. */
	const char* tag_result; /**< Conversion. %s for the whole original record. Setting an empty tag automatically ignore the option. */
	const char* value_result; /**< Conversion. %s for the whole original record. */
	int autoreg; /**< Auto registration. One of ADV_CONF_CONF_AUTOREG_*. */
} adv_conf_conv;

adv_error conf_input_file_load(adv_conf* context, int priority, const char* file, conf_error_callback* error, void* error_context);
adv_error conf_input_file_load_adv(adv_conf* context, int priority, const char* file_in, const char* file_out, adv_bool ignore_unknown, adv_bool multi_line, const adv_conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context);
adv_error conf_input_args_load(adv_conf* context, int priority, const char* section, int* argc, char* argv[], conf_error_callback* error, void* error_context);

#define conf_size(v) sizeof(v)/sizeof(v[0])
#define conf_enum(v) v, sizeof(v)/sizeof(v[0])

adv_bool conf_is_registered(adv_conf* context, const char* tag);
void conf_bool_register(adv_conf* context, const char* tag);
void conf_bool_register_default(adv_conf* context, const char* tag, adv_bool def);
void conf_int_register(adv_conf* context, const char* tag);
void conf_int_register_default(adv_conf* context, const char* tag, int def);
void conf_int_register_limit(adv_conf* context, const char* tag, int limit_low, int limit_high);
void conf_int_register_limit_default(adv_conf* context, const char* tag, int limit_low, int limit_high, int def);
void conf_int_register_enum_default(adv_conf* context, const char* tag, adv_conf_enum_int* enum_map, unsigned enum_mac, int def);
void conf_float_register(adv_conf* context, const char* tag);
void conf_float_register_default(adv_conf* context, const char* tag, double def);
void conf_float_register_limit_default(adv_conf* context, const char* tag, double limit_low, double limit_high, double def);
void conf_string_register(adv_conf* context, const char* tag);
void conf_string_register_multi(adv_conf* context, const char* tag);
void conf_string_register_default(adv_conf* context, const char* tag, const char* def);
void conf_string_register_enum_default(adv_conf* context, const char* tag, adv_conf_enum_string* enum_map, unsigned enum_mac, const char* def);

/**
 * Iterator for multi value options.
 */
typedef struct adv_conf_iterator_struct {
	adv_conf* context; /**< Parent context. */
	struct adv_conf_value_struct* value; /**< Value. */
} adv_conf_iterator;

void conf_iterator_begin(adv_conf_iterator* i, adv_conf* context, const char* tag);
void conf_iterator_section_begin(adv_conf_iterator* i, adv_conf* context, const char* section, const char* tag);
void conf_iterator_next(adv_conf_iterator* i);
void conf_iterator_remove(adv_conf_iterator* i);
adv_bool conf_iterator_is_end(const adv_conf_iterator* i);
const char* conf_iterator_string_get(const adv_conf_iterator* i);

void conf_section_set(adv_conf* context, const char** section_map, unsigned section_mac);

adv_bool conf_bool_get_default(adv_conf* context, const char* tag);
int conf_int_get_default(adv_conf* context, const char* tag);
double conf_float_get_default(adv_conf* context, const char* tag);
const char* conf_string_get_default(adv_conf* context, const char* tag);
adv_error conf_bool_get(adv_conf* context, const char* tag, adv_bool* result);
adv_error conf_int_get(adv_conf* context, const char* tag, int* result);
adv_error conf_float_get(adv_conf* context, const char* tag, double* result);
adv_error conf_string_get(adv_conf* context, const char* tag, const char** result);
adv_error conf_int_section_get(adv_conf* context, const char* section, const char* tag, int* result);
adv_error conf_bool_section_get(adv_conf* context, const char* section, const char* tag, int* result);
adv_error conf_string_section_get(adv_conf* context, const char* section, const char* tag, const char** result);

adv_conf_value* conf_value_get(adv_conf* context, const char* tag);
adv_bool conf_value_bool_get(const adv_conf_value* value);
int conf_value_int_get(const adv_conf_value* value);
double conf_value_float_get(const adv_conf_value* value);
const char* conf_value_string_get(const adv_conf_value* value);
const char* conf_value_section_get(const adv_conf_value* value);
enum adv_conf_enum conf_value_type_get(const adv_conf_value* value);

adv_error conf_bool_set(adv_conf* context, const char* section, const char* tag, adv_bool value);
adv_error conf_int_set(adv_conf* context, const char* section, const char* tag, int value);
adv_error conf_float_set(adv_conf* context, const char* section, const char* tag, double value);
adv_error conf_string_set(adv_conf* context, const char* section, const char* tag, const char* value);
adv_error conf_set(adv_conf* context, const char* section, const char* tag, const char* value);

adv_error conf_bool_set_if_different(adv_conf* context, const char* section, const char* tag, adv_bool value);
adv_error conf_int_set_if_different(adv_conf* context, const char* section, const char* tag, int value);
adv_error conf_float_set_if_different(adv_conf* context, const char* section, const char* tag, double value);
adv_error conf_string_set_if_different(adv_conf* context, const char* section, const char* tag, const char* value);
adv_error conf_set_if_different(adv_conf* context, const char* section, const char* tag, const char* value);

adv_error conf_setdefault(adv_conf* context, const char* section, const char* tag);

void conf_setdefault_all_if_missing(adv_conf* context, const char* section);
void conf_remove_all_if_default(adv_conf* context, const char* section);

adv_error conf_remove(adv_conf* context, const char* section, const char* tag);
adv_error conf_remove_if_default(adv_conf* context, const char* section, const char* tag);
adv_error conf_remove_if_notdefault(adv_conf* context, const char* section, const char* tag);

adv_error conf_autoreg_string_set(adv_conf* context, const char* section, const char* tag, const char* value);
adv_error conf_autoreg_string_get(adv_conf* context, const char* tag, const char** result);
adv_error conf_autoreg_remove(adv_conf* context, const char* section, const char* tag);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif


