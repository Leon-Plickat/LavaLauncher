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

#ifndef LAVALAUNCHER_LAVALAUNCHER_H
#define LAVALAUNCHER_LAVALAUNCHER_H

#include<stdbool.h>

#include"wlr-layer-shell-unstable-v1-protocol.h"
#include"xdg-output-unstable-v1-protocol.h"
#include"buffer.h"

enum Bar_position
{
	POSITION_TOP,
	POSITION_RIGHT,
	POSITION_BOTTOM,
	POSITION_LEFT
};

enum Bar_orientation
{
	ORIENTATION_VERTICAL,
	ORIENTATION_HORIZONTAL
};

enum Bar_mode
{
	MODE_DEFAULT,
	MODE_FULL,
	MODE_SIMPLE
};

enum Bar_alignment
{
	ALIGNMENT_START,
	ALIGNMENT_CENTER,
	ALIGNMENT_END
};

enum Draw_effect
{
	EFFECT_NONE,
	EFFECT_BOX,
	EFFECT_PHONE
};

struct Lava_button;

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
	struct Lava_button *last_button;

	/* Return value. */
	int ret;

	/* Mode and positioning of the bar. These re responsible for the shape
	 * of the visual bar and the actual surface.
	 */
	enum Bar_position    position;
	enum Bar_alignment   alignment;
	enum Bar_orientation orientation;
	enum Bar_mode        mode;

	/* Amount of buttons defined by the user. */
	int button_amount;

	/* Layer the surface will be rendered on. */
	enum zwlr_layer_shell_v1_layer layer;

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
	float  effect_colour[4];
	char  *effect_colour_hex;

	/* Draw effect applied to button. */
	enum Draw_effect effect;
	int effect_padding;

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

	/* Directional margins of the surface. */
	int margin_top, margin_right, margin_bottom, margin_left;

	/* Still running? */
	bool loop;

	/* Verbose output? */
	bool verbose;

	/* Everything that is needed for changing the cursor. */
	struct {
		char                   *name;
		struct wl_cursor_theme *theme;
		struct wl_cursor_image *image;
		struct wl_surface      *surface;
	} cursor;
};

#endif
