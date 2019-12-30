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

#include<stdbool.h>

#include"wlr-layer-shell-unstable-v1-client-protocol.h"
#include"xdg-output-unstable-v1-client-protocol.h"
#include"pool-buffer.h"

enum Bar_orientation
{
	ORIENTATION_VERTICAL = 0,
	ORIENTATION_HORIZONTAL
};

enum Bar_mode
{
	MODE_DEFAULT = 0,
	MODE_AGGRESSIVE,
	MODE_FULL,
	MODE_FULL_CENTER
};

enum Bar_position
{
	POSITION_TOP = 0,
	POSITION_RIGHT,
	POSITION_BOTTOM,
	POSITION_LEFT,
	POSITION_CENTER
};

struct Lava_data
{
	struct wl_display             *display;
	struct wl_registry            *registry;
	struct wl_compositor          *compositor;
	struct wl_shm                 *shm;
	struct zwlr_layer_shell_v1    *layer_shell;
	struct zxdg_output_manager_v1 *xdg_output_manager;

	struct wl_list outputs;
	struct wl_list seats;
	struct wl_list buttons;

	int button_amount;

	enum zwlr_layer_shell_v1_layer layer;

	enum Bar_position  position;
	enum Bar_mode      mode;
	int                icon_size;
	int                border_size;
	int                margin;
	char              *only_output;
	float              bar_colour[4];
	char              *bar_colour_hex;
	float              border_colour[4];
	char              *border_colour_hex;
	uint32_t           w;
	uint32_t           h;

	bool loop;
	bool verbose;
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
	int32_t                w;
	int32_t                h;
	int32_t                bar_y_offset;
	int32_t                bar_x_offset;

	struct wl_surface             *wl_surface;
	struct zwlr_layer_surface_v1  *layer_surface;
	bool                           configured;


	struct pool_buffer  buffers[2];
	struct pool_buffer *current_buffer;
};

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_seat *wl_seat;

	struct
	{
		struct wl_pointer  *wl_pointer;
		int32_t             x;
		int32_t             y;
		struct Lava_output *output;
		struct Lava_button *button;
	} pointer;
};

struct Lava_button
{
	struct wl_list link;

	const char *img_path;
	const char *cmd;
};

#endif
