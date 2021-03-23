/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2021 Leon Henrik Plickat
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

#include<stdlib.h>
#include<stdbool.h>
#include<signal.h>
#include<unistd.h>
#include<poll.h>
#include<string.h>
#include<errno.h>

#include<wayland-client.h>

#include"wlr-foreign-toplevel-management-unstable-v1-protocol.h"

#include"lavalauncher.h"
#include"foreign-toplevel-management.h"
#include"output.h"
#include"bar.h"
#include"item.h"
#include"util.h"

void destroy_toplevel (struct Lava_toplevel *toplevel)
{
	free_if_set(toplevel->pending.app_id);
	free_if_set(toplevel->current.app_id);
	DESTROY(toplevel->handle, zwlr_foreign_toplevel_handle_v1_destroy);
	wl_list_remove(&toplevel->link);
	free(toplevel);
}

static void toplevel_handle_handle_app_id (void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
		const char *app_id)
{
	struct Lava_toplevel *toplevel = (struct Lava_toplevel *)data;
	free_if_set(toplevel->pending.app_id);
	toplevel->pending.app_id = strdup(app_id);
}

static void toplevel_handle_handle_state (void *data, struct zwlr_foreign_toplevel_handle_v1 *handle,
		struct wl_array *states)
{
	struct Lava_toplevel *toplevel = (struct Lava_toplevel *)data;

	toplevel->pending.activated  = false;

	uint32_t *state;
	wl_array_for_each(state, states)
		if ( *state == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED )
		{
			toplevel->pending.activated = true;
			return;
		}
}

static void toplevel_cleanup_indicators (struct Lava_toplevel *toplevel)
{
	struct Lava_output *output;
	wl_list_for_each(output, &context.outputs, link)
	{
		if ( output->bar_instance == NULL )
			continue;

		bool need_frame = false;
		for (int i = 0; i < output->bar_instance->active_items; i++)
		{
			if ( output->bar_instance->item_instances[i].item->associated_app_id == NULL )
				continue;
			if ( output->bar_instance->item_instances[i].item->type != TYPE_BUTTON )
				continue;

			if ( strcmp(output->bar_instance->item_instances[i].item->associated_app_id,
					toplevel->current.app_id) == 0 )
			{
				counter_safe_subtract(&(output->bar_instance->item_instances[i].toplevel_exists_indicator), 1);
				if (toplevel->current.activated)
					counter_safe_subtract(&(output->bar_instance->item_instances[i].toplevel_activated_indicator), 1);
				output->bar_instance->item_instances[i].dirty = true;
				need_frame = true;
			}
		}

		if (need_frame)
			bar_instance_schedule_frame(output->bar_instance);
	}
}

static void toplevel_update_indicators (struct Lava_toplevel *toplevel,
		bool app_id_changed, bool activated_changed)
{
	if ( ! app_id_changed && ! activated_changed )
		return;
	struct Lava_output *output;
	wl_list_for_each(output, &context.outputs, link)
	{
		if ( output->bar_instance == NULL )
			continue;

		bool need_frame = false;
		for (int i = 0; i < output->bar_instance->active_items; i++)
		{
			if ( output->bar_instance->item_instances[i].item->associated_app_id == NULL )
				continue;
			if ( output->bar_instance->item_instances[i].item->type != TYPE_BUTTON )
				continue;

			if ( strcmp(output->bar_instance->item_instances[i].item->associated_app_id,
					toplevel->current.app_id) == 0 )
			{
				if (app_id_changed)
				{
					output->bar_instance->item_instances[i].toplevel_exists_indicator++;
					if (toplevel->current.activated)
						output->bar_instance->item_instances[i].toplevel_activated_indicator++;
					output->bar_instance->item_instances[i].dirty = true;
					need_frame = true;
				}
				else if (activated_changed)
				{
					if (toplevel->current.activated)
						output->bar_instance->item_instances[i].toplevel_activated_indicator++;
					else
						counter_safe_subtract(&(output->bar_instance->item_instances[i].toplevel_activated_indicator), 1);
					output->bar_instance->item_instances[i].dirty = true;
					need_frame = true;
				}
			}
		}

		if (need_frame)
			bar_instance_schedule_frame(output->bar_instance);
	}
}

static void toplevel_handle_handle_done (void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	struct Lava_toplevel *toplevel = (struct Lava_toplevel *)data;

	/* Apply pending app_id. Behold: strcmp() is not necessarily NULL safe,
	 * hence the slightly more complicated code here.
	 */
	bool app_id_changed = false;
	if ( toplevel->pending.app_id != NULL && toplevel->current.app_id == NULL )
	{
		toplevel->current.app_id = toplevel->pending.app_id;
		toplevel->pending.app_id = NULL;

		app_id_changed = true;
	}
	else if ( toplevel->pending.app_id != NULL && toplevel->current.app_id != NULL )
	{
		if ( strcmp(toplevel->pending.app_id, toplevel->current.app_id) != 0 )
		{
			/* Cleanup indicators of item instances  associated
			 * with current app-id
			 */
			toplevel_cleanup_indicators(toplevel);

			free(toplevel->current.app_id);

			toplevel->current.app_id = toplevel->pending.app_id;
			toplevel->pending.app_id = NULL;

			app_id_changed = true;
		}
	}
	/* The case of the pending app_id being NULL (meaning app_id did not
	 * change) or even both pending and current app_id being NULL need no handling.
	 */

	/* No need to continue if there is no app_id. */
	if ( toplevel->current.app_id == NULL )
		return;

	/* Apply pending activated state. */
	bool activated_changed = false;
	if ( toplevel->pending.activated != toplevel->current.activated )
	{
		toplevel->current.activated = toplevel->pending.activated;
		activated_changed = true;
	}

	/* Update indicators. */
	toplevel_update_indicators(toplevel, app_id_changed, activated_changed);
}

static void toplevel_handle_handle_closed (void *data, struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	log_message(1, "[toplevel] Tolevel closing.\n");
	struct Lava_toplevel *toplevel = (struct Lava_toplevel *)data;
	if ( toplevel->current.app_id != NULL )
		toplevel_cleanup_indicators(toplevel);
	destroy_toplevel(toplevel);
}

static void noop () {}

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_handle_listener = {
	.title        = noop,
	.app_id       = toplevel_handle_handle_app_id,
	.output_enter = noop,
	.output_leave = noop,
	.state        = toplevel_handle_handle_state,
	.done         = toplevel_handle_handle_done,
	.closed       = toplevel_handle_handle_closed,
	.parent       = noop
};

static void toplevel_manager_handle_toplevel (void *data, struct zwlr_foreign_toplevel_manager_v1 *manager,
		struct zwlr_foreign_toplevel_handle_v1 *handle)
{
	log_message(1, "[toplevel] New toplevel.\n");

	NEW(struct Lava_toplevel, toplevel);
	zwlr_foreign_toplevel_handle_v1_add_listener(handle, &toplevel_handle_listener, toplevel);

	toplevel->handle = handle;

	toplevel->current.app_id    = NULL;
	toplevel->current.activated = false;

	toplevel->pending.app_id    = NULL;
	toplevel->pending.activated = false;

	wl_list_insert(&context.toplevels, &toplevel->link);
}

static void toplevel_manager_handle_finished (void *data, struct zwlr_foreign_toplevel_manager_v1 *manager)
{
	DESTROY_NULL(context.foreign_toplevel_manager, zwlr_foreign_toplevel_manager_v1_destroy);
}

static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_listener = {
	.toplevel = toplevel_manager_handle_toplevel,
	.finished = toplevel_manager_handle_finished
};

void init_foreign_toplevel_management (void)
{
	zwlr_foreign_toplevel_manager_v1_add_listener(context.foreign_toplevel_manager,
			&toplevel_manager_listener, NULL);
}

struct Lava_toplevel *find_toplevel_with_app_id (const char *app_id)
{
	if ( app_id == NULL )
		return NULL;
	struct Lava_toplevel *toplevel;
	wl_list_for_each(toplevel, &context.toplevels, link)
		if ( toplevel->current.app_id != NULL && strcmp(app_id, toplevel->current.app_id) == 0 )
			return toplevel;
	return NULL;
}

