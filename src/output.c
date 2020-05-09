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
#include<assert.h>

#include<wayland-server.h>
#include<wayland-client.h>
#include<wayland-client-protocol.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"output.h"
#include"layersurface.h"
#include"draw.h"

/* No-Op function. */
static void noop () {}

static void update_bar_offset (struct Lava_data *data, struct Lava_output *output)
{
	if ( output->w == 0 || output->h == 0 )
		return;

	if ( data->mode == MODE_SIMPLE )
	{
		output->bar_x_offset = output->bar_y_offset = 0;
		return;
	}

	switch (data->alignment)
	{
		case ALIGNMENT_START:
			output->bar_x_offset = 0;
			output->bar_y_offset = 0;
			break;

		case ALIGNMENT_CENTER:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				output->bar_x_offset = (output->w / 2) - (data->w / 2);
				output->bar_y_offset = 0;
			}
			else
			{
				output->bar_x_offset = 0;
				output->bar_y_offset = (output->h / 2) - (data->h / 2);
			}
			break;

		case ALIGNMENT_END:
			if ( data->orientation == ORIENTATION_HORIZONTAL )
			{
				output->bar_x_offset = output->w  - data->w;
				output->bar_y_offset = 0;
			}
			else
			{
				output->bar_x_offset = 0;
				output->bar_y_offset = output->h - data->h;
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
	 * See: configure_surface()
	 *
	 * Here we set the margins parallel to the edges length, which are
	 * "fake", by adjusting the offset of the bar.
	 */
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		output->bar_x_offset += data->margin_left - data->margin_right;
	else
		output->bar_y_offset += data->margin_top - data->margin_bottom;

	if (data->verbose)
		fprintf(stderr, "Aligning bar: global_name=%d x-offset=%d y-offset=%d\n",
				output->global_name, output->bar_x_offset, output->bar_y_offset);
}

static void output_handle_scale (void *raw_data, struct wl_output *wl_output,
		int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->scale              = factor;

	if (output->data->verbose)
		fprintf(stderr, "Output update scale: global_name=%d scale=%d\n",
				output->global_name, output->scale);

	configure_surface(output->data, output);
	update_bar_offset(output->data, output);
	render_bar_frame(output->data, output);
}

static const struct wl_output_listener output_listener = {
	.scale    = output_handle_scale,
	.geometry = noop,
	.mode     = noop,
	.done     = noop
};

static void xdg_output_handle_name (void *raw_data,
		struct zxdg_output_v1 *xdg_output, const char *name)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->name               = strdup(name);

	if (output->data->verbose)
		fprintf(stderr, "XDG-Output update name: global_name=%d name=%s\n",
				output->global_name, name);
}

static void xdg_output_handle_logical_size (void *raw_data,
		struct zxdg_output_v1 *xdg_output, int32_t w, int32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->w                  = w;
	output->h                  = h;

	if (output->data->verbose)
		fprintf(stderr, "XDG-Output update logical size: global_name=%d "
				"w=%d h=%d\n", output->global_name, w, h);

	update_bar_offset(output->data, output);
	render_bar_frame(output->data, output);
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_size     = xdg_output_handle_logical_size,
	.name             = xdg_output_handle_name,
	.logical_position = noop,
	.description      = noop,
	.done             = noop
};

bool configure_output (struct Lava_data *data, struct Lava_output *output)
{
	if (data->verbose)
		fprintf(stderr, "Configuring output: global_name=%d\n",
				output->global_name);

	/* Create xdg_output and attach listeners. */
	output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
			data->xdg_output_manager, output->wl_output);
	if ( output->xdg_output == NULL )
	{
		fputs("ERROR: Could not get XDG output.\n", stderr);
		return false;
	}
	zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener,
			output);

	/* Roundtrip to allow the output handlers to "catch up", as the outputs
	 * dimensions as well as name are needed for creating the bar.
	 */
	if ( wl_display_roundtrip(data->display) == -1 )
	{
		fputs("ERROR: Roundtrip failed.\n", stderr);
		return false;
	}

	if ( output->w == 0 || output->h == 0 )
	{
		fprintf(stderr, "WARNING: Nonsensical output: global_name=%d\n"
				"         This output has a width and/or height of 0.\n"
				"         This is very likely a bug in your compositor.\n"
				"         LavaLauncher will ignore this output.\n",
				output->global_name);
		goto unused;
	}


	/* If either the name of output equals only_output or if no only_output
	 * has been given, create a surface for this new output.
	 */
	if ( data->only_output == NULL || ! strcmp(output->name, data->only_output) )
	{
		output->status = OUTPUT_STATUS_CONFIGURED;
		if (! create_bar(data, output))
		{
			data->loop = false;
			data->ret  = EXIT_FAILURE;
			return false;
		}
		return true;
	}

unused:
	if (data->verbose)
		fprintf(stderr, "Output will not be used: global_name=%d\n",
				output->global_name);
	output->status = OUTPUT_STATUS_UNUSED;
	return true;
}

bool create_output (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	if (data->verbose)
		fprintf(stderr, "Adding output: global_name=%d\n", name);

	struct wl_output *wl_output = wl_registry_bind(registry, name,
			&wl_output_interface, 3);
	assert(wl_output);

	struct Lava_output *output = calloc(1, sizeof(struct Lava_output));
	if ( output == NULL )
	{
		fputs("ERROR: Could not allocate.\n", stderr);
		return false;
	}

	output->data        = data;
	output->global_name = name;
	output->scale       = 1;
	output->wl_output   = wl_output;
	output->status      = OUTPUT_STATUS_UNCONFIGURED;

	wl_list_insert(&data->outputs, &output->link);
	wl_output_set_user_data(wl_output, output);
	wl_output_add_listener(wl_output, &output_listener, output);

	/* We can only use the output if we have both xdg_output_manager and
	 * the layer_shell. If either one is not available yet, we have to
	 * configure the output later (see init_wayland()).
	 */
	if ( data->xdg_output_manager != NULL && data->layer_shell != NULL )
	{
		if (! configure_output (data, output))
			return false;
	}
	else if (data->verbose)
		fputs("Output not yet configureable.\n", stderr);

	return true;
}

struct Lava_output *get_output_from_global_name (struct Lava_data *data, uint32_t name)
{
	struct Lava_output *op_1, *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
		if ( op_1->global_name == name )
			return op_1;
	return NULL;
}

void destroy_output (struct Lava_output *output)
{
	if ( output == NULL )
		return;

	wl_list_remove(&output->link);
	wl_output_destroy(output->wl_output);
	if ( output->layer_surface != NULL )
		zwlr_layer_surface_v1_destroy(output->layer_surface);
	if ( output->wl_surface != NULL )
		wl_surface_destroy(output->wl_surface);
	free(output->name);
	free(output);
}

void destroy_all_outputs (struct Lava_data *data)
{
	if (data->verbose)
		fputs("Destroying outputs.\n", stderr);
	struct Lava_output *op_1, *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
		destroy_output(op_1);
}
