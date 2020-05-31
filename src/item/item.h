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

#ifndef LAVALAUNCHER_ITEM_H
#define LAVALAUNCHER_ITEM_H

#include<stdbool.h>
#include<wayland-server.h>
#include<cairo/cairo.h>

struct Lava_data;
struct Lava_bar_pattern;
struct Lava_bar;

enum Item_type
{
	TYPE_BUTTON,
	TYPE_SPACER
};

struct Lava_item
{
	struct wl_list   link;
	enum Item_type   type;

	char            *cmd;
	cairo_surface_t *img;
	unsigned int     index, ordinate, length;
	float            background_colour[4];
	char            *background_colour_hex;
};

bool create_item (struct Lava_bar_pattern *pattern, enum Item_type type);
bool item_set_variable (struct Lava_item *item, const char *variable,
		const char *value, int line);
void item_interaction (struct Lava_bar *bar, struct Lava_item *item);
struct Lava_item *item_from_coords (struct Lava_bar *bar, int32_t x, int32_t y);
unsigned int get_item_length_sum (struct Lava_bar_pattern *pattern);
bool finalize_items (struct Lava_bar_pattern *pattern);
void destroy_all_items (struct Lava_bar_pattern *pattern);

#endif

