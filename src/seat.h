#ifndef LAVALAUNCHER_SEAT_H
#define LAVALAUNCHER_SEAT_H

struct Lava_data;
struct Lava_output;

struct Lava_seat
{
	struct wl_list    link;
	struct Lava_data *data;

	struct wl_seat *wl_seat;

	struct
	{
		struct wl_pointer  *wl_pointer;
		int32_t             x;
		int32_t             y;
		struct Lava_output *output;
		struct Lava_button *button;
	} pointer;

	struct
	{
		struct wl_touch    *wl_touch;
		int32_t             id;
		struct Lava_output *output;
		struct Lava_button *button;
	} touch;
};

bool create_seat (struct Lava_data *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);

#endif
