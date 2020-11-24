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

#ifndef LAVALAUNCHER_BOX_T_H
#define LAVALAUNCHER_BOX_T_H

#include<stdint.h>

/* Various "box" type structs, containing four values each. */

typedef struct
{
	uint32_t x, y, w, h;
} ubox_t;

typedef struct
{
	uint32_t top, right, bottom, left;
} udirections_t;

typedef struct
{
	uint32_t top_left, top_right, bottom_left, bottom_right;
} uradii_t;

void ubox_t_set_all (ubox_t *box, uint32_t val);
ubox_t ubox_t_scale (ubox_t *in, uint32_t scale);
void udirections_t_set_all (udirections_t *box, uint32_t val);
udirections_t udirections_t_scale (udirections_t *in, uint32_t scale);
void uradii_t_set_all (uradii_t *box, uint32_t val);
uradii_t uradii_t_scale (uradii_t *in, uint32_t scale);

#endif

