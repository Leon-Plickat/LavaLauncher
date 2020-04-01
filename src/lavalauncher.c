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

#define VERSION "1.6"
#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<assert.h>
#include<poll.h>
#include<errno.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>
#include<cairo/cairo.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"config.h"
#include"layersurface.h"
#include"draw.h"
#include"buffer.h"
#include"command.h"
#include"input.h"

static const char usage[] = "LavaLauncher -- Version "VERSION"\n\n"
                            "Usage: lavalauncher [options...]\n"
                            "  -b <path> <command>               Add a button.\n"
                            "  -c <colour>                       Background colour.\n"
                            "  -C <colour>                       Border colour.\n"
                            "  -e <mode>                         Exclusive zone.\n"
                            "  -h                                Display help text and exit.\n"
                            "  -l <layer>                        Layer of surface.\n"
                            "  -m <margin>                       Margin.\n"
                            "  -o <name>                         Name of exclusive output.\n"
                            "  -O <orientation>                  Orientation of bar.\n"
                            "  -p <position>                     Position of the bar.\n"
                            "  -s <size>                         Icon size.\n"
                            "  -S <size>                         All border sizes.\n"
                            "  -S <top> <right> <bottom> <left>  Individual border sizes.\n"
                            "  -v                                Verbose output.\n";

/* No-Op function. */
static void noop () {}

static void seat_handle_capabilities (void *raw_data, struct wl_seat *wl_seat,
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
		if (data->verbose)
			fputs("Seat has pointer capabilities.\n", stderr);
		seat->pointer.wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer.wl_pointer,
				&pointer_listener, seat);
	}

	if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
	{
		if (data->verbose)
			fputs("Seat has touch capabilities.\n", stderr);
		seat->touch.wl_touch = wl_seat_get_touch(wl_seat);
		wl_touch_add_listener(seat->touch.wl_touch,
				&touch_listener, seat);
	}
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name         = noop
};

static void output_handle_scale (void *raw_data, struct wl_output *wl_output,
		int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->scale              = factor;

	if (output->data->verbose)
		fprintf(stderr, "Output update scale: s=%d\n", output->scale);

	configure_surface(output->data, output);
	render_bar_frame(output->data, output);
}

static const struct wl_output_listener output_listener = {
	.scale    = output_handle_scale,
	.geometry = noop,
	.mode     = noop,
	.done     = noop
};

static void xdg_output_handle_name (void *raw_data,
		struct zxdg_output_v1 *xdg_output, const char *name)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->name               = strdup(name);

	if (output->data->verbose)
		fprintf(stderr, "XDG-Output update name: name=%s\n", name);
}

static void xdg_output_handle_logical_size (void *raw_data,
		struct zxdg_output_v1 *xdg_output, int32_t w, int32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->w                  = w;
	output->h                  = h;

	if (output->data->verbose)
		fprintf(stderr, "XDG-Output update logical size: w=%d h=%d\n",
				w, h);

	render_bar_frame(output->data, output);
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_size     = xdg_output_handle_logical_size,
	.name             = xdg_output_handle_name,
	.logical_position = noop,
	.description      = noop,
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
			fputs("ERROR: Could not allocate.\n", stderr);
			data->loop = false;
			data->ret  = EXIT_FAILURE;
			return;
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

		struct wl_output *wl_output = wl_registry_bind(registry, name,
				&wl_output_interface, 3);
		struct Lava_output *output = calloc(1, sizeof(struct Lava_output));
		if ( output == NULL )
		{
			fputs("ERROR: Could not allocate.\n", stderr);
			data->loop = false;
			data->ret  = EXIT_FAILURE;
			return;
		}

		output->data        = data;
		output->global_name = name;
		output->wl_output   = wl_output;
		output->xdg_output  = zxdg_output_manager_v1_get_xdg_output(
				data->xdg_output_manager, output->wl_output);
		output->configured  = false;

		wl_list_insert(&data->outputs, &output->link);
		wl_output_set_user_data(wl_output, output);
		wl_output_add_listener(wl_output, &output_listener, output);
		zxdg_output_v1_add_listener(output->xdg_output,
				&xdg_output_listener, output);

		/* Roundtrip to allow the output handlers to "catch up", as
		 * the outputs dimensions as well as name are needed for
		 * creating the bar.
		 */
		if ( wl_display_roundtrip(data->display) == -1 )
		{
			fputs("ERROR: Roundtrip failed.\n", stderr);
			data->loop = false;
			data->ret  = EXIT_FAILURE;
			return;
		}

		assert(output->name);
		if ( data->layer_shell == NULL )
			return;

		/* If either the name of output equals only_output or if no
		 * only_output has been given, create a surface for this new
		 * output.
		 */
		if ( data->only_output == NULL
				|| ! strcmp(output->name, data->only_output) )
			if (! create_bar(data, output))
			{
				data->loop = false;
				data->ret  = EXIT_FAILURE;
			}
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

/* Helper function for capability support error message. */
static bool capability_test (void *ptr, const char *name)
{
	if ( ptr == NULL )
	{
		fprintf(stderr, "ERROR: Wayland compositor does not support %s.\n", name);
		return false;
	}
	return true;
}

static bool init_wayland (struct Lava_data *data)
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
		fputs("ERROR: Can not connect to a Wayland server.\n", stderr);
		return false;
	}

	/* Get registry and add listeners. */
	if (data->verbose)
		fputs("Get wl_registry.\n", stderr);
	data->registry = wl_display_get_registry(data->display);
	if ( data->registry == NULL )
	{
		fputs("ERROR: Can not get registry.\n", stderr);
		return false;
	}
	wl_registry_add_listener(data->registry, &registry_listener, data);

	/* Allow registry listeners to catch up. */
	if ( wl_display_roundtrip(data->display) == -1 )
	{
		fputs("ERROR: Roundtrip failed.\n", stderr);
		return false;
	}

	/* Testing compatibilities. */
	if (! capability_test(data->compositor, "wl_compositor"))
		return false;
	if (! capability_test(data->shm, "wl_shm"))
		return false;
	if (! capability_test(data->layer_shell, "zwlr_layer_shell"))
		return false;

	return true;
}

static void destroy_buttons (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Destroying buttons and freeing icons.\n", stderr);

	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &data->buttons, link)
	{
		wl_list_remove(&bt_1->link);
		cairo_surface_destroy(bt_1->img);
		free(bt_1);
	}
}

/* Finish him! */
static void finish_wayland (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Finish Wayland.\nDestroying outputs.\n", stderr);

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
		fputs("Destroying wlr_layer_shell_v1, wl_compositor, "
				"wl_shm and wl_registry.\n", stderr);

	if ( data->layer_shell != NULL )
		zwlr_layer_shell_v1_destroy(data->layer_shell);
	if ( data->compositor != NULL )
		wl_compositor_destroy(data->compositor);
	if ( data->shm != NULL )
		wl_shm_destroy(data->shm);
	if ( data->registry != NULL )
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
				fprintf(stderr, "ERROR: wl_display_flush: %s\n",
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
			fprintf(stderr, "ERROR: wl_display_dispatch: %s\n",
					strerror(errno));
			break;
		}
		if ( fds.revents & POLLOUT && wl_display_flush(data->display) == -1 )
		{
			fprintf(stderr, "ERROR: wl_display_flush: %s\n",
					strerror(errno));
			break;
		}
	}
}

/* This functions calculates the dimensions of the minimal visible part of the
 * bar.
 */
static void calculate_dimensions (struct Lava_data *data)
{
	if ( data->orientation == ORIENTATION_HORIZONTAL)
	{
		data->w = (uint32_t)((data->button_amount * data->icon_size)
				+ data->border_left + data->border_right);
		data->h = (uint32_t)(data->icon_size + data->border_top
				+ data->border_bottom);
		if ( data->exclusive_zone == 1 )
			data->exclusive_zone = data->h;
	}
	else
	{
		data->w = (uint32_t)(data->icon_size + data->border_right
				+ data->border_left);
		data->h = (uint32_t)((data->button_amount * data->icon_size)
				+ data->border_top + data->border_bottom);
		if ( data->exclusive_zone == 1 )
			data->exclusive_zone = data->w;
	}
}

/* getopts() only handles command flags with none or a single argument directly;
 * For multiple argument we have to do some of the work ourself. This function
 * counts the arguments to a command flag.
 */
static int count_arguments (int optind, int argc, char *argv[])
{
	int args = 0, index = optind - 1;
	while ( index < argc )
	{
		if ( *argv[index] == '-' )
			break;
		args++, index++;
	}
	return args;
}

static bool handle_command_flags (int argc, char *argv[], struct Lava_data *data)
{
	int temp;
	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "b:c:C:e:hl:m:o:O:p:s:S:v")) != -1 ;)
	{
		switch (c)
		{
			case 'b':
				if( count_arguments(optind, argc, argv) != 2 )
				{
					fputs("ERROR: '-b' expects 2 arguments.\n",
							stderr);
					data->ret = EXIT_FAILURE;
					break;
				}
				config_add_button(data, argv[optind-1], argv[optind]);
				optind++; /* Tell getopt() to skip one argv field. */
				break;

			case 'c': config_set_bar_colour(data, optarg);    break;
			case 'C': config_set_border_colour(data, optarg); break;
			case 'e': config_set_exclusive(data, optarg);     break;
			case 'h': fputs(usage, stderr);                   return EXIT_SUCCESS;
			case 'l': config_set_layer(data, optarg);         break;
			case 'm': config_set_margin(data, optarg);        break;
			case 'o': data->only_output = optarg;             break;
			case 'O': config_set_orientation(data, optarg);   break;
			case 'p': config_set_position(data, optarg);      break;
			case 's': config_set_icon_size(data, optarg);     break;

			case 'S':
				temp = count_arguments(optind, argc, argv);
				if ( temp == 1 )
					config_set_border_size_all(data, atoi(optarg));
				else if ( temp == 4 )
				{
					config_set_border_size_specific(data,
							atoi(argv[optind-1]), atoi(argv[optind]),
							atoi(argv[optind+1]), atoi(argv[optind+2]));
					optind += 3; /* Tell getopt() to "skip" three argv fields. */
				}
				else
				{
					fputs("ERROR: '-S' expects 1 or 4 arguments.\n",
							stderr);
					data->ret = EXIT_FAILURE;
				}
				break;

			case 'v': data->verbose = true;    break;

			default:
				return false;
		}

		if ( data->ret == EXIT_FAILURE )
		{
			if ( wl_list_length(&(data->buttons)) > 0 )
				destroy_buttons(data);
			return false;;
		}
	}

	return true;
}

/* Set the orientation depending on the positon. */
static void automatic_orientation (struct Lava_data *data)
{
	switch (data->anchors)
	{
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT:
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT:
			data->orientation = ORIENTATION_VERTICAL;
			break;

		default:
			data->orientation = ORIENTATION_HORIZONTAL;
			break;
	}
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {0};
	data.ret              = EXIT_SUCCESS;
	wl_list_init(&(data.buttons));

	/* Init data struct with sensible defaults. */
	sensible_defaults(&data);

	/* Handle command flags. */
	if (! handle_command_flags(argc, argv, &data))
		return EXIT_FAILURE;

	/* Count buttons. If none are defined, exit. */
	data.button_amount = wl_list_length(&(data.buttons));
	if (! data.button_amount)
	{
		fputs("ERROR: No buttons defined.\n", stderr);
		return EXIT_FAILURE;
	}

	/* If the user did not specify an orientation, chose a good default,
	 * depending on what edge the surface is anchored to.
	 */
	if ( data.orientation == ORIENTATION_AUTO )
		automatic_orientation(&data);

	/* Calculate dimension of the visible part of the bar. */
	calculate_dimensions(&data);

	if (data.verbose)
		fprintf(stderr, "LavaLauncher Version "VERSION"\n"
				"Bar: w=%d h=%d buttons=%d\n", data.w, data.h,
				data.button_amount);

	/* Prevent zombies. */
	signal(SIGCHLD, SIG_IGN);

	if (! init_wayland(&data))
		data.ret = EXIT_FAILURE;
	else
		main_loop(&data);

	destroy_buttons(&data);
	finish_wayland(&data);
	return data.ret;
}
