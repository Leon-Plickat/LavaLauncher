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

static void clear_cairo_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if (! output->configured)
		return;

	/* Get new/next buffer. */
	if (! next_buffer(&output->current_buffer, data->shm, output->buffers,
				data->w * output->scale, data->h * output->scale))
		return;

	cairo_t *cairo = output->current_buffer->cairo;
	clear_cairo_buffer(cairo);

	/* Draw bar. */
	if (data->verbose)
		fputs("Drawing bar.\n", stderr);
	draw_bar(cairo, 0, 0, data->w, data->h,
			data->border_top, data->border_right,
			data->border_bottom, data->border_left,
			output->scale,
			data->bar_colour, data->border_colour);

	/* Draw icons. */
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);
	draw_icons(cairo, data->border_left * output->scale, data->border_top * output->scale,
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
