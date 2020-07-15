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

#ifndef LAVALAUNCHER_BAR_H
#define LAVALAUNCHER_BAR_H

#include<wayland-server.h>
#include"bar/buffer.h"

struct Lava_data;
struct Lava_output;
struct Lava_bar_pattern;

struct Lava_bar
{
	struct Lava_data             *data;
	struct wl_list                link;
	struct Lava_bar_pattern      *pattern;
	struct Lava_output           *output;
	struct wl_surface            *wl_surface;
	struct zwlr_layer_surface_v1 *layer_surface;

	int32_t buffer_width, buffer_height;
	int32_t bar_x, bar_y, bar_width, bar_height;
	int32_t item_area_x, item_area_y, item_area_width, item_area_height;

	struct Lava_buffer  buffers[2];
	struct Lava_buffer *current_buffer;

	bool configured;
};

bool create_bar (struct Lava_bar_pattern *pattern, struct Lava_output *output);
void destroy_bar (struct Lava_bar *bar);
void destroy_all_bars (struct Lava_output *output);
void update_bar (struct Lava_bar *bar);
struct Lava_bar *bar_from_surface (struct Lava_data *data, struct wl_surface *surface);

#endif

