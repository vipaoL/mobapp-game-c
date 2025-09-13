#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include "box2d/id.h"
#include "box2d/math_functions.h"
#include <stdbool.h>

typedef enum {
	BODY_TYPE_NONE,
	BODY_TYPE_CAR_CHASSIS,
	BODY_TYPE_CAR_WHEEL,
	BODY_TYPE_LANDSCAPE,
} BodyType;

typedef struct {
	BodyType type;
} BodyData;

typedef struct {
	b2BodyId chassis;
	b2BodyId leftWheel;
	b2BodyId rightWheel;
	bool motor_on;
	int angle_deg;
	b2Vec2 position;
	b2Vec2 prev_position;
	float last_flip_x;
	int ticks_stuck;
	int damage;
	uint32_t last_damage_time;
	bool left_wheel_contacts;
	bool right_wheel_contacts;
	bool car_body_contacts;
} CarState;

typedef struct BodyNode {
	b2BodyId bodyId;
	BodyType type;
	float end_x;
	struct BodyNode* next;
} BodyNode;

typedef struct {
	b2WorldId worldId;
	BodyNode* body_list;
	int last_x;
	int last_y;
} WorldState;

#endif
