#ifndef STRUCTURE_PLACER_H
#define STRUCTURE_PLACER_H

#include "game_types.h"

#define STRUCTURE_ID_ARC1 0
#define STRUCTURE_ID_SIN 1
#define STRUCTURE_ID_FLOOR_STAT 2
#define STRUCTURE_ID_ARC2 3
#define STRUCTURE_ID_ABYSS 4
#define STRUCTURE_ID_SLANTED_DOTTED_LINE 5

#define BUILTIN_STRUCTS_NUMBER 6
#define FLOOR_RANDOM_WEIGHT 4

typedef struct {
	int x;
	int y;
} EndPoint;

EndPoint place_arc1_struct(int x, int y, int r);

EndPoint place_sin_struct(int x, int y, int l, int half_periods, int offset, int amp);

EndPoint place_horizontal_floor_struct(int x, int y, int l);

EndPoint place_arc2_struct(int x, int y, int r, int sn);

EndPoint place_abyss_struct(int x, int y, int l);

EndPoint place_slanted_dotted_line_struct(int x, int y, int n);

EndPoint place_floor_struct(int x, int y, int l, int y2);

#endif
