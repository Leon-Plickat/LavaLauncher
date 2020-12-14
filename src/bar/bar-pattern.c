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
#include"str.h"
#include"config.h"
#include"item.h"
#include"bar/bar-pattern.h"
#include"types/colour_t.h"
#include"types/string_t.h"
#include"types/box_t.h"

static void bar_config_sensible_defaults (struct Lava_bar_configuration *config)
{
	config->position  = POSITION_BOTTOM;
	config->alignment = ALIGNMENT_CENTER;
	config->mode      = MODE_DEFAULT;
	config->layer     = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	config->size              = 60;
	config->hidden_size       = 10;
	config->hidden_mode       = HIDDEN_MODE_NEVER;
	config->icon_padding      = 4;
	config->exclusive_zone    = 1;
	config->indicator_padding = 0;
	config->indicator_style   = STYLE_ROUNDED_RECTANGLE;

	udirections_t_set_all(&config->border, 1);
	udirections_t_set_all(&config->margin, 0);
	uradii_t_set_all(&config->radii, 5);

	colour_t_from_string(&config->bar_colour, "#000000");
	colour_t_from_string(&config->border_colour, "#ffffff");
	colour_t_from_string(&config->indicator_hover_colour, "#404040");
	colour_t_from_string(&config->indicator_active_colour, "#606060");

	config->condition_scale      = 0;
	config->condition_transform  = -1;
	config->condition_resolution = RESOLUTION_ALL;

	config->cursor_name = NULL;
	config->only_output = NULL;
	config->namespace   = NULL;
}

static void bar_config_copy_settings (struct Lava_bar_configuration *config,
		struct Lava_bar_configuration *default_config)
{
	config->position  = default_config->position;
	config->alignment = default_config->alignment;
	config->mode      = default_config->mode;
	config->layer     = default_config->layer;

	config->size              = default_config->size;
	config->hidden_size       = default_config->hidden_size;
	config->hidden_mode       = default_config->hidden_mode;
	config->icon_padding      = default_config->icon_padding;
	config->exclusive_zone    = default_config->exclusive_zone;
	config->indicator_padding = default_config->indicator_padding;
	config->indicator_style   = default_config->indicator_style;

	memcpy(&config->border, &default_config->border, sizeof(udirections_t));
	memcpy(&config->margin, &default_config->margin, sizeof(udirections_t));
	memcpy(&config->radii, &default_config->radii, sizeof(uradii_t));

	memcpy(&config->bar_colour, &default_config->bar_colour, sizeof(colour_t));
	memcpy(&config->border_colour, &default_config->border_colour, sizeof(colour_t));
	memcpy(&config->indicator_hover_colour, &default_config->indicator_hover_colour, sizeof(colour_t));
	memcpy(&config->indicator_active_colour, &default_config->indicator_active_colour, sizeof(colour_t));

	config->condition_scale      = default_config->condition_scale;
	config->condition_transform  = default_config->condition_transform;
	config->condition_resolution = default_config->condition_resolution;

	config->cursor_name = default_config->cursor_name == NULL ? NULL : strdup(default_config->cursor_name);
	config->only_output = default_config->only_output == NULL ? NULL : strdup(default_config->only_output);
	config->namespace = default_config->namespace == NULL ? NULL : strdup(default_config->namespace);
}

struct Lava_bar_configuration *pattern_get_first_config (struct Lava_bar_pattern *pattern)
{
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &pattern->configs, link)
		return config;
	return NULL;
}

struct Lava_bar_configuration *pattern_get_last_config (struct Lava_bar_pattern *pattern)
{
	int len = wl_list_length(&pattern->configs) - 1, i = 0;
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &pattern->configs, link)
		if ( i++ == len )
			return config;
	return NULL;
}

bool create_bar_config (struct Lava_bar_pattern *pattern, bool default_config)
{
	struct Lava_bar_configuration *config = calloc(1, sizeof(struct Lava_bar_configuration));
	if ( config == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	if (default_config)
		bar_config_sensible_defaults(config);
	else
		bar_config_copy_settings(config, pattern_get_first_config(pattern));

	wl_list_insert(&pattern->configs, &config->link);
	return true;
}

static void destroy_bar_config (struct Lava_bar_configuration *config)
{
	free_if_set(config->cursor_name);
	free_if_set(config->only_output);
	free_if_set(config->namespace);
	free(config);
}

static void destroy_all_bar_configs (struct Lava_bar_pattern *pattern)
{
	struct Lava_bar_configuration *config, *temp;
	wl_list_for_each_safe(config, temp, &pattern->configs, link)
		destroy_bar_config(config);
}

static void finalize_bar_config (struct Lava_bar_configuration *config)
{
	switch (config->position)
	{
		case POSITION_TOP:
		case POSITION_BOTTOM:
			config->orientation = ORIENTATION_HORIZONTAL;
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			config->orientation = ORIENTATION_VERTICAL;
			break;
	}

	if ( config->icon_padding > config->size / 3 )
	{
		log_message(NULL, 0, "WARNING: Configured 'icon-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		config->icon_padding = config->size / 3;
	}
	if ( config->indicator_padding > config->size / 3 )
	{
		log_message(NULL, 0, "WARNING: Configured 'indicator-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		config->indicator_padding = config->size / 3;
	}
}

static void finalize_all_bar_configs (struct Lava_bar_pattern *pattern)
{
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &pattern->configs, link)
		finalize_bar_config(config);
}

bool create_bar_pattern (struct Lava_data *data)
{
	struct Lava_bar_pattern *pattern = calloc(1, sizeof(struct Lava_bar_pattern));
	if ( pattern == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	pattern->data = data;
	data->last_pattern = pattern; // TODO we don't actually need this,
	                              //  -> implement get_last_pattern()

	wl_list_init(&pattern->items);
	wl_list_init(&pattern->configs);

	/* Create default configuration. */
	if (! create_bar_config(pattern, true))
	{
		free(pattern);
		return false;
	}

	wl_list_insert(&data->patterns, &pattern->link);

	return true;
}

bool finalize_bar_pattern (struct Lava_bar_pattern *pattern)
{
	log_message(pattern->data, 1, "[bar-pattern] Finalize bar pattern.\n");
	if (! finalize_items(pattern))
		return false;
	finalize_all_bar_configs(pattern);
	return true;
}

static void destroy_bar_pattern (struct Lava_bar_pattern *pattern)
{
	wl_list_remove(&pattern->link);
	destroy_all_items(pattern);
	destroy_all_bar_configs(pattern);
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

/****************************
 * Bar config configuration *
 ****************************/
#define BAR_CONFIG(A) static bool A (struct Lava_bar_configuration *config, struct Lava_data *data, const char *arg)
#define BAR_CONFIG_COLOUR(A, B) \
	static bool A (struct Lava_bar_configuration *config, struct Lava_data *data, const char *arg) \
	{ \
		return colour_t_from_string(&config->B, arg); \
	}
#define BAR_CONFIG_STRING(A, B) \
	static bool A (struct Lava_bar_configuration *config, struct Lava_data *data, const char *arg) \
	{ \
		set_string(&config->B, (char *)arg); \
		return true; \
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

BAR_CONFIG_STRING(bar_config_set_cursor_name, cursor_name)
BAR_CONFIG_STRING(bar_config_set_only_output, only_output)
BAR_CONFIG_STRING(bar_config_set_namespace, namespace)

BAR_CONFIG_COLOUR(bar_config_set_bar_colour, bar_colour)
BAR_CONFIG_COLOUR(bar_config_set_border_colour, border_colour)
BAR_CONFIG_COLOUR(bar_config_set_indicator_colour_active, indicator_active_colour)
BAR_CONFIG_COLOUR(bar_config_set_indicator_colour_hover, indicator_hover_colour)

BAR_CONFIG(bar_config_set_border_size)
{
	return directional_config(&config->border.top, &config->border.right,
			&config->border.bottom, &config->border.left,
			arg, "border", "Border size");
}

BAR_CONFIG(bar_config_set_margin_size)
{
	return directional_config(&config->margin.top, &config->margin.right,
			&config->margin.bottom, &config->margin.left,
			arg, "margin", "Margins");
}

BAR_CONFIG(bar_config_set_radius)
{
	return directional_config(&config->radii.top_left, &config->radii.top_right,
			&config->radii.bottom_left, &config->radii.bottom_right,
			arg, "radius", "Radii");
}

BAR_CONFIG(bar_config_set_position)
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
		log_message(NULL, 0, "ERROR: Unrecognized position \"%s\".\n"
				"INFO: Possible positions are 'top', 'right', "
				"'bottom' and 'left'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_alignment)
{
	if (! strcmp(arg, "start"))
		config->alignment = ALIGNMENT_START;
	else if (! strcmp(arg, "center"))
		config->alignment = ALIGNMENT_CENTER;
	else if (! strcmp(arg, "end"))
		config->alignment = ALIGNMENT_END;
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized alignment \"%s\".\n"
				"INFO: Possible alignments are 'start', "
				"'center' and 'end'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_mode)
{
	if (! strcmp(arg, "default"))
		config->mode = MODE_DEFAULT;
	else if (! strcmp(arg, "full"))
		config->mode = MODE_FULL;
	else if (! strcmp(arg, "aggressive"))
		config->mode = MODE_AGGRESSIVE;
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized mode \"%s\".\n"
				"INFO: Possible alignments are 'default', "
				"'full' and 'simple'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_layer)
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
		log_message(NULL, 0, "ERROR: Unrecognized layer \"%s\".\n"
				"INFO: Possible layers are 'overlay', "
				"'top', 'bottom', and 'background'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_size)
{
	int32_t size = atoi(arg);
	if ( size <= 0 )
	{
		log_message(NULL, 0, "ERROR: Size must be greater than zero.\n");
		return false;
	}
	config->size = (uint32_t)size;
	return true;
}

BAR_CONFIG(bar_config_set_icon_padding)
{
	int32_t temp = atoi(arg);
	if ( temp < 0 )
	{
		log_message(NULL, 0, "ERROR: Icon padding must be greater than or equal to zero.\n");
		return false;
	}
	config->icon_padding = (uint32_t)temp;
	return true;
}

BAR_CONFIG(bar_config_set_exclusive_zone)
{
	if (is_boolean_true(arg))
		config->exclusive_zone = 1;
	else if (is_boolean_false(arg))
		config->exclusive_zone = 0;
	else if (! strcmp(arg, "stationary"))
		config->exclusive_zone = -1;
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
				"INFO: Possible options are 'true', "
				"'false' and 'stationary'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_hidden_size)
{
	int32_t hidden_size = atoi(arg);
	if ( hidden_size < 1 )
	{
		log_message(NULL, 0, "ERROR: Hidden size may not be smaller than 1.\n");
		return false;
	}
	config->hidden_size = (uint32_t)hidden_size;
	return true;
}

BAR_CONFIG(bar_config_set_hidden_mode)
{
	if (! strcmp(arg, "never"))
		config->hidden_mode = HIDDEN_MODE_NEVER;
	else if (! strcmp(arg, "always"))
		config->hidden_mode = HIDDEN_MODE_ALWAYS;
	else if (! strcmp(arg, "river-auto"))
	{
		config->hidden_mode = HIDDEN_MODE_RIVER_AUTO;
		data->need_river_status = true;
	}
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized hidden mode option \"%s\".\n"
				"INFO: Possible options are 'always', 'never' and 'river-auto'.\n", arg);
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_condition_scale)
{
	if (! strcmp(arg, "all"))
	{
		config->condition_scale = 0;
		return true;
	}

	int32_t temp = atoi(arg);
	if ( temp <= 0 )
	{
		log_message(NULL, 0, "ERROR: Scale condition must be an integer greater than  zero or 'all'.\n");
		return false;
	}
	config->condition_scale = (uint32_t)temp;
	return true;
}

BAR_CONFIG(bar_config_set_condition_resolution)
{
	if (! strcmp(arg, "all"))
		config->condition_resolution = RESOLUTION_ALL;
	else if (! strcmp(arg, "wider-than-high"))
		config->condition_resolution = RESOLUTION_WIDER_THAN_HIGH;
	else if (! strcmp(arg, "higher-than-wide"))
		config->condition_resolution = RESOLUTION_HIGHER_THEN_WIDE;
	else
	{
		log_message(NULL, 0, "ERROR: Resolution condition can be 'all', 'wider-than-high' or 'higher-than-wide'.\n" );
		return false;
	}

	return true;
}

BAR_CONFIG(bar_config_set_condition_transform)
{
	if (! strcmp(arg, "all"))
	{
		config->condition_transform = -1;
		return true;
	}

	config->condition_transform = atoi(arg);
	if ( config->condition_transform < 0 || config->condition_transform > 4 )
	{
		log_message(NULL, 0, "ERROR: Transform condition can be 0, 1, 2, 3 or 'all'.\n");
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_indicator_padding)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(NULL, 0, "ERROR: Indicator padding must be equal to or greater than zero.\n");
		return false;
	}
	config->indicator_padding = (uint32_t)size;
	return true;
}

BAR_CONFIG(bar_config_set_indicator_style)
{
	if (! strcmp(arg, "rectangle"))
		config->indicator_style = STYLE_RECTANGLE;
	else if (! strcmp(arg, "rounded-rectangle"))
		config->indicator_style = STYLE_ROUNDED_RECTANGLE;
	else if (! strcmp(arg, "circle"))
		config->indicator_style = STYLE_CIRCLE;
	else
	{
		log_message(NULL, 0, "ERROR: Unrecognized indicator style \"%s\".\n"
				"INFO: Possible styles are 'rectangle', "
				"'rounded-rectangle' and 'circle'.\n", arg);
		return false;
	}
	return true;
}

#undef BAR_CONFIG
#undef BAR_CONFIG_STRING
#undef BAR_CONFIG_COLOUR

bool bar_config_set_variable (struct Lava_bar_configuration *config,
		struct Lava_data *data, const char *variable, const char *value,
		int line)
{
	struct
	{
		const char *variable;
		bool (*set)(struct Lava_bar_configuration*, struct Lava_data*, const char*);
	} configs[] = {
		{ .variable = "alignment",               .set = bar_config_set_alignment               },
		{ .variable = "background-colour",       .set = bar_config_set_bar_colour              },
		{ .variable = "border-colour",           .set = bar_config_set_border_colour           },
		{ .variable = "border",                  .set = bar_config_set_border_size             },
		{ .variable = "condition-resolution",    .set = bar_config_set_condition_resolution    },
		{ .variable = "condition-scale",         .set = bar_config_set_condition_scale         },
		{ .variable = "condition-transform",     .set = bar_config_set_condition_transform     },
		{ .variable = "cursor-name",             .set = bar_config_set_cursor_name             },
		{ .variable = "exclusive-zone",          .set = bar_config_set_exclusive_zone          },
		{ .variable = "hidden-size",             .set = bar_config_set_hidden_size             },
		{ .variable = "hidden-mode",             .set = bar_config_set_hidden_mode             },
		{ .variable = "icon-padding",            .set = bar_config_set_icon_padding            },
		{ .variable = "indicator-active-colour", .set = bar_config_set_indicator_colour_active },
		{ .variable = "indicator-hover-colour",  .set = bar_config_set_indicator_colour_hover  },
		{ .variable = "indicator-padding",       .set = bar_config_set_indicator_padding       },
		{ .variable = "indicator-style",         .set = bar_config_set_indicator_style,        },
		{ .variable = "layer",                   .set = bar_config_set_layer                   },
		{ .variable = "margin",                  .set = bar_config_set_margin_size             },
		{ .variable = "mode",                    .set = bar_config_set_mode                    },
		{ .variable = "namespace",               .set = bar_config_set_namespace               },
		{ .variable = "output",                  .set = bar_config_set_only_output             },
		{ .variable = "position",                .set = bar_config_set_position                },
		{ .variable = "radius",                  .set = bar_config_set_radius                  },
		{ .variable = "size",                    .set = bar_config_set_size                    }
	};

	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
		{
			if (configs[i].set(config, data, value))
				return true;
			goto exit;
		}

	log_message(NULL, 0, "ERROR: Unrecognized bar setting \"%s\".\n", variable);
exit:
	log_message(NULL, 0, "INFO: The error is on line %d in \"%s\".\n",
			line, data->config_path);
	return false;
}

