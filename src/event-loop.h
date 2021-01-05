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

#ifndef LAVALAUNCHER_EVENT_LOOP_H
#define LAVALAUNCHER_EVENT_LOOP_H

#include<stdbool.h>
#include<stdint.h>
#include<poll.h>
#include<wayland-client.h>

struct Lava_data;

struct Lava_event_loop
{
	nfds_t fd_count;
	struct wl_list sources;
};

struct Lava_event_source
{
	struct wl_list link;
	bool (*init)(struct pollfd *, struct Lava_data *);
	bool (*finish)(struct pollfd *, struct Lava_data *);
	bool (*flush)(struct pollfd *, struct Lava_data *);
	bool (*handle_in)(struct pollfd *, struct Lava_data *);
	bool (*handle_out)(struct pollfd *, struct Lava_data *);
	nfds_t index;
};

void event_loop_init (struct Lava_event_loop *loop);
void event_loop_add_event_source (struct Lava_event_loop *loop, struct Lava_event_source *source);
bool event_loop_run (struct  Lava_event_loop *loop, struct Lava_data *data);

#endif
