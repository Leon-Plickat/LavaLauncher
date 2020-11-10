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
#include<stdint.h>
#include<stdbool.h>
#include<string.h>
#include<cairo/cairo.h>

#include"str.h"
#include"types/colour.h"

static bool colour_from_hex_string (struct Lava_colour *colour, const char *hex)
{
	unsigned int r = 0, g = 0, b = 0, a = 255;
	if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a)
			&& 3 != sscanf(hex, "#%02x%02x%02x", &r, &g, &b)
			&& 4 != sscanf(hex, "0x%02x%02x%02x%02x", &r, &g, &b, &a)
			&& 3 != sscanf(hex, "0x%02x%02x%02x", &r, &g, &b) )
		return false;

	colour->r = (float)r / 255.0f;
	colour->g = (float)g / 255.0f;
	colour->b = (float)b / 255.0f;
	colour->a = (float)a / 255.0f;

	return true;
}

static bool colour_from_rgb_string (struct Lava_colour *colour, const char *str)
{
	int32_t r = 0, g = 0, b = 0, a = 255;
	if ( 4 != sscanf(str, "rgba(%d,%d,%d,%d)", &r, &g, &b, &a)
			&& 3 != sscanf(str, "rgb(%d,%d,%d)", &r, &g, &b) )
		return false;

	if ( r > 255 || g > 255 || b > 255 || a > 255 )
		return false;
	if ( r < 0 || g < 0 || b < 0 || a < 0 )
		return false;

	colour->r = (float)r / 255.0f;
	colour->g = (float)g / 255.0f;
	colour->b = (float)b / 255.0f;
	colour->a = (float)a / 255.0f;

	return true;
}

bool colour_from_string (struct Lava_colour *colour, const char *str)
{
	if ( colour == NULL || str == NULL || *str == '\0' )
		goto error;

	// TODO maybe parse multiple hex codes per Lava_colour for gradients?
	//      -> "#rrggbbaa-#rrggbbaa" horizontal gradient
	//      -> "#rrggbbaa|#rrggbbaa" vertical   gradient

	if ( *str == '#' || strstr(str, "0x") == str )
	{
		if (! colour_from_hex_string(colour, str))
			goto error;
	}
	else if ( strstr(str, "rgb") == str )
	{
		if (! colour_from_rgb_string(colour, str))
			goto error;
	}
	else
		goto error;

	return true;

error:
	log_message(NULL, 0, "ERROR: \"%s\" is not a valid colour.\n"
			"INFO: Read lavalauncher(1) to find out what colour formats are supported.\n", str);
	return false;
}

void colour_set_cairo_source (cairo_t *cairo, struct Lava_colour *colour)
{
	cairo_set_source_rgba(cairo, colour->r, colour->g, colour->b, colour->a);
}

