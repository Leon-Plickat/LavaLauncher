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

#ifndef LAVALAUNCHER_BUFFER_H
#define LAVALAUNCHER_BUFFER_H

#include<stdint.h>
#include<stdbool.h>
#include<cairo/cairo.h>
#include<wayland-client.h>

struct Lava_buffer
{
	struct wl_buffer *buffer;
	cairo_surface_t  *surface;
	cairo_t          *cairo;
	uint32_t          w;
	uint32_t          h;
	void             *memory_object;
	size_t            size;
	bool              busy;
};

bool next_buffer (struct Lava_buffer **buffer, struct wl_shm *shm, struct Lava_buffer buffers[static 2], uint32_t w, uint32_t h);
void finish_buffer (struct Lava_buffer *buffer);

#endif
