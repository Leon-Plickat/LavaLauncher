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

	/* Has the surface been configured? This is needed to prevent trying to
	 * render an unconfigured surface.
	 */
	bool configured;

	struct Lava_buffer  buffers[2];
	struct Lava_buffer *current_buffer;
};


bool create_output (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

#endif
