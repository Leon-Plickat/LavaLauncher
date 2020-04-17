/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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

struct Lava_data;

enum Lava_output_status
{
	/* Output has been created, but does not yet have an xdg_output or a
	 * layershell surface.
	 */
	OUTPUT_STATUS_UNCONFIGURED,

	/* Output has xdg_output and layershell surface. */
	OUTPUT_STATUS_CONFIGURED,

	/* Outputs layershell surface has received a configure event at least once. */
	OUTPUT_STATUS_SURFACE_CONFIGURED,

	/* Output has xdg_output, but no layershell surface and is not in use. */
	OUTPUT_STATUS_UNUSED
};

struct Lava_output
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_output      *wl_output;
	struct zxdg_output_v1 *xdg_output;
	char                  *name;
	uint32_t               global_name;
	int32_t                scale;
	int32_t                w, h;
	int32_t                bar_x_offset, bar_y_offset;

	struct wl_surface             *wl_surface;
	struct zwlr_layer_surface_v1  *layer_surface;

	enum Lava_output_status status;

	struct Lava_buffer  buffers[2];
	struct Lava_buffer *current_buffer;
};

bool create_output (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
bool configure_output (struct Lava_data *data, struct Lava_output *output);
struct Lava_output *get_output_from_global_name (struct Lava_data *data, uint32_t name);
void destroy_output (struct Lava_output *output);
void destroy_all_outputs (struct Lava_data *data);

#endif
