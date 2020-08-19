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

#ifndef LAVALAUNCHER_TYPES_STRING_CONTAINER_H
#define LAVALAUNCHER_TYPES_STRING_CONTAINER_H

#include<stdint.h>

#if SVG_SUPPORT
#include<librsvg-2.0/librsvg/rsvg.h>
#endif

struct Lava_string_container
{
	char *string;
	uint32_t references, length;
};

struct Lava_string_container *string_container_from (const char *in);
struct Lava_string_container *string_container_reference (struct Lava_string_container *sc);
void string_container_destroy (struct Lava_string_container *sc);

#endif

