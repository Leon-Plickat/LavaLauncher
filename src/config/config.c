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
#include<errno.h>
#include<string.h>
#include<ctype.h>

#include"lavalauncher.h"
#include"config/config.h"
#include"config/parser.h"
#include"item/item.h"

static void sensible_defaults (struct Lava_config *config)
{
	config->position          = POSITION_BOTTOM;
	config->alignment         = ALIGNMENT_CENTER;
	config->mode              = MODE_DEFAULT;
	config->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	config->icon_size         = 80;
	config->border_top        = 2;
	config->border_right      = 2;
	config->border_bottom     = 2;
	config->border_left       = 2;
	config->margin_top        = 0;
	config->margin_right      = 0;
	config->margin_bottom     = 0;
	config->margin_left       = 0;

	config->only_output       = NULL;
	config->exclusive_zone    = 1;

	config->cursor_name       = strdup("pointing_hand");

	config->bar_colour_hex    = strdup("#000000FF");
	config->bar_colour[0]     = 0.0f;
	config->bar_colour[1]     = 0.0f;
	config->bar_colour[2]     = 0.0f;
	config->bar_colour[3]     = 1.0f;

	config->border_colour_hex = strdup("#FFFFFFFF");
	config->border_colour[0]  = 1.0f;
	config->border_colour[1]  = 1.0f;
	config->border_colour[2]  = 1.0f;
	config->border_colour[3]  = 1.0f;

	config->effect_colour_hex = strdup("#FFFFFFFF");
	config->effect_colour[0]  = 1.0f;
	config->effect_colour[1]  = 1.0f;
	config->effect_colour[2]  = 1.0f;
	config->effect_colour[3]  = 1.0f;

	config->effect            = EFFECT_NONE;
	config->effect_padding    = 5;
}

/* Calculate the dimensions of the visible part of the bar. */
static void calculate_dimensions (struct Lava_data *data)
{
	struct Lava_config *config = &data->config;
	switch (config->position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			config->orientation = ORIENTATION_VERTICAL;
			config->w = (uint32_t)(config->icon_size + config->border_right
					+ config->border_left);
			config->h = (uint32_t)(get_item_length_sum(data)
					+ config->border_top + config->border_bottom);
			if ( config->exclusive_zone == 1 )
				config->exclusive_zone = config->w;
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			config->orientation = ORIENTATION_HORIZONTAL;
			config->w = (uint32_t)(get_item_length_sum(data)
					+ config->border_left + config->border_right);
			config->h = (uint32_t)(config->icon_size + config->border_top
					+ config->border_bottom);
			if ( config->exclusive_zone == 1 )
				config->exclusive_zone = config->h;
			break;
	}
}

bool init_config (struct Lava_data *data)
{
	struct Lava_config *config = &data->config;

	if (data->verbose)
		fputs("Init config.\n", stderr);

	sensible_defaults(config);
	if (! parse_config_file(data))
		goto error;
	if (! finalize_items(data, config->icon_size))
		goto error;
	calculate_dimensions(data);

	return true;

error:
	finish_config(data);
	return false;
}

void finish_config (struct Lava_data *data)
{
	struct Lava_config *config = &data->config;
	if (data->verbose)
		fputs("Finish config.\n", stderr);
	if ( config->only_output != NULL )
		free(config->only_output);
	if ( config->cursor_name != NULL )
		free(config->cursor_name);
	if ( config->bar_colour_hex != NULL )
		free(config->bar_colour_hex);
	if ( config->border_colour_hex != NULL )
		free(config->border_colour_hex);
	if ( config->effect_colour_hex != NULL )
		free(config->effect_colour_hex);
}
