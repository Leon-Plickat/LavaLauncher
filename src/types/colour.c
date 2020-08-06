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

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#include<cairo/cairo.h>

#include"log.h"
#include"types/colour.h"

bool colour_from_string (struct Lava_colour *colour, const char *hex)
{
	if ( colour == NULL || hex == NULL || *hex == '\0' || *hex == ' ' )
		goto error;


	memset(colour, '\0', sizeof(struct Lava_colour));
	strncpy(colour->hex, hex, sizeof(colour->hex) - 1);

	unsigned int r = 0, g = 0, b = 0, a = 255;
	if (! strcmp(hex, "transparent"))
		r = g = b = a = 0;
	else if (! strcmp(hex, "black"))
		r = g = b = 0, a = 255;
	else if (! strcmp(hex, "white"))
		r = g = b = a = 255;
	else if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a)
			&& 3 != sscanf(hex, "#%02x%02x%02x", &r, &g, &b) )
		goto error;

	// TODO parse multiple hex codes per Lava_colour for gradients
	//      -> "#rrggbbaa|#rrggbbaa" horizontal gradient
	//      -> "#rrggbbaa-#rrggbbaa" vertical   gradient

	colour->r = (float)r / 255.0f;
	colour->g = (float)g / 255.0f;
	colour->b = (float)b / 255.0f;
	colour->a = (float)a / 255.0f;

	return true;

error:
	log_message(NULL, 0, "ERROR: \"%s\" is not a valid colour.\n"
			"INFO: Colour codes are expected to be in the "
			"format #RRGGBBAA or #RRGGBB\n", hex);
	return false;
}

void colour_set_cairo_source (cairo_t *cairo, struct Lava_colour *colour)
{
	cairo_set_source_rgba(cairo, colour->r, colour->g, colour->b, colour->a);
}

