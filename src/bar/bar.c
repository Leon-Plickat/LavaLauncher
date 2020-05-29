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
#include<cairo/cairo.h>
#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"output.h"
#include"draw-generics.h"
#include"bar/bar.h"
#include"bar/layersurface.h"
#include"bar/render.h"

struct Lava_bar *create_bar (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	struct Lava_bar *bar = calloc(1, sizeof(struct Lava_bar));
	if ( bar == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return NULL;
	}

	if ( NULL == (bar->wl_surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Compositor did not create wl_surface.\n", stderr);
		goto error;
	}

	if ( NULL == (bar->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					data->layer_shell, bar->wl_surface,
					output->wl_output, data->layer, "LavaLauncher")) )
	{
		fputs("ERROR: Compositor did not create layer_surface.\n", stderr);
		goto error;
	}

	bar->output = output;
	bar->data   = data;

	configure_layer_surface(bar);
	zwlr_layer_surface_v1_add_listener(bar->layer_surface,
			&layer_surface_listener, bar);
	wl_surface_commit(bar->wl_surface);
	return bar;

error:
	destroy_bar(bar);
	return NULL;
}

void destroy_bar (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	if ( bar->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(bar->layer_surface);
	if ( bar->wl_surface != NULL )
		wl_surface_destroy(bar->wl_surface);
	free(bar);
}

static void update_offset (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_data   *data   = bar->data;
	struct Lava_output *output = bar->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	if ( data->mode == MODE_SIMPLE )
	{
		bar->x_offset = bar->y_offset = 0;
		return;
	}

	switch (data->alignment)
	{
		case ALIGNMENT_START:
			bar->x_offset = 0;
			bar->y_offset = 0;
			break;

		case ALIGNMENT_CENTER:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				bar->x_offset = (output->w / 2) - (data->w / 2);
				bar->y_offset = 0;
			}
			else
			{
				bar->x_offset = 0;
				bar->y_offset = (output->h / 2) - (data->h / 2);
			}
			break;

		case ALIGNMENT_END:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				bar->x_offset = output->w  - data->w;
				bar->y_offset = 0;
			}
			else
			{
				bar->x_offset = 0;
				bar->y_offset = output->h - data->h;
			}
			break;
	}

	/* Set margin.
	 * Since we create a surface spanning the entire length of an outputs
	 * edge, margins parallel to it would move it outside the boundaries of
	 * the output, which may or may not cause issues in some compositors.
	 * To work around this, we simply cheat a bit: Margins parallel to the
	 * bar will be simulated in the draw code by adjusting the bar offsets.
	 *
	 * See: configure_layer_surface()
	 *
	 * Here we set the margins parallel to the edges length, which are
	 * "fake", by adjusting the offset of the bar.
	 */
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		bar->x_offset += data->margin_left - data->margin_right;
	else
		bar->y_offset += data->margin_top - data->margin_bottom;

	if (data->verbose)
		fprintf(stderr, "Aligning bar: global_name=%d x-offset=%d y-offset=%d\n",
				bar->output->global_name, bar->x_offset, bar->y_offset);
}

void update_bar (struct Lava_bar *bar)
{
	configure_layer_surface(bar);
	update_offset(bar);
	render_bar_frame(bar);
}


struct Lava_bar *bar_from_surface (struct Lava_data *data, struct wl_surface *surface)
{
	if ( data == NULL || surface == NULL )
		return NULL;
	struct Lava_output *op1, *op2;
	wl_list_for_each_safe(op1, op2, &data->outputs, link)
		if ( op1->bar->wl_surface == surface )
			return op1->bar;
	return NULL;
}

