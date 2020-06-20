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

bool is_boolean_true (const char *in)
{
	if ( ! strcmp(in, "true") || ! strcmp(in, "yes") || ! strcmp(in, "on") )
		return true;
	return false;
}

bool is_boolean_false (const char *in)
{
	if ( ! strcmp(in, "false") || ! strcmp(in, "no") || ! strcmp(in, "off") )
		return true;
	return false;
}

