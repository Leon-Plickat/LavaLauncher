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
#include"config.h"

void config_sensible_defaults (struct Lava_data *data)
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

	data->only_output       = NULL;
	data->exclusive_zone    = 1;

	data->cursor.name       = strdup("pointing_hand");

	data->bar_colour_hex    = strdup("#000000FF");
	data->bar_colour[0]     = 0.0f;
	data->bar_colour[1]     = 0.0f;
	data->bar_colour[2]     = 0.0f;
	data->bar_colour[3]     = 1.0f;

	data->border_colour_hex = strdup("#FFFFFFFF");
	data->border_colour[0]  = 1.0f;
	data->border_colour[1]  = 1.0f;
	data->border_colour[2]  = 1.0f;
	data->border_colour[3]  = 1.0f;

	data->effect_colour_hex = strdup("#FFFFFFFF");
	data->effect_colour[0]  = 1.0f;
	data->effect_colour[1]  = 1.0f;
	data->effect_colour[2]  = 1.0f;
	data->effect_colour[3]  = 1.0f;

	data->effect            = EFFECT_NONE;
	data->effect_padding    = 5;

	data->widget_background_colour_hex = strdup("#000000FF");
	data->widget_background_colour[0]  = 1.0f;
	data->widget_background_colour[1]  = 1.0f;
	data->widget_background_colour[2]  = 1.0f;
	data->widget_background_colour[3]  = 1.0f;

	data->widget_border_colour_hex = strdup("#000000FF");
	data->widget_border_colour[0]  = 1.0f;
	data->widget_border_colour[1]  = 1.0f;
	data->widget_border_colour[2]  = 1.0f;
	data->widget_border_colour[3]  = 1.0f;

	data->widget_margin = 20;
	data->widget_border_t = 2;
	data->widget_border_r = 2;
	data->widget_border_b = 2;
	data->widget_border_l = 2;
}

/* Free all heap object created by config. */
void config_free_settings (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Freeing configuration.\n", stderr);
	if ( data->only_output != NULL )
		free(data->only_output);
	if ( data->cursor.name != NULL )
		free(data->cursor.name);
	if ( data->bar_colour_hex != NULL )
		free(data->bar_colour_hex);
	if ( data->border_colour_hex != NULL )
		free(data->border_colour_hex);
	if ( data->effect_colour_hex != NULL )
		free(data->effect_colour_hex);
	if ( data->widget_background_colour_hex != NULL )
		free(data->widget_background_colour_hex);
	if ( data->widget_border_colour_hex != NULL )
		free(data->widget_border_colour_hex);
}

/* Convert a hex colour string with or without alpha channel into RGBA floats. */
bool hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
{
	if ( hex == NULL || *hex == '\0' || *hex == ' ' )
		goto error;

	unsigned int r = 0, g = 0, b = 0, a = 255;

	if (! strcmp(hex, "transparent"))
		r = g = b = a = 0;
	else if (! strcmp(hex, "black"))
		r = g = b = 0, a = 255;
	else if (! strcmp(hex, "white"))
		r = g = b = a = 255;
	else if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a)
			&& 3 != sscanf(hex, "#%02x%02x%02x", &r, &g, &b) )
		goto error;

	*c_r = r / 255.0f;
	*c_g = g / 255.0f;
	*c_b = b / 255.0f;
	*c_a = a / 255.0f;

	return true;

error:
	fprintf(stderr, "ERROR: \"%s\" is not a valid colour.\n"
			"INFO: Colour codes are expected to be in the "
			"format #RRGGBBAA or #RRGGBB\n", hex);
	return false;
}

static bool config_set_position (struct Lava_data *data, const char *arg,
		const char direction)
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
		fprintf(stderr, "ERROR: Unrecognized position \"%s\".\n"
				"INFO: Possible positions are 'top', 'right', "
				"'bottom' and 'left'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"position", arg);
	return true;
}

static bool config_set_alignment (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "start"))
		data->alignment = ALIGNMENT_START;
	else if (! strcmp(arg, "center"))
		data->alignment = ALIGNMENT_CENTER;
	else if (! strcmp(arg, "end"))
		data->alignment = ALIGNMENT_END;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized alignment \"%s\".\n"
				"INFO: Possible alignments are 'start', "
				"'center' and 'end'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"alignment", arg);
	return true;
}

static bool config_set_mode (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "default"))
		data->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "full"))
		data->mode = MODE_FULL;
	else if (! strcmp(arg, "simple"))
		data->mode = MODE_SIMPLE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized mode \"%s\".\n"
				"INFO: Possible alignments are 'default', "
				"'full' and 'simple'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n", "mode", arg);
	return true;
}

static bool config_set_layer (struct Lava_data *data, const char *arg,
		const char direction)
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
		fprintf(stderr, "ERROR: Unrecognized layer \"%s\".\n"
				"INFO: Possible layers are 'overlay', "
				"'top', 'bottom', and 'background'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n", "layer", arg);
	return true;
}

static bool config_set_icon_size (struct Lava_data *data, const char *arg,
		const char direction)
{
	// TODO check issdigit()
	data->icon_size = atoi(arg);
	if ( data->icon_size <= 0 )
	{
		fputs("ERROR: Icon size must be greater than zero.\n", stderr);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"icon-size", arg);
	return true;
}

static bool config_set_border_size (struct Lava_data *data, const char *arg,
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
		case 't': data->border_top    = size; break;
		case 'r': data->border_right  = size; break;
		case 'b': data->border_bottom = size; break;
		case 'l': data->border_left   = size; break;
		case 'a':
			data->border_top    = size;
			data->border_right  = size;
			data->border_bottom = size;
			data->border_left   = size;
			break;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s direction=%c\n",
				"border", arg, direction);
	return true;
}

static bool config_set_margin_size (struct Lava_data *data, const char *arg,
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
		case 't': data->margin_top    = size; break;
		case 'r': data->margin_right  = size; break;
		case 'b': data->margin_bottom = size; break;
		case 'l': data->margin_left   = size; break;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s direction=%c\n",
				"margin", arg, direction);
	return true;
}

static bool config_set_only_output (struct Lava_data *data, const char *arg,
		const char direction)
{
	if ( data->only_output != NULL )
		free(data->only_output);
	if ( ! strcmp(arg, "all") || *arg == '*' )
		data->only_output = NULL;
	else
		data->only_output = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n", "output", arg);
	return true;
}

static bool config_set_exclusive_zone (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "true"))
		data->exclusive_zone = 1;
	else if (! strcmp(arg, "false"))
		data->exclusive_zone = 0;
	else if (! strcmp(arg, "stationary"))
		data->exclusive_zone = -1;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
				"INFO: Possible options are 'true', "
				"'false' and 'stationary'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"exclusive-zone", arg);
	return true;
}

static bool config_set_cursor_name (struct Lava_data *data, const char *arg,
		const char direction)
{
	if ( data->cursor.name != NULL )
		free(data->cursor.name);
	data->cursor.name = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"cursor-name", arg);
	return true;
}

static bool config_set_bar_colour (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (! hex_to_rgba(arg, &(data->bar_colour[0]), &(data->bar_colour[1]),
				&(data->bar_colour[2]), &(data->bar_colour[3])))
		return false;
	if ( data->bar_colour_hex != NULL )
		free(data->bar_colour_hex);
	data->bar_colour_hex = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"background-colour", arg);
	return true;
}

static bool config_set_border_colour (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (!  hex_to_rgba(arg, &(data->border_colour[0]), &(data->border_colour[1]),
				&(data->border_colour[2]), &(data->border_colour[3])))
		return false;
	if ( data->border_colour_hex != NULL )
		free(data->border_colour_hex);
	data->border_colour_hex = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"border-colour", arg);
	return true;
}

static bool config_set_effect_colour (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (!  hex_to_rgba(arg, &(data->effect_colour[0]), &(data->effect_colour[1]),
				&(data->effect_colour[2]), &(data->effect_colour[3])))
		return false;
	if ( data->effect_colour_hex != NULL )
		free(data->effect_colour_hex);
	data->effect_colour_hex = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"effect-colour", arg);
	return true;
}

static bool config_set_effect (struct Lava_data *data, const char *arg,
		const char direction)
{
	if (! strcmp(arg, "none"))
		data->effect = EFFECT_NONE;
	else if (! strcmp(arg, "box"))
		data->effect = EFFECT_BOX;
	else if (! strcmp(arg, "phone"))
		data->effect = EFFECT_PHONE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized effect \"%s\".\n"
				"INFO: Possible options are 'none', "
				"'box' and 'phone'.\n", arg);
		return false;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n", "effect", arg);
	return true;
}

static bool config_set_effect_padding (struct Lava_data *data, const char *arg,
		const char direction)
{
	data->effect_padding = atoi(arg);
	if ( data->effect_padding < 0 )
	{
		fputs("ERROR: Effect padding size must be equal to or "
				"greater than zero.\n", stderr);
		return false;
	}
	return true;
}

static bool config_set_widget_background_colour (struct Lava_data *data,
		const char *arg, const char direction)
{
	if (! hex_to_rgba(arg, &(data->widget_background_colour[0]),
				&(data->widget_background_colour[1]),
				&(data->widget_background_colour[2]),
				&(data->widget_background_colour[3])))
		return false;
	if ( data->widget_border_colour_hex != NULL )
		free(data->widget_background_colour_hex);
	data->widget_background_colour_hex = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"widget-background-colour", arg);
	return true;
}

static bool config_set_widget_border_colour (struct Lava_data *data,
		const char *arg, const char direction)
{
	if (! hex_to_rgba(arg, &(data->widget_border_colour[0]),
				&(data->widget_border_colour[1]),
				&(data->widget_border_colour[2]),
				&(data->widget_border_colour[3])))
		return false;
	if ( data->widget_border_colour_hex != NULL )
		free(data->widget_border_colour_hex);
	data->widget_border_colour_hex = strdup(arg);
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"widget-background-colour", arg);
	return true;
}

static bool config_set_widget_margin_size (struct Lava_data *data,
		const char *arg, const char direction)
{
	int size = atoi(arg);
	if ( size < 0 )
	{
		fputs("ERROR: Widget margin size must be equal to or greater than zero.\n",
				stderr);
		return false;
	}
	data->widget_margin = size;
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s\n",
				"widget-background-colour", arg);
	return true;
}

static bool config_set_widget_border_size (struct Lava_data *data,
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
		case 't': data->widget_border_t = size; break;
		case 'r': data->widget_border_r = size; break;
		case 'b': data->widget_border_b = size; break;
		case 'l': data->widget_border_l = size; break;
		case 'a':
			data->widget_border_t = size;
			data->widget_border_r = size;
			data->widget_border_b = size;
			data->widget_border_l = size;
			break;
	}
	if (data->verbose)
		fprintf(stderr, "Config: setting=%s value=%s direction=%c\n",
				"widget-background-colour", arg, direction);
	return true;
}

struct Configs
{
	const char *variable, direction;
	bool (*set)(struct Lava_data*, const char*, const char);
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

bool config_set_variable (struct Lava_data *data, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
			return configs[i].set(data, value, configs[i].direction);
	fprintf(stderr, "ERROR: Unrecognized setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}
