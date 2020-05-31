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

struct Lava_data;
struct Lava_bar;

void lldg_circle (cairo_t *cairo, int32_t x, int32_t y, int32_t size);
void lldg_rounded_square (cairo_t *cairo, int32_t x, int32_t y, int32_t size);
void lldg_draw_square_image (cairo_t *cairo, int32_t x, int32_t y,
		int32_t icon_size, cairo_surface_t *img);
void lldg_draw_bordered_rectangle (cairo_t *cairo, int32_t x, int32_t y,
		int32_t w, int32_t h, int32_t border_top, int32_t border_right,
		int32_t border_bottom, int32_t border_left,
		float scale, float center_colour[4], float border_colour[4]);
void lldg_clear_buffer (cairo_t *cairo);

#endif
