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

#ifndef LAVALAUNCHER_SEAT_H
#define LAVALAUNCHER_SEAT_H

#include<stdbool.h>
#include<stdint.h>

struct Lava_data;
struct Lava_bar;

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_seat *wl_seat;

	struct
	{
		struct wl_pointer *wl_pointer;

		/* Current position. */
		int32_t           x, y;
		struct Lava_bar  *bar;
		struct Lava_item *item;

		/* Stuff needed to gracefully handle scroll events. */
		uint32_t   discrete_steps, last_update_time;
		wl_fixed_t value;

		/* Stuff needed to change the cursor image. */
		struct wl_surface      *cursor_surface;
		struct wl_cursor_theme *cursor_theme;
		struct wl_cursor_image *cursor_image;
		struct wl_cursor       *cursor;
	} pointer;

	struct
	{
		struct wl_touch  *wl_touch;
		int32_t           id;
		struct Lava_bar  *bar;
		struct Lava_item *item;
	} touch;
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version);
void destroy_all_seats (struct Lava_data *data);

#endif
