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
#include"item/item.h"
#include"config/config.h"
#include"config/colour.h"

bool create_button (struct Lava_data *data)
{
	struct Lava_item *new_button = calloc(1, sizeof(struct Lava_item));
	if ( new_button == NULL )
	{
		fprintf(stderr, "ERROR: Could not allocate.\n");
		return false;
	}

	item_nullify(new_button);
	new_button->type = TYPE_BUTTON;


	data->last_item = new_button;
	wl_list_insert(&data->items, &new_button->link);
	return true;
}

static bool button_set_command (struct Lava_item *button, const char *command)
{
	if ( button->cmd != NULL )
		free(button->cmd);
	button->cmd = strdup(command);
	return true;
}

static bool button_set_image_path (struct Lava_item *button, const char *path)
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

static bool button_set_background_colour (struct Lava_item *button, const char *arg)
{
	if (! hex_to_rgba(arg, &(button->background_colour[0]),
				&(button->background_colour[1]),
				&(button->background_colour[2]),
				&(button->background_colour[3])))
		return false;
	if ( button->background_colour_hex != NULL )
		free(button->background_colour_hex);
	button->background_colour_hex = strdup(arg);
	return true;
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_item*, const char*);
} button_configs[] = {
	{ .variable = "command",           .set = button_set_command           },
	{ .variable = "image-path",        .set = button_set_image_path        },
	{ .variable = "background-colour", .set = button_set_background_colour }
};

bool button_set_variable (struct Lava_item *button, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(button_configs) / sizeof(button_configs[0])); i++)
		if (! strcmp(button_configs[i].variable, variable))
			return button_configs[i].set(button, value);
	fprintf(stderr, "ERROR: Unrecognized button setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}
