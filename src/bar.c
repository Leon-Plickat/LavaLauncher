/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 - 2021 Leon Henrik Plickat
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

#include<wayland-client.h>

#include"lavalauncher.h"
#include"util.h"
#include"config-parser.h"
#include"seat.h"
#include"item.h"
#include"output.h"
#include"bar.h"
#include"foreign-toplevel-management.h"
#include"types/colour_t.h"
#include"types/box_t.h"

/***************************
 *                         *
 *  Bar configuration set  *
 *                         *
 ***************************/
static void bar_config_sensible_defaults (struct Lava_bar_configuration *config)
{
	config->position  = POSITION_BOTTOM;
	config->mode      = MODE_DEFAULT;
	config->layer     = ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;

	config->size              = 60;
	config->hidden_size       = 10;
	config->hidden_mode       = HIDDEN_MODE_NEVER;
	config->icon_padding      = 4;
	config->exclusive_zone    = 1;
	config->indicator_padding = 0;

	udirections_t_set_all(&config->border, 1);
	udirections_t_set_all(&config->margin, 0);
	uradii_t_set_all(&config->radii, 5);

	colour_t_from_string(&config->bar_colour, "0x000000");
	colour_t_from_string(&config->border_colour, "0xffffff");
	colour_t_from_string(&config->indicator_hover_colour, "0x404040");
	colour_t_from_string(&config->indicator_active_colour, "0x606060");

	config->condition_scale      = 0;
	config->condition_transform  = -1;
	config->condition_resolution = RESOLUTION_ALL;

	config->cursor_name_default = NULL;
	config->cursor_name_hover = NULL;

	/* Use XCURSOR_SIZE if it exists. */
	const char *cursor_size = getenv("XCURSOR_SIZE");
	if ( cursor_size == NULL )
		config->cursor_size = 24;
	else
	{
		config->cursor_size = atoi(cursor_size);
		if ( config->cursor_size < 24 )
		{
			log_message(0, "WARNING: Invalid $XCURSOR_SIZE. Defaulting to 24.\n");
			config->cursor_size = 24;
		}
	}

	config->only_output = NULL;
	config->namespace   = NULL;
}

static void bar_config_copy_settings (struct Lava_bar_configuration *config,
		struct Lava_bar_configuration *default_config)
{
	memcpy(config, default_config, sizeof(struct Lava_bar_configuration));

	/* If the configuration set has any strings, we just copied the pointers.
	 * To make later logic simpler, every configuration set should have their
	 * own copy of the string.
	 */
	if ( config->cursor_name_default != NULL )
		config->cursor_name_default = strdup(default_config->cursor_name_default);
	if ( config->cursor_name_hover != NULL )
		config->cursor_name_hover = strdup(default_config->cursor_name_hover);
	if ( config->only_output != NULL )
		config->only_output = strdup(default_config->only_output);
	if ( config->namespace  != NULL )
		config->namespace  = strdup(default_config->namespace);
}

bool create_bar_config (void)
{
	TRY_NEW(struct Lava_bar_configuration, config, false);

	/* First config will automatically be the default one. */
	if ( context.default_config == NULL )
	{
		context.default_config = config;
		bar_config_sensible_defaults(config);
	}
	else
		bar_config_copy_settings(config, context.default_config);

	context.last_config = config;

	wl_list_insert(&context.configs, &config->link);
	return true;
}

static void destroy_bar_config (struct Lava_bar_configuration *config)
{
	free_if_set(config->cursor_name_default);
	free_if_set(config->cursor_name_hover);
	free_if_set(config->only_output);
	free_if_set(config->namespace);
	free(config);
}

void destroy_all_bar_configs (void)
{
	struct Lava_bar_configuration *config, *temp;
	wl_list_for_each_safe(config, temp, &context.configs, link)
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
		log_message(0, "WARNING: Configured 'icon-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		config->icon_padding = config->size / 3;
	}
	if ( config->indicator_padding > config->size / 3 )
	{
		log_message(0, "WARNING: Configured 'indicator-padding' too large for bar size. "
				"Automatically shrinking to a reasonable size.\n");
		config->indicator_padding = config->size / 3;
	}
}

bool finalize_all_bar_configs (void)
{
	/* No bar configuration? Then add a default one. */
	if ( wl_list_length(&context.configs) == 0 )
	{
		log_message(1, "[bar] No bar configuration, creating default.\n");
		if (! create_bar_config())
			return false;
	}

	struct Lava_bar_configuration *config;
	wl_list_for_each(config, &context.configs, link)
		finalize_bar_config(config);
	return true;
}

static bool bar_config_conditions_match_output (struct Lava_bar_configuration *config,
		struct Lava_output *output)
{
	if ( config->only_output != NULL && strcmp(output->name, config->only_output) )
		return false;

	if ( config->condition_scale != 0
			&& config->condition_scale != output->scale )
		return false;

	if ( config->condition_resolution == RESOLUTION_WIDER_THAN_HIGH
			&& output->w < output->h )
		return false;
	if ( config->condition_resolution == RESOLUTION_HIGHER_THEN_WIDE
			&& output->h < output->w )
		return false;

	if ( config->condition_transform != -1 && config->condition_transform != (int32_t)output->transform )
		return false;

	return true;
}

struct Lava_bar_configuration *get_bar_config_for_output (struct Lava_output *output)
{
	struct Lava_bar_configuration *config;
	wl_list_for_each_reverse(config, &context.configs, link)
		if (bar_config_conditions_match_output(config, output))
			return config;
	return NULL;
}

/***********************
 *                     *
 *  Bar configuration  *
 *                     *
 ***********************/
#define BAR_CONFIG(A) static bool A (struct Lava_bar_configuration *config, const char *arg)
#define BAR_CONFIG_COLOUR(A, B) \
	static bool A (struct Lava_bar_configuration *config, const char *arg) \
	{ \
		return colour_t_from_string(&config->B, arg); \
	}
#define BAR_CONFIG_STRING(A, B) \
	static bool A (struct Lava_bar_configuration *config, const char *arg) \
	{ \
		set_string(&config->B, (char *)arg); \
		return true; \
	}

static bool bar_config_directional_config (uint32_t *_a, uint32_t *_b, uint32_t *_c, uint32_t *_d,
		const char *arg, const char *conf_name, const char *conf_name_2)
{
	int32_t a, b, c, d;

	if ( 4 == count_tokens(arg) && 4 == sscanf(arg, "%d %d %d %d", &a, &b, &c, &d) )
		goto done;

	if ( 1 == count_tokens(arg) && 1 == sscanf(arg, "%d", &d) )
	{
		a = b = c = d;
		goto done;
	}

	log_message(0, "ERROR: Invalid %s configuration: %s\n"
			"INFO: You have to specify either one or four integers.\n",
			conf_name, arg);
	return false;

done:
	if ( a < 0 || b < 0 || c < 0 || d < 0 )
	{
		log_message(0, "ERROR: %s can not be negative.\n", conf_name_2);
		return false;
	}
	
	*_a = (uint32_t)a;
	*_b = (uint32_t)b;
	*_c = (uint32_t)c;
	*_d = (uint32_t)d;

	return true;
}

BAR_CONFIG_STRING(bar_config_set_cursor_name_default, cursor_name_default)
BAR_CONFIG_STRING(bar_config_set_cursor_name_hover, cursor_name_hover)
BAR_CONFIG_STRING(bar_config_set_namespace, namespace)

BAR_CONFIG_COLOUR(bar_config_set_bar_colour, bar_colour)
BAR_CONFIG_COLOUR(bar_config_set_border_colour, border_colour)
BAR_CONFIG_COLOUR(bar_config_set_indicator_colour_active, indicator_active_colour)
BAR_CONFIG_COLOUR(bar_config_set_indicator_colour_hover, indicator_hover_colour)

BAR_CONFIG(bar_config_set_cursor_size)
{
	config->cursor_size = atoi(arg);
	if ( config->cursor_size < 24 )
	{
		log_message(0, "ERROR: Cursor size must be at least 24.\n");
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_only_output)
{
	if ( ! strcmp(arg, "all") || ! strcmp(arg, "*") )
	{
		free_if_set(config->only_output);
		config->only_output = NULL;
		return true;
	}

	set_string(&config->only_output, (char *)arg);
	return true;
}

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
		log_message(0, "ERROR: Unrecognized position \"%s\".\n"
				"INFO: Possible positions are 'top', 'right', "
				"'bottom' and 'left'.\n", arg);
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
	else
	{
		log_message(0, "ERROR: Unrecognized mode \"%s\".\n"
				"INFO: Possible modes are 'default' and 'full'.\n", arg);
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
		log_message(0, "ERROR: Unrecognized layer \"%s\".\n"
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
		log_message(0, "ERROR: Size must be greater than zero.\n");
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
		log_message(0, "ERROR: Icon padding must be greater than or equal to zero.\n");
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
		log_message(0, "ERROR: Unrecognized exclusive zone option \"%s\".\n"
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
		log_message(0, "ERROR: Hidden size may not be smaller than 1.\n");
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
		context.need_river_status = true;
	}
	else
	{
		log_message(0, "ERROR: Unrecognized hidden mode option \"%s\".\n"
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
		log_message(0, "ERROR: Scale condition must be an integer greater than  zero or 'all'.\n");
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
		log_message(0, "ERROR: Resolution condition can be 'all', 'wider-than-high' or 'higher-than-wide'.\n" );
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
		log_message(0, "ERROR: Transform condition can be 0, 1, 2, 3 or 'all'.\n");
		return false;
	}
	return true;
}

BAR_CONFIG(bar_config_set_indicator_padding)
{
	int32_t size = atoi(arg);
	if ( size < 0 )
	{
		log_message(0, "ERROR: Indicator padding must be equal to or greater than zero.\n");
		return false;
	}
	config->indicator_padding = (uint32_t)size;
	return true;
}

#undef BAR_CONFIG
#undef BAR_CONFIG_STRING
#undef BAR_CONFIG_COLOUR

bool bar_config_set_variable (struct Lava_bar_configuration *config,
		const char *variable, const char *value, uint32_t line)
{
	struct
	{
		const char *variable;
		bool (*set)(struct Lava_bar_configuration*, const char*);
	} configs[] = {
		{ .variable = "background-colour",       .set = bar_config_set_bar_colour              },
		{ .variable = "border-colour",           .set = bar_config_set_border_colour           },
		{ .variable = "border",                  .set = bar_config_set_border_size             },
		{ .variable = "condition-resolution",    .set = bar_config_set_condition_resolution    },
		{ .variable = "condition-scale",         .set = bar_config_set_condition_scale         },
		{ .variable = "condition-transform",     .set = bar_config_set_condition_transform     },
		{ .variable = "cursor-default",          .set = bar_config_set_cursor_name_default     },
		{ .variable = "cursor-hover",            .set = bar_config_set_cursor_name_hover       },
		{ .variable = "cursor-size",             .set = bar_config_set_cursor_size             },
		{ .variable = "exclusive-zone",          .set = bar_config_set_exclusive_zone          },
		{ .variable = "hidden-size",             .set = bar_config_set_hidden_size             },
		{ .variable = "hidden-mode",             .set = bar_config_set_hidden_mode             },
		{ .variable = "icon-padding",            .set = bar_config_set_icon_padding            },
		{ .variable = "indicator-active-colour", .set = bar_config_set_indicator_colour_active },
		{ .variable = "indicator-hover-colour",  .set = bar_config_set_indicator_colour_hover  },
		{ .variable = "indicator-padding",       .set = bar_config_set_indicator_padding       },
		{ .variable = "layer",                   .set = bar_config_set_layer                   },
		{ .variable = "margin",                  .set = bar_config_set_margin_size             },
		{ .variable = "mode",                    .set = bar_config_set_mode                    },
		{ .variable = "namespace",               .set = bar_config_set_namespace               },
		{ .variable = "output",                  .set = bar_config_set_only_output             },
		{ .variable = "position",                .set = bar_config_set_position                },
		{ .variable = "radius",                  .set = bar_config_set_radius                  },
		{ .variable = "size",                    .set = bar_config_set_size                    }
	};

	FOR_ARRAY(configs, i) if (! strcmp(configs[i].variable, variable))
	{
		if (configs[i].set(config, value))
			return true;
		goto exit;
	}

	log_message(0, "ERROR: Unrecognized bar setting \"%s\".\n", variable);
exit:
	log_message(0, "INFO: The error is on line %d in \"%s\".\n",
			line, context.config_path);
	return false;
}

/******************
 *                *
 *  Bar instance  *
 *                *
 ******************/
static void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y,
		uint32_t w, uint32_t h, uradii_t *_radii, uint32_t scale)
{
	const double degrees = 3.1415927 / 180.0;
	x *= scale, y *= scale, w *= scale, h *= scale;
	uradii_t radii = uradii_t_scale(_radii, scale);
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + w - radii.top_right,    y     + radii.top_right,    radii.top_right,   -90 * degrees,   0 * degrees);
	cairo_arc(cairo, x + w - radii.bottom_right, y + h - radii.bottom_right, radii.bottom_right,  0 * degrees,  90 * degrees);
	cairo_arc(cairo, x     + radii.bottom_left,  y + h - radii.bottom_left,  radii.bottom_left,  90 * degrees, 180 * degrees);
	cairo_arc(cairo, x     + radii.top_left,     y     + radii.top_left,     radii.top_left,    180 * degrees, 270 * degrees);
	cairo_close_path(cairo);
}

static void clear_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

/* Draw a rectangle with configurable borders and corners. */
static void draw_bar_background (cairo_t *cairo, ubox_t *_dim, udirections_t *_border,
		uradii_t *_radii, colour_t *bar_colour, colour_t *border_colour, uint32_t scale)
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
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii, 1);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii, 1);
			colour_t_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			rounded_rectangle(cairo, center.x, center.y, center.w, center.h, &radii, 1);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}

	cairo_restore(cairo);
}

static void bar_instance_next_frame (struct Lava_bar_instance *instance)
{
	log_message(2, "[bar] Render bar frame: global_name=%d\n", instance->output->global_name);

	struct Lava_bar_configuration *config = instance->config;
	const uint32_t scale = instance->output->scale;

	if (instance->hidden)
	{
		if (! next_buffer(&instance->current_buffer, context.shm, instance->buffers,
				instance->surface_hidden_dim.w, instance->surface_hidden_dim.h))
		return;

		cairo_t *cairo = instance->current_buffer->cairo;
		clear_buffer(cairo);

		goto attach;
	}

	if (! next_buffer(&instance->current_buffer, context.shm, instance->buffers,
			instance->surface_dim.w * scale, instance->surface_dim.h * scale))
		return;

	cairo_t *cairo = instance->current_buffer->cairo;
	clear_buffer(cairo);

	/* Draw background an border */
	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);
	draw_bar_background(cairo, &instance->bar_dim, &config->border, &config->radii,
			&config->bar_colour, &config->border_colour, scale);

	const uint32_t icon_padding = instance->config->icon_padding;
	const uint32_t indicator_padding = instance->config->indicator_padding;
	for (int i = 0; i < instance->active_items; i++)
	{
		if ( instance->item_instances[i].item->type != TYPE_BUTTON )
			continue;

		if (! instance->dirty)
		{
			if (! instance->item_instances[i].dirty)
				continue;

			/* Damage the surface at the icon location, so the
			 * compositor knows which parts of the surface it needs
			 * to update.
			 */
			wl_surface_damage_buffer(instance->wl_surface,
					instance->item_instances[i].x * (int32_t)scale,
					instance->item_instances[i].y * (int32_t)scale,
					(int32_t)(instance->item_instances[i].w * scale),
					(int32_t)(instance->item_instances[i].h * scale));
		}

		instance->item_instances[i].dirty = false;

		/* Draw indicator. */
		if ( instance->item_instances[i].active_indicator > 0 )
		{
			rounded_rectangle(cairo,
					(uint32_t)instance->item_instances[i].x + indicator_padding,
					(uint32_t)instance->item_instances[i].y + indicator_padding,
					instance->item_instances[i].w - (2 * indicator_padding),
					instance->item_instances[i].h - (2 * indicator_padding),
					&instance->config->radii, scale);
			colour_t_set_cairo_source(cairo, &instance->config->indicator_active_colour);
			cairo_fill(cairo);
		}
		else if ( instance->item_instances[i].indicator > 0 )
		{
			rounded_rectangle(cairo,
					(uint32_t)instance->item_instances[i].x + indicator_padding,
					(uint32_t)instance->item_instances[i].y + indicator_padding,
					instance->item_instances[i].w - (2 * indicator_padding),
					instance->item_instances[i].h - (2 * indicator_padding),
					&instance->config->radii, scale);
			colour_t_set_cairo_source(cairo, &instance->config->indicator_hover_colour);
			cairo_fill(cairo);
		}

		/* Draw toplevel indicators. */
		if ( instance->item_instances[i].toplevel_activated_indicator > 0 )
		{
			// TODO
		}
		if ( instance->item_instances[i].toplevel_exists_indicator > 0 )
		{
			// TODO
		}

		/* Draw icon. */
		if ( instance->item_instances[i].item->img != NULL )
			image_t_draw_to_cairo(cairo, instance->item_instances[i].item->img,
					(uint32_t)(instance->item_instances[i].x) + icon_padding,
					(uint32_t)(instance->item_instances[i].y) + icon_padding,
					instance->item_instances[i].w - (2 * icon_padding),
					instance->item_instances[i].h - (2 * icon_padding), scale);
	}

attach:
	if (instance->dirty)
		wl_surface_damage_buffer(instance->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	instance->dirty = false;
	wl_surface_set_buffer_scale(instance->wl_surface, (int32_t)scale);
	wl_surface_attach(instance->wl_surface, instance->current_buffer->buffer, 0, 0);
}

static void frame_callback_handle_done (void *data, struct wl_callback *wl_callback, uint32_t time)
{
	struct Lava_bar_instance *instance = (struct Lava_bar_instance *)data;
	wl_callback_destroy(wl_callback);
	instance->frame_callback = NULL;
	bar_instance_next_frame(instance);
	wl_surface_commit(instance->wl_surface);
}

static const struct wl_callback_listener frame_callback_listener = {
	.done = frame_callback_handle_done,
};

void bar_instance_schedule_frame (struct Lava_bar_instance *instance)
{
	if ( instance->frame_callback != NULL )
		return;
	instance->frame_callback = wl_surface_frame(instance->wl_surface);
	wl_callback_add_listener(instance->frame_callback, &frame_callback_listener, instance);
	wl_surface_commit(instance->wl_surface);
}

static uint32_t get_anchor (struct Lava_bar_configuration *config)
{
	const uint32_t anchors[4][2] = {
		[POSITION_TOP] = {
			[MODE_DEFAULT] = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			[MODE_FULL]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		},
		[POSITION_RIGHT] = {
			[MODE_DEFAULT] = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			[MODE_FULL]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
		},
		[POSITION_BOTTOM] = {
			[MODE_DEFAULT] = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			[MODE_FULL]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		},
		[POSITION_LEFT] = {
			[MODE_DEFAULT] = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
			[MODE_FULL]    = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
		},
	};
	return anchors[config->position][config->mode];
}

static void bar_instance_configure_layer_surface (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config = instance->config;

	log_message(1, "[bar] Configuring bar instance: global_name=%d\n",
			instance->output->global_name);

	ubox_t *surface_dim, *bar_dim;
	if (instance->hidden)
		surface_dim = &instance->surface_hidden_dim, bar_dim = &instance->bar_hidden_dim;
	else
		surface_dim = &instance->surface_dim, bar_dim = &instance->bar_dim;

	zwlr_layer_surface_v1_set_size(instance->layer_surface, surface_dim->w, surface_dim->h);

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
			exclusive_zone = (int32_t)surface_dim->h;
		else
			exclusive_zone = (int32_t)surface_dim->w;
	}
	else
		exclusive_zone = config->exclusive_zone;
	zwlr_layer_surface_v1_set_exclusive_zone(instance->layer_surface,
			exclusive_zone);

	/* In MODE_FULL the actual surface can be larger than the visible bar if
	 * margins parallell to the bars orientation are used. In that case we
	 * have to set the input region so the unused parts of the buffer do not
	 * receive pointer/touch input.
	 */
	if ( config->mode == MODE_FULL )
	{
		if ( config->orientation == ORIENTATION_HORIZONTAL
				&& config->margin.left == 0 && config->margin.right == 0 )
			return;
		if ( config->orientation == ORIENTATION_VERTICAL
				&& config->margin.top == 0 && config->margin.bottom == 0 )
			return;

		struct wl_region *region = wl_compositor_create_region(context.compositor);
		wl_region_add(region, (int32_t)bar_dim->x, (int32_t)bar_dim->y,
				(int32_t)bar_dim->w, (int32_t)bar_dim->h);
		wl_surface_set_input_region(instance->wl_surface, region);
		wl_region_destroy(region);
	}
}

static void layer_surface_handle_configure (void *data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_bar_instance *instance = (struct Lava_bar_instance *)data;

	log_message(1, "[bar] Layer surface configure request: global_name=%d w=%d h=%d serial=%d\n",
			instance->output->global_name, w, h, serial);

	// TODO respect new size
	instance->configured = true;
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	update_bar_instance(instance, true, false);
}

static void layer_surface_handle_closed (void *data, struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_bar_instance *instance = (struct Lava_bar_instance *)data;
	struct Lava_output       *output   = instance->output;
	log_message(1, "[bar] Layer surface has been closed: global_name=%d\n",
				instance->output->global_name);
	destroy_bar_instance(instance);
	output->bar_instance = NULL;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

// TODO XXX [item-rework] does the scaling work correctly?
static void bar_instance_update_dimensions (struct Lava_bar_instance *instance)
{
	struct Lava_bar_configuration *config  = instance->config;
	struct Lava_output            *output  = instance->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	/* Position of the item area */
	if ( instance->config->mode == MODE_DEFAULT )
	{
		instance->item_area_dim.x = config->border.left;
		instance->item_area_dim.y = config->border.top;
	}
	else /* MODE_FULL */
	{
		if ( config->orientation == ORIENTATION_HORIZONTAL )
		{
			instance->item_area_dim.x = (output->w / 2) - (instance->item_area_dim.w / 2)
				+ (uint32_t)(config->margin.left - config->margin.right);
			instance->item_area_dim.y = config->border.top;
		}
		else
		{
			instance->item_area_dim.x = config->border.left;
			instance->item_area_dim.y = (output->h / 2) - (instance->item_area_dim.h / 2)
				+ (uint32_t)(config->margin.top - config->margin.bottom);
		}
	}

	/* Dimensions of the individual item instances. */
	uint32_t item_area_length = 0;
	{
		uint32_t x = instance->item_area_dim.x;
		uint32_t y = instance->item_area_dim.y;

		instance->active_items = context.item_amount;

		for (int i = 0; i < context.item_amount; i++)
		{
			instance->item_instances[i].active = true;
			instance->item_instances[i].dirty = true;

			instance->item_instances[i].x = (int32_t)x;
			instance->item_instances[i].y = (int32_t)y;

			switch (instance->item_instances[i].item->type)
			{
				case TYPE_BUTTON:
					instance->item_instances[i].w = config->size;
					instance->item_instances[i].h = config->size;
					if ( config->orientation == ORIENTATION_HORIZONTAL )
					{
						x += instance->item_instances[i].w;
						item_area_length += instance->item_instances[i].w;
					}
					else
					{
						y += instance->item_instances[i].h;
						item_area_length += instance->item_instances[i].h;
					}
					break;

				case TYPE_SPACER:
					if ( config->orientation == ORIENTATION_HORIZONTAL )
					{
						instance->item_instances[i].w = instance->item_instances[i].item->spacer_length;
						instance->item_instances[i].h = config->size;
						x += instance->item_instances[i].w;
						item_area_length += instance->item_instances[i].w;
					}
					else
					{
						instance->item_instances[i].w = config->size;
						instance->item_instances[i].h = instance->item_instances[i].item->spacer_length;
						y += instance->item_instances[i].h;
						item_area_length += instance->item_instances[i].h;
					}
					break;
			}
		}
	}

	/* Size of item area. */
	// TODO [item-rework] if bar gets too large, try de-activating a few items until it fits again
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		instance->item_area_dim.w = item_area_length;
		instance->item_area_dim.h = config->size;
	}
	else
	{
		instance->item_area_dim.w = config->size;
		instance->item_area_dim.h = item_area_length;
	}

	/* Other dimensions. */
	if ( instance->config->mode == MODE_DEFAULT )
	{
		/* Position of bar (on surface). */
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
	else if ( instance->config->mode == MODE_FULL )
	{
		if ( config->orientation == ORIENTATION_HORIZONTAL )
		{
			instance->bar_dim.x = (uint32_t)config->margin.left;
			instance->bar_dim.y = 0;

			instance->bar_dim.w = output->w - (uint32_t)(config->margin.left + config->margin.right);
			instance->bar_dim.h = instance->item_area_dim.h + config->border.top + config->border.bottom;

			instance->surface_dim.w = instance->output->w;
			instance->surface_dim.h = instance->bar_dim.h;

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

			instance->surface_dim.w = instance->bar_dim.w;
			instance->surface_dim.h = instance->output->h;

			instance->surface_hidden_dim.w = config->hidden_size;
			instance->surface_hidden_dim.h = instance->surface_dim.h;

			instance->bar_hidden_dim.w = config->hidden_size;
			instance->bar_hidden_dim.h = instance->bar_dim.h;
		}
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

struct Lava_bar_instance *create_bar_instance (struct Lava_output *output,
		struct Lava_bar_configuration *config)
{
	log_message(1, "[bar] Creating bar instance: global_name=%d\n", output->global_name);

	TRY_NEW(struct Lava_bar_instance, instance, NULL);

	instance->config        = config;
	instance->output        = output;
	instance->wl_surface    = NULL;
	instance->layer_surface = NULL;
	instance->configured    = false;
	instance->hover         = false;
	instance->dirty         = true;
	instance->hidden        = bar_instance_should_hide(instance);
	instance->frame_callback = NULL;

	/* Set up item_instances. */
	{
		instance->active_items   = context.item_amount;
		instance->item_instances = calloc((size_t)context.item_amount,
				sizeof(struct Lava_item_instance));

		/* Dimensions and active state get filled in when bar dimensions
		 * are calculated. Here we just assign item instances to their items.
		 */
		int i = 0;
		struct Lava_item *item;
		wl_list_for_each_reverse(item, &context.items, link)
		{
			instance->item_instances[i].item = item;
			instance->item_instances[i].indicator = 0;
			instance->item_instances[i].active_indicator = 0;
			instance->item_instances[i].toplevel_exists_indicator = 0;
			instance->item_instances[i].toplevel_activated_indicator = 0;

			/* If a toplevel with a matching app_id exist, apply indicators. */
			if ( item->associated_app_id != NULL )
			{
				struct Lava_toplevel *toplevel;
				wl_list_for_each(toplevel, &context.toplevels, link)
				{
					if ( toplevel->current.app_id != NULL && strcmp(item->associated_app_id, toplevel->current.app_id) == 0 )
					{
						instance->item_instances[i].toplevel_exists_indicator++;
						if (toplevel->current.activated) 
							instance->item_instances[i].toplevel_activated_indicator++;
					}
				}
			}

			i++;
		}
	}

	instance->wl_surface = wl_compositor_create_surface(context.compositor);
	instance->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			context.layer_shell, instance->wl_surface,
			output->wl_output, config->layer,
			str_orelse(config->namespace, "lhp.LavaLauncher"));


	/* We draw when this surface gets a configure event. */
	bar_instance_update_dimensions(instance);
	bar_instance_configure_layer_surface(instance);
	zwlr_layer_surface_v1_add_listener(instance->layer_surface,
			&layer_surface_listener, instance);
	wl_surface_commit(instance->wl_surface);

	return instance;
}

void destroy_bar_instance (struct Lava_bar_instance *instance)
{
	if ( instance == NULL )
		return;

	free_if_set(instance->item_instances);

	DESTROY(instance->layer_surface, zwlr_layer_surface_v1_destroy);
	DESTROY(instance->wl_surface, wl_surface_destroy);
	DESTROY(instance->frame_callback, wl_callback_destroy);

	finish_buffer(&instance->buffers[0]);
	finish_buffer(&instance->buffers[1]);

	free(instance);
}

void update_bar_instance (struct Lava_bar_instance *instance, bool need_new_dimensions,
		bool only_update_on_hide_change)
{
	/* It is possible that this function is called by output events before
	 * the bar instance has been created. This function will return and
	 * abort unless it is called either when a surface configure event has
	 * been received at least once, it is called by a surface configure
	 * event or it is called during the creation of the surface.
	 */
	if ( instance == NULL || ! instance->configured )
		return;

	/* An instance with no fitting configuration must be destroyed. */
	if ( instance->config == NULL )
	{
		log_message(2, "[bar] No configuration set, destroying bar: global-name=%d\n",
			instance->output->global_name);
		struct Lava_output *output = instance->output;
		destroy_bar_instance(instance);
		output->bar_instance = NULL;
		return;
	}

	if (need_new_dimensions)
		bar_instance_update_dimensions(instance);

	const bool currently_hidden = instance->hidden;
	instance->hidden = bar_instance_should_hide(instance);
	if ( only_update_on_hide_change && ( currently_hidden == instance->hidden ) )
		return;

	bar_instance_configure_layer_surface(instance);
	bar_instance_next_frame(instance);
	wl_surface_commit(instance->wl_surface);
}

/* Call this to handle all changes to a bar instance when it is entered by a pointer. */
void bar_instance_pointer_enter (struct Lava_bar_instance *instance)
{
	/* If binds using the keyboard have been defined, request keyboard interactivity.
         *
	 * Yes this is a ugly hack, because layer shell keyboard interactivity is seriously
	 * messed up. A bar on the top layer would /always/ have keyboard focus if we
	 * do not dynamically request and un-request it. Utter pain and does not work
	 * correctly with multiple seats, but its the only way to get this somewhat
	 * working, at least when the layer is either top or overlay. For bottom or
	 * background layer, keyboard focus requires an explicit focus, which for most
	 * compositors means clicking on the bar, so forget about scroll binds with
	 * modifiers on those layers. Basically forget about any sane keyboard interactivity.
	 * I have the feeling this part of the layer shell has not been thought through
	 * very thoroughly...
	 *
	 * Yes, I am annoyed. Because this sucks.
	 *
	 * Improvements or, better yet, a layer-shell replacement highly welcome.
	 */
	if (context.need_keyboard)
	{
		zwlr_layer_surface_v1_set_keyboard_interactivity(instance->layer_surface, true);
		wl_surface_commit(instance->wl_surface);
	}

	instance->hover = true;
	update_bar_instance(instance, false, true);
}

/* Call this to handle all changes to a bar instance when it is left by a pointer. */
void bar_instance_pointer_leave (struct Lava_bar_instance *instance)
{
	/* We have to check every seat before we can be sure that no pointer
	 * hovers over the bar. Only then can we hide the bar.
	 */
	struct Lava_seat *seat;
	wl_list_for_each(seat, &context.seats, link)
		if ( seat->pointer.instance == instance )
			return;

	/* If binds using the keyboard have been defined, unset keyboard interactivity.
	 *
	 * See comment in bar_instance_pointer_enter() to learn why this is a hack and
	 * then weep in agony and dispair.
	 */
	if (context.need_keyboard)
	{
		zwlr_layer_surface_v1_set_keyboard_interactivity(instance->layer_surface, false);
		wl_surface_commit(instance->wl_surface);
	}

	instance->hover = false;
	update_bar_instance(instance, false, true);
}

struct Lava_bar_instance *bar_instance_from_surface (struct wl_surface *surface)
{
	if ( surface == NULL )
		return NULL;
	struct Lava_output *output;
	wl_list_for_each(output, &context.outputs, link)
		if ( output->bar_instance != NULL && output->bar_instance->wl_surface == surface )
			return output->bar_instance;
	return NULL;
}

struct Lava_item_instance *bar_instance_get_item_instance_from_coords (
		struct Lava_bar_instance *instance, int32_t x, int32_t y)
{
	for (int i = 0; i < instance->active_items; i++)
	{
		if ( x < instance->item_instances[i].x )
			continue;
		if ( y < instance->item_instances[i].y )
			continue;
		if ( x >= instance->item_instances[i].x + (int32_t)instance->item_instances[i].w )
			continue;
		if ( y >= instance->item_instances[i].y + (int32_t)instance->item_instances[i].h )
			continue;
		return &((instance->item_instances)[i]);
	}

	return NULL;
}

