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
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"bar/layersurface.h"
#include"bar/render.h"

bool create_bar (struct Lava_bar_pattern *pattern, struct Lava_output *output)
{
	struct Lava_data *data = pattern->data;
	if (data->verbose)
		fputs("Creating bar.\n", stderr);

	struct Lava_bar *bar = calloc(1, sizeof(struct Lava_bar));
	if ( bar == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return false;
	}
	wl_list_insert(&output->bars, &bar->link);
	bar->data          = data;
	bar->pattern       = pattern;
	bar->output        = output;
	bar->wl_surface    = NULL;
	bar->layer_surface = NULL;

	if ( NULL == (bar->wl_surface = wl_compositor_create_surface(data->compositor)) )
	{
		fputs("ERROR: Compositor did not create wl_surface.\n", stderr);
		return false;
	}

	if ( NULL == (bar->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
					data->layer_shell, bar->wl_surface,
					output->wl_output, pattern->layer,
					"LavaLauncher")) )
	{
		fputs("ERROR: Compositor did not create layer_surface.\n", stderr);
		return false;
	}

	configure_layer_surface(bar);
	zwlr_layer_surface_v1_add_listener(bar->layer_surface,
			&layer_surface_listener, bar);
	wl_surface_commit(bar->wl_surface);

	return true;
}

static void destroy_bar (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	if ( bar->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(bar->layer_surface);
	if ( bar->wl_surface != NULL )
		wl_surface_destroy(bar->wl_surface);
	wl_list_remove(&bar->link);
	free(bar);
}

void destroy_all_bars (struct Lava_output *output)
{
	if (output->data->verbose)
		fputs("Destroying bars.\n", stderr);
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(b1, b2, &output->bars, link)
		destroy_bar(b1);
}

static void update_offset (struct Lava_bar *bar)
{
	if ( bar == NULL )
		return;

	struct Lava_data        *data    = bar->data;
	struct Lava_bar_pattern *pattern = bar->pattern;
	struct Lava_output      *output  = bar->output;

	if ( output->w == 0 || output->h == 0 )
		return;

	if ( pattern->mode == MODE_SIMPLE )
	{
		bar->x_offset = bar->y_offset = 0;
		return;
	}

	switch (pattern->alignment)
	{
		case ALIGNMENT_START:
			bar->x_offset = 0;
			bar->y_offset = 0;
			break;

		case ALIGNMENT_CENTER:
			if ( pattern->orientation == ORIENTATION_HORIZONTAL )
			{
				bar->x_offset = (output->w / 2) - (pattern->w / 2);
				bar->y_offset = 0;
			}
			else
			{
				bar->x_offset = 0;
				bar->y_offset = (output->h / 2) - (pattern->h / 2);
			}
			break;

		case ALIGNMENT_END:
			if ( pattern->orientation == ORIENTATION_HORIZONTAL )
			{
				bar->x_offset = output->w  - pattern->w;
				bar->y_offset = 0;
			}
			else
			{
				bar->x_offset = 0;
				bar->y_offset = output->h - pattern->h;
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
	if ( pattern->orientation == ORIENTATION_HORIZONTAL )
		bar->x_offset += pattern->margin_left - pattern->margin_right;
	else
		bar->y_offset += pattern->margin_top - pattern->margin_bottom;

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

void update_all_bars (struct Lava_output *output)
{
	if (wl_list_empty(&output->bars))
		return;
	if (output->data->verbose)
		fprintf(stderr, "Updating all bars on output: name=%d\n",
				output->global_name);
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(b1, b2, &output->bars, link)
		update_bar(b1);
}

struct Lava_bar *bar_from_surface (struct Lava_data *data, struct wl_surface *surface)
{
	if ( data == NULL || surface == NULL )
		return NULL;
	struct Lava_output *op1, *op2;
	struct Lava_bar *b1, *b2;
	wl_list_for_each_safe(op1, op2, &data->outputs, link)
	{
		wl_list_for_each_safe(b1, b2, &op1->bars, link)
		{
			if ( b1->wl_surface == surface )
				return b1;
		}
	}
	return NULL;
}

