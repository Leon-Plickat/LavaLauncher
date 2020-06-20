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

#define _POSIX_C_SOURCE 200809L

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>

#include"lavalauncher.h"
#include"cursor.h"

static bool global_set_cursor_name (struct Lava_data *data, const char *arg)
{
	if ( data->cursor.name != NULL )
		free(data->cursor.name);
	data->cursor.name = strdup(arg);
	return true;
}

static bool global_set_watch (struct Lava_data *data, const char *arg)
{
	if ( ! strcmp(arg, "true") || ! strcmp(arg, "yes") || ! strcmp(arg, "on") )
		data->watch = true;
	else if ( ! strcmp(arg, "false") || ! strcmp(arg, "no") || ! strcmp(arg, "off") )
		data->watch = false;
	else
	{
		fputs("ERROR: 'watch-config-file' expects a boolean value.\n", stderr);
		return false;
	}
	return true;
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_data*, const char*);
} global_configs[] = {
	{ .variable = "cursor-name",       .set = global_set_cursor_name },
	{ .variable = "watch-config-file", .set = global_set_watch       }
};

bool global_set_variable (struct Lava_data *data, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(global_configs) / sizeof(global_configs[0])); i++)
		if (! strcmp(global_configs[i].variable, variable))
			return global_configs[i].set(data, value);
	fprintf(stderr, "ERROR: Unrecognized global setting \"%s\": "
			"line=%d\n", variable, line);
	return false;
}
