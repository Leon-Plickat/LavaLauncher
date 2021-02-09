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
#include"str.h"

void event_loop_init (struct Lava_event_loop *loop)
{
	loop->fd_count = 0;
	wl_list_init(&loop->sources);
}

void event_loop_add_event_source (struct Lava_event_loop *loop, struct Lava_event_source *source)
{
	source->index = loop->fd_count;
	loop->fd_count++;
	wl_list_insert(&loop->sources, &source->link);
}

bool event_loop_run (struct  Lava_event_loop *loop)
{
	log_message(1, "[loop] Starting main loop.\n");

	struct pollfd *fds = calloc(loop->fd_count, sizeof(struct pollfd));
	if ( fds == NULL )
	{
		log_message(0, "ERROR: Can not allocate.\n");
		return NULL;
	}
	bool ret = true;

	/* Call init functions */
	struct Lava_event_source *source;
	wl_list_for_each(source, &loop->sources, link)
		if (! (source->init(&fds[source->index])))
		{
			ret = false;
			goto exit;
		}

	while (context.loop)
	{
		errno = 0;

		/* Call flush functions */
		wl_list_for_each(source, &loop->sources, link)
			if (! (source->flush(&fds[source->index])))
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
		wl_list_for_each(source, &loop->sources, link)
		{
			if ( fds[source->index].revents & POLLIN )
			{
				if (! (source->handle_in(&fds[source->index])))
				{
					ret = false;
					goto exit;
				}
			}

			if ( fds[source->index].revents & POLLOUT )
			{
				if (! (source->handle_out(&fds[source->index])))
				{
					ret = false;
					goto exit;
				}
			}
		}
	}

exit:
	/* Call finish functions */
	wl_list_for_each(source, &loop->sources, link)
		if (! (source->finish(&fds[source->index])))
				ret = false;

	free(fds);
	return ret;
}

