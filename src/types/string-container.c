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

#include"types/string-container.h"
#include<log.h>

char *string_container_get_string_or_else (struct Lava_string_container *sc, char *or_else)
{
	if ( sc != NULL )
		return sc->string;
	else
		return or_else;
}

struct Lava_string_container *string_container_from (const char *in)
{
	struct Lava_string_container *sc = calloc(1, sizeof(struct Lava_string_container));
	if ( sc == NULL )
	{
		log_message(NULL, 0, "ERROR: Failed to allocate string container.\n");
		return NULL;
	}

	sc->string     = strdup(in);
	sc->length     = (uint32_t)strlen(sc->string);
	sc->references = 1;

	return sc;
}

struct Lava_string_container *string_container_reference (struct Lava_string_container *sc)
{
	sc->references++;
	return sc;
}

void string_container_destroy (struct Lava_string_container *sc)
{
	sc->references--;

	if ( sc->references > 0 )
		return;

	if ( sc->string != NULL )
		free(sc->string);

	free(sc);
}

