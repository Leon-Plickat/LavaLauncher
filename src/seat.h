#ifndef LAVALAUNCHER_SEAT_H
#define LAVALAUNCHER_SEAT_H

#include"lavalauncher.h"

bool create_seat (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

#endif
