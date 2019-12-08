/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<cairo/cairo.h>
#include<wayland-server.h>

#include"lavalauncher.h"
#include"draw.h"

static void cairo_draw_bar_icons (cairo_t *cairo, struct Lava_data *data)
{
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);

	cairo_surface_t *image = NULL;

	int x = 0, y = 0;
	enum Orientation orientation;

	switch (data->position)
	{
		case POSITION_CENTER:
		case POSITION_BOTTOM:
			y += data->border_width;
		case POSITION_TOP:
			x += data->border_width;
			orientation = ORIENTATION_HORIZONTAL;
			break;

		case POSITION_RIGHT:
			x += data->border_width;
		case POSITION_LEFT:
			y += data->border_width;
			orientation = ORIENTATION_VERTICAL;
			break;
	}

	float w, h;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		cairo_save(cairo);

		image = cairo_image_surface_create_from_png(bt_1->img_path);
		w = cairo_image_surface_get_width (image);
		h = cairo_image_surface_get_height (image);
		cairo_translate(cairo, x, y);
		cairo_scale(cairo, data->bar_width / w, data->bar_width / h);
		cairo_set_source_surface(cairo, image, 0, 0);
		cairo_paint(cairo);
		cairo_surface_destroy(image);

		cairo_restore(cairo);


		if ( orientation == ORIENTATION_HORIZONTAL )
			x += data->bar_width;
		else if (orientation == ORIENTATION_VERTICAL )
			y += data->bar_width;
	}
}

static void cairo_draw_coloured_rectangle (cairo_t *cairo, float colour[4],
		int x, int y,
		int w, int h)
{
	cairo_set_source_rgba(cairo,
			colour[0],
			colour[1],
			colour[2],
			colour[3]);
	cairo_rectangle(cairo, x, y, w, h);
	cairo_close_path(cairo);
	cairo_fill(cairo);
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fputs("Rendering bar frame.\nPreparing buffer.\n", stderr);

	int buffer_w = data->w * output->scale, buffer_h = data->h * output->scale;
	output->current_buffer = get_next_buffer(data->shm,
			output->buffers,
			buffer_w, buffer_h);
	if (! output->current_buffer)
		return;

	cairo_t *cairo = output->current_buffer->cairo;
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);

	if (data->verbose)
		fputs("Drawing bar.\n", stderr);

	/* Draw the bar. */
	switch (data->position)
	{
		case POSITION_BOTTOM:
			cairo_draw_coloured_rectangle(cairo,
					data->bar_colour,
					data->border_width, data->border_width,
					data->w - 2 * data->border_width, data->h - data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->w - data->border_width, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->border_width, 0,
					data->w - 2 * data->border_width, data->border_width);
			cairo_draw_bar_icons(cairo, data);
			break;

		case POSITION_TOP:
			cairo_draw_coloured_rectangle(cairo,
					data->bar_colour,
					data->border_width, 0,
					data->w - 2 * data->border_width, data->h - data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->w - data->border_width, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->border_width, data->h - data->border_width,
					data->w - 2 * data->border_width, data->border_width);
			cairo_draw_bar_icons(cairo, data);
			break;

		case POSITION_LEFT:
			cairo_draw_coloured_rectangle(cairo,
					data->bar_colour,
					0, data->border_width,
					data->w - data->border_width, data->h - 2 * data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, 0,
					data->w, data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, data->h - data->border_width,
					data->w, data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->w - data->border_width, data->border_width,
					data->border_width, data->h - 2 * data->border_width);
			cairo_draw_bar_icons(cairo, data);
			break;

		case POSITION_RIGHT:
			cairo_draw_coloured_rectangle(cairo,
					data->bar_colour,
					data->border_width, data->border_width,
					data->w - data->border_width, data->h - 2 * data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, 0,
					data->w, data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, data->h - data->border_width,
					data->w, data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, data->border_width,
					data->border_width, data->h - 2 * data->border_width);
			cairo_draw_bar_icons(cairo, data);
			break;

		case POSITION_CENTER:
			cairo_draw_coloured_rectangle(cairo,
					data->bar_colour,
					data->border_width, data->border_width,
					data->w - 2 * data->border_width, data->h - 2 * data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					0, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->w - data->border_width, 0,
					data->border_width, data->h);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->border_width, 0,
					data->w - 2 * data->border_width, data->border_width);
			cairo_draw_coloured_rectangle(cairo,
					data->border_colour,
					data->border_width, data->bar_width + data->border_width,
					data->w - 2 * data->border_width, data->border_width);
			cairo_draw_bar_icons(cairo, data);
			break;
	}

	if (data->verbose)
		fputs("Scaling, attaching, damaging and committing surface.\n", stderr);

	wl_surface_set_buffer_scale(output->wl_surface, output->scale);
	wl_surface_attach(output->wl_surface, output->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(output->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->wl_surface);
}
