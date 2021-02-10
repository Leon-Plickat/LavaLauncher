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

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>

#include"lavalauncher.h"
#include"str.h"
#include"item.h"
#include"bar.h"

bool is_boolean_true (const char *str)
{
	return ( ! strcmp(str, "true") || ! strcmp(str, "yes") || ! strcmp(str, "on") || ! strcmp(str, "1") );
}

bool is_boolean_false (const char *str)
{
	return ( ! strcmp(str, "false") || ! strcmp(str, "no") || ! strcmp(str, "off") || ! strcmp(str, "0") );
}

bool set_boolean (bool *b, const char *value)
{
	if (is_boolean_true(value))
		*b = true;
	else if (is_boolean_false(value))
		*b = false;
	else
	{
		log_message(0, "ERROR: Not a boolean: %s\n", value);
		return false;
	}
	return true;
}

static bool global_set_watch (const char *arg)
{
#ifdef WATCH_CONFIG
	return set_boolean(&context.watch, arg);
#else
	log_message(0, "WARNING: LavaLauncher has been compiled without the ability to watch the configuration file for changes.\n");
	return true;
#endif
}

bool global_set_variable (const char *variable, const char *value, int line)
{
	struct
	{
		const char *variable;
		bool (*set)(const char*);
	} configs[] = {
		{ .variable = "watch-config-file", .set = global_set_watch }
	};

	FOR_ARRAY(configs, i) if (! strcmp(configs[i].variable, variable))
	{
		if (configs[i].set(value))
			return true;
		goto exit;
	}

	log_message(0, "ERROR: Unrecognized global setting \"%s\".\n", variable);
exit:
	log_message(0, "INFO: The error is on line %d in \"%s\".\n", line, context.config_path);
	return false;
}

enum Parser_state
{
	STATE_EXPECT_NAME_OR_CB, /* Name of variable or context or closing bracket. */
	STATE_EXPECT_OB,         /* Opening bracket. */
	STATE_EXPECT_EQUALS,
	STATE_EXPECT_VALUE,
	STATE_EXPECT_SEMICOLON
};

enum Parser_context
{
	CONTEXT_NONE,
	CONTEXT_GLOBAL_SETTINGS,
	CONTEXT_CONFIG,
	CONTEXT_BUTTON,
	CONTEXT_SPACER
};

struct Parser
{
	FILE *file;
	int line;

	enum Parser_state   state;
	enum Parser_context context;

	char name_buffer[1024];
	size_t name_buffer_length;
	char value_buffer[1024];
	size_t value_buffer_length;
};

static bool parser_get_char (struct Parser *parser, char *ch)
{
	errno = 0;
	*ch = (char)fgetc(parser->file);

	switch (*ch)
	{
		case EOF:
			if ( errno != 0 )
			{
				log_message(0, "ERROR: fgetc: %s\n", strerror(errno));
				return false;
			}
			*ch = '\0';
			break;

		case '\n':
			parser->line++;
			break;

		default:
			break;
	}

	return true;
}

static bool parser_handle_eof (struct Parser *parser)
{
	if ( parser->state == STATE_EXPECT_NAME_OR_CB && parser->context == CONTEXT_NONE )
		return true;
	log_message(0, "ERROR: Unexpectedly reached end of config file.\n");
	return false;
}

static bool parser_ignore_line (struct Parser *parser, bool *eof)
{
	*eof = false;
	for (char ch;;)
	{
		if (! parser_get_char(parser, &ch))
			return false;
		if ( ch == '\n' )
			return true;
		if ( ch == '\0' )
		{
			*eof = true;
			return true;
		}
	}
}

static bool parser_handle_bracket (struct Parser *parser, const char ch)
{
	if ( ch == '{' && parser->state == STATE_EXPECT_OB )
		parser->state = STATE_EXPECT_NAME_OR_CB;
	else if ( ch == '}' && parser->state == STATE_EXPECT_NAME_OR_CB )
	{
		if ( parser->context == CONTEXT_NONE )
			goto error;
		parser->context = CONTEXT_NONE;
	}
	else
		goto error;

	return true;

error:
	log_message(0, "ERROR: Unexpected '%c' in config file on line %d.\n", ch, parser->line);
	return false;
}

static bool parser_handle_equals (struct Parser *parser, const char ch)
{
	if ( parser->state != STATE_EXPECT_EQUALS )
	{
		log_message(0, "ERROR: Unexpected '=' in config file on line %d.\n", parser->line);
		return false;
	}
	parser->state = STATE_EXPECT_VALUE;
	return true;
}

static bool parser_handle_semicolon (struct Parser *parser, const char ch)
{
	if ( parser->state != STATE_EXPECT_SEMICOLON )
	{
		log_message(0, "ERROR: Unexpected ';' in config file on line %d.\n", parser->line);
		return false;
	}
	parser->state = STATE_EXPECT_NAME_OR_CB;
	return true;
}

static void buffer_add_char (char (*buffer)[1024], size_t *length, char ch)
{
	(*buffer)[*length] = ch;
	(*length)++;
	(*buffer)[*length] = '\0';
}

static bool parser_string_handle_escape (struct Parser *parser, char (*buffer)[1024], size_t *length)
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

	buffer_add_char(buffer, length, ch);

	return true;
}

static void buffer_trim_whitespace (char (*buffer)[1024], size_t length)
{
	size_t i = length;
	for (; isspace((*buffer)[i-1]); i--);
	(*buffer)[i] = '\0';
}

static bool parser_get_string (struct Parser *parser, char (*buffer)[1024], size_t *length,
		const char last_ch, bool single_word)
{
	bool quoted;
	if ( last_ch == '"' )
	{
		if (single_word)
			goto unexpected_quote;

		*length    = 0;
		*buffer[0] = '\0';
		quoted     = true;
	}
	else
	{
		*buffer[0] = last_ch;
		*buffer[1] = '\0';
		*length    = 1;
		quoted     = false;
	}

	for (char ch;;)
	{
		if ( *length >= sizeof(*buffer) - 1 )
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
				if (! isspace((*buffer)[*length-1]))
					buffer_add_char(buffer, length, ' ');
				continue;
			}
		}
		else if ( ch == '#' || ch == ';' )
		{
			if ( single_word || ! quoted )
				goto done_seek;
		}
		else if ( ch == '=' )
		{
			if ( single_word && ! quoted )
				goto done_seek;
		}
		else if ( ch == '"' )
		{
			if (quoted)
				goto done;
		}
		else if ( ch == '\\' )
		{
			if (! parser_string_handle_escape(parser, buffer, length))
					return false;
			continue;
		}

		buffer_add_char(buffer, length, ch);
	}

done_seek:
	fseek(parser->file, -1, SEEK_CUR);
done:
	(*buffer)[1024-1] = '\0';
	if (! quoted)
		buffer_trim_whitespace(buffer, *length);
	return true;

unterm:
	log_message(0, "ERROR: Unterminated string on line %d.\n", parser->line);
	return false;

overflow:
	log_message(0, "ERROR: Buffer overflow due to too long string on line %d.\n", parser->line);
	return false;

unexpected_quote:
	log_message(0, "ERROR: Unexpected quotes on line %d.\n", parser->line);
	return false;
}

static bool parser_handle_string (struct Parser *parser, const char ch)
{
	if ( parser->state == STATE_EXPECT_NAME_OR_CB )
	{
		if (! parser_get_string(parser, &parser->name_buffer,
					&parser->name_buffer_length, ch, true))
			return false;

		if ( parser->context == CONTEXT_NONE )
		{
			if (! strcmp(parser->name_buffer, "global-settings"))
			{
				parser->context = CONTEXT_GLOBAL_SETTINGS;
				parser->state = STATE_EXPECT_OB;
				return true;
			}
			if (! strcmp(parser->name_buffer, "config"))
			{
				parser->context = CONTEXT_CONFIG;
				parser->state = STATE_EXPECT_OB;
				return create_bar_config();
			}
			// TODO eventually just have a list for all item types
			//      and maybe CONTEXT_ITEM instead of individual contexts
			if (! strcmp(parser->name_buffer, "button"))
			{
				parser->context = CONTEXT_BUTTON;
				parser->state = STATE_EXPECT_OB;
				return create_item(TYPE_BUTTON);
			}
			else if (! strcmp(parser->name_buffer, "spacer"))
			{
				parser->context = CONTEXT_SPACER;
				parser->state = STATE_EXPECT_OB;
				return create_item(TYPE_SPACER);
			}
			else
			{
				log_message(0, "ERROR: Unexpected '%s' on line %d.\n", parser->name_buffer, parser->line);
				return false;
			}
		}
		else
		{
			parser->state = STATE_EXPECT_EQUALS;
			return true;
		}
	}
	else if ( parser->state == STATE_EXPECT_VALUE )
	{
		if (! parser_get_string(parser, &parser->value_buffer,
					&parser->value_buffer_length, ch, false))
			return false;

		parser->state = STATE_EXPECT_SEMICOLON;
		switch (parser->context)
		{
			case CONTEXT_GLOBAL_SETTINGS:
				return global_set_variable(parser->name_buffer,
						parser->value_buffer, parser->line);

			case CONTEXT_CONFIG:
				/* Change settings of latest configuration set. */
				return bar_config_set_variable(context.last_config,
						parser->name_buffer, parser->value_buffer,
						parser->line);

			case CONTEXT_BUTTON:
			case CONTEXT_SPACER:
				return item_set_variable(context.last_item,
						parser->name_buffer, parser->value_buffer,
						parser->line);

			default:
				return false;
		}
	}

	log_message(0, "ERROR: Incorrect assignment on line %d.\n", parser->line);
	return false;
}

bool parse_config_file (void)
{
	errno = 0;
	struct Parser parser = {
		.file    = NULL,
		.line    = 1,
		.context = CONTEXT_NONE,
		.state   = STATE_EXPECT_NAME_OR_CB
	};
	if ( NULL == (parser.file = fopen(context.config_path, "r")) )
	{
		log_message(0, "ERROR: Can not open config file \"%s\".\n"
				"ERROR: fopen: %s\n",
				context.config_path, strerror(errno));
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

		if ( ch == '\0' )
		{
			ret = parser_handle_eof(&parser);
			goto exit;
		}
		else if ( ch == '#')
		{
			bool eof;
			ret = parser_ignore_line(&parser, &eof);
			if ( eof && ret )
			{
				ret = parser_handle_eof(&parser);
				goto exit;
			}
		}
		else if (isspace(ch))
			continue;
		else if ( ch == '{' || ch == '}' )
			ret = parser_handle_bracket(&parser, ch);
		else if ( ch == '=' )
			ret = parser_handle_equals(&parser, ch);
		else if ( ch == ';' )
			ret = parser_handle_semicolon(&parser, ch);
		else
			ret = parser_handle_string(&parser, ch);

		if (! ret)
			goto exit;
	}

exit:
	fclose(parser.file);
	return ret;
}

