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

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include"lavalauncher.h"
#include"str.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"output.h"
#include"types/image.h"
#include"types/string_t.h"
#include"item/item.h"
#include"item/command.h"
#include"item/button.h"
#include"item/spacer.h"

static void item_nullify (struct Lava_item *item)
{
	item->type                  = 0;
	item->index                 = 0;
	item->ordinate              = 0;
	item->length                = 0;
	item->img                   = NULL;

	for (int i = 0; i < TYPE_AMOUNT; i++)
		item->command[i] = NULL;
}

static const char *item_type_to_string (enum Item_type type)
{
	switch (type)
	{
		case TYPE_BUTTON: return "button";
		case TYPE_SPACER: return "spacer";
	}
	return NULL;
}

bool create_item (struct Lava_bar_pattern *pattern, enum Item_type type)
{
	log_message(pattern->data, 2, "[item] Creating item: type=%s\n", item_type_to_string(type));
	struct Lava_item *new_item = calloc(1, sizeof(struct Lava_item));
	if ( new_item == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	item_nullify(new_item);
	new_item->type  = type;
	pattern->last_item = new_item;
	wl_list_insert(&pattern->items, &new_item->link);
	return true;
}

bool copy_item (struct Lava_bar_pattern *pattern, struct Lava_item *item)
{
	if (! create_item(pattern, item->type))
		return false;
	struct Lava_item *new_item = pattern->last_item;

	/* Copy item settings. */
	new_item->type                 = item->type;
	new_item->index                = item->index;
	new_item->ordinate             = item->ordinate;
	new_item->length               = item->length;

	for (int i = 0; i < TYPE_AMOUNT; i++)
		if ( item->command[i] != NULL )
			new_item->command[i] = string_t_reference(item->command[i]);

	if ( item->img != NULL )
		new_item->img = image_reference(item->img);

	return true;
}

bool item_set_variable (struct Lava_data *data, struct Lava_item *item,
		const char *variable, const char *value, int line)
{
	switch (item->type)
	{
		case TYPE_BUTTON: return button_set_variable(data, item, variable, value, line);
		case TYPE_SPACER: return spacer_set_variable(data, item, variable, value, line);
		default:          return false;
	}
}

void item_interaction (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type)
{
	switch (item->type)
	{
		case TYPE_BUTTON:
			item_command(bar, item, type);
			break;

		case TYPE_SPACER:
		default:
			break;
	}
}

/* Return pointer to Lava_item struct from item list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_item *item_from_coords (struct Lava_bar *bar, uint32_t x, uint32_t y)
{
	struct Lava_bar_pattern *pattern = bar->pattern;
	uint32_t ordinate;
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - bar->item_area.x;
	else
		ordinate = y - bar->item_area.y;

	struct Lava_item *item, *temp;
	wl_list_for_each_reverse_safe(item, temp, &pattern->items, link)
	{
		if ( ordinate >= item->ordinate
				&& ordinate < item->ordinate + item->length )
			return item;
	}
	return NULL;
}

unsigned int get_item_length_sum (struct Lava_bar_pattern *pattern)
{
	unsigned int sum = 0;
	struct Lava_item *it1, *it2;
	wl_list_for_each_reverse_safe (it1, it2, &pattern->items, link)
		sum += it1->length;
	return sum;
}

/* When items are created when parsing the config file, the size is not yet
 * available, so items need to be finalized later.
 */
bool finalize_items (struct Lava_bar_pattern *pattern)
{
	pattern->item_amount = wl_list_length(&pattern->items);
	if ( pattern->item_amount == 0 )
	{
		log_message(NULL, 0, "ERROR: Configuration defines a bar without items.\n");
		return false;
	}

	unsigned int index = 0, ordinate = 0;
	struct Lava_item *it1, *it2;
	wl_list_for_each_reverse_safe(it1, it2, &pattern->items, link)
	{
		if ( it1->type == TYPE_BUTTON )
			it1->length = pattern->size;

		it1->index    = index;
		it1->ordinate = ordinate;

		index++;
		ordinate += it1->length;
	}

	return true;
}

static void destroy_item (struct Lava_item *item)
{
	wl_list_remove(&item->link);

	for (int i = 0; i < TYPE_AMOUNT; i++)
		if ( item->command[i] != NULL )
			string_t_destroy(item->command[i]);

	if ( item->img != NULL )
		image_destroy(item->img);

	free(item);
}

void destroy_all_items (struct Lava_bar_pattern *pattern)
{
	log_message(pattern->data, 1, "[items] Destroying all items.\n");
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &pattern->items, link)
		destroy_item(bt_1);
}

