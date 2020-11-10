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

#include<stdbool.h>
#include<stdarg.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

#include"lavalauncher.h"

void log_message (struct Lava_data *data, int level, const char *fmt, ...)
{
	if ( data != NULL && level > data->verbosity )
		return;

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
}


void free_if_set (void *ptr)
{
	if ( ptr != NULL )
		free(ptr);
}

void set_string (char **ptr, char *arg)
{
	free_if_set(*ptr);
	*ptr = (char *)strdup(arg);
}

char *get_formatted_buffer (const char *fmt, ...)
{
	/* First we determine the lenght of the formatted text. */
	va_list args;
	va_start(args, fmt);
	unsigned long length = (unsigned long)vsnprintf(NULL, 0, fmt, args) + 1; /* +1 for NULL terminator. */
	va_end(args);

	/* Then we allocate a buffer to hold the formatted text. */
	char *buffer = calloc(1, length);
	if ( buffer == NULL )
	{
		log_message(NULL, 0, "ERROR: Could not allocate text buffer.\n");
		return NULL;
	}

	/* Then we put the formatted text into the buffer. */
	va_start(args, fmt);
	vsnprintf(buffer, length, fmt, args);
	va_end(args);

	return buffer;
}

