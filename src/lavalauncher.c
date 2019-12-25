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
#include"xdg-output-unstable-v1-client-protocol.h"
#include"xdg-shell-client-protocol.h"
#include"pool-buffer.h"

#define SHELL "/bin/sh"

static const char usage[] = "LavaLauncher -- Version 1.1\n\n"
                            "Usage: lavalauncher [options...]\n\n"
                            "  -b <path> <command>                      Add a button.\n"
                            "  -c <colour>                              Background colour.\n"
                            "  -C <colour>                              Border colour.\n"
                            "  -h                                       Display this help text and exit.\n"
                            "  -l <overlay|top|bottom|background>       Layer of the bar surface.\n"
                            "  -m <default|aggressive|full|full-center> Display mode of bar.\n"
                            "  -M <size>                                Margin to screen edge.\n"
                            "  -p <top|bottom|left|right|center>        Position of the bar.\n"
                            "  -s <size>                                Icon size.\n"
                            "  -S <size>                                Border size.\n"
                            "  -v                                       Verbose output.\n\n"
                            "Buttons are displayed in the order in which they are declared.\n"
                            "Commands will be executed with sh(1).\n"
                            "Colours are expected to be in the format #RRGGBBAA.\n"
                            "Sizes are expected to be in pixels.\n\n"
                            "You can send bug reports, contributions and user feedback to the mailinglist:\n"
                            "<~leon_plickat/lavalauncher@lists.sr.ht>\n";

/* No-Op function. */
static void noop () {}

static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;
	output->configured         = true;

	if (data->verbose)
		fprintf(stderr, "Layer surface configure request"
				" w=%d h=%d serial=%d\n", w, h, serial);

	uint32_t width = data->w, height = data->h;

	if ( data->mode != MODE_DEFAULT )
		switch (data->position)
		{
			case POSITION_TOP:
			case POSITION_BOTTOM:
				width = output->w;
				break;

			case POSITION_LEFT:
			case POSITION_RIGHT:
				height = output->h;
				break;

			case POSITION_CENTER:
				/* Will never be reached. */
				break;
		}

	if (data->verbose)
		fprintf(stderr, "Resizing surface w=%d h=%d\n",
				width, height);

	zwlr_layer_surface_v1_ack_configure(surface, serial);
	zwlr_layer_surface_v1_set_size(output->layer_surface, width, height);
	wl_surface_commit(output->wl_surface);

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
					data->mode != MODE_DEFAULT
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->h);
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					data->margin, 0, 0, 0);
			break;

		case POSITION_RIGHT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->mode != MODE_DEFAULT
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->w);
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, data->margin, 0, 0);
			break;

		case POSITION_BOTTOM:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->mode != MODE_DEFAULT
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->h);
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, 0, data->margin, 0);
			break;

		case POSITION_LEFT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					data->mode != MODE_DEFAULT
					? ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					: ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					data->w);
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, 0, 0, data->margin);
			break;

		case POSITION_CENTER:
			zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
					-1);
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

static struct Lava_button *button_from_ordinate (struct Lava_data *data,
		struct Lava_output *output, int ordinate,
		enum Bar_orientation orientation)
{
	int pre_button_zone = 0;
	if ( data->mode == MODE_DEFAULT || data->mode == MODE_AGGRESSIVE )
		pre_button_zone += data->border_size;
	if ( orientation == ORIENTATION_HORIZONTAL )
		pre_button_zone += output->bar_x_offset;
	else
		pre_button_zone += output->bar_y_offset;

	int i = pre_button_zone;
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		i += data->icon_size;
		if ( ordinate < i && ordinate >= pre_button_zone)
			return bt_1;
	}
	return NULL;
}

static void pointer_handle_leave (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface)
{
	struct Lava_seat *seat = data;
	seat->pointer.x        = -1;
	seat->pointer.y        = -1;
	seat->pointer.output   = NULL;
	seat->pointer.button   = NULL;

	if (seat->data->verbose)
		fputs("Pointer left surface.\n", stderr);
}

static void pointer_motion (struct Lava_seat *seat, wl_fixed_t x, wl_fixed_t y)
{
	seat->pointer.x = wl_fixed_to_int(x);
	seat->pointer.y = wl_fixed_to_int(y);

	struct Lava_button *old_button = seat->pointer.button;

	switch (seat->data->position)
	{
		case POSITION_TOP:
		case POSITION_BOTTOM:
		case POSITION_CENTER:
			seat->pointer.button = button_from_ordinate(seat->data,
					seat->pointer.output,
					seat->pointer.x,
					ORIENTATION_HORIZONTAL);
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			seat->pointer.button = button_from_ordinate(seat->data,
					seat->pointer.output,
					seat->pointer.y,
					ORIENTATION_VERTICAL);
			break;
	}

	// TODO draw highlighting
	//if ( old_button != seat->pointer.button )
	//	render_bar_frame(seat->data, seat->pointer.output);
}

static void pointer_handle_enter (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct Lava_seat *seat = data;

	struct Lava_output *op1, *op2;
	wl_list_for_each_safe(op1, op2, &seat->data->outputs, link)
	{
		if ( op1->wl_surface == surface )
		{
			seat->pointer.output = op1;
			goto handle_motion;
		}
	}
	seat->pointer.output = NULL;

handle_motion:

	pointer_motion(seat, surface_x, surface_y);

	if (seat->data->verbose)
		fprintf(stderr, "Pointer entered surface x=%d y=%d\n",
				seat->pointer.x, seat->pointer.y);
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct Lava_seat *seat = data;
	pointer_motion(seat, surface_x, surface_y);
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

	if ( seat->pointer.button == NULL )
	{
		if (seat->data->verbose)
			fputs("\n", stderr);
		return;
	}

	if (seat->data->verbose)
		fprintf(stderr, " cmd=%s\n", seat->pointer.button->cmd);

	if (! strcmp(seat->pointer.button->cmd, "exit"))
		seat->data->loop = false;
	else
		exec_cmd(seat->pointer.button->cmd);
}

static const struct wl_pointer_listener pointer_listener = {
	.enter  = pointer_handle_enter,
	.leave  = pointer_handle_leave,
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

static void update_bar_offset (struct Lava_data *data, struct Lava_output *output)
{
	if ( data->mode != MODE_AGGRESSIVE && data->mode != MODE_FULL_CENTER )
	{
		output->bar_x_offset = 0;
		output->bar_y_offset = 0;
		return;
	}

	switch (data->position)
	{
		case POSITION_TOP:
		case POSITION_BOTTOM:
			output->bar_x_offset = (output->w / 2) - (data->w / 2);
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			output->bar_y_offset = (output->h / 2) - (data->h / 2);
			break;

		case POSITION_CENTER:
			/* Will never be reached. */
			break;
	}

	if (data->verbose)
		fprintf(stderr, "Centering bar x-offset=%d y-offset=%d\n",
				output->bar_x_offset, output->bar_y_offset);
}

static void output_handle_scale (void *raw_data, struct wl_output *wl_output,
		int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->scale              = factor;

	if (output->data->verbose)
		fprintf(stderr, "Output update scale s=%d\n", output->scale);

	update_bar_offset(output->data, output);
	render_bar_frame(output->data, output);
}

static const struct wl_output_listener output_listener = {
	.scale    = output_handle_scale,
	.geometry = noop,
	.mode     = noop,
	.done     = noop
};

static void xdg_output_handle_logical_size (void *raw_data,
		struct zxdg_output_v1 *xdg_output, int32_t w, int32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->w                  = w;
	output->h                  = h;

	if (output->data->verbose)
		fprintf(stderr, "XDG-Output update logical size w=%d h=%d\n",
				w, h);

	update_bar_offset(output->data, output);
	render_bar_frame(output->data, output);
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_size     = xdg_output_handle_logical_size,
	.logical_position = noop,
	.description      = noop,
	.name             = noop,
	.done             = noop
};

static void registry_handle_global (void *raw_data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;

	if (! strcmp(interface, wl_compositor_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_compositor.\n", stderr);
		data->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_shm.\n", stderr);
		data->shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		if (data->verbose)
			fputs("Get zwlr_layer_shell_v1.\n", stderr);
		data->layer_shell = wl_registry_bind(registry, name,
				&zwlr_layer_shell_v1_interface, 1);
	}
	else if (! strcmp(interface, zxdg_output_manager_v1_interface.name))
	{
		if (data->verbose)
			fputs("Get zxdg_output_manager_v1.\n", stderr);
		data->xdg_output_manager = wl_registry_bind(registry, name,
				&zxdg_output_manager_v1_interface, 2);
	}
	else if (! strcmp(interface, wl_seat_interface.name))
	{
		if (data->verbose)
			fputs("Adding seat.\n", stderr);

		struct wl_seat *wl_seat = wl_registry_bind(registry, name,
				&wl_seat_interface, 3);
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
		output->xdg_output  = zxdg_output_manager_v1_get_xdg_output(
				data->xdg_output_manager, output->wl_output);
		output->scale       = 1;
		output->w           = 0;
		output->h           = 0;
		output->configured  = false;
		wl_list_insert(&data->outputs, &output->link);
		wl_output_set_user_data(wl_output, output);
		wl_output_add_listener(wl_output, &output_listener, output);
		zxdg_output_v1_add_listener(output->xdg_output,
				&xdg_output_listener, output);
		create_bar(data, output);
	}
}

static void registry_handle_global_remove (void *raw_data,
		struct wl_registry *registry, uint32_t name)
{
	struct Lava_data   *data = (struct Lava_data *)raw_data;
	struct Lava_output *op1, *op2;

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
	for (int c; (c = getopt(argc, argv, "b:l:m:M:p:s:S:c:C:hv")) != -1 ;)
		switch (c)
		{
			/* Weirdly formatted for readability. */
			case 'v': data.verbose           = true; break;
			case 'h': fputs                    (usage, stderr); return EXIT_SUCCESS;
			case 'b': config_add_button        (&data, argv[optind-1], argv[optind]); optind++; break;
			case 'l': config_set_layer         (&data, optarg); break;
			case 'm': config_set_mode          (&data, optarg); break;
			case 'M': config_set_margin        (&data, optarg); break;
			case 'p': config_set_position      (&data, optarg); break;
			case 's': config_set_icon_size     (&data, optarg); break;
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
		fputs("LavaLauncher Version 1.1\n", stderr);

	/* Calculating the size of the bar. At the "docking" edge no border will
	 * be drawn.
	 */
	switch (data.position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			data.w = (uint32_t)(data.icon_size + data.border_size);
			data.h = (uint32_t)((data.button_amount * data.icon_size)
					+ (2 * data.border_size));
			if (data.margin)
				data.w += data.border_size;
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			data.w = (uint32_t)((data.button_amount * data.icon_size)
					+ (2 * data.border_size));
			data.h = (uint32_t)(data.icon_size + data.border_size);
			if (data.margin)
				data.h += data.border_size;
			break;

		case POSITION_CENTER:
			data.mode = MODE_DEFAULT;
			data.w = (uint32_t)((data.button_amount * data.icon_size)
					+ (2 * data.border_size));
			data.h = (uint32_t)(data.icon_size + (2 * data.border_size));
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
