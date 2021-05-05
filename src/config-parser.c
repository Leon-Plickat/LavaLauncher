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

#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<errno.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>

#include"lib-infinitesimal.h"

#include"lavalauncher.h"
#include"util.h"
#include"item.h"
#include"bar.h"

#define BUFFER_SIZE 1024

/*********************
 *                   *
 *  global settings  *
 *                   *
 *********************/
static bool global_set_watch (const char *arg)
{
#ifdef WATCH_CONFIG
	return set_boolean(&context.watch, arg);
#else
	log_message(0, "WARNING: LavaLauncher has been compiled without the ability to watch the configuration file for changes.\n");
	return true;
#endif
}

bool global_set_variable (const char *variable, const char *value, uint32_t line)
{
	struct
	{
		const char *variable;
		bool (*set)(const char*);
	} configs[] = {
		{ .variable = "watch-config-file", .set = global_set_watch }
	};

	FOR_ARRAY(configs, i) if (! strcmp(configs[i].variable, variable))
	{
		if (configs[i].set(value))
			return true;
		goto exit;
	}

	log_message(0, "ERROR: Unrecognized global setting \"%s\".\n", variable);
exit:
	log_message(0, "INFO: The error is on line %d in \"%s\".\n", line, context.config_path);
	return false;
}

/************
 *          *
 *  Parser  *
 *          *
 ************/
static bool get_default_config_path (void)
{
	struct
	{
		const char *fmt;
		const char *env;
	} paths[] = {
		{ .fmt = "./lavalauncher.conf",                           .env = NULL                      },
		{ .fmt = "%s/lavalauncher/lavalauncher.conf",             .env = getenv("XDG_CONFIG_HOME") },
		{ .fmt = "%s/.config/lavalauncher/lavalauncher.conf",     .env = getenv("HOME")            },
		{ .fmt = "/usr/local/etc/lavalauncher/lavalauncher.conf", .env = NULL                      },
		{ .fmt = "/etc/lavalauncher/lavalauncher.conf",           .env = NULL                      }
	};

	FOR_ARRAY(paths, i)
	{
		context.config_path = get_formatted_buffer(paths[i].fmt, paths[i].env);
		if (! access(context.config_path, F_OK | R_OK))
		{
			log_message(1, "[config] Using default configuration file path: %s\n", context.config_path);
			return true;
		}
		free_if_set(context.config_path);
	}

	log_message(0, "ERROR: Can not find configuration file.\n"
			"INFO: You can provide a path manually with '-c'.\n");
	return false;
}

enum Config_section
{
	SECTION_NONE,
	SECTION_GLOBAL_SETTINGS,
	SECTION_CONFIG,
	SECTION_ITEM
};

static bool section_callback (void *user_data, uint32_t line, const char *section_name)
{
	enum Config_section *section = (enum Config_section *)user_data;
	if (! strcmp(section_name, "[global-settings]"))
	{
		*section = SECTION_GLOBAL_SETTINGS;
		return true;
	}
	else if (! strcmp(section_name, "[config]"))
	{
		*section = SECTION_CONFIG;
		return create_bar_config();
	}
	else if (! strcmp(section_name, "[item:button]"))
	{
		*section = SECTION_ITEM;
		return create_item(TYPE_BUTTON);
	}
	else if (! strcmp(section_name, "[item:spacer]"))
	{
		*section = SECTION_ITEM;
		return create_item(TYPE_SPACER);
	}

	log_message(0, "ERROR: Invalid section '%s' on line %d in \"%s\".\n",
		section_name, line, context.config_path);
	return false;
}

static bool assign_callback (void *user_data, uint32_t line, const char *variable, const char *value)
{
	enum Config_section *section = (enum Config_section *)user_data;
	if ( *section == SECTION_NONE )
	{
		log_message(0, "ERROR: Assignment outside of a section on line %d in \"%s\".\n",
				line, context.config_path);
		return false;
	}
	else if ( *section == SECTION_GLOBAL_SETTINGS )
		return global_set_variable(variable, value, line);
	else if ( *section == SECTION_CONFIG )
		return bar_config_set_variable(context.last_config, variable, value, line);
	else if ( *section == SECTION_ITEM )
		return item_set_variable(context.last_item, variable, value, line);

	/* Unreachable. */
	return false;
}

static void error_callback (void *user_data, uint32_t line, const char *msg)
{
	log_message(0, "ERROR: Failed to parse line %d in \"%s\": %s.\n",
			line, context.config_path, msg);
}

bool parse_config_file (void)
{
	if ( context.config_path == NULL && !get_default_config_path() )
		return false;

	FILE *file = fopen(context.config_path, "r");
	if ( file == NULL )
	{
		log_message(0, "ERROR: Can not open config file \"%s\".\n"
				"ERROR: fopen: %s\n", context.config_path, strerror(errno));
		return false;
	}

	enum Config_section section = SECTION_NONE;
	bool ret = infinitesimal_parse_file(file, (void *)&section,
		&section_callback, &assign_callback, &error_callback);

	if ( ret == true && section == SECTION_NONE )
	{
		log_message(0, "ERROR: Configuration file is void of any meaningful content.\n");
		ret = false;
	}

	fclose(file);
	return ret;
}

