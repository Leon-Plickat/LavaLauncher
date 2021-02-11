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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <cairo/cairo.h>

#include"buffer.h"
#include"util.h"

static void randomize_string (char *str, size_t len)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;

	for (size_t i = 0; i < len; i++, str++)
	{
		/* Use two byte from the current nano-second to pseudo-randomly
		 * increase the ASCII character 'A' into another character,
		 * which will then subsitute the character at *str.
		 */
		*str = (char)('A' + (r&15) + (r&16));
		r >>= 5;
	}
}

/* Tries to create a shared memory object and returns its file descriptor if
 * successful.
 */
static bool get_shm_fd (int *fd, size_t size)
{
	char name[] = "/lavalauncher-RANDOM";
	char *rp    = name + 14; /* Pointer to random part. */
	size_t rl   = 6;         /* Length of random part. */

	/* Try a few times to get a unique name. */
	for (int tries = 100; tries > 0; tries--)
	{
		/* Make the name pseudo-random to not conflict with other
		 * running instances of LavaLauncher.
		 */
		randomize_string(rp, rl);

		/* Try to create a shared memory object. Returns -1 if the
		 * memory object already exists.
		 */
		errno = 0;
		*fd   = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);

		/* If a shared memory object was created, set its size and
		 * return its file descriptor.
		 */
		if ( *fd >= 0 )
		{
			shm_unlink(name);
			if ( ftruncate(*fd, (off_t)size) < 0 )
			{
				close(*fd);
				return false;
			}
			return true;
		}

		/* The EEXIST error means that the name is not unique and we
		 * must try again.
		 */
		if ( errno != EEXIST )
		{
			log_message(0, "ERROR: shm_open: %s\n", strerror(errno));
			return false;
		}
	}

	return false;
}

static void buffer_handle_release (void *data, struct wl_buffer *wl_buffer)
{
	struct Lava_buffer *buffer = (struct Lava_buffer *)data;
	buffer->busy               = false;
}

static const struct wl_buffer_listener buffer_listener = {
	.release = buffer_handle_release,
};

static bool create_buffer (struct wl_shm *shm, struct Lava_buffer *buffer,
		uint32_t _w, uint32_t _h)
{
	int32_t w = (int32_t)_w, h = (int32_t)_h;

	const enum wl_shm_format wl_fmt    = WL_SHM_FORMAT_ARGB8888;
	const cairo_format_t     cairo_fmt = CAIRO_FORMAT_ARGB32;

	int32_t stride = cairo_format_stride_for_width(cairo_fmt, w);
	size_t   size  = (size_t)(stride * h);

	buffer->w    = _w;
	buffer->h    = _h;
	buffer->size = size;

	if ( size == 0 )
	{
		buffer->memory_object = NULL;
		buffer->surface       = NULL;
		buffer->cairo         = NULL;
		return true;
	}

	int fd;
	if (! get_shm_fd(&fd, size))
		return false;

	errno = 0;
	if ( MAP_FAILED == (buffer->memory_object = mmap(NULL, size,
					PROT_READ | PROT_WRITE, MAP_SHARED,
					fd, 0)) )
	{
		close(fd);
		log_message(0, "ERROR: mmap: %s\n", strerror(errno));
		return false;
	}

	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, (int32_t)size);
	buffer->buffer = wl_shm_pool_create_buffer(pool, 0, w, h, stride,
			wl_fmt);
	wl_buffer_add_listener(buffer->buffer, &buffer_listener, buffer);
	wl_shm_pool_destroy(pool);

	close(fd);

	buffer->surface = cairo_image_surface_create_for_data(
		buffer->memory_object, cairo_fmt, w, h, stride);
	buffer->cairo = cairo_create(buffer->surface);

	return true;
}

void finish_buffer (struct Lava_buffer *buffer)
{
	if (buffer->buffer)
		wl_buffer_destroy(buffer->buffer);
	if (buffer->cairo)
		cairo_destroy(buffer->cairo);
	if (buffer->surface)
		cairo_surface_destroy(buffer->surface);
	if (buffer->memory_object)
		munmap(buffer->memory_object, buffer->size);
	memset(buffer, 0, sizeof(struct Lava_buffer));
}

bool next_buffer (struct Lava_buffer **buffer, struct wl_shm *shm,
		struct Lava_buffer buffers[static 2], uint32_t w, uint32_t h)
{
	/* Check if buffers are busy and use first non-busy one, if it exists.
	 * If all buffers are busy, exit.
	 */
	if (! buffers[0].busy)
		*buffer = &buffers[0];
	else if (! buffers[1].busy)
		*buffer = &buffers[1];
	else
	{
		log_message(0, "ERROR: All buffers are busy.\n");
		*buffer = NULL;
		return false;
	}

	/* If the buffers dimensions do not match, or if there is no wl_buffer
	 * or if the buffer does not exist, close it and create a new one.
	 */
	if ( (*buffer)->w != w || (*buffer)->h != h || ! (*buffer)->buffer )
	{
		finish_buffer(*buffer);
		if (! create_buffer(shm, *buffer, w, h))
			return false;
	}

	return true;
}
