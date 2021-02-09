/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 - 2021 Leon Henrik Plickat
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
#include<poll.h>
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
#include"event-loop.h"


/**************
 *            *
 *  Registry  *
 *            *
 **************/
static void registry_handle_global (void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (! strcmp(interface, wl_compositor_interface.name))
	{
		log_message(2, "[registry] Get wl_compositor.\n");
		context.compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	}
	if (! strcmp(interface, wl_subcompositor_interface.name))
	{
		log_message(2, "[registry] Get wl_subcompositor.\n");
		context.subcompositor = wl_registry_bind(registry, name,
				&wl_subcompositor_interface, 1);
	}
	else if (! strcmp(interface, wl_shm_interface.name))
	{
		log_message(2, "[registry] Get wl_shm.\n");
		context.shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	}
	else if (! strcmp(interface, zwlr_layer_shell_v1_interface.name))
	{
		log_message(2, "[registry] Get zwlr_layer_shell_v1.\n");
		context.layer_shell = wl_registry_bind(registry, name,
				&zwlr_layer_shell_v1_interface, 1);
	}
	else if (! strcmp(interface, zxdg_output_manager_v1_interface.name))
	{
		log_message(2, "[registry] Get zxdg_output_manager_v1.\n");
		context.xdg_output_manager = wl_registry_bind(registry, name,
				&zxdg_output_manager_v1_interface, 3);
	}
	else if (! strcmp(interface, wl_seat_interface.name))
	{
		if (! create_seat( registry, name, interface, version))
			goto error;
	}
	else if (! strcmp(interface, wl_output_interface.name))
	{
		if (! create_output(registry, name, interface, version))
			goto error;
	}
	else if (! strcmp(interface, zriver_status_manager_v1_interface.name))
	{
		if (context.need_river_status)
			context.river_status_manager = wl_registry_bind(registry, name,
				&zriver_status_manager_v1_interface, 1);
	}

	return;
error:
	context.loop = false;
	context.ret  = EXIT_FAILURE;
}

static void registry_handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
	log_message(1, "[registry] Global remove.\n");
	destroy_output(get_output_from_global_name(name));
}

static const struct wl_registry_listener registry_listener = {
	.global        = registry_handle_global,
	.global_remove = registry_handle_global_remove
};

/* Test if all required interfaces are available. If not, return the name of the
 * missing interface.
 */
static char *check_for_required_interfaces (void)
{
	if ( context.compositor == NULL )
		return "wl_compositor";
	if ( context.subcompositor == NULL )
		return "wl_subcompositor";
	if ( context.shm == NULL )
		return "wl_shm";
	if ( context.layer_shell == NULL )
		return "zwlr_layershell_v1";
	if ( context.xdg_output_manager == NULL )
		return "zxdg_output_manager";

	if ( context.need_river_status && context.river_status_manager == NULL )
		return "zriver_status_v1";

	return NULL;
}

/****************
 *              *
 *  Connection  *
 *              *
 ****************/
static bool init_wayland (void)
{
	log_message(1, "[registry] Init Wayland.\n");

	/* Connect to Wayland server. */ // TODO does the display really need to be global?
	log_message(2, "[registry] Connecting to server.\n");
	if ( NULL == (context.display = wl_display_connect(NULL)) )
	{
		log_message(0, "ERROR: Can not connect to a Wayland server.\n");
		return false;
	}

	/* Get registry and add listeners. */ // TODO does registry really need to be global?
	log_message(2, "[registry] Get wl_registry.\n");
	if ( NULL == (context.registry = wl_display_get_registry(context.display)) )
	{
		log_message(0, "ERROR: Can not get registry.\n");
		return false;
	}
	wl_registry_add_listener(context.registry, &registry_listener, NULL);

	/* Allow registry listeners to catch up. */
	if ( wl_display_roundtrip(context.display) == -1 )
	{
		log_message(0, "ERROR: Roundtrip failed.\n");
		return false;
	}

	const char *missing = check_for_required_interfaces();
	if ( missing != NULL )
	{
		log_message(0, "ERROR: Wayland compositor does not support %s.\n", missing);
		return false;
	}

	/* Configure all outputs that were created before xdg_output_manager or
	 * the layer_shell were available.
	 */
	log_message(2, "[registry] Catching up on output configuration.\n");
	struct Lava_output *op, *temp;
	wl_list_for_each_safe(op, temp, &context.outputs, link)
		if ( op->status == OUTPUT_STATUS_UNCONFIGURED )
			if (! configure_output(op))
				return false;

	return true;
}

/* Finish him! */
static void finish_wayland (void)
{
	log_message(1, "[registry] Finish Wayland.\n");

	destroy_all_outputs();
	destroy_all_seats();

	log_message(2, "[registry] Destroying Wayland objects.\n");

	DESTROY(context.layer_shell, zwlr_layer_shell_v1_destroy);
	DESTROY(context.compositor, wl_compositor_destroy);
	DESTROY(context.subcompositor, wl_subcompositor_destroy);
	DESTROY(context.shm, wl_shm_destroy);
	DESTROY(context.registry, wl_registry_destroy);

	DESTROY(context.river_status_manager, zriver_status_manager_v1_destroy);

	if ( context.display != NULL )
	{
		log_message(2, "[registry] Diconnecting from server.\n");
		wl_display_disconnect(context.display);
	}
}

/**************************
 *                        *
 *  Wayland event source  *
 *                        *
 **************************/
static bool wayland_source_init (struct pollfd *fd)
{
	log_message(1, "[loop] Setting up Wayland event source.\n");

	if (! init_wayland())
		return false;

	fd->events = POLLIN;
	if ( -1 == (fd->fd = wl_display_get_fd(context.display)) )
	{
		log_message(0, "ERROR: Unable to open Wayland display fd.\n");
		return false;
	}
	return true;
}

static bool wayland_source_finish (struct pollfd *fd)
{
	if ( fd->fd != -1 )
		close(fd->fd);
	finish_wayland();
	return true;
}

static bool wayland_source_flush (struct pollfd *fd)
{
	do {
		if ( wl_display_flush(context.display) == 1 && errno != EAGAIN )
		{
			log_message(0, "ERROR: wl_display_flush: %s\n",
					strerror(errno));
			break;
		}
	} while ( errno == EAGAIN );
	return true;
}

static bool wayland_source_handle_in (struct pollfd *fd)
{
	if ( wl_display_dispatch(context.display) == -1 )
	{
		log_message(0, "ERROR: wl_display_dispatch: %s\n", strerror(errno));
		return false;
	}
	return true;
}

static bool wayland_source_handle_out (struct pollfd *fd)
{
	if ( wl_display_flush(context.display) == -1 )
	{
		log_message(0, "ERROR: wl_display_flush: %s\n", strerror(errno));
		return false;
	}
	return true;
}

struct Lava_event_source wayland_source = {
	.init       = wayland_source_init,
	.finish     = wayland_source_finish,
	.flush      = wayland_source_flush,
	.handle_in  = wayland_source_handle_in,
	.handle_out = wayland_source_handle_out
};

