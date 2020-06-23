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
#include"output.h"
#include"cursor.h"
#include"seat.h"
#include"bar/bar.h"

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
	struct Lava_data       *data           = seat->data;
	struct wl_pointer      *pointer        = seat->pointer.wl_pointer;
	struct wl_surface      *cursor_surface = seat->pointer.cursor_surface;
	struct wl_cursor_theme *theme          = seat->pointer.cursor_theme;
	struct wl_cursor_image *image          = seat->pointer.cursor_image;
	struct wl_cursor       *cursor         = seat->pointer.cursor;

	unsigned int scale       = seat->pointer.bar->output->scale;
	unsigned int cursor_size = 24; // TODO ?

	/* Cleanup any leftover cursor stuff. */
	cleanup_cursor(seat);

	if ( NULL == (theme = wl_cursor_theme_load(NULL, cursor_size * scale, data->shm)) )
	{
		fputs("ERROR: Could not load cursor theme.\n", stderr);
		return;
	}

	if ( NULL == (cursor = wl_cursor_theme_get_cursor(theme, data->cursor_name)) )
	{
		fprintf(stderr, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n",
				data->cursor_name);
		return;
	}

	image = cursor->images[0];
	assert(image);

	if ( NULL == (cursor_surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Could not create cursor surface.\n", stderr);
		return;
	}

	/* The entire dance of getting cursor image and surface and damaging the
	 * latter is indeed necessary everytime a seats pointer enters the
	 * surface and we want to change its cursor image, because we need to
	 * apply the scale of the current output to the cursor.
	 */

	wl_surface_set_buffer_scale(cursor_surface, scale);
	wl_surface_attach(cursor_surface, wl_cursor_image_get_buffer(image), 0, 0);
	wl_surface_damage_buffer(cursor_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(cursor_surface);

	wl_pointer_set_cursor(pointer, serial, cursor_surface,
			image->hotspot_x / scale, image->hotspot_y / scale);
}
