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

#ifndef LAVALAUNCHER_SEAT_H
#define LAVALAUNCHER_SEAT_H

#include<stdbool.h>
#include<stdint.h>
#include<xkbcommon/xkbcommon.h>

#include"types/buffer.h"

struct Lava_bar;
struct Lava_item_indicator;

enum Modifiers
{
	ALT     = 1 << 0,
	CAPS    = 1 << 1,
	CONTROL = 1 << 2,
	LOGO    = 1 << 3,
	NUM     = 1 << 4,
	SHIFT   = 1 << 5
};

enum Lava_cursor_type
{
	CURSOR_NONE,
	CURSOR_DEFAULT,
	CURSOR_POINTER
};

struct Lava_touchpoint
{
	struct wl_list            link;
	int32_t                   id;

	struct Lava_bar_instance  *instance;
	struct Lava_item_instance *item_instance;
};

struct Lava_seat
{
	struct wl_list link;
	struct wl_seat *wl_seat;
	uint32_t global_name;

	struct
	{
		struct wl_keyboard *wl_keyboard;

		struct xkb_context *context;
		struct xkb_keymap  *keymap;
		struct xkb_state   *state;

		enum Modifiers modifiers;
	} keyboard;

	struct
	{
		struct wl_pointer *wl_pointer;

		int click;

		/* Serial of enter event, used for setting the cursor. */
		uint32_t serial;

		/* Current position. */
		uint32_t x, y;
		struct Lava_bar_instance  *instance;
		struct Lava_item_instance *item_instance;

		/* Stuff needed to gracefully handle scroll events. */
		uint32_t   discrete_steps, last_update_time;
		wl_fixed_t value;

		/* Hover indicator. */
		struct Lava_item_indicator *indicator;

		struct
		{
			enum Lava_cursor_type   type;
			struct wl_surface      *surface;
			struct wl_cursor_theme *theme;
			struct wl_cursor_image *image;
			struct wl_cursor       *wl_cursor;
		} cursor;
	} pointer;

	struct
	{
		struct wl_touch *wl_touch;
		struct wl_list   touchpoints;
	} touch;
};

bool create_seat (struct wl_registry *registry, uint32_t name,
		const char *interface, uint32_t version);
struct Lava_seat *get_seat_from_global_name (uint32_t name);
void destroy_seat (struct Lava_seat *seat);

void destroy_touchpoint (struct Lava_touchpoint *touchpoint);

#endif

