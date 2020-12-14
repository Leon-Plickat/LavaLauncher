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

#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<sys/wait.h>

#include"lavalauncher.h"
#include"item.h"
#include"str.h"
#include"bar/bar-pattern.h"
#include"bar/bar.h"
#include"output.h"
#include"types/image_t.h"
#include"types/string_t.h"

/************************
 * Button configuration *
 ************************/
static bool button_set_command (struct Lava_item *button,
		const char *command, enum Interaction_type type)
{
	if ( button->command[type] != NULL )
		string_t_destroy(button->command[type]);
	button->command[type] = string_t_from(command);
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
		image_t_destroy(button->img);
	if ( NULL == (button->img = image_t_create_from_file(path)) )
		return false;
	return true;
}

static bool button_set_variable (struct Lava_data *data, struct Lava_item *button,
		const char *variable, const char *value, int line)
{
	struct
	{
		const char *variable;
		bool (*set)(struct Lava_item*, const char*, enum Interaction_type);
		enum Interaction_type type;
	} configs[] = {
		{ .variable = "command",              .set = button_set_all_commands, .type = 0                 },
		{ .variable = "left-click-command",   .set = button_set_command,      .type = TYPE_LEFT_CLICK   },
		{ .variable = "middle-click-command", .set = button_set_command,      .type = TYPE_MIDDLE_CLICK },
		{ .variable = "right-click-command",  .set = button_set_command,      .type = TYPE_RIGHT_CLICK  },
		{ .variable = "scroll-up-command",    .set = button_set_command,      .type = TYPE_SCROLL_UP    },
		{ .variable = "scroll-down-command",  .set = button_set_command,      .type = TYPE_SCROLL_DOWN  },
		{ .variable = "touch-command",        .set = button_set_command,      .type = TYPE_TOUCH        },
		{ .variable = "image-path",           .set = button_set_image_path,   .type = 0                 }
	};

	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
		{
			if (configs[i].set(button, value, configs[i].type))
				return true;
			goto exit;
		}

	log_message(NULL, 0, "ERROR: Unrecognized button setting \"%s\".\n", variable);
exit:
	log_message(NULL, 0, "INFO: The error is on line %d in \"%s\".\n",
			line, data->config_path);
	return false;
}

/************************
 * Spacer configuration *
 ************************/
static bool spacer_set_length (struct Lava_item *spacer, const char *length)
{
	int len = atoi(length);
	if ( len <= 0 )
	{
		log_message(NULL, 0, "ERROR: Spacer size must be greater than 0.\n");
		return false;
	}
	spacer->length = (unsigned int)len;
	return true;
}

static bool spacer_set_variable (struct Lava_data *data, struct Lava_item *spacer,
		const char *variable, const char *value, int line)
{
	struct
	{
		const char *variable;
		bool (*set)(struct Lava_item*, const char*);
	} configs[] = {
		{ .variable = "length", .set = spacer_set_length }
	};

	for (size_t i = 0; i < (sizeof(configs) / sizeof(configs[0])); i++)
		if (! strcmp(configs[i].variable, variable))
		{
			if (configs[i].set(spacer, value))
				return true;
			goto exit;
		}

	log_message(NULL, 0, "ERROR: Unrecognized spacer setting \"%s\".\n", variable);
exit:
	log_message(NULL, 0, "INFO: The error is on line %d in \"%s\".\n",
			line, data->config_path);
	return false;
}

bool item_set_variable (struct Lava_data *data, struct Lava_item *item,
		const char *variable, const char *value, int line)
{
	switch (item->type)
	{
		case TYPE_BUTTON: return button_set_variable(data, item, variable, value, line);
		case TYPE_SPACER: return spacer_set_variable(data, item, variable, value, line);
		default:          return false;
	}
}

/* We need to fork two times for UNIXy resons. */
static void secondary_fork (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		/* Prepare environment variables. */
		setenvf("LAVALAUNCHER_OUTPUT_NAME",  "%s", bar->output->name);
		setenvf("LAVALAUNCHER_OUTPUT_SCALE", "%d", bar->output->scale);

		/* execl() only returns on error; On success it replaces this process. */
		execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
		log_message(NULL, 0, "ERROR: execl: %s\n", strerror(errno));
		_exit(EXIT_FAILURE);
	}
	else if ( ret < 0 )
	{
		log_message(NULL, 0, "ERROR: fork: %s\n", strerror(errno));
		_exit(EXIT_FAILURE);
	}
}

/* We need to fork two times for UNIXy resons. */
static void primary_fork (struct Lava_bar *bar, struct Lava_item *item, const char *cmd)
{
	errno = 0;
	int ret = fork();
	if ( ret == 0 )
	{
		setsid();

		/* Restore signals. */
		sigset_t mask;
		sigemptyset(&mask);
		sigprocmask(SIG_SETMASK, &mask, NULL);

		secondary_fork(bar, item, cmd);
		_exit(EXIT_SUCCESS);
	}
	else if ( ret < 0 )
		log_message(NULL, 0, "ERROR: fork: %s\n", strerror(errno));
	else
		waitpid(ret, NULL, 0);
}

/* Wrapper function that catches any special cases before executing the item command. */
static bool item_command (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type)
{
	if ( item->command[type] == NULL )
		return false;

	if (! strcmp(item->command[type]->string, "exit"))
	{
		log_message(bar->data, 1, "[command] Exiting due to button command \"exit\".\n");
		bar->data->loop = false;
	}
	else if (! strcmp(item->command[type]->string, "reload"))
	{
		log_message(bar->data, 1, "[command] Reloading due to button command \"reload\".\n");
		bar->data->loop = false;
		bar->data->reload = true;
	}
	else
	{
		log_message(bar->data, 1, "[command] Executing command: %s\n",
				item->command[type]->string);
		primary_fork(bar, item, item->command[type]->string);
	}

	return true;
}

void item_interaction (struct Lava_bar *bar, struct Lava_item *item,
		enum Interaction_type type)
{
	switch (item->type)
	{
		case TYPE_BUTTON:
			item_command(bar, item, type);
			break;

		case TYPE_SPACER:
		default:
			break;
	}
}

static void item_nullify (struct Lava_item *item)
{
	item->type                  = 0;
	item->index                 = 0;
	item->ordinate              = 0;
	item->length                = 0;
	item->img                   = NULL;

	for (int i = 0; i < TYPE_AMOUNT; i++)
		item->command[i] = NULL;
}

static const char *item_type_to_string (enum Item_type type)
{
	switch (type)
	{
		case TYPE_BUTTON: return "button";
		case TYPE_SPACER: return "spacer";
	}
	return NULL;
}

bool create_item (struct Lava_bar_pattern *pattern, enum Item_type type)
{
	log_message(pattern->data, 2, "[item] Creating item: type=%s\n", item_type_to_string(type));
	struct Lava_item *new_item = calloc(1, sizeof(struct Lava_item));
	if ( new_item == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate.\n");
		return false;
	}

	item_nullify(new_item);
	new_item->type  = type;
	pattern->last_item = new_item;
	wl_list_insert(&pattern->items, &new_item->link);
	return true;
}

/* Return pointer to Lava_item struct from item list which includes the
 * given surface-local coordinates on the surface of the given output.
 */
struct Lava_item *item_from_coords (struct Lava_bar *bar, uint32_t x, uint32_t y)
{
	struct Lava_bar_pattern       *pattern = bar->pattern;
	struct Lava_bar_configuration *config  = bar->config;
	uint32_t ordinate;
	if ( config->orientation == ORIENTATION_HORIZONTAL )
		ordinate = x - bar->item_area.x;
	else
		ordinate = y - bar->item_area.y;

	struct Lava_item *item, *temp;
	wl_list_for_each_reverse_safe(item, temp, &pattern->items, link)
	{
		if ( ordinate >= item->ordinate
				&& ordinate < item->ordinate + item->length )
			return item;
	}
	return NULL;
}

unsigned int get_item_length_sum (struct Lava_bar_pattern *pattern)
{
	unsigned int sum = 0;
	struct Lava_item *it1, *it2;
	wl_list_for_each_reverse_safe (it1, it2, &pattern->items, link)
		sum += it1->length;
	return sum;
}

/* When items are created when parsing the config file, the size is not yet
 * available, so items need to be finalized later.
 */
bool finalize_items (struct Lava_bar_pattern *pattern)
{
	pattern->item_amount = wl_list_length(&pattern->items);
	if ( pattern->item_amount == 0 )
	{
		log_message(NULL, 0, "ERROR: Configuration defines a bar without items.\n");
		return false;
	}

	// TODO XXX set size to -1, which should cause it to automatically be config->size
	struct Lava_bar_configuration *config = pattern_get_first_config(pattern);

	unsigned int index = 0, ordinate = 0;
	struct Lava_item *it1, *it2;
	wl_list_for_each_reverse_safe(it1, it2, &pattern->items, link)
	{
		if ( it1->type == TYPE_BUTTON )
			it1->length = config->size;

		it1->index    = index;
		it1->ordinate = ordinate;

		index++;
		ordinate += it1->length;
	}

	return true;
}

static void destroy_item (struct Lava_item *item)
{
	wl_list_remove(&item->link);

	for (int i = 0; i < TYPE_AMOUNT; i++)
		if ( item->command[i] != NULL )
			string_t_destroy(item->command[i]);

	if ( item->img != NULL )
		image_t_destroy(item->img);

	free(item);
}

void destroy_all_items (struct Lava_bar_pattern *pattern)
{
	log_message(pattern->data, 1, "[items] Destroying all items.\n");
	struct Lava_item *bt_1, *bt_2;
	wl_list_for_each_safe(bt_1, bt_2, &pattern->items, link)
		destroy_item(bt_1);
}

