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
#include<cairo/cairo.h>
#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"lavalauncher.h"
#include"seat.h"
#include"output.h"
#include"bar/bar.h"
#include"bar/bar-pattern.h"
#include"item/item.h"
#include"bar/draw-generics.h"
#include"bar/indicator.h"
#include"types/colour_t.h"
#include"str.h"

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

	wl_surface_commit(indicator->bar->bar_surface);
	wl_list_remove(&indicator->link);
	free(indicator);
}

struct Lava_item_indicator *create_indicator (struct Lava_bar *bar)
{
	struct Lava_data *data = bar->data;

	struct Lava_item_indicator *indicator = calloc(1, sizeof(struct Lava_item_indicator));
	if ( indicator == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return NULL;
	}

	wl_list_insert(&bar->indicators, &indicator->link);

	indicator->seat       = NULL;
	indicator->touchpoint = NULL;
	indicator->bar        = bar;

	if ( NULL == (indicator->indicator_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_surface.\n");
		goto error;
	}
	if ( NULL == (indicator->indicator_subsurface = wl_subcompositor_get_subsurface(
					data->subcompositor, indicator->indicator_surface,
					bar->bar_surface)) )
	{
		log_message(NULL, 0, "ERROR: Compositor did not create wl_subsurface.\n");
		goto error;
	}

	wl_subsurface_set_desync(indicator->indicator_subsurface);
	wl_subsurface_place_below(indicator->indicator_subsurface, bar->icon_surface);
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
	struct Lava_bar         *bar     = indicator->bar;
	struct Lava_data        *data    = bar->data;
	struct Lava_bar_pattern *pattern = bar->pattern;
	uint32_t                 scale   = bar->output->scale;

	uint32_t buffer_size = pattern->size - (2 * pattern->indicator_padding);
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

	switch (pattern->indicator_style)
	{
		case STYLE_RECTANGLE:
			/* Cairo implicitly fills everything if no shape has been drawn. */
			cairo_rectangle(cairo, 0, 0, buffer_size, buffer_size);
			break;

		case STYLE_ROUNDED_RECTANGLE:
			rounded_rectangle(cairo, 0, 0, buffer_size, buffer_size,
					pattern->radius_top_left, pattern->radius_top_right,
					pattern->radius_bottom_left, pattern->radius_bottom_right);
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
	struct Lava_bar         *bar     = indicator->bar;
	struct Lava_bar_pattern *pattern = bar->pattern;

	int32_t x = (int32_t)(bar->item_area.x + pattern->indicator_padding);
	int32_t y = (int32_t)(bar->item_area.y + pattern->indicator_padding);
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		x += (int32_t)item->ordinate;
	else
		y += (int32_t)item->ordinate;

	wl_subsurface_set_position(indicator->indicator_subsurface, x, y);
}

void indicator_commit (struct Lava_item_indicator *indicator)
{
	wl_surface_commit(indicator->indicator_surface);
	wl_surface_commit(indicator->bar->bar_surface);
}

