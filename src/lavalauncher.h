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

#ifndef LAVALAUNCHER_LAVALAUNCHER_H
#define LAVALAUNCHER_LAVALAUNCHER_H

#include<stdbool.h>
#include<stdint.h>
#include<wayland-server.h>

/* Helper macro to iterate over a struct array. */
#define FOR_ARRAY(A, B) for (size_t B = 0; B < (sizeof(A) / sizeof(A[0])); B++)

/* Helper macro to reduce error handling boiler plate code. */
#define TRY(A) \
	{ \
		if (A)\
			return true; \
		goto error; \
	}

/* Helper macro to try allocate something. */
#define TRY_NEW(A, B, C) \
	A *B = calloc(1, sizeof(A)); \
	if ( B == NULL ) \
	{ \
		log_message(NULL, 0, "ERROR: Can not allocate.\n"); \
		return C; \
	}

/* Helper macro to destroy something if it is not NULL. */
#define DESTROY(A, B) \
	if ( A != NULL ) \
	{ \
		B(A); \
	}

/* Helper macro to destroy something and set it to NULL if it is not NULL. */
#define DESTROY_NULL(A, B) \
	if ( A != NULL ) \
	{ \
		B(A); \
		A = NULL; \
	}

struct Lava_item;
struct Lava_bar;

struct Lava_data
{
	struct wl_display             *display;
	struct wl_registry            *registry;

	/* Wayland interfaces */
	struct wl_compositor          *compositor;
	struct wl_subcompositor       *subcompositor;
	struct wl_shm                 *shm;
	struct zwlr_layer_shell_v1    *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;

	/* Optional Wayland interfaces */
	struct zriver_status_manager_v1 *river_status_manager;
	bool need_river_status;

	/* Which input devices do we need? */
	bool need_keyboard;
	bool need_touch;
	bool need_pointer;

	char *config_path;

	struct wl_list bars;
	struct Lava_bar *last_bar;

	struct wl_list outputs;
	struct wl_list seats;

	bool loop;
	bool reload;
	int  verbosity;
	int  ret;

#ifdef WATCH_CONFIG
	bool watch;
#endif
};

#endif
