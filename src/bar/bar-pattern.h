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

#ifndef LAVALAUNCHER_BAR_PATTERN_H
#define LAVALAUNCHER_BAR_PATTERN_H

#include<wayland-server.h>
#include"wlr-layer-shell-unstable-v1-protocol.h"

struct Lava_data;
struct Lava_item;

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
	EFFECT_PHONE,
	EFFECT_CIRCLE,
};

struct Lava_bar_pattern
{
	struct Lava_data *data;
	struct wl_list    link;

	struct wl_list    items;
	struct Lava_item *last_item;
	int               item_amount;

	/* Mode and positioning of the bar. These are responsible for the shape
	 * of the visual bar and the actual surface.
	 */
	enum Bar_position    position;
	enum Bar_alignment   alignment;
	enum Bar_mode        mode;
	enum Bar_orientation orientation;

	/* Layer the surface will be rendered on. */
	enum zwlr_layer_shell_v1_layer layer;

	/* Size of icons and bar borders. */
	int icon_size;
	int border_top, border_right, border_bottom, border_left;

	/* Colours of the bar and its border; In float for actual usage and as
	 * hex string for insertion into commands.
	 */
	float  bar_colour[4];
	char  *bar_colour_hex;
	float  border_colour[4];
	char  *border_colour_hex;
	float  effect_colour[4];
	char  *effect_colour_hex;

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


	/* Draw effect applied to item. */
	enum Draw_effect effect;
	int effect_padding;

	/* Name of cursor which should be attached to pointer on hover. */
	char *cursor_name;
};

bool create_bar_pattern (struct Lava_data *data);
bool finalize_bar_pattern (struct Lava_bar_pattern *pattern);
void destroy_all_bar_patterns (struct Lava_data *data);
bool bar_pattern_set_variable (struct Lava_bar_pattern *pattern,
		const char *variable, const char *value, int line);

#endif

