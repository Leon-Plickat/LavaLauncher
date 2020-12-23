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
#include<errno.h>
#include<sys/mman.h>
#include<assert.h>

#include<wayland-client.h>
#include<wayland-client-protocol.h>
#include<wayland-cursor.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"str.h"
#include"seat.h"
#include"bar.h"
#include"item.h"
#include"output.h"

/* No-Op function. */
static void noop () {}

/**************
 *            *
 *  Keyboard  *
 *            *
 **************/
static const struct wl_keyboard_listener keyboard_listener  = {
	.enter       = noop,
	.keymap      = noop,
	.key         = noop,
	.leave       = noop,
	.modifiers   = noop,
	.repeat_info = noop
};

static void seat_release_keyboard (struct Lava_seat *seat)
{
	if ( seat->keyboard.wl_keyboard != NULL )
	{
		wl_keyboard_release(seat->keyboard.wl_keyboard);
		seat->keyboard.wl_keyboard = NULL;
	}

	if ( seat->keyboard.context != NULL )
	{
		xkb_context_unref(seat->keyboard.context);
		seat->keyboard.context = NULL;
	}
	if ( seat->keyboard.keymap != NULL )
	{
		xkb_keymap_unref(seat->keyboard.keymap);
		seat->keyboard.keymap = NULL;
	}

	if ( seat->keyboard.state != NULL )
	{
		xkb_state_unref(seat->keyboard.state);
		seat->keyboard.state = NULL;
	}

	seat->keyboard.modifiers   = 0;
}

static void seat_bind_keyboard (struct Lava_seat *seat)
{
	log_message(seat->data, 2, "[seat] Binding keyboard.\n");

	seat->keyboard.wl_keyboard = wl_seat_get_keyboard(seat->wl_seat);
	wl_keyboard_add_listener(seat->keyboard.wl_keyboard, &keyboard_listener, seat);

	/* Set up xkbcommon stuff. */
	if ( NULL == (seat->keyboard.context = xkb_context_new(0)) )
	{
		log_message(NULL, 0, "Error: Failed to setup xkb context.\n");
		seat_release_keyboard(seat);
		return;
	}

	struct xkb_rule_names rules = {
		.rules   = NULL,
		.model   = NULL,
		.layout  = NULL,
		.variant = NULL,
		.options = NULL
	};
	if ( NULL == (seat->keyboard.keymap = xkb_keymap_new_from_names(
					seat->keyboard.context, &rules, 0)) )
	{
		log_message(NULL, 0, "Error: Failed to setup xkb keymap.\n");
		seat_release_keyboard(seat);
		return;
	}

	if ( NULL == (seat->keyboard.state = xkb_state_new(seat->keyboard.keymap)) )
	{
		log_message(NULL, 0, "Error: Failed to setup xkb state.\n");
		seat_release_keyboard(seat);
		return;
	}
}

static void seat_init_keyboard (struct Lava_seat *seat)
{
	seat->keyboard.wl_keyboard = NULL;
	seat->keyboard.context     = NULL;
	seat->keyboard.keymap      = NULL;
	seat->keyboard.state       = NULL;
	seat->keyboard.modifiers   = 0;
}

/*****************
 *               *
 *  Touchpoints  *
 *               *
 *****************/
static bool create_touchpoint (struct Lava_seat *seat, int32_t id,
          struct Lava_bar_instance *instance, struct Lava_item *item)
{
	log_message(seat->data, 1, "[seat] Creating touchpoint.\n");

	struct Lava_touchpoint *touchpoint = calloc(1, sizeof(struct Lava_touchpoint));
	if ( touchpoint == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not allocate.\n");
		return false;
	}

	touchpoint->id       = id;
	touchpoint->instance = instance;
	touchpoint->item     = item;

	touchpoint->indicator = create_indicator(instance);
	if ( touchpoint->indicator != NULL )
	{
		touchpoint->indicator->touchpoint = touchpoint;
		indicator_set_colour(touchpoint->indicator, &instance->config->indicator_active_colour);
		move_indicator(touchpoint->indicator, item);
		indicator_commit(touchpoint->indicator);
	}
	else
		log_message(NULL, 0, "ERROR: Could not create indicator.\n");

	wl_list_insert(&seat->touch.touchpoints, &touchpoint->link);

	return true;
}

static void destroy_touchpoint (struct Lava_touchpoint *touchpoint)
{
	if ( touchpoint->indicator != NULL )
		destroy_indicator(touchpoint->indicator);

	wl_list_remove(&touchpoint->link);
	free(touchpoint);
}

static void destroy_all_touchpoints (struct Lava_seat *seat)
{
	log_message(seat->data, 1, "[seat] Destroying all touchpoints.\n");
	struct Lava_touchpoint *tp, *temp;
	wl_list_for_each_safe(tp, temp, &seat->touch.touchpoints, link)
		destroy_touchpoint(tp);
}

static struct Lava_touchpoint *touchpoint_from_id (struct Lava_seat *seat, int32_t id)
{
	struct Lava_touchpoint *touchpoint;
	wl_list_for_each(touchpoint, &seat->touch.touchpoints, link)
		if ( touchpoint->id == id )
			return touchpoint;
	return NULL;
}

/***********
 *         *
 *  Touch  *
 *         *
 ***********/
static void touch_handle_up (void *raw_data, struct wl_touch *wl_touch,
		uint32_t serial, uint32_t time, int32_t id)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_touchpoint *touchpoint = touchpoint_from_id(seat, id);
	if ( touchpoint == NULL )
		return;

	log_message(seat->data, 1, "[input] Touch up.\n");

	item_interaction(touchpoint->item, touchpoint->instance,
			INTERACTION_TOUCH,
			seat->keyboard.modifiers, 0);
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

	struct Lava_bar_instance *instance  = bar_instance_from_surface(seat->data, surface);
	struct Lava_item         *item = item_from_coords(instance, x, y);
	if (! create_touchpoint(seat, id, instance, item))
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
	if ( item_from_coords(touchpoint->instance, x, y) != touchpoint->item )
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
static const struct wl_touch_listener touch_listener = {
	.cancel      = touch_handle_cancel,
	.down        = touch_handle_down,
	.frame       = noop,
	.motion      = touch_handle_motion,
	.orientation = noop,
	.shape       = noop,
	.up          = touch_handle_up
};

static void seat_release_touch (struct Lava_seat *seat)
{
	destroy_all_touchpoints(seat);

	if ( seat->touch.wl_touch != NULL )
	{
		wl_touch_release(seat->touch.wl_touch);
		seat->touch.wl_touch = NULL;
	}
}

static void seat_bind_touch (struct Lava_seat *seat)
{
	log_message(seat->data, 2, "[seat] Binding touch.\n");
	seat->touch.wl_touch = wl_seat_get_touch(seat->wl_seat);
	wl_touch_add_listener(seat->touch.wl_touch, &touch_listener, seat);
}

static void seat_init_touch (struct Lava_seat *seat)
{
	seat->touch.wl_touch = NULL;
	wl_list_init(&seat->touch.touchpoints);
}

/************
 *          *
 *  Cursor  *
 *          *
 ************/
static void seat_pointer_unset_cursor (struct Lava_seat *seat)
{
	if ( seat->pointer.cursor_theme != NULL )
	{
		wl_cursor_theme_destroy(seat->pointer.cursor_theme);
		seat->pointer.cursor_theme = NULL;
	}
	if ( seat->pointer.cursor_surface != NULL )
	{
		wl_surface_destroy(seat->pointer.cursor_surface);
		seat->pointer.cursor_surface = NULL;
	}

	 /* These just points back to the theme. */
	seat->pointer.cursor       = NULL;
	seat->pointer.cursor_image = NULL;
}

static void seat_pointer_set_cursor (struct Lava_seat *seat, uint32_t serial, const char *name)
{
	struct Lava_data  *data    = seat->data;
	struct wl_pointer *pointer = seat->pointer.wl_pointer;

	int32_t scale       = (int32_t)seat->pointer.instance->output->scale;
	int32_t cursor_size = 24; // TODO ?

	/* Cleanup any leftover cursor stuff. */
	seat_pointer_unset_cursor(seat);

	if ( NULL == (seat->pointer.cursor_theme = wl_cursor_theme_load(NULL, cursor_size * scale, data->shm)) )
	{
		log_message(NULL, 0, "ERROR: Could not load cursor theme.\n");
		return;
	}

	if ( NULL == (seat->pointer.cursor = wl_cursor_theme_get_cursor(
					seat->pointer.cursor_theme, name)) )
	{
		log_message(NULL, 0, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n",
				name);
		seat_pointer_unset_cursor(seat);
		return;
	}

	seat->pointer.cursor_image = seat->pointer.cursor->images[0];
	assert(seat->pointer.cursor_image); // TODO Propably not needed; A non-fatal fail would be better here anyway

	if ( NULL == (seat->pointer.cursor_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Could not create cursor surface.\n");
		seat_pointer_unset_cursor(seat);
		return;
	}

	/* The entire dance of getting cursor image and surface and damaging
	 * the latter is indeed necessary every time a seats pointer enters the
	 * surface and we want to change its cursor image, because we need to
	 * apply the scale of the current output to the cursor.
	 */

	wl_surface_set_buffer_scale(seat->pointer.cursor_surface, scale);
	wl_surface_attach(seat->pointer.cursor_surface,
			wl_cursor_image_get_buffer(seat->pointer.cursor_image),
			0, 0);
	wl_surface_damage_buffer(seat->pointer.cursor_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(seat->pointer.cursor_surface);

	wl_pointer_set_cursor(pointer, serial, seat->pointer.cursor_surface,
			(int32_t)seat->pointer.cursor_image->hotspot_x / scale,
			(int32_t)seat->pointer.cursor_image->hotspot_y / scale);
}

/*************
 *           *
 *  Pointer  *
 *           *
 *************/
#define CONTINUOUS_SCROLL_THRESHHOLD 10000
#define CONTINUOUS_SCROLL_TIMEOUT    1000

static void pointer_handle_leave (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface)
{
	struct Lava_seat *seat = data;

	if ( seat->pointer.indicator != NULL )
		destroy_indicator(seat->pointer.indicator);

	/* We have to check every seat before we can be sure that no pointer
	 * hovers over the bar. Only if this is the case can we hide the bar.
	 */
	// TODO we may need to consider touch points here
	struct Lava_seat *st;
	wl_list_for_each(st, &seat->data->seats, link)
		if ( st != seat && st->pointer.instance == seat->pointer.instance )
			goto skip_update_hiding;
	seat->pointer.instance->hover = false;
	bar_instance_update_hidden_status(seat->pointer.instance);

skip_update_hiding:

	seat->pointer.x        = 0;
	seat->pointer.y        = 0;
	seat->pointer.instance = NULL;
	seat->pointer.item     = NULL;

	log_message(seat->data, 1, "[input] Pointer left surface.\n");
}

static void pointer_handle_enter (void *data, struct wl_pointer *wl_pointer,
		uint32_t serial, struct wl_surface *surface,
		wl_fixed_t x, wl_fixed_t y)
{
	struct Lava_seat *seat = data;

	if ( NULL == (seat->pointer.instance = bar_instance_from_surface(seat->data, surface)) )
		return;

	seat_pointer_set_cursor(seat, serial, str_orelse(seat->pointer.instance->config->cursor_name, "pointer"));

	seat->pointer.instance->hover = true;
	bar_instance_update_hidden_status(seat->pointer.instance);

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

	struct Lava_item *item = item_from_coords(seat->pointer.instance,
			seat->pointer.x, seat->pointer.y);

	if ( item == NULL || item->type != TYPE_BUTTON )
	{
		if ( seat->pointer.indicator != NULL )
			destroy_indicator(seat->pointer.indicator);
		return;
	}

	if ( seat->pointer.indicator == NULL )
	{
		seat->pointer.indicator = create_indicator(seat->pointer.instance);
		if ( seat->pointer.indicator == NULL )
		{
			log_message(NULL, 0, "ERROR: Could not create indicator.\n");
			return;
		}
		seat->pointer.indicator->seat = seat;

		indicator_set_colour(seat->pointer.indicator,
				&seat->pointer.instance->config->indicator_hover_colour);
	}

	move_indicator(seat->pointer.indicator, item);
	indicator_commit(seat->pointer.indicator);
}

static void pointer_handle_button (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t serial, uint32_t time, uint32_t button, uint32_t button_state)
{
	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.instance == NULL )
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
					&seat->pointer.instance->config->indicator_active_colour);
			indicator_commit(seat->pointer.indicator);
		}

		log_message(seat->data, 1, "[input] Button pressed: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);
		seat->pointer.item = item_from_coords(seat->pointer.instance,
				seat->pointer.x, seat->pointer.y);
	}
	else
	{
		if ( seat->pointer.indicator != NULL )
		{
			indicator_set_colour(seat->pointer.indicator,
					&seat->pointer.instance->config->indicator_hover_colour);
			indicator_commit(seat->pointer.indicator);
		}

		log_message(seat->data, 1, "[input] Button released: x=%d y=%d\n",
					seat->pointer.x, seat->pointer.y);

		if ( seat->pointer.item == NULL )
			return;

		struct Lava_item *item = item_from_coords(seat->pointer.instance,
				seat->pointer.x, seat->pointer.y);

		if ( item != seat->pointer.item )
			return;

		seat->pointer.item = NULL;

		item_interaction(item, seat->pointer.instance,
				INTERACTION_MOUSE_BUTTON,
				seat->keyboard.modifiers, button);
	}
}

static void pointer_handle_axis (void *raw_data, struct wl_pointer *wl_pointer,
		uint32_t time, uint32_t axis, wl_fixed_t value)
{
	/* We only handle up and down scrolling. */
	if ( axis != WL_POINTER_AXIS_VERTICAL_SCROLL )
		return;

	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.instance == NULL )
	{
		log_message(NULL, 0, "ERROR: Scrolling could not be handled: "
				"Bar instance could not be found.\n");
		return;
	}

	if ( seat->pointer.discrete_steps == 0
			&& time - seat->pointer.last_update_time > CONTINUOUS_SCROLL_TIMEOUT )
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
	if ( seat->pointer.instance == NULL )
	{
		log_message(NULL, 0, "ERROR: Discrete scrolling could not be handled: "
				"Bar instance could not be found.\n");
		return;
	}

	seat->pointer.discrete_steps += (uint32_t)abs(steps);
}

static void pointer_handle_frame (void *raw_data, struct wl_pointer *wl_pointer)
{
	struct Lava_seat *seat = raw_data;
	if ( seat->pointer.instance == NULL )
		return;


	int value_change;
	uint32_t direction; /* 0 == down, 1 == up */
	if ( wl_fixed_to_int(seat->pointer.value) > 0 )
		direction = 0, value_change = -CONTINUOUS_SCROLL_THRESHHOLD;
	else
		direction = 1, value_change = CONTINUOUS_SCROLL_THRESHHOLD;

	struct Lava_item *item = item_from_coords(seat->pointer.instance,
			seat->pointer.x, seat->pointer.y);

	if (seat->pointer.discrete_steps)
	{
		for (uint32_t i = 0; i < seat->pointer.discrete_steps; i++)
			item_interaction(item, seat->pointer.instance,
					INTERACTION_MOUSE_SCROLL,
					seat->keyboard.modifiers, direction);

		seat->pointer.discrete_steps = 0;
		seat->pointer.value          = 0;
	}
	else while ( abs(seat->pointer.value) > CONTINUOUS_SCROLL_THRESHHOLD )
	{
		item_interaction(item, seat->pointer.instance,
				INTERACTION_MOUSE_SCROLL,
				seat->keyboard.modifiers, direction);
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
static const struct wl_pointer_listener pointer_listener = {
	.axis_discrete = pointer_handle_axis_discrete,
	.axis          = pointer_handle_axis,
	.axis_source   = noop,
	.axis_stop     = noop,
	.button        = pointer_handle_button,
	.enter         = pointer_handle_enter,
	.frame         = pointer_handle_frame,
	.leave         = pointer_handle_leave,
	.motion        = pointer_handle_motion
};

static void seat_release_pointer (struct Lava_seat *seat)
{
	seat_pointer_unset_cursor(seat);

	if ( seat->pointer.wl_pointer != NULL )
	{
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}
}

static void seat_bind_pointer (struct Lava_seat *seat)
{
	log_message(seat->data, 2, "[seat] Binding pointer.\n");
	seat->pointer.wl_pointer = wl_seat_get_pointer(seat->wl_seat);
	wl_pointer_add_listener(seat->pointer.wl_pointer, &pointer_listener, seat);
}

static void seat_init_pointer (struct Lava_seat *seat)
{
	seat->pointer.wl_pointer       = NULL;
	seat->pointer.x                = 0;
	seat->pointer.y                = 0;
	seat->pointer.instance         = NULL;
	seat->pointer.item             = NULL;
	seat->pointer.discrete_steps   = 0;
	seat->pointer.last_update_time = 0;
	seat->pointer.value            = wl_fixed_from_int(0);
	seat->pointer.indicator        = NULL;
	seat->pointer.cursor_surface   = NULL;
	seat->pointer.cursor_theme     = NULL;
	seat->pointer.cursor_image     = NULL;
	seat->pointer.cursor           = NULL;
}

/**********
 *        *
 *  Seat  *
 *        *
 **********/
static void seat_handle_capabilities (void *raw_data, struct wl_seat *wl_seat,
		uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_data *data = seat->data;

	log_message(data, 1, "[seat] Handling seat capabilities.\n");

	if ( capabilities & WL_SEAT_CAPABILITY_KEYBOARD && data->need_keyboard )
		seat_bind_keyboard(seat);
	else
		seat_release_keyboard(seat);

	if ( capabilities & WL_SEAT_CAPABILITY_POINTER && data->need_pointer )
		seat_bind_pointer(seat);
	else
		seat_release_pointer(seat);

	if ( capabilities & WL_SEAT_CAPABILITY_TOUCH && data->need_touch )
		seat_bind_touch(seat);
	else
		seat_release_touch(seat);
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name         = noop
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	log_message(data, 1, "[seat] Adding seat.\n");

	struct wl_seat *wl_seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
	struct Lava_seat *seat = calloc(1, sizeof(struct Lava_seat));
	if ( seat == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not allocate.\n");
		return false;
	}

	wl_seat_add_listener(wl_seat, &seat_listener, seat);

	seat->data    = data;
	seat->wl_seat = wl_seat;

	seat_init_touch(seat);
	seat_init_keyboard(seat);
	seat_init_pointer(seat);

	wl_list_insert(&data->seats, &seat->link);

	return true;
}

static void destroy_seat (struct Lava_seat *seat)
{
	seat_release_keyboard(seat);
	seat_release_touch(seat);
	seat_release_pointer(seat);
	wl_seat_release(seat->wl_seat);
	wl_list_remove(&seat->link);
	free(seat);
}

void destroy_all_seats (struct Lava_data *data)
{
	log_message(data, 1, "[seat] Destroying all seats.\n");
	struct Lava_seat *st_1, *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
		destroy_seat(st_1);
}

