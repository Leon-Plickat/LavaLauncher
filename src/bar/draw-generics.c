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
#include"types/colour.h"

void circle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t size)
{
	cairo_arc(cairo, x + (size/2), y + (size/2), size / 2, 0, 2 * 3.1415927);
}

void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y,
		uint32_t width, uint32_t height,
		double tl_r, double tr_r, double bl_r, double br_r)
{
	double degrees = 3.1415927 / 180.0;

	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + width - tr_r,          y + tr_r, tr_r, -90 * degrees,   0 * degrees);
	cairo_arc(cairo, x + width - br_r, y + height - br_r, br_r,   0 * degrees,  90 * degrees);
	cairo_arc(cairo, x +         bl_r, y + height - bl_r, bl_r,  90 * degrees, 180 * degrees);
	cairo_arc(cairo, x +         tl_r, y          + tl_r, tl_r, 180 * degrees, 270 * degrees);
	cairo_close_path(cairo);
}

/* Draw a rectangle with configurable borders and corners. */
void draw_bar_background (cairo_t *cairo,
		uint32_t x, uint32_t y, uint32_t w, uint32_t h,
		uint32_t border_top, uint32_t border_right,
		uint32_t border_bottom, uint32_t border_left,
		uint32_t top_left_radius, uint32_t top_right_radius,
		uint32_t bottom_left_radius, uint32_t bottom_right_radius,
		uint32_t scale, struct Lava_colour *bar_colour,
		struct Lava_colour *border_colour)
{
	/* Scale. */
	x *= scale, y *= scale, w *= scale, h *= scale;
	border_top *= scale, border_bottom *= scale;
	border_left *= scale, border_right *= scale;
	top_left_radius *= scale, top_right_radius *= scale;
	bottom_left_radius *= scale, bottom_right_radius *= scale;

	/* Calculate dimensions of center. */
	uint32_t cx = x + border_left,
		 cy = y + border_top,
		 cw = w - (border_left + border_right),
		 ch = h - (border_top + border_bottom);


	/* Avoid radii so big they cause unexpected drawing behaviour. */
	uint32_t smallest_side = cw < ch ? cw : ch;
	if ( top_left_radius > smallest_side / 2 )
		top_left_radius = smallest_side / 2;
	if ( top_right_radius > smallest_side / 2 )
		top_right_radius = smallest_side / 2;
	if ( bottom_left_radius > smallest_side / 2 )
		bottom_left_radius = smallest_side / 2;
	if ( bottom_right_radius > smallest_side / 2 )
		bottom_right_radius = smallest_side / 2;

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

	if ( top_left_radius == 0 && top_right_radius == 0
			&& bottom_left_radius == 0 && bottom_right_radius == 0 )
	{
		if ( border_top == 0 && border_bottom == 0
				&& border_left == 0 && border_right == 0 )
		{
			cairo_rectangle(cairo, x, y, w, h);
			colour_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			/* Borders. */
			cairo_rectangle(cairo, x, y, w, border_top);
			cairo_rectangle(cairo, x + w - border_right, y + border_top,
					border_right, h - border_top - border_bottom);
			cairo_rectangle(cairo, x, y + h - border_bottom, w, border_bottom);
			cairo_rectangle(cairo, x, y + border_top, border_left,
					h - border_top - border_bottom);
			colour_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			/* Center. */
			cairo_rectangle(cairo, cx, cy, cw, ch);
			colour_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}
	else
	{
		if ( border_top == 0 && border_bottom == 0
				&& border_left == 0 && border_right == 0 )
		{
			rounded_rectangle(cairo, x, y, w, h,
					top_left_radius, top_right_radius,
					bottom_left_radius, bottom_right_radius);
			colour_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			rounded_rectangle(cairo, x, y, w, h,
					top_left_radius, top_right_radius,
					bottom_left_radius, bottom_right_radius);
			colour_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			rounded_rectangle(cairo, cx, cy, cw, ch,
					top_left_radius, top_right_radius,
					bottom_left_radius, bottom_right_radius);
			colour_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}
	cairo_restore(cairo);
}

void clear_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

