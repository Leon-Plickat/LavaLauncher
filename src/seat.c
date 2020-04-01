#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"input.h"

/* No-Op function. */
static void noop () {}

static void seat_handle_capabilities (void *raw_data, struct wl_seat *wl_seat,
		uint32_t capabilities)
{
	struct Lava_seat *seat = (struct Lava_seat *)raw_data;
	struct Lava_data *data = seat->data;

	if (data->verbose)
		fputs("Handling seat capabilities.\n", stderr);

	if ( seat->pointer.wl_pointer != NULL )
	{
		wl_pointer_release(seat->pointer.wl_pointer);
		seat->pointer.wl_pointer = NULL;
	}

	if ( capabilities & WL_SEAT_CAPABILITY_POINTER )
	{
		if (data->verbose)
			fputs("Seat has pointer capabilities.\n", stderr);
		seat->pointer.wl_pointer = wl_seat_get_pointer(wl_seat);
		wl_pointer_add_listener(seat->pointer.wl_pointer,
				&pointer_listener, seat);
	}

	if ( capabilities & WL_SEAT_CAPABILITY_TOUCH )
	{
		if (data->verbose)
			fputs("Seat has touch capabilities.\n", stderr);
		seat->touch.wl_touch = wl_seat_get_touch(wl_seat);
		wl_touch_add_listener(seat->touch.wl_touch,
				&touch_listener, seat);
	}
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
	.name         = noop
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (data->verbose)
		fputs("Adding seat.\n", stderr);

	struct wl_seat *wl_seat = wl_registry_bind(registry, name,
			&wl_seat_interface, 3);
	struct Lava_seat *seat = calloc(1, sizeof(struct Lava_seat));
	if ( seat == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		data->loop = false;
		data->ret  = EXIT_FAILURE;
		return false;
	}

	seat->data    = data;
	seat->wl_seat = wl_seat;
	wl_list_insert(&data->seats, &seat->link);
	wl_seat_add_listener(wl_seat, &seat_listener, seat);

	return true;
}
