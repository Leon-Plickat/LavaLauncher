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

#define VERSION "1.7.1"

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<poll.h>
#include<errno.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"config/config.h"
#include"registry.h"
#include"item/item.h"

static const char usage[] = "LavaLauncher -- Version "VERSION"\n\n"
                            "Usage: lavalauncher [options...]\n"
			    "  -c <path> Path to config file.\n"
			    "  -h        Print this help text.\n"
			    "  -v        Enable verbose output.\n\n"
			    "The configuration syntax is documented in the man page.\n";

static bool handle_command_flags (struct Lava_data *data, int argc, char *argv[])
{
	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "c:hv")) != -1 ;)
		switch (c)
		{
			case 'c':
				data->config_path = optarg;
				break;

			case 'h':
				fputs(usage, stderr);
				data->ret = EXIT_SUCCESS;
				return false;

			case 'v':
				data->verbose = true;
				break;

			default:
				return false;
		}

	if ( data->config_path == NULL)
	{
		fputs("You need to provide the path to a config file.\n", stderr);
		return false;
	}

	return true;
}

static void init_data (struct Lava_data *data)
{
	data->ret         = EXIT_FAILURE;
	data->loop        = true;
	data->verbose     = false;
	data->config_path = NULL;
	wl_list_init(&data->items);
	wl_list_init(&data->outputs);
	wl_list_init(&data->seats);
}

int main (int argc, char *argv[])
{
	struct Lava_data data;
	init_data(&data);

	if (! handle_command_flags(&data, argc, argv))
		return data.ret;
	if (! init_config(&data))
		goto early_exit;
	if (! init_wayland(&data))
		goto exit;
	if (! init_cursor(&data))
		fputs("WARNING: Changing the cursor is disabled due to an error.\n", stderr);

	/* Prevent zombies. */
	signal(SIGCHLD, SIG_IGN);

	data.ret = EXIT_SUCCESS;
	while ( data.loop && wl_display_dispatch(data.display)  != -1 );

exit:
	finish_cursor(&data);
	finish_wayland(&data);
early_exit:
	finish_config(&data);
	destroy_all_items(&data);
	return data.ret;
}
