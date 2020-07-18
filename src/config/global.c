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
#include"log.h"
#include"config/parse-boolean.h"

static bool global_set_watch (struct Lava_data *data, const char *arg)
{
#ifdef WATCH_CONFIG
	if (is_boolean_true(arg))
		data->watch = true;
	else if (is_boolean_false(arg))
		data->watch = false;
	else
	{
		log_message(NULL, 0, "ERROR: 'watch-config-file' expects a boolean value.\n");
		return false;
	}
	return true;
#else
	log_message(NULL, 0, "WARNING: LavaLauncher has been compiled without the ability to watch the configuration file.\n");
	return true;
#endif
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_data*, const char*);
} global_configs[] = {
	{ .variable = "watch-config-file", .set = global_set_watch }
};

bool global_set_variable (struct Lava_data *data, const char *variable,
		const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(global_configs) / sizeof(global_configs[0])); i++)
		if (! strcmp(global_configs[i].variable, variable))
			return global_configs[i].set(data, value);
	log_message(NULL, 0, "ERROR: Unrecognized global setting \"%s\" on line %d.\n", variable, line);
	return false;
}
