/***************************************************************************

    xmlfile.c

    XML file parsing code.

    Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#include "mame.h"
#include "fileio.h"
#include "xmlfile.h"
#include <ctype.h>
#include <expat.h>

#define TEMP_BUFFER_SIZE		4096


typedef struct _xml_parse_info xml_parse_info;
struct _xml_parse_info
{
	XML_Parser			parser;
	xml_data_node *		rootnode;
	xml_data_node *		curnode;
	UINT32				flags;
};



/*************************************
 *
 *  Utility function for copying a
 *  string
 *
 *************************************/

static const char *copystring(const char *input)
{
	char *newstr;

	/* NULL just passes through */
	if (!input)
		return NULL;

	/* make a lower-case copy if the allocation worked */
	newstr = malloc(strlen(input) + 1);
	if (newstr)
		strcpy(newstr, input);

	return newstr;
}



/*************************************
 *
 *  Utility function for copying a
 *  string and converting to lower
 *  case
 *
 *************************************/

static const char *copystring_lower(const char *input)
{
	char *newstr;
	int i;

	/* NULL just passes through */
	if (!input)
		return NULL;

	/* make a lower-case copy if the allocation worked */
	newstr = malloc(strlen(input) + 1);
	if (newstr)
	{
		for (i = 0; input[i] != 0; i++)
			newstr[i] = tolower(input[i]);
		newstr[i] = 0;
	}

	return newstr;
}



/*************************************
 *
 *  Add a new child node
 *
 *************************************/

static xml_data_node *add_child(xml_data_node *parent, const char *name, const char *value)
{
	xml_data_node **pnode;
	xml_data_node *node;

	/* new element: create a new node */
	node = malloc(sizeof(*node));
	if (!node)
		return NULL;

	/* initialize the members */
	node->next = NULL;
	node->parent = parent;
	node->child = NULL;
	node->name = copystring_lower(name);
	if (!node->name)
	{
		free(node);
		return NULL;
	}
	node->value = copystring(value);
	if (!node->value && value)
	{
		free((void *)node->name);
		free(node);
		return NULL;
	}
	node->attribute = NULL;

	/* add us to the end of the list of siblings */
	for (pnode = &parent->child; *pnode; pnode = &(*pnode)->next) ;
	*pnode = node;

	return node;
}



/*************************************
 *
 *  Add a new attribute node
 *
 *************************************/

static xml_attribute_node *add_attribute(xml_data_node *node, const char *name, const char *value)
{
	xml_attribute_node *anode, **panode;

	/* allocate a new attribute node */
	anode = malloc(sizeof(*anode));
	if (!anode)
		return NULL;

	/* fill it in */
	anode->next = NULL;
	anode->name = copystring_lower(name);
	if (!anode->name)
	{
		free(anode);
		return NULL;
	}
	anode->value = copystring(value);
	if (!anode->value)
	{
		free((void *)anode->name);
		free(anode);
		return NULL;
	}

	/* add us to the end of the list of attributes */
	for (panode = &node->attribute; *panode; panode = &(*panode)->next) ;
	*panode = anode;

	return anode;
}



/*************************************
 *
 *  XML callback for a new element
 *
 *************************************/

static void xml_element_start(void *data, const XML_Char *name, const XML_Char **attributes)
{
	xml_parse_info *parse_info = (xml_parse_info *) data;
	xml_data_node **curnode = &parse_info->curnode;
	xml_data_node *newnode;
	int attr;

	/* add a new child node to the current node */
	newnode = add_child(*curnode, name, NULL);
	if (!newnode)
		return;

	/* add all the attributes as well */
	for (attr = 0; attributes[attr]; attr += 2)
		add_attribute(newnode, attributes[attr+0], attributes[attr+1]);

	/* set us up as the current node */
	*curnode = newnode;
}



/*************************************
 *
 *  XML callback for element data
 *
 *************************************/

static void xml_data(void *data, const XML_Char *s, int len)
{
	xml_parse_info *parse_info = (xml_parse_info *) data;
	xml_data_node **curnode = &parse_info->curnode;
	int oldlen = 0;
	char *newdata;

	/* if no data, skip */
	if (len == 0)
		return;

	/* determine how much data we currently have */
	if ((*curnode)->value)
		oldlen = strlen((*curnode)->value);

	/* realloc */
	newdata = realloc((void *)(*curnode)->value, oldlen + len + 1);
	if (!newdata)
		return;

	/* copy in the new data a NULL-terminate */
	memcpy(&newdata[oldlen], s, len);
	newdata[oldlen + len] = 0;
	(*curnode)->value = newdata;
}



/*************************************
 *
 *  XML callback for element end
 *
 *************************************/

static void xml_element_end(void *data, const XML_Char *name)
{
	xml_parse_info *parse_info = (xml_parse_info *) data;
	xml_data_node **curnode = &parse_info->curnode;
	char *orig;

	/* strip leading/trailing spaces from the value data */
	orig = (char *)(*curnode)->value;
	if (orig && !(parse_info->flags & XML_PARSE_FLAG_WHITESPACE_SIGNIFICANT))
	{
		char *start = orig;
		char *end = start + strlen(start);

		/* first strip leading spaces */
		while (*start && isspace(*start))
			start++;

		/* then strip trailing spaces */
		while (end > start && isspace(end[-1]))
			end--;

		/* if nothing left, just free it */
		if (start == end)
		{
			free(orig);
			(*curnode)->value = NULL;
		}

		/* otherwise, memmove the data */
		else
		{
			memmove(orig, start, end - start);
			orig[end - start] = 0;
		}
	}

	/* back us up a node */
	*curnode = (*curnode)->parent;
}



/*************************************
 *
 *  XML file create
 *
 *************************************/

xml_data_node *xml_file_create(void)
{
	xml_data_node *rootnode;

	/* create a root node */
	rootnode = malloc(sizeof(*rootnode));
	if (!rootnode)
		return NULL;
	memset(rootnode, 0, sizeof(*rootnode));
	return rootnode;
}



/*************************************
 *
 *  XML parser setup
 *
 *************************************/

static int setup_parser(xml_parse_info *parse_info, xml_parse_options *opts)
{
	/* setup parse_info structure */
	memset(parse_info, 0, sizeof(*parse_info));
	if (opts)
	{
		parse_info->flags = opts->flags;
		if (opts->error)
		{
			opts->error->error_message = NULL;
			opts->error->error_line = 0;
			opts->error->error_column = 0;
		}
	}

	/* create a root node */
	parse_info->rootnode = xml_file_create();
	if (!parse_info->rootnode)
		return FALSE;
	parse_info->curnode = parse_info->rootnode;

	/* create the XML parser */
	parse_info->parser = XML_ParserCreate(NULL);
	if (!parse_info->parser)
	{
		free(parse_info->rootnode);
		return FALSE;
	}

	/* configure the parser */
	XML_SetElementHandler(parse_info->parser, xml_element_start, xml_element_end);
	XML_SetCharacterDataHandler(parse_info->parser, xml_data);
	XML_SetUserData(parse_info->parser, parse_info);

	/* optional parser initialization step */
	if (opts && opts->init_parser)
		opts->init_parser(parse_info->parser);
	return TRUE;
}



/*************************************
 *
 *  XML file read
 *
 *************************************/

xml_data_node *xml_file_read(mame_file *file, xml_parse_options *opts)
{
	xml_parse_info parse_info;
	int done;

	/* set up the parser */
	if (!setup_parser(&parse_info, opts))
		return NULL;

	/* loop through the file and parse it */
	do
	{
		char tempbuf[TEMP_BUFFER_SIZE];

		/* read as much as we can */
		int bytes = mame_fread(file, tempbuf, sizeof(tempbuf));
		done = mame_feof(file);

		/* parse the data */
		if (XML_Parse(parse_info.parser, tempbuf, bytes, done) == XML_STATUS_ERROR)
		{
			if (opts && opts->error)
		{
				opts->error->error_message = XML_ErrorString(XML_GetErrorCode(parse_info.parser));
				opts->error->error_line = XML_GetCurrentLineNumber(parse_info.parser);
				opts->error->error_column = XML_GetCurrentColumnNumber(parse_info.parser);
			}

			xml_file_free(parse_info.rootnode);
			XML_ParserFree(parse_info.parser);
			return NULL;
		}

	} while (!done);

	/* free the parser */
	XML_ParserFree(parse_info.parser);

	/* return the root node */
	return parse_info.rootnode;
}



/*************************************
 *
 *  XML string read
 *
 *************************************/

xml_data_node *xml_string_read(const char *string, xml_parse_options *opts)
{
	xml_parse_info parse_info;
	int length = strlen(string);

	/* set up the parser */
	if (!setup_parser(&parse_info, opts))
		return NULL;

	/* parse the data */
	if (XML_Parse(parse_info.parser, string, length, TRUE) == XML_STATUS_ERROR)
	{
		if (opts && opts->error)
		{
			opts->error->error_message = XML_ErrorString(XML_GetErrorCode(parse_info.parser));
			opts->error->error_line = XML_GetCurrentLineNumber(parse_info.parser);
			opts->error->error_column = XML_GetCurrentColumnNumber(parse_info.parser);
	}

		xml_file_free(parse_info.rootnode);
		XML_ParserFree(parse_info.parser);
		return NULL;
	}

	/* free the parser */
	XML_ParserFree(parse_info.parser);

	/* return the root node */
	return parse_info.rootnode;
}



/*************************************
 *
 *  Recursive XML node writer
 *
 *************************************/

static void xml_write_node_recursive(xml_data_node *node, int indent, mame_file *file)
{
	xml_attribute_node *anode;
	xml_data_node *child;

	/* output this tag */
	mame_fprintf(file, "%*s<%s", indent, "", node->name);

	/* output any attributes */
	for (anode = node->attribute; anode; anode = anode->next)
		mame_fprintf(file, " %s=\"%s\"", anode->name, anode->value);

	/* if there are no children and no value, end the tag here */
	if (!node->child && !node->value)
		mame_fprintf(file, " />\n");

	/* otherwise, close this tag and output more stuff */
	else
	{
		mame_fprintf(file, ">\n");

		/* if there is a value, output that here */
		if (node->value)
			mame_fprintf(file, "%*s%s\n", indent + 4, "", node->value);

		/* loop over children and output them as well */
		if (node->child)
		{
			for (child = node->child; child; child = child->next)
				xml_write_node_recursive(child, indent + 4, file);
		}

		/* write a closing tag */
		mame_fprintf(file, "%*s</%s>\n", indent, "", node->name);
	}
}



/*************************************
 *
 *  XML file write
 *
 *************************************/

void xml_file_write(xml_data_node *node, mame_file *file)
{
	/* ensure this is a root node */
	assert_always(node->name == NULL, "xml_file_write called with a non-root node");

	/* output a simple header */
	mame_fprintf(file, "<?xml version=\"1.0\"?>\n");
	mame_fprintf(file, "<!-- This file is autogenerated; comments and unknown tags will be stripped -->\n");

	/* loop over children of the root and output */
	for (node = node->child; node; node = node->next)
		xml_write_node_recursive(node, 0, file);
}



/*************************************
 *
 *  Recursive XML node freeing
 *
 *************************************/

static void xml_free_node_recursive(xml_data_node *node)
{
	xml_attribute_node *anode, *nanode;
	xml_data_node *child, *nchild;

	/* free name/value */
	if (node->name)
		free((void *)node->name);
	if (node->value)
		free((void *)node->value);

	/* free attributes */
	for (anode = node->attribute; anode; anode = nanode)
	{
		/* free name/value */
		if (anode->name)
			free((void *)anode->name);
		if (anode->value)
			free((void *)anode->value);

		/* note the next node and free this node */
		nanode = anode->next;
		free(anode);
	}

	/* free the children */
	for (child = node->child; child; child = nchild)
	{
		/* note the next node and free this node */
		nchild = child->next;
		xml_free_node_recursive(child);
	}

	/* finally free ourself */
	free(node);
}



/*************************************
 *
 *  XML file free
 *
 *************************************/

void xml_file_free(xml_data_node *node)
{
	/* ensure this is a root node */
	assert_always(node->name == NULL, "xml_file_free called with a non-root node");

	xml_free_node_recursive(node);
}



/*************************************
 *
 *  Child node counter
 *
 *************************************/

int xml_count_children(xml_data_node *node)
{
	int count = 0;

	/* loop over children and count */
	for (node = node->child; node; node = node->next)
		count++;
	return count;
}



/*************************************
 *
 *  Sibling node finder
 *
 *************************************/

xml_data_node *xml_get_sibling(xml_data_node *node, const char *name)
{
	/* loop over siblings and find a matching name */
	for ( ; node; node = node->next)
		if (!strcmp(node->name, name))
			return node;
	return NULL;
}



/*************************************
 *
 *  Sibling node finder via attributes
 *
 *************************************/

xml_data_node *xml_find_matching_sibling(xml_data_node *node, const char *name, const char *attribute, const char *matchval)
{
	/* loop over siblings and find a matching attribute */
	for ( ; node; node = node->next)
	{
		/* can pass NULL as a wildcard for the node name */
		if (!name || !strcmp(name, node->name))
		{
			/* find a matching attribute */
			xml_attribute_node *attr = xml_get_attribute(node, attribute);
			if (attr && !strcmp(attr->value, matchval))
				return node;
		}
	}
	return NULL;
}



/*************************************
 *
 *  Attribute node finder
 *
 *************************************/

xml_attribute_node *xml_get_attribute(xml_data_node *node, const char *attribute)
{
	xml_attribute_node *anode;

	/* loop over attributes and find a match */
	for (anode = node->attribute; anode; anode = anode->next)
		if (!strcmp(anode->name, attribute))
			return anode;
	return NULL;
}


const char *xml_get_attribute_string(xml_data_node *node, const char *attribute, const char *defvalue)
{
	xml_attribute_node *attr = xml_get_attribute(node, attribute);
	return attr ? attr->value : defvalue;
}


int xml_get_attribute_int(xml_data_node *node, const char *attribute, int defvalue)
{
	const char *string = xml_get_attribute_string(node, attribute, NULL);
	int value;

	if (!string || sscanf(string, "%d", &value) != 1)
		return defvalue;
	return value;
}


float xml_get_attribute_float(xml_data_node *node, const char *attribute, float defvalue)
{
	const char *string = xml_get_attribute_string(node, attribute, NULL);
	float value;

	if (!string || sscanf(string, "%f", &value) != 1)
		return defvalue;
	return value;
}



/*************************************
 *
 *  Add a new child node
 *
 *************************************/

xml_data_node *xml_add_child(xml_data_node *node, const char *name, const char *value)
{
	/* just a standard add child */
	return add_child(node, name, value);
}



/*************************************
 *
 *  Find a child node; if not there,
 *  add a new one
 *
 *************************************/

xml_data_node *xml_get_or_add_child(xml_data_node *node, const char *name, const char *value)
{
	xml_data_node *child;

	/* find the child first */
	child = xml_get_sibling(node->child, name);
	if (child)
		return child;

	/* if not found, do a standard add child */
	return add_child(node, name, value);
}



/*************************************
 *
 *  Set an attribute on a node
 *
 *************************************/

xml_attribute_node *xml_set_attribute(xml_data_node *node, const char *name, const char *value)
{
	xml_attribute_node *anode;

	/* first find an existing one to replace */
	anode = xml_get_attribute(node, name);

	/* if we found it, free the old value and replace it */
	if (anode)
	{
		if (anode->value)
			free((void *)anode->value);
		anode->value = copystring(value);
	}

	/* otherwise, create a new node */
	else
		anode = add_attribute(node, name, value);

	return anode;
}


xml_attribute_node *xml_set_attribute_int(xml_data_node *node, const char *name, int value)
{
	char buffer[100];
	sprintf(buffer, "%d", value);
	return xml_set_attribute(node, name, buffer);
}


xml_attribute_node *xml_set_attribute_float(xml_data_node *node, const char *name, float value)
{
	char buffer[100];
	sprintf(buffer, "%f", value);
	return xml_set_attribute(node, name, buffer);
}



/*************************************
 *
 *  Delete a node
 *
 *************************************/

void xml_delete_node(xml_data_node *node)
{
	xml_data_node **pnode;

	/* first unhook us from the list of children of our parent */
	for (pnode = &node->parent->child; *pnode; pnode = &(*pnode)->next)
		if (*pnode == node)
		{
			*pnode = node->next;
			break;
		}

	/* now free ourselves and our children */
	xml_free_node_recursive(node);
}
