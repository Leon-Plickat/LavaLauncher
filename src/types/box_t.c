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

#include"types/box_t.h"

void ubox_t_set_all (ubox_t *box, uint32_t val)
{
	box->x = val;
	box->y = val;
	box->w = val;
	box->h = val;
}

ubox_t ubox_t_scale (ubox_t *in, uint32_t scale)
{
	ubox_t out = {
		.x = in->x * scale,
		.y = in->y * scale,
		.w = in->w * scale,
		.h = in->h * scale
	};
	return out;
}

void udirections_t_set_all (udirections_t *box, uint32_t val)
{
	box->top    = val;
	box->right  = val;
	box->bottom = val;
	box->left   = val;
}

udirections_t udirections_t_scale (udirections_t *in, uint32_t scale)
{
	udirections_t out = {
		.top    = in->top    * scale,
		.right  = in->right  * scale,
		.bottom = in->bottom * scale,
		.left   = in->left   * scale
	};
	return out;
}

void uradii_t_set_all (uradii_t *box, uint32_t val)
{
	box->top_left     = val;
	box->top_right    = val;
	box->bottom_left  = val;
	box->bottom_right = val;
}

uradii_t uradii_t_scale (uradii_t *in, uint32_t scale)
{
	uradii_t out = {
		.top_left     = in->top_left     * scale,
		.top_right    = in->top_right    * scale,
		.bottom_left  = in->bottom_left  * scale,
		.bottom_right = in->bottom_right * scale
	};
	return out;
}

