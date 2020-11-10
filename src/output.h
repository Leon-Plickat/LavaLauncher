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

#include<wayland-server.h>

struct Lava_data;

enum Lava_output_status
{
	/* Output has been created, but does not yet have an xdg_output or any bars. */
	OUTPUT_STATUS_UNCONFIGURED,

	/* Output has xdg_output, and bars. */
	OUTPUT_STATUS_USED,

	/* Output has xdg_output, but currently no bars. */
	OUTPUT_STATUS_UNUSED,
};

struct Lava_output
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_list bars;

	struct wl_output      *wl_output;
	struct zxdg_output_v1 *xdg_output;

	char    *name;
	uint32_t global_name;
	uint32_t scale;
	uint32_t transform;
	uint32_t w, h;

	enum Lava_output_status status;
};

bool create_output (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version);
bool configure_output (struct Lava_output *output);
struct Lava_output *get_output_from_global_name (struct Lava_data *data,
		uint32_t name);
void destroy_output (struct Lava_output *output);
void destroy_all_outputs (struct Lava_data *data);

#endif
