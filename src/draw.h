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

#ifndef DRAW_H
#define DRAW_H

#define _POSIX_C_SOURCE 200112L
#include"lavalauncher.h"

enum Draw_direction
{
	DRAW_DIRECTION_VERTICAL = 0,
	DRAW_DIRECTION_HORIZONTAL
};

void render_bar_frame (struct Lava_data *data, struct Lava_output *output);

#endif
