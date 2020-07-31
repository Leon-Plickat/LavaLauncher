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
#include<unistd.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-foreign-toplevel-management-unstable-v1-protocol.h"

#include"lavalauncher.h"
#include"toplevel.h"
#include"log.h"

/* Returns amount of toplevels with the given app_id. */
int32_t count_toplevels_with_appid (struct Lava_data *data, const char *app_id)
{
	if ( data->toplevel_manager == NULL )
		return -1;
	if ( *app_id == '\0' )
		return -2;

	int32_t ret = 0;
	struct Lava_toplevel *toplevel;
	wl_list_for_each(toplevel, &data->toplevels, link)
		if (! strcmp(app_id, toplevel->current_app_id))
		{
			log_message(NULL, 0, "found one.\n");
			ret++;
		}

	return ret;
}

/* Returns the first (meaning the oldest) toplevel with the given app-id. */
struct Lava_toplevel *toplevel_from_app_id (struct Lava_data *data, const char *app_id)
{
	if ( data->toplevel_manager == NULL || *app_id == '\0' )
		return NULL;

	struct Lava_toplevel *toplevel;
	wl_list_for_each(toplevel, &data->toplevels, link)
		if (! strcmp(app_id, toplevel->current_app_id))
			return toplevel;

	return NULL;
}

/* No-Op function. */
static void noop () {}

static void destroy_toplevel (struct Lava_toplevel *toplevel)
{
	wl_list_remove(&toplevel->link);
        zwlr_foreign_toplevel_handle_v1_destroy(toplevel->toplevel);
	free(toplevel);
}

void destroy_all_toplevels (struct Lava_data *data)
{
	log_message(data, 1, "[toplevel] Destroying all toplevels.\n");
	struct Lava_toplevel *toplevel, *temp;
	wl_list_for_each_safe(toplevel, temp, &data->toplevels, link)
		destroy_toplevel(toplevel);
}

static void toplevel_handle_app_id (void *raw, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel,
		const char *app_id)
{
        struct Lava_toplevel *toplevel = (struct Lava_toplevel *)raw;
	log_message(toplevel->data, 2, "[toplevel] Toplevel update app_id.\n");
	strncpy(toplevel->pending_app_id, app_id, sizeof(toplevel->pending_app_id) - 1);
}

static void toplevel_handle_done (void *raw, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel)
{
        struct Lava_toplevel *toplevel = (struct Lava_toplevel *)raw;
	if (strcmp(toplevel->current_app_id, toplevel->pending_app_id))
	{
		log_message(toplevel->data, 2, "[toplevel] Toplevel update current state.\n");
		strncpy(toplevel->current_app_id, toplevel->pending_app_id,
				sizeof(toplevel->current_app_id));
	}
}

static void toplevel_handle_closed (void *raw, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel)
{
        struct Lava_toplevel *toplevel = (struct Lava_toplevel *)raw;
	log_message(toplevel->data, 2, "[toplevel] Toplevel closed.\n");
	destroy_toplevel(toplevel);
}

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_listener = {
        .title = noop,
        .app_id = toplevel_handle_app_id,
        .output_enter = noop,
        .output_leave = noop,
        .state = noop,
        .done = toplevel_handle_done,
        .closed = toplevel_handle_closed,
};

static void toplevel_manager_handle_toplevel (void *raw,
		struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager,
		struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel)
{
	struct Lava_data     *data     = (struct Lava_data *)raw;
	log_message(data, 2, "[toplevel] New toplevel.\n");

	struct Lava_toplevel *toplevel = calloc(1, sizeof(struct Lava_toplevel));
	if ( toplevel == NULL )
	{
		log_message(NULL, 0, "ERROR: Can not allocate.\n");
		data->loop = false;
		data->ret  = EXIT_FAILURE;
		return;
	}

	memset(toplevel->current_app_id, '\0', sizeof(toplevel->current_app_id));
	memset(toplevel->pending_app_id, '\0', sizeof(toplevel->pending_app_id));

	toplevel->data     = data;
	toplevel->toplevel = zwlr_toplevel;
	wl_list_insert(&data->toplevels, &toplevel->link);
	zwlr_foreign_toplevel_handle_v1_add_listener(zwlr_toplevel,
			&toplevel_listener, toplevel);
}

static void toplevel_manager_handle_finished (void *raw,
		struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager)
{
	struct Lava_data *data = (struct Lava_data *)raw;
	zwlr_foreign_toplevel_manager_v1_destroy(toplevel_manager);
	destroy_all_toplevels(data);
}

static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
	.toplevel = toplevel_manager_handle_toplevel,
	.finished = toplevel_manager_handle_finished
};

bool create_toplevel_manager (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name)
{
	if ( NULL == (data->toplevel_manager = wl_registry_bind(registry, name,
			&zwlr_foreign_toplevel_manager_v1_interface, 2)) )
	{
		log_message(NULL, 0, "ERROR: Can not bind toplevel.\n");
		return false;
	}
	zwlr_foreign_toplevel_manager_v1_add_listener(data->toplevel_manager,
			&toplevel_manager_listener, data);
	return true;
}
