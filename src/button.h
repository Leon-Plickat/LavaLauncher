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

enum Button_type
{
	TYPE_BUTTON,
	TYPE_SPACER
};

enum Button_config
{
	BUTTON_CONFIG_COMMAND,
	BUTTON_CONFIG_IMAGE_PATH,
	BUTTON_CONFIG_ERROR
};

enum Spacer_config
{
	SPACER_CONFIG_LENGTH,
	SPACER_CONFIG_ERROR
};

struct Lava_button
{
	struct wl_list    link;
	enum Button_type  type;
	cairo_surface_t  *img;
	char             *cmd;
	unsigned int      index, ordinate, length;
};

struct Lava_button *button_from_coords (struct Lava_data *data, struct Lava_output *output, int32_t x, int32_t y);
unsigned int get_button_length_sum (struct Lava_data *data);
bool init_buttons (struct Lava_data *data);
void destroy_buttons (struct Lava_data *data);

bool create_button (struct Lava_data *data);
enum Button_config button_variable_from_string (const char *string);
bool button_value_from_string (struct Lava_data *data, enum Button_config config, const char *string);

bool create_spacer (struct Lava_data *data);
enum Spacer_config spacer_variable_from_string (const char *string);
bool spacer_value_from_string (struct Lava_data *data, enum Spacer_config config, const char *string);

#endif
