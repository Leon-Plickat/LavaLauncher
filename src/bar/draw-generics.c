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
#include"types/colour_t.h"
#include"types/box_t.h"

void circle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t size)
{
	cairo_arc(cairo, x + (size/2.0), y + (size/2.0), size / 2.0, 0, 2 * 3.1415927);
}

void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uradii_t *radii)
{
	double degrees = 3.1415927 / 180.0;
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + w - radii->top_right,    y     + radii->top_right,    radii->top_right,   -90 * degrees,   0 * degrees);
	cairo_arc(cairo, x + w - radii->bottom_right, y + h - radii->bottom_right, radii->bottom_right,  0 * degrees,  90 * degrees);
	cairo_arc(cairo, x     + radii->bottom_left,  y + h - radii->bottom_left,  radii->bottom_left,  90 * degrees, 180 * degrees);
	cairo_arc(cairo, x     + radii->top_left,     y     + radii->top_left,     radii->top_left,    180 * degrees, 270 * degrees);
	cairo_close_path(cairo);
}

/* Draw a rectangle with configurable borders and corners. */
void draw_bar_background (cairo_t *cairo, ubox_t *_dim, udirections_t *_border, uradii_t *_radii,
		uint32_t scale, colour_t *bar_colour, colour_t *border_colour)
{
	ubox_t        dim    = ubox_t_scale(_dim, scale);
	udirections_t border = udirections_t_scale(_border, scale);
	uradii_t      radii  = uradii_t_scale(_radii, scale);

	ubox_t center = {
		.x = dim.x + border.left,
		.y = dim.y + border.top,
		.w = dim.w - (border.left + border.right),
		.h = dim.h - (border.top + border.bottom)
	};

	/* Avoid radii so big they cause unexpected drawing behaviour. */
	uint32_t smallest_side = center.w < center.h ? center.w : center.h;
	if ( radii.top_left > smallest_side / 2 )
		radii.top_left = smallest_side / 2;
	if ( radii.top_right > smallest_side / 2 )
		radii.top_right = smallest_side / 2;
	if ( radii.bottom_left > smallest_side / 2 )
		radii.bottom_left = smallest_side / 2;
	if ( radii.bottom_right > smallest_side / 2 )
		radii.bottom_right = smallest_side / 2;

	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);

	if ( radii.top_left == 0 && radii.top_right == 0 && radii.bottom_left == 0 && radii.bottom_right == 0 )
	{
		if ( border.top == 0 && border.bottom == 0 && border.left == 0 && border.right == 0 )
		{
			cairo_rectangle(cairo, dim.x, dim.y, dim.w, dim.h);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			/* Borders. */
			cairo_rectangle(cairo, dim.x, dim.y, dim.w, border.top);
			cairo_rectangle(cairo, dim.x + dim.w - border.right, dim.y + border.top,
					border.right, dim.h - border.top - border.bottom);
			cairo_rectangle(cairo, dim.x, dim.y + dim.h - border.bottom, dim.w, border.bottom);
			cairo_rectangle(cairo, dim.x, dim.y + border.top, border.left,
					dim.h - border.top - border.bottom);
			colour_t_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			/* Center. */
			cairo_rectangle(cairo, center.x, center.y, center.w, center.h);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
	}
	else
	{
		if ( border.top == 0 && border.bottom == 0 && border.left == 0 && border.right == 0 )
		{
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii);
			colour_t_set_cairo_source(cairo, bar_colour);
			cairo_fill(cairo);
		}
		else
		{
			rounded_rectangle(cairo, dim.x, dim.y, dim.w, dim.h, &radii);
			colour_t_set_cairo_source(cairo, border_colour);
			cairo_fill(cairo);

			rounded_rectangle(cairo, center.x, center.y, center.w, center.h, &radii);
			colour_t_set_cairo_source(cairo, bar_colour);
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

