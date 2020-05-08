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
#include"config.h"
#include"registry.h"
#include"cursor.h"
#include"button.h"
#include"parser.h"

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

/* Calculate the dimensions of the visible part of the bar. */
static void calculate_dimensions (struct Lava_data *data)
{
	switch (data->position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			data->orientation = ORIENTATION_VERTICAL;
			data->w = (uint32_t)(data->icon_size + data->border_right
					+ data->border_left);
			data->h = (uint32_t)(get_button_length_sum(data)
					+ data->border_top + data->border_bottom);
			if ( data->exclusive_zone == 1 )
				data->exclusive_zone = data->w;
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			data->orientation = ORIENTATION_HORIZONTAL;
			data->w = (uint32_t)(get_button_length_sum(data)
					+ data->border_left + data->border_right);
			data->h = (uint32_t)(data->icon_size + data->border_top
					+ data->border_bottom);
			if ( data->exclusive_zone == 1 )
				data->exclusive_zone = data->h;
			break;
	}
}

int main (int argc, char *argv[])
{
	/* This struct holds all the global data. */
	struct Lava_data data = {
		.ret  = EXIT_SUCCESS,
		.loop = true
	};
	char *config_path = NULL;

	wl_list_init(&(data.buttons));
	wl_list_init(&(data.outputs));
	wl_list_init(&(data.seats));

	/* Sensible configuration defaults. */
	sensible_defaults(&data);

	/* Parse command flags. */
	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "c:hv")) != -1 ;)
		switch (c)
		{
			case 'c':
				if ( config_path != NULL )
					free(config_path);
				config_path = strdup(optarg);
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

	/* There is no default configuration file. */
	if ( config_path == NULL)
	{
		fputs("You need to provide the path to a config file.\n", stderr);
		return EXIT_FAILURE;
	}

	if (! parse_config(&data, config_path) || ! init_buttons(&data) )
		goto exit;

	calculate_dimensions(&data);

	if (data.verbose)
		fprintf(stderr, "LavaLauncher Version "VERSION"\n"
				"Bar: w=%d h=%d buttons=%d\n",
				data.w, data.h, data.button_amount);

	/* Prevent zombies. */
	signal(SIGCHLD, SIG_IGN);

	if ( init_wayland(&data) && init_cursor(&data) )
		main_loop(&data);
	else
		data.ret = EXIT_FAILURE;

exit:
	if ( data.only_output != NULL )
		free(data.only_output);
	if ( data.cursor.name != NULL )
		free(data.cursor.name);
	destroy_buttons(&data);
	finish_cursor(&data);
	finish_wayland(&data);
	return data.ret;
}
