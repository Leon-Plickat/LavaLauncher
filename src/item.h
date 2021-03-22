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
#include<wayland-client.h>
#include<cairo/cairo.h>

#include"types/image_t.h"

struct Lava_bar_instance;

enum Item_type
{
	TYPE_BUTTON,
	TYPE_SPACER
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

	uint32_t spacer_length;

	image_t *img;
	struct wl_list commands;

	char *associated_app_id;
};

struct Lava_item_instance
{
	struct Lava_item *item;
	int32_t x, y;
	uint32_t w, h;

	uint32_t indicator;
	uint32_t active_indicator;
	uint32_t toplevel_exists_indicator;
	uint32_t toplevel_activated_indicator;

	/** Is the item displayed on this bar instance? */
	bool active;

	/** Does the item need to be redrawn? */
	bool dirty;
};

bool create_item (enum Item_type type);
bool item_set_variable (struct Lava_item *item, const char *variable,
		const char *value, uint32_t line);
void item_interaction (struct Lava_item *item, struct Lava_bar_instance *instance,
		enum Interaction_type type, uint32_t modifiers, uint32_t special);
void destroy_all_items (void);

#endif

