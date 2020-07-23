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

#ifndef LAVALAUNCHER_TOPLEVEL_H
#define LAVALAUNCHER_TOPLEVEL_H

#include<stdbool.h>
#include<stdint.h>

struct Lava_data;
struct wl_registry;

struct Lava_toplevel
{
	struct wl_list link;
	struct Lava_data *data;
	struct zwlr_foreign_toplevel_handle_v1 *toplevel;
	char current_app_id[64];
	char pending_app_id[64];
};

bool create_toplevel_manager (struct Lava_data *data, struct wl_registry *registry,
		uint32_t name);
void destroy_all_toplevels (struct Lava_data *data);

#endif
