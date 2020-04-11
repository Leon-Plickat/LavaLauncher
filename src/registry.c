/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"seat.h"
#include"output.h"

static void registry_handle_global (void *raw_data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;

	if (! strcmp(interface, wl_compositor_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_compositor.\n", stderr);
		data->compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		if (data->verbose)
			fputs("Get wl_shm.\n", stderr);
		data->shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		if (data->verbose)
			fputs("Get zwlr_layer_shell_v1.\n", stderr);
		data->layer_shell = wl_registry_bind(registry, name,
				&zwlr_layer_shell_v1_interface, 1);
	}
	else if (! strcmp(interface, zxdg_output_manager_v1_interface.name))
	{
		if (data->verbose)
			fputs("Get zxdg_output_manager_v1.\n", stderr);
		data->xdg_output_manager = wl_registry_bind(registry, name,
				&zxdg_output_manager_v1_interface, 2);
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

	return;
error:
	data->loop = false;
	data->ret  = EXIT_FAILURE;
}

static void registry_handle_global_remove (void *raw_data,
		struct wl_registry *registry, uint32_t name)
{
	struct Lava_data   *data = (struct Lava_data *)raw_data;
	struct Lava_output *op1, *op2;

	if (data->verbose)
		fputs("Global remove.\n", stderr);

	wl_list_for_each_safe(op1, op2, &data->outputs, link)
	{
		if ( op1->global_name == name )
		{
			wl_list_remove(&op1->link);
			wl_output_destroy(op1->wl_output);
			if ( op1->layer_surface != NULL )
				zwlr_layer_surface_v1_destroy(op1->layer_surface);
			if ( op1->wl_surface != NULL )
				wl_surface_destroy(op1->wl_surface);
			free(op1->name);
			free(op1);
			break;
		}
	}
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

/* Helper function for capability support error message. */
static bool capability_test (void *ptr, const char *name)
{
	if ( ptr == NULL )
	{
		fprintf(stderr, "ERROR: Wayland compositor does not support %s.\n", name);
		return false;
	}
	return true;
}

bool init_wayland (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Init Wayland.\n", stderr);

	/* Init lists. */
	wl_list_init(&data->outputs);
	wl_list_init(&data->seats);

	/* Connect to Wayland server. */
	if (data->verbose)
		fputs("Connecting to server.\n", stderr);
	const char *wayland_display = getenv("WAYLAND_DISPLAY");
	data->display = wl_display_connect(wayland_display);
	if ( data->display == NULL || wayland_display == NULL )
	{
		fputs("ERROR: Can not connect to a Wayland server.\n", stderr);
		return false;
	}

	/* Get registry and add listeners. */
	if (data->verbose)
		fputs("Get wl_registry.\n", stderr);
	data->registry = wl_display_get_registry(data->display);
	if ( data->registry == NULL )
	{
		fputs("ERROR: Can not get registry.\n", stderr);
		return false;
	}
	wl_registry_add_listener(data->registry, &registry_listener, data);

	/* Allow registry listeners to catch up. */
	if ( wl_display_roundtrip(data->display) == -1 )
	{
		fputs("ERROR: Roundtrip failed.\n", stderr);
		return false;
	}

	/* Testing compatibilities. */
	if (! capability_test(data->compositor, "wl_compositor"))
		return false;
	if (! capability_test(data->shm, "wl_shm"))
		return false;
	if (! capability_test(data->layer_shell, "zwlr_layer_shell"))
		return false;

	return true;
}

/* Finish him! */
void finish_wayland (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Finish Wayland.\nDestroying outputs.\n", stderr);

	struct Lava_output *op_1, *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
	{
		wl_list_remove(&op_1->link);
		wl_output_destroy(op_1->wl_output);
		if ( op_1->layer_surface != NULL )
			zwlr_layer_surface_v1_destroy(op_1->layer_surface);
		if ( op_1->wl_surface != NULL )
			wl_surface_destroy(op_1->wl_surface);
		free(op_1->name);
		free(op_1);
	}

	if (data->verbose)
		fputs("Destroying seats.\n", stderr);
	struct Lava_seat *st_1, *st_2;
	wl_list_for_each_safe(st_1, st_2, &data->seats, link)
	{
		wl_list_remove(&st_1->link);
		wl_seat_release(st_1->wl_seat);
		if (st_1->pointer.wl_pointer)
			wl_pointer_release(st_1->pointer.wl_pointer);
		free(st_1);
	}

	if (data->verbose)
		fputs("Destroying Wayland objects.\n", stderr);
	if ( data->layer_shell != NULL )
		zwlr_layer_shell_v1_destroy(data->layer_shell);
	if ( data->compositor != NULL )
		wl_compositor_destroy(data->compositor);
	if ( data->shm != NULL )
		wl_shm_destroy(data->shm);
	if ( data->registry != NULL )
		wl_registry_destroy(data->registry);

	if ( data->display != NULL )
	{
		if (data->verbose)
			fputs("Diconnecting from server.\n", stderr);
		wl_display_disconnect(data->display);
	}
}

