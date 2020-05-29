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
#include"items/item.h"
#include"bar/bar.h"
#include"widget/widget.h"
#include"output.h"
#include"config.h"
#include"command.h"

#include"items/button.h"
#include"items/spacer.h"

bool create_item (struct Lava_data *data, enum Item_type type)
{
	switch (type)
	{
		case TYPE_BUTTON: return create_button(data);
		case TYPE_SPACER: return create_spacer(data);
		default:          return false;
	}
}

bool item_set_variable (struct Lava_data *data, enum Item_type type,
		const char *variable, const char *value, int line)
{
	switch (type)
	{
		case TYPE_BUTTON: return button_set_variable(data, variable, value, line);
		case TYPE_SPACER: return spacer_set_variable(data, variable, value, line);
		default:          return false;
	}
}

void item_interaction (struct Lava_data *data, struct Lava_bar *bar,
		struct Lava_item *item)
{
	switch (item->type)
	{
		case TYPE_BUTTON:
			item_command(data, item, bar->output);
			spawn_widget(item->widget_command);
			break;

		case TYPE_SPACER:
		default:
			break;
	}
}

void item_nullify (struct Lava_item *item)
{
	item->type                         = 0;
	item->img                          = NULL;
	item->index                        = 0;
	item->ordinate                     = 0;
	item->length                       = 0;
	item->cmd                          = 0;
	item->background_colour_hex        = NULL;
	item->background_colour[0]         = 0.0;
	item->background_colour[1]         = 0.0;
	item->background_colour[2]         = 0.0;
	item->background_colour[3]         = 1.0;
	item->widget_command               = NULL;
	item->widget_border_t              = 0;
	item->widget_border_r              = 0;
	item->widget_border_b              = 0;
	item->widget_border_l              = 0;
	item->widget_margin                = 0;
	item->widget_background_colour_hex = NULL;
	item->widget_background_colour[0]  = 0.0;
	item->widget_background_colour[1]  = 0.0;
	item->widget_background_colour[2]  = 0.0;
	item->widget_background_colour[3]  = 1.0;
	item->widget_border_colour_hex     = NULL;
	item->widget_border_colour[0]      = 0.0;
	item->widget_border_colour[1]      = 0.0;
	item->widget_border_colour[2]      = 0.0;
	item->widget_border_colour[3]      = 1.0;
}

/* Return pointer to Lava_item struct from item list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_item *item_from_coords (struct Lava_data *data,
		struct Lava_bar *bar, int32_t x, int32_t y)
{
	unsigned int ordinate;
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - (bar->x_offset + data->border_left);
	else
		ordinate = y - (bar->y_offset + data->border_top);

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
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_reverse_safe (bt_1, bt_2, &data->items, link)
		sum += bt_1->length;
	return sum;
}

bool init_items (struct Lava_data *data)
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
			it_1->length = data->icon_size;

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
	if ( item->widget_background_colour_hex != NULL )
		free(item->widget_background_colour_hex);
	if ( item->background_colour != NULL )
		free(item->background_colour);
	if ( item->widget_border_colour_hex != NULL )
		free(item->widget_border_colour_hex);
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
