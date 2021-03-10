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

#include<stdbool.h>
#include<stdlib.h>
#include<stdint.h>
#include<poll.h>
#include<errno.h>
#include<string.h>
#include<wayland-client.h>

#include"lavalauncher.h"
#include"event-loop.h"
#include"util.h"

bool event_loop_init (struct Lava_event_loop *loop, nfds_t capacity)
{
	loop->fd_count = 0;
	loop->capacity = capacity;
	loop->sources = malloc(sizeof(struct Lava_event_source) * capacity);
	if ( loop->sources == NULL )
	{
		log_message(0, "ERROR: Failed to allocate.\n");
		return false;
	}
	return true;
}

void event_loop_finish (struct Lava_event_loop *loop)
{
	if ( loop->sources != NULL )
		free(loop->sources);
}

void event_loop_add_event_source (struct Lava_event_loop *loop, struct Lava_event_source *source)
{
	/* We don't check for overflow because the maximum amount of event
	 * sources is known at compile time. As long as we are careful with the
	 * capacity we pass event_loop_init() we'll be fine.
	 */
	loop->sources[loop->fd_count].init       = source->init;
	loop->sources[loop->fd_count].finish     = source->finish;
	loop->sources[loop->fd_count].flush      = source->flush;
	loop->sources[loop->fd_count].handle_in  = source->handle_in;
	loop->sources[loop->fd_count].handle_out = source->handle_out;
	loop->fd_count++;
}

bool event_loop_run (struct  Lava_event_loop *loop)
{
	log_message(1, "[loop] Starting main loop.\n");

	struct pollfd *fds = calloc(loop->fd_count, sizeof(struct pollfd));
	if ( fds == NULL )
	{
		log_message(0, "ERROR: Can not allocate.\n");
		return false;
	}
	bool ret = true;

	/* Call init functions */
	for (nfds_t i = 0; i < loop->fd_count; i++)
		if (! (loop->sources[i].init(&fds[i])))
		{
			ret = false;
			goto exit;
		}

	while (context.loop)
	{
		errno = 0;

		/* Call flush functions */
		for (nfds_t i = 0; i < loop->fd_count; i++)
			if (! (loop->sources[i].flush(&fds[i])))
			{
				ret = false;
				goto exit;
			}

		if ( poll(fds, loop->fd_count, -1) < 0 )
		{
			if ( errno == EINTR )
				continue;

			log_message(0, "poll: %s\n", strerror(errno));
			ret = false;
			goto exit;
		}

		/* Check for events and call handlers */
		for (nfds_t i = 0; i < loop->fd_count; i++)
		{
			if ( fds[i].revents & POLLIN )
			{
				if (! (loop->sources[i].handle_in(&fds[i])))
				{
					ret = false;
					goto exit;
				}
			}

			if ( fds[i].revents & POLLOUT )
			{
				if (! (loop->sources[i].handle_out(&fds[i])))
				{
					ret = false;
					goto exit;
				}
			}
		}
	}

exit:
	/* Call finish functions */
	for (nfds_t i = 0; i < loop->fd_count; i++)
		if (! (loop->sources[i].finish(&fds[i])))
			ret = false;

	free(fds);
	return ret;
}

