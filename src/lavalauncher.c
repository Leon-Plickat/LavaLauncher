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
#define STRING_BUFFER_SIZE 4096
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

#include"lavalauncher.h"
#include"config.h"
#include"draw.h"

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"
#include"buffer.h"

#define SHELL "/bin/sh"

static const char usage[] = "LavaLauncher -- Version "VERSION"\n\n"
                            "Usage: lavalauncher [options...]\n"
                            "  -a <alignments>                   Alignment.\n"
                            "  -b <path> <command>               Add a button.\n"
                            "  -c <colour>                       Background colour.\n"
                            "  -C <colour>                       Border colour.\n"
                            "  -e <mode>                         Exclusive zone.\n"
                            "  -h                                Display help text and exit.\n"
                            "  -l <layer>                        Layer of surface.\n"
                            "  -m <mode>                         Display mode of bar.\n"
                            "  -M <top> <right> <bottom> <left>  Margin.\n"
                            "  -o <name>                         Name of exclusive output.\n"
                            "  -p <position>                     Bar position.\n"
                            "  -s <size>                         Icon size.\n"
                            "  -S <top> <right> <bottom> <left>  Border sizes.\n"
                            "  -v                                Verbose output.\n";

/* No-Op function. */
static void noop () {}

/* Helper function to configure the bar surface. Careful: This function does not
 * commit!
 */
static void configure_surface (struct Lava_data *data, struct Lava_output *output)
{
	uint32_t width, height;
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		width = output->w, height = data->h;
	else
		width = data->w, height = output->h;

	if (data->verbose)
		fprintf(stderr, "Surface size: w=%d h=%d\n", width, height);

	zwlr_layer_surface_v1_set_size(output->layer_surface, width, height);

	/* Anchor the surface to the correct screen edge. */
	switch (data->position)
	{
		case POSITION_TOP:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
			break;

		case POSITION_RIGHT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
			break;

		case POSITION_BOTTOM:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
			break;

		case POSITION_LEFT:
			zwlr_layer_surface_v1_set_anchor(output->layer_surface,
					ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
					| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM);
			break;
	}

	/* Set margin.
	 * Since we create a surface spanning the entire length of an outputs
	 * edge, margins parallel to it would move it outside the boundaries of
	 * the output, which may or may not cause issues in some compositors.
	 * To work around this, we simply cheat a bit: Margins parallel to the
	 * bar will be simulated in the draw code by adjusting the bar offsets.
	 *
	 * See: update_bar_offset()
	 *
	 * Here we set the margins not parallel to the edges length, which are
	 * real layer shell margins.
	 */
	if ( data->orientation == ORIENTATION_HORIZONTAL )
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					data->margin_top, 0,
					data->margin_bottom, 0);
	else
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, data->margin_right,
					0, data->margin_left);

	/* Set exclusive zone to prevent other surfaces from obstructing ours. */
	zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
			data->exclusive_zone);

	struct wl_region *region = wl_compositor_create_region(data->compositor);
	if ( data->mode == MODE_DEFAULT )
		wl_region_add(region, output->bar_x_offset, output->bar_y_offset,
				data->w, data->h);
	else
		wl_region_add(region, 0, 0, output->w, output->h);

	/* Set input region. This is necessary to prevent the unused parts of
	 * the surface to catch pointer and touch events.
	 */
	wl_surface_set_input_region(output->wl_surface, region);

	/* If both border and background are opaque, set opaque region. This
	 * will inform the compositor that it does not have to render anything
	 * below the surface.
	 */
	if ( data->bar_colour[3] == 1 && data->border_colour[3] == 1 )
		wl_surface_set_opaque_region(output->wl_surface, region);

	wl_region_destroy(region);
}

static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;
	output->configured         = true;

	if (data->verbose)
		fprintf(stderr, "Layer surface configure request:"
				" w=%d h=%d serial=%d\n", w, h, serial);

	configure_surface(data, output);
	zwlr_layer_surface_v1_ack_configure(surface, serial);
	render_bar_frame(data, output);
}

static void layer_surface_handle_closed (void *raw_data,
		struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;
	fputs("Layer surface has been closed.\n", stderr);
	data->loop = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

static bool create_bar (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	output->wl_surface = wl_compositor_create_surface(data->compositor);
	if ( output->wl_surface == NULL )
	{
		fputs("ERROR: Compositor did not create wl_surface.\n", stderr);
		return false;
	}

	output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			data->layer_shell, output->wl_surface,
			output->wl_output, data->layer, "LavaLauncher");
	if ( output->layer_surface == NULL )
	{
		fputs("ERROR: Compositor did not create layer_surface.\n", stderr);
		return false;
	}

	configure_surface(data, output);
	zwlr_layer_surface_v1_add_listener(output->layer_surface,
			&layer_surface_listener, output);
	wl_surface_commit(output->wl_surface);
	return true;
}

/* This function will search for a string (*srch) inside a given string (**str)
 * and replace it either with another string (*repl_s) or with a string-ified
 * integer (repl_i, when *repl_s == NULL).
 */
// This likely sucks...
static void string_insert (char **str, char *srch, char *repl_s, int repl_i, size_t size)
{
	char  buffer[STRING_BUFFER_SIZE]; /* Local editing buffer. */
	char *p;                          /* Pointer to beginning of *srch. */

	/* Iterate over all occurrences of *srch */
	while ((p = strstr(*str, srch)))
	{
		/* Copy str to buffer, but only until p. */
		strncpy(buffer, *str, p - *str);

		/* Insert replacement and rest of string. */
		if ( repl_s == NULL )
			sprintf(buffer + (p - *str), "%d%s", repl_i, p + strlen(srch));
		else
			sprintf(buffer + (p - *str), "%s%s", repl_s, p + strlen(srch));

		/* Copy buffer back to str. */
		strncpy(*str, buffer, size);
	}
}

static void exec_cmd (struct Lava_data *data, struct Lava_output *output,
		const char *cmd)
{
	errno   = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		errno = 0;
		if ( setsid() == -1 )
		{
			fprintf(stderr, "ERROR: setsid: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		char *buffer = malloc(STRING_BUFFER_SIZE);
		strncpy(buffer, cmd, STRING_BUFFER_SIZE);

		string_insert(&buffer, "%buttons%",       NULL,                    data->button_amount, STRING_BUFFER_SIZE);
		string_insert(&buffer, "%icon-size%",     NULL,                    data->icon_size,     STRING_BUFFER_SIZE);
		string_insert(&buffer, "%border-top%",    NULL,                    data->border_top,    STRING_BUFFER_SIZE);
		string_insert(&buffer, "%border-left%",   NULL,                    data->border_left,   STRING_BUFFER_SIZE);
		string_insert(&buffer, "%border-bottom%", NULL,                    data->border_bottom, STRING_BUFFER_SIZE);
		string_insert(&buffer, "%border-right%",  NULL,                    data->border_right,  STRING_BUFFER_SIZE);
		string_insert(&buffer, "%colour%",        data->bar_colour_hex,    0,                   STRING_BUFFER_SIZE);
		string_insert(&buffer, "%border-colour%", data->border_colour_hex, 0,                   STRING_BUFFER_SIZE);
		string_insert(&buffer, "%output%",        output->name,            0,                   STRING_BUFFER_SIZE);
		string_insert(&buffer, "%scale%",         NULL,                    output->scale,       STRING_BUFFER_SIZE);

		if (data->verbose)
			fprintf(stderr, "Executing: cmd='%s' raw='%s'\n",
					buffer, cmd);

		errno = 0;
		if ( system(buffer) == -1 && errno != ECHILD )
		{
			fprintf(stderr, "ERROR: system: %s\n", strerror(errno));
			free(buffer);
			exit(EXIT_FAILURE);
		}

		free(buffer);
		exit(EXIT_SUCCESS);
	}
	else if ( ret < 0 )
		fprintf(stderr, "ERROR: fork: %s\n", strerror(errno));
}

/* Return pointer to Lava_button struct from button list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
static struct Lava_button *button_from_coords (struct Lava_data *data,
		struct Lava_output *output, int32_t x, int32_t y)
{
	int32_t ordinate;
	int32_t pre_button_zone = 0;

	if ( data->orientation == ORIENTATION_HORIZONTAL )
	{
		pre_button_zone += output->bar_x_offset + data->border_left;
		ordinate         = x;
	}
	else
	{
		pre_button_zone += output->bar_y_offset + data->border_top;
		ordinate         = y;
	}

	int32_t i = pre_button_zone;
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

static void pointer_handle_enter (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = data;

	seat->pointer.output = NULL;
	struct Lava_output *op1, *op2;
	wl_list_for_each_safe(op1, op2, &seat->data->outputs, link)
	{
		if ( op1->wl_surface == surface )
		{
			seat->pointer.output = op1;
			break;
		}
	}

	seat->pointer.x = wl_fixed_to_int(x);
	seat->pointer.y = wl_fixed_to_int(y);

	if (seat->data->verbose)
		fprintf(stderr, "Pointer entered surface: x=%d y=%d\n",
				seat->pointer.x, seat->pointer.y);
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = data;

	seat->pointer.x = wl_fixed_to_int(x);
	seat->pointer.y = wl_fixed_to_int(y);
}

static void pointer_handle_button (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t button_state)
{
	struct Lava_seat *seat = raw_data;

	/* Only execute the command if the pointer button was pressed and
	 * released over the same button on the bar.
	 */
	if ( button_state == WL_POINTER_BUTTON_STATE_PRESSED )
	{
		if (seat->data->verbose)
			fprintf(stderr, "Button pressed: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);
		seat->pointer.button = button_from_coords(seat->data,
				seat->pointer.output,
				seat->pointer.x, seat->pointer.y);
	}
	else
	{
		if (seat->data->verbose)
			fprintf(stderr, "Button released: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);

		if ( seat->pointer.button == NULL )
			return;

		struct Lava_button *bar_button = button_from_coords(seat->data,
				seat->pointer.output,
				seat->pointer.x, seat->pointer.y);

		if ( bar_button != seat->pointer.button )
			return;

		seat->pointer.button = NULL;

		if (! strcmp(bar_button->cmd, "exit"))
			seat->data->loop = false;
		else
			exec_cmd(seat->data, seat->pointer.output, bar_button->cmd);
	}
}

/* These are the listeners for pointer events. Only if a mouse button has been
 * pressed and released over the same bar button do we want that buttons command
 * to be executed. To achieve this, pointer_handle_enter() and
 * pointer_handle_motion() will update the cursor coordinates stored in the seat.
 * pointer_handle_button() will on press store the bar button under the pointer
 * in the seat. On release it will check whether the button under the pointer is
 * the one stored in the seat and execute the buttons commands if this is the
 * case. pointer_handle_leave() will simply abort the pointer operation.
 */
static const struct wl_pointer_listener pointer_listener = {
	.enter  = pointer_handle_enter,
	.leave  = pointer_handle_leave,
	.motion = pointer_handle_motion,
	.button = pointer_handle_button,
	.axis   = noop
};

static void touch_handle_up (void *raw_data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;

	if (seat->data->verbose)
		fputs("Touch up.\n", stderr);

	/* If this touch event does not have the same id as the touch-down event
	 * which initiated the touch operation, abort. Also abort if no touch
	 * operation has been initiated.
	 */
	if ( ! seat->touch.button || id != seat->touch.id )
	{
		seat->touch.button = NULL;
		seat->touch.output = NULL;
		return;
	}

	/* At this point, we know for sure that we have received a touch-down
	 * and the following touch-up event over the same button, so we can
	 * execute its command.
	 */
	if (! strcmp(seat->touch.button->cmd, "exit"))
		seat->data->loop = false;
	else
		exec_cmd(seat->data, seat->pointer.output, seat->touch.button->cmd);
}

static void touch_handle_down (void *raw_data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time,
		struct wl_surface *surface, int32_t id,
		wl_fixed_t fx, wl_fixed_t fy)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;

	uint32_t x = wl_fixed_to_int(fx), y = wl_fixed_to_int(fy);

	if (seat->data->verbose)
		fprintf(stderr, "Touch down: x=%d y=%d\n", x, y);

	/* Find the output of the touch event. */
	seat->touch.output = NULL;
	struct Lava_output *op1, *op2;
	wl_list_for_each_safe(op1, op2, &seat->data->outputs, link)
	{
		if ( op1->wl_surface == surface )
		{
			seat->touch.output = op1;
			break;
		}
	}

	seat->touch.id     = id;
	seat->touch.button = button_from_coords(seat->data, seat->touch.output,
			x, y);
}

static void touch_handle_motion (void *raw_data, struct wl_touch *wl_touch,
		uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;

	if (seat->data->verbose)
		fputs("Touch move.\n", stderr);

	if ( id != seat->touch.id )
		return;

	/* The touch point has been moved, meaning it likely is no longer over
	 * the button, so we simply abort the touch operation.
	 */
	seat->touch.button = NULL;
	seat->touch.output = NULL;
}

/* These are the handlers for touch events. We only want to execute a buttons
 * command, if both touch-down and touch-up were over the same button. To
 * achieve this, touch_handle_down() will store the touch id and the button in
 * the seat. touch_handle_up() checks whether a button is stored in the seat and
 * whether its id is the same as the one stored in the seat and if both are true
 * executes the buttons command. touch_handle_motion() will simply remove the
 * button from the seat, effectively aborting the touch operation.
 */
static const struct wl_touch_listener touch_listener = {
	.down        = touch_handle_down,
	.up          = touch_handle_up,
	.motion      = touch_handle_motion,
	.frame       = noop,
	.cancel      = noop,
	.shape       = noop,
	.orientation = noop
};

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

static void update_bar_offset (struct Lava_data *data, struct Lava_output *output)
{
	if ( output->w == 0 || output->h == 0 )
		return;

	switch (data->alignment)
	{
		case ALIGNMENT_START:
			output->bar_x_offset = 0;
			output->bar_y_offset = 0;
			break;

		case ALIGNMENT_CENTER:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				output->bar_x_offset = (output->w / 2) - (data->w / 2);
				output->bar_y_offset = 0;
			}
			else
			{
				output->bar_x_offset = 0;
				output->bar_y_offset = (output->h / 2) - (data->h / 2);
			}
			break;

		case ALIGNMENT_END:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				output->bar_x_offset = output->w  - data->w;
				output->bar_y_offset = 0;
			}
			else
			{
				output->bar_x_offset = 0;
				output->bar_y_offset = output->h - data->h;
			}
			break;
	}

	/* Set margin.
	 * Since we create a surface spanning the entire length of an outputs
	 * edge, margins parallel to it would move it outside the boundaries of
	 * the output, which may or may not cause issues in some compositors.
	 * To work around this, we simply cheat a bit: Margins parallel to the
	 * bar will be simulated in the draw code by adjusting the bar offsets.
	 *
	 * See: configure_surface()
	 *
	 * Here we set the margins parallel to the edges length, which are
	 * "fake", by adjusting the offset of the bar.
	 */
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		output->bar_x_offset += data->margin_left - data->margin_right;
	else
		output->bar_y_offset += data->margin_top - data->margin_bottom;

	if (data->verbose)
		fprintf(stderr, "Aligning bar: x-offset=%d y-offset=%d\n",
				output->bar_x_offset, output->bar_y_offset);
}

static void output_handle_scale (void *raw_data, struct wl_output *wl_output,
		int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->scale              = factor;

	if (output->data->verbose)
		fprintf(stderr, "Output update scale: s=%d\n", output->scale);

	update_bar_offset(output->data, output);
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

	update_bar_offset(output->data, output);
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
			if (! create_bar(output->data, output))
			{
				output->data->loop = false;
				output->data->ret  = EXIT_FAILURE;
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
	if ( data->compositor == NULL )
	{
		fputs("ERROR: Wayland compositor does not support wl_compositor.\n",
				stderr);
		return false;
	}
	if ( data->shm == NULL )
	{
		fputs("ERROR: Wayland compositor does not support wl_shm.\n",
				stderr);
		return false;
	}
	if ( data->layer_shell == NULL )
	{
		fputs("ERROR: Wayland compositor does not support zwlr_layer_shell_v1.\n",
				stderr);
		return false;
	}

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
	switch (data->position)
	{
		case POSITION_LEFT:
		case POSITION_RIGHT:
			data->orientation = ORIENTATION_VERTICAL;
			data->w = (uint32_t)(data->icon_size + data->border_right
					+ data->border_left);
			data->h = (uint32_t)((data->button_amount * data->icon_size)
					+ data->border_top + data->border_bottom);
			if ( data->exclusive_zone == 1 )
				data->exclusive_zone = data->w;
			break;

		case POSITION_TOP:
		case POSITION_BOTTOM:
			data->orientation = ORIENTATION_HORIZONTAL;
			data->w = (uint32_t)((data->button_amount * data->icon_size)
					+ data->border_left + data->border_right);
			data->h = (uint32_t)(data->icon_size + data->border_top
					+ data->border_bottom);
			if ( data->exclusive_zone == 1 )
				data->exclusive_zone = data->h;
			break;
	}
}

/* getopts() only handles command flags with none or a single argument directly;
 * For multiple argument we have to do some of the work ourself. This function
 * counts the arguments to a command flag and checks if that is expected amount.
 * If it is the correct amount, it will return true, if not, an error message,
 * specifying the flag and the expected amount of arguments, is printed and the
 * function returns false.
 */
static bool check_arguments (int optind, int argc, char *argv[], int expected,
		const char *flag)
{
	int args = 0, index = optind - 1;
	while ( index < argc )
	{
		if ( *argv[index] == '-' )
			break;
		args++, index++;
	}

	if ( args == expected )
		return true;

	fprintf(stderr, "ERROR: '%s' expects %d arguments.\n", flag, expected);
	return false;
}

int main (int argc, char *argv[])
{
	struct Lava_data data = {0};
	data.ret              = EXIT_SUCCESS;
	wl_list_init(&(data.buttons));

	/* Init data struct with sensible defaults. */
	sensible_defaults(&data);

	/* Handle command flags. */
	extern int optind;
	extern char *optarg;
	for (int c; (c = getopt(argc, argv, "a:b:e:hl:m:M:o:p:s:S:c:C:v")) != -1 ;)
	{
		switch (c)
		{
			case 'a': config_set_alignment(&data, optarg); break;

			case 'b':
				if(! check_arguments(optind, argc, argv, 2, "-b"))
				{
					data.ret = EXIT_FAILURE;
					break;
				}
				config_add_button(&data, argv[optind-1], argv[optind]);
				optind++; /* Tell getopt() to "skip" one argv field. */
				break;

			case 'c': config_set_bar_colour(&data, optarg);    break;
			case 'C': config_set_border_colour(&data, optarg); break;
			case 'e': config_set_exclusive(&data, optarg);     break;
			case 'h': fputs(usage, stderr);                    return EXIT_SUCCESS;
			case 'l': config_set_layer(&data, optarg);         break;
			case 'm': config_set_mode(&data, optarg);          break;

			case 'M':
				if (! check_arguments(optind, argc, argv, 4, "-M"))
				{
					data.ret = EXIT_FAILURE;
					break;
				}
				config_set_margin(&data, atoi(argv[optind-1]),
						atoi(argv[optind]), atoi(argv[optind+1]),
						atoi(argv[optind+2]));
				optind += 3; /* Tell getopt() to "skip" three argv fields. */
				break;

			case 'o': data.only_output = optarg;               break;
			case 'p': config_set_position(&data, optarg);      break;
			case 's': config_set_icon_size(&data, optarg);     break;

			case 'S':
				if (! check_arguments(optind, argc, argv, 4, "-S"))
				{
					data.ret = EXIT_FAILURE;
					break;
				}
				config_set_border_size(&data,
						atoi(argv[optind-1]), atoi(argv[optind]),
						atoi(argv[optind+1]), atoi(argv[optind+2]));
				optind += 3; /* Tell getopt() to "skip" three argv fields. */
				break;

			case 'v': data.verbose = true; break;

			default:
				return EXIT_FAILURE;
		}

		if ( data.ret == EXIT_FAILURE )
		{
			if ( wl_list_length(&(data.buttons)) > 0 )
				destroy_buttons(&data);
			return EXIT_FAILURE;
		}
	}

	/* Count buttons. If none are defined, exit. */
	data.button_amount = wl_list_length(&(data.buttons));
	if (! data.button_amount)
	{
		fputs("ERROR: No buttons defined.\n", stderr);
		return EXIT_FAILURE;
	}

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
