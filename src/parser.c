/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>

#include"lavalauncher.h"
#include"config.h"
#include"parser.h"
#include"button.h"

/* Get the next char from file stream. */
static bool parser_get_char (struct Lava_parser *parser, char *ch)
{
	errno = 0;
	*ch = fgetc(parser->file);
	parser->last_char = *ch;

	switch (*ch)
	{
		case EOF:
			if ( errno != 0 )
				fprintf(stderr, "ERROR: fgetc: %s\n", strerror(errno));
			*ch = '\0';
			return false;

		case '\n':
			parser->line++;
			parser->column = 0;
			break;

		default:
			parser->column++;
			break;
	}

	return true;
}

/* Skip a line, meaning ignore all chars until the next newline or EOF, starting
 * from the current column.
 */
static bool parser_ignore_line (struct Lava_parser *parser)
{
	for (char ch;;)
	{
		if (! parser_get_char(parser, &ch))
			return false;
		if ( ch == '\n' || ch == '\0' )
			return true;
	}
}

/* Handle reaching EOF. */
static bool parser_handle_eof (struct Lava_parser *parser)
{
	if ( parser->action == ACTION_NONE && parser->context == CONTEXT_NONE )
		return true;
	else
	{
		fputs("ERROR: Unexpectedly reached end of config file.\n", stderr);
		return false;
	}
}

/* Enter a global *_PRE context. */
static bool parser_enter_global_pre_context (struct Lava_parser *parser)
{
	if (! strcmp(parser->buffer, "settings"))
		parser->context = CONTEXT_SETTINGS_PRE;
	else if (! strcmp(parser->buffer, "items"))
		parser->context = CONTEXT_ITEMS_PRE;
	else
		return false;
	return true;
}

/* Enter an item *_PRE context. */
static bool parser_enter_item_pre_context (struct Lava_parser *parser)
{
	if (! strcmp(parser->buffer, "button"))
		parser->context = CONTEXT_BUTTON_PRE;
	else if (! strcmp(parser->buffer, "spacer"))
		parser->context = CONTEXT_SPACER_PRE;
	else
		return false;
	return true;
}

/* Fully enter a context. */
static bool parser_enter_context (struct Lava_parser *parser)
{
	bool ret = true;
	switch (parser->context)
	{
		case CONTEXT_SETTINGS_PRE: parser->context = CONTEXT_SETTINGS; break;
		case CONTEXT_ITEMS_PRE:    parser->context = CONTEXT_ITEMS;    break;

		case CONTEXT_BUTTON_PRE:
			ret = create_button(parser->data);
			parser->context = CONTEXT_BUTTON;
			break;

		case CONTEXT_SPACER_PRE:
			ret = create_spacer(parser->data);
			parser->context = CONTEXT_SPACER;
			break;

		default: parser->context = CONTEXT_NONE;     break;
	}
	return ret;
}

/* Leave a context and go back to the parent context. */
static void parser_leave_context (struct Lava_parser *parser)
{
	switch (parser->context)
	{
		case CONTEXT_SETTINGS: parser->context = CONTEXT_NONE;  break;
		case CONTEXT_ITEMS:    parser->context = CONTEXT_NONE;  break;
		case CONTEXT_BUTTON:   parser->context = CONTEXT_ITEMS; break;
		case CONTEXT_SPACER:   parser->context = CONTEXT_ITEMS; break;
		default:               parser->context = CONTEXT_NONE;  break;
	}
}

/* Check if the parser is currently waiting for a bracket. { } */
static bool parser_is_waiting_for_bracket (struct Lava_parser *parser, const char bracket)
{
	if ( parser->action != ACTION_NONE )
		return false;

	if ( bracket == '{' )
		switch (parser->context)
		{
			case CONTEXT_SETTINGS_PRE:
			case CONTEXT_ITEMS_PRE:
			case CONTEXT_BUTTON_PRE:
			case CONTEXT_SPACER_PRE:
				return true;

			default:
				return false;
		}
	else if ( bracket == '}' )
		switch (parser->context)
		{
			case CONTEXT_SETTINGS:
			case CONTEXT_ITEMS:
			case CONTEXT_BUTTON:
			case CONTEXT_SPACER:
				return true;

			default:
				return false;
		}
	else
		return false;
}

/* Handle brackets. { } */
static bool parser_handle_bracket (struct Lava_parser *parser, const char ch)
{
	if (! parser_is_waiting_for_bracket(parser, ch))
	{
		fprintf(stderr, "ERROR: Unexpected '%c' in config file: line=%d\n",
				ch, parser->line);
		return false;
	}

	if ( ch == '{' )
		return parser_enter_context(parser);
	else
		parser_leave_context(parser);

	return true;
}

static void parser_get_string (struct Lava_parser *parser)
{
	parser->buffer[0]     = parser->last_char;
	parser->buffer[1]     = '\0';
	parser->buffer_length = 1;
	for (char ch;;)
	{
		parser_get_char(parser, &ch);
		if ( ch == '\n' )
		{
			parser->line--;
			goto seek;
		}
		if ( isspace(ch) || ch == '\0' || ch == '\n' || ch == '#' || ch == ';' )
			goto seek;
		parser->buffer[parser->buffer_length] = ch;
		parser->buffer_length++;
		parser->buffer[parser->buffer_length] = '\0';
	}
seek:
	parser->buffer[1024-1] = '\0'; /* Overflow protection. */
	fseek(parser->file, -1, SEEK_CUR);
	return;
}

static bool parser_get_quoted_string (struct Lava_parser *parser)
{
	parser->buffer_length = 0;
	for (char ch;;)
	{
		parser_get_char(parser, &ch);
		if ( ch == '\0' || ch == '\n' )
		{
			fprintf(stderr, "ERROR: Unterminated quoted string: line=%d\n",
					parser->line);
			return false;
		}
		else if ( ch == '"' )
		{
			parser->buffer[1024-1] = '\0'; /* Overflow protection. */
			return true;
		}
		parser->buffer[parser->buffer_length] = ch;
		parser->buffer_length++;
		parser->buffer[parser->buffer_length] = '\0';
	}
}

static bool parser_handle_settings (struct Lava_parser *parser)
{
	if ( parser->action == ACTION_NONE )
	{
		strncpy(parser->buffer_2, parser->buffer, 1024-1);
		parser->buffer_2_length = parser->buffer_length;
		parser->action = ACTION_ASSIGN;
		return true;
	}
	else if ( parser->action == ACTION_ASSIGN )
	{
		parser->action = ACTION_ASSIGNED;
		switch (parser->context)
		{
			case CONTEXT_SETTINGS:
				return config_set_variable(parser->data,
						parser->buffer_2, parser->buffer,
						parser->line);

			case CONTEXT_BUTTON:
				return button_set_variable(parser->data,
						parser->buffer_2, parser->buffer,
						parser->line);

			case CONTEXT_SPACER:
				return spacer_set_variable(parser->data,
						parser->buffer_2, parser->buffer,
						parser->line);
			default:
				break;
		}
	}
	return false;
}

static bool parser_handle_string (struct Lava_parser *parser)
{
	if ( parser->action == ACTION_ASSIGNED )
		goto error;

	switch (parser->context)
	{
		case CONTEXT_NONE:
			if (! parser_enter_global_pre_context(parser))
				goto error;
			return true;
		case CONTEXT_ITEMS:
			if (! parser_enter_item_pre_context(parser))
				goto error;
			return true;

		case CONTEXT_SETTINGS:
		case CONTEXT_BUTTON:
		case CONTEXT_SPACER:
			return parser_handle_settings(parser);

		case CONTEXT_SETTINGS_PRE:
		case CONTEXT_ITEMS_PRE:
		case CONTEXT_BUTTON_PRE:
		case CONTEXT_SPACER_PRE:
		default:
			goto error;
	}

error:
	fprintf(stderr, "ERROR: Unexpected \"%s\" in config file: line=%d\n",
			parser->buffer, parser->line);
	return false;
}

bool parse_config (struct Lava_data *data, const char *config_path)
{
	errno = 0;
	struct Lava_parser parser = {
		.data    = data,
		.file    = fopen(config_path, "r"),
		.line    = 1,
		.column  = 0,
		.context = CONTEXT_NONE,
		.action  = ACTION_NONE
	};
	if ( parser.file == NULL )
	{
		fprintf(stderr, "ERROR: Can not open config file.\n"
				"ERROR: fopen: %s\n", strerror(errno));
		return false;
	}

	bool ret = true;
	for(char ch;;)
	{
		parser_get_char(&parser, &ch);

		/* We have to check both the current read char as well as the
		 * last one, as parser_ignore_line() can encounter a EOF but
		 * does not handle it.
		 */
		if ( parser.last_char == '\0' || ch == '\0' ) /* Handle EOF. */
		{
			ret = parser_handle_eof(&parser);
			goto exit;
		}
		else if ( ch == '#' ) /* Skip comments. */
		{
			if (! parser_ignore_line(&parser))
			{
				ret = false;
				goto exit;
			}
		}
		else if (isspace(ch)) /* Skip whitespace. */
			continue;
		else if ( ch == '{' || ch == '}' ) /* Handle brackets. */
		{
			if (! parser_handle_bracket(&parser, ch))
			{
				ret = false;
				goto exit;
			}
		}
		else if ( ch == ';' )
		{
			if ( parser.action != ACTION_ASSIGNED )
			{
				fprintf(stderr, "ERROR: Unexpected ';' in config file: line=%d\n",
						parser.line);
				ret = false;
				goto exit;
			}
			parser.action = ACTION_NONE;
		}
		else /* Parse commands. */
		{
			if ( ch == '\"' )
			{
				if (! parser_get_quoted_string(&parser))
				{
					ret = false;
					goto exit;
				}
			}
			else
				parser_get_string(&parser);

			if (! parser_handle_string(&parser))
			{
				ret = false;
				goto exit;
			}
		}
	}

exit:
	fclose(parser.file);
	return ret;
}
