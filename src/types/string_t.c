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
#include<stdint.h>
#include<unistd.h>
#include<string.h>

#include"types/string_t.h"
#include<str.h>

char *string_t_get_string_or_else (string_t *str, char *or_else)
{
	if ( str != NULL )
		return str->string;
	else
		return or_else;
}

string_t *string_t_from (const char *in)
{
	string_t *str = calloc(1, sizeof(string_t));
	if ( str == NULL )
	{
		log_message(NULL, 0, "ERROR: Failed to allocate string container.\n");
		return NULL;
	}

	str->string     = strdup(in);
	str->references = 1;

	return str;
}

string_t *string_t_reference (string_t *str)
{
	str->references++;
	return str;
}

void string_t_destroy (string_t *str)
{
	if ( --str->references > 0 )
		return;
	free_if_set(str->string);
	free(str);
}

