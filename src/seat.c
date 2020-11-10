/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Henrik Plickat
 * Copyright (C) 2020 Nicolai Dagestad
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
#include"str.h"
#include"seat.h"
#include"cursor.h"
#include"input.h"
#include"bar/bar.h"
#include"bar/bar-pattern.h"
#include"bar/indicator.h"

/* No-Op function. */
static void noop () {}

static void seat_handle_capabilities (void *raw_data, struct wl_seat *wl_seat,
		uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_data *data = seat->data;

	log_message(data, 1, "[seat] Handling seat capabilities.\n");

	if ( seat->pointer.wl_pointer != NULL )
	{
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}

	if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
	{
		log_message(data, 2, "[seat] Seat has pointer capabilities.\n");
		seat->pointer.wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer.wl_pointer,
				&pointer_listener, seat);
	}

	if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
	{
		log_message(data, 2, "[seat] Seat has touch capabilities.\n");
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
	log_message(data, 1, "[seat] Adding seat.\n");

	struct wl_seat *wl_seat = wl_registry_bind(registry, name,
			&wl_seat_interface, 5);
	struct Lava_seat *seat = calloc(1, sizeof(struct Lava_seat));
	if ( seat == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not allocate.\n");
		return false;
	}

	seat->data    = data;
	seat->wl_seat = wl_seat;
	wl_list_init(&seat->touch.touchpoints);
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
	if (seat->touch.wl_touch)
		wl_touch_release(seat->touch.wl_touch);
	cleanup_cursor(seat);
	destroy_all_touchpoints(seat);
	free(seat);
}

void destroy_all_seats (struct Lava_data *data)
{
	log_message(data, 1, "[seat] Destroying all seats.\n");
	struct Lava_seat *st_1, *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
		destroy_seat(st_1);
}

bool create_touchpoint (struct Lava_seat *seat, int32_t id,
          struct Lava_bar *bar, struct Lava_item *item)
{
	log_message(seat->data, 1, "[seat] Creating touchpoint.\n");

	struct Lava_touchpoint *touchpoint = calloc(1, sizeof(struct Lava_touchpoint));
	if ( touchpoint == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not allocate.\n");
		return false;
	}

	touchpoint->id   = id;
	touchpoint->bar  = bar;
	touchpoint->item = item;

	touchpoint->indicator = create_indicator(bar);
	if ( touchpoint->indicator != NULL )
	{
		touchpoint->indicator->touchpoint = touchpoint;
		indicator_set_colour(touchpoint->indicator, &bar->pattern->indicator_active_colour);
		move_indicator(touchpoint->indicator, item);
		indicator_commit(touchpoint->indicator);
	}
	else
		log_message(NULL, 0, "ERROR: Could not create indicator.\n");

	wl_list_insert(&seat->touch.touchpoints, &touchpoint->link);

	return true;
}

void destroy_touchpoint (struct Lava_touchpoint *touchpoint)
{
	if ( touchpoint->indicator != NULL )
		destroy_indicator(touchpoint->indicator);

	wl_list_remove(&touchpoint->link);
	free(touchpoint);
}

void destroy_all_touchpoints (struct Lava_seat *seat)
{
	log_message(seat->data, 1, "[seat] Destroying all touchpoints.\n");
	struct Lava_touchpoint *tp, *temp;
	wl_list_for_each_safe(tp, temp, &seat->touch.touchpoints, link)
		destroy_touchpoint(tp);
}

struct Lava_touchpoint *touchpoint_from_id (struct Lava_seat *seat, int32_t id)
{
	struct Lava_touchpoint *touchpoint;
	wl_list_for_each(touchpoint, &seat->touch.touchpoints, link)
		if ( touchpoint->id == id )
			return touchpoint;
	return NULL;
}
