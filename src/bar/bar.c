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

#include<cairo/cairo.h>
#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"bar/bar.h"
#include"bar/bar-pattern.h"
#include"bar/draw-generics.h"
#include"bar/indicator.h"
#include"draw-generics.h"
#include"item.h"
#include"lavalauncher.h"
#include"str.h"
#include"output.h"
#include"types/box_t.h"
#include"types/buffer.h"
#include"types/image_t.h"
#include"types/string_t.h"

static void draw_items (struct Lava_bar *bar, cairo_t *cairo)
{
	struct Lava_bar_configuration *config  = bar->config;
	struct Lava_bar_pattern       *pattern = bar->pattern;

	uint32_t scale = bar->output->scale;
	uint32_t size = config->size * scale;
	uint32_t *increment, increment_offset;
	uint32_t x = 0, y = 0;
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = x;
	else
		increment = &y, increment_offset = y;

	struct Lava_item *item;
	wl_list_for_each_reverse(item, &pattern->items, link) if ( item->type == TYPE_BUTTON )
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

static void render_bar_frame (struct Lava_bar *bar)
{
	struct Lava_data              *data   = bar->data;
	struct Lava_bar_configuration *config = bar->config;
	struct Lava_output            *output = bar->output;
	uint32_t                       scale  = output->scale;

	ubox_t *buffer_dim, *bar_dim;
	if (bar->hidden)
		buffer_dim = &bar->surface_hidden, bar_dim = &bar->bar_hidden;
	else
		buffer_dim = &bar->surface, bar_dim = &bar->bar;

	log_message(data, 2, "[render] Render bar frame: global_name=%d\n", bar->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&bar->current_bar_buffer, data->shm, bar->bar_buffers,
				buffer_dim->w  * scale, buffer_dim->h * scale))
		return;

	cairo_t *cairo = bar->current_bar_buffer->cairo;
	clear_buffer(cairo);

	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	/* Draw bar. */
	if (! bar->hidden)
	{
		log_message(data, 2, "[render] Drawing bar.\n");
		draw_bar_background(cairo, bar_dim, &config->border, &config->radii,
				scale, &config->bar_colour, &config->border_colour);
	}

	wl_surface_set_buffer_scale(bar->bar_surface, (int32_t)scale);
	wl_surface_attach(bar->bar_surface, bar->current_bar_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->bar_surface, 0, 0, INT32_MAX, INT32_MAX);
}

static void render_icon_frame (struct Lava_bar *bar)
{
	struct Lava_data   *data    = bar->data;
	struct Lava_output *output  = bar->output;
	uint32_t            scale   = output->scale;

	log_message(data, 2, "[render] Render icon frame: global_name=%d\n", bar->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&bar->current_icon_buffer, data->shm, bar->icon_buffers,
				bar->item_area.w  * scale, bar->item_area.h * scale))
		return;

	cairo_t *cairo = bar->current_icon_buffer->cairo;
	clear_buffer(cairo);

	cairo_set_antialias(cairo, CAIRO_ANTIALIAS_BEST);

	/* Draw icons. */
	if (! bar->hidden)
		draw_items(bar, cairo);

	wl_surface_set_buffer_scale(bar->icon_surface, (int32_t)scale);
	wl_surface_attach(bar->icon_surface, bar->current_icon_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->icon_surface, 0, 0, INT32_MAX, INT32_MAX);
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

static void configure_layer_surface (struct Lava_bar *bar)
{
	struct Lava_data              *data   = bar->data;
	struct Lava_bar_configuration *config = bar->config;

	log_message(bar->data, 1, "[bar] Configuring bar: global_name=%d\n",
			bar->output->global_name);

	ubox_t *buffer_dim, *bar_dim;
	if (bar->hidden)
		buffer_dim = &bar->surface_hidden, bar_dim = &bar->bar_hidden;
	else
		buffer_dim = &bar->surface, bar_dim = &bar->bar;

	zwlr_layer_surface_v1_set_size(bar->layer_surface, buffer_dim->w, buffer_dim->h);

	/* Anchor the surface to the correct edge. */
	zwlr_layer_surface_v1_set_anchor(bar->layer_surface, get_anchor(config));

	if ( config->mode == MODE_DEFAULT )
		zwlr_layer_surface_v1_set_margin(bar->layer_surface,
				(int32_t)config->margin.top, (int32_t)config->margin.right,
				(int32_t)config->margin.bottom, (int32_t)config->margin.left);
	else if ( config->orientation == ORIENTATION_HORIZONTAL )
		zwlr_layer_surface_v1_set_margin(bar->layer_surface,
				(int32_t)config->margin.top, 0,
				(int32_t)config->margin.bottom, 0);
	else
		zwlr_layer_surface_v1_set_margin(bar->layer_surface,
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
	zwlr_layer_surface_v1_set_exclusive_zone(bar->layer_surface,
			exclusive_zone);

	/* Create a region of the visible part of the surface.
	 * Behold: In MODE_AGGRESSIVE, the actual surface is larger than the visible bar.
	 */
	struct wl_region *region = wl_compositor_create_region(data->compositor);
	wl_region_add(region, (int32_t)bar->bar.x, (int32_t)bar->bar.y,
			(int32_t)bar_dim->w, (int32_t)bar_dim->h);

	/* Set input region. This is necessary to prevent the unused parts of
	 * the surface to catch pointer and touch events.
	 */
	wl_surface_set_input_region(bar->bar_surface, region);

	wl_region_destroy(region);
}


static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_bar  *bar  = (struct Lava_bar *)raw_data;
	struct Lava_data *data = bar->data;

	log_message(data, 1, "[bar] Layer surface configure request: global_name=%d w=%d h=%d serial=%d\n",
			bar->output->global_name, w, h, serial);

	bar->configured = true;
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	update_bar(bar);
}

static void layer_surface_handle_closed (void *data,
		struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_bar *bar = (struct Lava_bar *)data;
	log_message(bar->data, 1, "[bar] Layer surface has been closed: global_name=%d\n",
				bar->output->global_name);
	destroy_bar(bar);
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

static void configure_subsurface (struct Lava_bar *bar)
{
	struct Lava_data        *data    = bar->data;

	log_message(bar->data, 1, "[bar] Configuring icons: global_name=%d\n",
			bar->output->global_name);

	wl_subsurface_set_position(bar->subsurface, (int32_t)bar->item_area.x, (int32_t)bar->item_area.y);

	/* We do not want to receive any input events from the subsurface.
	 * Almot everything in LavaLauncher uses the coords of the parent surface.
	 */
	struct wl_region *region = wl_compositor_create_region(data->compositor);
	wl_surface_set_input_region(bar->icon_surface, region);
	wl_region_destroy(region);
}

/* Positions and dimensions for MODE_AGGRESSIVE. */
static void mode_aggressive_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config = bar->config;
	struct Lava_output           *output  = bar->output;

	/* Position of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL ) switch (config->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area.x = config->border.left + (uint32_t)config->margin.left;
			bar->item_area.y = config->border.top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area.x = (output->w / 2) - (bar->item_area.w / 2)
				+ (uint32_t)(config->margin.left - config->margin.right);
			bar->item_area.y = config->border.top;
			break;

		case ALIGNMENT_END:
			bar->item_area.x = output->w - bar->item_area.w
				- config->border.right - (uint32_t)config->margin.right;
			bar->item_area.y = config->border.top;
			break;
	}
	else switch (config->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area.x = config->border.left;
			bar->item_area.y = config->border.top + (uint32_t)config->margin.top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area.x = config->border.left;
			bar->item_area.y = (output->h / 2) - (bar->item_area.h / 2)
				+ (uint32_t)(config->margin.top - config->margin.bottom);
			break;

		case ALIGNMENT_END:
			bar->item_area.x = config->border.left;
			bar->item_area.y = output->h - bar->item_area.h
				- config->border.bottom - (uint32_t)config->margin.bottom;
			break;
	}

	/* Position of bar. */
	bar->bar.x = bar->item_area.x - config->border.left;
	bar->bar.y = bar->item_area.y - config->border.top;

	/* Size of bar. */
	bar->bar.w = bar->item_area.w + config->border.left + config->border.right;
	bar->bar.h = bar->item_area.h + config->border.top  + config->border.bottom;

	/* Size of buffer / surface and hidden dimensions. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->surface.w = bar->output->w;
		bar->surface.h = config->size + config->border.top + config->border.bottom;

		bar->surface_hidden.w = bar->surface.w;
		bar->surface_hidden.h = config->hidden_size;

		bar->bar_hidden.w = bar->bar.w;
		bar->bar_hidden.h = config->hidden_size;
	}
	else
	{
		bar->surface.w = config->size + config->border.left + config->border.right;
		bar->surface.h = bar->output->h;

		bar->surface_hidden.w = config->hidden_size;
		bar->surface_hidden.h = bar->surface.h;

		bar->bar_hidden.w = config->hidden_size;
		bar->bar_hidden.h = bar->bar.h;
	}
	bar->bar_hidden.x = bar->bar.x;
	bar->bar_hidden.y = bar->bar.y;
}

/* Positions and dimensions for MODE_FULL. */
static void mode_full_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config = bar->config;
	struct Lava_output           *output  = bar->output;

	/* Position of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL ) switch (config->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area.x = config->border.left + (uint32_t)config->margin.left;
			bar->item_area.y = config->border.top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area.x = (output->w / 2) - (bar->item_area.w / 2)
				+ (uint32_t)(config->margin.left - config->margin.right);
			bar->item_area.y = config->border.top;
			break;

		case ALIGNMENT_END:
			bar->item_area.x = output->w - bar->item_area.w
				- config->border.right - (uint32_t)config->margin.right;
			bar->item_area.y = config->border.top;
			break;
	}
	else switch (config->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area.x = config->border.left;
			bar->item_area.y = config->border.top + (uint32_t)config->margin.top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area.x = config->border.left;
			bar->item_area.y = (output->h / 2) - (bar->item_area.h / 2)
				+ (uint32_t)(config->margin.top - config->margin.bottom);
			break;

		case ALIGNMENT_END:
			bar->item_area.x = config->border.left;
			bar->item_area.y = output->h - bar->item_area.h
				- config->border.bottom - (uint32_t)config->margin.bottom;
			break;
	}

	/* Position and size of bar and size of buffer / surface and hidden dimensions. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->bar.x = (uint32_t)config->margin.left;
		bar->bar.y = 0;

		bar->bar.w = output->w - (uint32_t)(config->margin.left + config->margin.right);
		bar->bar.h = bar->item_area.h + config->border.top + config->border.bottom;

		bar->surface.w = bar->output->w;
		bar->surface.h = config->size + config->border.top + config->border.bottom;

		bar->surface_hidden.w = bar->surface.w;
		bar->surface_hidden.h = config->hidden_size;

		bar->bar_hidden.w = bar->bar.w;
		bar->bar_hidden.h = config->hidden_size;
	}
	else
	{
		bar->bar.x = 0;
		bar->bar.y = (uint32_t)config->margin.top;

		bar->bar.w = bar->item_area.w + config->border.left + config->border.right;
		bar->bar.h = output->h - (uint32_t)(config->margin.top + config->margin.bottom);

		bar->surface.w = config->size + config->border.left + config->border.right;
		bar->surface.h = bar->output->h;

		bar->surface_hidden.w = config->hidden_size;
		bar->surface_hidden.h = bar->surface.h;

		bar->bar_hidden.w = config->hidden_size;
		bar->bar_hidden.h = bar->bar.h;
	}
}

/* Position and size for MODE_DEFAULT. */
static void mode_default_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config = bar->config;

	/* Position of item area. */
	bar->item_area.x = config->border.left;
	bar->item_area.y = config->border.top;

	/* Position of bar. */
	bar->bar.x = 0;
	bar->bar.y = 0;

	/* Size of bar. */
	bar->bar.w = bar->item_area.w + config->border.left + config->border.right;
	bar->bar.h = bar->item_area.h + config->border.top  + config->border.bottom;

	/* Size of hidden bar. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->bar_hidden.w = bar->bar.w;
		bar->bar_hidden.h = config->hidden_size;
	}
	else
	{
		bar->bar_hidden.w = config->hidden_size;
		bar->bar_hidden.h = bar->bar.h;
	}

	/* Size of buffer / surface. */
	bar->surface = bar->bar;
	bar->surface_hidden = bar->bar_hidden;
}

static void update_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_configuration *config  = bar->config;
	struct Lava_bar_pattern       *pattern = bar->pattern;
	struct Lava_output            *output  = bar->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	/* Size of item area. */
	if ( config->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->item_area.w = get_item_length_sum(pattern);
		bar->item_area.h = config->size;
	}
	else
	{
		bar->item_area.w = config->size;
		bar->item_area.h = get_item_length_sum(pattern);
	}

	/* Other dimensions. */
	switch (bar->config->mode)
	{
		case MODE_DEFAULT:    mode_default_dimensions(bar);    break;
		case MODE_FULL:       mode_full_dimensions(bar);       break;
		case MODE_AGGRESSIVE: mode_aggressive_dimensions(bar); break;
	}
}

/* Return a bool indicating if the bar should currently be hidden or not. */
static bool bar_should_hide (struct Lava_bar *bar)
{
	switch(bar->config->hidden_mode)
	{
		case HIDDEN_MODE_ALWAYS:
			return !bar->hover;

		case HIDDEN_MODE_RIVER_AUTO:
			return ( bar->output->river_output_occupied && ! bar->hover );

		case HIDDEN_MODE_NEVER:
		default:
			return false;
	}
}

void bar_update_hidden_status (struct Lava_bar *bar)
{
	bool current = bar->hidden;
	bar->hidden = bar_should_hide(bar);

	/* No need to do something if nothing changed. */
	if (current == bar->hidden)
		return;

	update_bar(bar);
}

bool create_bar (struct Lava_bar_pattern *pattern, struct Lava_bar_configuration *config,
		struct Lava_output *output)
{
	struct Lava_data *data = pattern->data;
	log_message(data, 1, "[bar] Creating bar: global_name=%d\n", output->global_name);

	struct Lava_bar *bar = calloc(1, sizeof(struct Lava_bar));
	if ( bar == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	wl_list_insert(&output->bars, &bar->link);
	bar->data          = data;
	bar->pattern       = pattern;
	bar->config        = config;
	bar->output        = output;
	bar->bar_surface   = NULL;
	bar->icon_surface  = NULL;
	bar->layer_surface = NULL;
	bar->subsurface    = NULL;
	bar->configured    = false;
	bar->hover         = false;
	bar->hidden        = bar_should_hide(bar);

	wl_list_init(&bar->indicators);

	/* Main surface for the bar. */
	if ( NULL == (bar->bar_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		return false;
	}
	if ( NULL == (bar->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					data->layer_shell, bar->bar_surface,
					output->wl_output, config->layer,
					str_orelse(config->namespace, "LavaLauncher"))) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create layer_surface.\n");
		return false;
	}

	/* Subsurface for the icons. */
	if ( NULL == (bar->icon_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		return false;
	}
	if ( NULL == (bar->subsurface = wl_subcompositor_get_subsurface(
					data->subcompositor, bar->icon_surface,
					bar->bar_surface)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_subsurface.\n");
		return false;
	}

	update_dimensions(bar);
	configure_layer_surface(bar);
	configure_subsurface(bar);
	zwlr_layer_surface_v1_add_listener(bar->layer_surface,
			&layer_surface_listener, bar);
	wl_surface_commit(bar->icon_surface);
	wl_surface_commit(bar->bar_surface);

	return true;
}

void destroy_bar (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_item_indicator *indicator, *temp;
	wl_list_for_each_safe(indicator, temp, &bar->indicators, link)
		destroy_indicator(indicator);

	if ( bar->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(bar->layer_surface);
	if ( bar->subsurface != NULL )
		wl_subsurface_destroy(bar->subsurface);
	if ( bar->bar_surface != NULL )
		wl_surface_destroy(bar->bar_surface);
	if ( bar->icon_surface != NULL )
		wl_surface_destroy(bar->icon_surface);

	finish_buffer(&bar->bar_buffers[0]);
	finish_buffer(&bar->bar_buffers[1]);
	finish_buffer(&bar->icon_buffers[0]);
	finish_buffer(&bar->icon_buffers[1]);

	wl_list_remove(&bar->link);
	free(bar);
}

void destroy_all_bars (struct Lava_output *output)
{
	log_message(output->data, 1, "[bar] Destroying bars: global-name=%d\n", output->global_name);
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(b1, b2, &output->bars, link)
		destroy_bar(b1);
}

void update_bar (struct Lava_bar *bar)
{
	/* It is possible that this function is called by output events before
	 * the bar has been created. This function will return and abort unless
	 * it is called either when a surface configure event has been received
	 * at least once, it is called by a surface configure event or
	 * it is called during the creation of the surface.
	 */
	if ( bar == NULL || ! bar->configured )
		return;

	update_dimensions(bar);

	configure_subsurface(bar);
	configure_layer_surface(bar);

	render_icon_frame(bar);
	render_bar_frame(bar);

	wl_surface_commit(bar->icon_surface);
	wl_surface_commit(bar->bar_surface);
}

struct Lava_bar *bar_from_surface (struct Lava_data *data, struct wl_surface *surface)
{
	if ( data == NULL || surface == NULL )
		return NULL;
	struct Lava_output *op1, *op2;
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(op1, op2, &data->outputs, link)
	{
		wl_list_for_each_safe(b1, b2, &op1->bars, link)
		{
			if ( b1->bar_surface == surface )
				return b1;
		}
	}
	return NULL;
}

