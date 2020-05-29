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

#include<wayland-cursor.h>

#include"lavalauncher.h"
#include"cursor.h"

struct Lava_cursor *create_cursor (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Creating cursor.\n", stderr);

	struct Lava_cursor *cursor = calloc(1, sizeof(struct Lava_cursor));
	if ( cursor == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return NULL;
	}

	cursor->data = data;

	if ( NULL == (cursor->theme = wl_cursor_theme_load(NULL, 16, data->shm)) )
	{
		fputs("ERROR: Could not load cursor theme.\n", stderr);
		destroy_cursor(cursor);
		return NULL;
	}

	struct wl_cursor *wl_cursor = wl_cursor_theme_get_cursor(cursor->theme,
			data->cursor_name);
	if ( wl_cursor == NULL )
	{
		fprintf(stderr, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n",
				data->cursor_name);
		destroy_cursor(cursor);
		return NULL;
	}

	cursor->image = wl_cursor->images[0];
	assert(cursor->image);

	if ( NULL == (cursor->surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Could not create cursor surface.\n", stderr);
		destroy_cursor(cursor);
		return NULL;
	}

	return cursor;
}

void destroy_cursor (struct Lava_cursor *cursor)
{
	if ( cursor == NULL )
		return;

	if (cursor->data->verbose)
		fputs("Destroying cursor.\n", stderr);

	/* cursor.image just points back to cursor.theme. */
	if ( cursor->theme != NULL )
		wl_cursor_theme_destroy(cursor->theme);
	if ( cursor->surface != NULL )
		wl_surface_destroy(cursor->surface);
	free(cursor);
}

void attach_cursor (struct Lava_cursor *cursor, struct wl_pointer *wl_pointer,
		uint32_t serial)
{
	if ( cursor == NULL )
		return;

	struct Lava_data *data = cursor->data;

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
