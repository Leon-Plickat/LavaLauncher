#ifndef LAVALAUNCHER_OUTPUT_H
#define LAVALAUNCHER_OUTPUT_H

#include"lavalauncher.h"

bool create_output (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

#endif
