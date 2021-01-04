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

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<poll.h>
#include<errno.h>
#include<getopt.h>

#if WATCH_CONFIG
#include<sys/inotify.h>
#endif

#if HANDLE_SIGNALS
#include<sys/signalfd.h>
#include<signal.h>
#endif

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"str.h"
#include"registry.h"
#include"config.h"
#include"bar.h"
#include"str.h"

static void main_loop (struct Lava_data *data)
{
	log_message(data, 1, "[main] Starting main loop.\n");

	nfds_t fd_count = 1;
	size_t current = 0, wayland_fd = current++;

#if HANDLE_SIGNALS
	fd_count++;
	size_t signal_fd = current++;
#endif

#if WATCH_CONFIG
	size_t inotify_fd = current++;
	if (data->watch)
		fd_count++;
#endif

	struct pollfd *fds = calloc(fd_count, sizeof(struct pollfd));

	/* Wayland fd. */
	fds[wayland_fd].events = POLLIN;
	if ( -1 ==  (fds[wayland_fd].fd = wl_display_get_fd(data->display)) )
	{
		log_message(NULL, 0, "ERROR: Unable to open Wayland display fd.\n");
		goto error;
	}

#if HANDLE_SIGNALS
	/* Signal fd. */
	sigset_t mask;
	struct signalfd_siginfo fdsi;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);

	if ( sigprocmask(SIG_BLOCK, &mask, NULL) == -1 )
	{
		log_message(NULL, 0, "ERROR: sigprocmask() failed.\n");
		goto error;
	}

	fds[signal_fd].events = POLLIN;
	if ( -1 ==  (fds[signal_fd].fd = signalfd(-1, &mask, 0)) )
	{
		log_message(NULL, 0, "ERROR: Unable to open signal fd.\n"
				"ERROR: signalfd: %s\n", strerror(errno));
		goto error;
	}
#endif

#if WATCH_CONFIG
	/* Inotify fd . */
	if (data->watch)
	{
		fds[inotify_fd].events = POLLIN;
		if ( -1 ==  (fds[inotify_fd].fd = inotify_init1(IN_NONBLOCK)) )
		{
			log_message(NULL, 0, "ERROR: Unable to open inotify fd.\n"
					"ERROR: inotify_init1: %s\n", strerror(errno));
			goto error;
		}

		/* Add config file to inotify watch list. */
		if ( -1 == inotify_add_watch(fds[inotify_fd].fd, data->config_path, IN_MODIFY) )
		{
			log_message(NULL, 0, "ERROR: Unable to add config path to inotify watchlist.\n");
			goto error;
		}
	}
#endif

	while (data->loop)
	{
		errno = 0;

		/* Flush Wayland events. */
		do {
			if ( wl_display_flush(data->display) == 1 && errno != EAGAIN )
			{
				log_message(NULL, 0, "ERROR: wl_display_flush: %s\n",
						strerror(errno));
				break;
			}
		} while ( errno == EAGAIN );

		/* Polling. */
		if ( poll(fds, fd_count, -1) < 0 )
		{
			if ( errno == EINTR )
				continue;
			log_message(NULL, 0, "poll: %s\n", strerror(errno));
			goto error;
		}

		/* Wayland events. */
		if ( fds[wayland_fd].revents & POLLIN && wl_display_dispatch(data->display) == -1 )
		{
			log_message(NULL, 0, "ERROR: wl_display_flush: %s\n", strerror(errno));
			goto error;
		}
		if ( fds[wayland_fd].revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			log_message(NULL, 0, "ERROR: wl_display_flush: %s\n", strerror(errno));
			goto error;
		}

#if HANDLE_SIGNALS
		/* Signal events. */
		if ( fds[signal_fd].revents & POLLIN )
		{
			if ( read(fds[signal_fd].fd, &fdsi, sizeof(struct signalfd_siginfo))
					!= sizeof(struct signalfd_siginfo) )
			{
				log_message(NULL, 0, "ERROR: Can not read signal info.\n");
				goto error;
			}

			if ( fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT )
			{
				log_message(data, 1, "[loop] Received SIGTERM or SIGQUIT; Exiting.\n");
				goto exit;
			}
			else if ( fdsi.ssi_signo == SIGUSR1 || fdsi.ssi_signo == SIGUSR2 )
			{
				log_message(data, 1, "[loop] Received SIGUSR; Triggering reload.\n");
				data->loop = false;
				data->reload = true;
				goto exit;
			}
		}
#endif

#if WATCH_CONFIG
		/* Inotify events. */
		if ( fds[inotify_fd].revents & POLLIN )
		{
			log_message(data, 1, "[main] Config file modified; Triggering reload.\n");
			data->loop = false;
			data->reload = true;
			goto exit;
		}
#endif
	}

error:
	data->ret = EXIT_FAILURE;
#if HANDLE_SIGNALS || WATCH_CONFIG
exit:
#endif
	if ( fds[wayland_fd].fd != -1 )
		close(fds[wayland_fd].fd);
#if HANDLE_SIGNALS
	if ( fds[signal_fd].fd != -1 )
		close(fds[signal_fd].fd);
#endif
#if WATCH_CONFIG
	if ( fds[inotify_fd].fd != -1 )
		close(fds[inotify_fd].fd);
#endif
	free(fds);
	return;
}

static bool handle_command_flags (struct Lava_data *data, int argc, char *argv[])
{
	const char usage[] =
		"Usage: lavalauncher [options...]\n"
		"  -c <path>, --config <path> Path to config file.\n"
		"  -h,        --help          Print this help text.\n"
		"  -v,        --verbose       Enable verbose output.\n"
		"  -V,        --version       Show version.\n"
		"\n"
		"The configuration syntax is documented in the man page.\n";

	static struct option opts[] = {
		{"config",  required_argument, NULL, 'c'},
		{"help",    no_argument,       NULL, 'h'},
		{"verbose", no_argument,       NULL, 'v'},
		{"version", no_argument,       NULL, 'V'},
		{0,         0,                 0,    0  }
	};

	extern int optind;
	optind = 0;
	extern char *optarg;
	for (int c; (c = getopt_long(argc, argv, "c:hvV", opts, &optind)) != -1 ;) switch (c)
	{
		case 'c':
			set_string(&data->config_path, optarg);
			break;

		case 'h':
			fputs(usage, stderr);
			data->ret = EXIT_SUCCESS;
			return false;

		case 'v':
			data->verbosity++;
			break;

		case 'V':
			fputs("LavaLauncher version " LAVALAUNCHER_VERSION"\n", stderr);
			data->ret = EXIT_SUCCESS;
			return false;

		default:
			return false;
	}

	return true;
}

static bool get_default_config_path (struct Lava_data *data)
{
	struct
	{
		const char *fmt;
		const char *env;
	} paths[] = {
		{ .fmt = "./lavalauncher.conf",                           .env = NULL                      },
		{ .fmt = "%s/lavalauncher/lavalauncher.conf",             .env = getenv("XDG_CONFIG_HOME") },
		{ .fmt = "%s/.config/lavalauncher/lavalauncher.conf",     .env = getenv("HOME")            },
		{ .fmt = "/usr/local/etc/lavalauncher/lavalauncher.conf", .env = NULL                      },
		{ .fmt = "/etc/lavalauncher/lavalauncher.conf",           .env = NULL                      }
	};

	FOR_ARRAY(paths, i)
	{
		data->config_path = get_formatted_buffer(paths[i].fmt, paths[i].env);
		if (! access(data->config_path, F_OK | R_OK))
		{
			log_message(data, 1, "[main] Using default configuration file path: %s\n", data->config_path);
			return true;
		}
		free_if_set(data->config_path);
	}

	log_message(NULL, 0, "ERROR: Can not find configuration file.\n"
			"INFO: You can provide a path manually with '-c'.\n");
	return false;
}

static void init_data (struct Lava_data *data)
{
	data->ret         = EXIT_FAILURE;
	data->loop        = true;
	data->reload      = false;
	data->verbosity   = 0;
	data->config_path = NULL;

#if WATCH_CONFIG
	data->watch = false;
#endif

	data->display            = NULL;
	data->registry           = NULL;

	data->compositor         = NULL;
	data->subcompositor      = NULL;
	data->shm                = NULL;
	data->layer_shell        = NULL;
	data->xdg_output_manager = NULL;

	data->river_status_manager = NULL;
	data->need_river_status    = false;

	data->need_keyboard = false;
	data->need_pointer  = false;
	data->need_touch    = false;

	wl_list_init(&data->bars);
	data->last_bar = NULL;

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

	log_message(&data, 1, "[main] LavaLauncher: version=%s\n", LAVALAUNCHER_VERSION);

	/* If the user did not provide the path to a configuration file, try
	 * the default location.
	 */
	if ( data.config_path == NULL )
		if (! get_default_config_path(&data))
			return EXIT_FAILURE;

	/* Try to parse the configuration file. If this fails, there might
	 * already be heap objects, so some cleanup is needed.
	 */
	if (! parse_config_file(&data))
		goto early_exit;

	if (! init_wayland(&data))
		goto exit;

	data.ret = EXIT_SUCCESS;
	main_loop(&data);

exit:
	finish_wayland(&data);
early_exit:
	destroy_all_bars(&data);
	if (data.reload)
		goto reload;
	return data.ret;
}

