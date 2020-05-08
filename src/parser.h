/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
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

#ifndef LAVALAUNCHER_PARSER_H
#define LAVALAUNCHER_PARSER_H

struct Lava_data;
struct Lava_button;

enum Parser_context
{
	/* *_PRE contexts are entered when the parser has read the context name
	 * and is waiting for '{'.
	 */
	CONTEXT_NONE,
	CONTEXT_SETTINGS_PRE,
	CONTEXT_SETTINGS,
	CONTEXT_ITEMS_PRE,
	CONTEXT_ITEMS,
	CONTEXT_BUTTON_PRE,
	CONTEXT_BUTTON,
	CONTEXT_SPACER_PRE,
	CONTEXT_SPACER
};

enum Parser_action
{
	/* Waiting for a variable name. */
	ACTION_NONE,

	/* Parser has read variable name and is now waiting for a value. */
	ACTION_ASSIGN,

	/* Parser has read value and is now waiting for ';'. */
	ACTION_ASSIGNED
};

struct Lava_parser
{
	struct Lava_data *data;
	FILE *file;
	int line, column;
	char last_char;

	enum Parser_context context;
	enum Parser_action  action;

	char buffer[1024];
	size_t buffer_length;
	char buffer_2[1024];
	size_t buffer_2_length;

	/* The button whichs config is currently read. */
	struct Lava_button *button;
};

bool parse_config (struct Lava_data *data, const char *config_path);

#endif
