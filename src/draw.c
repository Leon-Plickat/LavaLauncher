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
#include"output.h"
#include"seat.h"
#include"draw.h"

// TODO Draw icons once on startup to a cairo_surface (and at scale updates)
// TODO Possibly draw icons to a subsurface so only the bar needs redrawing for animations
// TODO Use something faster than cairo for drawing the icons

static void draw_single_icon (cairo_t *cairo, int32_t x, int32_t y,
		int32_t icon_size, cairo_surface_t *img)
{
	float w = cairo_image_surface_get_width(img);
	float h = cairo_image_surface_get_height(img);

	cairo_save(cairo);
	cairo_translate(cairo, x, y);
	cairo_scale(cairo, icon_size / w, icon_size / h);
	cairo_set_source_surface(cairo, img, 0, 0);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

static void draw_icons (cairo_t *cairo, int32_t x, int32_t y, int32_t icon_size,
		enum Bar_orientation orientation, struct wl_list *button_list)
{
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, button_list, link)
	{
		draw_single_icon(cairo, x, y, icon_size, bt_1->img);

		if ( orientation == ORIENTATION_HORIZONTAL )
			x += icon_size;
		else
			y += icon_size;
	}
}

/* Draw the bar background plus border. */
static void draw_bar (cairo_t *cairo, int32_t x, int32_t y, int32_t w, int32_t h,
		int32_t border_top, int32_t border_right,
		int32_t border_bottom, int32_t border_left,
		float scale, float center_colour[4], float border_colour[4])
{
	/* Scale. */
	x *= scale, y *= scale, w *= scale, h *= scale;
	border_top *= scale, border_bottom *= scale;
	border_left *= scale, border_right *= scale;

	/* Calculate dimensions of center. */
	int32_t cx = x + border_left,
		cy = y + border_top,
		cw = w - border_left - border_right,
		ch = h - border_top  - border_bottom;

	/* Draw center. */
	cairo_rectangle(cairo, cx, cy, cw, ch);
	cairo_set_source_rgba(cairo, center_colour[0], center_colour[1],
			center_colour[2], center_colour[3]);
	cairo_fill(cairo);

	/* Draw borders. Top - Right - Bottom - Left */
	cairo_rectangle(cairo, x, y, w, border_top);
	cairo_rectangle(cairo, x + w - border_right, y + border_top, border_right, h - border_top - border_bottom);
	cairo_rectangle(cairo, x, y + h - border_bottom, w, border_bottom);
	cairo_rectangle(cairo, x, y + border_top, border_left, h - border_top - border_bottom);
	cairo_set_source_rgba(cairo, border_colour[0], border_colour[1],
			border_colour[2], border_colour[3]);
	cairo_fill(cairo);
}

static void clear_cairo_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

static void calculate_buffer_size (struct Lava_data *data, struct Lava_output *output,
		int *w, int *h)
{
	if ( data->orientation == ORIENTATION_HORIZONTAL )
	{
		*w = output->w * output->scale;
		*h = data->h   * output->scale;
	}
	else
	{
		*w = data->w   * output->scale;
		*h = output->h * output->scale;
	}
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if (! output->configured)
		return;

	/* Get new/next buffer. */
	int buffer_w, buffer_h;
	calculate_buffer_size(data, output, &buffer_w, &buffer_h);
	if (! next_buffer(&output->current_buffer, data->shm, output->buffers,
				buffer_w, buffer_h))
		return;

	cairo_t *cairo = output->current_buffer->cairo;
	clear_cairo_buffer(cairo);

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
	else
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
