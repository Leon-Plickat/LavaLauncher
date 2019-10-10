/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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
#include<poll.h>
#include<errno.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>
//#include<cairo/cairo.h>

#include"wlr-layer-shell-unstable-v1-client-protocol.h"
#include"xdg-shell-client-protocol.h"

#define SHELL "/bin/sh"

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

enum Bar_position
{
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
	//struct zxdg_output_manager_v1 *xdg_output_manager; // TODO ??? XXX
	struct wl_surface             *wl_surface;
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

	bool loop;
};

struct Lava_output
{
	struct wl_list           link;
	struct Lava_data        *data;
	uint32_t                 global_name;
	struct wl_output        *wl_output;
//	struct zxdg_output_v1   *xdg_output;
	char                    *name;
	enum wl_output_subpixel  subpixel;
	int32_t                  scale;
};

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;
	struct wl_seat   *wl_seat;

	struct
	{
		struct wl_pointer *wl_pointer;
		int32_t            x;
		int32_t            y;
	} pointer;
};

struct Lava_button
{
	struct wl_list link;

	const char *img_path;
	const char *cmd;
};



/* No-Op function. */
static void noop () {}

// TODO
static void pointer_handle_motion () {}

// TODO
static void pointer_handle_button () {}

static const struct wl_pointer_listener pointer_listener = {
	.enter  = noop,
	.leave  = noop,
	.motion = pointer_handle_motion, //TODO: maybe noop
	.button = pointer_handle_button,
	.axis   = noop
};

static void seat_handle_capabilities (void *data, struct wl_seat *wl_seat, uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)data;

	if ( seat->pointer.wl_pointer != NULL )
	{
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}

	if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
	{
		seat->pointer.wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer.wl_pointer,
				&pointer_listener,
				seat);
	}
	else
		fputs("Compositor seat does not have pointer capabilities.\n"
				"You will not be able to click on icons.\n"
				"Continuing anyway.\n",
				stderr);
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name         = noop
};

static void output_handle_geometry (void *data, struct wl_output *wl_output, int32_t x, int32_t y, int32_t phy_width, int32_t phy_height, int32_t subpixel, const char *make, const char *model, int32_t transform)
{
	struct Lava_output *output = (struct Lava_output *)data;
	output->subpixel           = subpixel;
}

static void output_handle_scale (void *data, struct wl_output *wl_output, int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)data;
	output->scale              = (int)factor;
}

static const struct wl_output_listener output_listener = {
	.geometry = output_handle_geometry,
	.mode     = noop,
	.done     = noop,
	.scale    = output_handle_scale
};

static void registry_handle_global (void *raw_data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;

	if (! strcmp(interface, wl_compositor_interface.name))
		data->compositor = wl_registry_bind(registry,
				name,
				&wl_compositor_interface,
				4);
	else if (! strcmp(interface, wl_shm_interface.name))
		data->shm = wl_registry_bind(registry,
				name,
				&wl_shm_interface,
				1);
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
		data->layer_shell = wl_registry_bind(registry,
				name,
				&wl_shm_interface,
				1);
	else if (! strcmp(interface, wl_seat_interface.name))
	{
		struct wl_seat *wl_seat = wl_registry_bind(registry,
				name,
				&wl_seat_interface,
				3);
		struct Lava_seat *seat = calloc(1, sizeof(struct Lava_seat));
		if ( seat == NULL )
		{
			fputs("Could not allocate.\n", stderr);
			exit(EXIT_FAILURE);
		}
		seat->data    = data;
		seat->wl_seat = wl_seat;
		wl_list_insert(&data->seats, &seat->link);
		wl_seat_add_listener(wl_seat, &seat_listener, seat);
	}
	else if (! strcmp(interface, wl_output_interface.name))
	{
		struct wl_output *wl_output = wl_registry_bind(registry,
				name,
				&wl_output_interface,
				3);
		struct Lava_output *output = calloc(1, sizeof(struct Lava_output));
		if ( output == NULL )
		{
			fputs("Could not allocate.\n", stderr);
			exit(EXIT_FAILURE);
		}
		output->data        = data;
		output->global_name = name;
		output->wl_output   = wl_output;
		output->scale       = 1; // TODO ?
		wl_list_insert(&data->outputs, &output->link);
		wl_output_set_user_data(wl_output, output);
		wl_output_add_listener(wl_output, &output_listener, output);
		// get_xdg_output(output); // TODO what does this do?
		// TODO: create bar surface
	}
	// TODO maybe test for zxdg_output_manager_v1_interface ?
}

static void registry_handle_global_remove (void *raw_data, struct wl_registry *registry, uint32_t name)
{
	struct Lava_data   *data = (struct Lava_data *)raw_data;
	struct Lava_output *op1;
	struct Lava_output *op2;
	wl_list_for_each_safe(op1, op2, &data->outputs, link)
	{
		if ( op1->global_name == name )
		{
			//if ( op1->data->surface_output == op1 )
			//	op1->data->surface_output == NULL;
			//if ( op1->data->layer_surface_output == op1 )
			//	op1->data->layer_surface_output == NULL;
			wl_list_remove(&op1->link);
			//if ( op1->xdg_output != NULL )
			//	zxdg_output_v1_destroy(op1->xdg_output);
			wl_output_destroy(op1->wl_output);
			free(op1->name);
			free(op1);
			break;
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

static void init_wayland (struct Lava_data *data)
{
	/* Init lists. */
	wl_list_init(&data->outputs);
	wl_list_init(&data->seats);

	/* Connect to Wayland server. */
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	data->display = wl_display_connect(wayland_display);
	assert(data->display);
	if ( data->display == NULL || wayland_display == NULL )
	{
		fputs("Can not connect to a Wayland server.\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Get registry and add listeners. */
	data->registry = wl_display_get_registry(data->display);
	if ( data->registry == NULL )
	{
		fputs("Can not get registry.\n", stderr);
		exit(EXIT_FAILURE);
	}
	wl_registry_add_listener(data->registry,
			&registry_listener,
			data);

	if ( wl_display_roundtrip(data->display) == -1 )
	{
		fputs("Roundtrip failed.\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Testing compatibilities. */
	if ( data->compositor == NULL )
	{
		fputs("Wayland compositor does not support wl_compositor.\n", stderr);
		exit(EXIT_FAILURE);
	}
	if ( data->shm == NULL )
	{
		fputs("Wayland compositor does not support wl_shm.\n", stderr);
		exit(EXIT_FAILURE);
	}
	if ( data->layer_shell == NULL )
	{
		fputs("Wayland compositor does not support zwlr_layer_shell_v1.\n", stderr);
		exit(EXIT_FAILURE);
	}

	// TODO: Not needed? We don't want to configure outputs...
	//if ( data->xdg_output_manager != NULL )
	//{
	//	struct Lava_output *op;
	//	wl_list_for_each(op, &data->outputs, link)
	//	{
	//		get_xdg_output(op);
	//	}
	//	if ( wl_display_roundtrip(data->display) == -1 )
	//	{
	//		fputs("Roundtrip failed.\n", stderr);
	//		exit(EXIT_FAILURE);
	//	}
	//}
}

static void deinit_wayland (struct Lava_data *data)
{
	if ( data->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(data->layer_surface);
	if ( data->wl_surface != NULL )
		wl_surface_destroy(data->wl_surface);

	struct Lava_output *op_1;
	struct Lava_output *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
	{
		wl_list_remove(&op_1->link);
		wl_output_destroy(op_1->wl_output);
		free(op_1->name);
		free(op_1);
	}

	struct Lava_seat *st_1;
	struct Lava_seat *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
	{
		wl_list_remove(&st_1->link);
		wl_seat_release(st_1->wl_seat);
		if (st_1->pointer.wl_pointer)
			wl_pointer_release(st_1->pointer.wl_pointer);
		free(st_1);
	}

	zwlr_layer_shell_v1_destroy(data->layer_shell);
	wl_compositor_destroy(data->compositor);
	wl_shm_destroy(data->shm);
	wl_registry_destroy(data->registry);
	wl_display_disconnect(data->display);
}

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

/* This function turns a colour string of the format #RRGGBBAA to usable RGBA
 * values.
 */
void hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a)
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

/* This function executes the given command with SHELL. */
static void exec_cmd (const char *cmd)
{
	if (! fork())
	{
		setsid();
		execl(SHELL, SHELL, "-c", cmd, (char *)NULL);
		exit(EXIT_SUCCESS);
	}
}

/* This function adds a button struct to the button list. */
static void config_add_button(struct Lava_data *data, char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad button configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	char *cmd = separate_string_at_delimiter(arg, ':');
	if ( cmd == NULL )
	{
		fputs("Bad button configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* *arg is now the image path and *cmd the shell command. */

	struct Lava_button *new_button = calloc(1, sizeof(struct Lava_button));
	new_button->img_path = arg;
	new_button->cmd      = cmd;

	wl_list_insert(&data->buttons, &new_button->link);
}

static void config_set_position(struct Lava_data *data, const char *arg)
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

static void config_set_bar_size(struct Lava_data *data, const char *arg)
{
	data->bar_width = atoi(arg);

	if ( data->bar_width <= 0 )
	{
		fputs("Bar width must be greater than zero.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

static void config_set_border_size(struct Lava_data *data, const char *arg)
{
	data->border_width = atoi(arg);

	if ( data->border_width < 0 )
	{
		fputs("Border width must be equal to or greater than zero.\n", stderr);
		exit(EXIT_FAILURE);
	}
}

static void config_set_bar_colour(struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg,
			&(data->bar_colour[0]),
			&(data->bar_colour[1]),
			&(data->bar_colour[2]),
			&(data->bar_colour[3]));
}

static void config_set_border_colour(struct Lava_data *data, const char *arg)
{
	if ( arg == NULL || *arg == '\0' || *arg == ' ' )
	{
		fputs("Bad colour configuration.\n", stderr);
		exit(EXIT_FAILURE);
	}

	hex_to_rgba(arg,
			&(data->border_colour[0]),
			&(data->border_colour[1]),
			&(data->border_colour[2]),
			&(data->border_colour[3]));
}

static void main_loop (struct Lava_data *data)
{
	struct pollfd fds = (struct pollfd) {
		.fd     = wl_display_get_fd(data->display),
		.events = POLLIN
	};

	for (data->loop = true; data->loop;)
	{
		errno = 0;

		/* Flush Wayland events. */
		do
		{
			if ( wl_display_flush(data->display) == -1  && errno != EAGAIN )
			{
				fprintf(stderr, "wl_display_flush: %s\n", strerror(errno));
				break;
			}
		} while ( errno == EAGAIN );

		/* Poll, aka wait for event. */
		if ( poll(&fds, 1, -1) < 0 )
		{
			fprintf(stderr, "poll: %s\n", strerror(errno));
			break;
		}

		/* Handle event. */
		if ( fds.revents & POLLIN && wl_display_dispatch(data->display) == -1 )
		{
			fprintf(stderr, "wl_display_dispatch: %s\n", strerror(errno));
			break;
		}
		if ( fds.revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			fprintf(stderr, "wl_display_flush: %s\n", strerror(errno));
			break;
		}
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
			case 'h': fputs                    (usage, stderr); return EXIT_SUCCESS;
			case 'b': config_add_button        (&data, optarg); break;
			case 'p': config_set_position      (&data, optarg); break;
			case 's': config_set_bar_size      (&data, optarg); break;
			case 'S': config_set_border_size   (&data, optarg); break;
			case 'c': config_set_bar_colour    (&data, optarg); break;
			case 'C': config_set_border_colour (&data, optarg); break;
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
	 * If they do not fit, we simply exit. That makes it unnecessary to
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

	init_wayland(&data);
	main_loop(&data);
	deinit_wayland(&data);

	return EXIT_SUCCESS;
}
