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

#ifndef __CONF_H
#define __CONF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef int conf_error; /**< Error. ==0 ok, !=0 error */
typedef int conf_bool; /**< Boolean value. */

/***************************************************************************/
/* Private */

enum conf_type {
	conf_type_bool,
	conf_type_int,
	conf_type_float,
	conf_type_string
};

struct conf_option {
	enum conf_type type; /**< Type of the configuration item */

	conf_bool is_multi; /**< Support multi definition */

	char* tag; /**< Name of the option. [heap] */

	union {
		struct {
			conf_bool has_def;
			conf_bool def;
		} base_bool;
		struct {
			conf_bool has_def;
			int def;
			conf_bool has_limit;
			int limit_low;
			int limit_high;
			conf_bool has_enum;
			struct conf_enum_int* enum_map;
			unsigned enum_mac;
		} base_int;
		struct {
			conf_bool has_def;
			double def;
			conf_bool has_limit;
			double limit_low;
			double limit_high;
		} base_float;
		struct {
			conf_bool has_def;
			char* def; /**< [heap] */
			conf_bool has_enum;
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

	conf_bool ignore_unknown_flag; /**< If the unknown options must be ignored */

	int priority; /**< Priority of the file. Bigger is more */

	struct conf_input* pred;
	struct conf_input* next;
};

struct conf_value {
	struct conf_option* option; /**< Registration entry of the value. */
	struct conf_input* input; /**< File where the value lives. */

	char* section; /**< Section of the value */
	char* comment; /**< Comment before the value */
	char* format; /**< Formatted text of the value */

	union {
		conf_bool bool_value;
		int int_value;
		double float_value;
		char* string_value; /* [heap] */
		const char* enum_string_value;
		int enum_int_value;
	} data;

	struct conf_value* pred;
	struct conf_value* next;
};

struct conf_context {
	struct conf_option* option_list;
	struct conf_input* input_list;
	struct conf_value* value_list;

	char** section_map; /**< Vector of section to search. [heap] */
	unsigned section_mac; /**< Size of the vector of sections */

	conf_bool is_modified; /**< If the configuration need to be saved */
};

/***************************************************************************/
/* Public */

/**
 * Initialization of the configuration system.
 */
struct conf_context* conf_init(void);

/**
 * Deinitialization of the configuration system.
 * Free all the memory and updates the configuration files.
 * After this function you can reinitialize calling the conf_init() function.
 */
void conf_done(struct conf_context*);

/**
 * Sort all the configuration options.
 */
void conf_sort(struct conf_context* context);

/**
 * Remove all the comments.
 */
void conf_uncomment(struct conf_context* context);

/**
 * Updates all the writable configuration files.
 * \param force force the rewrite also if the configuration file are unchanged
 */
conf_error conf_save(struct conf_context* context, conf_bool force);

enum conf_callback_error {
	conf_error_unknown, /**< Unknown option. */
	conf_error_invalid, /**< Invalid argument. */
	conf_error_missing /**< Missing argument. */
};

/**
 * Callback used to return errors.
 */
typedef void conf_error_callback(void* context, enum conf_callback_error error, const char* file, const char* tag, const char* valid, const char* desc, ...);

/**
 * Load an input file and store it in memory.
 * \param file path of the configuration file
 * \return
 *   - ==0 if ok
 *   - !=0 if not ok and error callback called
 */
conf_error conf_input_file_load(struct conf_context* context, int priority, const char* file, conf_error_callback* error, void* error_context);

struct conf_conv {
	char* section_glob; /**< Option to recognize. * any char. */
	char* tag_glob; /**< Option to recognize. * any char. */
	char* value_glob; /**< Value to recognize. * any char. */
	char* section_result; /**< Conversion. %s for the whole original record. */
	char* tag_result; /**< Conversion. %s for the whole original record. Setting an empty tag automatically ignore the option. */
	char* value_result; /**< Conversion. %s for the whole original record. */
	conf_bool autoreg; /**< Auto registration */
};

/**
 * Load an input file and store it in memory.
 * The old format file is only read, the file is written with the new format.
 * Any unknown option in the old format is ignored.
 * \param file_in path of the configuration file in the old format
 * \param file_out path of the output configuration, use 0 for a readonly file
 * \return
 *   - ==0 if ok
 *   - !=0 if not ok and error callback called
 */
conf_error conf_input_file_load_adv(struct conf_context* context, int priority, const char* file_in, const char* file_out, conf_bool ignore_unknown, conf_bool multi_line, const struct conf_conv* conv_map, unsigned conv_mac, conf_error_callback* error, void* error_context);

/**
 * Set the list of input arguments.
 * After the call the argc and argv argument contain the unused options.
 * \param section section to use for the command line arguments
 * \param argc pointer at the number of arguments
 * \param argv pointer at the arguments
 */
conf_error conf_input_args_load(struct conf_context* context, int priority, const char* section, int* argc, char* argv[], conf_error_callback* error, void* error_context);

#define conf_size(v) sizeof(v)/sizeof(v[0])
#define conf_enum(v) v, sizeof(v)/sizeof(v[0])

struct conf_enum_string {
	const char* value;
};

struct conf_enum_int {
	const char* value;
	int map;
};

/**
 * Register a value type.
 * \note All the pointer argument must be kept valid until the conf_done() call.
 * \param tag tag of the value
 * \param def default value
 * \param limit_low low limit of the numerical value
 * \param limit_high high limit of the numerical value
 * \param limit_map vector of allowable values
 * \param limit_mac number of element in the vector
 */
void conf_bool_register(struct conf_context* context, const char* tag);
void conf_bool_register_default(struct conf_context* context, const char* tag, conf_bool def);
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
 * Check if a tag is already registered.
 */
conf_bool conf_is_registered(struct conf_context* context, const char* tag);

/**
 * Set the list of sections to try for every get.
 * Every _get operation search on this list of section from the first to
 * the latter for the specified tag.
 * You can change this list every time.
 * \param section_map section list
 */
void conf_section_set(struct conf_context* context, const char** section_map, unsigned section_mac);

/**
 * Get a value with a default.
 * This function get a configuration value which as a default defined.
 * Because of the specification of the default value this function can't fail.
 * \param section the section of the value
 * \param tag the tag of the value
 * \return the value got
 */
conf_bool conf_bool_get_default(struct conf_context* context, const char* tag);
int conf_int_get_default(struct conf_context* context, const char* tag);
double conf_float_get_default(struct conf_context* context, const char* tag);
const char* conf_string_get_default(struct conf_context* context, const char* tag);

/**
 * Iterator for multi value options.
 */
typedef struct conf_iterator_struct {
	struct conf_context* context;
	struct conf_value* value;
} conf_iterator;

void conf_iterator_begin(conf_iterator* i, struct conf_context* context, const char* tag);
void conf_iterator_next(conf_iterator* i);
conf_bool conf_iterator_is_end(const conf_iterator* i);
const char* conf_iterator_string_get(const conf_iterator* i);

/**
 * Get a value.
 * \note The value is searched in the list of section specified by conf_section_set.
 *   If no sections are specified no value is found.
 * \param tag the tag of the value
 * \param result where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
conf_error conf_bool_get(struct conf_context* context, const char* tag, conf_bool* result);
conf_error conf_int_get(struct conf_context* context, const char* tag, int* result);
conf_error conf_float_get(struct conf_context* context, const char* tag, double* result);
conf_error conf_string_get(struct conf_context* context, const char* tag, const char** result);

/**
 * Get a value in the specified section.
 * \param section the section of the value
 * \param tag the tag of the value
 * \param result where the value is copied
 * \return
 *   - ==0 if a value is found
 *   - !=0 if not found
 */
conf_error conf_string_section_get(struct conf_context* context, const char* section, const char* tag, const char** result);

/**
 * Set a value.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 */
conf_error conf_bool_set(struct conf_context* context, const char* section, const char* tag, conf_bool value);
conf_error conf_int_set(struct conf_context* context, const char* section, const char* tag, int value);
conf_error conf_float_set(struct conf_context* context, const char* section, const char* tag, double value);
conf_error conf_string_set(struct conf_context* context, const char* section, const char* tag, const char* value);

/**
 * Set a value of any format.
 * On multi option the values are added and not overwritten.
 * The option is added in the last writable file.
 */
conf_error conf_set(struct conf_context* context, const char* section, const char* tag, const char* value);

/**
 * Set the default value.
 * The option is added in the last writable file.
 */
conf_error conf_set_default(struct conf_context* context, const char* section, const char* tag);

/**
 * Set the default value if the option is missing.
 * If the option is missing in all the input files is added at the last writable.
 */
void conf_set_default_if_missing(struct conf_context* context, const char* section);

/**
 * Remove the option if is set with the default value.
 */
void conf_remove_if_default(struct conf_context* context, const char* section);

/**
 * Remove an option.
 * In case of multi options, all the options are cleared.
 * \param section section of the option
 * \param tag tag of the option
 * \return
 *   - ==0 if a value is found and removed
 *   - !=0 if not found
 */
conf_error conf_remove(struct conf_context* context, const char* section, const char* tag);

#ifdef __cplusplus
}
#endif

#endif
