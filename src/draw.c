/*
 * LavaLauncher - A simple launcher for Wayland
 *
 * Copyright (C) 2019 Leon Plickat <leonhenrik.plickat@stud.uni-goettingen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3 as
 * published by the Free Software Foundation.
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
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<cairo/cairo.h>
#include<wayland-server.h>

#include"lavalauncher.h"
#include"draw.h"

/* Draw a single icon (square picture). */
static void draw_icon (cairo_t *cairo, int32_t x, int32_t y,
		int32_t icon_size, const char *path)
{
	float w, h;
	cairo_surface_t *image = NULL;

	image = cairo_image_surface_create_from_png(path);
	w     = cairo_image_surface_get_width(image);
	h     = cairo_image_surface_get_height(image);

	cairo_translate(cairo, x, y);
	cairo_scale(cairo, icon_size / w, icon_size / h);
	cairo_set_source_surface(cairo, image, 0, 0);
	cairo_paint(cairo);

	cairo_surface_destroy(image);
}

/* Draw the icons for all defined buttons. */
static void draw_icons (cairo_t *cairo, int32_t x, int32_t y, int32_t icon_size,
		enum Bar_orientation orientation, struct wl_list *button_list)
{
	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, button_list, link)
	{
		cairo_save(cairo);
		draw_icon(cairo, x, y, icon_size, bt_1->img_path);
		cairo_restore(cairo);

		if ( orientation == ORIENTATION_HORIZONTAL )
			x += icon_size;
		else if (orientation == ORIENTATION_VERTICAL )
			y += icon_size;
	}
}

/* Draw a coloured rectangle. */
static void draw_coloured_rectangle (cairo_t *cairo, float colour[4],
		int x, int y, int w, int h, float scale)
{
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_rectangle(cairo, x * scale, y * scale, w *scale, h * scale);
	cairo_close_path(cairo);
	cairo_fill(cairo);
}

/* Draw the bar background plus border. */
static void draw_bar (cairo_t *cairo, int32_t x, int32_t y, int32_t w, int32_t h,
		int32_t border_size, float scale, enum Bar_position position,
		float center_colour[4], float border_colour[4],
		bool margin, bool full)
{
	/* Calculate dimensions of center. */
	int32_t cx = x, cy = y, cw = w, ch = h;
	switch (position)
	{
		case POSITION_TOP:
			ch -= border_size;
			if (! full)
			{
				cx += border_size;
				cw -= 2 * border_size;
			}
			if (margin)
			{
				cy += border_size;
				ch -= border_size;
			}
			break;

		case POSITION_BOTTOM:
			cy += border_size;
			ch -= border_size;
			if (! full)
			{
				cx += border_size;
				cw -= 2 * border_size;
			}
			if (margin)
				ch -= border_size;
			break;

		case POSITION_LEFT:
			cw -= border_size;
			if (! full)
			{
				cy += border_size;
				ch -= 2 * border_size;
			}
			if (margin)
			{
				cx += border_size;
				cw -= border_size;
			}
			break;

		case POSITION_RIGHT:
			cx += border_size;
			cw -= border_size;
			if (! full)
			{
				cy += border_size;
				ch -= 2 * border_size;
			}
			if (margin)
				cw -= border_size;
			break;

		case POSITION_CENTER:
			cx += border_size;
			cy += border_size;
			cw -= 2 * border_size;
			ch -= 2 * border_size;
			break;
	}

	/* Draw center. */
	draw_coloured_rectangle(cairo, center_colour, cx, cy, cw, ch, scale);

	/* Draw borders. */
	if (! border_size)
		return;
	int y_add = 0, x_add = 0;
	switch (position)
	{
		case POSITION_BOTTOM:
			draw_coloured_rectangle(cairo, border_colour,
					x, y,
					w, border_size, scale);
			if (! full)
			{
				draw_coloured_rectangle(cairo, border_colour,
						x, y + border_size,
						border_size, ch, scale);
				draw_coloured_rectangle(cairo, border_colour,
						x + cw + border_size, y + border_size,
						border_size, ch, scale);
			}
			if (margin)
				draw_coloured_rectangle(cairo, border_colour,
						x, y + ch + border_size,
						w, border_size, scale);
			break;

		case POSITION_TOP:
			if (margin)
			{
				y_add = border_size;
				draw_coloured_rectangle(cairo, border_colour,
						x, y,
						w, border_size, scale);
			}
			if (! full)
			{
				draw_coloured_rectangle(cairo, border_colour,
						x, y + y_add,
						border_size, ch, scale);
				draw_coloured_rectangle(cairo, border_colour,
						x + cw + border_size, y + y_add,
						border_size, ch, scale);
			}
			draw_coloured_rectangle(cairo, border_colour,
					x, y + ch + y_add,
					w, border_size, scale);
			break;

		case POSITION_RIGHT:
			draw_coloured_rectangle(cairo, border_colour,
					x, y,
					border_size, h, scale);
			if (! full)
			{
				draw_coloured_rectangle(cairo, border_colour,
						x + border_size, y,
						cw, border_size, scale);
				draw_coloured_rectangle(cairo, border_colour,
						x + border_size, y + border_size + ch,
						cw, border_size, scale);
			}
			if (margin)
				draw_coloured_rectangle(cairo, border_colour,
						x + cw + border_size, y,
						border_size, h, scale);
			break;

		case POSITION_LEFT:
			if (margin)
			{
				x_add = border_size;
				draw_coloured_rectangle(cairo, border_colour,
						x, y,
						border_size, h, scale);
			}
			if (! full)
			{
				draw_coloured_rectangle(cairo, border_colour,
						x + x_add, y,
						cw, border_size, scale);
				draw_coloured_rectangle(cairo, border_colour,
						x + x_add, y + border_size + ch,
						cw, border_size, scale);
			}
			draw_coloured_rectangle(cairo, border_colour,
					x + cw + x_add, y,
					border_size, h, scale);
			break;

		case POSITION_CENTER:
			draw_coloured_rectangle(cairo, border_colour,
					x, y,
					w, border_size, scale);
			draw_coloured_rectangle(cairo, border_colour,
					x, y + ch + border_size,
					w, border_size, scale);
			draw_coloured_rectangle(cairo, border_colour,
					x, y + border_size,
					border_size, ch, scale);
			draw_coloured_rectangle(cairo, border_colour,
					x + cw + border_size, y + border_size,
					border_size, ch, scale);
			break;
	}
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if (! output->configured)
		return;

	/* Get orientation. */
	enum Bar_orientation orientation;
	switch (data->position)
	{
		case POSITION_CENTER:
		case POSITION_TOP:
		case POSITION_BOTTOM:
			orientation = ORIENTATION_HORIZONTAL;
			break;

		case POSITION_LEFT:
		case POSITION_RIGHT:
			orientation = ORIENTATION_VERTICAL;
	}

	/* Calculate buffer size. */
	int buffer_w = data->w * output->scale, buffer_h = data->h * output->scale;
	if ( data->mode != MODE_DEFAULT )
		switch (data->position)
		{
			case POSITION_TOP:
			case POSITION_BOTTOM:
				buffer_w = output->w;
				break;

			case POSITION_LEFT:
			case POSITION_RIGHT:
				buffer_h = output->h;
				break;

			case POSITION_CENTER:
				/* Will never be reached. */
				break;
		}

	/* Get new/next buffer. */
	output->current_buffer = get_next_buffer(data->shm, output->buffers,
			buffer_w, buffer_h);
	if (! output->current_buffer)
		return;

	/* Clear buffer. */
	cairo_t *cairo = output->current_buffer->cairo;
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);

	/* Calculate size of bar. */
	int bar_x = 0, bar_y = 0, bar_w = data->w, bar_h = data->h;
	bool full = false, margin = data->margin ? true : false;
	if ( data->mode == MODE_FULL || data->mode == MODE_FULL_CENTER )
	{
		full  = true;
		switch (data->position)
		{
			case POSITION_CENTER:
			case POSITION_TOP:
			case POSITION_BOTTOM:
				bar_w = output->w;
				break;

			case POSITION_LEFT:
			case POSITION_RIGHT:
				bar_h = output->h;
		}
	}
	else if ( data->mode == MODE_AGGRESSIVE )
	{
		bar_x = output->bar_x_offset;
		bar_y = output->bar_y_offset;
	}

	/* Draw bar. */
	if (data->verbose)
		fputs("Drawing bar.\n", stderr);
	draw_bar(cairo, bar_x, bar_y, bar_w, bar_h, data->border_size,
			output->scale, data->position,
			data->bar_colour, data->border_colour,
			margin, full);

	/* Calculate size of icons. */
	int icons_x = output->bar_x_offset, icons_y = output->bar_y_offset;
	if ( data->mode == MODE_DEFAULT || data->mode == MODE_AGGRESSIVE )
		switch (data->position)
		{
			case POSITION_CENTER:
			case POSITION_BOTTOM:
			case POSITION_RIGHT:
				icons_x += data->border_size;
				icons_y += data->border_size;
				break;

			case POSITION_TOP:
				icons_x += data->border_size;
				if (margin)
					icons_y += data->border_size;
				break;

			case POSITION_LEFT:
				icons_y += data->border_size;
				if (margin)
					icons_x += data->border_size;
				break;
		}
	else
		switch (data->position)
		{
			case POSITION_BOTTOM:
				icons_y += data->border_size;
				if ( data->mode == MODE_FULL_CENTER )
					icons_x += data->border_size;
				break;

			case POSITION_TOP:
				if ( data->mode == MODE_FULL_CENTER )
					icons_x += data->border_size;
				if (margin)
					icons_y += data->border_size;
				break;

			case POSITION_RIGHT:
				icons_x += data->border_size;
				if ( data->mode == MODE_FULL_CENTER )
					icons_y += data->border_size;
				break;

			case POSITION_LEFT:
				if ( data->mode == MODE_FULL_CENTER )
					icons_y += data->border_size;
				if (margin)
					icons_x += data->border_size;
				break;

			case POSITION_CENTER:
				/* Will never be reached. */
				break;
		}

	/* Draw icons. */
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);
	draw_icons(cairo, icons_x, icons_y, data->icon_size, orientation,
			&data->buttons);

	/* Commit surface. */
	if (data->verbose)
		fputs("Committing surface.\n", stderr);
	wl_surface_set_buffer_scale(output->wl_surface, output->scale);
	wl_surface_attach(output->wl_surface, output->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(output->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->wl_surface);
}
