/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Henrik Plickat
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
#include"log.h"
#include"config/parser.h"
#include"config/global.h"
#include"item/item.h"
#include"bar/bar-pattern.h"

/* Get the next char from file stream. */
static bool parser_get_char (struct Lava_parser *parser, char *ch)
{
	errno = 0;
	*ch = (char)fgetc(parser->file);
	parser->last_char = *ch;

	switch (*ch)
	{
		case EOF:
			if ( errno != 0 )
			{
				log_message(NULL, 0, "ERROR: fgetc: %s\n", strerror(errno));
				return false;
			}
			*ch = '\0';
			break;

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
		log_message(NULL, 0, "ERROR: Unexpectedly reached end of config file.\n");
		return false;
	}
}

/* Fully enter a context. */
static bool parser_enter_context (struct Lava_parser *parser)
{
	bool ret = true;
	switch (parser->context)
	{
		case CONTEXT_GLOBAL_SETTINGS_PRE:
			parser->context = CONTEXT_GLOBAL_SETTINGS;
			break;

		case CONTEXT_BAR_PRE:
			ret = create_bar_pattern(parser->data);
			parser->context = CONTEXT_BAR;
			break;

		case CONTEXT_BAR_COPY_PRE:
			ret = copy_last_bar_pattern(parser->data);
			parser->context = CONTEXT_BAR;
			break;

		case CONTEXT_BUTTON_PRE:
			ret = create_item(parser->data->last_pattern, TYPE_BUTTON);
			parser->context = CONTEXT_BUTTON;
			break;

		case CONTEXT_SPACER_PRE:
			ret = create_item(parser->data->last_pattern, TYPE_SPACER);
			parser->context = CONTEXT_SPACER;
			break;

		default:
			parser->context = CONTEXT_NONE;
			break;
	}
	return ret;
}

/* Leave a context and go back to the parent context. */
static bool parser_leave_context (struct Lava_parser *parser)
{
	switch (parser->context)
	{
		case CONTEXT_GLOBAL_SETTINGS: parser->context = CONTEXT_NONE; break;

		case CONTEXT_BAR:
			parser->context = CONTEXT_NONE;
			if (! finalize_bar_pattern(parser->data->last_pattern))
				return false;
			break;

		case CONTEXT_BUTTON: parser->context = CONTEXT_BAR;  break;
		case CONTEXT_SPACER: parser->context = CONTEXT_BAR;  break;
		default:             parser->context = CONTEXT_NONE; break;
	}
	return true;
}

/* Check if the parser is currently waiting for a bracket. { } */
static bool parser_is_waiting_for_bracket (struct Lava_parser *parser,
		const char bracket)
{
	if ( parser->action != ACTION_NONE )
		return false;

	if ( bracket == '{' )
		switch (parser->context)
		{
			case CONTEXT_GLOBAL_SETTINGS_PRE:
			case CONTEXT_BAR_PRE:
			case CONTEXT_BAR_COPY_PRE:
			case CONTEXT_BUTTON_PRE:
			case CONTEXT_SPACER_PRE:
				return true;

			default:
				return false;
		}
	else if ( bracket == '}' )
		switch (parser->context)
		{
			case CONTEXT_GLOBAL_SETTINGS:
			case CONTEXT_BAR:
			case CONTEXT_BUTTON:
			case CONTEXT_SPACER:
				return true;

			default:
				return false;
		}
	else
		return false;
}

static bool parser_handle_bracket (struct Lava_parser *parser, const char ch)
{
	if (! parser_is_waiting_for_bracket(parser, ch))
	{
		log_message(NULL, 0, "ERROR: Unexpected '%c' in config file on line %d.\n",
				ch, parser->line);
		return false;
	}

	if ( ch == '{' )
		return parser_enter_context(parser);
	else
		return parser_leave_context(parser);
}

static bool parser_handle_semicolon (struct Lava_parser *parser, const char ch)
{
	if ( parser->action != ACTION_ASSIGNED )
	{
		log_message(NULL, 0, "ERROR: Unexpected ';' in config file on line %d.\n",
				parser->line);
		return false;
	}
	parser->action = ACTION_NONE;
	return true;
}

static void parser_buffer_trim_whitespace (struct Lava_parser *parser)
{
	size_t i = parser->buffer_length;
	for (; isspace(parser->buffer[i-1]); i--);
	parser->buffer[i] = '\0';
}

static void parser_buffer_add_char (struct Lava_parser *parser, char ch)
{
	parser->buffer[parser->buffer_length] = ch;
	parser->buffer_length++;
	parser->buffer[parser->buffer_length] = '\0';
}

static bool parser_string_handle_escape (struct Lava_parser *parser)
{
	char ch;
	if (! parser_get_char(parser, &ch))
		return false;

	if ( ch == 'n' )
		ch = '\n';
	else if ( ch == 't' )
		ch = '\t';
	else if ( ch == 'v' )
		ch = '\v';

	parser_buffer_add_char(parser, ch);

	return true;
}

static bool parser_get_string (struct Lava_parser *parser, const char last_ch, bool single_word)
{
	bool quoted;
	if ( last_ch == '"' )
	{
		if (single_word)
			goto unexpected_quote;

		parser->buffer_length = 0;
		parser->buffer[0]     = '\0';
		quoted                = true;
	}
	else
	{
		parser->buffer[0]     = last_ch;
		parser->buffer[1]     = '\0';
		parser->buffer_length = 1;
		quoted                = false;
	}

	for (char ch;;)
	{
		if ( parser->buffer_length >= sizeof(parser->buffer) - 1 )
			goto overflow;

		if (! parser_get_char(parser, &ch))
			return false;

		if ( ch == '\0' )
			goto unterm;
		else if ( isspace(ch) ) /* Also handles '\n' */
		{
			if (single_word)
				goto done;
			if (! quoted)
			{
				if (! isspace(parser->buffer[parser->buffer_length-1]))
					parser_buffer_add_char(parser, ' ');
				continue;
			}
		}
		else if ( ch == '#' || ch == ';' )
		{
			if ( single_word || ! quoted )
				goto done_seek;
		}
		else if ( ch == '"' )
		{
			if (quoted)
				goto done;
		}
		else if ( ch == '\\' )
		{
			if (! parser_string_handle_escape(parser))
					return false;
			continue;
		}

		parser_buffer_add_char(parser, ch);
	}

done_seek:
	fseek(parser->file, -1, SEEK_CUR);
done:
	parser->buffer[1024-1] = '\0';
	if (! quoted)
		parser_buffer_trim_whitespace(parser);
	return true;

unterm:
	log_message(NULL, 0, "ERROR: Unterminated string on line %d.\n", parser->line);
	return false;

overflow:
	log_message(NULL, 0, "ERROR: Buffer overflow due to too long string on line %d.\n", parser->line);
	return false;

unexpected_quote:
	log_message(NULL, 0, "ERROR: Unexpected quotes on line %d.\n", parser->line);
	return false;
}

/* Handle a string which is part of an assign-operation. The string could be
 * either value or variable (tracked in parser->action) in one of three contexts.
 */
static bool parser_handle_settings (struct Lava_parser *parser)
{
	if ( parser->action == ACTION_NONE )
	{
		strncpy(parser->buffer_2, parser->buffer, 1024);
		parser->buffer_2[1024-1] = '\0';
		parser->buffer_2_length  = parser->buffer_length;
		parser->action           = ACTION_ASSIGN;
		return true;
	}
	else if ( parser->action == ACTION_ASSIGN )
	{
		parser->action = ACTION_ASSIGNED;
		switch (parser->context)
		{
			case CONTEXT_GLOBAL_SETTINGS:
				return global_set_variable(parser->data,
						parser->buffer_2, parser->buffer,
						parser->line);

			case CONTEXT_BAR:
				return bar_pattern_set_variable(parser->data->last_pattern,
						parser->buffer_2, parser->buffer,
						parser->line);

			case CONTEXT_BUTTON:
			case CONTEXT_SPACER:
				return item_set_variable(parser->data,
						parser->data->last_pattern->last_item,
						parser->buffer_2, parser->buffer,
						parser->line);

			default:
				break;
		}
	}
	return false;
}

static bool parser_handle_string (struct Lava_parser *parser, const char ch)
{
	if ( parser->action == ACTION_ASSIGNED )
		goto error;

	if (! parser_get_string(parser, ch, parser->action == ACTION_NONE))
		return false;

	switch (parser->context)
	{
		case CONTEXT_NONE:
			if (! strcmp(parser->buffer, "bar"))
			{
				parser->context = CONTEXT_BAR_PRE;
				return true;
			}
			if (! strcmp(parser->buffer, "bar-copy"))
			{
				parser->context = CONTEXT_BAR_COPY_PRE;
				return true;
			}
			if (! strcmp(parser->buffer, "global-settings"))
			{
				parser->context = CONTEXT_GLOBAL_SETTINGS_PRE;
				return true;
			}
			else
				goto error;

		case CONTEXT_BAR:
			if (! strcmp(parser->buffer, "button"))
			{
				parser->context = CONTEXT_BUTTON_PRE;
				return true;
			}
			else if (! strcmp(parser->buffer, "spacer"))
			{
				parser->context = CONTEXT_SPACER_PRE;
				return true;
			}
			else
				return parser_handle_settings(parser);

		case CONTEXT_GLOBAL_SETTINGS:
		case CONTEXT_BUTTON:
		case CONTEXT_SPACER:
			return parser_handle_settings(parser);

		case CONTEXT_BAR_PRE:
		case CONTEXT_BAR_COPY_PRE:
		case CONTEXT_BUTTON_PRE:
		case CONTEXT_SPACER_PRE:
		default:
			goto error;
	}

error:
	log_message(NULL, 0, "ERROR: Unexpected \"%s\" in config file on line %d.\n",
			parser->buffer, parser->line);
	return false;
}

bool parse_config_file (struct Lava_data *data)
{
	errno = 0;
	struct Lava_parser parser = {
		.data    = data,
		.file    = NULL,
		.line    = 1,
		.column  = 0,
		.context = CONTEXT_NONE,
		.action  = ACTION_NONE
	};
	if ( NULL == (parser.file = fopen(data->config_path, "r")) )
	{
		log_message(NULL, 0, "ERROR: Can not open config file \"%s\".\n"
				"ERROR: fopen: %s\n",
				data->config_path, strerror(errno));
		return false;
	}

	bool ret = true;
	for(char ch;;)
	{
		if (! parser_get_char(&parser, &ch))
		{
			ret = false;
			goto exit;
		}

		/* We have to check both the current read char as well as the
		 * last one, as parser_ignore_line() can encounter an EOF but
		 * does not handle it.
		 */
		if ( parser.last_char == '\0' || ch == '\0' )
		{
			ret = parser_handle_eof(&parser);
			goto exit;
		}
		else if ( ch == '#' )
			ret = parser_ignore_line(&parser);
		else if (isspace(ch))
			continue;
		else if ( ch == '{' || ch == '}' )
			ret = parser_handle_bracket(&parser, ch);
		else if ( ch == ';' )
			ret = parser_handle_semicolon(&parser, ch);
		else
			ret = parser_handle_string(&parser, ch);

		/* If an error occurred, exit. */
		if (! ret)
			goto exit;
	}

exit:
	fclose(parser.file);
	return ret;
}
