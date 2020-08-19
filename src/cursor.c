/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Henrik Plickat
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
#include<assert.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>
#include<wayland-cursor.h>

#include"lavalauncher.h"
#include"log.h"
#include"output.h"
#include"cursor.h"
#include"seat.h"
#include"bar/bar.h"
#include"bar/bar-pattern.h"
#include"types/string-container.h"

void cleanup_cursor (struct Lava_seat *seat)
{
	 /* image just points back to theme. */
	if ( seat->pointer.cursor_theme != NULL )
		wl_cursor_theme_destroy(seat->pointer.cursor_theme );
	if ( seat->pointer.cursor_surface != NULL )
		wl_surface_destroy(seat->pointer.cursor_surface );
}

void attach_cursor (struct Lava_seat *seat, uint32_t serial)
{
	struct Lava_data  *data        = seat->data;
	struct wl_pointer *pointer     = seat->pointer.wl_pointer;
	char              *cursor_name = seat->pointer.bar->pattern->cursor_name->string;

	int32_t scale       = (int32_t)seat->pointer.bar->output->scale;
	int32_t cursor_size = 24; // TODO ?

	/* Cleanup any leftover cursor stuff. */
	cleanup_cursor(seat);

	if ( NULL == (seat->pointer.cursor_theme = wl_cursor_theme_load(NULL, cursor_size * scale, data->shm)) )
	{
		log_message(NULL, 0, "ERROR: Could not load cursor theme.\n");
		return;
	}

	if ( NULL == (seat->pointer.cursor = wl_cursor_theme_get_cursor(
					seat->pointer.cursor_theme, cursor_name)) )
	{
		log_message(NULL, 0, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n",
				cursor_name);
		return;
	}

	seat->pointer.cursor_image = seat->pointer.cursor->images[0];
	assert(seat->pointer.cursor_image);

	if ( NULL == (seat->pointer.cursor_surface = wl_compositor_create_surface(data->compositor)) )
	{
		log_message(NULL, 0, "ERROR: Could not create cursor surface.\n");
		return;
	}

	/* The entire dance of getting cursor image and surface and damaging the
	 * latter is indeed necessary everytime a seats pointer enters the
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
