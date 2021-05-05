/*
 * LavaLauncher - A simple launcher panel for Wayland
 *
 * Copyright (C) 2020 - 2021 Leon Henrik Plickat
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
#include<unistd.h>
#include<stdbool.h>
#include<string.h>
#include<getopt.h>

#include"bar.h"
#include"item.h"
#include"config-parser.h"
#include"event-loop.h"
#include"lavalauncher.h"
#include"util.h"
#include"wayland-connection.h"
#include"signal-event-source.h"
#if WATCH_CONFIG
#include"inotify-event-source.h"
#endif

/* The context is used basically everywhere. So instead of passing pointers
 * around, just have it global.
 */
struct Lava_context context = {0};

static bool handle_command_flags (int argc, char *argv[])
{
	const char usage[] =
		"Usage: lavalauncher [options...]\n"
		"  -c <path>, --config <path> Path to config file.\n"
		"  -h,        --help          Print this help text.\n"
		"  -v,        --verbose       Enable verbose output.\n"
		"  -V,        --version       Show version.\n"
		"\n"
		"The configuration syntax is documented in the man page.\n";

	static struct option opts[] = {
		{"config",  required_argument, NULL, 'c'},
		{"help",    no_argument,       NULL, 'h'},
		{"verbose", no_argument,       NULL, 'v'},
		{"version", no_argument,       NULL, 'V'},
		{0,         0,                 0,    0  }
	};

	extern int optind;
	optind = 0;
	extern char *optarg;
	for (int c; (c = getopt_long(argc, argv, "c:hvV", opts, &optind)) != -1 ;) switch (c)
	{
		case 'c':
			set_string(&context.config_path, optarg);
			break;

		case 'h':
			fputs(usage, stderr);
			context.ret = EXIT_SUCCESS;
			return false;

		case 'v':
			context.verbosity++;
			break;

		case 'V':
			fputs("LavaLauncher version " LAVALAUNCHER_VERSION"\n", stderr);
			context.ret = EXIT_SUCCESS;
			return false;

		default:
			return false;
	}

	return true;
}

static void init_context (void)
{
	context.ret         = EXIT_FAILURE;
	context.loop        = true;
	context.reload      = false;
	context.verbosity   = 0;
	context.config_path = NULL;

#if WATCH_CONFIG
	context.watch = false;
#endif

	context.display            = NULL;
	context.registry           = NULL;
	context.sync               = NULL;

	context.compositor         = NULL;
	context.shm                = NULL;
	context.layer_shell        = NULL;
	context.xdg_output_manager = NULL;

	context.river_status_manager = NULL;
	context.need_river_status    = false;
	context.foreign_toplevel_manager = NULL;
	context.need_foreign_toplevel    = false;

	context.need_keyboard = false;
	context.need_pointer  = false;
	context.need_touch    = false;

	context.last_config    = NULL;
	context.default_config = NULL;
	context.last_item      = NULL;

	wl_list_init(&context.outputs);
	wl_list_init(&context.seats);
	wl_list_init(&context.items);
	wl_list_init(&context.configs);
	wl_list_init(&context.toplevels);
}

int main (int argc, char *argv[])
{
reload:
	init_context();

	if (! handle_command_flags(argc, argv))
		return context.ret;

	log_message(1, "[main] LavaLauncher: version=%s\n", LAVALAUNCHER_VERSION);

	/* Try to parse the configuration file. If this fails, there might
	 * already be heap objects, so some cleanup is needed.
	 */
	if (! parse_config_file())
		goto exit;
	if (! finalize_all_bar_configs())
		goto exit;
	context.item_amount = wl_list_length(&context.items);
	if ( context.item_amount == 0 )
	{
		log_message(0, "ERROR: No items configured.\n");
		goto exit;
	}

	struct Lava_event_loop loop;
	if (! event_loop_init(&loop, 3))
		goto exit;
	event_loop_add_event_source(&loop, &wayland_source);
#if WATCH_CONFIG
	if (context.watch)
		event_loop_add_event_source(&loop, &inotify_source);
#endif
	event_loop_add_event_source(&loop, &signal_source);
	context.ret = event_loop_run(&loop) ? EXIT_SUCCESS : EXIT_FAILURE;

	/* We only need to finish the event loop if it was successfully
	 * initialized, which is why we do that here instead of like the other
	 * cleanup code after the exit: label.
	 */
	event_loop_finish(&loop);

exit:
	free(context.config_path);

	destroy_all_items();
	destroy_all_bar_configs();

	if (context.reload)
		goto reload;

	return context.ret;
}

