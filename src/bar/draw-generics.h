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

#ifndef LAVALAUNCHER_BAR_DRAW_GENERICS_H
#define LAVALAUNCHER_BAR_DRAW_GENERICS_H

#include<stdint.h>
#include<cairo/cairo.h>

#include"types/box_t.h"
#include"types/colour_t.h"

struct Lava_data;
struct Lava_bar;

/* A set functions for generic drawing operations. These are almost pure;
 * The only side effect being modifications to the cairo context.
 */

void circle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t size);
void rounded_rectangle (cairo_t *cairo, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uradii_t *radii);
void draw_bar_background (cairo_t *cairo, ubox_t *_dim, udirections_t *_border, uradii_t *_radii,
		uint32_t scale, colour_t *bar_colour, colour_t *border_colour);
void clear_buffer (cairo_t *cairo);

#endif

