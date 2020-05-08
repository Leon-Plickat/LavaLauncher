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

#ifndef LAVALAUNCHER_CONFIG_H
#define LAVALAUNCHER_CONFIG_H

#include"lavalauncher.h"

enum Lava_config
{
	CONFIG_POSITION,
	CONFIG_ALIGNMENT,
	CONFIG_MODE,
	CONFIG_LAYER,
	CONFIG_ICON_SIZE,
	CONFIG_BORDER_TOP,
	CONFIG_BORDER_RIGHT,
	CONFIG_BORDER_LEFT,
	CONFIG_BORDER_BOTTOM,
	CONFIG_MARGIN_TOP,
	CONFIG_MARGIN_RIGHT,
	CONFIG_MARGIN_LEFT,
	CONFIG_MARGIN_BOTTOM,
	CONFIG_ONLY_OUTPUT,
	CONFIG_EXCLUSIVE_ZONE,
	CONFIG_CURSOR_NAME,
	CONFIG_BAR_COLOUR,
	CONFIG_BORDER_COLOUR,
	CONFIG_EFFECT_COLOUR,
	CONFIG_EFFECT,

	CONFIG_ERROR
};

void sensible_defaults (struct Lava_data *data);
enum Lava_config config_variable_from_string (const char *string);
bool config_value_from_string (struct Lava_data *data, enum Lava_config config, const char *string);
bool hex_to_rgba (const char *hex, float *c_r, float *c_g, float *c_b, float *c_a);

#endif
