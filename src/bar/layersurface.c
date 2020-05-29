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

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"config/config.h"
#include"output.h"
#include"seat.h"
#include"bar/bar.h"

uint32_t get_anchor (struct Lava_config *config)
{
	struct {
		uint32_t singular_anchor;
		uint32_t anchor_triplet;
	} edges[4] = {
		[POSITION_TOP] = {
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP,
			.anchor_triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		},
		[POSITION_RIGHT] = {
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT,
			.anchor_triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
		},
		[POSITION_BOTTOM] = {
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM,
			.anchor_triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT
		},
		[POSITION_LEFT] = {
			.singular_anchor = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT,
			.anchor_triplet  = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP
				| ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
		},
	};

	/* MODE_SIMPLE is only anchored to a single edge; All other modes are
	 * anchored to three edges.
	 */
	if ( config->mode == MODE_SIMPLE )
		return edges[config->position].singular_anchor;
	else
		return edges[config->position].anchor_triplet;
}

void configure_layer_surface (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_data   *data   = bar->data;
	struct Lava_config *config = data->config;
	struct Lava_output *output = bar->output;

	/* It is possible that this function is called by output events before
	 * the bar has been created. This function will return and abort unless
	 * it is called either when a surface configure event has been received
	 * at least once, it is called by a surface configure event or
	 * it is called during the creation of the surface.
	 */
	if ( output->status != OUTPUT_STATUS_SURFACE_CONFIGURED
			&& output->status != OUTPUT_STATUS_CONFIGURED )
		return;
	if (data->verbose)
		fputs("Configuring bar.\n", stderr);

	uint32_t width, height;
	if ( config->mode == MODE_SIMPLE )
		width = config->w, height = config->h;
	else if ( config->orientation == ORIENTATION_HORIZONTAL )
		width = bar->output->w * output->scale, height = config->h;
	else
		width = config->w, height = output->h * output->scale;

	if (data->verbose)
		fprintf(stderr, "Surface size: w=%d h=%d\n", width, height);

	zwlr_layer_surface_v1_set_size(bar->layer_surface, width, height);

	/* Anchor the surface to the correct edge. */
	zwlr_layer_surface_v1_set_anchor(bar->layer_surface, get_anchor(config));

	/* Set margin.
	* Since we create a surface spanning the entire length of an outputs
	* edge, margins parallel to it would move it outside the boundaries of
	* the output, which may or may not cause issues in some compositors.
	* To work around this, we simply cheat a bit: Margins parallel to the
	* bar will be simulated in the draw code by adjusting the bar offsets.
	*
	* See: bar_update_offset()
	*
	* Here we set the margins not parallel to the edges length, which are
	* real layer shell margins.
	*/
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		zwlr_layer_surface_v1_set_margin(bar->layer_surface,
				config->margin_top, 0,
				config->margin_bottom, 0);
	else
		zwlr_layer_surface_v1_set_margin(bar->layer_surface,
				0, config->margin_right,
				0, config->margin_left);

	/* Set exclusive zone to prevent other surfaces from obstructing ours. */
	zwlr_layer_surface_v1_set_exclusive_zone(bar->layer_surface,
			config->exclusive_zone);

	/* Create a region of the visible part of the surface.
	 * Behold: In MODE_DEFAULT, the actual surface is larger than the visible
	 * bar.
	 */
	struct wl_region *region = wl_compositor_create_region(data->compositor);
	if ( config->mode == MODE_DEFAULT )
		wl_region_add(region, bar->x_offset, bar->y_offset,
				config->w, config->h);
	else
		wl_region_add(region, 0, 0, bar->output->w, bar->output->h);

	/* Set input region. This is necessary to prevent the unused parts of
	 * the surface to catch pointer and touch events.
	 */
	wl_surface_set_input_region(bar->wl_surface, region);

	/* If both border and background are opaque, set opaque region. This
	 * will inform the compositor that it does not have to render anything
	 * below the surface.
	 */
	if ( config->bar_colour[3] == 1 && config->border_colour[3] == 1 )
		wl_surface_set_opaque_region(bar->wl_surface, region);

	wl_region_destroy(region);
}


static void layer_surface_handle_configure (void *raw_data,
		struct zwlr_layer_surface_v1 *surface, uint32_t serial,
		uint32_t w, uint32_t h)
{
	struct Lava_bar  *bar  = (struct Lava_bar *)raw_data;
	struct Lava_data *data = bar->data;
	bar->output->status    = OUTPUT_STATUS_SURFACE_CONFIGURED;

	if (data->verbose)
		fprintf(stderr, "Layer surface configure request:"
				" w=%d h=%d serial=%d\n", w, h, serial);

	zwlr_layer_surface_v1_ack_configure(surface, serial);
	update_bar(bar);
}

static void layer_surface_handle_closed (void *data,
		struct zwlr_layer_surface_v1 *surface)
{
	struct Lava_bar *bar = (struct Lava_bar *)data;
	fputs("Layer surface has been closed.\n", stderr);
	bar->data->loop = false;
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
	.configure = layer_surface_handle_configure,
	.closed    = layer_surface_handle_closed
};

