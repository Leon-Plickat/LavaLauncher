/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>

#include"lavalauncher.h"
#include"config.h"

void sensible_defaults (struct Lava_data *data)
{
	data->position          = POSITION_BOTTOM;
	data->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	data->bar_width         = 80;
	data->border_width      = 2;
	data->bar_colour[0]     = 0.0f;
	data->bar_colour[1]     = 0.0f;
	data->bar_colour[2]     = 0.0f;
	data->bar_colour[3]     = 1.0f;
	data->border_colour[0]  = 1.0f;
	data->border_colour[1]  = 1.0f;
	data->border_colour[2]  = 1.0f;
	data->border_colour[3]  = 1.0f;
	data->verbose           = false;
	data->aggressive_anchor = false;
}

static void hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
{
	unsigned int r = 0, g = 0, b = 0, a = 0;

	if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a) )
	{
		fputs("Colour codes are expected to use the format #RRGGBBAA\n",
				stderr);
		exit(EXIT_FAILURE);
	}

	*c_r = r / 255.0f;
	*c_g = g / 255.0f;
	*c_b = b / 255.0f;
	*c_a = a / 255.0f;
}

void config_add_button(struct Lava_data *data, char *path, char *cmd)
{
	struct Lava_button *new_button = calloc(1, sizeof(struct Lava_button));
	new_button->img_path = path;
	new_button->cmd      = cmd;
	wl_list_insert(&data->buttons, &new_button->link);
}

void config_set_layer(struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "overlay"))
		data->layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
	else if (! strcmp(arg, "top"))
		data->layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
	else if (! strcmp(arg, "bottom"))
		data->layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	else if (! strcmp(arg, "background"))
		data->layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
	else
	{
		fputs("Possible layers are 'overlay', 'top',"
				"'bottom' and 'background'.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_position(struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "top"))
		data->position = POSITION_TOP;
	else if (! strcmp(arg, "right"))
		data->position = POSITION_RIGHT;
	else if (! strcmp(arg, "bottom"))
		data->position = POSITION_BOTTOM;
	else if (! strcmp(arg, "left"))
		data->position = POSITION_LEFT;
	else if (! strcmp(arg, "center"))
		data->position = POSITION_CENTER;
	else
	{
		fputs("Possible positions are 'top', 'right',"
				"'bottom','left' and 'center'.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_bar_size(struct Lava_data *data, const char *arg)
{
	data->bar_width = atoi(arg);

	if ( data->bar_width <= 0 )
	{
		fputs("Bar width must be greater than zero.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_border_size(struct Lava_data *data, const char *arg)
{
	data->border_width = atoi(arg);

	if ( data->border_width < 0 )
	{
		fputs("Border width must be equal to or greater than zero.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_bar_colour(struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg,
			&(data->bar_colour[0]),
			&(data->bar_colour[1]),
			&(data->bar_colour[2]),
			&(data->bar_colour[3]));
}

void config_set_border_colour(struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg,
			&(data->border_colour[0]),
			&(data->border_colour[1]),
			&(data->border_colour[2]),
			&(data->border_colour[3]));
}

