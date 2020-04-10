/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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
#include<unistd.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"output.h"
#include"seat.h"
#include"command.h"

/* No-Op function. */
static void noop () {}

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

		button_command(seat->data, bar_button, seat->pointer.output);
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
const struct wl_pointer_listener pointer_listener = {
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
	button_command(seat->data, seat->touch.button, seat->touch.output);
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
 *
 * Behold: Since I lack the necessary hardware, touch support is untested.
 */
const struct wl_touch_listener touch_listener = {
	.down        = touch_handle_down,
	.up          = touch_handle_up,
	.motion      = touch_handle_motion,
	.frame       = noop,
	.cancel      = noop,
	.shape       = noop,
	.orientation = noop
};
