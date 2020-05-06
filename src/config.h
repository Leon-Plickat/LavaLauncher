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

void sensible_defaults (struct Lava_data *data);
bool config_set_position (struct Lava_data *data, const char *arg);
bool config_set_mode (struct Lava_data *data, const char *arg);
bool config_set_alignment (struct Lava_data *data, const char *arg);
bool config_set_layer (struct Lava_data *data, const char *arg);
bool config_set_exclusive (struct Lava_data *data, const char *arg);
bool config_set_margin (struct Lava_data *data, int top, int right, int bottom, int left);
bool config_set_icon_size (struct Lava_data *data, const char *arg);
bool config_set_border_size (struct Lava_data *data, int top, int right, int bottom, int left);
bool config_set_bar_colour (struct Lava_data *data, const char *arg);
bool config_set_border_colour (struct Lava_data *data, const char *arg);
bool config_set_only_output (struct Lava_data *data, const char *arg);
bool config_set_cursor_name (struct Lava_data *data, const char *arg);
bool config_set_effect (struct Lava_data *data, const char *effect, const char *colour);

#endif
