/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Henrik Plickat
 * Copyright (C) 2020 Nicolai Dagestad
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
#include"seat.h"
#include"item.h"
#include"output.h"
#include"bar.h"
#include"types/colour_t.h"
#include"types/string_t.h"
#include"types/box_t.h"

/*************************
 * Bar configuration set *
 *************************/
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

struct Lava_bar_configuration *bar_get_first_config (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &bar->configs, link)
		return config;
	return NULL;
}

struct Lava_bar_configuration *bar_get_last_config (struct Lava_bar *bar)
{
	int len = wl_list_length(&bar->configs) - 1, i = 0;
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &bar->configs, link)
		if ( i++ == len )
			return config;
	return NULL;
}

bool create_bar_config (struct Lava_bar *bar, bool default_config)
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
		bar_config_copy_settings(config, bar_get_first_config(bar));

	wl_list_insert(&bar->configs, &config->link);
	return true;
}

static void destroy_bar_config (struct Lava_bar_configuration *config)
{
	free_if_set(config->cursor_name);
	free_if_set(config->only_output);
	free_if_set(config->namespace);
	free(config);
}

static void destroy_all_bar_configs (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config, *temp;
	wl_list_for_each_safe(config, temp, &bar->configs, link)
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

static void finalize_all_bar_configs (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &bar->configs, link)
		finalize_bar_config(config);
}

/***************
 * Logical bar *
 * *************/
bool create_bar (struct Lava_data *data)
{
	struct Lava_bar *bar = calloc(1, sizeof(struct Lava_bar));
	if ( bar == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	bar->data = data;

	wl_list_init(&bar->items);
	wl_list_init(&bar->configs);

	/* Create default configuration. */
	if (! create_bar_config(bar, true))
	{
		free(bar);
		return false;
	}

	wl_list_insert(&data->bars, &bar->link);

	return true;
}

struct Lava_bar *get_last_bar (struct Lava_data *data)
{
	int len = wl_list_length(&data->bars) - 1, i = 0;
	struct Lava_bar *bar;
	wl_list_for_each(bar, &data->bars, link)
		if ( i++ == len )
			return bar;
	return NULL;
}

bool finalize_bar (struct Lava_bar *bar)
{
	log_message(bar->data, 1, "[bar] Finalize bar.\n");
	if (! finalize_items(bar))
		return false;
	finalize_all_bar_configs(bar);
	return true;
}

static void destroy_bar (struct Lava_bar *bar)
{
	wl_list_remove(&bar->link);
	destroy_all_items(bar);
	destroy_all_bar_configs(bar);
	free(bar);
}

void destroy_all_bars (struct Lava_data *data)
{
	log_message(data, 1, "[bar] Destroying all bars.\n");
	struct Lava_bar *bar, *temp;
	wl_list_for_each_safe(bar, temp, &data->bars, link)
		destroy_bar(bar);
}

/*********************
 * Bar configuration *
 *********************/
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

static uint32_t bar_config_count_args (const char *arg)
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

static bool bar_config_directional_config (uint32_t *_a, uint32_t *_b, uint32_t *_c, uint32_t *_d,
		const char *arg, const char *conf_name, const char *conf_name_2)
{
	int32_t a, b, c, d;

	if ( 4 == bar_config_count_args(arg) && 4 == sscanf(arg, "%d %d %d %d", &a, &b, &c, &d) )
		goto done;

	if ( 1 == bar_config_count_args(arg) && 1 == sscanf(arg, "%d", &d) )
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
	return bar_config_directional_config(&config->border.top, &config->border.right,
			&config->border.bottom, &config->border.left,
			arg, "border", "Border size");
}

BAR_CONFIG(bar_config_set_margin_size)
{
	return bar_config_directional_config(&config->margin.top, &config->margin.right,
			&config->margin.bottom, &config->margin.left,
			arg, "margin", "Margins");
}

BAR_CONFIG(bar_config_set_radius)
{
	return bar_config_directional_config(&config->radii.top_left, &config->radii.top_right,
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

/********************************
 * Generic cairo draw functions *
 ********************************/
static void circle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t size)
{
	cairo_arc(cairo, x + (size/2.0), y + (size/2.0), size / 2.0, 0, 2 * 3.1415927);
}

static void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uradii_t *radii)
{
	double degrees = 3.1415927 / 180.0;
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + w - radii->top_right,    y     + radii->top_right,    radii->top_right,   -90 * degrees,   0 * degrees);
	cairo_arc(cairo, x + w - radii->bottom_right, y + h - radii->bottom_right, radii->bottom_right,  0 * degrees,  90 * degrees);
	cairo_arc(cairo, x     + radii->bottom_left,  y + h - radii->bottom_left,  radii->bottom_left,  90 * degrees, 180 * degrees);
	cairo_arc(cairo, x     + radii->top_left,     y     + radii->top_left,     radii->top_left,    180 * degrees, 270 * degrees);
	cairo_close_path(cairo);
}

static void clear_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

/**************
 * Indicators *
 **************/
void destroy_indicator (struct Lava_item_indicator *indicator)
{
	if ( indicator->indicator_subsurface != NULL )
		wl_subsurface_destroy(indicator->indicator_subsurface);
	if ( indicator->indicator_surface != NULL )
		wl_surface_destroy(indicator->indicator_surface);

	/* Cleanup in the parent. */
	if ( indicator->seat != NULL )
		indicator->seat->pointer.indicator = NULL;
	if ( indicator->touchpoint != NULL )
		indicator->touchpoint->indicator = NULL;

	finish_buffer(&indicator->indicator_buffers[0]);
	finish_buffer(&indicator->indicator_buffers[1]);

	wl_surface_commit(indicator->instance->bar_surface);
	wl_list_remove(&indicator->link);
	free(indicator);
}

struct Lava_item_indicator *create_indicator (struct Lava_bar_instance *instance)
{
	struct Lava_data *data = instance->data;

	struct Lava_item_indicator *indicator = calloc(1, sizeof(struct Lava_item_indicator));
	if ( indicator == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return NULL;
	}

	wl_list_insert(&instance->indicators, &indicator->link);

	indicator->seat       = NULL;
	indicator->touchpoint = NULL;
	indicator->instance   = instance;

	if ( NULL == (indicator->indicator_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		goto error;
	}
	if ( NULL == (indicator->indicator_subsurface = wl_subcompositor_get_subsurface(
					data->subcompositor, indicator->indicator_surface,
					instance->bar_surface)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_subsurface.\n");
		goto error;
	}

	wl_subsurface_set_desync(indicator->indicator_subsurface);
	wl_subsurface_place_below(indicator->indicator_subsurface, instance->icon_surface);
	wl_subsurface_set_position(indicator->indicator_subsurface, 0, 0);

	struct wl_region *region = wl_compositor_create_region(data->compositor);
	wl_surface_set_input_region(indicator->indicator_surface, region);
	wl_region_destroy(region);

	return indicator;

error:
	destroy_indicator(indicator);
	return NULL;
}

void indicator_set_colour (struct Lava_item_indicator *indicator, colour_t *colour)
{
	struct Lava_bar_instance      *instance = indicator->instance;
	struct Lava_bar               *bar      = instance->bar;
	struct Lava_data              *data     = bar->data;
	struct Lava_bar_configuration *config   = instance->config;
	uint32_t                       scale    = instance->output->scale;

	uint32_t buffer_size = config->size - (2 * config->indicator_padding);
	buffer_size *= scale;

	/* Get new/next buffer. */
	if (! next_buffer(&indicator->current_indicator_buffer,
				data->shm, indicator->indicator_buffers,
				buffer_size, buffer_size))
	{
		destroy_indicator(indicator);
		return;
	}

	cairo_t *cairo = indicator->current_indicator_buffer->cairo;
	clear_buffer(cairo);
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	switch (config->indicator_style)
	{
		case STYLE_RECTANGLE:
			/* Cairo implicitly fills everything if no shape has been drawn. */
			cairo_rectangle(cairo, 0, 0, buffer_size, buffer_size);
			break;

		case STYLE_ROUNDED_RECTANGLE:
			rounded_rectangle(cairo, 0, 0, buffer_size, buffer_size, &config->radii);
			break;

		case STYLE_CIRCLE:
			circle(cairo, 0, 0, buffer_size);
			break;
	}

	colour_t_set_cairo_source(cairo, colour);
	cairo_fill(cairo);

	wl_surface_set_buffer_scale(indicator->indicator_surface, (int32_t)scale);
	wl_surface_attach(indicator->indicator_surface,
			indicator->current_indicator_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(indicator->indicator_surface, 0, 0, INT32_MAX, INT32_MAX);
}

void move_indicator (struct Lava_item_indicator *indicator, struct Lava_item *item)
{
	struct Lava_bar_instance      *instance = indicator->instance;
	struct Lava_bar_configuration *config   = instance->config;

	int32_t x = (int32_t)(instance->item_area_dim.x + config->indicator_padding);
	int32_t y = (int32_t)(instance->item_area_dim.y + config->indicator_padding);
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		x += (int32_t)item->ordinate;
	else
		y += (int32_t)item->ordinate;

	wl_subsurface_set_position(indicator->indicator_subsurface, x, y);
}

void indicator_commit (struct Lava_item_indicator *indicator)
{
	wl_surface_commit(indicator->indicator_surface);
	wl_surface_commit(indicator->instance->bar_surface);
}

/****************
 * Bar instance *
 ****************/
static void draw_items (struct Lava_bar_instance *instance, cairo_t *cairo)
{
	struct Lava_bar_configuration *config = instance->config;
	struct Lava_bar               *bar    = instance->bar;

	uint32_t scale = instance->output->scale;
	uint32_t size = config->size * scale;
	uint32_t *increment, increment_offset;
	uint32_t x = 0, y = 0;
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = x;
	else
		increment = &y, increment_offset = y;

	struct Lava_item *item;
	wl_list_for_each_reverse(item, &bar->items, link) if ( item->type == TYPE_BUTTON )
	{
		*increment = (item->ordinate * scale) + increment_offset;
		if ( item->img != NULL )
			image_t_draw_to_cairo(cairo, item->img,
					x + config->icon_padding,
					y + config->icon_padding,
					size - (2 * config->icon_padding),
					size - (2 * config->icon_padding));
	}
}

/* Draw a rectangle with configurable borders and corners. */
void draw_bar_background (cairo_t *cairo, ubox_t *_dim, udirections_t *_border, uradii_t *_radii,
		uint32_t scale, colour_t *bar_colour, colour_t *border_colour)
{
	ubox_t        dim    = ubox_t_scale(_dim, scale);
	udirections_t border = udirections_t_scale(_border, scale);
	uradii_t      radii  = uradii_t_scale(_radii, scale);

	ubox_t center = {
		.x = dim.x + border.left,
		.y = dim.y + border.top,
		.w = dim.w - (border.left + border.right),
		.h = dim.h - (border.top + border.bottom)
	};

	/* Avoid radii so big they cause unexpected drawing behaviour. */
	uint32_t smallest_side = center.w < center.h ? center.w : center.h;
	if ( radii.top_left > smallest_side / 2 )
		radii.top_left = smallest_side / 2;
	if ( radii.top_right > smallest_side / 2 )
		radii.top_right = smallest_side / 2;
	if ( radii.bottom_left > smallest_side / 2 )
		radii.bottom_left = smallest_side / 2;
	if ( radii.bottom_right > smallest_side / 2 )
		radii.bottom_right = smallest_side / 2;

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

	if ( radii.top_left == 0 && radii.top_right == 0 && radii.bottom_left == 0 && radii.bottom_right == 0 )
	{
		if ( border.top == 0 && border.bottom == 0 && border.left == 0 && border.right == 0 )
		{
			cairo_rectangle(cairo, dim.x, dim.y, dim.w, dim.h);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			/* Borders. */
			cairo_rectangle(cairo, dim.x, dim.y, dim.w, border.top);
			cairo_rectangle(cairo, dim.x + dim.w - border.right, dim.y + border.top,
					border.right, dim.h - border.top - border.bottom);
			cairo_rectangle(cairo, dim.x, dim.y + dim.h - border.bottom, dim.w, border.bottom);
			cairo_rectangle(cairo, dim.x, dim.y + border.top, border.left,
					dim.h - border.top - border.bottom);
			colour_t_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			/* Center. */
			cairo_rectangle(cairo, center.x, center.y, center.w, center.h);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}
	else
	{
		if ( border.top == 0 && border.bottom == 0 && border.left == 0 && border.right == 0 )
		{
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii);
			colour_t_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			rounded_rectangle(cairo, center.x, center.y, center.w, center.h, &radii);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}

	cairo_restore(cairo);
}

static void bar_instance_render_icon_frame (struct Lava_bar_instance *instance)
{
	struct Lava_data   *data    = instance->data;
	struct Lava_output *output  = instance->output;
	uint32_t            scale   = output->scale;

	log_message(data, 2, "[bar] Render icon frame: global_name=%d\n",
			instance->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&instance->current_icon_buffer, data->shm, instance->icon_buffers,
				instance->item_area_dim.w  * scale, instance->item_area_dim.h * scale))
		return;

	cairo_t *cairo = instance->current_icon_buffer->cairo;
	clear_buffer(cairo);

	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	/* Draw icons. */
	if (! instance->hidden)
		draw_items(instance, cairo);

	wl_surface_set_buffer_scale(instance->icon_surface, (int32_t)scale);
	wl_surface_attach(instance->icon_surface, instance->current_icon_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(instance->icon_surface, 0, 0, INT32_MAX, INT32_MAX);
}

static void bar_instance_render_background_frame (struct Lava_bar_instance *instance)
{
	struct Lava_data              *data   = instance->data;
	struct Lava_bar_configuration *config = instance->config;
	struct Lava_output            *output = instance->output;
	uint32_t                       scale  = output->scale;

	ubox_t *buffer_dim, *bar_dim;
	if (instance->hidden)
		buffer_dim = &instance->surface_hidden_dim, bar_dim = &instance->bar_hidden_dim;
	else
		buffer_dim = &instance->surface_dim, bar_dim = &instance->bar_dim;

	log_message(data, 2, "[bar] Render bar frame: global_name=%d\n", instance->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&instance->current_bar_buffer, data->shm, instance->bar_buffers,
				buffer_dim->w  * scale, buffer_dim->h * scale))
		return;

	cairo_t *cairo = instance->current_bar_buffer->cairo;
	clear_buffer(cairo);

	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	/* Draw bar. */
	if (! instance->hidden)
	{
		log_message(data, 2, "[bar] Drawing bar background.\n");
		draw_bar_background(cairo, bar_dim, &config->border, &config->radii,
				scale, &config->bar_colour, &config->border_colour);
	}

	wl_surface_set_buffer_scale(instance->bar_surface, (int32_t)scale);
	wl_surface_attach(instance->bar_surface, instance->current_bar_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(instance->bar_surface, 0, 0, INT32_MAX, INT32_MAX);
}

static uint32_t get_anchor (struct Lava_bar_configuration *config)
{
	/* Look-Up-Table; Not fancy but still the best solution for this. */
	struct
	{
		uint32_t triplet; /* Used for MODE_FULL and MODE_AGGRESSIVE. */
		uint32_t mode_default[3];
	} anchors[4] = {
		[POSITION_TOP] = {
			.triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			.mode_default = {
				[ALIGNMENT_START]  = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
				[ALIGNMENT_CENTER] = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
				[ALIGNMENT_END]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
			}
		},
		[POSITION_RIGHT] = {
			.triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.mode_default = {
				[ALIGNMENT_START]  = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
				[ALIGNMENT_CENTER] = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
				[ALIGNMENT_END]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
			}
		},
		[POSITION_BOTTOM] = {
			.triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			.mode_default = {
				[ALIGNMENT_START]  = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
				[ALIGNMENT_CENTER] = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
				[ALIGNMENT_END]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
			}
		},
		[POSITION_LEFT] = {
			.triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.mode_default = {
				[ALIGNMENT_START]  = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
				[ALIGNMENT_CENTER] = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
				[ALIGNMENT_END]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
			}
		},
	};

	if ( config->mode == MODE_DEFAULT )
		return anchors[config->position].mode_default[config->alignment];
	else
		return anchors[config->position].triplet;
}

static void bar_instance_configure_layer_surface (struct Lava_bar_instance *instance)
{
	struct Lava_data              *data   = instance->data;
	struct Lava_bar_configuration *config = instance->config;

	log_message(instance->data, 1, "[bar] Configuring bar instance: global_name=%d\n",
			instance->output->global_name);

	ubox_t *buffer_dim, *bar_dim;
	if (instance->hidden)
		buffer_dim = &instance->surface_hidden_dim, bar_dim = &instance->bar_hidden_dim;
	else
		buffer_dim = &instance->surface_dim, bar_dim = &instance->bar_dim;

	zwlr_layer_surface_v1_set_size(instance->layer_surface, buffer_dim->w, buffer_dim->h);

	/* Anchor the surface to the correct edge. */
	zwlr_layer_surface_v1_set_anchor(instance->layer_surface, get_anchor(config));

	if ( config->mode == MODE_DEFAULT )
		zwlr_layer_surface_v1_set_margin(instance->layer_surface,
				(int32_t)config->margin.top, (int32_t)config->margin.right,
				(int32_t)config->margin.bottom, (int32_t)config->margin.left);
	else if ( config->orientation == ORIENTATION_HORIZONTAL )
		zwlr_layer_surface_v1_set_margin(instance->layer_surface,
				(int32_t)config->margin.top, 0,
				(int32_t)config->margin.bottom, 0);
	else
		zwlr_layer_surface_v1_set_margin(instance->layer_surface,
				0, (int32_t)config->margin.right,
				0, (int32_t)config->margin.left);

	/* Set exclusive zone to prevent other surfaces from obstructing ours. */
	int32_t exclusive_zone;
	if ( config->exclusive_zone == 1 )
	{
		if ( config->orientation == ORIENTATION_HORIZONTAL )
			exclusive_zone = (int32_t)buffer_dim->h;
		else
			exclusive_zone = (int32_t)buffer_dim->w;
	}
	else
		exclusive_zone = config->exclusive_zone;
	zwlr_layer_surface_v1_set_exclusive_zone(instance->layer_surface,
			exclusive_zone);

	/* Create a region of the visible part of the surface.
	 * Behold: In MODE_AGGRESSIVE, the actual surface is larger than the visible bar.
	 */
	struct wl_region *region = wl_compositor_create_region(data->compositor);
	wl_region_add(region, (int32_t)instance->bar_dim.x, (int32_t)instance->bar_dim.y,
			(int32_t)bar_dim->w, (int32_t)bar_dim->h);

	/* Set input region. This is necessary to prevent the unused parts of
	 * the surface to catch pointer and touch events.
	 */
	wl_surface_set_input_region(instance->bar_surface, region);

	wl_region_destroy(region);
}


static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_bar_instance *instance = (struct Lava_bar_instance *)raw_data;
	struct Lava_data         *data     = instance->data;

	log_message(data, 1, "[bar] Layer surface configure request: global_name=%d w=%d h=%d serial=%d\n",
			instance->output->global_name, w, h, serial);

	instance->configured = true;
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	update_bar_instance(instance);
}

static void layer_surface_handle_closed (void *data, struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_bar_instance *instance = (struct Lava_bar_instance *)data;
	log_message(instance->data, 1, "[bar] Layer surface has been closed: global_name=%d\n",
				instance->output->global_name);
	destroy_bar_instance(instance);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

static void bar_instance_configure_subsurface (struct Lava_bar_instance *instance)
{
	struct Lava_data *data = instance->data;

	log_message(instance->data, 1, "[bar] Configuring icons: global_name=%d\n",
			instance->output->global_name);

	wl_subsurface_set_position(instance->subsurface,
			(int32_t)instance->item_area_dim.x, (int32_t)instance->item_area_dim.y);

	/* We do not want to receive any input events from the subsurface.
	 * Almot everything in LavaLauncher uses the coords of the parent surface.
	 */
	struct wl_region *region = wl_compositor_create_region(data->compositor);
	wl_surface_set_input_region(instance->icon_surface, region);
	wl_region_destroy(region);

}

/* Positions and dimensions for MODE_AGGRESSIVE. */
static void bar_instance_mode_aggressive_dimensions (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config = instance->config;
	struct Lava_output           *output  = instance->output;

	/* Position of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL ) switch (config->alignment)
	{
		case ALIGNMENT_START:
			instance->item_area_dim.x = config->border.left + (uint32_t)config->margin.left;
			instance->item_area_dim.y = config->border.top;
			break;

		case ALIGNMENT_CENTER:
			instance->item_area_dim.x = (output->w / 2) - (instance->item_area_dim.w / 2)
				+ (uint32_t)(config->margin.left - config->margin.right);
			instance->item_area_dim.y = config->border.top;
			break;

		case ALIGNMENT_END:
			instance->item_area_dim.x = output->w - instance->item_area_dim.w
				- config->border.right - (uint32_t)config->margin.right;
			instance->item_area_dim.y = config->border.top;
			break;
	}
	else switch (config->alignment)
	{
		case ALIGNMENT_START:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = config->border.top + (uint32_t)config->margin.top;
			break;

		case ALIGNMENT_CENTER:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = (output->h / 2) - (instance->item_area_dim.h / 2)
				+ (uint32_t)(config->margin.top - config->margin.bottom);
			break;

		case ALIGNMENT_END:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = output->h - instance->item_area_dim.h
				- config->border.bottom - (uint32_t)config->margin.bottom;
			break;
	}

	/* Position of bar. */
	instance->bar_dim.x = instance->item_area_dim.x - config->border.left;
	instance->bar_dim.y = instance->item_area_dim.y - config->border.top;

	/* Size of bar. */
	instance->bar_dim.w = instance->item_area_dim.w + config->border.left + config->border.right;
	instance->bar_dim.h = instance->item_area_dim.h + config->border.top  + config->border.bottom;

	/* Size of buffer / surface and hidden dimensions. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		instance->surface_dim.w = instance->output->w;
		instance->surface_dim.h = config->size + config->border.top + config->border.bottom;

		instance->surface_hidden_dim.w = instance->surface_dim.w;
		instance->surface_hidden_dim.h = config->hidden_size;

		instance->bar_hidden_dim.w = instance->bar_dim.w;
		instance->bar_hidden_dim.h = config->hidden_size;
	}
	else
	{
		instance->surface_dim.w = config->size + config->border.left + config->border.right;
		instance->surface_dim.h = instance->output->h;

		instance->surface_hidden_dim.w = config->hidden_size;
		instance->surface_hidden_dim.h = instance->surface_dim.h;

		instance->bar_hidden_dim.w = config->hidden_size;
		instance->bar_hidden_dim.h = instance->bar_dim.h;
	}
	instance->bar_hidden_dim.x = instance->bar_dim.x;
	instance->bar_hidden_dim.y = instance->bar_dim.y;
}

/* Positions and dimensions for MODE_FULL. */
static void bar_instance_mode_full_dimensions (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config = instance->config;
	struct Lava_output           *output  = instance->output;

	/* Position of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL ) switch (config->alignment)
	{
		case ALIGNMENT_START:
			instance->item_area_dim.x = config->border.left + (uint32_t)config->margin.left;
			instance->item_area_dim.y = config->border.top;
			break;

		case ALIGNMENT_CENTER:
			instance->item_area_dim.x = (output->w / 2) - (instance->item_area_dim.w / 2)
				+ (uint32_t)(config->margin.left - config->margin.right);
			instance->item_area_dim.y = config->border.top;
			break;

		case ALIGNMENT_END:
			instance->item_area_dim.x = output->w - instance->item_area_dim.w
				- config->border.right - (uint32_t)config->margin.right;
			instance->item_area_dim.y = config->border.top;
			break;
	}
	else switch (config->alignment)
	{
		case ALIGNMENT_START:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = config->border.top + (uint32_t)config->margin.top;
			break;

		case ALIGNMENT_CENTER:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = (output->h / 2) - (instance->item_area_dim.h / 2)
				+ (uint32_t)(config->margin.top - config->margin.bottom);
			break;

		case ALIGNMENT_END:
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = output->h - instance->item_area_dim.h
				- config->border.bottom - (uint32_t)config->margin.bottom;
			break;
	}

	/* Position and size of bar and size of buffer / surface and hidden dimensions. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		instance->bar_dim.x = (uint32_t)config->margin.left;
		instance->bar_dim.y = 0;

		instance->bar_dim.w = output->w - (uint32_t)(config->margin.left + config->margin.right);
		instance->bar_dim.h = instance->item_area_dim.h + config->border.top + config->border.bottom;

		instance->surface_dim.w = instance->output->w;
		instance->surface_dim.h = config->size + config->border.top + config->border.bottom;

		instance->surface_hidden_dim.w = instance->surface_dim.w;
		instance->surface_hidden_dim.h = config->hidden_size;

		instance->bar_hidden_dim.w = instance->bar_dim.w;
		instance->bar_hidden_dim.h = config->hidden_size;
	}
	else
	{
		instance->bar_dim.x = 0;
		instance->bar_dim.y = (uint32_t)config->margin.top;

		instance->bar_dim.w = instance->item_area_dim.w + config->border.left + config->border.right;
		instance->bar_dim.h = output->h - (uint32_t)(config->margin.top + config->margin.bottom);

		instance->surface_dim.w = config->size + config->border.left + config->border.right;
		instance->surface_dim.h = instance->output->h;

		instance->surface_hidden_dim.w = config->hidden_size;
		instance->surface_hidden_dim.h = instance->surface_dim.h;

		instance->bar_hidden_dim.w = config->hidden_size;
		instance->bar_hidden_dim.h = instance->bar_dim.h;
	}
}

/* Position and size for MODE_DEFAULT. */
static void bar_instance_mode_default_dimensions (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config = instance->config;

	/* Position of item area. */
	instance->item_area_dim.x = config->border.left;
	instance->item_area_dim.y = config->border.top;

	/* Position of bar. */
	instance->bar_dim.x = 0;
	instance->bar_dim.y = 0;

	/* Size of bar. */
	instance->bar_dim.w = instance->item_area_dim.w + config->border.left + config->border.right;
	instance->bar_dim.h = instance->item_area_dim.h + config->border.top  + config->border.bottom;

	/* Size of hidden bar. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		instance->bar_hidden_dim.w = instance->bar_dim.w;
		instance->bar_hidden_dim.h = config->hidden_size;
	}
	else
	{
		instance->bar_hidden_dim.w = config->hidden_size;
		instance->bar_hidden_dim.h = instance->bar_dim.h;
	}

	/* Size of buffer / surface. */
	instance->surface_dim        = instance->bar_dim;
	instance->surface_hidden_dim = instance->bar_hidden_dim;
}

static void bar_instance_update_dimensions (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config  = instance->config;
	struct Lava_bar               *bar     = instance->bar;
	struct Lava_output            *output  = instance->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	/* Size of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		instance->item_area_dim.w = get_item_length_sum(bar);
		instance->item_area_dim.h = config->size;
	}
	else
	{
		instance->item_area_dim.w = config->size;
		instance->item_area_dim.h = get_item_length_sum(bar);
	}

	/* Other dimensions. */
	switch (instance->config->mode)
	{
		case MODE_DEFAULT:    bar_instance_mode_default_dimensions(instance);    break;
		case MODE_FULL:       bar_instance_mode_full_dimensions(instance);       break;
		case MODE_AGGRESSIVE: bar_instance_mode_aggressive_dimensions(instance); break;
	}
}

/* Return a bool indicating if the bar instance should currently be hidden or not. */
static bool bar_instance_should_hide (struct Lava_bar_instance *instance)
{
	switch(instance->config->hidden_mode)
	{
		case HIDDEN_MODE_ALWAYS:
			return !instance->hover;

		case HIDDEN_MODE_RIVER_AUTO:
			return ( instance->output->river_output_occupied && ! instance->hover );

		case HIDDEN_MODE_NEVER:
		default:
			return false;
	}
}

void bar_instance_update_hidden_status (struct Lava_bar_instance *instance)
{
	bool current     = instance->hidden;
	instance->hidden = bar_instance_should_hide(instance);

	/* No need to do something if nothing changed. */
	if (current == instance->hidden)
		return;

	update_bar_instance(instance);
}

bool create_bar_instance (struct Lava_bar *bar, struct Lava_bar_configuration *config,
		struct Lava_output *output)
{
	struct Lava_data *data = bar->data;
	log_message(data, 1, "[bar] Creating bar instance: global_name=%d\n", output->global_name);

	struct Lava_bar_instance *instance = calloc(1, sizeof(struct Lava_bar_instance));
	if ( instance == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	wl_list_insert(&output->bar_instances, &instance->link);
	instance->data          = data;
	instance->bar           = bar;
	instance->config        = config;
	instance->output        = output;
	instance->bar_surface   = NULL;
	instance->icon_surface  = NULL;
	instance->layer_surface = NULL;
	instance->subsurface    = NULL;
	instance->configured    = false;
	instance->hover         = false;
	instance->hidden        = bar_instance_should_hide(instance);

	wl_list_init(&instance->indicators);

	/* Main surface for the bar. */
	if ( NULL == (instance->bar_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		return false;
	}
	if ( NULL == (instance->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					data->layer_shell, instance->bar_surface,
					output->wl_output, config->layer,
					str_orelse(config->namespace, "LavaLauncher"))) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create layer_surface.\n");
		return false;
	}

	/* Subsurface for the icons. */
	if ( NULL == (instance->icon_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		return false;
	}
	if ( NULL == (instance->subsurface = wl_subcompositor_get_subsurface(
					data->subcompositor, instance->icon_surface,
					instance->bar_surface)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_subsurface.\n");
		return false;
	}

	bar_instance_update_dimensions(instance);
	bar_instance_configure_layer_surface(instance);
	bar_instance_configure_subsurface(instance);
	zwlr_layer_surface_v1_add_listener(instance->layer_surface,
			&layer_surface_listener, instance);
	wl_surface_commit(instance->icon_surface);
	wl_surface_commit(instance->bar_surface);

	return true;
}

void destroy_bar_instance (struct Lava_bar_instance *instance)
{
	if ( instance == NULL )
		return;

	struct Lava_item_indicator *indicator, *temp;
	wl_list_for_each_safe(indicator, temp, &instance->indicators, link)
		destroy_indicator(indicator);

	if ( instance->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(instance->layer_surface);
	if ( instance->subsurface != NULL )
		wl_subsurface_destroy(instance->subsurface);
	if ( instance->bar_surface != NULL )
		wl_surface_destroy(instance->bar_surface);
	if ( instance->icon_surface != NULL )
		wl_surface_destroy(instance->icon_surface);

	finish_buffer(&instance->bar_buffers[0]);
	finish_buffer(&instance->bar_buffers[1]);
	finish_buffer(&instance->icon_buffers[0]);
	finish_buffer(&instance->icon_buffers[1]);

	wl_list_remove(&instance->link);
	free(instance);
}

void destroy_all_bar_instances (struct Lava_output *output)
{
	log_message(output->data, 1, "[bar] Destroying bar instances: global-name=%d\n", output->global_name);
	struct Lava_bar_instance *instance, *temp;
	wl_list_for_each_safe(instance, temp, &output->bar_instances, link)
		destroy_bar_instance(instance);
}

void update_bar_instance (struct Lava_bar_instance *instance)
{
	/* It is possible that this function is called by output events before
	 * the bar instance has been created. This function will return and
	 * abort unless it is called either when a surface configure event has
	 * been received at least once, it is called by a surface configure
	 * event or it is called during the creation of the surface.
	 */
	if ( instance == NULL || ! instance->configured )
		return;

	bar_instance_update_dimensions(instance);

	bar_instance_configure_subsurface(instance);
	bar_instance_configure_layer_surface(instance);

	bar_instance_render_icon_frame(instance);
	bar_instance_render_background_frame(instance);

	wl_surface_commit(instance->icon_surface);
	wl_surface_commit(instance->bar_surface);
}

struct Lava_bar_instance *bar_instance_from_surface (struct Lava_data *data, struct wl_surface *surface)
{
	if ( data == NULL || surface == NULL )
		return NULL;
	struct Lava_output *output;
	struct Lava_bar_instance *instance;
	wl_list_for_each(output, &data->outputs, link)
		wl_list_for_each(instance,  &output->bar_instances, link)
			if ( instance->bar_surface == surface )
				return instance;
	return NULL;
}

