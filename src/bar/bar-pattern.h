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

#include"types/colour_t.h"
#include"types/box_t.h"

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
	MODE_AGGRESSIVE
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

enum Item_indicator_style
{
	STYLE_RECTANGLE,
	STYLE_ROUNDED_RECTANGLE,
	STYLE_CIRCLE
};

enum Hidden_mode
{
	HIDDEN_MODE_NEVER,
	HIDDEN_MODE_ALWAYS,
	HIDDEN_MODE_RIVER_AUTO
};

struct Lava_bar_configuration
{
	struct wl_list link;

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
	udirections_t border;
	udirections_t margin;
	uradii_t radii;

	uint32_t hidden_size;
	enum Hidden_mode hidden_mode;

	colour_t bar_colour;
	colour_t border_colour;

	uint32_t indicator_padding;
	colour_t indicator_hover_colour;
	colour_t indicator_active_colour;
	enum Item_indicator_style indicator_style;

	/* If only_output is NULL, a surface will be created for all outputs.
	 * Otherwise only on the output which name is equal to *only_output.
	 * Examples of valid names are "eDP-1" or "HDMI-A-1" (likely compositor
	 * dependant).
	 */
	char *only_output;

	/* Some compositors apparently expose different behaviours based on the
	 * namespace of a layer surface, therefore it must be configurable.
	 * This may be dropped once the layershell has been standardized.
	 */
	char *namespace;

	/* If exclusive_zone is 1, it will be set to the width/height of the bar
	 * at startup, otherwise its exact value is used, which should be either
	 * 0 or -1.
	 */
	int32_t exclusive_zone;

	/* Name of cursor which should be attached to pointer on hover. */
	char *cursor_name;

	/* Conditions an output must match for the pattern to generate a bar on it. */
	uint32_t condition_scale;
	int32_t  condition_transform;
	enum Condition_resolution condition_resolution;
};

struct Lava_bar_pattern
{
	struct Lava_data *data;
	struct wl_list    link;

	struct wl_list    items;
	struct Lava_item *last_item;
	int               item_amount;

	/* The different configurations of the bar pattern. The first one is
	 * treated as default.
	 */
	struct Lava_bar_configuration *current_config;
	struct wl_list configs;
};

bool create_bar_config (struct Lava_bar_pattern *pattern, bool default_config);
struct Lava_bar_configuration *pattern_get_first_config (struct Lava_bar_pattern *pattern);
struct Lava_bar_configuration *pattern_get_last_config (struct Lava_bar_pattern *pattern);
bool create_bar_pattern (struct Lava_data *data);
bool finalize_bar_pattern (struct Lava_bar_pattern *pattern);
void destroy_all_bar_patterns (struct Lava_data *data);
bool bar_config_set_variable (struct Lava_bar_configuration *config,
		struct Lava_data *data, const char *variable, const char *value,
		int line);

#endif

