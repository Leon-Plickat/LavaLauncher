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
#define STRING_BUFFER_SIZE 4096

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include"lavalauncher.h"
#include"output.h"
#include"command.h"
#include"item/item.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"

/* This function will search for a string (*srch) inside a given string (**str)
 * and replace it either with another string (*repl_s) or with a string-ified
 * integer (repl_i, when *repl_s == NULL).
 */
// This likely sucks...
static void replace_token (char **str, const char *srch, const char *repl_s,
		const int repl_i, size_t size)
{
	char  buffer[STRING_BUFFER_SIZE+1]; /* Local editing buffer. */
	char *p;                            /* Pointer to beginning of *srch. */

	/* Iterate over all occurrences of *srch */
	while ((p = strstr(*str, srch)))
	{
		/* Copy str to buffer, but only until p. */
		strncpy(buffer, *str, p - *str);

		/* Insert replacement and rest of string. */
		if ( repl_s == NULL )
			sprintf(buffer + (p - *str), "%d%s", repl_i, p + strlen(srch));
		else
			sprintf(buffer + (p - *str), "%s%s", repl_s, p + strlen(srch));

		/* Copy buffer back to str. */
		strncpy(*str, buffer, size+1);
	}
}

static void handle_tokens (struct Lava_bar *bar, struct Lava_item *item, char *buffer)
{
	struct Lava_output      *output  = bar->output;
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct
	{
		const char *token;
		const char *replacement_string;
		const int   replacement_int;
	} tokens[] = {
		{ .token = "%index%",         .replacement_string = NULL,         .replacement_int = item->index,            },
		{ .token = "%items%",         .replacement_string = NULL,         .replacement_int = pattern->item_amount,   },
		{ .token = "%icon-size%",     .replacement_string = NULL,         .replacement_int = pattern->icon_size,     },
		{ .token = "%border-top%",    .replacement_string = NULL,         .replacement_int = pattern->border_top,    },
		{ .token = "%border-left%",   .replacement_string = NULL,         .replacement_int = pattern->border_left,   },
		{ .token = "%border-bottom%", .replacement_string = NULL,         .replacement_int = pattern->border_bottom, },
		{ .token = "%border-right%",  .replacement_string = NULL,         .replacement_int = pattern->border_right,  },
		{ .token = "%margin-top%",    .replacement_string = NULL,         .replacement_int = pattern->margin_top,    },
		{ .token = "%margin-right%",  .replacement_string = NULL,         .replacement_int = pattern->margin_right,  },
		{ .token = "%margin-bottom%", .replacement_string = NULL,         .replacement_int = pattern->margin_bottom, },
		{ .token = "%margin-left%",   .replacement_string = NULL,         .replacement_int = pattern->margin_left,   },
		{ .token = "%output%",        .replacement_string = output->name, .replacement_int = 0,                      },
		{ .token = "%scale%",         .replacement_string = NULL,         .replacement_int = output->scale,          }
	};
	for (size_t i = 0; i < (sizeof(tokens) / sizeof(tokens[0])); i++)
		replace_token(&buffer, tokens[i].token,
				tokens[i].replacement_string,
				tokens[i].replacement_int, STRING_BUFFER_SIZE);
}

static void exec_cmd (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno   = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		errno = 0;
		if ( setsid() == -1 )
		{
			fprintf(stderr, "ERROR: setsid: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		char buffer[STRING_BUFFER_SIZE+1];
		strncpy(buffer, cmd, STRING_BUFFER_SIZE);
		handle_tokens(bar, item, buffer);

		if (bar->data->verbose)
			fprintf(stderr, "Executing: cmd='%s' raw='%s'\n",
					buffer, cmd);

		/* execl() only returns on error. */
		errno = 0;
		execl("/bin/sh", "/bin/sh", "-c", buffer, NULL);
		fprintf(stderr, "ERROR: execl: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if ( ret < 0 )
		fprintf(stderr, "ERROR: fork: %s\n", strerror(errno));
}

static char **command_pointer_by_type(struct Lava_item *item,
		enum Interaction_type type)
{
	switch (type)
	{
		case TYPE_LEFT_CLICK:   return &item->left_click_command;
		case TYPE_MIDDLE_CLICK: return &item->middle_click_command;
		case TYPE_RIGHT_CLICK:  return &item->right_click_command;
		case TYPE_SCROLL_UP:    return &item->scroll_up_command;
		case TYPE_SCROLL_DOWN:  return &item->scroll_down_command;
		case TYPE_TOUCH:        return &item->touch_command;
	}

	return NULL;
}

/* Helper function that catches any special cases before executing the item
 * command.
 */
bool item_command (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type)
{
	char **cmd = command_pointer_by_type(item, type);

	if ( cmd == NULL || *cmd == NULL )
		return false;

	if (! strcmp(*cmd, "exit"))
	{
		if (bar->pattern->data->verbose)
			fputs("Exiting due to button command \"exit\".\n", stderr);
		bar->data->loop = false;
	}
	else if (! strcmp(*cmd, "reload"))
	{
		if (bar->pattern->data->verbose)
			fputs("Reloding due to button command \"reload\".\n", stderr);
		bar->data->loop = false;
		bar->data->reload = true;
	}
	else
		exec_cmd(bar, item, *cmd);

	return true;
}
