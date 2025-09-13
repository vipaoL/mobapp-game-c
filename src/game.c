#include "car.h"
#include "game.h"
#include "box2d/box2d.h"
#include "worldgen.h"
#include "compat.h"

#include <string.h>
#include <stdio.h>

#define POINTS_DIVIDER 2000

#define GAME_OVER_DAMAGE 8
#define GAME_OVER_STUCK_TIME_MS 1000
#define DAMAGE_COOLDOWN_MS 200
#define NO_COLOR 0x1000000

bool g_is_paused;
int score;
int flip_indicator;
int flip_state;
int backflip_count;
bool prev_flip_dir;
int next_score_target_x;
int tick;
int debug_text_offset;
int frame_count = 0;
uint32_t fps_last_measured_time = 0;
int fps;
int last_tick_time;
int screen_width;
int screen_height;
int screen_min_side;
int zoom_out;
int view_field;
int offset_x;
int offset_y;
CarState g_car;
WorldState g_world;

void game_init(void)
{
	if (b2World_IsValid(g_world.worldId)) {
		b2DestroyWorld(g_world.worldId);
	}
	worldgen_clear_body_list();

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, 10.0f};
	g_world.worldId = b2CreateWorld(&worldDef);

	memset(&g_car, 0, sizeof(CarState));
	g_car.last_flip_x = -9999.0f;

	flip_state = 0;
	backflip_count = 0;
	prev_flip_dir = false;
	flip_indicator = 255;
	g_is_paused = false;

	car_create(g_world.worldId, (b2Vec2){-3000.0f / WORLD_SCALE, -400.0f / WORLD_SCALE});
	world_generate_initial_landscape();

	b2Vec2 car_pos = b2Body_GetPosition(g_car.chassis);

	score = 0;
	next_score_target_x = (car_pos.x * WORLD_SCALE) + POINTS_DIVIDER;
}

void game_destroy(void)
{
	worldgen_clear_body_list();
}

static void update_camera(void)
{
	float f = g_car.position.y / 10.0f - 1;
	if (f < 0) {
		f = -f;
	}
	zoom_out = WORLD_SCALE * 10000 * (f + 2) / screen_min_side;

#ifdef SLOW_CAMERA
	b2Vec2 delta = b2Sub(g_car.position, (b2Vec2){g_world.camera_x, g_world.camera_y});
	float follow_speed = 0.001f;
	g_world.camera_x += delta.x * delta.x * delta.x * follow_speed;
	g_world.camera_y += delta.y * delta.y * delta.y * follow_speed;
#else
	offset_x = -g_car.position.x * PIXELS_PER_METER + screen_width / 3;
	offset_y = -g_car.position.y * PIXELS_PER_METER + screen_height * 2 / 3;
	offset_y += g_car.position.y * screen_min_side / 20000;
	offset_y = CONSTRAIN(-g_car.position.y * PIXELS_PER_METER + screen_height/16, offset_y, -g_car.position.y * PIXELS_PER_METER + screen_height*4/5);
#endif
	view_field = screen_width * zoom_out / 1000;
}

static bool update_damage_and_gameover(void)
{
	if (sys_timer_ms() - g_car.last_damage_time < DAMAGE_COOLDOWN_MS) {
		return false;
	}

	bool upside_down = (g_car.angle_deg > 140 && g_car.angle_deg < 220);

	if ((upside_down && g_car.car_body_contacts) || g_car.position.y > 20.0f) {
		g_car.last_damage_time = sys_timer_ms();
		if (g_car.damage < GAME_OVER_DAMAGE) {
			g_car.damage++;
		} else {
			return true;
		}
	} else {
		g_car.last_damage_time = sys_timer_ms();
		if (g_car.damage > 0) {
			g_car.damage--;
		}
	}

	if (g_car.motor_on && b2DistanceSquared(g_car.prev_position, g_car.position) < 0.001f) {
		g_car.ticks_stuck++;
		if (g_car.ticks_stuck * DAMAGE_COOLDOWN_MS > GAME_OVER_STUCK_TIME_MS) {
			// car_destroy(); // not implemented yet
			return true;
		}
	} else {
		g_car.ticks_stuck = 0;
	}
	return false;
}

static void update_flips()
{
	if (flip_indicator < 255) {
		flip_indicator += 64;
		if (flip_indicator > 255) flip_indicator = 255;
	}

	if (car_ticks_flying < 1) {
		flip_state = 0;
		backflip_count = 0;
		return;
	}

	bool flip_dir = b2Body_GetAngularVelocity(g_car.chassis) > 0;

	if (flip_dir != prev_flip_dir) {
		flip_state = 0;
		backflip_count = 0;
	}
	prev_flip_dir = flip_dir;

	switch (flip_state) {
		case 0:
			if (80 < g_car.angle_deg && g_car.angle_deg < 100) {
				flip_state = flip_dir ? 1 : 3;
#ifdef DEBUG_FLIP_COUNTER
				printf("from %d to %d\n", 0, flip_state);
#endif
			}
			break;

		case 1:
			if (170 < g_car.angle_deg && g_car.angle_deg < 190) {
				flip_state = 2;
#ifdef DEBUG_FLIP_COUNTER
				printf("from %d to %d\n", 1, flip_state);
#endif
			}
			break;

		case 2:
			if (260 < g_car.angle_deg && g_car.angle_deg < 280) {
				flip_state = flip_dir ? 3 : 1;
#ifdef DEBUG_FLIP_COUNTER
				printf("from %d to %d\n", 2, flip_state);
#endif
			}
			break;

		case 3:
			if (350 < g_car.angle_deg || g_car.angle_deg < 10) {
				if (flip_dir) {
					score++;
					flip_indicator = 0;
				} else {
					backflip_count++;
					if (backflip_count >= 2) {
						score++;
						flip_indicator = 0;
						backflip_count = 0;
					}
				}
				flip_state = 0;
#ifdef DEBUG_FLIP_COUNTER
				printf("from %d to %d\n", 3, flip_state);
#endif
			}
			break;
	}
}


static void update_distance_score(void)
{
	int car_x_units = g_car.position.x * WORLD_SCALE;
	if (car_x_units > next_score_target_x) {
		score++;
		next_score_target_x += POINTS_DIVIDER;
	}
}

void game_update(int dt)
{
	last_tick_time = dt;
	frame_count++;
	uint32_t current_time = sys_timer_ms();
	if (current_time - fps_last_measured_time >= 1000) {
		fps = frame_count;
		frame_count = 0;
		fps_last_measured_time = current_time;
	}

	b2World_Step(g_world.worldId, dt / 1000.0f, 8);

	car_update_state();
	car_check_contacts();
	car_update_controls(dt);

	if (update_damage_and_gameover()) {
		game_init();
		return;
	}

	update_flips();
	update_distance_score();

	world_generator_tick();
}

static void draw_pause_screen(void)
{
	uint16_t pause_line_color = RGB565(0x0000FF);
	int d = screen_context.height / 40;
	for (int i = 0; i <= screen_context.height; i++) {
		draw_line(&screen_context, screen_context.width / 2, 0, d * i, screen_context.height, 1, pause_line_color);
	}
	draw_text(&screen_context, "PAUSED", screen_context.width/2, screen_context.height/3, RGB565(0xFFFFFF), ANCHOR_HCENTER | ANCHOR_TOP);
}

static void game_draw_hud(void)
{
	int w = screen_context.width;
	int h = screen_context.height;

	debug_text_offset = 0;

	if (g_car.damage > 1) {
		// red "!"
		uint16_t red = RGB565(0xFF0000);
		int base_size = h / 120;
		if (base_size < 1) base_size = 1;
		int x = w / 2 - base_size / 2;
		int y = h / 3;
		fill_rect(&screen_context, x, y, base_size, base_size * 5, red);
		fill_rect(&screen_context, x, y + base_size * 6, base_size, base_size, red);

		int blue = 127 * (GAME_OVER_DAMAGE - g_car.damage) / GAME_OVER_DAMAGE;
		if (blue > 255) blue = 255;
		if (blue < 0) blue = 0;
		uint16_t color = RGB565(blue);

		int overlay_h = h * g_car.damage / GAME_OVER_DAMAGE / 2;
		fill_rect(&screen_context, 0, 0, w, overlay_h + 1, color);
		fill_rect(&screen_context, 0, h - overlay_h, w, h, color);
	}

	char score_str[16];
	sprintf(score_str, "%d", score);
	int c_val = flip_indicator;
	uint16_t score_color = RGB565((c_val << 16) | (c_val << 8) | 0xFF);
	draw_text(&screen_context, score_str, w / 2, h * 15 / 16, score_color, ANCHOR_HCENTER | ANCHOR_BOTTOM);

	if (g_is_paused) {
		draw_pause_screen();
	}

	#ifdef DEBUG_SHOW_FPS
		char str[16];
		sprintf(str, "FPS: %d, dt: %d", fps, last_tick_time);
		draw_text(&screen_context, str, 0, debug_text_offset, RGB565(0xffffff), ANCHOR_TOP | ANCHOR_LEFT);
		debug_text_offset += FONT_H;
	#endif
}

void draw_body(b2BodyId bodyId)
{
	b2Transform transform = b2Body_GetTransform(bodyId);
	BodyData* data = (BodyData*)b2Body_GetUserData(bodyId);

	uint32_t fill_color = 0xffffff; // defaults
	uint32_t stroke_color = NO_COLOR;

	if (data) {
		if (data->type == BODY_TYPE_CAR_CHASSIS) {
			fill_color = 0x000000;
			stroke_color = 0xffffff;
		} else if (data->type == BODY_TYPE_CAR_WHEEL) {
			fill_color = 0x000000;
			stroke_color = 0xffffff;
		} else if (data->type == BODY_TYPE_LANDSCAPE) {
			fill_color = 0x4444ff;
			stroke_color = NO_COLOR;
		}
	}

	int shape_count = b2Body_GetShapeCount(bodyId);
	if (shape_count == 0) return;

	b2ShapeId shape_array[shape_count];
	b2Body_GetShapes(bodyId, shape_array, shape_count);

	for (int i_shape = 0; i_shape < shape_count; ++i_shape) {
		b2ShapeId shapeId = shape_array[i_shape];
		b2ShapeType type = b2Shape_GetType(shapeId);

		if (type == b2_polygonShape) {
			b2Polygon polygon = b2Shape_GetPolygon(shapeId);
			vec2d screen_verts[B2_MAX_POLYGON_VERTICES];
			for (int i = 0; i < polygon.count; i++) {
				screen_verts[i] = world_to_screen(b2TransformPoint(transform, polygon.vertices[i]));
			}

			if (fill_color != NO_COLOR) {
				fill_polygon(&screen_context, screen_verts, polygon.count, RGB565(fill_color));
			}
			if (stroke_color != NO_COLOR) {
				draw_polygon(&screen_context, screen_verts, polygon.count, 0.15f * PIXELS_PER_METER, RGB565(stroke_color));
			}
		} else if (type == b2_circleShape) {
			b2Circle circle = b2Shape_GetCircle(shapeId);
			vec2d p = world_to_screen(b2TransformPoint(transform, circle.center));
			int r = circle.radius * PIXELS_PER_METER;
			if (fill_color != NO_COLOR) {
				draw_solid_circle(&screen_context, p.x, p.y, r, RGB565(fill_color));
			}
			if (stroke_color != NO_COLOR) {
				draw_circle(&screen_context, p.x, p.y, r, 0.07f * PIXELS_PER_METER, RGB565(stroke_color));
			}
		} else if (type == b2_chainSegmentShape) {
			b2ChainSegment chain_segment = b2Shape_GetChainSegment(shapeId);
			vec2d p1 = world_to_screen(b2TransformPoint(transform, chain_segment.segment.point1));
			vec2d p2 = world_to_screen(b2TransformPoint(transform, chain_segment.segment.point2));
			if (p1.x == p2.x && p1.y == p2.y) continue;
			draw_line(&screen_context, p1.x, p1.y, p2.x, p2.y, 0.2f * PIXELS_PER_METER, RGB565(fill_color));
		}
	}
}

static void draw_car_wheels(void)
{
	if (b2Body_IsValid(g_car.leftWheel)) {
		draw_body(g_car.leftWheel);
	}
	if (b2Body_IsValid(g_car.rightWheel)) {
		draw_body(g_car.rightWheel);
	}
}

static void draw_bodies(void)
{
	BodyNode* current = g_world.body_list;
	while(current != NULL) {
		if (b2Body_IsValid(current->bodyId)) {
			draw_body(current->bodyId);
		}
		current = current->next;
	}
	draw_car_wheels();
}

void update_screen_size(int w, int h)
{
	screen_width = w;
	screen_height = h;
	screen_min_side = w > h ? h : w;
}

void game_draw(GraphicsContext* ctx)
{
	clear(ctx);
	update_screen_size(ctx->width, ctx->height);
	update_camera();
	draw_bodies();
	game_draw_hud();
}

void game_handle_keydown_default(void) { g_car.motor_on = true; }

void game_handle_keyup_default(void) { g_car.motor_on = false; }

void game_print_debug(void)
{
	b2Vec2 pos_meters = g_car.position;

	int pos_x_units = (int)(pos_meters.x * WORLD_SCALE);
	int pos_y_units = (int)(pos_meters.y * WORLD_SCALE);

	int meters_x_int = (int)pos_meters.x;
	int meters_x_frac = (int)(fabsf(pos_meters.x * 100.0f)) % 100;

	int meters_y_int = (int)pos_meters.y;
	int meters_y_frac = (int)(fabsf(pos_meters.y * 100.0f)) % 100;

	printf("\n");
	printf("Car Pos: x=%d.%02dm, y=%d.%02dm", meters_x_int, meters_x_frac, meters_y_int, meters_y_frac);
	printf(" (x=%du, y=%du)\n", pos_x_units, pos_y_units);
}

vec2d world_to_screen(b2Vec2 worldPoint)
{
	int x = worldPoint.x * PIXELS_PER_METER + offset_x;
	int y = worldPoint.y * PIXELS_PER_METER + offset_y;
	return (vec2d){x, y};
}
