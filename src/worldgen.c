#include "worldgen.h"
#include "box2d/box2d.h"
#include "game.h"
#include "structure_placer.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int prev_structure_id = -1;
extern int zoom_out, view_field;

b2BodyId worldgen_create_body(const b2BodyDef* def, BodyType type, float end_x)
{
	b2BodyId bodyId = b2CreateBody(g_world.worldId, def);
	if (b2Body_IsValid(bodyId)) {
		BodyData* data = (BodyData*)malloc(sizeof(BodyData));
		if (data) {
			data->type = type;
			b2Body_SetUserData(bodyId, data);
		}

		BodyNode* newNode = (BodyNode*)malloc(sizeof(BodyNode));
		if (newNode) {
			newNode->bodyId = bodyId;
			newNode->type = type;
			newNode->end_x = end_x;
			newNode->next = g_world.body_list;
			g_world.body_list = newNode;
		}
	}
	return bodyId;
}

void worldgen_clear_body_list(void)
{
	BodyNode* current = g_world.body_list;
	while (current != NULL) {
		BodyNode* next = current->next;
		if(b2Body_IsValid(current->bodyId)) {
			void* userData = b2Body_GetUserData(current->bodyId);
			if (userData) {
				free(userData);
			}
		}
		free(current);
		current = next;
	}
	g_world.body_list = NULL;
}

void world_create_two_sided_landscape(const b2Vec2* points, int count, int end_x)
{
	if (count < 2) return;

	b2BodyDef groundBodyDef = b2DefaultBodyDef();
	b2BodyId groundBodyId = worldgen_create_body(&groundBodyDef, BODY_TYPE_LANDSCAPE, end_x/(float)WORLD_SCALE);

	b2ChainDef topChainDef = b2DefaultChainDef();
	b2Vec2 points_with_dummy_ghosts[count + 2];
	points_with_dummy_ghosts[0] = points[0]; // ghost point
	for (int i = 0; i < count; ++i) {
		points_with_dummy_ghosts[1 + i] = points[i];
	}
	points_with_dummy_ghosts[count + 1] = points[count - 1]; // ghost point
	topChainDef.points = points_with_dummy_ghosts;
	topChainDef.count = count + 2;
	b2CreateChain(groundBodyId, &topChainDef);

	b2Vec2 bottom_points_with_dummy_ghosts[count + 2];
	bottom_points_with_dummy_ghosts[0] = points[count - 1]; // ghost point
	for (int i = 0; i < count; ++i) {
		bottom_points_with_dummy_ghosts[1 + i] = points[count - 1 - i];
	}
	bottom_points_with_dummy_ghosts[count + 1] = points[0]; // ghost point

	b2ChainDef bottomChainDef = b2DefaultChainDef();
	bottomChainDef.points = bottom_points_with_dummy_ghosts;
	bottomChainDef.count = count + 2;
	b2CreateChain(groundBodyId, &bottomChainDef);
}

void world_generate_initial_landscape(void)
{
	g_world.last_x = -2900;
	g_world.last_y = 0;

	int border_start_x = g_world.last_x - 600;
	int border_start_y = g_world.last_y - 100;

	int start_platform_end_x = g_world.last_x + 1000;

	b2Vec2 points[] = {
		{border_start_x / WORLD_SCALE, border_start_y / WORLD_SCALE},
		{g_world.last_x / WORLD_SCALE, g_world.last_y / WORLD_SCALE},
		{start_platform_end_x / WORLD_SCALE, g_world.last_y / WORLD_SCALE},
	};

	world_create_two_sided_landscape(points, 3, start_platform_end_x);
	g_world.last_x = start_platform_end_x;
}

void world_generate_next_structure(void)
{
	EndPoint ep = {g_world.last_x, g_world.last_y};
	int id;

	do {
		id = rand() % 10;
	} while (id == prev_structure_id);

	if (g_world.last_y < -1000 || 1000 < g_world.last_y) {
		ep = place_floor_struct(g_world.last_x, g_world.last_y, 1000 + rand() % 4 * 100, (rand() % 7 - 3) * 100);
	} else {
		switch (id)
		{
		case STRUCTURE_ID_ARC1:
			int halfPeriods = 4 + rand() % 8;
			int l = halfPeriods * 180;
			int amp = 15;
			ep = place_sin_struct(g_world.last_x, g_world.last_y, l, halfPeriods, 0, amp);
			break;
		case STRUCTURE_ID_SIN:
			ep = place_arc1_struct(g_world.last_x, g_world.last_y, 200 + abs(rand()) % 400);
			break;
		case STRUCTURE_ID_FLOOR_STAT:
			ep = place_horizontal_floor_struct(g_world.last_x, g_world.last_y, 400 + rand() % 10 * 100);
			break;
		case STRUCTURE_ID_ARC2:
			ep = place_arc2_struct(g_world.last_x, g_world.last_y, 500 + rand() % 500, 20);
			break;
		case STRUCTURE_ID_ABYSS:
			ep = place_abyss_struct(g_world.last_x, g_world.last_y, rand() % 6 * 1000);
			break;
		case STRUCTURE_ID_SLANTED_DOTTED_LINE:
			int n = rand() % 6 + 5;
			ep = place_slanted_dotted_line_struct(g_world.last_x, g_world.last_y, n);
			break;
		default:
			ep = place_floor_struct(g_world.last_x, g_world.last_y, 400 + rand() % 10 * 100, (rand() % 7 - 3) * 100);
			break;
		}
	}

	g_world.last_x = ep.x;
	g_world.last_y = ep.y;
}

static void remove_old_structures(void)
{
	BodyNode** current_ptr = &g_world.body_list;

	while (*current_ptr) {
		BodyNode* entry = *current_ptr;
		if (entry->type == BODY_TYPE_LANDSCAPE && entry->end_x < g_car.position.x - view_field * 2) {
			void* userData = b2Body_GetUserData(entry->bodyId);
			if (userData) {
				free(userData);
			}
			b2DestroyBody(entry->bodyId);
			*current_ptr = entry->next;
			free(entry);
		} else {
			current_ptr = &entry->next;
		}
	}
}

void world_generator_tick(void)
{
	if (g_car.position.x + view_field*2 / WORLD_SCALE > g_world.last_x / WORLD_SCALE) {
		world_generate_next_structure();
	}

	remove_old_structures();
}
