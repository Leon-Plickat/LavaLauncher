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

/* Various generic little helper things that don't really fit anywhere else. */

#ifndef LAVALAUNCHER_UTIL_H
#define LAVALAUNCHER_UTIL_H

#include<stdbool.h>

/* Helper macro to iterate over a struct array. */
#define FOR_ARRAY(A, B) \
	for (size_t B = 0; B < (sizeof(A) / sizeof(A[0])); B++)

/* Helper macros to try allocate something. */
#define TRY_NEW(A, B, C) \
	A *B = calloc(1, sizeof(A)); \
	if ( B == NULL ) \
	{ \
		log_message(0, "ERROR: Can not allocate.\n"); \
		return C; \
	}
#define NEW(A, B) \
	A *B = calloc(1, sizeof(A)); \
	if ( B == NULL ) \
	{ \
		log_message(0, "ERROR: Can not allocate.\n"); \
		return; \
	}

/* Helper macro to destroy something if it is not NULL. */
#define DESTROY(A, B) \
	if ( A != NULL ) \
	{ \
		B(A); \
	}

/* Helper macro to destroy something and set it to NULL if it is not NULL. */
#define DESTROY_NULL(A, B) \
	if ( A != NULL ) \
	{ \
		B(A); \
		A = NULL; \
	}

void log_message (int level, const char *fmt, ...);
void free_if_set (void *ptr);
void set_string (char **ptr, char *arg);
char *get_formatted_buffer (const char *fmt, ...);
const char *str_orelse (const char *str, const char *orelse);
void setenvf (const char *name, const char *fmt, ...);
bool string_starts_with(const char *str, const char *prefix);
bool is_boolean_true (const char *str);
bool is_boolean_false (const char *str);
bool set_boolean (bool *b, const char *value);
uint32_t count_tokens (const char *arg);
void counter_safe_subtract (uint32_t *counter, uint32_t subtract);

#endif

