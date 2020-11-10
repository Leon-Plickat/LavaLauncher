#ifndef LAVALAUNCHER_DIRECTIONS_T_H
#define LAVALAUNCHER_DIRECTIONS_T_H

#include<stdint.h>

typedef struct
{
	int32_t top, right, bottom, left;
} directions_t;

typedef struct
{
	uint32_t top, right, bottom, left;
} udirections_t;

#endif

