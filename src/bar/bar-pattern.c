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
#include"log.h"
#include"bar/bar-pattern.h"
#include"item/item.h"
#include"config/parse-boolean.h"
#include"types/colour.h"

static void sensible_defaults (struct Lava_bar_pattern *pattern)
{
	pattern->position          = POSITION_BOTTOM;
	pattern->alignment         = ALIGNMENT_CENTER;
	pattern->mode              = MODE_DEFAULT;
	pattern->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	pattern->size                = 60;
	pattern->icon_padding        = 4;
	pattern->border_top          = 1;
	pattern->border_right        = 1;
	pattern->border_bottom       = 1;
	pattern->border_left         = 1;
	pattern->radius_top_left     = 5;
	pattern->radius_top_right    = 5;
	pattern->radius_bottom_left  = 5;
	pattern->radius_bottom_right = 5;
	pattern->margin_top          = 0;
	pattern->margin_right        = 0;
	pattern->margin_bottom       = 0;
	pattern->margin_left         = 0;
	pattern->exclusive_zone      = 1;
	pattern->indicator_padding   = 0;

	colour_from_string(&pattern->bar_colour, "#000000");
	colour_from_string(&pattern->border_colour, "#ffffff");
	colour_from_string(&pattern->indicator_hover_colour, "#404040");
	colour_from_string(&pattern->indicator_active_colour, "#606060");

	strncpy(pattern->cursor_name, "pointer", sizeof(pattern->cursor_name) - 1);

	pattern->condition_scale      = 0;
	pattern->condition_transform  = -1;
	pattern->condition_resolution = RESOLUTION_ALL;
}

bool create_bar_pattern (struct Lava_data *data)
{
	log_message(data, 1, "[bar-pattern] Creating bar pattern.\n");

	struct Lava_bar_pattern *pattern = calloc(1, sizeof(struct Lava_bar_pattern));
	if ( pattern == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	memset(pattern->cursor_name, '\0', sizeof(pattern->cursor_name));
	memset(pattern->only_output, '\0', sizeof(pattern->only_output));

	sensible_defaults(pattern);
	pattern->data = data;
	data->last_pattern = pattern;
	wl_list_init(&pattern->items);
	wl_list_insert(&data->patterns, &pattern->link);

	return true;
}

bool copy_last_bar_pattern (struct Lava_data *data)
{
	log_message(data, 1, "[bar-pattern] Copying last bar pattern.\n");

	if ( data->last_pattern == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not copy bar. There is no previous bar to copy.\n");
		return false;
	}

	struct Lava_bar_pattern *last_pattern = data->last_pattern;
	if (! create_bar_pattern(data))
		return false;
	struct Lava_bar_pattern *pattern = data->last_pattern;

	/* Copy all settings. */
	pattern->position          = last_pattern->position;
	pattern->alignment         = last_pattern->alignment;
	pattern->mode              = last_pattern->mode;
	pattern->layer             = last_pattern->layer;
	pattern->size              = last_pattern->size;
	pattern->icon_padding      = last_pattern->icon_padding;
	pattern->border_top        = last_pattern->border_top;
	pattern->border_right      = last_pattern->border_right;
	pattern->border_bottom     = last_pattern->border_bottom;
	pattern->border_left       = last_pattern->border_left;
	pattern->margin_top        = last_pattern->margin_top;
	pattern->margin_right      = last_pattern->margin_right;
	pattern->margin_bottom     = last_pattern->margin_bottom;
	pattern->margin_left       = last_pattern->margin_left;
	pattern->exclusive_zone    = last_pattern->exclusive_zone;
	pattern->indicator_padding = last_pattern->indicator_padding;

	memcpy(&pattern->bar_colour, &last_pattern->bar_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->border_colour, &last_pattern->border_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->indicator_hover_colour, &last_pattern->indicator_hover_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->indicator_active_colour, &last_pattern->indicator_active_colour,
			sizeof(struct Lava_colour));

	/* These strings are guaranteed to be terminated in last_pattern,
	 * so we copy them entirely to avoid false compiler warnings when
	 * -Wstringop-truncation is used.
	 */
	strncpy(pattern->cursor_name, last_pattern->cursor_name,
			sizeof(pattern->cursor_name));
	strncpy(pattern->only_output, last_pattern->only_output,
			sizeof(pattern->only_output));

	struct Lava_item *item, *temp;
	wl_list_for_each_reverse_safe(item, temp, &last_pattern->items, link)
		if (! copy_item(pattern, item))
				return false;

	return true;
}

bool finalize_bar_pattern (struct Lava_bar_pattern *pattern)
{
	log_message(pattern->data, 1, "[bar-pattern] Finalize bar pattern.\n");
	if (! finalize_items(pattern))
		return false;
	switch (pattern->position)
	{
		case POSITION_TOP:
		case POSITION_BOTTOM:
			pattern->orientation = ORIENTATION_HORIZONTAL;
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			pattern->orientation = ORIENTATION_VERTICAL;
			break;
	}
	return true;
}

static void destroy_bar_pattern (struct Lava_bar_pattern *pattern)
{
	wl_list_remove(&pattern->link);
	destroy_all_items(pattern);
	free(pattern);
}

void destroy_all_bar_patterns (struct Lava_data *data)
{
	log_message(data, 1, "[bar-pattern] Destroying all bar patterns.\n");
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
		log_message(NULL, 0, "ERROR: Unrecognized position \"%s\".\n"
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
		log_message(NULL, 0, "ERROR: Unrecognized alignment \"%s\".\n"
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
		log_message(NULL, 0, "ERROR: Unrecognized mode \"%s\".\n"
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
		log_message(NULL, 0, "ERROR: Unrecognized layer \"%s\".\n"
				"INFO: Possible layers are 'overlay', "
				"'top', 'bottom', and 'background'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	// TODO check issdigit()
	pattern->size = (uint32_t)atoi(arg);
	if ( pattern->size <= 0 )
	{
		log_message(NULL, 0, "ERROR: Size must be greater than zero.\n");
		return false;
	}
	return true;
}

static bool bar_pattern_set_icon_padding (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	int32_t temp = atoi(arg);
	if ( temp < 0 )
	{
		log_message(NULL, 0, "ERROR: Icon padding must be greater than or equal to zero.\n");
		return false;
	}
	pattern->icon_padding = (uint32_t)temp;
	return true;
}

static bool bar_pattern_set_border_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(NULL, 0, "ERROR: Border size must be equal to or greater than zero.\n");
		return false;
	}

	switch (direction)
	{
		case 't': pattern->border_top    = (uint32_t)size; break;
		case 'r': pattern->border_right  = (uint32_t)size; break;
		case 'b': pattern->border_bottom = (uint32_t)size; break;
		case 'l': pattern->border_left   = (uint32_t)size; break;
		case 'a':
			pattern->border_top    = (uint32_t)size;
			pattern->border_right  = (uint32_t)size;
			pattern->border_bottom = (uint32_t)size;
			pattern->border_left   = (uint32_t)size;
			break;
	}
	return true;
}

static bool bar_pattern_set_margin_size (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(NULL, 0, "ERROR: Margin size must be equal to or greater than zero.\n");
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
	if ( ! strcmp(arg, "all") || *arg == '*' )
		pattern->only_output[0] = '\0';
	else
		strncpy(pattern->only_output, arg, sizeof(pattern->only_output) - 1);
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
		log_message(NULL, 0, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
				"INFO: Possible options are 'true', "
				"'false' and 'stationary'.\n", arg);
		return false;
	}
	return true;
}

static bool bar_pattern_set_bar_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	return colour_from_string(&pattern->bar_colour, arg);
}

static bool bar_pattern_set_border_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	return colour_from_string(&pattern->border_colour, arg);
}

static bool bar_pattern_set_cursor_name (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	strncpy(pattern->cursor_name, arg, sizeof(pattern->cursor_name) - 1);
	return true;
}

static bool bar_pattern_set_condition_scale (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "all"))
	{
		pattern->condition_scale = 0;
		return true;
	}

	int32_t temp = atoi(arg);
	if ( temp <= 0 )
	{
		log_message(NULL, 0, "ERROR: Scale condition must be an integer greater than  zero or 'all'.\n");
		return false;
	}
	pattern->condition_scale = (uint32_t)temp;
	return true;
}

static bool bar_pattern_set_condition_resolution (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "all"))
		pattern->condition_resolution = RESOLUTION_ALL;
	else if (! strcmp(arg, "wider-than-high"))
		pattern->condition_resolution = RESOLUTION_WIDER_THAN_HIGH;
	else if (! strcmp(arg, "higher-than-wide"))
		pattern->condition_resolution = RESOLUTION_HIGHER_THEN_WIDE;
	else
	{
		log_message(NULL, 0, "ERROR: Resolution condition can be 'all', 'wider-than-high' or 'higher-than-wide'.\n" );
		return false;
	}

	return true;
}

static bool bar_pattern_set_condition_transform (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if (! strcmp(arg, "all"))
	{
		pattern->condition_transform = -1;
		return true;
	}

	pattern->condition_transform = atoi(arg);
	if ( pattern->condition_transform < 0 || pattern->condition_transform > 4 )
	{
		log_message(NULL, 0, "ERROR: Transform condition can be 0, 1, 2, 3 or 'all'.\n");
		return false;
	}
	return true;
}

static bool bar_pattern_set_radius (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(NULL, 0, "ERROR: Radius must be equal to or greater than zero.\n");
		return false;
	}

	switch (direction)
	{
		case 'l': pattern->radius_top_left     = (uint32_t)size; break;
		case 'r': pattern->radius_top_right    = (uint32_t)size; break;
		case 'L': pattern->radius_bottom_left  = (uint32_t)size; break;
		case 'R': pattern->radius_bottom_right = (uint32_t)size; break;
		case 'a':
			pattern->radius_top_left     = (uint32_t)size;
			pattern->radius_top_right    = (uint32_t)size;
			pattern->radius_bottom_left  = (uint32_t)size;
			pattern->radius_bottom_right = (uint32_t)size;
			break;
	}
	return true;
}

static bool bar_pattern_set_indicator_padding (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(NULL, 0, "ERROR: Indicator padding must be equal to or greater than zero.\n");
		return false;
	}
	pattern->indicator_padding = (uint32_t)size;
	return true;
}

static bool bar_pattern_set_indicator_colour (struct Lava_bar_pattern *pattern,
		const char *arg, const char direction)
{
	if ( direction == 'a')
		return colour_from_string(&pattern->indicator_active_colour, arg);
	else if ( direction == 'h')
		return colour_from_string(&pattern->indicator_hover_colour, arg);
	else
		return false;
}


struct
{
	const char *variable, direction;
	bool (*set)(struct Lava_bar_pattern*, const char*, const char);
} pattern_configs[] = {
	{ .variable = "position",                .set = bar_pattern_set_position,             .direction = '0'},
	{ .variable = "alignment",               .set = bar_pattern_set_alignment,            .direction = '0'},
	{ .variable = "mode",                    .set = bar_pattern_set_mode,                 .direction = '0'},
	{ .variable = "layer",                   .set = bar_pattern_set_layer,                .direction = '0'},
	{ .variable = "size",                    .set = bar_pattern_set_size,                 .direction = '0'},
	{ .variable = "icon-padding",            .set = bar_pattern_set_icon_padding,         .direction = '0'},
	{ .variable = "border",                  .set = bar_pattern_set_border_size,          .direction = 'a'},
	{ .variable = "border-top",              .set = bar_pattern_set_border_size,          .direction = 't'},
	{ .variable = "border-right",            .set = bar_pattern_set_border_size,          .direction = 'r'},
	{ .variable = "border-bottom",           .set = bar_pattern_set_border_size,          .direction = 'b'},
	{ .variable = "border-left",             .set = bar_pattern_set_border_size,          .direction = 'l'},
	{ .variable = "margin-top",              .set = bar_pattern_set_margin_size,          .direction = 't'},
	{ .variable = "margin-right",            .set = bar_pattern_set_margin_size,          .direction = 'r'},
	{ .variable = "margin-bottom",           .set = bar_pattern_set_margin_size,          .direction = 'b'},
	{ .variable = "margin-left",             .set = bar_pattern_set_margin_size,          .direction = 'l'},
	{ .variable = "output",                  .set = bar_pattern_set_only_output,          .direction = '0'},
	{ .variable = "exclusive-zone",          .set = bar_pattern_set_exclusive_zone,       .direction = '0'},
	{ .variable = "background-colour",       .set = bar_pattern_set_bar_colour,           .direction = '0'},
	{ .variable = "border-colour",           .set = bar_pattern_set_border_colour,        .direction = '0'},
	{ .variable = "cursor-name",             .set = bar_pattern_set_cursor_name,          .direction = '0'},
	{ .variable = "condition-scale",         .set = bar_pattern_set_condition_scale,      .direction = '0'},
	{ .variable = "condition-resolution",    .set = bar_pattern_set_condition_resolution, .direction = '0'},
	{ .variable = "condition-transform",     .set = bar_pattern_set_condition_transform,  .direction = '0'},
	{ .variable = "radius",                  .set = bar_pattern_set_radius,               .direction = 'a'},
	{ .variable = "radius-top-left",         .set = bar_pattern_set_radius,               .direction = 'l'},
	{ .variable = "radius-top-right",        .set = bar_pattern_set_radius,               .direction = 'r'},
	{ .variable = "radius-bottom-left",      .set = bar_pattern_set_radius,               .direction = 'L'},
	{ .variable = "radius-bottom-right",     .set = bar_pattern_set_radius,               .direction = 'R'},
	{ .variable = "indicator-padding",       .set = bar_pattern_set_indicator_padding,    .direction = '0'},
	{ .variable = "indicator-active-colour", .set = bar_pattern_set_indicator_colour,     .direction = 'a'},
	{ .variable = "indicator-hover-colour",  .set = bar_pattern_set_indicator_colour,     .direction = 'h'}
};

bool bar_pattern_set_variable (struct Lava_bar_pattern *pattern,
		const char *variable, const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(pattern_configs) / sizeof(pattern_configs[0])); i++)
		if (! strcmp(pattern_configs[i].variable, variable))
			return pattern_configs[i].set(pattern, value,
					pattern_configs[i].direction);
	log_message(NULL, 0, "ERROR: Unrecognized bar setting \"%s\" on line %d\n", variable, line);
	return false;
}

