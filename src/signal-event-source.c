/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2021 Leon Henrik Plickat
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
#include<signal.h>

#ifndef BSD
#include<execinfo.h>
#endif

#include"lavalauncher.h"
#include"event-loop.h"
#include"util.h"

int signal_pipe[2];

static void handle_user_signal (int signum)
{
	const char *kind = signum == SIGUSR1 ? "1" : "2";
	const ssize_t w = write(signal_pipe[1], kind, strlen(kind)+1);
	if ( w <= 0 )
		log_message(0, "ERROR: write: %s\n", strerror(errno));
}

static void handle_error (int signum)
{
	const char *msg =
		"\n"
		"┌───────────────────────────────────────────┐\n"
		"│                                           │\n"
		"│        LavaLauncher has crashed.          │\n"
		"│  Please report this to the mailing list.  │\n"
		"│                                           │\n"
		"│  ~leon_plickat/lavalauncher@lists.sr.ht   │\n"
		"│                                           │\n"
		"└───────────────────────────────────────────┘\n"
		"\n";
	fputs(msg, stderr);

#ifndef BSD
	fputs("Attempting to get backtrace:\n", stderr);

	/* In some rare cases, getting a backtrace can also cause a segfault.
	 * There is nothing we can or should do about that. All hope is lost at
	 * that point.
	 */
	void *buffer[255];
	const int calls = backtrace(buffer, sizeof(buffer) / sizeof(void *));
	backtrace_symbols_fd(buffer, calls, fileno(stderr));
	fputs("\n", stderr);
#endif

	/* Let's let the default handlers deal with the rest. */
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}

static void handle_kill (int signum)
{
	/* We only intercept this signal to print a message for the logs. */
	fputs("LavaLauncher has been killed.\n", stderr);
	signal(signum, SIG_DFL);
	kill(getpid(), signum);
}

static void handle_soft_kill (int signum)
{
	signal(signum, handle_kill);
	log_message(1, "[signal] Interrupt received.\n");
	if ( signum == SIGINT )
		log_message(0, "Press Ctrl-C again to abort the cleanup and exit immediately.\n");
	context.loop = false;
	context.reload = false;
}

static bool signal_source_init (struct pollfd *fd)
{
	log_message(1, "[loop] Setting up signal event source.\n");

	if ( pipe(signal_pipe) == -1 )
	{
		log_message(0, "ERROR: pipe: %s\n", strerror(errno));
		return false;
	}

	signal(SIGSEGV, handle_error);
	signal(SIGFPE, handle_error);
	signal(SIGKILL, handle_kill);
	signal(SIGINT, handle_soft_kill);
	signal(SIGTERM, handle_soft_kill);
	signal(SIGUSR1, handle_user_signal);
	signal(SIGUSR2, handle_user_signal);

	fd->events = POLLIN;
	fd->fd = signal_pipe[0];

	return true;
}

static bool signal_source_finish (struct pollfd *fd)
{
	close(signal_pipe[0]);
	close(signal_pipe[1]);
	return true;
}

static bool signal_source_flush (struct pollfd *fd)
{
	return true;
}

#define BUFFER_SIZE 10
static bool signal_source_handle_in (struct pollfd *fd)
{
	char buffer[BUFFER_SIZE];
	const ssize_t r = read(signal_pipe[0], buffer, BUFFER_SIZE);
	if ( r > 0 )
	{
		log_message(0, "[signal] Reived SIGUSR%s. User signals are currently not handled.\n", buffer);
		return true;
	}
	else
	{
		log_message(0, "ERROR: read: %s\n", strerror(errno));
		return false;
	}
}
#undef BUFFER_SIZE

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

