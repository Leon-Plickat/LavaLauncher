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

#include"lavalauncher.h"
#include"output.h"
#include"bar/bar.h"
#include"bar/render.h"
#include"bar/draw-generics.h"
#include"bar/buffer.h"
#include"item/item.h"
#include"config/config.h"

static void item_replace_background (cairo_t *cairo, int32_t x, int32_t y,
		int32_t size, float colour[4])
{
	cairo_save(cairo);
	cairo_rectangle(cairo, x, y, size, size);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_fill(cairo);
	cairo_restore(cairo);
}

static void draw_effect (cairo_t *cairo, int32_t x, int32_t y, int32_t size,
		int32_t padding, float colour[4], enum Draw_effect effect)
{
	if ( effect == EFFECT_NONE )
		return;

	cairo_save(cairo);

	x += padding, y += padding, size -= 2 * padding;
	double radius, degrees;
	switch (effect)
	{
		case EFFECT_BOX:
			cairo_rectangle(cairo, x, y, size, size);
			break;

		case EFFECT_PHONE:
			radius = size / 10.0;
			degrees = 3.1415927 / 180.0;

			cairo_new_sub_path(cairo);
			cairo_arc(cairo, x + size - radius, y + radius, radius,
					-90 * degrees, 0 * degrees);
			cairo_arc(cairo, x + size - radius, y + size - radius,
					radius, 0 * degrees, 90 * degrees);
			cairo_arc(cairo, x + radius, y + size - radius, radius,
					90 * degrees, 180 * degrees);
			cairo_arc(cairo, x + radius, y + radius, radius,
					180 * degrees, 270 * degrees);
			cairo_close_path(cairo);
			break;

		default:
			break;
	}

	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_fill(cairo);

	cairo_restore(cairo);
}

static void calculate_bar_buffer_size (struct Lava_output *output, int *w, int *h)
{
	struct Lava_config *config = output->data->config;

	if ( config->mode == MODE_SIMPLE )
		*w = config->w, *h = config->h;
	else if ( config->orientation == ORIENTATION_HORIZONTAL )
		*w = output->w, *h = config->h;
	else
		*w = config->w, *h = output->h;

	*w *= output->scale, *h *= output->scale;
}

static void draw_items (struct Lava_bar *bar, cairo_t *cairo)
{
	struct Lava_data   *data     = bar->data;
	struct Lava_config *config   = data->config;

	int scale    = bar->output->scale,
	    x_offset = (bar->x_offset + config->border_top)  * scale,
	    y_offset = (bar->y_offset + config->border_left) * scale;

	int32_t size = config->icon_size * scale;
	int32_t x = x_offset, y = y_offset, *increment, *increment_offset;
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = &x_offset;
	else
		increment = &y, increment_offset = &y_offset;

	struct Lava_item *it_1, *it_2;
	wl_list_for_each_reverse_safe(it_1, it_2, &data->items, link)
		if ( it_1->type == TYPE_BUTTON )
		{
			*increment = (it_1->ordinate * scale) + *increment_offset;
			if ( it_1->background_colour_hex != NULL )
				item_replace_background(cairo, x, y, it_1->length,
						it_1->background_colour);
			draw_effect(cairo, x, y, size, config->effect_padding,
					config->effect_colour, config->effect);
			lldg_draw_square_image(cairo, x, y, size, it_1->img);
		}
}

void render_bar_frame (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_data   *data   = bar->data;
	struct Lava_config *config = data->config;
	struct Lava_output *output = bar->output;

	if ( output->status != OUTPUT_STATUS_SURFACE_CONFIGURED )
		return;

	/* Get new/next buffer. */
	int buffer_w, buffer_h;
	calculate_bar_buffer_size(output, &buffer_w, &buffer_h);
	if (! next_buffer(&bar->current_buffer, data->shm, bar->buffers,
				buffer_w, buffer_h))
		return;

	cairo_t *cairo = bar->current_buffer->cairo;
	lldg_clear_buffer(cairo);

	/* Draw bar. */
	if (data->verbose)
		fputs("Drawing bar.\n", stderr);
	if ( config->mode == MODE_FULL )
	{
		int bar_w, bar_h;
		if ( config->orientation == ORIENTATION_HORIZONTAL )
			bar_w = output->w, bar_h = config->h;
		else
			bar_w = config->w, bar_h = config->h;

		lldg_draw_bordered_rectangle(cairo, 0, 0, bar_w, bar_h,
				config->border_top, config->border_right,
				config->border_bottom, config->border_left,
				bar->output->scale,
				config->bar_colour, config->border_colour);
	}
	else
		lldg_draw_bordered_rectangle(cairo,
				bar->x_offset, bar->y_offset, config->w, config->h,
				config->border_top, config->border_right,
				config->border_bottom, config->border_left,
				output->scale, config->bar_colour, config->border_colour);

	/* Draw icons. */
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);
	draw_items(bar, cairo);

	/* Commit surface. */
	if (data->verbose)
		fputs("Committing surface.\n", stderr);
	wl_surface_set_buffer_scale(bar->wl_surface, output->scale);
	wl_surface_attach(bar->wl_surface, bar->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(bar->wl_surface);
}
