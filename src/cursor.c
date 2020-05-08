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

void attach_cursor_surface (struct Lava_data *data, struct wl_pointer *wl_pointer,
		uint32_t serial)
{
	if ( data->cursor.surface == NULL )
		return;

	if (data->verbose)
		fputs("Attaching cursor surface.\n", stderr);

	// TODO: Find out whether the surface damaging / commiting could be done
	//       once globally instead of on each indivual enter event.
	wl_surface_attach(data->cursor.surface,
			wl_cursor_image_get_buffer(data->cursor.image), 0, 0);
	wl_surface_damage(data->cursor.surface, 1, 0,
			data->cursor.image->width,
			data->cursor.image->height);
	wl_surface_commit(data->cursor.surface);
	wl_pointer_set_cursor(wl_pointer, serial, data->cursor.surface,
			data->cursor.image->hotspot_x,
			data->cursor.image->hotspot_y);
}

bool init_cursor (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Loading cursor theme.\n", stderr);
	data->cursor.theme = wl_cursor_theme_load(NULL, 16, data->shm);
	if ( data->cursor.theme == NULL )
	{
		fputs("ERROR: Could not load cursor theme.\n", stderr);
		return false;
	}

	if (data->verbose)
		fputs("Getting cursor.\n", stderr);
	struct wl_cursor *cursor = wl_cursor_theme_get_cursor(data->cursor.theme,
			data->cursor.name);
	if ( cursor == NULL )
	{
		fprintf(stderr, "WARNING: Could not get cursor \"%s\".\n"
				"         This cursor is likely missing from your cursor theme.\n"
				"         Changing the cursor is disabled.\n",
				data->cursor.name);
		finish_cursor(data);
		data->cursor.theme   = NULL;
		data->cursor.image   = NULL;
		data->cursor.surface = NULL;
		return true;
	}

	if (data->verbose)
		fputs("Getting cursor image.\n", stderr);
	data->cursor.image = cursor->images[0];
	assert(data->cursor.image);

	if (data->verbose)
		fputs("Creating cursor surface.\n", stderr);
	data->cursor.surface = wl_compositor_create_surface(data->compositor);
	if ( data->cursor.surface == NULL )
	{
		fputs("ERROR: Could not create cursor surface.\n", stderr);
		return false;
	}

	return true;
}

void finish_cursor (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Finish cursor.\n", stderr);

	/* cursor.image just points back to cursor.theme. */
	if ( data->cursor.theme != NULL )
		wl_cursor_theme_destroy(data->cursor.theme);
	if ( data->cursor.surface != NULL )
		wl_surface_destroy(data->cursor.surface);
}
