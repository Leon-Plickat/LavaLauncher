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
#include"config/config.h"
#include"cursor.h"

bool init_cursor (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Init cursor.\n", stderr);

	struct Lava_cursor *cursor = &data->cursor;
	cursor->name    = NULL;
	cursor->theme   = NULL;
	cursor->image   = NULL;
	cursor->surface = NULL;

	if ( NULL == (cursor->theme = wl_cursor_theme_load(NULL, 16, data->shm)) )
	{
		fputs("ERROR: Could not load cursor theme.\n", stderr);
		goto error;
	}

	struct wl_cursor *wl_cursor = wl_cursor_theme_get_cursor(cursor->theme,
			data->config.cursor_name);
	if ( wl_cursor == NULL )
	{
		fprintf(stderr, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n",
				data->config.cursor_name);
		goto error;
	}

	cursor->image = wl_cursor->images[0];
	assert(cursor->image);

	if ( NULL == (cursor->surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Could not create cursor surface.\n", stderr);
		goto error;
	}

	return true;

error:
	finish_cursor(data);
	return false;
}

void finish_cursor (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Finish cursor.\n", stderr);

	struct Lava_cursor *cursor = &data->cursor;

	/* cursor.image just points back to cursor.theme. */
	if ( cursor->theme != NULL )
	{
		wl_cursor_theme_destroy(cursor->theme);
		cursor->theme = NULL;
	}
	if ( cursor->surface != NULL )
	{
		wl_surface_destroy(cursor->surface);
		cursor->surface = NULL;
	}
}

void attach_cursor (struct Lava_data *data, struct wl_pointer *wl_pointer,
		uint32_t serial)
{
	struct Lava_cursor *cursor = &data->cursor;

	if ( cursor->surface == NULL )
		return;

	if (data->verbose)
		fputs("Attaching cursor surface.\n", stderr);

	// TODO: Find out whether the surface damaging / commiting could be done
	//       once globally instead of on each indivual enter event.
	wl_surface_attach(cursor->surface,
			wl_cursor_image_get_buffer(cursor->image), 0, 0);
	wl_surface_damage(cursor->surface, 1, 0,
			cursor->image->width, cursor->image->height);
	wl_surface_commit(cursor->surface);
	wl_pointer_set_cursor(wl_pointer, serial, cursor->surface,
			cursor->image->hotspot_x,
			cursor->image->hotspot_y);
}

