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

#include"xdg-output-unstable-v1-protocol.h"
#include"xdg-shell-protocol.h"

#include"lavalauncher.h"
#include"str.h"
#include"output.h"
#include"bar/bar.h"
#include"bar/bar-pattern.h"

/* No-Op function. */
static void noop () {}

static struct Lava_bar *bar_from_pattern (struct Lava_bar_pattern *pattern,
		struct Lava_output *output)
{
	struct Lava_bar *bar, *temp;
	wl_list_for_each_safe(bar, temp, &output->bars, link)
		if ( bar->pattern == pattern )
			return bar;
	return NULL;
}

static bool pattern_conditions_match_output (struct Lava_bar_pattern *pattern,
		struct Lava_output *output)
{
	if ( pattern->condition_scale != 0
			&& pattern->condition_scale != output->scale )
		return false;

	if ( pattern->condition_resolution == RESOLUTION_WIDER_THAN_HIGH
			&& output->w < output->h )
		return false;
	if ( pattern->condition_resolution == RESOLUTION_HIGHER_THEN_WIDE
			&& output->h < output->w )
		return false;

	if ( pattern->condition_transform != -1
			&& pattern->condition_transform != (int32_t)output->transform )
		return false;

	return true;
}

static bool update_bars_on_output (struct Lava_output *output)
{
	/* No xdg_output events have been received yet, so there is nothing todo. */
	if ( output->status == OUTPUT_STATUS_UNCONFIGURED || output->name == NULL )
		return true;

	struct Lava_data *data = output->data;
	log_message(data, 1, "[output] Updating bars: global_name=%d\n", output->global_name);

	if ( output->w == 0 || output->h == 0 )
	{
		destroy_all_bars(output);
		output->status = OUTPUT_STATUS_UNUSED;
		return true;
	}

	struct Lava_bar_pattern *pattern, *temp;
	wl_list_for_each_safe(pattern, temp, &data->patterns, link)
	{
		/* The name of an output can expected to remain the same. */
		if ( pattern->only_output != NULL && strcmp(output->name, pattern->only_output->string) )
			continue;

		bool conditions = pattern_conditions_match_output(pattern, output);

		struct Lava_bar *bar;
		if ( NULL != (bar = bar_from_pattern(pattern, output)) )
		{
			if (conditions)
				update_bar(bar);
			else
				destroy_bar(bar);
		}
		else if (conditions)
		{
			if (! create_bar(pattern, output))
			{
				log_message(NULL, 0, "ERROR: Could not create bar.\n");
				data->loop = false;
				data->ret  = EXIT_FAILURE;
				return false;
			}
		}
	}

	if (wl_list_empty(&output->bars))
		output->status = OUTPUT_STATUS_UNUSED;
	else
		output->status = OUTPUT_STATUS_USED;

	return true;
}

static void output_handle_scale (void *raw_data, struct wl_output *wl_output,
		int32_t factor)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->scale              = (uint32_t)factor;

	log_message(output->data, 1, "[output] Property update: global_name=%d scale=%d\n",
				output->global_name, output->scale);
}

static void output_handle_geometry(void *raw_data, struct wl_output *wl_output,
		int32_t x, int32_t y, int32_t phy_width, int32_t phy_height,
		int32_t subpixel, const char *make, const char *model,
		int32_t transform)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->transform          = (uint32_t)transform;

	log_message(output->data, 1, "[output] Property update: global_name=%d transform=%d\n",
				output->global_name, output->transform);
}

static void output_handle_done (void *raw_data, struct wl_output *wl_output)
{
	/* This event is sent after all output property changes (by wl_output
	 * and by xdg_output) have been advertised by preceding events.
	 */
	struct Lava_output *output = (struct Lava_output *)raw_data;

	log_message(output->data, 1, "[output] Atomic update complete: global_name=%d\n",
				output->global_name);

	update_bars_on_output(output);
}

static const struct wl_output_listener output_listener = {
	.scale    = output_handle_scale,
	.geometry = output_handle_geometry,
	.mode     = noop,
	.done     = output_handle_done
};

static void xdg_output_handle_name (void *raw_data,
		struct zxdg_output_v1 *xdg_output, const char *name)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	set_string(&output->name, (char *)name);

	log_message(output->data, 1, "[output] Property update: global_name=%d name=%s\n",
				output->global_name, name);
}

static void xdg_output_handle_logical_size (void *raw_data,
		struct zxdg_output_v1 *xdg_output, int32_t w, int32_t h)
{
	struct Lava_output *output = (struct Lava_output *)raw_data;
	output->w                  = (uint32_t)w;
	output->h                  = (uint32_t)h;

	log_message(output->data, 1, "[output] Property update: global_name=%d width=%d height=%d\n",
				output->global_name, w, h);
}

static const struct zxdg_output_v1_listener xdg_output_listener = {
	.logical_size     = xdg_output_handle_logical_size,
	.name             = xdg_output_handle_name,
	.logical_position = noop,
	.description      = noop,

	/* Deprecated since version 3, xdg_output property changes now send wl_output.done */
	.done             = noop
};

bool configure_output (struct Lava_output *output)
{
	struct Lava_data *data = output->data;

	log_message(data, 1, "[output] Configuring: global_name=%d\n", output->global_name);

	/* Create xdg_output and attach listeners. */
	if ( NULL == (output->xdg_output = zxdg_output_manager_v1_get_xdg_output(
					data->xdg_output_manager, output->wl_output)) )
	{
		log_message(NULL, 0, "ERROR: Could not get XDG output.\n");
		return false;
	}

	zxdg_output_v1_add_listener(output->xdg_output, &xdg_output_listener,
			output);

	output->status = OUTPUT_STATUS_UNUSED;

	/* The bars will be created when the wl_output.done event is received.
	 * This event is guaranteed to be send, since the name and logical
	 * size are guaranteed to be advertised.
	 */

	return true;
}

bool create_output (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version)
{
	log_message(data, 1, "[output] Creating: global_name=%d\n", name);

	struct wl_output *wl_output = wl_registry_bind(registry, name,
			&wl_output_interface, 3);
	assert(wl_output);

	struct Lava_output *output = calloc(1, sizeof(struct Lava_output));
	if ( output == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	output->data          = data;
	output->global_name   = name;
	output->name          = NULL;
	output->scale         = 1;
	output->wl_output     = wl_output;
	output->status        = OUTPUT_STATUS_UNCONFIGURED;
	output->w = output->h = 0;

	wl_list_init(&output->bars);

	wl_list_insert(&data->outputs, &output->link);
	wl_output_set_user_data(wl_output, output);
	wl_output_add_listener(wl_output, &output_listener, output);

	/* We can only use the output if we have both xdg_output_manager and
	 * the layer_shell. If either one is not available yet, we have to
	 * configure the output later (see init_wayland()).
	 */
	if ( data->xdg_output_manager != NULL && data->layer_shell != NULL )
	{
		if (! configure_output(output))
			return false;
	}
	else
		log_message(data, 2, "[output] Not yet configureable.\n");

	return true;
}

struct Lava_output *get_output_from_global_name (struct Lava_data *data, uint32_t name)
{
	struct Lava_output *op, *temp;
	wl_list_for_each_safe(op, temp, &data->outputs, link)
		if ( op->global_name == name )
			return op;
	return NULL;
}

void destroy_output (struct Lava_output *output)
{
	if ( output == NULL )
		return;

	destroy_all_bars(output);
	wl_list_remove(&output->link);
	wl_output_destroy(output->wl_output);
	free(output);
}

void destroy_all_outputs (struct Lava_data *data)
{
	log_message(data, 1, "[output] Destroying all outputs.\n");
	struct Lava_output *op_1, *op_2;
	wl_list_for_each_safe(op_1, op_2, &data->outputs, link)
		destroy_output(op_1);
}
