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

#include"lavalauncher.h"
#include"output.h"
#include"draw-generics.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"bar/layersurface.h"
#include"bar/render.h"
#include"item/item.h"

/* Positions and dimensions for MODE_DEFAULT. */
static void mode_default_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;

	/* Position of item area. */
	if ( pattern->orientation == ORIENTATION_HORIZONTAL ) switch (pattern->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area_x = pattern->border_left + pattern->margin_left;
			bar->item_area_y = pattern->border_top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area_x = (output->w / 2) - (bar->item_area_width / 2)
				+ (pattern->margin_left - pattern->margin_right);
			bar->item_area_y = pattern->border_top;
			break;

		case ALIGNMENT_END:
			bar->item_area_x = output->w - bar->item_area_width
				- pattern->border_right - pattern->margin_right;
			bar->item_area_y = pattern->border_top;
			break;
	}
	else switch (pattern->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = pattern->border_top + pattern->margin_top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = (output->h / 2) - (bar->item_area_height / 2)
				+ (pattern->margin_top - pattern->margin_bottom);
			break;

		case ALIGNMENT_END:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = output->h - bar->item_area_height
				- pattern->border_bottom - pattern->margin_bottom;
			break;
	}

	/* Position of bar. */
	bar->bar_x = bar->item_area_x - pattern->border_left;
	bar->bar_y = bar->item_area_y - pattern->border_top;

	/* Size of bar. */
	bar->bar_width  = bar->item_area_width  + pattern->border_left + pattern->border_right;
	bar->bar_height = bar->item_area_height + pattern->border_top  + pattern->border_bottom;

	/* Size of buffer / surface. */
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->buffer_width  = bar->output->w;
		bar->buffer_height = pattern->icon_size
			+ pattern->border_top
			+ pattern->border_bottom;
	}
	else
	{
		bar->buffer_width  = pattern->icon_size
			+ pattern->border_left
			+ pattern->border_right;
		bar->buffer_height = bar->output->h;
	}
}

/* Positions and dimensions for MODE_FULL. */
static void mode_full_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;

	/* Position of item area. */
	if ( pattern->orientation == ORIENTATION_HORIZONTAL ) switch (pattern->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area_x = pattern->border_left + pattern->margin_left;
			bar->item_area_y = pattern->border_top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area_x = (output->w / 2) - (bar->item_area_width / 2)
				+ (pattern->margin_left - pattern->margin_right);
			bar->item_area_y = pattern->border_top;
			break;

		case ALIGNMENT_END:
			bar->item_area_x = output->w - bar->item_area_width
				- pattern->border_right - pattern->margin_right;
			bar->item_area_y = pattern->border_top;
			break;
	}
	else switch (pattern->alignment)
	{
		case ALIGNMENT_START:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = pattern->border_top + pattern->margin_top;
			break;

		case ALIGNMENT_CENTER:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = (output->h / 2) - (bar->item_area_height / 2)
				+ (pattern->margin_top - pattern->margin_bottom);
			break;

		case ALIGNMENT_END:
			bar->item_area_x = pattern->border_left;
			bar->item_area_y = output->h - bar->item_area_height
				- pattern->border_bottom - pattern->margin_bottom;
			break;
	}

	/* Position and size of bar and size of buffer / surface. */
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->bar_x = pattern->margin_left;
		bar->bar_y = 0;

		bar->bar_width  = output->w - pattern->margin_left - pattern->margin_right;
		bar->bar_height = bar->item_area_height + pattern->border_top + pattern->border_bottom;

		bar->buffer_width  = bar->output->w;
		bar->buffer_height = pattern->icon_size
			+ pattern->border_top
			+ pattern->border_bottom;
	}
	else
	{
		bar->bar_x = 0;
		bar->bar_y = pattern->margin_top;

		bar->bar_width  = bar->item_area_width + pattern->border_left + pattern->border_right;
		bar->bar_height = output->h - pattern->margin_top - pattern->margin_bottom;

		bar->buffer_width  = pattern->icon_size
			+ pattern->border_left
			+ pattern->border_right;
		bar->buffer_height = bar->output->h;
	}
}

/* Position and size for MODE_SIMPLE. */
static void mode_simple_dimensions (struct Lava_bar *bar)
{
	struct Lava_bar_pattern *pattern = bar->pattern;

	/* Position of item area. */
	bar->item_area_x = pattern->border_left;
	bar->item_area_y = pattern->border_top;

	/* Position of bar. */
	bar->bar_x = 0;
	bar->bar_y = 0;

	/* Size of bar. */
	bar->bar_width  = bar->item_area_width  + pattern->border_left + pattern->border_right;
	bar->bar_height = bar->item_area_height + pattern->border_top  + pattern->border_bottom;

	/* Size of buffer / surface. */
	bar->buffer_width  = bar->bar_width;
	bar->buffer_height = bar->bar_height;
}

static void update_dimensions (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	/* Size of item area. */
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
	{
		bar->item_area_width  = get_item_length_sum(pattern);
		bar->item_area_height = pattern->icon_size;
	}
	else
	{
		bar->item_area_width  = pattern->icon_size;
		bar->item_area_height = get_item_length_sum(pattern);
	}

	/* Other dimensions. */
	switch (bar->pattern->mode)
	{
		case MODE_DEFAULT: mode_default_dimensions(bar); break;
		case MODE_FULL:    mode_full_dimensions(bar);    break;
		case MODE_SIMPLE:  mode_simple_dimensions(bar);  break;
	}
}

bool create_bar (struct Lava_bar_pattern *pattern, struct Lava_output *output)
{
	struct Lava_data *data = pattern->data;
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	struct Lava_bar *bar = calloc(1, sizeof(struct Lava_bar));
	if ( bar == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return false;
	}
	wl_list_insert(&output->bars, &bar->link);
	bar->data          = data;
	bar->pattern       = pattern;
	bar->output        = output;
	bar->wl_surface    = NULL;
	bar->layer_surface = NULL;

	if ( NULL == (bar->wl_surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Compositor did not create wl_surface.\n", stderr);
		return false;
	}

	if ( NULL == (bar->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					data->layer_shell, bar->wl_surface,
					output->wl_output, pattern->layer,
					"LavaLauncher")) )
	{
		fputs("ERROR: Compositor did not create layer_surface.\n", stderr);
		return false;
	}

	update_dimensions(bar);
	configure_layer_surface(bar);
	zwlr_layer_surface_v1_add_listener(bar->layer_surface,
			&layer_surface_listener, bar);
	wl_surface_commit(bar->wl_surface);

	return true;
}

static void destroy_bar (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	if ( bar->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(bar->layer_surface);
	if ( bar->wl_surface != NULL )
		wl_surface_destroy(bar->wl_surface);
	wl_list_remove(&bar->link);
	free(bar);
}

void destroy_all_bars (struct Lava_output *output)
{
	if (output->data->verbose)
		fputs("Destroying bars.\n", stderr);
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(b1, b2, &output->bars, link)
		destroy_bar(b1);
}

void update_bar (struct Lava_bar *bar)
{
	update_dimensions(bar);
	configure_layer_surface(bar);
	render_bar_frame(bar);
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
			if ( b1->wl_surface == surface )
				return b1;
		}
	}
	return NULL;
}

