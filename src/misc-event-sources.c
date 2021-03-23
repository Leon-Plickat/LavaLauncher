/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 - 2021 Leon Henrik Plickat
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
#include<poll.h>
#include<errno.h>

#if WATCH_CONFIG
#include<sys/inotify.h>
#endif

#if HANDLE_SIGNALS
#include<sys/signalfd.h>
#include<signal.h>
#endif

#include"lavalauncher.h"
#include"event-loop.h"
#include"str.h"

/**************************
 *                        *
 *  Inotify event source  *
 *                        *
 **************************/
#if WATCH_CONFIG
static bool inotify_source_init (struct pollfd *fd)
{
	log_message(1, "[loop] Setting up inotify event source.\n");

	fd->events = POLLIN;
	if ( -1 == (fd->fd = inotify_init1(IN_NONBLOCK)) )
	{
		log_message(0, "ERROR: Unable to open inotify fd.\n"
				"ERROR: inotify_init1: %s\n", strerror(errno));
		return false;
	}

	/* Add config file to inotify watch list. */
	if ( -1 == inotify_add_watch(fd->fd, context.config_path, IN_MODIFY) )
	{
		log_message(0, "ERROR: Unable to add config path to inotify watchlist.\n");
		return false;
	}

	return true;
}

static bool inotify_source_finish (struct pollfd *fd)
{
	if ( fd->fd != -1 )
		close(fd->fd);
	return true;
}

static bool inotify_source_flush (struct pollfd *fd)
{
	return true;
}

static bool inotify_source_handle_in (struct pollfd *fd)
{
	log_message(1, "[main] Config file modified; Triggering reload.\n");
	context.loop = false;
	context.reload = true;
	return true;
}

static bool inotify_source_handle_out (struct pollfd *fd)
{
	return true;
}

struct Lava_event_source inotify_source = {
	.init       = inotify_source_init,
	.finish     = inotify_source_finish,
	.flush      = inotify_source_flush,
	.handle_in  = inotify_source_handle_in,
	.handle_out = inotify_source_handle_out
};
#endif

/*************************
 *                       *
 *  Signal event source  *
 *                       *
 *************************/
#if HANDLE_SIGNALS
static bool signal_source_init (struct pollfd *fd)
{
	log_message(1, "[loop] Setting up signalfd event source.\n");

	sigset_t mask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGUSR1);
	sigaddset(&mask, SIGUSR2);

	if ( sigprocmask(SIG_BLOCK, &mask, NULL) == -1 )
	{
		log_message(0, "ERROR: sigprocmask() failed.\n");
		return false;
	}

	fd->events = POLLIN;
	if ( -1 == (fd->fd = signalfd(-1, &mask, 0)) )
	{
		log_message(0, "ERROR: Unable to open signal fd.\n"
				"ERROR: signalfd: %s\n", strerror(errno));
		return false;
	}

	return true;
}

static bool signal_source_finish (struct pollfd *fd)
{
	if ( fd->fd != -1 )
		close(fd->fd);
	return true;
}

static bool signal_source_flush (struct pollfd *fd)
{
	return true;
}

static bool signal_source_handle_in (struct pollfd *fd)
{
	struct signalfd_siginfo fdsi;
	if ( read(fd->fd, &fdsi, sizeof(struct signalfd_siginfo))
			!= sizeof(struct signalfd_siginfo) )
	{
		log_message(0, "ERROR: Can not read signal info.\n");
		return false;
	}

	if ( fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGQUIT )
	{
		log_message(1, "[loop] Received SIGTERM or SIGQUIT; Exiting.\n");
		return false;
	}
	else if ( fdsi.ssi_signo == SIGUSR1 || fdsi.ssi_signo == SIGUSR2 )
	{
		log_message(1, "[loop] Received SIGUSR; Triggering reload.\n");
		context.loop = false;
		context.reload = true;
		return false;
	}

	return true;
}

static bool signal_source_handle_out (struct pollfd *fd)
{
	return true;
}

struct Lava_event_source signal_source = {
	.init       = signal_source_init,
	.finish     = signal_source_finish,
	.flush      = signal_source_flush,
	.handle_in  = signal_source_handle_in,
	.handle_out = signal_source_handle_out
};
#endif

