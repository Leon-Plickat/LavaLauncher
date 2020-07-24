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
#include"log.h"
#include"output.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"bar/render.h"
#include"bar/draw-generics.h"
#include"bar/buffer.h"
#include"item/item.h"

static void item_replace_background (cairo_t *cairo, uint32_t x, uint32_t y,
		uint32_t size, float colour[4])
{
	cairo_save(cairo);
	cairo_rectangle(cairo, x, y, size, size);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_fill(cairo);
	cairo_restore(cairo);
}

static void draw_effect (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t size,
		uint32_t padding, float colour[4], enum Draw_effect effect)
{
	if ( effect == EFFECT_NONE )
		return;

	cairo_save(cairo);

	x += padding, y += padding, size -= 2 * padding;
	switch (effect)
	{
		case EFFECT_BOX:
			cairo_rectangle(cairo, x, y, size, size);
			break;

		case EFFECT_PHONE:
			lldg_rounded_square(cairo, x, y, size);
			break;

		case EFFECT_CIRCLE:
			lldg_circle(cairo, x, y, size);
			break;

		default:
			break;
	}

	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_fill(cairo);

	cairo_restore(cairo);
}

static void draw_items (struct Lava_bar *bar, cairo_t *cairo)
{
	struct Lava_bar_pattern *pattern = bar->pattern;

	uint32_t scale = bar->output->scale;
	uint32_t size = pattern->size * scale;
	uint32_t *increment, increment_offset;
	uint32_t x = bar->item_area_x * scale, y = bar->item_area_y * scale;
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = x;
	else
		increment = &y, increment_offset = y;

	struct Lava_item *item, *temp;
	wl_list_for_each_reverse_safe(item, temp, &pattern->items, link)
		if ( item->type == TYPE_BUTTON )
		{
			*increment = (item->ordinate * scale) + increment_offset;
			if (item->replace_background)
				item_replace_background(cairo, x, y, item->length,
						item->background_colour);
			draw_effect(cairo, x, y, size, pattern->effect_padding,
					pattern->effect_colour, pattern->effect);
			lldg_draw_square_image(cairo,
					x + pattern->icon_padding,
					y + pattern->icon_padding,
					size - (2 * pattern->icon_padding),
					item->img);
		}
}

void render_bar_frame (struct Lava_bar *bar)
{
	struct Lava_data        *data    = bar->data;
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;
	uint32_t                 scale   = output->scale;

	log_message(data, 1, "[render] Render bar fraome: global_name=%d\n", bar->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&bar->current_buffer, data->shm, bar->buffers,
				bar->buffer_width  * scale,
				bar->buffer_height * scale))
		return;

	cairo_t *cairo = bar->current_buffer->cairo;
	lldg_clear_buffer(cairo);

	/* Draw bar. */
	log_message(data, 2, "[render] Drawing bar.\n");
	lldg_draw_bordered_rectangle(cairo,
			bar->bar_x, bar->bar_y, bar->bar_width, bar->bar_height,
			pattern->border_top, pattern->border_right,
			pattern->border_bottom, pattern->border_left,
			scale, pattern->bar_colour, pattern->border_colour);

	/* Draw icons. */
	log_message(data, 2, "[render] Drawing icons.\n");
	draw_items(bar, cairo);

	/* Commit surface. */
	log_message(data, 2, "[render] Commiting surface.\n");
	wl_surface_set_buffer_scale(bar->wl_surface, (int32_t)scale);
	wl_surface_attach(bar->wl_surface, bar->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(bar->wl_surface);
}
