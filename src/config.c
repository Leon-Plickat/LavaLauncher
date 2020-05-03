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
#include<errno.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<cairo/cairo.h>

#include"lavalauncher.h"
#include"config.h"

void sensible_defaults (struct Lava_data *data)
{
	data->position          = POSITION_BOTTOM;
	data->alignment         = ALIGNMENT_CENTER;
	data->mode              = MODE_DEFAULT;
	data->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	data->icon_size         = 80;
	data->border_top        = 2;
	data->border_right      = 2;
	data->border_bottom     = 2;
	data->border_left       = 2;
	data->margin_top        = 0;
	data->margin_right      = 0;
	data->margin_bottom     = 0;
	data->margin_left       = 0;

	data->verbose           = false;
	data->only_output       = NULL;
	data->exclusive_zone    = 1;

	data->cursor.name       = strdup("pointing_hand");

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

/* Convert a hex colour string with or without alpha channel into RGBA floats. */
static bool hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
{
	unsigned int r = 0, g = 0, b = 0, a = 255;

	if (! strcmp(hex, "transparent"))
		r = g = b = a = 0;
	else if (! strcmp(hex, "black"))
		r = g = b = 0, a = 255;
	else if (! strcmp(hex, "white"))
		r = g = b = a = 255;
	else if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a)
			&& 3 != sscanf(hex, "#%02x%02x%02x", &r, &g, &b) )
	{
		fputs("ERROR: Colour codes are expected to be in the format"
				" #RRGGBBAA or #RRGGBB\n",
				stderr);
		return false;
	}

	*c_r = r / 255.0f;
	*c_g = g / 255.0f;
	*c_b = b / 255.0f;
	*c_a = a / 255.0f;

	return true;
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
	else
	{
		fputs("ERROR: Possible positions are 'top', 'right',"
				" 'bottom' and 'left'.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
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
		fputs("ERROR: Possible layers are 'overlay', 'top',"
				"'bottom' and 'background'.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_mode (struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "default"))
		data->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "full"))
		data->mode = MODE_FULL;
	else if (! strcmp(arg, "simple"))
		data->mode = MODE_SIMPLE;
	else
	{
		fputs("ERROR: Possible modes are 'default', 'full' and 'simple'.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_alignment (struct Lava_data *data, const char *arg)
{
	if (! strcmp(arg, "start"))
		data->alignment = ALIGNMENT_START;
	else if (! strcmp(arg, "center"))
		data->alignment = ALIGNMENT_CENTER;
	else if (! strcmp(arg, "end"))
		data->alignment = ALIGNMENT_END;
	else
	{
		fputs("ERROR: Possible alignments are 'start', 'center',"
				" and 'end'.\n", stderr);
		data->ret = EXIT_FAILURE;
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
		fputs("ERROR: Exclusive zone can be 'true', 'false' or 'stationary'.\n",
				stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_margin (struct Lava_data *data, int top, int right, int bottom, int left)
{
	data->margin_top    = top;
	data->margin_right  = right;
	data->margin_bottom = bottom;
	data->margin_left   = left;
	if ( top < 0 || right < 0 || bottom < 0 || left < 0 )
	{
		fputs("ERROR: Margins must be equal to or greater than zero.\n",
				stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_icon_size (struct Lava_data *data, const char *arg)
{
	data->icon_size = atoi(arg);
	if ( data->icon_size <= 0 )
	{
		fputs("ERROR: Icon size must be greater than zero.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_border_size (struct Lava_data *data, int top, int right,
		int bottom, int left)
{
	data->border_top    = top;
	data->border_right  = right;
	data->border_bottom = bottom;
	data->border_left   = left;
	if ( top < 0 || right < 0 || bottom < 0 || left < 0 )
	{
		fputs("ERROR: Border size must be equal to or greater than zero.\n",
				stderr);
		data->ret = EXIT_FAILURE;
	}
}

void config_set_bar_colour (struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' '
			|| ! hex_to_rgba(arg, &(data->bar_colour[0]),
				&(data->bar_colour[1]), &(data->bar_colour[2]),
				&(data->bar_colour[3])))
	{
		fputs("ERROR: Bad colour configuration.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
	data->bar_colour_hex = (char *)arg;
}

void config_set_border_colour (struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' || !  hex_to_rgba(arg,
				&(data->border_colour[0]), &(data->border_colour[1]),
				&(data->border_colour[2]), &(data->border_colour[3])))
	{
		fputs("Bad colour configuration.\n", stderr);
		data->ret = EXIT_FAILURE;
	}
	data->border_colour_hex = (char *)arg;
}

void config_set_only_output (struct Lava_data *data, const char *arg)
{
	if ( ! strcmp(arg, "all") || *arg == '*' )
		data->only_output = NULL;
	else
		data->only_output = strdup(arg);
}

void config_set_cursor_name (struct Lava_data *data, const char *arg)
{
		data->cursor.name = strdup(arg);
}
