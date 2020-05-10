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
#include<cairo/cairo.h>

#include"lavalauncher.h"
#include"button.h"
#include"output.h"
#include"config.h"

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

bool create_button (struct Lava_data *data)
{
	struct Lava_button *new_button = calloc(1, sizeof(struct Lava_button));
	if ( new_button == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	/* We do not know the icon size yet. */
	new_button->length = 0;
	new_button->type   = TYPE_BUTTON;
	new_button->cmd    = NULL;
	new_button->img    = NULL;

	new_button->background_colour_hex = NULL;
	new_button->background_colour[0]  = 0.0;
	new_button->background_colour[1]  = 0.0;
	new_button->background_colour[2]  = 0.0;
	new_button->background_colour[3]  = 1.0;

	data->last_button = new_button;
	wl_list_insert(&data->buttons, &new_button->link);
	return true;
}

static bool button_set_command (struct Lava_button *button, const char *command)
{
	if ( button->cmd != NULL )
		free(button->cmd);
	button->cmd = strdup(command);
	return true;
}

static bool button_set_image_path (struct Lava_button *button, const char *path)
{
	if ( button->img != NULL )
		cairo_surface_destroy(button->img);
	errno       = 0;
	button->img = cairo_image_surface_create_from_png(path);
	if ( errno != 0 )
	{
		fprintf(stderr, "ERROR: Failed loading image: %s\n"
				"ERROR: cairo_image_surface_create_from_png: %s\n",
				path, strerror(errno));
		return false;
	}
	return true;
}

static bool button_set_background_colour (struct Lava_button *button,
		const char *arg)
{
	if (! hex_to_rgba(arg, &(button->background_colour[0]),
				&(button->background_colour[1]),
				&(button->background_colour[2]),
				&(button->background_colour[3])))
		return false;
	button->background_colour_hex = (char *)arg;
	return true;
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_button*, const char*);
} button_configs[] = {
	{ .variable = "command",           .set = button_set_command           },
	{ .variable = "image-path",        .set = button_set_image_path        },
	{ .variable = "background-colour", .set = button_set_background_colour }
};

bool button_set_variable (struct Lava_data *data, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(button_configs) / sizeof(button_configs[0])); i++)
		if (! strcmp(button_configs[i].variable, variable))
			return button_configs[i].set(data->last_button, value);
	fprintf(stderr, "ERROR: Unrecognized button setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}

bool create_spacer (struct Lava_data *data)
{
	struct Lava_button *new_spacer = calloc(1, sizeof(struct Lava_button));
	if ( new_spacer == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	new_spacer->length = 0;
	new_spacer->type   = TYPE_SPACER;
	new_spacer->cmd    = NULL;
	new_spacer->img    = NULL;

	data->last_button = new_spacer;
	wl_list_insert(&data->buttons, &new_spacer->link);
	return true;
}

static bool spacer_set_length (struct Lava_button *button, const char *length)
{
	int l = atoi(length);
	if ( l <= 0 )
	{
		fputs("ERROR: Spacer size must be greater than 0.\n", stderr);
		return false;
	}
	button->length = (unsigned int)l;
	return true;
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_button*, const char*);
} spacer_configs[] = {
	{ .variable = "length", .set = spacer_set_length }
};

bool spacer_set_variable (struct Lava_data *data, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(spacer_configs) / sizeof(spacer_configs[0])); i++)
		if (! strcmp(spacer_configs[i].variable, variable))
			return spacer_configs[i].set(data->last_button, value);
	fprintf(stderr, "ERROR: Unrecognized spacer setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
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
	if ( data->button_amount == 0 )
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
