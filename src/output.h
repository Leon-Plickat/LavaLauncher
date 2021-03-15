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

#ifndef LAVALAUNCHER_OUTPUT_H
#define LAVALAUNCHER_OUTPUT_H

#include<wayland-client.h>

enum Lava_output_status
{
	/* Output has been created, but does not yet have an xdg_output or a bar. */
	OUTPUT_STATUS_UNCONFIGURED,

	/* Output has xdg_output and bar. */
	OUTPUT_STATUS_USED,

	/* Output has xdg_output but currently no bar. */
	OUTPUT_STATUS_UNUSED
};

struct Lava_output
{
	struct wl_list link;

	struct Lava_bar_instance *bar_instance;

	struct wl_output      *wl_output;
	struct zxdg_output_v1 *xdg_output;

	/* River output status stuff (optional). */
	struct zriver_output_status_v1 *river_status;
	uint32_t river_focused_tags, river_view_tags;
	bool river_output_occupied;

	char    *name;
	uint32_t global_name;
	uint32_t scale;
	uint32_t transform;
	uint32_t w, h;

	enum Lava_output_status status;
};

bool create_output (struct wl_registry *registry, uint32_t name,
				struct wl_output *wl_output);
void configure_output (struct Lava_output *output);
struct Lava_output *get_output_from_global_name (uint32_t name);
void destroy_output (struct Lava_output *output);

#endif

