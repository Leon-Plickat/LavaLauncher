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
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"river-status-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"str.h"
#include"seat.h"
#include"output.h"

static void registry_handle_global (void *raw_data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;

	if (! strcmp(interface, wl_compositor_interface.name))
	{
		log_message(data, 2, "[registry] Get wl_compositor.\n");
		data->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	}
	if (! strcmp(interface, wl_subcompositor_interface.name))
	{
		log_message(data, 2, "[registry] Get wl_subcompositor.\n");
		data->subcompositor = wl_registry_bind(registry, name,
				&wl_subcompositor_interface, 1);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		log_message(data, 2, "[registry] Get wl_shm.\n");
		data->shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		log_message(data, 2, "[registry] Get zwlr_layer_shell_v1.\n");
		data->layer_shell = wl_registry_bind(registry, name,
				&zwlr_layer_shell_v1_interface, 1);
	}
	else if (! strcmp(interface, zxdg_output_manager_v1_interface.name))
	{
		log_message(data, 2, "[registry] Get zxdg_output_manager_v1.\n");
		data->xdg_output_manager = wl_registry_bind(registry, name,
				&zxdg_output_manager_v1_interface, 3);
	}
	else if (! strcmp(interface, wl_seat_interface.name))
	{
		if (! create_seat(data, registry, name, interface, version))
			goto error;
	}
	else if (! strcmp(interface, wl_output_interface.name))
	{
		if (! create_output(data, registry, name, interface, version))
			goto error;
	}
	else if (! strcmp(interface, zriver_status_manager_v1_interface.name))
	{
		if (data->need_river_status)
			data->river_status_manager = wl_registry_bind(registry, name,
				&zriver_status_manager_v1_interface, 1);
	}

	return;
error:
	data->loop = false;
	data->ret  = EXIT_FAILURE;
}

static void registry_handle_global_remove (void *raw_data,
		struct wl_registry *registry, uint32_t name)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;
	log_message(data, 1, "[registry] Global remove.\n");
	destroy_output(get_output_from_global_name(data, name));
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

/* Test if all required interfaces are available. If not, return the name of the
 * missing interface.
 */
static char *check_for_required_interfaces (struct Lava_data *data)
{
	if ( data->compositor == NULL )
		return "wl_compositor";
	if ( data->subcompositor == NULL )
		return "wl_subcompositor";
	if ( data->shm == NULL )
		return "wl_shm";
	if ( data->layer_shell == NULL )
		return "zwlr_layershell_v1";
	if ( data->xdg_output_manager == NULL )
		return "zxdg_output_manager";

	if ( data->need_river_status && data->river_status_manager == NULL )
		return "zriver_status_v1";

	return NULL;
}

bool init_wayland (struct Lava_data *data)
{
	log_message(data, 1, "[registry] Init Wayland.\n");

	/* Connect to Wayland server. */
	log_message(data, 2, "[registry] Connecting to server.\n");
	if ( NULL == (data->display = wl_display_connect(NULL)) )
	{
		log_message(NULL, 0, "ERROR: Can not connect to a Wayland server.\n");
		return false;
	}

	/* Get registry and add listeners. */
	log_message(data, 2, "[registry] Get wl_registry.\n");
	if ( NULL == (data->registry = wl_display_get_registry(data->display)) )
	{
		log_message(NULL, 0, "ERROR: Can not get registry.\n");
		return false;
	}
	wl_registry_add_listener(data->registry, &registry_listener, data);

	/* Allow registry listeners to catch up. */
	if ( wl_display_roundtrip(data->display) == -1 )
	{
		log_message(NULL, 0, "ERROR: Roundtrip failed.\n");
		return false;
	}

	const char *missing = check_for_required_interfaces(data);
	if ( missing != NULL )
	{
		log_message(NULL, 0, "ERROR: Wayland compositor does not support %s.\n", missing);
		return false;
	}

	/* Configure all outputs that were created before xdg_output_manager or
	 * the layer_shell were available.
	 */
	log_message(data, 2, "[registry] Catching up on output configuration.\n");
	struct Lava_output *op, *temp;
	wl_list_for_each_safe(op, temp, &data->outputs, link)
		if ( op->status == OUTPUT_STATUS_UNCONFIGURED )
			if (! configure_output(op))
				return false;

	return true;
}

/* Finish him! */
void finish_wayland (struct Lava_data *data)
{
	log_message(data, 1, "[registry] Finish Wayland.\n");

	destroy_all_outputs(data);
	destroy_all_seats(data);

	log_message(data, 2, "[registry] Destroying Wayland objects.\n");

	DESTROY(data->layer_shell, zwlr_layer_shell_v1_destroy);
	DESTROY(data->compositor, wl_compositor_destroy);
	DESTROY(data->subcompositor, wl_subcompositor_destroy);
	DESTROY(data->shm, wl_shm_destroy);
	DESTROY(data->registry, wl_registry_destroy);

	DESTROY(data->river_status_manager, zriver_status_manager_v1_destroy);

	if ( data->display != NULL )
	{
		log_message(data, 2, "[registry] Diconnecting from server.\n");
		wl_display_disconnect(data->display);
	}
}

