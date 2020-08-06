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

#ifndef LAVALAUNCHER_TYPES_IMAGE_H
#define LAVALAUNCHER_TYPES_IMAGE_H

#include<cairo/cairo.h>

#if SVG_SUPPORT
#include<librsvg-2.0/librsvg/rsvg.h>
#endif

struct Lava_image
{
	cairo_surface_t *cairo_surface;

#if SVG_SUPPORT
	RsvgHandle *rsvg_handle;
#endif

	int references;
};

struct Lava_image *image_create_from_file (const char *path);
struct Lava_image *image_reference (struct Lava_image *image);
void image_destroy (struct Lava_image *image);
void image_draw_to_cairo (cairo_t *cairo, struct Lava_image *image,
		uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#endif

