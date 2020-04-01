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
#include"output.h"
#include"seat.h"
#include"draw.h"

/* Helper function to configure the bar surface. Careful: This function does not
 * commit!
 */
void configure_surface (struct Lava_data *data, struct Lava_output *output)
{
	if (! output->configured)
		return;

	zwlr_layer_surface_v1_set_size(output->layer_surface,
			data->w * output->scale, data->h * output->scale);
	zwlr_layer_surface_v1_set_anchor(output->layer_surface, data->anchors);
	zwlr_layer_surface_v1_set_exclusive_zone(output->layer_surface,
			data->exclusive_zone > 0 ? data->exclusive_zone * output->scale : data->exclusive_zone);

	/* Set margin only to the edge the bar is anchored to. */
	switch (data->anchors)
	{
		case ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP:
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					data->margin, 0, 0, 0);
			break;

		case ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT:
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, data->margin, 0, 0);
			break;

		case ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM:
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, 0, data->margin, 0);
			break;

		case ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT:
			zwlr_layer_surface_v1_set_margin(output->layer_surface,
					0, 0, 0, data->margin);
			break;
	}

	/* If both border and background are opaque, set opaque region. This
	 * will inform the compositor that it does not have to render anything
	 * below the surface.
	 */
	if ( data->bar_colour[3] == 1 && data->border_colour[3] == 1 )
	{
		struct wl_region *region = wl_compositor_create_region(data->compositor);
		wl_region_add(region, 0, 0,
				data->w * output->scale, data->h * output->scale);
		wl_surface_set_opaque_region(output->wl_surface, region);
		wl_region_destroy(region);
	}
}

static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	struct Lava_data   *data   = output->data;
	output->configured         = true;

	if (data->verbose)
		fprintf(stderr, "Layer surface configure request:"
				" w=%d h=%d serial=%d\n", w, h, serial);

	zwlr_layer_surface_v1_ack_configure(surface, serial);
	configure_surface(data, output);
	render_bar_frame(data, output);
}

static void layer_surface_handle_closed (void *raw_data,
		struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_data *data = (struct Lava_data *)raw_data;
	fputs("Layer surface has been closed.\n", stderr);
	data->loop = false;
}

static const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

bool create_bar (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	output->wl_surface = wl_compositor_create_surface(data->compositor);
	if ( output->wl_surface == NULL )
	{
		fputs("ERROR: Compositor did not create wl_surface.\n", stderr);
		return false;
	}

	output->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
			data->layer_shell, output->wl_surface,
			output->wl_output, data->layer, "LavaLauncher");
	if ( output->layer_surface == NULL )
	{
		fputs("ERROR: Compositor did not create layer_surface.\n", stderr);
		return false;
	}

	configure_surface(data, output);
	zwlr_layer_surface_v1_add_listener(output->layer_surface,
			&layer_surface_listener, output);
	wl_surface_commit(output->wl_surface);
	return true;
}
