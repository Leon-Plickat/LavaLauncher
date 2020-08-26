/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Henrik Plickat
 * Copyright (C) 2020 Nicolai Dagestad
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
#include<linux/input-event-codes.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"log.h"
#include"output.h"
#include"seat.h"
#include"cursor.h"
#include"item/item.h"
#include"bar/bar.h"
#include"bar/bar-pattern.h"
#include"bar/indicator.h"

const int      continuous_scroll_threshhold = 10000;
const uint32_t continuous_scroll_timeout    = 1000;

/* No-Op function. */
static void noop () {}

static void pointer_handle_leave (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface)
{
	struct Lava_seat *seat = data;

	if ( seat->pointer.indicator != NULL )
		destroy_indicator(seat->pointer.indicator);

	seat->pointer.x        = 0;
	seat->pointer.y        = 0;
	seat->pointer.bar      = NULL;
	seat->pointer.item     = NULL;

	log_message(seat->data, 1, "[input] Pointer left surface.\n");
}

static void pointer_handle_enter (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = data;

	if ( NULL == (seat->pointer.bar = bar_from_surface(seat->data, surface)) )
		return;

	attach_cursor(seat, serial);

	seat->pointer.x = (uint32_t)wl_fixed_to_int(x);
	seat->pointer.y = (uint32_t)wl_fixed_to_int(y);

	log_message(seat->data, 1, "[input] Pointer entered surface: x=%d y=%d\n",
				seat->pointer.x, seat->pointer.y);
}

static void pointer_handle_motion(void *data, struct wl_pointer *wl_pointer,
		uint32_t time, wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = data;

	seat->pointer.x = (uint32_t)wl_fixed_to_int(x);
	seat->pointer.y = (uint32_t)wl_fixed_to_int(y);

	/* It is enough to only update the indicator every other motion event. */
	static bool skip = false;
	if (skip)
	{
		skip = false;
		return;
	}
	else
		skip = true;

	struct Lava_item *item = item_from_coords(seat->pointer.bar,
			seat->pointer.x, seat->pointer.y);

	if ( item == NULL || item->type != TYPE_BUTTON )
	{
		if ( seat->pointer.indicator != NULL )
			destroy_indicator(seat->pointer.indicator);
		return;
	}

	if ( seat->pointer.indicator == NULL )
	{
		seat->pointer.indicator = create_indicator(seat->pointer.bar);
		if ( seat->pointer.indicator == NULL )
		{
			log_message(NULL, 0, "ERROR: Could not create indicator.\n");
			return;
		}
		seat->pointer.indicator->seat = seat;

		indicator_set_colour(seat->pointer.indicator,
				&seat->pointer.bar->pattern->indicator_hover_colour);
	}

	move_indicator(seat->pointer.indicator, item);
	indicator_commit(seat->pointer.indicator);
}

static void pointer_handle_button (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t button_state)
{
	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.bar == NULL )
	{
		log_message(NULL, 0, "ERROR: Button press could not be handled: "
				"Bar could not be found.\n");
		return;
	}

	/* Only interact with the item if the pointer button was pressed and
	 * released over the same item on the bar.
	 */
	if ( button_state == WL_POINTER_BUTTON_STATE_PRESSED )
	{
		if ( seat->pointer.indicator != NULL )
		{
			indicator_set_colour(seat->pointer.indicator,
					&seat->pointer.bar->pattern->indicator_active_colour);
			indicator_commit(seat->pointer.indicator);
		}

		log_message(seat->data, 1, "[input] Button pressed: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);
		seat->pointer.item = item_from_coords(seat->pointer.bar,
				seat->pointer.x, seat->pointer.y);
	}
	else
	{
		if ( seat->pointer.indicator != NULL )
		{
			indicator_set_colour(seat->pointer.indicator,
					&seat->pointer.bar->pattern->indicator_hover_colour);
			indicator_commit(seat->pointer.indicator);
		}

		log_message(seat->data, 1, "[input] Button released: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);

		if ( seat->pointer.item == NULL )
			return;

		struct Lava_item *item = item_from_coords(seat->pointer.bar,
				seat->pointer.x, seat->pointer.y);

		if ( item != seat->pointer.item )
			return;

		seat->pointer.item = NULL;

		enum Interaction_type type;
		switch (button)
		{
			case BTN_RIGHT:  type = TYPE_RIGHT_CLICK;  break;
			case BTN_MIDDLE: type = TYPE_MIDDLE_CLICK; break;
			default:
			case BTN_LEFT:   type = TYPE_LEFT_CLICK;   break;
		}
		item_interaction(seat->pointer.bar, item, type);
	}
}

static void pointer_handle_axis (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis, wl_fixed_t value)
{
	/* We only handle up and down scrolling. */
	if ( axis != WL_POINTER_AXIS_VERTICAL_SCROLL )
		return;

	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.bar == NULL )
	{
		log_message(NULL, 0, "ERROR: Scrolling could not be handled: "
				"Bar could not be found.\n");
		return;
	}

	if ( seat->pointer.discrete_steps == 0
			&& time - seat->pointer.last_update_time > continuous_scroll_timeout )
		seat->pointer.value = 0;

	seat->pointer.value            += value;
	seat->pointer.last_update_time  = time;
}

static void pointer_handle_axis_discrete (void *raw_data,
		struct wl_pointer *wl_pointer, uint32_t axis, int32_t steps)
{
	/* We only handle up and down scrolling. */
	if ( axis != WL_POINTER_AXIS_VERTICAL_SCROLL )
		return;

	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.bar == NULL )
	{
		log_message(NULL, 0, "ERROR: Discrete scrolling could not be handled: "
				"Bar could not be found.\n");
		return;
	}

	seat->pointer.discrete_steps += (uint32_t)abs(steps);
}

static void pointer_handle_frame (void *raw_data, struct wl_pointer *wl_pointer)
{
	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.bar == NULL )
		return;


	int value_change;
	enum Interaction_type type;
	if ( wl_fixed_to_int(seat->pointer.value) > 0 )
		type = TYPE_SCROLL_DOWN, value_change = -continuous_scroll_threshhold;
	else
		type = TYPE_SCROLL_UP, value_change = continuous_scroll_threshhold;

	struct Lava_item *item = item_from_coords(seat->pointer.bar,
			seat->pointer.x, seat->pointer.y);

	if (seat->pointer.discrete_steps)
	{
		for (uint32_t i = 0; i < seat->pointer.discrete_steps; i++)
			item_interaction(seat->pointer.bar, item, type);

		seat->pointer.discrete_steps = 0;
		seat->pointer.value          = 0;
	}
	else while ( abs(seat->pointer.value) > continuous_scroll_threshhold )
	{
		item_interaction(seat->pointer.bar, item, type);
		seat->pointer.value += value_change;
	}
}

/*
 * These are the listeners for pointer events. Only if a mouse button has been
 * pressed and released over the same bar item do we want that to interact with
 * the item. To achieve this, pointer_handle_enter() and pointer_handle_motion()
 * will update the cursor coordinates stored in the seat.
 * pointer_handle_button() will on press store the bar item under the pointer
 * in the seat. On release it will check whether the item under the pointer is
 * the one stored in the seat and interact with the item if this is the case.
 * pointer_handle_leave() will simply abort the pointer operation.
 *
 * All axis events (meaning scrolling actions) are handled in the frame event.
 * This is done, because pointing device like mice send a "click" (discrete axis
 * event) as well as a scroll value, while other devices like touchpads only
 * send a scroll value. For the second type of device, the value must be
 * converted into virtual "clicks". Because these devices scroll very fast, the
 * virtual "clicks" have a higher associated scroll value then physical
 * "clicks". Physical "clicks" must also be handled here to reset the scroll
 * value to avoid handling scrolling twice (Remember: Wayland makes no
 * guarantees regarding the order in which axis and axis_discrete events are
 * received).
 */
const struct wl_pointer_listener pointer_listener = {
	.enter         = pointer_handle_enter,
	.leave         = pointer_handle_leave,
	.motion        = pointer_handle_motion,
	.button        = pointer_handle_button,
	.axis          = pointer_handle_axis,
	.frame         = pointer_handle_frame,
	.axis_source   = noop,
	.axis_stop     = noop,
	.axis_discrete = pointer_handle_axis_discrete
};

static void touch_handle_up (void *raw_data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_touchpoint *touchpoint = touchpoint_from_id(seat, id);
	if ( touchpoint == NULL )
		return;

	log_message(seat->data, 1, "[input] Touch up.\n");

	item_interaction(touchpoint->bar, touchpoint->item, TYPE_TOUCH);
	destroy_touchpoint(touchpoint);
}

static void touch_handle_down (void *raw_data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time,
		struct wl_surface *surface, int32_t id,
		wl_fixed_t fx, wl_fixed_t fy)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	uint32_t x = (uint32_t)wl_fixed_to_int(fx), y = (uint32_t)wl_fixed_to_int(fy);

	log_message(seat->data, 1, "[input] Touch down: x=%d y=%d\n", x, y);

	struct Lava_bar  *bar  = bar_from_surface(seat->data, surface);
	struct Lava_item *item = item_from_coords(bar, x, y);
	if (! create_touchpoint(seat, id, bar, item))
		log_message(NULL, 0, "ERROR: could not create touchpoint\n");
}

static void touch_handle_motion (void *raw_data, struct wl_touch *wl_touch,
		uint32_t time, int32_t id, wl_fixed_t fx, wl_fixed_t fy)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_touchpoint *touchpoint = touchpoint_from_id(seat, id);
	if ( touchpoint == NULL )
		return;

	log_message(seat->data, 2, "[input] Touch move\n");

	/* If the item under the touch point is not the same we first touched,
	 * we simply abort the touch operation.
	 */
	uint32_t x = (uint32_t)wl_fixed_to_int(fx), y = (uint32_t)wl_fixed_to_int(fy);
	if ( item_from_coords(touchpoint->bar, x, y) != touchpoint->item )
		destroy_touchpoint(touchpoint);
}

static void touch_handle_cancel (void *raw, struct wl_touch *touch)
{
	/* The cancel event means that the compositor has decided to take over
	 * the touch-input, possibly for gestures, and that therefore we should
	 * stop caring about all active touchpoints.
	 *
	 * The vast majority of such compositor guestures will already be caught
	 * by touch_handle_motion(), but nothing stops a compositor from having
	 * "hold for X seconds" as a valid gesture.
	 */

	struct Lava_seat *seat = (struct Lava_seat *)raw;
	destroy_all_touchpoints(seat);
}

/* These are the handlers for touch events. We only want to interact with an
 * item, if both touch-down and touch-up were over the same item. To
 * achieve this, each touch event is stored in the wl_list, inside seat->touch.
 * This ways we can follow each of them without needing any extra logic.
 */
const struct wl_touch_listener touch_listener = {
	.down        = touch_handle_down,
	.up          = touch_handle_up,
	.motion      = touch_handle_motion,
	.frame       = noop,
	.cancel      = touch_handle_cancel,
	.shape       = noop,
	.orientation = noop
};
