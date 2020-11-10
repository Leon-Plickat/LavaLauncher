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

#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<sys/wait.h>

#include"lavalauncher.h"
#include"str.h"
#include"output.h"
#include"item/command.h"
#include"item/item.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"types/string-container.h"

static void setenvf (const char *name, const char *fmt, ...)
{
	char buffer[64];
	memset(buffer, '\0', sizeof(buffer));

	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
	va_end(args);

	setenv(name, buffer, true);
}

static void prepare_env (struct Lava_bar *bar)
{
		setenvf("LAVALAUNCHER_OUTPUT_NAME",  "%s", bar->output->name);
		setenvf("LAVALAUNCHER_OUTPUT_SCALE", "%d", bar->output->scale);
}

static void exec_cmd (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	prepare_env(bar);

	/* execl() only returns on error, on success it replaces this process. */
	execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
	log_message(NULL, 0, "ERROR: execl: %s\n", strerror(errno));
}

/* We need to fork two times for UNIXy resons. */
static void secondary_fork (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		exec_cmd(bar, item, cmd);
		_exit(EXIT_FAILURE);
	}
	else if ( ret < 0 )
	{
		log_message(NULL, 0, "ERROR: fork: %s\n", strerror(errno));
		_exit(EXIT_FAILURE);
	}
}

/* We need to fork two times for UNIXy resons. */
static void primary_fork (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		setsid();

		/* Restore signals. */
		sigset_t mask;
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, NULL);

		secondary_fork(bar, item, cmd);
		_exit(EXIT_SUCCESS);
	}
	else if ( ret < 0 )
		log_message(NULL, 0, "ERROR: fork: %s\n", strerror(errno));
	else
		waitpid(ret, NULL, 0);
}

/* Wrapper function that catches any special cases before executing the item command. */
bool item_command (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type)
{
	if ( item->command[type] == NULL )
		return false;

	if (! strcmp(item->command[type]->string, "exit"))
	{
		log_message(bar->data, 1, "[command] Exiting due to button command \"exit\".\n");
		bar->data->loop = false;
	}
	else if (! strcmp(item->command[type]->string, "reload"))
	{
		log_message(bar->data, 1, "[command] Reloading due to button command \"reload\".\n");
		bar->data->loop = false;
		bar->data->reload = true;
	}
	else
	{
		log_message(bar->data, 1, "[command] Executing command: %s\n",
				item->command[type]->string);
		primary_fork(bar, item, item->command[type]->string);
	}

	return true;
}

