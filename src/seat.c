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

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"seat.h"
#include"input.h"

/* No-Op function. */
static void noop () {}

static void seat_handle_capabilities (void *raw_data, struct wl_seat *wl_seat,
		uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_data *data = seat->data;

	if (data->verbose)
		fputs("Handling seat capabilities.\n", stderr);

	if ( seat->pointer.wl_pointer != NULL )
	{
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}

	if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
	{
		if (data->verbose)
			fputs("Seat has pointer capabilities.\n", stderr);
		seat->pointer.wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer.wl_pointer,
				&pointer_listener, seat);
	}

	if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
	{
		if (data->verbose)
			fputs("Seat has touch capabilities.\n", stderr);
		seat->touch.wl_touch = wl_seat_get_touch(wl_seat);
		wl_touch_add_listener(seat->touch.wl_touch,
				&touch_listener, seat);
	}
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name         = noop
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (data->verbose)
		fputs("Adding seat.\n", stderr);

	struct wl_seat *wl_seat = wl_registry_bind(registry, name,
			&wl_seat_interface, 3);
	struct Lava_seat *seat = calloc(1, sizeof(struct Lava_seat));
	if ( seat == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return false;
	}

	seat->data    = data;
	seat->wl_seat = wl_seat;
	wl_list_insert(&data->seats, &seat->link);
	wl_seat_add_listener(wl_seat, &seat_listener, seat);

	return true;
}

static void destroy_seat (struct Lava_seat *seat)
{
	wl_list_remove(&seat->link);
	wl_seat_release(seat->wl_seat);
	if (seat->pointer.wl_pointer)
		wl_pointer_release(seat->pointer.wl_pointer);
	free(seat);
}

void destroy_all_seats (struct Lava_data *data)
{
	struct Lava_seat *st_1, *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
		destroy_seat(st_1);
}
