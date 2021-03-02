/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 - 2021 Leon Henrik Plickat
 * Copyright (C) 2020 Nicolai Dagestad
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

#ifndef LAVALAUNCHER_BAR_H
#define LAVALAUNCHER_BAR_H

#include<wayland-server.h>
#include"wlr-layer-shell-unstable-v1-protocol.h"

#include"types/colour_t.h"
#include"types/box_t.h"
#include"types/buffer.h"

struct Lava_item;
struct Lava_item_instance;

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
	MODE_FULL
};

enum Condition_resolution
{
	RESOLUTION_WIDER_THAN_HIGH,
	RESOLUTION_HIGHER_THEN_WIDE,
	RESOLUTION_ALL
};

enum Hidden_mode
{
	HIDDEN_MODE_NEVER,
	HIDDEN_MODE_ALWAYS,
	HIDDEN_MODE_RIVER_AUTO
};

/* This struct holds a configuration set. */
struct Lava_bar_configuration
{
	struct wl_list link;

	/* Mode and positioning of the bar. These are responsible for the shape
	 * of the visual bar and the actual surface.
	 */
	enum Bar_position    position;
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

	/* Conditions an output must match for the bar to generate an instance on it. */
	uint32_t condition_scale;
	int32_t  condition_transform;
	enum Condition_resolution condition_resolution;
};

struct Lava_bar_instance
{
	struct Lava_bar_configuration *config;
	struct Lava_output            *output;
	struct wl_surface             *wl_surface;
	struct zwlr_layer_surface_v1  *layer_surface;

	struct wl_callback *frame_callback;

	struct Lava_item_instance *item_instances;
	int active_items;

	ubox_t surface_dim;
	ubox_t surface_hidden_dim;
	ubox_t bar_dim;
	ubox_t bar_hidden_dim;
	ubox_t item_area_dim;

	bool hidden, hover;

	struct Lava_buffer  buffers[2];
	struct Lava_buffer *current_buffer;

	struct wl_list indicators;

	bool dirty;

	bool configured;
};

bool create_bar_config (void);
void destroy_all_bar_configs (void);
bool finalize_all_bar_configs (void);
struct Lava_bar_configuration *get_bar_config_for_output (struct Lava_output *output);
bool bar_config_set_variable (struct Lava_bar_configuration *config,
		const char *variable, const char *value, uint32_t line);

struct Lava_bar_instance *create_bar_instance (struct Lava_output *output,
		struct Lava_bar_configuration *config);
void destroy_bar_instance (struct Lava_bar_instance *instance);
void update_bar_instance (struct Lava_bar_instance *instance, bool need_new_dimensions,
		bool only_update_on_hide_change);
struct Lava_bar_instance *bar_instance_from_surface (struct wl_surface *surface);
void bar_instance_pointer_leave (struct Lava_bar_instance *instance);
void bar_instance_pointer_enter (struct Lava_bar_instance *instance);
struct Lava_item_instance *bar_instance_get_item_instance_from_coords (
		struct Lava_bar_instance *instance, int32_t x, int32_t y);
void bar_instance_schedule_frame (struct Lava_bar_instance *instance);

#endif

