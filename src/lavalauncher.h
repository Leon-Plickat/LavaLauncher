/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LAVALAUNCHER_H
#define LAVALAUNCHER_H

#define _POSIX_C_SOURCE 200112L
#include<stdbool.h>

#include"wlr-layer-shell-unstable-v1-client-protocol.h"
#include"pool-buffer.h"

enum Orientation
{
	ORIENTATION_VERTICAL = 0,
	ORIENTATION_HORIZONTAL
};

enum Bar_position
{
	POSITION_TOP = 0,
	POSITION_RIGHT,
	POSITION_BOTTOM,
	POSITION_LEFT
};

struct Lava_data
{
	struct wl_display             *display;
	struct wl_registry            *registry;
	struct wl_compositor          *compositor;
	struct wl_shm                 *shm;
	struct zwlr_layer_shell_v1    *layer_shell;
	//struct zxdg_output_manager_v1 *xdg_output_manager; // TODO ??? XXX

	int32_t scale;

	struct wl_list outputs;
	struct wl_list seats;
	struct wl_list buttons;

	int button_amount;

	enum zwlr_layer_shell_v1_layer layer;

	enum Bar_position position;
	bool              aggressive_anchor;
	int               bar_width;
	int               border_width;
	float             bar_colour[4];
	float             border_colour[4];

	uint32_t w;
	uint32_t h;

	bool loop;
	bool verbose;
};

struct Lava_output
{
	struct wl_list                 link;
	struct Lava_data              *data;
	uint32_t                       global_name;
	struct wl_output              *wl_output;
//	struct zxdg_output_v1         *xdg_output;
	char                          *name;
	enum wl_output_subpixel        subpixel;
	int32_t                        scale;
	struct wl_surface             *wl_surface;
	struct zwlr_layer_surface_v1  *layer_surface;
	int                            w;
	int                            h;

	struct pool_buffer  buffers[2];
	struct pool_buffer *current_buffer;
};

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;
	struct wl_seat   *wl_seat;

	struct
	{
		struct wl_pointer *wl_pointer;
		int32_t            x;
		int32_t            y;
	} pointer;
};

struct Lava_button
{
	struct wl_list link;

	const char *img_path;
	const char *cmd;
};

#endif
