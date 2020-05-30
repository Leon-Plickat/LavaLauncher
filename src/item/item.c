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
#include"bar/bar.h"
#include"output.h"
#include"config/config.h"
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
	item->cmd                   = 0;
	item->background_colour_hex = NULL;
	item->background_colour[0]  = 0.0;
	item->background_colour[1]  = 0.0;
	item->background_colour[2]  = 0.0;
	item->background_colour[3]  = 1.0;
}

bool create_item (struct Lava_data *data, enum Item_type type)
{
	struct Lava_item *new_item = calloc(1, sizeof(struct Lava_item));
	if ( new_item == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	item_nullify(new_item);
	new_item->type  = type;
	data->last_item = new_item;
	wl_list_insert(&data->items, &new_item->link);
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

void item_interaction (struct Lava_bar *bar, struct Lava_item *item)
{
	switch (item->type)
	{
		case TYPE_BUTTON:
			item_command(bar->data, item, bar->output);
			break;

		case TYPE_SPACER:
		default:
			break;
	}
}

/* Return pointer to Lava_item struct from item list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_item *item_from_coords (struct Lava_data *data,
		struct Lava_bar *bar, int32_t x, int32_t y)
{
	struct Lava_config *config = data->config;
	unsigned int ordinate;
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - (bar->x_offset + config->border_left);
	else
		ordinate = y - (bar->y_offset + config->border_top);

	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->items, link)
	{
		if ( ordinate >= bt_1->ordinate
				&& ordinate < bt_1->ordinate + bt_1->length )
			return bt_1;
	}
	return NULL;
}

unsigned int get_item_length_sum (struct Lava_data *data)
{
	unsigned int sum = 0;
	struct Lava_item *it_1, *it_2;
	wl_list_for_each_reverse_safe (it_1, it_2, &data->items, link)
		sum += it_1->length;
	return sum;
}

/* When items are created when parsing the config file, the icon size is not yet
 * available, so items need to be finalized later.
 */
bool finalize_items (struct Lava_data *data, const int icon_size)
{
	data->item_amount = wl_list_length(&(data->items));
	if ( data->item_amount == 0 )
	{
		fputs("ERROR: No items defined.\n", stderr);
		return false;
	}

	unsigned int index = 0, ordinate = 0;
	struct Lava_item *it_1, *it_2;
	wl_list_for_each_reverse_safe(it_1, it_2, &data->items, link)
	{
		if ( it_1->type == TYPE_BUTTON )
			it_1->length = icon_size;

		it_1->index    = index;
		it_1->ordinate = ordinate;

		index++;
		ordinate += it_1->length;
	}

	return true;
}

static void destroy_item (struct Lava_item *item)
{
	wl_list_remove(&item->link);
	if ( item->img != NULL )
		cairo_surface_destroy(item->img);
	if ( item->cmd != NULL )
		free(item->cmd);
	if ( item->background_colour_hex != NULL )
		free(item->background_colour_hex);
	free(item);
}

void destroy_all_items (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Destroying items.\n", stderr);
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &data->items, link)
		destroy_item(bt_1);
}
