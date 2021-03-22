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

#include<wayland-client.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"river-status-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"util.h"
#include"seat.h"
#include"output.h"
#include"event-loop.h"
#include"foreign-toplevel-management.h"


/**************
 *            *
 *  Registry  *
 *            *
 **************/
static void registry_handle_global (void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if ( strcmp(interface, wl_compositor_interface.name) == 0 )
	{
		log_message(2, "[registry] Get wl_compositor.\n");
		context.compositor = wl_registry_bind(registry, name,
				&wl_compositor_interface, 4);
	}
	else if ( strcmp(interface, wl_shm_interface.name) == 0 )
	{
		log_message(2, "[registry] Get wl_shm.\n");
		context.shm = wl_registry_bind(registry, name,
				&wl_shm_interface, 1);
	}
	else if ( strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0 )
	{
		log_message(2, "[registry] Get zwlr_layer_shell_v1.\n");
		context.layer_shell = wl_registry_bind(registry, name,
				&zwlr_layer_shell_v1_interface, 1);
	}
	else if ( strcmp(interface, zxdg_output_manager_v1_interface.name) == 0 )
	{
		log_message(2, "[registry] Get zxdg_output_manager_v1.\n");
		context.xdg_output_manager = wl_registry_bind(registry, name,
				&zxdg_output_manager_v1_interface, 3);
	}
	else if ( strcmp(interface, wl_seat_interface.name) == 0 )
	{
		if (! create_seat( registry, name, interface, version))
			goto error;
	}
	else if ( strcmp(interface, wl_output_interface.name) == 0 )
	{
		struct wl_output *wl_output = wl_registry_bind(registry, name,
			&wl_output_interface, version);
		if (! create_output(registry, name, wl_output))
			goto error;
	}
	else if ( strcmp(interface, zriver_status_manager_v1_interface.name) == 0)
	{
		if (context.need_river_status)
			context.river_status_manager = wl_registry_bind(registry, name,
				&zriver_status_manager_v1_interface, 1);
	}
	else if ( strcmp(interface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0 )
	{
		/* Binding the foreign_toplevel_manager in the initial registry
		 * burst means that it might be bound before the outputs, which
		 * causes the toplevel.output_enter event to not be send, but
		 * we don't actually use that, so whatever.
		 */
		if (context.need_foreign_toplevel)
		{
			context.foreign_toplevel_manager = wl_registry_bind(context.registry,
					name, &zwlr_foreign_toplevel_manager_v1_interface, version);
			init_foreign_toplevel_management();
		}
	}

	return;
error:
	context.loop = false;
	context.ret  = EXIT_FAILURE;
}

static void registry_handle_global_remove (void *data, struct wl_registry *registry, uint32_t name)
{
	log_message(1, "[registry] Global remove.\n");

	/* Wayland allows binding multiple objects to the same name, but since
	 * we do not do that in LavaLauncher, we can safely return after
	 * destroying the first object we find.
	 */

	struct Lava_output *output = get_output_from_global_name(name);
	if ( output != NULL )
	{
		destroy_output(output);
		return;
	}

	struct Lava_seat *seat = get_seat_from_global_name(name);
	if ( seat != NULL )
	{
		destroy_seat(seat);
		return;
	}
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
	if ( context.shm == NULL )
		return "wl_shm";
	if ( context.layer_shell == NULL )
		return "wlr_layershell_v1";
	if ( context.xdg_output_manager == NULL )
		return "xdg_output_manager";

	if ( context.need_river_status && context.river_status_manager == NULL )
		return "river_status_v1";
	if ( context.need_foreign_toplevel && context.foreign_toplevel_manager == NULL )
		return "wlr_foreign_toplevel_management_v1";

	return NULL;
}

static void sync_handle_done (void *data, struct wl_callback *wl_callback, uint32_t other_data)
{
	log_message(1, "[registry:sync] Initial registry advertising is done.\n");
	wl_callback_destroy(wl_callback);
	context.sync = NULL;

	/* Initial burst of registry advertising is done, check if everything we
	 * need is available.
	 */
	const char *missing = check_for_required_interfaces();
	if ( missing != NULL )
	{
		log_message(0, "ERROR: Wayland compositor does not support %s.\n", missing);
		context.loop = false;
		context.ret  = EXIT_FAILURE;
		return;
	}

	/* Configure all outputs that were created before xdg_output_manager or
	 * the layer_shell were available.
	 */
	log_message(2, "[registry:sync] Catching up on output configuration.\n");
	struct Lava_output *op;
	wl_list_for_each(op, &context.outputs, link)
		if ( op->status == OUTPUT_STATUS_UNCONFIGURED )
			configure_output(op);

}

static const struct wl_callback_listener sync_callback_listener = {
	.done = sync_handle_done,
};

/****************
 *              *
 *  Connection  *
 *              *
 ****************/
static bool init_wayland (void)
{
	log_message(1, "[registry] Init Wayland.\n");

	/* We query the display name here instead of letting wl_display_connect()
	 * figure it out itself, because libwayland (for legacy reasons) falls
	 * back to using "wayland-0" when $WAYLAND_DISPLAY is not set, which is
	 * generally not desirable.
	 */
	const char *display_name = getenv("WAYLAND_DISPLAY");
	if ( display_name == NULL )
	{
		log_message(0, "ERROR: WAYLAND_DISPLAY is not set.\n");
		return false;
	}

	/* Connect to Wayland server. */
	log_message(2, "[registry] Connecting to server.\n");
	context.display = wl_display_connect(display_name);
	if ( context.display == NULL )
	{
		log_message(0, "ERROR: Can not connect to a Wayland server.\n");
		return false;
	}

	/* Get registry and add listeners. */
	log_message(2, "[registry] Get wl_registry.\n");
	context.registry = wl_display_get_registry(context.display);
	wl_registry_add_listener(context.registry, &registry_listener, NULL);

	context.sync = wl_display_sync(context.display);
	wl_callback_add_listener(context.sync, &sync_callback_listener, NULL);

	return true;
}

/* Finish him! */
static void finish_wayland (void)
{
	if ( context.display == NULL )
		return;

	log_message(1, "[registry] Finish Wayland.\n");

	struct Lava_seat *seat, *stemp;
	wl_list_for_each_safe(seat, stemp, &context.seats, link)
		destroy_seat(seat);

	struct Lava_output *output, *otemp;
	wl_list_for_each_safe(output, otemp, &context.outputs, link)
		destroy_output(output);

	struct Lava_toplevel *toplevel, *ttemp;
	wl_list_for_each_safe(toplevel, ttemp, &context.toplevels, link)
		destroy_toplevel(toplevel);

	log_message(2, "[registry] Destroying Wayland objects.\n");

	DESTROY(context.layer_shell, zwlr_layer_shell_v1_destroy);
	DESTROY(context.compositor, wl_compositor_destroy);
	DESTROY(context.shm, wl_shm_destroy);
	DESTROY(context.registry, wl_registry_destroy);
	DESTROY(context.sync, wl_callback_destroy);
	DESTROY(context.xdg_output_manager, zxdg_output_manager_v1_destroy);

	DESTROY(context.river_status_manager, zriver_status_manager_v1_destroy);
	DESTROY(context.foreign_toplevel_manager, zwlr_foreign_toplevel_manager_v1_destroy);

	log_message(2, "[registry] Diconnecting from server.\n");
	wl_display_disconnect(context.display);
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

