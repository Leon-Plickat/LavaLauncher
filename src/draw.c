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

#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<unistd.h>
#include<cairo/cairo.h>
#include<wayland-server.h>

#include"lavalauncher.h"
#include"output.h"
#include"seat.h"
#include"draw.h"
#include"button.h"

static void draw_effect (cairo_t *cairo, int32_t x, int32_t y, int32_t size,
		float colour[4], enum Draw_effect effect)
{
	if ( effect == EFFECT_NONE )
		return;

	cairo_save(cairo);

	x += 0.08 * size, y += 0.08 * size, size = size * 0.84;
	double radius, degrees;
	switch (effect)
	{
		case EFFECT_BOX:
			cairo_rectangle(cairo, x, y, size, size);
			break;

		case EFFECT_PHONE:
			radius = size / 10.0;
			degrees = 3.1415927 / 180.0;

			cairo_new_sub_path(cairo);
			cairo_arc(cairo, x + size - radius, y + radius, radius, -90 * degrees, 0 * degrees);
			cairo_arc(cairo, x + size - radius, y + size - radius, radius, 0 * degrees, 90 * degrees);
			cairo_arc(cairo, x + radius, y + size - radius, radius, 90 * degrees, 180 * degrees);
			cairo_arc(cairo, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);
			cairo_close_path(cairo);
			break;

		default:
			break;
	}

	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba(cairo, colour[0], colour[1], colour[2], colour[3]);
	cairo_fill(cairo);

	cairo_restore(cairo);
}

static void draw_icon (cairo_t *cairo, int32_t x, int32_t y,
		int32_t icon_size, cairo_surface_t *img)
{
	if ( img == NULL )
		return;

	float w = cairo_image_surface_get_width(img);
	float h = cairo_image_surface_get_height(img);

	cairo_save(cairo);
	cairo_translate(cairo, x, y);
	cairo_scale(cairo, icon_size / w, icon_size / h);
	cairo_set_source_surface(cairo, img, 0, 0);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

static void draw_buttons (cairo_t *cairo, struct Lava_data *data,
		int32_t x_offset, int32_t y_offset, float scale)
{
	x_offset *= scale, y_offset *= scale;
	int32_t size = data->icon_size * scale;
	int32_t x = x_offset, y = y_offset, *increment, *increment_offset;
	if ( data->orientation == ORIENTATION_HORIZONTAL )
		increment = &x, increment_offset = &x_offset;
	else
		increment = &y, increment_offset = &y_offset;

	struct Lava_button *bt_1, *bt_2;
	wl_list_for_each_reverse_safe(bt_1, bt_2, &data->buttons, link)
	{
		if ( bt_1->type == TYPE_BUTTON )
		{
			*increment = (bt_1->ordinate * scale) + *increment_offset;
			draw_effect(cairo, x, y, size, data->effect_colour, data->effect);
			draw_icon(cairo, x, y, size, bt_1->img);
		}
	}
}

/* Draw the bar background plus border. */
static void draw_bar (cairo_t *cairo, int32_t x, int32_t y, int32_t w, int32_t h,
		int32_t border_top, int32_t border_right,
		int32_t border_bottom, int32_t border_left,
		float scale, float center_colour[4], float border_colour[4])
{
	/* Scale. */
	x *= scale, y *= scale, w *= scale, h *= scale;
	border_top *= scale, border_bottom *= scale;
	border_left *= scale, border_right *= scale;

	/* Calculate dimensions of center. */
	int32_t cx = x + border_left,
		cy = y + border_top,
		cw = w - border_left - border_right,
		ch = h - border_top  - border_bottom;

	/* Draw center. */
	cairo_rectangle(cairo, cx, cy, cw, ch);
	cairo_set_source_rgba(cairo, center_colour[0], center_colour[1],
			center_colour[2], center_colour[3]);
	cairo_fill(cairo);

	/* Draw borders. Top - Right - Bottom - Left */
	cairo_rectangle(cairo, x, y, w, border_top);
	cairo_rectangle(cairo, x + w - border_right, y + border_top,
			border_right, h - border_top - border_bottom);
	cairo_rectangle(cairo, x, y + h - border_bottom, w, border_bottom);
	cairo_rectangle(cairo, x, y + border_top, border_left,
			h - border_top - border_bottom);
	cairo_set_source_rgba(cairo, border_colour[0], border_colour[1],
			border_colour[2], border_colour[3]);
	cairo_fill(cairo);
}

static void clear_cairo_buffer (cairo_t *cairo)
{
	cairo_save(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
	cairo_paint(cairo);
	cairo_restore(cairo);
}

static void calculate_buffer_size (struct Lava_data *data, struct Lava_output *output,
		int *w, int *h)
{
	if ( data->mode == MODE_SIMPLE )
		*w = data->w, *h = data->h;
	else if ( data->orientation == ORIENTATION_HORIZONTAL )
		*w = output->w, *h = data->h;
	else
		*w = data->w, *h = output->h;

	*w *= output->scale, *h *= output->scale;
}

void render_bar_frame (struct Lava_data *data, struct Lava_output *output)
{
	if ( output->status != OUTPUT_STATUS_SURFACE_CONFIGURED )
		return;

	/* Get new/next buffer. */
	int buffer_w, buffer_h;
	calculate_buffer_size(data, output, &buffer_w, &buffer_h);
	if (! next_buffer(&output->current_buffer, data->shm, output->buffers,
				buffer_w, buffer_h))
		return;

	cairo_t *cairo = output->current_buffer->cairo;
	clear_cairo_buffer(cairo);

	/* Draw bar. */
	if (data->verbose)
		fputs("Drawing bar.\n", stderr);
	if ( data->mode == MODE_FULL )
	{
		int bar_w, bar_h;
		if ( data->orientation == ORIENTATION_HORIZONTAL )
			bar_w = output->w, bar_h = data->h;
		else
			bar_w = data->w, bar_h = output->h;

		draw_bar(cairo, 0, 0, bar_w, bar_h,
				data->border_top, data->border_right,
				data->border_bottom, data->border_left,
				output->scale,
				data->bar_colour, data->border_colour);
	}
	else
		draw_bar(cairo, output->bar_x_offset, output->bar_y_offset,
				data->w, data->h,
				data->border_top, data->border_right,
				data->border_bottom, data->border_left,
				output->scale,
				data->bar_colour, data->border_colour);

	/* Draw icons. */
	if (data->verbose)
		fputs("Drawing icons.\n", stderr);
	draw_buttons(cairo, data, output->bar_x_offset + data->border_left,
			output->bar_y_offset + data->border_top,
			output->scale);

	/* Commit surface. */
	if (data->verbose)
		fputs("Committing surface.\n", stderr);
	wl_surface_set_buffer_scale(output->wl_surface, output->scale);
	wl_surface_attach(output->wl_surface, output->current_buffer->buffer, 0, 0);
	wl_surface_damage_buffer(output->wl_surface, 0, 0, INT32_MAX, INT32_MAX);
	wl_surface_commit(output->wl_surface);
}
