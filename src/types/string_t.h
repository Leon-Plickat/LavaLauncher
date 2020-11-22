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

/* string_t is a simple ref-counted string type. Its only use case is reducing
 * the memory size of bar copies. Once bar copies are eventually removed in
 * favour of a better conditional configuration method, this type can be removed
 * as well.
 */

#ifndef LAVALAUNCHER_TYPES_STRING_T_H
#define LAVALAUNCHER_TYPES_STRING_T_H

#include<stdint.h>

#if SVG_SUPPORT
#include<librsvg-2.0/librsvg/rsvg.h>
#endif

typedef struct
{
	char *string;
	uint32_t references;
} string_t;

char *string_t_get_string_or_else (string_t *str, char *or_else);
string_t *string_t_from (const char *in);
string_t *string_t_reference (string_t *str);
void string_t_destroy (string_t *str);

#endif

