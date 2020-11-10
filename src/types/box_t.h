#ifndef LAVALAUNCHER_BOX_T_H
#define LAVALAUNCHER_BOX_T_H

#include<stdint.h>

typedef struct
{
	int32_t x, y, w, h;
} box_t;

typedef struct
{
	uint32_t x, y, w, h;
} ubox_t;

#endif

