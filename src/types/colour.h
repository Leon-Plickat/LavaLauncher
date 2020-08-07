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

#ifndef LAVALAUNCHER_TYPES_COLOUR_H
#define LAVALAUNCHER_TYPES_COLOUR_H

#include<stdbool.h>
#include<cairo/cairo.h>

struct Lava_colour
{
	double r, g, b, a;
};

bool colour_from_string (struct Lava_colour *colour, const char *hex);
void colour_set_cairo_source (cairo_t *cairo, struct Lava_colour *colour);

#endif

