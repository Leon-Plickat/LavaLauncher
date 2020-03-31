#define _POSIX_C_SOURCE 200809L
#define STRING_BUFFER_SIZE 4096

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include"lavalauncher.h"
#include"command.h"

/* This function will search for a string (*srch) inside a given string (**str)
 * and replace it either with another string (*repl_s) or with a string-ified
 * integer (repl_i, when *repl_s == NULL).
 */
// This likely sucks...
static void replace_token (char **str, char *srch, char *repl_s, int repl_i, size_t size)
{
	char  buffer[STRING_BUFFER_SIZE]; /* Local editing buffer. */
	char *p;                          /* Pointer to beginning of *srch. */

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
		strncpy(*str, buffer, size);
	}
}

static void handle_tokens (struct Lava_data *data, struct Lava_output *output,
		char *buffer)
{
	replace_token(&buffer, "%buttons%",       NULL,                    data->button_amount, STRING_BUFFER_SIZE);
	replace_token(&buffer, "%icon-size%",     NULL,                    data->icon_size,     STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-top%",    NULL,                    data->border_top,    STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-left%",   NULL,                    data->border_left,   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-bottom%", NULL,                    data->border_bottom, STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-right%",  NULL,                    data->border_right,  STRING_BUFFER_SIZE);
	replace_token(&buffer, "%margin%",        NULL,                    data->margin,        STRING_BUFFER_SIZE);
	replace_token(&buffer, "%colour%",        data->bar_colour_hex,    0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%border-colour%", data->border_colour_hex, 0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%output%",        output->name,            0,                   STRING_BUFFER_SIZE);
	replace_token(&buffer, "%scale%",         NULL,                    output->scale,       STRING_BUFFER_SIZE);
}

static void exec_cmd (struct Lava_data *data, struct Lava_output *output, const char *cmd)
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

		char *buffer = malloc(STRING_BUFFER_SIZE);
		strncpy(buffer, cmd, STRING_BUFFER_SIZE);
		handle_tokens(data, output, buffer);

		if (data->verbose)
			fprintf(stderr, "Executing: cmd='%s' raw='%s'\n",
					buffer, cmd);

		// TODO Use something other than system()
		errno = 0;
		if ( system(buffer) == -1 && errno != ECHILD )
		{
			fprintf(stderr, "ERROR: system: %s\n", strerror(errno));
			free(buffer);
			exit(EXIT_FAILURE);
		}

		free(buffer);
		exit(EXIT_SUCCESS);
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
		exec_cmd(data, output, button->cmd);

	return true;
}
