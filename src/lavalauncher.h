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

#ifndef LAVALAUNCHER_LAVALAUNCHER_H
#define LAVALAUNCHER_LAVALAUNCHER_H

#include<stdbool.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"buffer.h"

enum Bar_orientation
{
	ORIENTATION_VERTICAL = 0,
	ORIENTATION_HORIZONTAL,
	ORIENTATION_AUTO
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

	/* Return value. */
	int ret;

	/* Amount of buttons defined by the user. */
	int button_amount;

	/* Anchors of the bar surface. */
	enum zwlr_layer_surface_v1_anchor anchors;

	/* Layer the surface will be rendered on. */
	enum zwlr_layer_shell_v1_layer layer;

	/* Orientation of the bar surface. */
	enum Bar_orientation orientation;

	/* Size of icons and bar borders. */
	int icon_size;
	int border_top, border_right, border_bottom, border_left;

	/* Expected (and enforced) width and height of the bar. */
	uint32_t w, h;

	/* Colours of the bar and its border; In float for actual usage and as
	 * hex string for insertion into commands.
	 */
	float  bar_colour[4];
	char  *bar_colour_hex;
	float  border_colour[4];
	char  *border_colour_hex;

	/* If *only_output is NULL, a surface will be created for all outputs.
	 * Otherwise only on the output which name is equal to *only_output.
	 * Examples of valid names are "eDP-1" or "HDMI-A-1" (likely compositor
	 * dependant).
	 */
	char *only_output;

	/* If exclusive_zone is 1, it will be set to the width/height of the bar
	 * at startup, otherwise its exact value is used, which should be either
	 * 0 or -1.
	 */
	uint32_t exclusive_zone;

	/* Margin to the edge the surface is anchored to. */
	int margin;

	/* Still running? */
	bool loop;

	/* Verbose output? */
	bool verbose;
};

struct Lava_button
{
	struct wl_list link;

	const char *img_path;
	const char *cmd;

	/* Button icon. The PNG images get loaded into the cairo surfaces at
	 * startup.
	 */
	cairo_surface_t *img;
};

#endif
