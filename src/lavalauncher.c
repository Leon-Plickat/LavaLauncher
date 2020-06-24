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
#include<sys/inotify.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"registry.h"
#include"config/parser.h"
#include"bar/bar-pattern.h"

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
	if (! data->watch)
		goto simple_loop;

	struct pollfd fds[2];

	/* Wayland fd. */
	fds[0].events = POLLIN;
	if ( -1 ==  (fds[0].fd = wl_display_get_fd(data->display)) )
	{
		fputs("ERROR: Unable to open Wayland display fd.\n", stderr);
		goto error;
	}

	/* Inotify fd to watch config file for modifications. */
	fds[1].events = POLLIN;
	if ( -1 ==  (fds[1].fd = inotify_init1(IN_NONBLOCK)) )
	{
		fputs("ERROR: Unable to open inotify fd.\n", stderr);
		perror("inotify_init1");
		goto error;
	}

	/* Add config file to inotify watch list. */
	if ( -1 == inotify_add_watch(fds[1].fd, data->config_path, IN_MODIFY) )
	{
		fputs("ERROR: Unable to add config path to inotify watchlist.\n", stderr);
		goto error;
	}

	while (data->loop)
	{
		errno = 0;

		/* Flush Wayland events. */
		do {
			if ( wl_display_flush(data->display) == 1 && errno != EAGAIN )
			{
				fprintf(stderr, "ERROR: wl_display_flush: %s\n",
						strerror(errno));
				break;
			}
		} while ( errno == EAGAIN );

		if ( poll(fds, 2, -1) < 0 )
		{
			if ( errno == EINTR )
				continue;
			fprintf(stderr, "poll: %s\n", strerror(errno));
			goto error;
		}

		/* Wayland events. */
		if ( fds[0].revents & POLLIN && wl_display_dispatch(data->display) == -1 )
		{
			fprintf(stderr, "ERROR: wl_display_flush: %s\n",
					strerror(errno));
			goto error;
		}
		if ( fds[0].revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			fprintf(stderr, "ERROR: wl_display_flush: %s\n",
					strerror(errno));
			goto error;
		}

		/* Inotify events. */
		if ( fds[1].revents & POLLIN )
		{
			if (data->verbose)
				fputs("Config file modified; Triggering reload.\n", stderr);
			data->loop = false;
			data->reload = true;
			goto exit;
		}
	}

error:
	data->ret = EXIT_FAILURE;
exit:
	if ( fds[0].fd != -1 )
		close(fds[0].fd);
	if ( fds[1].fd != -1 )
		close(fds[1].fd);
	return;
	
	/* Simple, Wayland-only loop, used when the configuration file is not
	 * watched. LavaLauncher will not reload on changes.
	 */
simple_loop:
	while ( data->loop && wl_display_dispatch(data->display) != -1 );
}

static bool handle_command_flags (struct Lava_data *data, int argc, char *argv[])
{
	extern int optind;
	optind = 0;
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
	data->ret          = EXIT_FAILURE;
	data->loop         = true;
	data->watch        = false;
	data->reload       = false;
	data->verbose      = false;
	data->config_path  = NULL;
	data->last_pattern = NULL;

	data->display            = NULL;
	data->registry           = NULL;
	data->compositor         = NULL;
	data->subcompositor      = NULL;
	data->shm                = NULL;
	data->layer_shell        = NULL;
	data->xdg_output_manager = NULL;

	wl_list_init(&data->patterns);
	wl_list_init(&data->outputs);
	wl_list_init(&data->seats);
}

int main (int argc, char *argv[])
{
	struct Lava_data data;
reload:
	init_data(&data);

	if (! handle_command_flags(&data, argc, argv))
		return data.ret;
	if (data.verbose)
		fputs("LavaLauncher " VERSION"\n", stderr);
	if (! parse_config_file(&data))
		goto early_exit;
	if (! init_wayland(&data))
		goto exit;

	/* Prevent zombies. */
	signal(SIGCHLD, SIG_IGN);

	data.ret = EXIT_SUCCESS;
	main_loop(&data);

exit:
	finish_wayland(&data);
early_exit:
	destroy_all_bar_patterns(&data);
	if (data.reload)
		goto reload;
	return data.ret;
}
