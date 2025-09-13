#ifndef CAR_H
#define CAR_H

#include "box2d/id.h"
#include "box2d/math_functions.h"

extern int car_ticks_flying;

void car_create(b2WorldId worldId, b2Vec2 position);
void car_update_state(void);
void car_update_controls(int dt);
void car_check_contacts(void);

#endif
