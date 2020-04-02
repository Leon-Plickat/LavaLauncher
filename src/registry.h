#ifndef LAVALAUNCHER_REGISTRY_H
#define LAVALAUNCHER_REGISTRY_H

struct Lava_data;

bool init_wayland (struct Lava_data *data);
void finish_wayland (struct Lava_data *data);

#endif
