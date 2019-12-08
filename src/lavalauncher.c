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
#include<cairo/cairo.h>

#include"lavalauncher.h"
#include"config.h"
#include"draw.h"

#include"wlr-layer-shell-unstable-v1-client-protocol.h"
#include"pool-buffer.h"
#include"xdg-shell-client-protocol.h"

#define SHELL "/bin/sh"

static const char usage[] = "LavaLauncher -- Version 1.0\n\n"
                            "Usage: lavalauncher [options...]\n\n"
                            "  -a                                 Aggressive anchoring.\n"
                            "  -b <path> <command>                Add a button.\n"
                            "  -c <colour>                        Background colour.\n"
                            "  -C <colour>                        Border colour.\n"
                            "  -h                                 Display this help text and exit.\n"
                            "  -l <overlay|top|bottom|background> Layer of the bar surface.\n"
                            "  -p <top|bottom|left|right|center>  Position of the bar.\n"
                            "  -s <size>                          Width of the bar.\n"
                            "  -S <size>                          Width of the border.\n"
                            "  -v                                 Verbose output.\n\n"
                            "Buttons are displayed in the order in which they are declared.\n"
                            "Commands will be executed with sh(1).\n"
                            "Colours are expected to be in the format #RRGGBBAA.\n"
                            "Sizes are expected to be in pixels.\n\n"
                            "You can send bug reports, contributions and user feedback to the mailinglist:\n"
                            "<~leon_plickat/lavalauncher@lists.sr.ht>\n";

/* No-Op function. */
static void noop () {}

static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface,
		uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;

	if (data->verbose)
		fprintf(stderr, "Layer surface configure request"
				" w=%d h=%d serial=%d\n", w, h, serial);

	if ( w == data->w && h == data->h )
	{
		if (data->verbose)
			fputs("Requested size equals needed size.\n", stderr);
		zwlr_layer_surface_v1_ack_configure(surface, serial);
	}
	else if (data->verbose)
	{
		fputs("Requested size does not equal needed size, ignoring.\n",
				stderr);
	}

	render_bar_frame(data, output);
}

static void layer_surface_handle_closed (void *raw_data,
		struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;
	if (data->verbose)
		fputs("Layer surface has been closed.\n", stderr);
	data->loop = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

static void create_bar (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	output->wl_surface = wl_compositor_create_surface(data->compositor);
	if ( output->wl_surface == NULL )
	{
		fputs("Compositor did not create wl_surface.\n", stderr);
		exit(EXIT_FAILURE);
	}

	output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			data->layer_shell,
			output->wl_surface,
			output->wl_output,
			data->layer,
			"LavaLauncher");
	if ( output->layer_surface == NULL )
	{
		fputs("Compositor did not create layer_surface.\n", stderr);
		exit(EXIT_FAILURE);
	}

	zwlr_layer_surface_v1_set_size(output->layer_surface, data->w, data->h);

	switch (data->position)
	{
		case POSITION_TOP:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->aggressive_anchor
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->h);
			break;

		case POSITION_RIGHT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->aggressive_anchor
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->w);
			break;

		case POSITION_BOTTOM:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->aggressive_anchor
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->h);
			break;

		case POSITION_LEFT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->aggressive_anchor
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->w);
			break;

		case POSITION_CENTER:
			break;
	}

	zwlr_layer_surface_v1_add_listener(output->layer_surface,
			&layer_surface_listener,
			output);

	wl_surface_commit(output->wl_surface);
}

static void exec_cmd (const char *cmd)
{
	if (! fork())
	{
		setsid();
		execl(SHELL, SHELL, "-c", cmd, (char *)NULL);
		exit(EXIT_SUCCESS);
	}
}

static struct Lava_button *button_from_ordinate (struct Lava_data *data, int ordinate)
{
	int i = data->border_width + data->bar_width;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		if ( ordinate < i )
			return bt_1;
		i += data->bar_width;
	}
	return NULL;
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct Lava_seat *seat = data;
	seat->pointer.x = wl_fixed_to_int(surface_x);
	seat->pointer.y = wl_fixed_to_int(surface_y);
}

static void pointer_handle_button (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button,
		uint32_t button_state)
{
	struct Lava_seat *seat = raw_data;

	if ( button_state != WL_POINTER_BUTTON_STATE_PRESSED )
		return;

	if (seat->data->verbose)
		fprintf(stderr, "Click! x=%d y=%d",
				seat->pointer.x, seat->pointer.y);

	struct Lava_button *bar_button;

	switch (seat->data->position)
	{
		case POSITION_TOP:
		case POSITION_BOTTOM:
		case POSITION_CENTER:
			bar_button = button_from_ordinate(seat->data, seat->pointer.x);
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			bar_button = button_from_ordinate(seat->data, seat->pointer.y);
			break;
	}

	if ( bar_button == NULL )
		return;

	if (seat->data->verbose)
		fprintf(stderr, " cmd=%s\n", bar_button->cmd);

	if (! strcmp(bar_button->cmd, "exit"))
		seat->data->loop = false;
	else
		exec_cmd(bar_button->cmd);
}

static const struct wl_pointer_listener pointer_listener = {
	.enter  = noop,
	.leave  = noop,
	.motion = pointer_handle_motion,
	.button = pointer_handle_button,
	.axis   = noop
};

static void seat_handle_capabilities (void *raw_data,
		struct wl_seat *wl_seat,
		uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_data *data = seat->data;

	if (data->verbose)
		fputs("Handling seat capabilities.\n", stderr);

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
		if (data->verbose)
			fputs("Seat has WL_SEAT_CAPABILITY_POINTER.\n", stderr);
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

static void output_handle_geometry (void *raw_data,
		struct wl_output *wl_output,
		int32_t x, int32_t y,
		int32_t phy_width, int32_t phy_height,
		int32_t subpixel,
		const char *make,
		const char *model,
		int32_t transform)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;
	output->subpixel           = subpixel;

	if (data->verbose)
		fputs("Output updated geometry.\n", stderr);
}

static int min (int a, int b)
{
	if ( a > b )
		return b;
	else
		return a;
}

static void output_handle_mode (void *raw_data, struct wl_output *wl_output,
		uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;

	if (data->verbose)
		fprintf(stderr, "Output update mode w=%d h=%d"
				" refresh=%d flags=%d\n",
				width, height, refresh, flags);

	output->w = width;
	output->h = height;

	/* Center the bar for aggressive_anchor, as anchoring it to three sides
	 * will place it as close to the origin (0, 0) as possible.
	 */
	/* The min() is an ugly workaround, as a margin larger than the surface
	 * itself causes weird behaviour. TODO FIXME
	 */
	if (data->aggressive_anchor)
	{
		if (data->verbose)
			fputs("Centering bar", stderr);

		int margin = 0;
		switch (data->position)
		{
			case POSITION_TOP:
			case POSITION_BOTTOM:
				margin = (width / 2) - (data->w / 2);
				zwlr_layer_surface_v1_set_margin(output->layer_surface,
						0, 0, 0, min(margin, data->w));
				break;

			case POSITION_LEFT:
			case POSITION_RIGHT:
				margin = (height / 2) - (data->h / 2);
				zwlr_layer_surface_v1_set_margin(output->layer_surface,
						min(margin, data->h), 0, 0, 0);
				break;

			case POSITION_CENTER:
				/* Should never be reached, as aggressive_anchor
				 * will always be false for POSITION_CENTER
				 * (see main)
				 */
				return;
		}
		if (data->verbose)
			fprintf(stderr, " margin=%d\n", margin);
	}
}

static void output_handle_scale (void *raw_data, struct wl_output *wl_output, int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;
	output->scale              = (int)factor;

	if (data->verbose)
		fputs("Output updated scale.\n", stderr);
}

static const struct wl_output_listener output_listener = {
	.geometry = output_handle_geometry,
	.mode     = output_handle_mode,
	.done     = noop,
	.scale    = output_handle_scale
};

static void registry_handle_global (void *raw_data,
		struct wl_registry *registry,
		uint32_t name,
		const char *interface,
		uint32_t version)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;

	if (! strcmp(interface, wl_compositor_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_compositor.\n", stderr);
		data->compositor = wl_registry_bind(registry,
				name,
				&wl_compositor_interface,
				4);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_shm.\n", stderr);
		data->shm = wl_registry_bind(registry,
				name,
				&wl_shm_interface,
				1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		if (data->verbose)
			fputs("Get zwlr_layer_shell_v1.\n", stderr);
		data->layer_shell = wl_registry_bind(registry,
				name,
				&zwlr_layer_shell_v1_interface,
				1);
	}
	else if (! strcmp(interface, wl_seat_interface.name))
	{
		if (data->verbose)
			fputs("Adding seat.\n", stderr);

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
		if (data->verbose)
			fputs("Adding output.\n", stderr);

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
		output->scale       = 1;
		wl_list_insert(&data->outputs, &output->link);
		wl_output_set_user_data(wl_output, output);
		wl_output_add_listener(wl_output, &output_listener, output);
		create_bar(data, output);
	}
}

static void registry_handle_global_remove (void *raw_data,
		struct wl_registry *registry,
		uint32_t name)
{
	struct Lava_data   *data = (struct Lava_data *)raw_data;
	struct Lava_output *op1;
	struct Lava_output *op2;

	if (data->verbose)
		fputs("Global remove.\n", stderr);

	wl_list_for_each_safe(op1, op2, &data->outputs, link)
	{
		if ( op1->global_name == name )
		{
			wl_list_remove(&op1->link);
			wl_output_destroy(op1->wl_output);
			if ( op1->layer_surface != NULL )
				zwlr_layer_surface_v1_destroy(op1->layer_surface);
			if ( op1->wl_surface != NULL )
				wl_surface_destroy(op1->wl_surface);
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
	if (data->verbose)
		fputs("Init Wayland.\n", stderr);

	/* Init lists. */
	wl_list_init(&data->outputs);
	wl_list_init(&data->seats);

	/* Connect to Wayland server. */
	if (data->verbose)
		fputs("Connecting to server.\n", stderr);
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	data->display = wl_display_connect(wayland_display);
	assert(data->display);
	if ( data->display == NULL || wayland_display == NULL )
	{
		fputs("Can not connect to a Wayland server.\n", stderr);
		exit(EXIT_FAILURE);
	}

	/* Get registry and add listeners. */
	if (data->verbose)
		fputs("Get wl_registry.\n", stderr);
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
		fputs("Wayland compositor does not support wl_compositor.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
	if ( data->shm == NULL )
	{
		fputs("Wayland compositor does not support wl_shm.\n", stderr);
		exit(EXIT_FAILURE);
	}
	if ( data->layer_shell == NULL )
	{
		fputs("Wayland compositor does not support zwlr_layer_shell_v1.\n",
				stderr);
		exit(EXIT_FAILURE);
	}
}

static void deinit_wayland (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Deinit Wayland.\nDestroying outputs.\n", stderr);

	struct Lava_output *op_1, *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
	{
		wl_list_remove(&op_1->link);
		wl_output_destroy(op_1->wl_output);
		if ( op_1->layer_surface != NULL )
			zwlr_layer_surface_v1_destroy(op_1->layer_surface);
		if ( op_1->wl_surface != NULL )
			wl_surface_destroy(op_1->wl_surface);
		free(op_1->name);
		free(op_1);
	}

	if (data->verbose)
		fputs("Destroying seats.\n", stderr);

	struct Lava_seat *st_1, *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
	{
		wl_list_remove(&st_1->link);
		wl_seat_release(st_1->wl_seat);
		if (st_1->pointer.wl_pointer)
			wl_pointer_release(st_1->pointer.wl_pointer);
		free(st_1);
	}

	if (data->verbose)
		fputs("Destroying buttons.\n", stderr);

	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &data->buttons, link)
	{
		wl_list_remove(&bt_1->link);
		free(bt_1);
	}

	if (data->verbose)
		fputs("Destroying wlr_layer_shell_v1, wl_compositor,"
				"wl_shm and wl_registry.\n",
				stderr);

	zwlr_layer_shell_v1_destroy(data->layer_shell);
	wl_compositor_destroy(data->compositor);
	wl_shm_destroy(data->shm);
	wl_registry_destroy(data->registry);

	if (data->verbose)
		fputs("Diconnecting from server.\n", stderr);

	wl_display_disconnect(data->display);
}

static void main_loop (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Starting main loop.\n", stderr);

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
				fprintf(stderr, "wl_display_flush: %s\n",
						strerror(errno));
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
			fprintf(stderr, "wl_display_dispatch: %s\n",
					strerror(errno));
			break;
		}
		if ( fds.revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			fprintf(stderr, "wl_display_flush: %s\n",
					strerror(errno));
			break;
		}
	}
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {0};
	wl_list_init(&(data.buttons));

	/* Init data struct with sensible defaults. */
	sensible_defaults(&data);

	/* Handle command flags. */
	for (int c; (c = getopt(argc, argv, "ab:l:p:s:S:c:C:hv")) != -1 ;)
		switch (c)
		{
			/* Weirdly formatted for readability. */
			case 'v': data.verbose           = true; break;
			case 'a': data.aggressive_anchor = true; break;
			case 'h': fputs                    (usage, stderr); return EXIT_SUCCESS;
			case 'b': config_add_button        (&data, argv[optind-1], argv[optind]); optind++; break;
			case 'l': config_set_layer         (&data, optarg); break;
			case 'p': config_set_position      (&data, optarg); break;
			case 's': config_set_bar_size      (&data, optarg); break;
			case 'S': config_set_border_size   (&data, optarg); break;
			case 'c': config_set_bar_colour    (&data, optarg); break;
			case 'C': config_set_border_colour (&data, optarg); break;
			default:
				  return EXIT_FAILURE;
		}

	/* Count buttons. If none are defined, exit. */
	data.button_amount = wl_list_length(&(data.buttons));
	if (! data.button_amount)
	{
		fputs("No buttons defined!\n", stderr);
		return EXIT_FAILURE;
	}

	if (data.verbose)
		fputs("LavaLauncher Version 1.0\n", stderr);

	/* Calculating the size of the bar. At the "docking" edge no border will
	 * be drawn.
	 */
	switch (data.position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			data.w = (uint32_t)(data.bar_width + data.border_width);
			data.h = (uint32_t)((data.button_amount * data.bar_width)
					+ (2 * data.border_width));
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			data.w = (uint32_t)((data.button_amount * data.bar_width)
					+ (2 * data.border_width));
			data.h = (uint32_t)(data.bar_width + data.border_width);
			break;

		case POSITION_CENTER:
			data.aggressive_anchor = false;
			data.w = (uint32_t)((data.button_amount * data.bar_width)
					+ (2 * data.border_width));
			data.h = (uint32_t)(data.bar_width + (2 * data.border_width));
			break;
	}

	if (data.verbose)
		fprintf(stderr, "Bar w=%d h=%d buttons=%d\n",
				data.w, data.h, data.button_amount);

	init_wayland(&data);
	main_loop(&data);
	deinit_wayland(&data);

	if (data.verbose)
		fputs("Exiting.\n", stderr);

	return EXIT_SUCCESS;
}
