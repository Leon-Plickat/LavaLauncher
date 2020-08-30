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
#include<unistd.h>
#include<string.h>
#include<errno.h>

#include"lavalauncher.h"
#include"log.h"
#include"types/image.h"
#include"types/colour.h"
#include"types/string-container.h"
#include"item/item.h"

static bool button_set_command (struct Lava_item *button,
		const char *command, enum Interaction_type type)
{
	if ( button->command[type] != NULL )
		string_container_destroy(button->command[type]);
	button->command[type] = string_container_from(command);
	return true;
}

static bool button_set_all_commands (struct Lava_item *button,
		const char *command, enum Interaction_type type)
{
	button_set_command(button, command, TYPE_LEFT_CLICK);
	button_set_command(button, command, TYPE_MIDDLE_CLICK);
	button_set_command(button, command, TYPE_RIGHT_CLICK);
	button_set_command(button, command, TYPE_TOUCH);
	return true;
}

static bool button_set_image_path (struct Lava_item *button, const char *path,
		enum Interaction_type type)
{
	if ( button->img != NULL )
		image_destroy(button->img);
	if ( NULL == (button->img = image_create_from_file(path)) )
		return false;
	return true;
}

struct
{
	const char *variable;
	bool (*set)(struct Lava_item*, const char*, enum Interaction_type);
	enum Interaction_type type;
} button_configs[] = {
	{ .variable = "command",              .set = button_set_all_commands,      .type = 0                 },
	{ .variable = "left-click-command",   .set = button_set_command,           .type = TYPE_LEFT_CLICK   },
	{ .variable = "middle-click-command", .set = button_set_command,           .type = TYPE_MIDDLE_CLICK },
	{ .variable = "right-click-command",  .set = button_set_command,           .type = TYPE_RIGHT_CLICK  },
	{ .variable = "scroll-up-command",    .set = button_set_command,           .type = TYPE_SCROLL_UP    },
	{ .variable = "scroll-down-command",  .set = button_set_command,           .type = TYPE_SCROLL_DOWN  },
	{ .variable = "touch-command",        .set = button_set_command,           .type = TYPE_TOUCH        },
	{ .variable = "image-path",           .set = button_set_image_path,        .type = 0                 }
};

bool button_set_variable (struct Lava_data *data, struct Lava_item *button,
		const char *variable, const char *value, int line)
{
	for (size_t i = 0; i < (sizeof(button_configs) / sizeof(button_configs[0])); i++)
		if (! strcmp(button_configs[i].variable, variable))
		{
			if (button_configs[i].set(button, value, button_configs[i].type))
				return true;
			goto exit;
		}
	log_message(NULL, 0, "ERROR: Unrecognized button setting \"%s\".\n", variable);
exit:
	log_message(NULL, 0, "INFO: The error is on line %d in \"%s\".\n",
			line, data->config_path);
	return false;
}
