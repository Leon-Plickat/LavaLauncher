#ifndef LAVALAUNCHER_LAYERSURFACE_H
#define LAVALAUNCHER_LAYERSURFACE_H

struct Lava_data;
struct Lava_output;

bool create_bar (struct Lava_data *data, struct Lava_output *output);
void configure_surface (struct Lava_data *data, struct Lava_output *output);

#endif
