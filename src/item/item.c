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
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"output.h"
#include"item/item.h"
#include"item/command.h"
#include"item/button.h"
#include"item/spacer.h"

static void item_nullify (struct Lava_item *item)
{
	item->type                  = 0;
	item->img                   = NULL;
	item->index                 = 0;
	item->ordinate              = 0;
	item->length                = 0;
	item->left_click_command    = NULL;
	item->middle_click_command  = NULL;
	item->right_click_command   = NULL;
	item->scroll_up_command     = NULL;
	item->scroll_down_command   = NULL;
	item->touch_command         = NULL;
	item->background_colour[0]  = 0.0;
	item->background_colour[1]  = 0.0;
	item->background_colour[2]  = 0.0;
	item->background_colour[3]  = 1.0;
	item->replace_background    = false;
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
	if (pattern->data->verbose)
		fprintf(stderr, "Creating item: type=%s\n", item_type_to_string(type));
	struct Lava_item *new_item = calloc(1, sizeof(struct Lava_item));
	if ( new_item == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	item_nullify(new_item);
	new_item->type  = type;
	pattern->last_item = new_item;
	wl_list_insert(&pattern->items, &new_item->link);
	return true;
}

bool item_set_variable (struct Lava_item *item, const char *variable,
		const char *value, int line)
{
	switch (item->type)
	{
		case TYPE_BUTTON: return button_set_variable(item, variable, value, line);
		case TYPE_SPACER: return spacer_set_variable(item, variable, value, line);
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
struct Lava_item *item_from_coords (struct Lava_bar *bar, int32_t x, int32_t y)
{
	struct Lava_bar_pattern *pattern = bar->pattern;
	unsigned int ordinate;
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - bar->item_area_x;
	else
		ordinate = y - bar->item_area_y;

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

/* When items are created when parsing the config file, the icon size is not yet
 * available, so items need to be finalized later.
 */
bool finalize_items (struct Lava_bar_pattern *pattern)
{
	pattern->item_amount = wl_list_length(&pattern->items);
	if ( pattern->item_amount == 0 )
	{
		fputs("ERROR: Configuration defines a bar without items.\n", stderr);
		return false;
	}

	unsigned int index = 0, ordinate = 0;
	struct Lava_item *it1, *it2;
	wl_list_for_each_reverse_safe(it1, it2, &pattern->items, link)
	{
		if ( it1->type == TYPE_BUTTON )
			it1->length = pattern->icon_size;

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
	if ( item->img != NULL )
		cairo_surface_destroy(item->img);
	if ( item->left_click_command != NULL )
		free(item->left_click_command);
	if ( item->middle_click_command != NULL )
		free(item->middle_click_command);
	if ( item->right_click_command != NULL )
		free(item->right_click_command);
	if ( item->scroll_up_command != NULL )
		free(item->scroll_up_command);
	if ( item->scroll_down_command != NULL )
		free(item->scroll_down_command);
	if ( item->touch_command != NULL )
		free(item->touch_command);
	free(item);
}

void destroy_all_items (struct Lava_bar_pattern *pattern)
{
	if (pattern->data->verbose)
		fputs("Destroying items.\n", stderr);
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &pattern->items, link)
		destroy_item(bt_1);
}

