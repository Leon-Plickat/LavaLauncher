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
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include"lavalauncher.h"
#include"log.h"
#include"output.h"
#include"toplevel.h"
#include"item/command.h"
#include"item/item.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"

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

static void fork_command (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno   = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		errno = 0;
		if ( setsid() == -1 )
		{
			log_message(NULL, 0, "ERROR: setsid: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		/* Output properties. */
		setenvf("LAVALAUNCHER_OUTPUT_NAME",  "%s", bar->output->name);
		setenvf("LAVALAUNCHER_OUTPUT_SCALE", "%d", bar->output->scale);

		/* Toplevel properties. */
		setenvf("LAVALAUNCHER_APPID_COUNT", "%d", count_toplevels_with_appid(bar->data, item->app_id));

		/* execl() only returns on error, on success it replaces this process. */
		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		log_message(NULL, 0, "ERROR: execl: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	else if ( ret < 0 )
		log_message(NULL, 0, "ERROR: fork: %s\n", strerror(errno));
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

/* Wrapper function that catches any special cases before executing the item
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
		log_message(bar->data, 1, "[command] Exiting due to button command \"exit\".\n");
		bar->data->loop = false;
	}
	else if (! strcmp(*cmd, "reload"))
	{
		log_message(bar->data, 1, "[command] Reloading due to button command \"reload\".\n");
		bar->data->loop = false;
		bar->data->reload = true;
	}
	else
	{
		log_message(bar->data, 1, "[command] Executing command: %s\n", *cmd);
		fork_command(bar, item, *cmd);
	}

	return true;
}
