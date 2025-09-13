#ifndef GAME_H
#define GAME_H

#include "graphics.h"
#include "game_types.h"

#define WORLD_SCALE 100.0f  // 100.0 emini units = 1.0 Box2D meter

// #define DEBUG_PIXELS_PER_METER 2.0f
// #define DEBUG_SHOW_FPS

#ifdef DEBUG_PIXELS_PER_METER
	#define PIXELS_PER_METER (DEBUG_PIXELS_PER_METER)
#else
	#define PIXELS_PER_METER (WORLD_SCALE * 1000.0f / zoom_out)
#endif

extern bool g_is_paused;
extern GraphicsContext screen_context;
extern CarState g_car;
extern WorldState g_world;

void game_init(void);
void game_destroy(void);
void game_update(int dt);
void game_draw(GraphicsContext* ctx);
void update_screen_size(int w, int h);

void game_handle_keydown_default(void);
void game_handle_keyup_default(void);
void game_print_debug(void);

vec2d world_to_screen(b2Vec2 worldPoint);

static inline float clamp(float value, float min_val, float max_val) {
	if (value < min_val) return min_val;
	if (value > max_val) return max_val;
	return value;
}

#define CONSTRAIN(min_a, a, max_a) ((a) < (min_a) ? (min_a) : ((a) < (max_a) ? (a) : (max_a)))

#endif
