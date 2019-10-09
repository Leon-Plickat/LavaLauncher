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
#include<string.h>
#include<assert.h>

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

	int button_amount;

	enum Bar_position position;
	int               bar_width;
	int               border_width;
	float             bar_colour[4];
	float             border_colour[4];

	int w;
	int h;
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
	if (! strcmp(arg, "top"))
		data->position = POSITION_TOP;
	else if (! strcmp(arg, "right"))
		data->position = POSITION_RIGHT;
	else if (! strcmp(arg, "bottom"))
		data->position = POSITION_BOTTOM;
	else if (! strcmp(arg, "left"))
		data->position = POSITION_LEFT;
	else
	{
		fputs("Possible positions are 'top', 'right', 'bottom' and 'left'.\n", stderr);
		exit(EXIT_FAILURE);
	}
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

static void sensible_defaults (struct Lava_data *data)
{
	data->position         = POSITION_LEFT;
	data->bar_width        = 80;
	data->border_width     = 2;
	data->bar_colour[0]    = 0.0f;
	data->bar_colour[1]    = 0.0f;
	data->bar_colour[2]    = 0.0f;
	data->bar_colour[3]    = 1.0f;
	data->border_colour[0] = 1.0f;
	data->border_colour[1] = 1.0f;
	data->border_colour[2] = 1.0f;
	data->border_colour[3] = 1.0f;
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {0};
	wl_list_init(&(data.buttons));

	/* Init data struct with sensible defaults. */
	sensible_defaults(&data);

	/* Handle command flags. */
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

	/* Count buttons. If none are defined, exit. */
	data.button_amount = wl_list_length(&(data.buttons));
	if (! data.button_amount)
	{
		fputs("No buttons defined!\n", stderr);
		return EXIT_FAILURE;
	}

	/* Calculating the size of the bar. At the "docking" edge no border will
	 * be drawn. Later, when outputs are added and the surface(s) are drawn,
	 * we will check whether the dimensions actually fit into the output(s).
	 * If they do not fit, we simply exit. That makes it unneccessary to
	 * implement complicated resizing functions; If the bar is to large, it
	 * simply is the users fault.
	 */
	if ( data.position == POSITION_LEFT || data.position == POSITION_RIGHT )
	{
		data.w = data.bar_width + data.border_width;
		data.h = (data.button_amount * data.bar_width) + (2 * data.border_width);
	}
	else if ( data.position == POSITION_TOP || data.position == POSITION_BOTTOM )
	{
		data.w = (data.button_amount * data.bar_width) + (2 * data.border_width);
		data.h = data.bar_width + data.border_width;
	}
	else /* Unexpeted error */
		return EXIT_FAILURE;

	/* Connect to Wayland server. */
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	data.display = wl_display_connect(wayland_display);
	assert(data.display);
	if ( data.display == NULL || wayland_display == NULL )
	{
		fputs("Can not connect to a Wayland server.\n", stderr);
		return EXIT_FAILURE;
	}

	/* TODO: Main Loop */
	//for (;;) { }

	/* Sever connection to Wayland server. */
	wl_display_terminate(data.display);

	return EXIT_SUCCESS;
}
