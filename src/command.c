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
#include"button.h"

/* This function will search for a string (*srch) inside a given string (**str)
 * and replace it either with another string (*repl_s) or with a string-ified
 * integer (repl_i, when *repl_s == NULL).
 */
// This likely sucks...
static void replace_token (char **str, char *srch, char *repl_s, int repl_i, size_t size)
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

static void handle_tokens (struct Lava_data *data, struct Lava_output *output,
		struct Lava_button *button, char *buffer)
{
	replace_token(&buffer, "%index%",         NULL,                    button->index,       STRING_BUFFER_SIZE);
	replace_token(&buffer, "%buttons%",       NULL,                    data->button_amount, STRING_BUFFER_SIZE);
	replace_token(&buffer, "%icon-size%",     NULL,                    data->icon_size,     STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-top%",    NULL,                    data->border_top,    STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-left%",   NULL,                    data->border_left,   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-bottom%", NULL,                    data->border_bottom, STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-right%",  NULL,                    data->border_right,  STRING_BUFFER_SIZE);
	replace_token(&buffer, "%margin-top%",    NULL,                    data->margin_top,    STRING_BUFFER_SIZE);
	replace_token(&buffer, "%margin-right%",  NULL,                    data->margin_right,  STRING_BUFFER_SIZE);
	replace_token(&buffer, "%margin-bottom%", NULL,                    data->margin_bottom, STRING_BUFFER_SIZE);
	replace_token(&buffer, "%margin-left%",   NULL,                    data->margin_left,   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%colour%",        data->bar_colour_hex,    0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-colour%", data->border_colour_hex, 0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%output%",        output->name,            0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%scale%",         NULL,                    output->scale,       STRING_BUFFER_SIZE);
}

static void exec_cmd (struct Lava_data *data, struct Lava_output *output,
		struct Lava_button *button)
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
		strncpy(buffer, button->cmd, STRING_BUFFER_SIZE);
		handle_tokens(data, output, button, buffer);

		if (data->verbose)
			fprintf(stderr, "Executing: cmd='%s' raw='%s'\n",
					buffer, button->cmd);

		/* execl() only returns on error. */
		errno = 0;
		execl("/bin/sh", "/bin/sh", "-c", buffer, NULL);
		fprintf(stderr, "ERROR: execl: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if ( ret < 0 )
		fprintf(stderr, "ERROR: fork: %s\n", strerror(errno));
}

/* Helper function that catches any special cases before executing the button
 * command.
 */
bool button_command (struct Lava_data *data, struct Lava_button *button,
		struct Lava_output *output)
{
	if ( button->cmd == NULL )
		return false;

	if (! strcmp(button->cmd, "exit"))
		data->loop = false;
	else
		exec_cmd(data, output, button);

	return true;
}
