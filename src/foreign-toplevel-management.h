/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2021 Leon Henrik Plickat
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

#ifndef LAVALAUNCHER_FOREIGN_TOPLEVEL_MANAGEMENT_H
#define LAVALAUNCHER_FOREIGN_TOPLEVEL_MANAGEMENT_H

#include<stdbool.h>

#include"wlr-foreign-toplevel-management-unstable-v1-protocol.h"

struct Lava_toplevel_state
{
	char *app_id;
	bool activated;
};

struct Lava_toplevel
{
	struct wl_list link;
	struct zwlr_foreign_toplevel_handle_v1 *handle;
	struct Lava_toplevel_state current, pending;
};

void init_foreign_toplevel_management (void);
void destroy_toplevel (struct Lava_toplevel *toplevel);
struct Lava_toplevel *find_toplevel_with_app_id (const char *app_id);

#endif

