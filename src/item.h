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

#include"types/image_t.h"

struct Lava_data;
struct Lava_bar;
struct Lava_bar_instance;

enum Item_type
{
	TYPE_BUTTON,
	TYPE_SPACER
};

// TODO [item-rework] position of the item on the bar.
//      -> in default mode, display as usual
//      -> in full mode there will be gaps between the three areads
enum Item_alignment
{
	ITEM_ALIGN_START,
	ITEM_ALIGN_CENTER,
	ITEM_ALIGN_END
};

enum Interaction_type
{
	INTERACTION_MOUSE_BUTTON,
	INTERACTION_MOUSE_SCROLL,
	INTERACTION_TOUCH,
	INTERACTION_UNIVERSAL
};

struct Lava_item_command
{
	struct wl_list link;

	enum Interaction_type type;
	char *command;
	uint32_t modifiers;

	/* For button events this is the button, for scroll events the direction. */
	uint32_t special;
};

struct Lava_item
{
	struct wl_list link;
	enum Item_type type;
	enum Item_alignment alignment;

	image_t *img;
	struct wl_list commands;

	// TODO [item-rework] all sizes are relational
	float relational_size;
};

bool create_item (struct Lava_bar *bar, enum Item_type type);
bool item_set_variable (struct Lava_data *data, struct Lava_item *item,
		const char *variable, const char *value, int line);
void item_interaction (struct Lava_item *item, struct Lava_bar_instance *instance,
		enum Interaction_type type, uint32_t modifiers, uint32_t special);
struct Lava_item *item_from_coords (struct Lava_bar_instance *instance, uint32_t x, uint32_t y);
unsigned int get_item_length_sum (struct Lava_bar *bar);
bool finalize_items (struct Lava_bar *bar);
void destroy_all_items (struct Lava_bar *bar);

#endif

