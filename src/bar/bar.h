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
#include"types/buffer.h"

struct Lava_data;
struct Lava_output;
struct Lava_bar_pattern;

struct Lava_bar
{
	struct Lava_data             *data;
	struct wl_list                link;
	struct Lava_bar_pattern      *pattern;
	struct Lava_output           *output;
	struct wl_surface            *bar_surface;
	struct wl_surface            *icon_surface;
	struct wl_subsurface         *subsurface;
	struct zwlr_layer_surface_v1 *layer_surface;

	uint32_t buffer_width, buffer_height;
	uint32_t bar_x, bar_y, bar_width, bar_height;
	uint32_t item_area_x, item_area_y, item_area_width, item_area_height;

	uint32_t buffer_width_hidden, buffer_height_hidden;
	uint32_t bar_width_hidden, bar_height_hidden;
	bool hidden;

	struct Lava_buffer  bar_buffers[2];
	struct Lava_buffer *current_bar_buffer;

	struct Lava_buffer  icon_buffers[2];
	struct Lava_buffer *current_icon_buffer;

	struct wl_list indicators;

	bool configured;
};

bool create_bar (struct Lava_bar_pattern *pattern, struct Lava_output *output);
void destroy_bar (struct Lava_bar *bar);
void destroy_all_bars (struct Lava_output *output);
void update_bar (struct Lava_bar *bar);
void hide_bar (struct Lava_bar *bar);
void unhide_bar (struct Lava_bar *bar);
struct Lava_bar *bar_from_surface (struct Lava_data *data, struct wl_surface *surface);

#endif

