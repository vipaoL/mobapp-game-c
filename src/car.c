#include "car.h"
#include "box2d/box2d.h"
#include "game.h"
#include "worldgen.h"
#include <stdio.h>
#include <math.h>

#define SPEED_THRESHOLD_HIGH 250
#define SPEED_THRESHOLD_LOW 100
#define ANGULAR_VELOCITY_THRESHOLD -7

#define POWER_HIGH 200
#define POWER_MEDIUM 100
#define POWER_LOW 10

#define TORQUE_IN_AIR -30
#define TORQUE_SELF_RIGHT -100
#define TORQUE_CORRECTION -50

#define BRAKE_MS 2000;
#define BRAKE_MULTIPLIER -7
#define BRAKE_TORQUE_MULTIPLIER -15

// #define DEBUG_SIMULATUION_MODE

int car_ticks_flying;
int car_brake_timer;

void car_create(b2WorldId worldId, b2Vec2 position) {
	float car_body_length = 240.0f / WORLD_SCALE;
	float car_body_height = 40.0f / WORLD_SCALE;
	float wheel_radius = 40.0f / WORLD_SCALE;

	float chassis_area = car_body_length * car_body_height;
	float wheel_area = M_PI * wheel_radius * wheel_radius;

	b2BodyDef bodyDef = b2DefaultBodyDef();
#ifndef DEBUG_SIMULATUION_MODE
	bodyDef.type = b2_dynamicBody;
#endif
	bodyDef.position = position;
	g_car.chassis = worldgen_create_body(&bodyDef, BODY_TYPE_CAR_CHASSIS, bodyDef.position.x);

	b2Polygon car_shape = b2MakeBox(car_body_length / 2.0f, car_body_height / 2.0f);
	b2ShapeDef shapeDef = b2DefaultShapeDef();
	shapeDef.density = 1.0f / chassis_area;
	shapeDef.material.friction = 0.0f;
	b2CreatePolygonShape(g_car.chassis, &shapeDef, &car_shape);

	b2Circle wheel_shape = {.radius = wheel_radius};
	shapeDef.density = 2.0f / wheel_area;
	shapeDef.material.friction = 0.0f;

	b2BodyDef wheelBodyDef = b2DefaultBodyDef();
	wheelBodyDef.type = b2_dynamicBody;

	wheelBodyDef.position = b2Body_GetWorldPoint(g_car.chassis, (b2Vec2){-1.0f, 0.35f});
	g_car.leftWheel = worldgen_create_body(&wheelBodyDef, BODY_TYPE_CAR_WHEEL, wheelBodyDef.position.x);
	b2CreateCircleShape(g_car.leftWheel, &shapeDef, &wheel_shape);

	wheelBodyDef.position = b2Body_GetWorldPoint(g_car.chassis, (b2Vec2){1.0f, 0.35f});
	g_car.rightWheel = worldgen_create_body(&wheelBodyDef, BODY_TYPE_CAR_WHEEL, wheelBodyDef.position.x);
	b2CreateCircleShape(g_car.rightWheel, &shapeDef, &wheel_shape);

	// joints
	b2WeldJointDef jointDef = b2DefaultWeldJointDef();
	jointDef.base.bodyIdA = g_car.chassis;

	float joint_x_anchor = car_body_length / 2.0f - wheel_radius;
	float joint_y_anchor = wheel_radius * 2.0f / 3.0f;

	jointDef.base.bodyIdB = g_car.leftWheel;
	jointDef.base.localFrameA.p = (b2Vec2){-joint_x_anchor, joint_y_anchor};
	jointDef.base.localFrameB.p = (b2Vec2){0.0f, 0.0f};
	jointDef.linearHertz = 0.0f;
	jointDef.angularHertz = 0.0f;
	b2CreateWeldJoint(worldId, &jointDef);

	jointDef.base.bodyIdB = g_car.rightWheel;
	jointDef.base.localFrameA.p = (b2Vec2){joint_x_anchor, joint_y_anchor};
	b2CreateWeldJoint(worldId, &jointDef);
}

void car_update_state(void) {
	g_car.prev_position = g_car.position;
	g_car.position = b2Body_GetPosition(g_car.chassis);
	g_car.angle_deg = b2Rot_GetAngle(b2Body_GetRotation(g_car.chassis)) * 180.0f / M_PI;
	while (g_car.angle_deg < 0) g_car.angle_deg += 360;
	while (g_car.angle_deg >= 360) g_car.angle_deg -= 360;
}

static bool check_contacts(b2BodyId bodyId) {
	int capacity = b2Body_GetContactCapacity(bodyId);
	b2ContactData contact_data[capacity];

	int count = b2Body_GetContactData(bodyId, contact_data, capacity);

	if (count > capacity) {
		printf("AAAAAAAAAAAA\n");
	}

	for (int i = 0; i < count; ++i) {
		if (contact_data[i].manifold.pointCount > 0) {
			return true;
		}
	}

	return false;
}

void car_check_contacts(void) {
	g_car.left_wheel_contacts = check_contacts(g_car.leftWheel);
	g_car.right_wheel_contacts = check_contacts(g_car.rightWheel);
	g_car.car_body_contacts = check_contacts(g_car.chassis);
	car_ticks_flying = (g_car.left_wheel_contacts || g_car.right_wheel_contacts) ? 0 : car_ticks_flying + 1;
}

void car_update_controls(int dt) {
#ifdef DEBUG_SIMULATUION_MODE
	b2Body_SetTransform(g_car.chassis, b2Add(g_car.position, (b2Vec2){1.0f, 0}), b2MakeRot(b2Rot_GetAngle(b2Body_GetRotation(g_car.chassis)) + -0.1f));
	return;
#endif
	bool on_ground = car_ticks_flying <= 1;

	if (g_car.motor_on) {
		car_brake_timer = BRAKE_MS;

		if (on_ground) {
			float power_level;
			b2Vec2 velocity = b2Body_GetLinearVelocity(g_car.chassis);
			float speed_sqr = b2Dot(velocity, velocity);

			if (speed_sqr > SPEED_THRESHOLD_HIGH) {
				power_level = POWER_LOW;
			} else if (speed_sqr > SPEED_THRESHOLD_LOW) {
				power_level = POWER_MEDIUM;
			} else {
				power_level = POWER_HIGH;
			}

			float angle_rad = b2Rot_GetAngle(b2Body_GetRotation(g_car.chassis));
			float force_angle = angle_rad;// - (15.0f * M_PI / 180.0f);
			b2Vec2 force_dir = {cosf(force_angle), sinf(force_angle)};
			b2Vec2 force = b2MulSV(power_level, force_dir);
			b2Body_ApplyForceToCenter(g_car.chassis, force, true);

			if (g_car.right_wheel_contacts || (!g_car.left_wheel_contacts && g_car.car_body_contacts)) {
				b2Body_ApplyTorque(g_car.chassis, TORQUE_CORRECTION, true);
			}

		} else {
			float torque = TORQUE_IN_AIR;
			if (g_car.car_body_contacts && g_car.angle_deg > 120 && g_car.angle_deg < 300) {
				torque *= 3.0f;
			}
			if (b2Body_GetAngularVelocity(g_car.chassis) > ANGULAR_VELOCITY_THRESHOLD) {
				b2Body_ApplyTorque(g_car.chassis, torque, true);
			}
		}

	} else {
		if (car_brake_timer > 0) {
			float angular_velocity = b2Body_GetAngularVelocity(g_car.chassis);
			if (angular_velocity < 0) {
				b2Body_ApplyTorque(g_car.chassis, BRAKE_TORQUE_MULTIPLIER * angular_velocity, true);
			}

			if (on_ground) {
				b2Vec2 velocity = b2Body_GetLinearVelocity(g_car.chassis);
				b2Body_ApplyForceToCenter(g_car.chassis, b2MulSV(BRAKE_MULTIPLIER, velocity), true);
			}

			car_brake_timer -= dt;
			if(on_ground) {
				car_brake_timer -= dt;
			}
		}
	}
}
