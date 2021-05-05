/*
 * lib-infinitesimal - A small callback based INI tokenizer
 *
 * Copyright (c) 2021 Leon Henrik Plickat
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License or the
 * Gnu General Public License as published by the Free Software Foundation,
 * either version 3 of those Licenses, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<stdbool.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>

#include "lib-infinitesimal.h"

struct Infinitesimal_parser
{
	FILE *file;
	uint32_t line;
	char *buffer;
	uint32_t length;
	uint32_t buffer_size;

	void *user_data;

	/* User callbacks. */
	infinitesimal_section_callback_t       section_callback;
	infinitesimal_assign_callback_t        assign_callback;
	infinitesimal_error_message_callback_t error_message_callback;
};

static bool get_char (struct Infinitesimal_parser *parser, char *ch)
{
	errno = 0;
	*ch = (char)fgetc(parser->file);

	if ( *ch == EOF )
	{
		if ( errno != 0 )
		{
			parser->error_message_callback(parser->user_data, parser->line,
					strerror(errno));
			return false;
		}
		*ch = '\0';
	}
	return true;
}

static bool buffer_add_char (struct Infinitesimal_parser *parser, char ch)
{
	if ( parser->length >= parser->buffer_size - 2 )
	{
		if ( parser->buffer_size > (UINT32_MAX / 2) )
			goto error;

		parser->buffer_size *= 2;
		parser->buffer = realloc(parser->buffer, sizeof(char) * parser->buffer_size);

		if ( parser->buffer == NULL )
			goto error;
	}

	parser->buffer[parser->length] = ch;
	parser->length++;
	parser->buffer[parser->length] = '\0';

	return true;

error:
	parser->error_message_callback(parser->user_data, parser->line,
			"Buffer reallocation failed");
	return false;
}

/* Read a single line and parse it. Returns false when last line has been parsed
 * or error was encountered.
 */
static bool parse_line (struct Infinitesimal_parser *parser, bool *success)
{
	/* First we get the line. */
	char *message;
	parser->buffer[0] = '\0';
	parser->length = 0;
	bool no_eof = true;
	bool line_has_started = false;
	for (char ch;;)
	{
		if(! get_char(parser, &ch))
			goto error;

		if ( ch == '\n' )
		{
			if (line_has_started)
				break;
			goto success;
		}
		else if ( ch == '\0' )
		{
			no_eof = false;
			if (line_has_started)
				break;
			goto success;
		}
		if (isspace(ch))
		{
			/* Ignore any preceding white space. */
			if (line_has_started)
			{
				if(! buffer_add_char(parser, ch))
					goto error;
			}
		}
		else if ( ch == '#' )
		{
			/* Comment, so let's ignore the rest of the line. */
			while (true)
			{
				if (! get_char(parser, &ch))
					goto error;
				if ( ch == '\n' )
				{
					if (line_has_started)
						break;
					goto  success;
				}
				else if ( ch == '\0' )
				{
					no_eof = false;
					if (line_has_started)
						break;
					goto success;
				}
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
			if (! get_char(parser, &ch))
				goto error;
			if ( ch == '\\' )
			{
				if (! buffer_add_char(parser, '\\'))
					goto error;
			}
			else if ( ch == '\n' )
			{
				if (! buffer_add_char(parser, ' '))
					goto error;
				parser->line++;
			}
			else if ( ch == '#' )
			{
				if (! buffer_add_char(parser, '#'))
					goto error;
			}
			else
			{
				message = "Unknown escape sequence";
				goto error_message;
			}
			line_has_started = true;
		}
		else
		{
			if (! buffer_add_char(parser, ch))
				goto error;
			line_has_started = true;
		}
	}

	/* Trim trailing whitespace. */
	for (; isspace(parser->buffer[parser->length-1]); parser->length--);
	parser->buffer[parser->length] = '\0';

	/* Is the line a section header?. */
	if ( parser->buffer[0] == '[' )
	{
		if ( parser->buffer[parser->length-1] == ']' )
		{
			if (! parser->section_callback(parser->user_data, parser->line, parser->buffer))
				goto error;
			goto success;
		}
		else
		{
			message = "Section names misses closing bracket";
			goto error_message;
		}
	}

	/* The line is not a section header. If we can find '=' it may be an
         * assignment. If we don't find it, the line is a syntax error.
         */
	size_t equals = 0;
	message = "Line is neither section header nor assignment";
	while (true)
	{
		if ( parser->buffer[equals] == '=' )
			break;
		if ( parser->buffer[equals] == '\0' )
			goto error_message;
		if ( equals >= parser->buffer_size - 2 )
			goto error_message;
		else
			equals++;
	}

	/* Is there a variable name? Remember: In parse_line() we ignore
	 * preceding whitespace, so if there are no non-whitespace characters
	 * on the line before '=' it will be at beginning of the buffer.
	 */
	if ( equals == 0 )
	{
		message = "No variable name before '='";
		goto error_message;
	}

	/* Is there a variable vallue? Remember: In parse_line() we trim trailing
	 * whitespace, so if there are no non-whitespace characters on the line
	 * after '=' it will be at the end of the buffer.
	 */
	 if ( equals == parser->length - 1 )
	 {
		message = "No value after '='";
		goto error_message;
	 }

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
	 * whitespace. Let's hand them off to the user defined handler.
	 */
	if (! parser->assign_callback(parser->user_data, parser->line,
			parser->buffer, &parser->buffer[value]))
		goto error;

success:
	parser->line++;
	return no_eof;

error_message:
	parser->error_message_callback(parser->user_data, parser->line, message);
error:
	*success = false;
	return false;
}

bool infinitesimal_parse_file (FILE *file, void *user_data,
		infinitesimal_section_callback_t section_callback,
		infinitesimal_assign_callback_t assign_callback,
		infinitesimal_error_message_callback_t error_message_callback)
{
	struct Infinitesimal_parser parser = {
		.file      = file,
		.line      = 1,

		.buffer      = malloc(sizeof(char) * 1024),
		.buffer_size = 1024,
		.length      = 0,

		.user_data = user_data,

		.section_callback       = section_callback,
		.assign_callback        = assign_callback,
		.error_message_callback = error_message_callback
	};

	if ( parser.buffer == NULL )
	{
		parser.error_message_callback(parser.user_data, parser.line,
				"Failed to allocate buffer");
		return false;
	}

	bool success = true;
	while (parse_line(&parser, &success));
	if ( parser.buffer != NULL )
		free(parser.buffer);
	return success;
}

