/*
 * This file is part of the Advance project.
 *
 * Copyright (C) 2026 Andrea Mazzoleni
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
 */

#include "portable.h"

#include <expat.h>

#define CHUNK_SIZE (64 * 1024)
#define TEXT_BUF_SIZE 16384

struct state_t {
	XML_Parser parser;
	FILE* out;
	int depth;
	int ignore_depth;
	int in_device;
	int text_accumulating;
	char text_tag[64];
	char text_buf[TEXT_BUF_SIZE];
	unsigned text_len;
};

static void write_escaped_xml(FILE* out, const char* s, size_t len, int is_attr)
{
	size_t i;
	for (i = 0; i < len; ++i) {
		unsigned char c = (unsigned char)s[i];
		switch (c) {
		case '&': fputs("&amp;", out); break;
		case '<': fputs("&lt;", out); break;
		case '>': fputs("&gt;", out); break;
		case '"':
			if (is_attr) fputs("&quot;", out);
			else fputc('"', out);
			break;
		case '\'':
			if (is_attr) fputs("&apos;", out);
			else fputc('\'', out);
			break;
		default:
			fputc(c, out);
			break;
		}
	}
}

static inline void write_attr(FILE* out, const char* key, const char* val)
{
	fputc(' ', out);
	fputs(key, out);
	fputs("=\"", out);
	write_escaped_xml(out, val, strlen(val), 1);
	fputc('"', out);
}

static int is_game_child_tag(const char* name)
{
	switch (name[0]) {
	case 'd':
		return strcmp(name, "description") == 0
			|| strcmp(name, "driver") == 0
			|| strcmp(name, "device") == 0
			|| strcmp(name, "display") == 0;
	case 'y':
		return strcmp(name, "year") == 0;
	case 'm':
		return strcmp(name, "manufacturer") == 0;
	case 'r':
		return strcmp(name, "rom") == 0
			|| strcmp(name, "romof") == 0
			|| strcmp(name, "runnable") == 0;
	case 'v':
		return strcmp(name, "video") == 0;
	case 'n':
		return strcmp(name, "name") == 0;
	case 'c':
		return strcmp(name, "cloneof") == 0;
	case 'i':
		return strcmp(name, "isbios") == 0
			|| strcmp(name, "isdevice") == 0;
	default:
		return 0;
	}
}

static void start_handler(void* data, const XML_Char* name, const XML_Char** attributes)
{
	struct state_t* state = (struct state_t*)data;
	int i;

	if (state->ignore_depth > 0) {
		++state->ignore_depth;
		return;
	}

	if (state->depth == 1) {
		if (!is_game_child_tag(name)) {
			state->ignore_depth = 1;
			return;
		}
	} else if (state->depth == 2) {
		if (state->in_device) {
			if (strcmp(name, "extension") != 0 && strcmp(name, "name") != 0 && strcmp(name, "type") != 0) {
				state->ignore_depth = 1;
				return;
			}
		}
	}

	if (state->depth >= 3 && !state->in_device) {
		state->ignore_depth = 1;
		return;
	}

	++state->depth;

	if (state->depth == 0) {
		fputs("<?xml version=\"1.0\"?>\n<", state->out);
		fputs(name, state->out);
		for (i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			if (strcmp(k, "build") == 0 || strcmp(k, "mameconfig") == 0) {
				write_attr(state->out, k, attributes[i + 1]);
			}
		}
		fputs(">\n", state->out);
	} else if (state->depth == 1) {
		fputs("\t<", state->out);
		fputs(name, state->out);
		for (i = 0; attributes[i]; i += 2) {
			const char* k = attributes[i];
			if (strcmp(k, "name") == 0 || strcmp(k, "cloneof") == 0 ||
				strcmp(k, "romof") == 0 || strcmp(k, "runnable") == 0 ||
				strcmp(k, "isbios") == 0 || strcmp(k, "isdevice") == 0) {
				write_attr(state->out, k, attributes[i + 1]);
			}
		}
		fputs(">\n", state->out);
	} else if (state->depth == 2) {
		if (strcmp(name, "rom") == 0) {
			fputs("\t\t<rom", state->out);
			for (i = 0; attributes[i]; i += 2) {
				const char* k = attributes[i];
				if (strcmp(k, "name") == 0 || strcmp(k, "size") == 0 || strcmp(k, "merge") == 0) {
					write_attr(state->out, k, attributes[i + 1]);
				}
			}
			fputs("/>\n", state->out);
		} else if (strcmp(name, "driver") == 0) {
			fputs("\t\t<driver", state->out);
			for (i = 0; attributes[i]; i += 2) {
				if (strcmp(attributes[i], "status") == 0) {
					write_attr(state->out, "status", attributes[i + 1]);
				}
			}
			fputs("/>\n", state->out);
		} else if (strcmp(name, "video") == 0 || strcmp(name, "display") == 0) {
			fputs("\t\t<video", state->out);
			for (i = 0; attributes[i]; i += 2) {
				const char* k = attributes[i];
				if (strcmp(k, "screen") == 0 || strcmp(k, "orientation") == 0 ||
					strcmp(k, "width") == 0 || strcmp(k, "height") == 0 ||
					strcmp(k, "aspectx") == 0 || strcmp(k, "aspecty") == 0) {
					write_attr(state->out, k, attributes[i + 1]);
				}
			}
			fputs("/>\n", state->out);
		} else if (strcmp(name, "device") == 0) {
			state->in_device = 1;
			fputs("\t\t<device", state->out);
			for (i = 0; attributes[i]; i += 2) {
				const char* k = attributes[i];
				if (strcmp(k, "name") == 0 || strcmp(k, "type") == 0) {
					write_attr(state->out, k, attributes[i + 1]);
				}
			}
			fputs(">\n", state->out);
		} else if (strcmp(name, "description") == 0 || strcmp(name, "year") == 0 || strcmp(name, "manufacturer") == 0 ||
				   strcmp(name, "name") == 0 || strcmp(name, "cloneof") == 0 || strcmp(name, "romof") == 0 ||
				   strcmp(name, "runnable") == 0 || strcmp(name, "isbios") == 0 || strcmp(name, "isdevice") == 0) {
			state->text_len = 0;
			state->text_accumulating = 1;
			snprintf(state->text_tag, sizeof(state->text_tag), "%s", name);
		}
	} else if (state->depth == 3 && state->in_device) {
		if (strcmp(name, "extension") == 0) {
			fputs("\t\t\t<extension", state->out);
			for (i = 0; attributes[i]; i += 2) {
				if (strcmp(attributes[i], "name") == 0) {
					write_attr(state->out, "name", attributes[i + 1]);
				}
			}
			fputs("/>\n", state->out);
		}
	}
}

static void end_handler(void* data, const XML_Char* name)
{
	struct state_t* state = (struct state_t*)data;

	if (state->ignore_depth > 0) {
		--state->ignore_depth;
		return;
	}

	if (state->depth == 0) {
		fputs("</", state->out);
		fputs(name, state->out);
		fputs(">\n", state->out);
	} else if (state->depth == 1) {
		fputs("\t</", state->out);
		fputs(name, state->out);
		fputs(">\n", state->out);
	} else if (state->depth == 2) {
		if (state->text_accumulating) {
			fputs("\t\t<", state->out);
			fputs(state->text_tag, state->out);
			fputc('>', state->out);
			write_escaped_xml(state->out, state->text_buf, state->text_len, 0);
			fputs("</", state->out);
			fputs(state->text_tag, state->out);
			fputs(">\n", state->out);
			state->text_accumulating = 0;
			state->text_len = 0;
		} else if (strcmp(name, "device") == 0) {
			fputs("\t\t</device>\n", state->out);
			state->in_device = 0;
		}
	}

	--state->depth;
}

static void data_handler(void* data, const XML_Char* s, int len)
{
	struct state_t* state = (struct state_t*)data;

	if (state->ignore_depth > 0 || !state->text_accumulating)
		return;

	if (state->text_len + len < sizeof(state->text_buf)) {
		memcpy(state->text_buf + state->text_len, s, len);
		state->text_len += len;
	}
}

int main(int argc, char* argv[])
{
	struct state_t state;

	(void)argc;
	(void)argv;

#if defined(O_BINARY) && defined(HAVE_SETMODE)
	setmode(fileno(stdin), O_BINARY);
	setmode(fileno(stdout), O_BINARY);
#endif

	memset(&state, 0, sizeof(state));
	state.out = stdout;
	state.depth = -1;
	state.ignore_depth = 0;
	state.parser = XML_ParserCreate(NULL);
	if (!state.parser) {
		fprintf(stderr, "advbare: failed to create XML parser\n");
		return EXIT_FAILURE;
	}

	XML_SetUserData(state.parser, &state);
	XML_SetElementHandler(state.parser, start_handler, end_handler);
	XML_SetCharacterDataHandler(state.parser, data_handler);
	XML_SetParamEntityParsing(state.parser, XML_PARAM_ENTITY_PARSING_NEVER);

	while (1) {
		void* buf = XML_GetBuffer(state.parser, CHUNK_SIZE);
		int len;
		int done;

		if (!buf) {
			fprintf(stderr, "advbare: out of memory\n");
			XML_ParserFree(state.parser);
			return EXIT_FAILURE;
		}

		len = fread(buf, 1, CHUNK_SIZE, stdin);
		if (len < 0) {
			fprintf(stderr, "advbare: read error from standard input\n");
			XML_ParserFree(state.parser);
			return EXIT_FAILURE;
		}

		done = feof(stdin) ? 1 : 0;

		if (XML_ParseBuffer(state.parser, len, done) == XML_STATUS_ERROR) {
			fprintf(stderr, "advbare: XML parse error at line %ld: %s\n",
				(long)XML_GetCurrentLineNumber(state.parser),
				XML_ErrorString(XML_GetErrorCode(state.parser)));
			XML_ParserFree(state.parser);
			return EXIT_FAILURE;
		}

		if (done)
			break;
	}

	XML_ParserFree(state.parser);

	return EXIT_SUCCESS;
}
