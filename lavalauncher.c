/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200112L
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>

#include<wayland-server.h>
#include<wayland-client-protocol.h>
#include<cairo/cairo.h>

#include"wlr-layer-shell-unstable-v1-client-protocol.h"
#include"xdg-output-unstable-v1-client-protocol.h"

static const char usage[] = "LavaLauncher -- Version 0.1-WIP\n"
                            "\n"
                            "Usage: lavalauncher [options...]\n"
                            "\n"
                            "  -b <path>:<command>         Add a button.\n"
                            "  -p <top|bottom|left|right>  Position of the bar.\n"
                            "  -s <size>                   Width of the bar.\n"
                            "  -S <size>                   Width of the border.\n"
                            "  -c <colour>                 Background colour.\n"
                            "  -C <colour>                 Border colour.\n"
                            "  -h                          Display this help text and exit.\n"
                            "\n"
                            "Buttons are displayed in the order in which they are given.\n"
                            "Commands will be executed with sh(1).\n"
                            "Colours are expected to be in the format #RRGGBBAA\n"
                            "Sizes are expected to be in pixels.\n";

enum Bar_position {
	POSITION_TOP = 0,
	POSITION_RIGHT,
	POSITION_BOTTOM,
	POSITION_LEFT
};

struct Lava_data
{
	struct wl_display             *display;
	struct wl_registry            *registry;
	struct wl_compositor          *compositor;
	struct wl_shm                 *shm;
	struct zwlr_layer_shell_v1    *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;
	struct wl_surface             *surface;
	struct zwlr_layer_surface_v1  *layer_surface;

	int32_t scale;

	struct wl_list outputs;
	struct wl_list seats;
	struct wl_list buttons;

	enum Bar_position position;
	int               bar_width;
	int               border_width;
	float             bar_colour[4];
	float             border_colour[4];
};



/* This function separates a string at a given delimiter, by turning the
 * delimiter into '\0'. The pointer to the second new string is returned, while
 * the pointer to the original string is now the pointer to the first new
 * string. If the delimiter is not found or no second string after the delimiter
 * exists, NULL is returned.
 */
char *separate_string_at_delimiter (char *str, char delim)
{
	char *stringrunner = str;

	for (;;)
	{
		if ( *stringrunner == '\0' )
			return NULL;

		if ( *stringrunner == delim )
			break;

		stringrunner++;
	}

	*stringrunner = '\0';
	stringrunner++;

	if ( *stringrunner == '\0' )
		return NULL;

	return stringrunner;
}

void hex_to_rgba (char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
{
	unsigned int r = 0, g = 0, b = 0, a = 0;

	if ( 4 != sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a) )
	{
		fputs("Colour codes are expected to use the format #RRGGBBAA\n", stderr);
		exit(EXIT_FAILURE);
	}

	*c_r = r / 255.0f;
	*c_g = g / 255.0f;
	*c_b = b / 255.0f;
	*c_a = a / 255.0f;
}

static void add_button(struct Lava_data *data, const char *arg)
{
	return;
}

static void set_position(struct Lava_data *data, const char *arg)
{
	return;
}

static void set_bar_size(struct Lava_data *data, const char *arg)
{
	return;
}

static void set_border_size(struct Lava_data *data, const char *arg)
{
	return;
}

static void set_bar_colour(struct Lava_data *data, const char *arg)
{
	return;
}

static void set_border_colour(struct Lava_data *data, const char *arg)
{
	return;
}

static void exec_cmd (const char *cmd)
{
	if (! fork())
	{
		setsid();
		execl("/bin/sh",
				"/bin/sh",
				"-c",
				cmd,
				(char *)NULL);
		exit(0);
	}
}

int main (int argc, char *argv[])
{
	/* Init data struct with sensible defaults */
	struct Lava_data data = {0};
	wl_list_init(&(data.buttons));
	data.position         = POSITION_LEFT;
	data.bar_width        = 80;
	data.border_width     = 2;
	data.bar_colour[0]    = 0.0f;
	data.bar_colour[1]    = 0.0f;
	data.bar_colour[2]    = 0.0f;
	data.bar_colour[3]    = 1.0f;
	data.border_colour[0] = 1.0f;
	data.border_colour[1] = 1.0f;
	data.border_colour[2] = 1.0f;
	data.border_colour[3] = 1.0f;

	for (int c; (c = getopt(argc, argv, "b:p:s:S:c:C:h")) != -1 ;)
		switch (c)
		{
			/* Weirdly formatted for readability. */
			case 'h': fputs             (usage, stderr); return EXIT_SUCCESS;
			case 'b': add_button        (&data, optarg); break;
			case 'p': set_position      (&data, optarg); break;
			case 's': set_bar_size      (&data, optarg); break;
			case 'S': set_border_size   (&data, optarg); break;
			case 'c': set_bar_colour    (&data, optarg); break;
			case 'C': set_border_colour (&data, optarg); break;
		}

	if (! wl_list_length(&(data.buttons)))
	{
		fputs("No buttons defined!\n", stderr);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
