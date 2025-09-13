#ifndef WORLD_H
#define WORLD_H

#include "box2d/types.h"
#include "game_types.h"

b2BodyId worldgen_create_body(const b2BodyDef* def, BodyType type, float end_x);
void worldgen_clear_body_list(void);

void world_create_two_sided_landscape(const b2Vec2* points, int count, int end_x);
void world_generate_initial_landscape(void);
void world_generate_next_structure(void);
void world_generator_tick(void);

void world_draw_bodies(void);

#endif
