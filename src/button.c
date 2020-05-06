/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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
#include<cairo/cairo.h>

#include"lavalauncher.h"
#include"button.h"
#include"output.h"

/* Return pointer to Lava_button struct from button list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_button *button_from_coords (struct Lava_data *data,
		struct Lava_output *output, int32_t x, int32_t y)
{
	unsigned int ordinate;
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - (output->bar_x_offset + data->border_left);
	else
		ordinate = y - (output->bar_y_offset + data->border_top);

	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		if ( ordinate >= bt_1->ordinate
				&& ordinate < bt_1->ordinate + bt_1->length )
			return bt_1;
	}
	return NULL;
}

bool add_button (struct Lava_data *data, char *path, char *cmd)
{
	errno = 0;

	struct Lava_button *new_button = calloc(1, sizeof(struct Lava_button));
	if ( new_button == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	/* We do not know the icon size yet. */
	new_button->length = 0;
	new_button->type   = TYPE_BUTTON;
	new_button->cmd    = strdup(cmd);
	new_button->img    = cairo_image_surface_create_from_png(path);
	if ( errno != 0 )
	{
		fprintf(stderr, "ERROR: Failed loading image: %s\n"
				"ERROR: cairo_image_surface_create_from_png: %s\n",
				path, strerror(errno));
		return false;
	}

	wl_list_insert(&data->buttons, &new_button->link);
	return true;
}

bool add_spacer (struct Lava_data *data, int length)
{
	if ( length <= 0 )
	{
		fputs("ERROR: Spacer size must be above 0.", stderr);
		return false;
	}

	struct Lava_button *new_spacer = calloc(1, sizeof(struct Lava_button));
	if ( new_spacer == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	new_spacer->length = length;
	new_spacer->type   = TYPE_SPACER;
	new_spacer->cmd    = NULL;
	new_spacer->img    = NULL;

	wl_list_insert(&data->buttons, &new_spacer->link);
	return true;
}

unsigned int get_button_length_sum (struct Lava_data *data)
{
	unsigned int sum = 0;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe (bt_1, bt_2, &data->buttons, link)
		sum += bt_1->length;
	return sum;
}

bool init_buttons (struct Lava_data *data)
{
	data->button_amount = wl_list_length(&(data->buttons));
	if (! data->button_amount)
	{
		fputs("ERROR: No buttons defined.\n", stderr);
		return false;
	}

	unsigned int index = 0, ordinate = 0;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		if ( bt_1->type == TYPE_BUTTON )
			bt_1->length = data->icon_size;

		bt_1->index    = index;
		bt_1->ordinate = ordinate;

		index++;
		ordinate += bt_1->length;
	}

	return true;
}

void destroy_buttons (struct Lava_data *data)
{
	if ( wl_list_length(&(data->buttons)) <= 0 )
		return;

	if (data->verbose)
		fputs("Destroying buttons and freeing icons.\n", stderr);

	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &data->buttons, link)
	{
		wl_list_remove(&bt_1->link);
		if ( bt_1->img != NULL )
			cairo_surface_destroy(bt_1->img);
		if ( bt_1->cmd != NULL )
			free(bt_1->cmd);
		free(bt_1);
	}
}
