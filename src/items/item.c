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
#include"item.h"
#include"output.h"
#include"config.h"

#include"button.h"
#include"spacer.h"

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

/* Return pointer to Lava_item struct from item list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_item *item_from_coords (struct Lava_data *data,
		struct Lava_output *output, int32_t x, int32_t y)
{
	unsigned int ordinate;
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - (output->bar_x_offset + data->border_left);
	else
		ordinate = y - (output->bar_y_offset + data->border_top);

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

void destroy_items (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Destroying items.\n", stderr);
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &data->items, link)
	{
		wl_list_remove(&bt_1->link);
		if ( bt_1->img != NULL )
			cairo_surface_destroy(bt_1->img);
		if ( bt_1->cmd != NULL )
			free(bt_1->cmd);
		free(bt_1);
	}
}
