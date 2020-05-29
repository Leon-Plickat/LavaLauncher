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

#include<wayland-server.h>
#include<wayland-client.h>
#include<cairo/cairo.h>

#include"lavalauncher.h"
#include"config/config.h"
#include"config/colour.h"

static bool config_set_position (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "top"))
		config->position = POSITION_TOP;
	else if (! strcmp(arg, "right"))
		config->position = POSITION_RIGHT;
	else if (! strcmp(arg, "bottom"))
		config->position = POSITION_BOTTOM;
	else if (! strcmp(arg, "left"))
		config->position = POSITION_LEFT;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized position \"%s\".\n"
				"INFO: Possible positions are 'top', 'right', "
				"'bottom' and 'left'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_alignment (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "start"))
		config->alignment = ALIGNMENT_START;
	else if (! strcmp(arg, "center"))
		config->alignment = ALIGNMENT_CENTER;
	else if (! strcmp(arg, "end"))
		config->alignment = ALIGNMENT_END;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized alignment \"%s\".\n"
				"INFO: Possible alignments are 'start', "
				"'center' and 'end'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_mode (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "default"))
		config->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "full"))
		config->mode = MODE_FULL;
	else if (! strcmp(arg, "simple"))
		config->mode = MODE_SIMPLE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized mode \"%s\".\n"
				"INFO: Possible alignments are 'default', "
				"'full' and 'simple'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_layer (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "overlay"))
		config->layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
	else if (! strcmp(arg, "top"))
		config->layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
	else if (! strcmp(arg, "bottom"))
		config->layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	else if (! strcmp(arg, "background"))
		config->layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized layer \"%s\".\n"
				"INFO: Possible layers are 'overlay', "
				"'top', 'bottom', and 'background'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_icon_size (struct Lava_config *config, const char *arg,
		const char direction)
{
	// TODO check issdigit()
	config->icon_size = atoi(arg);
	if ( config->icon_size <= 0 )
	{
		fputs("ERROR: Icon size must be greater than zero.\n", stderr);
		return false;
	}
	return true;
}

static bool config_set_border_size (struct Lava_config *config, const char *arg,
		const char direction)
{
	int size = atoi(arg);
	if ( size < 0 )
	{
		fputs("ERROR: Border size must be equal to or greater than zero.\n",
				stderr);
		return false;
	}

	switch (direction)
	{
		case 't': config->border_top    = size; break;
		case 'r': config->border_right  = size; break;
		case 'b': config->border_bottom = size; break;
		case 'l': config->border_left   = size; break;
		case 'a':
			config->border_top    = size;
			config->border_right  = size;
			config->border_bottom = size;
			config->border_left   = size;
			break;
	}
	return true;
}

static bool config_set_margin_size (struct Lava_config *config, const char *arg,
		const char direction)
{
	int size = atoi(arg);
	if ( size < 0 )
	{
		fputs("ERROR: Margin size must be equal to or greater than zero.\n",
				stderr);
		return false;
	}

	switch (direction)
	{
		case 't': config->margin_top    = size; break;
		case 'r': config->margin_right  = size; break;
		case 'b': config->margin_bottom = size; break;
		case 'l': config->margin_left   = size; break;
	}
	return true;
}

static bool config_set_only_output (struct Lava_config *config, const char *arg,
		const char direction)
{
	if ( config->only_output != NULL )
		free(config->only_output);
	if ( ! strcmp(arg, "all") || *arg == '*' )
		config->only_output = NULL;
	else
		config->only_output = strdup(arg);
	return true;
}

static bool config_set_exclusive_zone (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "true"))
		config->exclusive_zone = 1;
	else if (! strcmp(arg, "false"))
		config->exclusive_zone = 0;
	else if (! strcmp(arg, "stationary"))
		config->exclusive_zone = -1;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
				"INFO: Possible options are 'true', "
				"'false' and 'stationary'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_cursor_name (struct Lava_config *config, const char *arg,
		const char direction)
{
	if ( config->cursor_name != NULL )
		free(config->cursor_name);
	config->cursor_name = strdup(arg);
	return true;
}

static bool config_set_bar_colour (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! hex_to_rgba(arg, &(config->bar_colour[0]), &(config->bar_colour[1]),
				&(config->bar_colour[2]), &(config->bar_colour[3])))
		return false;
	if ( config->bar_colour_hex != NULL )
		free(config->bar_colour_hex);
	config->bar_colour_hex = strdup(arg);
	return true;
}

static bool config_set_border_colour (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (!  hex_to_rgba(arg, &(config->border_colour[0]), &(config->border_colour[1]),
				&(config->border_colour[2]), &(config->border_colour[3])))
		return false;
	if ( config->border_colour_hex != NULL )
		free(config->border_colour_hex);
	config->border_colour_hex = strdup(arg);
	return true;
}

static bool config_set_effect_colour (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (!  hex_to_rgba(arg, &(config->effect_colour[0]), &(config->effect_colour[1]),
				&(config->effect_colour[2]), &(config->effect_colour[3])))
		return false;
	if ( config->effect_colour_hex != NULL )
		free(config->effect_colour_hex);
	config->effect_colour_hex = strdup(arg);
	return true;
}

static bool config_set_effect (struct Lava_config *config, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "none"))
		config->effect = EFFECT_NONE;
	else if (! strcmp(arg, "box"))
		config->effect = EFFECT_BOX;
	else if (! strcmp(arg, "phone"))
		config->effect = EFFECT_PHONE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized effect \"%s\".\n"
				"INFO: Possible options are 'none', "
				"'box' and 'phone'.\n", arg);
		return false;
	}
	return true;
}

static bool config_set_effect_padding (struct Lava_config *config, const char *arg,
		const char direction)
{
	config->effect_padding = atoi(arg);
	if ( config->effect_padding < 0 )
	{
		fputs("ERROR: Effect padding size must be equal to or "
				"greater than zero.\n", stderr);
		return false;
	}
	return true;
}

static bool config_set_widget_background_colour (struct Lava_config *config,
		const char *arg, const char direction)
{
	if (! hex_to_rgba(arg, &(config->widget_background_colour[0]),
				&(config->widget_background_colour[1]),
				&(config->widget_background_colour[2]),
				&(config->widget_background_colour[3])))
		return false;
	if ( config->widget_border_colour_hex != NULL )
		free(config->widget_background_colour_hex);
	config->widget_background_colour_hex = strdup(arg);
	return true;
}

static bool config_set_widget_border_colour (struct Lava_config *config,
		const char *arg, const char direction)
{
	if (! hex_to_rgba(arg, &(config->widget_border_colour[0]),
				&(config->widget_border_colour[1]),
				&(config->widget_border_colour[2]),
				&(config->widget_border_colour[3])))
		return false;
	if ( config->widget_border_colour_hex != NULL )
		free(config->widget_border_colour_hex);
	config->widget_border_colour_hex = strdup(arg);
	return true;
}

static bool config_set_widget_margin_size (struct Lava_config *config,
		const char *arg, const char direction)
{
	int size = atoi(arg);
	if ( size < 0 )
	{
		fputs("ERROR: Widget margin size must be equal to or greater than zero.\n",
				stderr);
		return false;
	}
	config->widget_margin = size;
	return true;
}

static bool config_set_widget_border_size (struct Lava_config *config,
		const char *arg, const char direction)
{
	int size = atoi(arg);
	if ( size < 0 )
	{
		fputs("ERROR: Widget border size must be equal to or greater than zero.\n",
				stderr);
		return false;
	}
	switch (direction)
	{
		case 't': config->widget_border_top    = size; break;
		case 'r': config->widget_border_right  = size; break;
		case 'b': config->widget_border_bottom = size; break;
		case 'l': config->widget_border_left   = size; break;
		case 'a':
			config->widget_border_top    = size;
			config->widget_border_right  = size;
			config->widget_border_bottom = size;
			config->widget_border_left   = size;
			break;
	}
	return true;
}

struct Configs
{
	const char *variable, direction;
	bool (*set)(struct Lava_config*, const char*, const char);
} configs[] = {
	{ .variable = "position",                 .set = config_set_position,                 .direction = '0'},
	{ .variable = "alignment",                .set = config_set_alignment,                .direction = '0'},
	{ .variable = "mode",                     .set = config_set_mode,                     .direction = '0'},
	{ .variable = "layer",                    .set = config_set_layer,                    .direction = '0'},
	{ .variable = "icon-size",                .set = config_set_icon_size,                .direction = '0'},
	{ .variable = "border",                   .set = config_set_border_size,              .direction = 'a'},
	{ .variable = "border-top",               .set = config_set_border_size,              .direction = 't'},
	{ .variable = "border-right",             .set = config_set_border_size,              .direction = 'r'},
	{ .variable = "border-bottom",            .set = config_set_border_size,              .direction = 'b'},
	{ .variable = "border-left",              .set = config_set_border_size,              .direction = 'l'},
	{ .variable = "margin-top",               .set = config_set_margin_size,              .direction = 't'},
	{ .variable = "margin-right",             .set = config_set_margin_size,              .direction = 'r'},
	{ .variable = "margin-bottom",            .set = config_set_margin_size,              .direction = 'b'},
	{ .variable = "margin-left",              .set = config_set_margin_size,              .direction = 'l'},
	{ .variable = "output",                   .set = config_set_only_output,              .direction = '0'},
	{ .variable = "exclusive-zone",           .set = config_set_exclusive_zone,           .direction = '0'},
	{ .variable = "cursor-name",              .set = config_set_cursor_name,              .direction = '0'},
	{ .variable = "background-colour",        .set = config_set_bar_colour,               .direction = '0'},
	{ .variable = "border-colour",            .set = config_set_border_colour,            .direction = '0'},
	{ .variable = "effect-colour",            .set = config_set_effect_colour,            .direction = '0'},
	{ .variable = "effect",                   .set = config_set_effect,                   .direction = '0'},
	{ .variable = "effect-padding",           .set = config_set_effect_padding,           .direction = '0'},
	{ .variable = "widget-background-colour", .set = config_set_widget_background_colour, .direction = '0'},
	{ .variable = "widget-border-colour",     .set = config_set_widget_border_colour,     .direction = '0'},
	{ .variable = "widget-margin",            .set = config_set_widget_margin_size,       .direction = '0'},
	{ .variable = "widget-border",            .set = config_set_widget_border_size,       .direction = 'a'},
	{ .variable = "widget-border-top",        .set = config_set_widget_border_size,       .direction = 't'},
	{ .variable = "widget-border-right",      .set = config_set_widget_border_size,       .direction = 'r'},
	{ .variable = "widget-border-bottom",     .set = config_set_widget_border_size,       .direction = 'b'},
	{ .variable = "widget-border-left",       .set = config_set_widget_border_size,       .direction = 'l'}
};

bool config_set_variable (struct Lava_config *config, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
			return configs[i].set(config, value, configs[i].direction);
	fprintf(stderr, "ERROR: Unrecognized setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}
