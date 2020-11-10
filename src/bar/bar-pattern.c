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
#include<ctype.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"log.h"
#include"config.h"
#include"bar/bar-pattern.h"
#include"item/item.h"
#include"types/colour.h"
#include"types/string-container.h"

static void sensible_defaults (struct Lava_bar_pattern *pattern)
{
	pattern->position          = POSITION_BOTTOM;
	pattern->alignment         = ALIGNMENT_CENTER;
	pattern->mode              = MODE_DEFAULT;
	pattern->layer             = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	pattern->size                = 60;
	pattern->hidden_size         = 0;
	pattern->icon_padding        = 4;
	pattern->border.top          = 1;
	pattern->border.right        = 1;
	pattern->border.bottom       = 1;
	pattern->border.left         = 1;
	pattern->radius_top_left     = 5;
	pattern->radius_top_right    = 5;
	pattern->radius_bottom_left  = 5;
	pattern->radius_bottom_right = 5;
	pattern->margin.top          = 0;
	pattern->margin.right        = 0;
	pattern->margin.bottom       = 0;
	pattern->margin.left         = 0;
	pattern->exclusive_zone      = 1;
	pattern->indicator_padding   = 0;
	pattern->indicator_style     = STYLE_ROUNDED_RECTANGLE;

	colour_from_string(&pattern->bar_colour, "#000000");
	colour_from_string(&pattern->border_colour, "#ffffff");
	colour_from_string(&pattern->indicator_hover_colour, "#404040");
	colour_from_string(&pattern->indicator_active_colour, "#606060");

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

	pattern->cursor_name = NULL;
	pattern->only_output = NULL;
	pattern->namespace   = NULL;

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
	pattern->hidden_size       = last_pattern->hidden_size;
	pattern->icon_padding      = last_pattern->icon_padding;
	pattern->border.top        = last_pattern->border.top;
	pattern->border.right      = last_pattern->border.right;
	pattern->border.bottom     = last_pattern->border.bottom;
	pattern->border.left       = last_pattern->border.left;
	pattern->margin.top        = last_pattern->margin.top;
	pattern->margin.right      = last_pattern->margin.right;
	pattern->margin.bottom     = last_pattern->margin.bottom;
	pattern->margin.left       = last_pattern->margin.left;
	pattern->exclusive_zone    = last_pattern->exclusive_zone;
	pattern->indicator_padding = last_pattern->indicator_padding;
	pattern->indicator_style   = last_pattern->indicator_style;

	memcpy(&pattern->bar_colour, &last_pattern->bar_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->border_colour, &last_pattern->border_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->indicator_hover_colour, &last_pattern->indicator_hover_colour,
			sizeof(struct Lava_colour));
	memcpy(&pattern->indicator_active_colour, &last_pattern->indicator_active_colour,
			sizeof(struct Lava_colour));

	if ( last_pattern->cursor_name != NULL )
		pattern->cursor_name = string_container_reference(last_pattern->cursor_name);
	if ( last_pattern->only_output != NULL )
		pattern->only_output = string_container_reference(last_pattern->only_output);
	if ( last_pattern->namespace != NULL )
		pattern->namespace = string_container_reference(last_pattern->namespace);

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

	if ( pattern->icon_padding > pattern->size / 3 )
	{
		log_message(NULL, 0, "WARNING: Configured 'icon-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		pattern->icon_padding = pattern->size / 3;
	}
	if ( pattern->indicator_padding > pattern->size / 3 )
	{
		log_message(NULL, 0, "WARNING: Configured 'indicator-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		pattern->indicator_padding = pattern->size / 3;
	}

	return true;
}

static void destroy_bar_pattern (struct Lava_bar_pattern *pattern)
{
	wl_list_remove(&pattern->link);
	destroy_all_items(pattern);
	if ( pattern->cursor_name != NULL )
		string_container_destroy(pattern->cursor_name);
	if ( pattern->only_output != NULL )
		string_container_destroy(pattern->only_output);
	if ( pattern->namespace != NULL )
		string_container_destroy(pattern->namespace);
	free(pattern);
}

void destroy_all_bar_patterns (struct Lava_data *data)
{
	log_message(data, 1, "[bar-pattern] Destroying all bar patterns.\n");
	struct Lava_bar_pattern *pt1, *pt2;
	wl_list_for_each_safe(pt1, pt2, &data->patterns, link)
		destroy_bar_pattern(pt1);
}

static uint32_t count_args (const char *arg)
{
	uint32_t args = 0;
	bool on_arg = false;
	for (const char *i = arg; *i != '\0'; i++)
	{
		if (isspace(*i))
		{
			if (on_arg)
				on_arg = false;
		}
		else if (! on_arg)
		{
			on_arg = true;
			args++;
		}
	}
	return args;
}

static bool directional_config (uint32_t *_a, uint32_t *_b, uint32_t *_c, uint32_t *_d,
		const char *arg, const char *conf_name, const char *conf_name_2)
{
	int32_t a, b, c, d;

	if ( 4 == count_args(arg) && 4 == sscanf(arg, "%d %d %d %d", &a, &b, &c, &d) )
		goto done;

	if ( 1 == count_args(arg) && 1 == sscanf(arg, "%d", &d) )
	{
		a = b = c = d;
		goto done;
	}

	log_message(NULL, 0, "ERROR: Invalid %s configuration: %s\n"
			"INFO: You have to specify either one or four integers.\n",
			conf_name, arg);
	return false;

done:
	if ( a < 0 || b < 0 || c < 0 || d < 0 )
	{
		log_message(NULL, 0, "ERROR: %s can not be negative.\n", conf_name_2);
		return false;
	}
	
	*_a = (uint32_t)a;
	*_b = (uint32_t)b;
	*_c = (uint32_t)c;
	*_d = (uint32_t)d;

	return true;
}

static bool bar_pattern_set_border_size (struct Lava_bar_pattern *pattern, const char *arg)
{
	return directional_config(&pattern->border.top, &pattern->border.right,
			&pattern->border.bottom, &pattern->border.left,
			arg, "border", "Border size");
}

static bool bar_pattern_set_margin_size (struct Lava_bar_pattern *pattern, const char *arg)
{
	return directional_config(&pattern->margin.top, &pattern->margin.right,
			&pattern->margin.bottom, &pattern->margin.left,
			arg, "margin", "Margins");
}

static bool bar_pattern_set_radius (struct Lava_bar_pattern *pattern, const char *arg)
{
	return directional_config(&pattern->radius_top_left, &pattern->radius_top_right,
			&pattern->radius_bottom_left, &pattern->radius_bottom_right,
			arg, "radius", "Radii");
}

static bool bar_pattern_set_position (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_alignment (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_mode (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_layer (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_size (struct Lava_bar_pattern *pattern, const char *arg)
{
	int32_t size = atoi(arg);
	if ( size <= 0 )
	{
		log_message(NULL, 0, "ERROR: Size must be greater than zero.\n");
		return false;
	}
	pattern->size = (uint32_t)size;
	return true;
}

static bool bar_pattern_set_icon_padding (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_only_output (struct Lava_bar_pattern *pattern, const char *arg)
{
	if ( pattern->only_output != NULL )
		string_container_destroy(pattern->only_output);

	if ( strcmp(arg, "all") && *arg != '*' )
		pattern->only_output = string_container_from(arg);

	return true;
}

static bool bar_pattern_set_namespace (struct Lava_bar_pattern *pattern, const char *arg)
{
	if ( pattern->namespace != NULL )
		string_container_destroy(pattern->namespace);

	pattern->namespace = string_container_from(arg);

	return true;
}

static bool bar_pattern_set_exclusive_zone (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_hidden_size (struct Lava_bar_pattern *pattern, const char *arg)
{
	int32_t hidden_size = atoi(arg);
	if ( hidden_size < 0 )
	{
		log_message(NULL, 0, "ERROR: Hidden size may not be smaller than zero.\n");
		return false;
	}
	pattern->hidden_size = (uint32_t)hidden_size;
	return true;
}

static bool bar_pattern_set_bar_colour (struct Lava_bar_pattern *pattern, const char *arg)
{
	return colour_from_string(&pattern->bar_colour, arg);
}

static bool bar_pattern_set_border_colour (struct Lava_bar_pattern *pattern, const char *arg)
{
	return colour_from_string(&pattern->border_colour, arg);
}

static bool bar_pattern_set_cursor_name (struct Lava_bar_pattern *pattern, const char *arg)
{
	if ( pattern->cursor_name != NULL )
		string_container_destroy(pattern->cursor_name);
	pattern->cursor_name = string_container_from(arg);
	return true;
}

static bool bar_pattern_set_condition_scale (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_condition_resolution (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_condition_transform (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_indicator_padding (struct Lava_bar_pattern *pattern, const char *arg)
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

static bool bar_pattern_set_indicator_colour_active (struct Lava_bar_pattern *pattern, const char *arg)
{
	return colour_from_string(&pattern->indicator_active_colour, arg);
}

static bool bar_pattern_set_indicator_colour_hover (struct Lava_bar_pattern *pattern, const char *arg)
{
	return colour_from_string(&pattern->indicator_hover_colour, arg);
}

static bool bar_pattern_set_indicator_style (struct Lava_bar_pattern *pattern, const char *arg)
{
	if (! strcmp(arg, "rectangle"))
		pattern->indicator_style = STYLE_RECTANGLE;
	else if (! strcmp(arg, "rounded-rectangle"))
		pattern->indicator_style = STYLE_ROUNDED_RECTANGLE;
	else if (! strcmp(arg, "circle"))
		pattern->indicator_style = STYLE_CIRCLE;
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized indicator style \"%s\".\n"
				"INFO: Possible styles are 'rectangle', "
				"'rounded-rectangle' and 'circle'.\n", arg);
		return false;
	}
	return true;
}


bool bar_pattern_set_variable (struct Lava_bar_pattern *pattern,
		const char *variable, const char *value, int line)
{
	struct
	{
		const char *variable;
		bool (*set)(struct Lava_bar_pattern*, const char*);
	} configs[] = {
		{ .variable = "alignment",               .set = bar_pattern_set_alignment               },
		{ .variable = "background-colour",       .set = bar_pattern_set_bar_colour              },
		{ .variable = "border-colour",           .set = bar_pattern_set_border_colour           },
		{ .variable = "border",                  .set = bar_pattern_set_border_size             },
		{ .variable = "condition-resolution",    .set = bar_pattern_set_condition_resolution    },
		{ .variable = "condition-scale",         .set = bar_pattern_set_condition_scale         },
		{ .variable = "condition-transform",     .set = bar_pattern_set_condition_transform     },
		{ .variable = "cursor-name",             .set = bar_pattern_set_cursor_name             },
		{ .variable = "exclusive-zone",          .set = bar_pattern_set_exclusive_zone          },
		{ .variable = "hidden-size",             .set = bar_pattern_set_hidden_size             },
		{ .variable = "icon-padding",            .set = bar_pattern_set_icon_padding            },
		{ .variable = "indicator-active-colour", .set = bar_pattern_set_indicator_colour_active },
		{ .variable = "indicator-hover-colour",  .set = bar_pattern_set_indicator_colour_hover  },
		{ .variable = "indicator-padding",       .set = bar_pattern_set_indicator_padding       },
		{ .variable = "indicator-style",         .set = bar_pattern_set_indicator_style,        },
		{ .variable = "layer",                   .set = bar_pattern_set_layer                   },
		{ .variable = "margin",                  .set = bar_pattern_set_margin_size             },
		{ .variable = "mode",                    .set = bar_pattern_set_mode                    },
		{ .variable = "namespace",               .set = bar_pattern_set_namespace               },
		{ .variable = "output",                  .set = bar_pattern_set_only_output             },
		{ .variable = "position",                .set = bar_pattern_set_position                },
		{ .variable = "radius",                  .set = bar_pattern_set_radius                  },
		{ .variable = "size",                    .set = bar_pattern_set_size                    }
	};

	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
		{
			if (configs[i].set(pattern, value))
				return true;
			goto exit;
		}

	log_message(NULL, 0, "ERROR: Unrecognized bar setting \"%s\".\n", variable);
exit:
	log_message(NULL, 0, "INFO: The error is on line %d in \"%s\".\n",
			line, pattern->data->config_path);
	return false;
}

