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

#include"types/string_t.h"

struct Lava_data;
struct Lava_bar_pattern;
struct Lava_bar;

enum Item_type
{
	TYPE_BUTTON,
	TYPE_SPACER
};

enum Interaction_type
{
	TYPE_RIGHT_CLICK = 0,
	TYPE_MIDDLE_CLICK,
	TYPE_LEFT_CLICK,
	TYPE_SCROLL_UP,
	TYPE_SCROLL_DOWN,
	TYPE_TOUCH,
	TYPE_AMOUNT
};

struct Lava_item
{
	struct wl_list   link;
	enum Item_type   type;

	struct Lava_image *img;

	unsigned int index, ordinate, length;

	string_t *command[TYPE_AMOUNT];
};

bool create_item (struct Lava_bar_pattern *pattern, enum Item_type type);
bool copy_item (struct Lava_bar_pattern *pattern, struct Lava_item *item);
bool item_set_variable (struct Lava_data *data, struct Lava_item *item,
		const char *variable, const char *value, int line);
void item_interaction (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type);
struct Lava_item *item_from_coords (struct Lava_bar *bar, uint32_t x, uint32_t y);
unsigned int get_item_length_sum (struct Lava_bar_pattern *pattern);
bool finalize_items (struct Lava_bar_pattern *pattern);
void destroy_all_items (struct Lava_bar_pattern *pattern);

#endif

