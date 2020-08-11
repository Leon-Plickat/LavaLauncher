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
#include"types/image.h"
#include"types/buffer.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"bar/render.h"
#include"bar/draw-generics.h"
#include"item/item.h"
#include"types/colour.h"

static void draw_items (struct Lava_bar *bar, cairo_t *cairo)
{
	struct Lava_bar_pattern *pattern = bar->pattern;

	uint32_t scale = bar->output->scale;
	uint32_t size = pattern->size * scale;
	uint32_t *increment, increment_offset;
	uint32_t x = 0, y = 0;
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = x;
	else
		increment = &y, increment_offset = y;

	struct Lava_item *item;
	wl_list_for_each_reverse(item, &pattern->items, link) if ( item->type == TYPE_BUTTON )
	{
		*increment = (item->ordinate * scale) + increment_offset;
		if ( item->img != NULL )
			image_draw_to_cairo(cairo, item->img,
					x + pattern->icon_padding,
					y + pattern->icon_padding,
					size - (2 * pattern->icon_padding),
					size - (2 * pattern->icon_padding));
	}
}

void render_bar_frame (struct Lava_bar *bar)
{
	struct Lava_data        *data    = bar->data;
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;
	uint32_t                 scale   = output->scale;

	log_message(data, 2, "[render] Render bar frame: global_name=%d\n", bar->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&bar->current_bar_buffer, data->shm, bar->bar_buffers,
				bar->buffer_width  * scale,
				bar->buffer_height * scale))
		return;

	cairo_t *cairo = bar->current_bar_buffer->cairo;
	clear_buffer(cairo);

	/* Draw bar. */
	log_message(data, 2, "[render] Drawing bar.\n");
	draw_bar_background(cairo,
			bar->bar_x, bar->bar_y, bar->bar_width, bar->bar_height,
			pattern->border_top, pattern->border_right,
			pattern->border_bottom, pattern->border_left,
			pattern->radius_top_left, pattern->radius_top_right,
			pattern->radius_bottom_left, pattern->radius_bottom_right,
			scale, &pattern->bar_colour, &pattern->border_colour);

	wl_surface_set_buffer_scale(bar->bar_surface, (int32_t)scale);
	wl_surface_attach(bar->bar_surface, bar->current_bar_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->bar_surface, 0, 0, INT32_MAX, INT32_MAX);
}

void render_icon_frame (struct Lava_bar *bar)
{
	struct Lava_data   *data    = bar->data;
	struct Lava_output *output  = bar->output;
	uint32_t            scale   = output->scale;

	log_message(data, 2, "[render] Render icon frame: global_name=%d\n", bar->output->global_name);

	/* Get new/next buffer. */
	if (! next_buffer(&bar->current_icon_buffer, data->shm, bar->icon_buffers,
				bar->item_area_width  * scale,
				bar->item_area_height * scale))
		return;

	cairo_t *cairo = bar->current_icon_buffer->cairo;
	clear_buffer(cairo);

	/* Draw icons. */
	draw_items(bar, cairo);

	wl_surface_set_buffer_scale(bar->icon_surface, (int32_t)scale);
	wl_surface_attach(bar->icon_surface, bar->current_icon_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(bar->icon_surface, 0, 0, INT32_MAX, INT32_MAX);
}
