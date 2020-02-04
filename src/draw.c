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

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<cairo/cairo.h>
#include<wayland-server.h>

#include"lavalauncher.h"
#include"draw.h"

/* Draw the icons for all defined buttons. */
// TODO remove conditional in loop
// TODO Draw once on startup to a cairo_surface (and at scale updates)
// TODO Possibly draw icons to a subsurface so only the bar needs redrawing for animations
// TODO Use something faster than cairo for drawing the icons
static void draw_icons (cairo_t *cairo, int32_t x, int32_t y, int32_t icon_size,
		enum Bar_orientation orientation, struct wl_list *button_list)
{
	float w, h;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, button_list, link)
	{
		w = cairo_image_surface_get_width(bt_1->img);
		h = cairo_image_surface_get_height(bt_1->img);

		cairo_save(cairo);
		cairo_translate(cairo, x, y);
		cairo_scale(cairo, icon_size / w, icon_size / h);
		cairo_set_source_surface(cairo, bt_1->img, 0, 0);
		cairo_paint(cairo);
		cairo_restore(cairo);

		if ( orientation == ORIENTATION_HORIZONTAL )
			x += icon_size;
		else
			y += icon_size;
	}
}

/* Draw a coloured rectangle. */
static void draw_coloured_rectangle (cairo_t *cairo, float colour[4],
		int x, int y, int w, int h, float scale)
{
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_rectangle(cairo, x * scale, y * scale, w *scale, h * scale);
	cairo_close_path(cairo);
	cairo_fill(cairo);
}

/* Draw the bar background plus border. */
static void draw_bar (cairo_t *cairo, int32_t x, int32_t y, int32_t w, int32_t h,
		int32_t border_top, int32_t border_right,
		int32_t border_bottom, int32_t border_left,
		float scale, float center_colour[4], float border_colour[4])
{
	/* Calculate dimensions of center. */
	int32_t cx = x + border_left,
		cy = y + border_top,
		cw = w - border_left - border_right,
		ch = h - border_top - border_bottom;

	/* Draw center. */
	draw_coloured_rectangle(cairo, center_colour, cx, cy, cw, ch, scale);

	/* Draw borders. Top - Right - Bottom - Left */
	draw_coloured_rectangle(cairo, border_colour, x, y,
			w, border_top, scale);
	draw_coloured_rectangle(cairo, border_colour, x + w - border_right, y + border_top,
			border_right, h - border_top - border_bottom, scale);
	draw_coloured_rectangle(cairo, border_colour, x, y + h - border_bottom,
			w, border_bottom, scale);
	draw_coloured_rectangle(cairo, border_colour, x, y + border_top,
			border_left, h - border_top - border_bottom, scale);
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if (! output->configured)
		return;

	/* Calculate buffer size. */
	int buffer_w, buffer_h;
	switch (data->orientation)
	{
		case ORIENTATION_HORIZONTAL:
			buffer_w = output->w;
			buffer_h = data->h * output->scale;
			break;

		case ORIENTATION_VERTICAL:
			buffer_w = data->w * output->scale;
			buffer_h = output->h;
			break;
	}

	/* Get new/next buffer. */
	if (! next_buffer(&output->current_buffer, data->shm, output->buffers,
				buffer_w, buffer_h))
		return;

	/* Clear buffer. */
	cairo_t *cairo = output->current_buffer->cairo;
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);

	/* Draw bar. */
	if (data->verbose)
		fputs("Drawing bar.\n", stderr);
	if ( data->mode == MODE_DEFAULT )
	{
		draw_bar(cairo, output->bar_x_offset, output->bar_y_offset,
				data->w, data->h,
				data->border_top, data->border_right,
				data->border_bottom, data->border_left,
				output->scale,
				data->bar_colour, data->border_colour);
	}
	else if ( data->mode == MODE_FULL )
	{
		int bar_w, bar_h;
		if ( data->orientation == ORIENTATION_HORIZONTAL )
		{
			bar_w = output->w;
			bar_h = data->h;
		}
		else
		{
			bar_w = data->w;
			bar_h = output->h;
		}
		draw_bar(cairo, 0, 0, bar_w, bar_h,
				data->border_top, data->border_right,
				data->border_bottom, data->border_left,
				output->scale,
				data->bar_colour, data->border_colour);
	}

	/* Draw icons. */
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);
	draw_icons(cairo, (output->bar_x_offset + data->border_left) * output->scale,
			(output->bar_y_offset + data->border_top) * output->scale,
			data->icon_size * output->scale, data->orientation,
			&data->buttons);

	/* Commit surface. */
	if (data->verbose)
		fputs("Committing surface.\n", stderr);
	wl_surface_set_buffer_scale(output->wl_surface, output->scale);
	wl_surface_attach(output->wl_surface, output->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(output->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->wl_surface);
}
