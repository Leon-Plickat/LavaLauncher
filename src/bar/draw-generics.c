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

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<cairo/cairo.h>
#include<wayland-server.h>

#include"lavalauncher.h"
#include"output.h"
#include"seat.h"
#include"draw-generics.h"

void lldg_rounded_square (cairo_t *cairo, int32_t x, int32_t y, int32_t size)
{
	double radius = size / 10.0, degrees = 3.1415927 / 180.0;

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
}

void lldg_draw_square_image (cairo_t *cairo, int32_t x, int32_t y,
		int32_t icon_size, cairo_surface_t *img)
{
	if ( img == NULL )
		return;

	float w = cairo_image_surface_get_width(img);
	float h = cairo_image_surface_get_height(img);

	cairo_save(cairo);
	cairo_translate(cairo, x, y);
	cairo_scale(cairo, icon_size / w, icon_size / h);
	cairo_set_source_surface(cairo, img, 0, 0);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

/* Draw a rectangle with borders. */
void lldg_draw_bordered_rectangle (cairo_t *cairo, int32_t x, int32_t y,
		int32_t w, int32_t h, int32_t border_top, int32_t border_right,
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
	cairo_rectangle(cairo, x + w - border_right, y + border_top,
			border_right, h - border_top - border_bottom);
	cairo_rectangle(cairo, x, y + h - border_bottom, w, border_bottom);
	cairo_rectangle(cairo, x, y + border_top, border_left,
			h - border_top - border_bottom);
	cairo_set_source_rgba(cairo, border_colour[0], border_colour[1],
			border_colour[2], border_colour[3]);
	cairo_fill(cairo);
}

void lldg_clear_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

