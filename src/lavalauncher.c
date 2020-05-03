/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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

static const char usage[] = "LavaLauncher -- Version "VERSION"\n\n"
                            "Usage: lavalauncher [options...]\n"
                            "  -a <alignment>                    Alignment of the bar / icons.\n"
                            "  -b <path> <command>               Add a button.\n"
                            "  -c <colour>                       Background colour.\n"
                            "  -C <colour>                       Border colour.\n"
                            "  -d <size>                         Add a spacer.\n"
                            "  -e <mode>                         Exclusive zone.\n"
                            "  -h                                Display help text and exit.\n"
                            "  -l <layer>                        Layer of surface.\n"
                            "  -m <mode>                         Display mode of the bar.\n"
                            "  -M <top> <right> <bottom> <left>  Directional margins of the bar / icons.\n"
                            "  -o <name>                         Name of exclusive output.\n"
                            "  -p <position>                     Position of the bar.\n"
                            "  -P <cursor>                       Name of the cursor when hovering over the bar.\n"
                            "  -s <size>                         Icon size.\n"
                            "  -S <size>                         All border sizes.\n"
                            "  -S <top> <right> <bottom> <left>  Individual border sizes.\n"
                            "  -v                                Verbose output.\n";

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

/* getopts() only handles command flags with none or a single argument directly;
 * For multiple argument we have to do some of the work ourself. This function
 * counts the arguments to a command flag.
 */
static int count_arguments (int optind, int argc, char *argv[])
{
	int args = 0, index = optind - 1;
	while ( index < argc )
	{
		if ( *argv[index] == '-' )
			break;
		args++, index++;
	}
	return args;
}

static bool handle_command_flags (int argc, char *argv[], struct Lava_data *data)
{
	int arguments;
	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "a:b:c:C:d:e:hl:m:M:o:p:P:s:S:v")) != -1 ;)
	{
		switch (c)
		{
			case 'a': config_set_alignment(data, optarg); break;
			case 'b':
				if( count_arguments(optind, argc, argv) != 2 )
				{
					fputs("ERROR: '-b' expects 2 arguments.\n", stderr);
					data->ret = EXIT_FAILURE;
					break;
				}
				if (! add_button(data, argv[optind-1], argv[optind]))
					data->ret = EXIT_FAILURE;
				optind++; /* Tell getopt() to skip one argv field. */
				break;

			case 'c': config_set_bar_colour(data, optarg);    break;
			case 'C': config_set_border_colour(data, optarg); break;

			case 'd':
				if (! add_spacer(data, atoi(optarg)))
					data->ret = EXIT_FAILURE;
				break;

			case 'e': config_set_exclusive(data, optarg);     break;
			case 'h': fputs(usage, stderr);                   return EXIT_SUCCESS;
			case 'l': config_set_layer(data, optarg);         break;
			case 'm': config_set_mode(data, optarg);          break;

			case 'M':
				if( count_arguments(optind, argc, argv) != 4 )
				{
					fputs("ERROR: '-M' expects 4 arguments.\n", stderr);
					data->ret = EXIT_FAILURE;
					break;
				}
				config_set_margin(data,
						atoi(argv[optind-1]), atoi(argv[optind]),
						atoi(argv[optind+1]), atoi(argv[optind+2]));
				optind += 3; /* Tell getopt() to skip three argv field. */
				break;

			case 'o': config_set_only_output(data, optarg);   break;
			case 'p': config_set_position(data, optarg);      break;
			case 'P': config_set_cursor_name(data, optarg);   break;
			case 's': config_set_icon_size(data, optarg);     break;

			case 'S':
				arguments = count_arguments(optind, argc, argv);
				if ( arguments == 1 )
					config_set_border_size(data,
							atoi(optarg), atoi(optarg),
							atoi(optarg), atoi(optarg));
				else if ( arguments == 4 )
				{
					config_set_border_size(data,
							atoi(argv[optind-1]), atoi(argv[optind]),
							atoi(argv[optind+1]), atoi(argv[optind+2]));
					optind += 3; /* Tell getopt() to "skip" three argv fields. */
				}
				else
				{
					fputs("ERROR: '-S' expects 1 or 4 arguments.\n", stderr);
					data->ret = EXIT_FAILURE;
				}
				break;

			case 'v': data->verbose = true;    break;

			default:
				return false;
		}

		/* Check for errors during configuration. */
		if ( data->ret == EXIT_FAILURE )
		{
			destroy_buttons(data);
			return false;
		}
	}

	return true;
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {0};
	data.ret              = EXIT_SUCCESS;
	data.loop             = true;
	wl_list_init(&(data.buttons));

	/* Sensible configuration defaults. */
	sensible_defaults(&data);

	if (! handle_command_flags(argc, argv, &data) || ! init_buttons(&data) )
		return EXIT_FAILURE;

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

	if ( data.only_output != NULL )
		free(data.only_output);
	if ( data.cursor.name != NULL )
		free(data.cursor.name);

	destroy_buttons(&data);
	finish_cursor(&data);
	finish_wayland(&data);
	return data.ret;
}
