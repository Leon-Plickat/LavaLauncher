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
#include<assert.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"bar/bar-pattern.h"
#include"item/item.h"
#include"config/colour.h"
#include"config/parse-boolean.h"

static void sensible_defaults (struct Lava_bar_pattern *pattern)
{
	pattern->position          = POSITION_BOTTOM;
	pattern->alignment         = ALIGNMENT_CENTER;
	pattern->mode              = MODE_DEFAULT;
	pattern->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	pattern->icon_size         = 80;
	pattern->border_top        = 2;
	pattern->border_right      = 2;
	pattern->border_bottom     = 2;
	pattern->border_left       = 2;
	pattern->margin_top        = 0;
	pattern->margin_right      = 0;
	pattern->margin_bottom     = 0;
	pattern->margin_left       = 0;

	pattern->only_output       = NULL;
	pattern->exclusive_zone    = 1;

	pattern->bar_colour_hex    = strdup("#000000FF");
	pattern->bar_colour[0]     = 0.0f;
	pattern->bar_colour[1]     = 0.0f;
	pattern->bar_colour[2]     = 0.0f;
	pattern->bar_colour[3]     = 1.0f;

	pattern->border_colour_hex = strdup("#FFFFFFFF");
	pattern->border_colour[0]  = 1.0f;
	pattern->border_colour[1]  = 1.0f;
	pattern->border_colour[2]  = 1.0f;
	pattern->border_colour[3]  = 1.0f;

	pattern->effect_colour_hex = strdup("#FFFFFFFF");
	pattern->effect_colour[0]  = 1.0f;
	pattern->effect_colour[1]  = 1.0f;
	pattern->effect_colour[2]  = 1.0f;
	pattern->effect_colour[3]  = 1.0f;

	pattern->effect            = EFFECT_NONE;
	pattern->effect_padding    = 5;

	pattern->cursor_name       = strdup("pointer");
}

/* Calculate the dimensions of the visible part of the bar. */
static void calculate_dimensions (struct Lava_bar_pattern *pattern)
{
	switch (pattern->position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			pattern->orientation = ORIENTATION_VERTICAL;
			pattern->w = (uint32_t)(pattern->icon_size + pattern->border_right
					+ pattern->border_left);
			pattern->h = (uint32_t)(get_item_length_sum(pattern)
					+ pattern->border_top + pattern->border_bottom);
			if ( pattern->exclusive_zone == 1 )
				pattern->exclusive_zone = pattern->w;
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			pattern->orientation = ORIENTATION_HORIZONTAL;
			pattern->w = (uint32_t)(get_item_length_sum(pattern)
					+ pattern->border_left + pattern->border_right);
			pattern->h = (uint32_t)(pattern->icon_size + pattern->border_top
					+ pattern->border_bottom);
			if ( pattern->exclusive_zone == 1 )
				pattern->exclusive_zone = pattern->h;
			break;
	}
}

bool create_bar_pattern (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Creating bar pattern.\n", stderr);

	struct Lava_bar_pattern *pattern = calloc(1, sizeof(struct Lava_bar_pattern));
	if ( pattern == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return false;
	}

	sensible_defaults(pattern);
	pattern->data = data;
	data->last_pattern = pattern;
	wl_list_init(&pattern->items);
	wl_list_insert(&data->patterns, &pattern->link);

	return true;
}

bool finalize_bar_pattern (struct Lava_bar_pattern *pattern)
{
	if (pattern->data->verbose)
		fputs("Finalize bar pattern.\n", stderr);
	if (! finalize_items(pattern))
		return false;
	calculate_dimensions(pattern);
	return true;
}

static void destroy_bar_pattern (struct Lava_bar_pattern *pattern)
{
	wl_list_remove(&pattern->link);
	destroy_all_items(pattern);
	if ( pattern->only_output != NULL )
		free(pattern->only_output);
	if ( pattern->bar_colour_hex != NULL )
		free(pattern->bar_colour_hex);
	if ( pattern->border_colour_hex != NULL )
		free(pattern->border_colour_hex);
	if ( pattern->effect_colour_hex != NULL )
		free(pattern->effect_colour_hex);
	if ( pattern->cursor_name != NULL )
		free(pattern->cursor_name);
	free(pattern);
}

void destroy_all_bar_patterns (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Destroy all bar patterns.\n", stderr);
	struct Lava_bar_pattern *pt1, *pt2;
	wl_list_for_each_safe(pt1, pt2, &data->patterns, link)
		destroy_bar_pattern(pt1);
}

static bool bar_pattern_set_position (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "top"))
		pattern->position = POSITION_TOP;
	else if (! strcmp(arg, "right"))
		pattern->position = POSITION_RIGHT;
	else if (! strcmp(arg, "bottom"))
		pattern->position = POSITION_BOTTOM;
	else if (! strcmp(arg, "left"))
		pattern->position = POSITION_LEFT;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized position \"%s\".\n"
				"INFO: Possible positions are 'top', 'right', "
				"'bottom' and 'left'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_alignment (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "start"))
		pattern->alignment = ALIGNMENT_START;
	else if (! strcmp(arg, "center"))
		pattern->alignment = ALIGNMENT_CENTER;
	else if (! strcmp(arg, "end"))
		pattern->alignment = ALIGNMENT_END;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized alignment \"%s\".\n"
				"INFO: Possible alignments are 'start', "
				"'center' and 'end'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_mode (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "default"))
		pattern->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "full"))
		pattern->mode = MODE_FULL;
	else if (! strcmp(arg, "simple"))
		pattern->mode = MODE_SIMPLE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized mode \"%s\".\n"
				"INFO: Possible alignments are 'default', "
				"'full' and 'simple'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_layer (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "overlay"))
		pattern->layer = ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
	else if (! strcmp(arg, "top"))
		pattern->layer = ZWLR_LAYER_SHELL_V1_LAYER_TOP;
	else if (! strcmp(arg, "bottom"))
		pattern->layer = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
	else if (! strcmp(arg, "background"))
		pattern->layer = ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized layer \"%s\".\n"
				"INFO: Possible layers are 'overlay', "
				"'top', 'bottom', and 'background'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_icon_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	// TODO check issdigit()
	pattern->icon_size = atoi(arg);
	if ( pattern->icon_size <= 0 )
	{
		fputs("ERROR: Icon size must be greater than zero.\n", stderr);
		return false;
	}
	return true;
}

static bool bar_pattern_set_border_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
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
		case 't': pattern->border_top    = size; break;
		case 'r': pattern->border_right  = size; break;
		case 'b': pattern->border_bottom = size; break;
		case 'l': pattern->border_left   = size; break;
		case 'a':
			pattern->border_top    = size;
			pattern->border_right  = size;
			pattern->border_bottom = size;
			pattern->border_left   = size;
			break;
	}
	return true;
}

static bool bar_pattern_set_margin_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
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
		case 't': pattern->margin_top    = size; break;
		case 'r': pattern->margin_right  = size; break;
		case 'b': pattern->margin_bottom = size; break;
		case 'l': pattern->margin_left   = size; break;
	}
	return true;
}

static bool bar_pattern_set_only_output (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if ( pattern->only_output != NULL )
		free(pattern->only_output);
	if ( ! strcmp(arg, "all") || *arg == '*' )
		pattern->only_output = NULL;
	else
		pattern->only_output = strdup(arg);
	return true;
}

static bool bar_pattern_set_exclusive_zone (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (is_boolean_true(arg))
		pattern->exclusive_zone = 1;
	else if (is_boolean_false(arg))
		pattern->exclusive_zone = 0;
	else if (! strcmp(arg, "stationary"))
		pattern->exclusive_zone = -1;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
				"INFO: Possible options are 'true', "
				"'false' and 'stationary'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_bar_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! hex_to_rgba(arg, &(pattern->bar_colour[0]), &(pattern->bar_colour[1]),
				&(pattern->bar_colour[2]), &(pattern->bar_colour[3])))
		return false;
	if ( pattern->bar_colour_hex != NULL )
		free(pattern->bar_colour_hex);
	pattern->bar_colour_hex = strdup(arg);
	return true;
}

static bool bar_pattern_set_border_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (!  hex_to_rgba(arg, &(pattern->border_colour[0]), &(pattern->border_colour[1]),
				&(pattern->border_colour[2]), &(pattern->border_colour[3])))
		return false;
	if ( pattern->border_colour_hex != NULL )
		free(pattern->border_colour_hex);
	pattern->border_colour_hex = strdup(arg);
	return true;
}

static bool bar_pattern_set_effect_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (!  hex_to_rgba(arg, &(pattern->effect_colour[0]), &(pattern->effect_colour[1]),
				&(pattern->effect_colour[2]), &(pattern->effect_colour[3])))
		return false;
	if ( pattern->effect_colour_hex != NULL )
		free(pattern->effect_colour_hex);
	pattern->effect_colour_hex = strdup(arg);
	return true;
}

static bool bar_pattern_set_effect (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "none"))
		pattern->effect = EFFECT_NONE;
	else if (! strcmp(arg, "box"))
		pattern->effect = EFFECT_BOX;
	else if (! strcmp(arg, "phone"))
		pattern->effect = EFFECT_PHONE;
	else if (! strcmp(arg, "circle"))
		pattern->effect = EFFECT_CIRCLE;
	else
	{
		fprintf(stderr, "ERROR: Unrecognized effect \"%s\".\n"
				"INFO: Possible options are 'none', "
				"'box', 'phone' and 'circle'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_effect_padding (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	pattern->effect_padding = atoi(arg);
	if ( pattern->effect_padding < 0 )
	{
		fputs("ERROR: Effect padding size must be equal to or "
				"greater than zero.\n", stderr);
		return false;
	}
	return true;
}

static bool bar_pattern_set_cursor_name (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if ( pattern->cursor_name != NULL )
		free(pattern->cursor_name);
	pattern->cursor_name = strdup(arg);
	return true;
}

struct
{
	const char *variable, direction;
	bool (*set)(struct Lava_bar_pattern*, const char*, const char);
} pattern_configs[] = {
	{ .variable = "position",          .set = bar_pattern_set_position,       .direction = '0'},
	{ .variable = "alignment",         .set = bar_pattern_set_alignment,      .direction = '0'},
	{ .variable = "mode",              .set = bar_pattern_set_mode,           .direction = '0'},
	{ .variable = "layer",             .set = bar_pattern_set_layer,          .direction = '0'},
	{ .variable = "icon-size",         .set = bar_pattern_set_icon_size,      .direction = '0'},
	{ .variable = "border",            .set = bar_pattern_set_border_size,    .direction = 'a'},
	{ .variable = "border-top",        .set = bar_pattern_set_border_size,    .direction = 't'},
	{ .variable = "border-right",      .set = bar_pattern_set_border_size,    .direction = 'r'},
	{ .variable = "border-bottom",     .set = bar_pattern_set_border_size,    .direction = 'b'},
	{ .variable = "border-left",       .set = bar_pattern_set_border_size,    .direction = 'l'},
	{ .variable = "margin-top",        .set = bar_pattern_set_margin_size,    .direction = 't'},
	{ .variable = "margin-right",      .set = bar_pattern_set_margin_size,    .direction = 'r'},
	{ .variable = "margin-bottom",     .set = bar_pattern_set_margin_size,    .direction = 'b'},
	{ .variable = "margin-left",       .set = bar_pattern_set_margin_size,    .direction = 'l'},
	{ .variable = "output",            .set = bar_pattern_set_only_output,    .direction = '0'},
	{ .variable = "exclusive-zone",    .set = bar_pattern_set_exclusive_zone, .direction = '0'},
	{ .variable = "background-colour", .set = bar_pattern_set_bar_colour,     .direction = '0'},
	{ .variable = "border-colour",     .set = bar_pattern_set_border_colour,  .direction = '0'},
	{ .variable = "effect-colour",     .set = bar_pattern_set_effect_colour,  .direction = '0'},
	{ .variable = "effect",            .set = bar_pattern_set_effect,         .direction = '0'},
	{ .variable = "effect-padding",    .set = bar_pattern_set_effect_padding, .direction = '0'},
	{ .variable = "cursor-name",       .set = bar_pattern_set_cursor_name,    .direction = '0'}
};

bool bar_pattern_set_variable (struct Lava_bar_pattern *pattern,
		const char *variable, const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(pattern_configs) / sizeof(pattern_configs[0])); i++)
		if (! strcmp(pattern_configs[i].variable, variable))
			return pattern_configs[i].set(pattern, value,
					pattern_configs[i].direction);
	fprintf(stderr, "ERROR: Unrecognized bar pattern setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}
