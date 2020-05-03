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

#ifndef LAVALAUNCHER_BUTTON_H
#define LAVALAUNCHER_BUTTON_H

struct Lava_data;
struct Lava_output;

struct Lava_button
{
	struct wl_list   link;
	cairo_surface_t *img;
	char            *cmd;
	unsigned int     index, ordinate, length;
};

struct Lava_button *button_from_coords (struct Lava_data *data, struct Lava_output *output, int32_t x, int32_t y);
bool add_button (struct Lava_data *data, char *path, char *cmd);
unsigned int get_button_length_sum (struct Lava_data *data);
bool init_buttons (struct Lava_data *data);
void destroy_buttons (struct Lava_data *data);

#endif
