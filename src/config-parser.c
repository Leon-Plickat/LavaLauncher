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
#include"util.h"
#include"item.h"
#include"bar.h"

#define BUFFER_SIZE 1024

/*********************
 *                   *
 *  global settings  *
 *                   *
 *********************/
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

/************
 *          *
 *  Parser  *
 *          *
 ************/
enum Parser_section
{
	SECTION_NONE,
	SECTION_GLOBAL_SETTINGS,
	SECTION_CONFIG,
	SECTION_ITEM
};

struct Parser
{
	FILE *file;
	int line;
	enum Parser_section section;
	char buffer[BUFFER_SIZE];
	size_t length;
};

static bool enter_section (struct Parser *parser)
{
	if (! strcmp(parser->buffer, "[global-settings]"))
	{
		parser->section = SECTION_GLOBAL_SETTINGS;
		return true;
	}
	if (! strcmp(parser->buffer, "[config]"))
	{
		parser->section = SECTION_CONFIG;
		return create_bar_config();
	}
	if (! strcmp(parser->buffer, "[item:button]"))
	{
		parser->section = SECTION_ITEM;
		return create_item(TYPE_BUTTON);
	}
	else if (! strcmp(parser->buffer, "[item:spacer]"))
	{
		parser->section = SECTION_ITEM;
		return create_item(TYPE_SPACER);
	}

	log_message(0, "ERROR: Invalid section '%s' on line %d in \"%s\".\n",
		parser->buffer, parser->line, context.config_path);
	return false;
}

static bool assignment (struct Parser *parser)
{
	/* Find '='. Careful: It may not exist. */
	size_t equals = 0;
	while (true)
	{
		if ( parser->buffer[equals] == '=' )
			break;
		if ( parser->buffer[equals] == '\0' )
			goto error;
		if ( equals >= BUFFER_SIZE - 2 )
			goto error;
		else
			equals++;
	}

	/* Is there a variable name? Remember: In parse_line() we ignore
	 * preceding whitespace, so if there are no non-whitespace characters
	 * on the line before '=' it will be at beginning of the buffer.
	 */
	if ( equals == 0 )
		goto error;

	/* Is there a variable vallue? Remember: In parse_line() we trim trailing
	 * whitespace, so if there are no non-whitespace characters on the line
	 * after '=' it will be at the end of the buffer.
	 */
	 if ( equals == parser->length - 1 )
		 goto error;

	/* When this point is reached, we know for sure there are non-whitespace
	 * characters between the beginning of the buffer and '='. Let's trim
	 * the whitespace between them and '='.
	 */
	size_t i = equals;
	for (; isspace(parser->buffer[i-1]); i--);
	parser->buffer[i] = '\0';

	/* If this point if reached, we know for sure there are non-whitespace
	 * characters between '=' and the end of the buffer. Let's find them.
	 */
	size_t value = equals + 1;
	for (; isspace(parser->buffer[value]); value++);

	/* Now finally we have confirmed that both variable namd and variable
	 * value exist, know where they are and have cleaned them from any pesky
	 * whitespace. Let's use 'em!
	 */
	if ( parser->section == SECTION_NONE )
	{
		log_message(0, "ERROR: Assignment outside of a section on line %d in \"%s\".\n",
				parser->line, context.config_path);
		return false;
	}
	else if ( parser->section == SECTION_GLOBAL_SETTINGS )
		return global_set_variable(parser->buffer, &parser->buffer[value], parser->line);
	else if ( parser->section == SECTION_CONFIG )
		return bar_config_set_variable(context.last_config,
				parser->buffer, &parser->buffer[value], parser->line);
	else if ( parser->section == SECTION_ITEM )
		return item_set_variable(context.last_item,
				parser->buffer, &parser->buffer[value], parser->line);

error:
	/* We can not output parser->buffer, because when this error message is
	 * displayed, we may already have modified it.
	 */
	log_message(0, "ERROR: Malformed assignment on line %d in \"%s\".\n",
			parser->line, context.config_path);
	return false;
}

static bool get_char (struct Parser *parser, char *ch)
{
	errno = 0;
	*ch = (char)fgetc(parser->file);

	if ( *ch == EOF )
	{
		if ( errno != 0 )
		{
			log_message(0, "ERROR: fgetc: %s\n", strerror(errno));
			return false;
		}
		*ch = '\0';
	}
	return true;
}

static bool buffer_add_char (struct Parser *parser, char ch)
{
	if ( parser->length >= BUFFER_SIZE - 2 )
	{
		log_message(0, "ERROR: Buffer overflow due to too long string on line %d "
				"in \"%s\"\n", parser->line, context.config_path);
		return false;
	}

	parser->buffer[parser->length] = ch;
	parser->length++;
	parser->buffer[parser->length] = '\0';

	return true;
}

/* Helper macro to reduce error handling boiler plate code. */
#define TRY(A) \
	{ \
		if (! A) \
		{ \
			*ret = false; \
			return false; \
		} \
	}

/* These macros deduplicate code and make following the control flow a bit simpler. */
#define HANDLE_NEWLINE \
	{ \
		if (line_has_started) \
			break; \
		else \
		{ \
			parser->line++; \
			return true; \
		} \
	}
#define HANDLE_EOF \
	{ \
		if (line_has_started) \
		{ \
			no_eof = false; \
			break; \
		} \
		else \
			return false; \
	}

/* Read a single line and parse it. Returns false when last line has been parsed
 * or error was encountered.
 */
static bool parse_line (struct Parser *parser, bool *ret)
{
	/* First we get the line. */
	parser->buffer[0] = '\0';
	parser->length = 0;
	bool no_eof = true;
	bool line_has_started = false;
	for (char ch;;)
	{
		TRY(get_char(parser, &ch))

		if ( ch == '\n' )
			HANDLE_NEWLINE
		else if ( ch == '\0' )
			HANDLE_EOF
		if (isspace(ch))
		{
			/* Ignore any preceding white space. */
			if (line_has_started)
				TRY(buffer_add_char(parser, ch))
		}
		else if ( ch == '#' )
		{
			/* Comment, so let's ignore the rest of the line. */
			while (true)
			{
				TRY(get_char(parser, &ch))
				if ( ch == '\n' )
					HANDLE_NEWLINE
				else if ( ch == '\0' )
					HANDLE_EOF
			}
			break;
		}
		else if ( ch == '\\' )
		{
			/* Ah, an escape sequence, how quaint. Note that we only
			 * care about the escape sequences that are relevant to
			 * parsing the configuration, meaning escaping a comment
			 * or newline.
			 */
			TRY(get_char(parser, &ch))
			if ( ch == '\\' )
				TRY(buffer_add_char(parser, '\\'))
			else if ( ch == '\n' )
			{
				TRY(buffer_add_char(parser, ' '))
				parser->line++;
			}
			else if ( ch == '#' )
				TRY(buffer_add_char(parser, '#'))
			else
			{
				log_message(0, "ERROR: Unknown escape sequence '\\%c' "
						"on line %d in \"%s\"\n",
						ch, parser->line, context.config_path);
				*ret = false;
				return false;
			}
			line_has_started = true;
		}
		else
		{
			TRY(buffer_add_char(parser, ch))
			line_has_started = true;
		}
	}

	/* Trim trailing whitespace. */
	for (; isspace(parser->buffer[parser->length-1]); parser->length--);
	parser->buffer[parser->length] = '\0';

	/* Then we parse the line. */
	if ( parser->buffer[0] == '[' )
		TRY(enter_section(parser))
	else
		TRY(assignment(parser))

	parser->line++;
	return no_eof;
}

bool parse_config_file (void)
{
	errno = 0;
	struct Parser parser = {
		.file    = NULL,
		.line    = 1,
		.section = SECTION_NONE,
	};

	if ( NULL == (parser.file = fopen(context.config_path, "r")) )
	{
		log_message(0, "ERROR: Can not open config file \"%s\".\n"
				"ERROR: fopen: %s\n",
				context.config_path, strerror(errno));
		return false;
	}

	bool ret = true;
	while (parse_line(&parser, &ret));
	if ( ret == true && parser.section == SECTION_NONE )
	{
		log_message(0, "ERROR: Configuration file is void of any meaningful content.\n");
		ret = false;
	}
	fclose(parser.file);
	return ret;
}

