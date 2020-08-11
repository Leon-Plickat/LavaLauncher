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

#include"types/colour.h"

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

enum Condition_resolution
{
	RESOLUTION_WIDER_THAN_HIGH,
	RESOLUTION_HIGHER_THEN_WIDE,
	RESOLUTION_ALL
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

	uint32_t size, icon_padding;
	uint32_t border_top, border_right, border_bottom, border_left;
	uint32_t radius_top_left, radius_top_right, radius_bottom_left, radius_bottom_right;

	struct Lava_colour bar_colour;
	struct Lava_colour border_colour;

	uint32_t indicator_padding;
	struct Lava_colour indicator_hover_colour;
	struct Lava_colour indicator_active_colour;

	/* If only_output[0] is \0, a surface will be created for all outputs.
	 * Otherwise only on the output which name is equal to *only_output.
	 * Examples of valid names are "eDP-1" or "HDMI-A-1" (likely compositor
	 * dependant).
	 */
	char only_output[64];

	/* If exclusive_zone is 1, it will be set to the width/height of the bar
	 * at startup, otherwise its exact value is used, which should be either
	 * 0 or -1.
	 */
	int32_t exclusive_zone;

	/* Directional margins of the surface. */
	int32_t margin_top, margin_right, margin_bottom, margin_left;

	/* Name of cursor which should be attached to pointer on hover. */
	char cursor_name[64];

	/* Conditions an output must match for the pattern to generate a bar on it. */
	uint32_t condition_scale;
	int32_t  condition_transform;
	enum Condition_resolution condition_resolution;
};

bool create_bar_pattern (struct Lava_data *data);
bool copy_last_bar_pattern (struct Lava_data *data);
bool finalize_bar_pattern (struct Lava_bar_pattern *pattern);
void destroy_all_bar_patterns (struct Lava_data *data);
bool bar_pattern_set_variable (struct Lava_bar_pattern *pattern,
		const char *variable, const char *value, int line);

#endif

