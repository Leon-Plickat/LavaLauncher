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
#include<errno.h>

#include"log.h"

#define BUFFER_SIZE 8

bool is_png_file (const char *path)
{
	FILE *file;
	if ( NULL == (file = fopen(path, "r")) )
	{
		log_message(NULL, 0, "ERROR: Can not open file: %s\n"
				"ERROR: fopen: %s\n", path, strerror(errno));
		return false;
	}

	unsigned char buffer[BUFFER_SIZE];
	int ret = fread(buffer, sizeof(unsigned char), BUFFER_SIZE, file);
	fclose(file);

	if ( ret == 0 )
	{
		log_message(NULL, 0, "ERROR: fread() failed when trying to fetch file magic.\n");
		return false;
	}

	unsigned char png_magic[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	for (int i = 0; i < 8; i++) if ( buffer[i] != png_magic[i] )
	{
		log_message(NULL, 0, "ERROR: Unsupported file type: %s\n"
				"INFO: LavaLauncher only supports PNG images.\n",
				path);
		return false;
	}

	return true;
}

