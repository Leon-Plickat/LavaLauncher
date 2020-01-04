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
	data->mode              = MODE_DEFAULT;
	data->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	data->icon_size         = 80;
	data->border_size       = 2;
	data->margin            = 0;
	data->verbose           = false;
	data->only_output       = NULL;
	data->exclusive_zone    = 1;
	data->bar_colour_hex    = "#000000FF";
	data->bar_colour[0]     = 0.0f;
	data->bar_colour[1]     = 0.0f;
	data->bar_colour[2]     = 0.0f;
	data->bar_colour[3]     = 1.0f;
	data->border_colour_hex = "#FFFFFFFF";
	data->border_colour[0]  = 1.0f;
	data->border_colour[1]  = 1.0f;
	data->border_colour[2]  = 1.0f;
	data->border_colour[3]  = 1.0f;
}

static void hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
{
	unsigned int r = 0, g = 0, b = 0, a = 0;

	if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a) )
	{
		fputs("Colour codes are expected to use the format #RRGGBBAA\n", stderr);
		exit(EXIT_FAILURE);
	}

	*c_r = r / 255.0f;
	*c_g = g / 255.0f;
	*c_b = b / 255.0f;
	*c_a = a / 255.0f;
}

void config_add_button (struct Lava_data *data, char *path, char *cmd)
{
	struct Lava_button *new_button = calloc(1, sizeof(struct Lava_button));
	new_button->img_path = path;
	new_button->cmd      = cmd;
	wl_list_insert(&data->buttons, &new_button->link);
}

void config_set_layer (struct Lava_data *data, const char *arg)
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
				"'bottom' and 'background'.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_mode (struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "default"))
		data->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "aggressive"))
		data->mode = MODE_AGGRESSIVE;
	else if (! strcmp(arg, "full"))
		data->mode = MODE_FULL;
	else if (! strcmp(arg, "full-center"))
		data->mode = MODE_FULL_CENTER;
	else
	{
		fputs("Possible modes are 'default', 'aggressive',"
				"'full' and 'full-center'.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_exclusive (struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "true"))
		data->exclusive_zone = 1;
	else if (! strcmp(arg, "false"))
		data->exclusive_zone = 0;
	else if (! strcmp(arg, "stationary"))
		data->exclusive_zone = -1;
	else
	{
		fputs("Exclusive zone can be 'true', 'false' or 'stationary'.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_position (struct Lava_data *data, const char *arg)
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
				"'bottom','left' and 'center'.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_margin (struct Lava_data *data, const char *arg)
{
	data->margin = atoi(arg);

	if ( data->margin < 0 )
	{
		fputs("Margin must be zero or greater.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_icon_size (struct Lava_data *data, const char *arg)
{
	data->icon_size = atoi(arg);

	if ( data->icon_size <= 0 )
	{
		fputs("Icon size must be greater than zero.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_border_size (struct Lava_data *data, const char *arg)
{
	data->border_size = atoi(arg);

	if ( data->border_size < 0 )
	{
		fputs("Border size must be equal to or greater than zero.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

void config_set_bar_colour (struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg, &(data->bar_colour[0]), &(data->bar_colour[1]),
			&(data->bar_colour[2]), &(data->bar_colour[3]));
	data->bar_colour_hex = (char *)arg;
}

void config_set_border_colour (struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg, &(data->border_colour[0]), &(data->border_colour[1]),
			&(data->border_colour[2]), &(data->border_colour[3]));
	data->border_colour_hex = (char *)arg;
}

