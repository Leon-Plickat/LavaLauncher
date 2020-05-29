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
#include"items/item.h"

static const char usage[] = "LavaLauncher -- Version "VERSION"\n\n"
                            "Usage: lavalauncher [options...]\n"
			    "  -c <path> Path to config file.\n"
			    "  -h        Print this help text.\n"
			    "  -v        Enable verbose output.\n\n"
			    "The configuration syntax is documented in the man page.\n";

static void main_loop (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Starting main loop.\n", stderr);

	struct pollfd fds = (struct pollfd) {
		.fd     = wl_display_get_fd(data->display),
		.events = POLLIN
	};

	while (data->loop)
	{
		errno = 0;

		/* Flush Wayland events. */
		do
		{
			if ( wl_display_flush(data->display) == -1  && errno != EAGAIN )
			{
				fprintf(stderr, "ERROR: wl_display_flush: %s\n",
						strerror(errno));
				break;
			}
		} while ( errno == EAGAIN );

		/* Poll, aka wait for event. */
		if ( poll(&fds, 1, -1) < 0 )
		{
			fprintf(stderr, "poll: %s\n", strerror(errno));
			break;
		}

		/* Handle event. */
		if ( fds.revents & POLLIN && wl_display_dispatch(data->display) == -1 )
		{
			fprintf(stderr, "ERROR: wl_display_dispatch: %s\n",
					strerror(errno));
			break;
		}
		if ( fds.revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			fprintf(stderr, "ERROR: wl_display_flush: %s\n",
					strerror(errno));
			break;
		}
	}
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {
		.ret     = EXIT_FAILURE,
		.loop    = true,
		.verbose = false
	};
	char *config_path = NULL;

	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "c:hv")) != -1 ;)
		switch (c)
		{
			case 'c':
				config_path = optarg;
				break;

			case 'h':
				fputs(usage, stderr);
				return EXIT_SUCCESS;

			case 'v':
				data.verbose = true;
				break;

			default:
				return EXIT_FAILURE;
		}

	if ( config_path == NULL)
	{
		fputs("You need to provide the path to a config file.\n", stderr);
		return EXIT_FAILURE;
	}

	wl_list_init(&(data.items));

	if ( NULL == (data.config = create_config(&data, config_path)) )
		goto early_exit;

	if (data.verbose)
		fprintf(stderr, "LavaLauncher Version "VERSION"\n"
				"Bar: w=%d h=%d items=%d\n",
				data.config->w, data.config->h, data.item_amount);

	/* Prevent zombies. */
	signal(SIGCHLD, SIG_IGN);

	if (init_wayland(&data))
	{
		data.ret = EXIT_SUCCESS;
		main_loop(&data);
	}

	finish_wayland(&data);
early_exit:
	destroy_config(data.config);
	destroy_all_items(&data);
	return data.ret;
}
