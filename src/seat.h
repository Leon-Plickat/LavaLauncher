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

#ifndef LAVALAUNCHER_SEAT_H
#define LAVALAUNCHER_SEAT_H

struct Lava_data;
struct Lava_output;

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_seat *wl_seat;

	struct
	{
		struct wl_pointer  *wl_pointer;
		int32_t             x;
		int32_t             y;
		struct Lava_output *output;
		struct Lava_button *button;
	} pointer;

	struct
	{
		struct wl_touch    *wl_touch;
		int32_t             id;
		struct Lava_output *output;
		struct Lava_button *button;
	} touch;
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
void destroy_all_seats (struct Lava_data *data);

#endif
