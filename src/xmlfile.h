/***************************************************************************

    xmlfile.h

    XML file parsing code.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __XMLFILE_H__
#define __XMLFILE_H__

#include "mamecore.h"
#include "fileio.h"



enum
{
	XML_PARSE_FLAG_WHITESPACE_SIGNIFICANT = 1
};



/*************************************
 *
 *  Type definitions
 *
 *************************************/

struct XML_ParserStruct;


typedef struct _xml_attribute_node xml_attribute_node;
struct _xml_attribute_node
{
	xml_attribute_node *next;				/* pointer to next attribute node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
};


/* In mamecore.h: typedef struct _xml_data_node xml_data_node; */
struct _xml_data_node
{
	xml_data_node *next;					/* pointer to next sibling node */
	xml_data_node *parent;					/* pointer to parent node */
	xml_data_node *child;					/* pointer to first child node */
	const char *name;						/* pointer to copy of tag name */
	const char *value;						/* pointer to copy of value string */
	xml_attribute_node *attribute;			/* pointer to array of attribute nodes */
};


typedef struct _xml_parse_error xml_parse_error;
struct _xml_parse_error
{
	const char *error_message;
	int error_line;
	int error_column;
};


typedef struct _xml_parse_options xml_parse_options;
struct _xml_parse_options
{
	xml_parse_error *error;
	void (*init_parser)(struct XML_ParserStruct *parser);
	UINT32 flags;
};



/*************************************
 *
 *  Function prototypes
 *
 *************************************/

xml_data_node *xml_file_create(void);
xml_data_node *xml_file_read(mame_file *file, xml_parse_options *opts);
xml_data_node *xml_string_read(const char *string, xml_parse_options *opts);
void xml_file_write(xml_data_node *node, mame_file *file);
void xml_file_free(xml_data_node *node);

int xml_count_children(xml_data_node *node);
xml_data_node *xml_get_sibling(xml_data_node *node, const char *name);
xml_data_node *xml_find_matching_sibling(xml_data_node *node, const char *name, const char *attribute, const char *matchval);
xml_attribute_node *xml_get_attribute(xml_data_node *node, const char *attribute);
const char *xml_get_attribute_string(xml_data_node *node, const char *attribute, const char *defvalue);
int xml_get_attribute_int(xml_data_node *node, const char *attribute, int defvalue);
float xml_get_attribute_float(xml_data_node *node, const char *attribute, float defvalue);

xml_data_node *xml_add_child(xml_data_node *node, const char *name, const char *value);
xml_data_node *xml_get_or_add_child(xml_data_node *node, const char *name, const char *value);
xml_attribute_node *xml_set_attribute(xml_data_node *node, const char *name, const char *value);
xml_attribute_node *xml_set_attribute_int(xml_data_node *node, const char *name, int value);
xml_attribute_node *xml_set_attribute_float(xml_data_node *node, const char *name, float value);
void xml_delete_node(xml_data_node *node);

#endif	/* __XMLFILE_H__ */
