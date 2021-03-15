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

/* Ref-counted image type, combining a cairo surface and an rsvg handle. Once
 * bar copies are removed in favour of a better conditional configuration method,
 * the ref count can be removed.
 */

#ifndef LAVALAUNCHER_TYPES_IMAGE_H
#define LAVALAUNCHER_TYPES_IMAGE_H

#include<stdint.h>
#include<cairo/cairo.h>

#if SVG_SUPPORT
#include<librsvg-2.0/librsvg/rsvg.h>
#endif

typedef struct
{
	cairo_surface_t *cairo_surface;

#if SVG_SUPPORT
	RsvgHandle *rsvg_handle;
#endif

	int references;
} image_t;

image_t *image_t_create_from_file (const char *path);
image_t *image_t_reference (image_t *image);
void image_t_destroy (image_t *image);
void image_t_draw_to_cairo (cairo_t *cairo, image_t *image,
		uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t scale);

#endif

