#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "syscode.h"
#include "cmd_def.h"

#include "box2d/box2d.h"
#include "box2d/types.h"

#include "graphics.h"

#define CREATE_ENUM_MEMBER(code, name) KEY_##name = code,

enum KeyCodes {
	KEYPAD_ENUM(CREATE_ENUM_MEMBER)
};

struct {
	int game_over;
	int pause;
	int score;
} game_state;

struct {
	b2WorldId worldId;
	b2DebugDraw debugDraw;
} world;

#define TICK_TIME 25
#define PIXELS_PER_METER 10.0f

static void *framebuf_mem = NULL;
static GraphicsContext screen_context;

void lcd_appinit(void)
{
	struct sys_display *disp = &sys_data.display;
	int w = disp->w1, h = disp->h1;
	disp->w2 = w;
	disp->h2 = h;
}

static void framebuf_alloc(void)
{
	struct sys_display *disp = &sys_data.display;
	size_t size = disp->w1 * disp->h1;
	uint8_t *p;
	framebuf_mem = p = malloc(size * 2 + 31);
	p += -(intptr_t)p & 31;
	screen_context.framebuf = (void*)p;
	screen_context.width = disp->w1;
	screen_context.height = disp->h1;
}

static inline uint16_t hex_color_to_rbg565(uint32_t rgb888_color)
{
	uint32_t rgb888 = rgb888_color;
	uint8_t r8 = (rgb888 >> 16) & 0xFF;
	uint8_t g8 = (rgb888 >> 8) & 0xFF;
	uint8_t b8 = rgb888 & 0xFF;
	uint16_t r5 = r8 >> 3;
	uint16_t g6 = g8 >> 2;
	uint16_t b5 = b8 >> 3;
	return (r5 << 11) | (g6 << 5) | b5;
}

static vec2d world_to_screen(b2Vec2 worldPoint)
{
	int x = (screen_context.width / 2.0f) + worldPoint.x * PIXELS_PER_METER;
	int y = (screen_context.height / 2.0f) + worldPoint.y * PIXELS_PER_METER;
	return (vec2d){x, y};
}

void DrawPolygonCallback(const b2Vec2* vertices, int vertexCount, b2HexColor color, void* context)
{
	(void)context;
	vec2d vertices_on_screen[vertexCount];

	for (int i = 0; i < vertexCount; ++i) {
		vertices_on_screen[i] = world_to_screen(vertices[i]);
	}
	draw_polygon(&screen_context, vertices_on_screen, vertexCount, hex_color_to_rbg565(color));
}

void DrawSolidPolygonCallback(b2Transform transform, const b2Vec2* vertices, int vertexCount, float radius, b2HexColor color, void* context)
{
	(void)radius;
	(void)context;
	vec2d vertices_on_screen[vertexCount];

	for (int i = 0; i < vertexCount; ++i) {
		b2Vec2 world_vertex = b2TransformPoint(transform, vertices[i]);
		vertices_on_screen[i] = world_to_screen(world_vertex);
	}

	fill_polygon(&screen_context, vertices_on_screen, vertexCount, hex_color_to_rbg565(color));
}

void DrawCircleCallback(b2Vec2 center, float radius, b2HexColor color, void* context)
{
	(void)context;
	vec2d center_on_screen = world_to_screen(center);

	draw_circle(&screen_context, center_on_screen.x, center_on_screen.y, (int)(radius * PIXELS_PER_METER), hex_color_to_rbg565(color));
}

void DrawSolidCircleCallback(b2Transform transform, float radius, b2HexColor color, void* context)
{
	(void)context;
	vec2d center_on_screen = world_to_screen(transform.p);

	draw_solid_circle(&screen_context, center_on_screen.x, center_on_screen.y, (int)(radius * PIXELS_PER_METER), hex_color_to_rbg565(color));
}

void DrawSegmentCallback(b2Vec2 p1, b2Vec2 p2, b2HexColor color, void* context)
{
	(void)context;
	// box2d uses this hardcoded color to draw landscape direction lines. See b2DrawShape() in box2d/src/physics_world.c
	if (color == b2_colorPaleGreen)
		return;
	vec2d s_p1 = world_to_screen(p1);
	vec2d s_p2 = world_to_screen(p2);
	draw_line(&screen_context, s_p1.x, s_p1.y, s_p2.x, s_p2.y, hex_color_to_rbg565(color));
}

// stubs to avoid null pointer
void DrawTransformCallback(b2Transform transform, void* context) { (void)transform; (void)context; }
void DrawStringCallback(b2Vec2 p, const char* s, b2HexColor color, void* context) { (void)p; (void)s; (void)color; (void)context; }
void DrawPointCallback(b2Vec2 p, float size, b2HexColor color, void* context) { (void)p; (void)size; (void)color; (void)context; }
void DrawSolidCapsuleCallback(b2Vec2 p1, b2Vec2 p2, float radius, b2HexColor color, void* context) { (void)p1; (void)p2; (void)radius; (void)color; (void)context; }

void create_landscape(b2WorldId worldId, int num_points, float center_x, float center_y, float width, float amp1, float amp2, float freq1, float freq2, float freq_offset)
{
	b2Vec2 landscape_points[num_points];

	for (int i = 0; i < num_points; ++i) {
		float x = center_x + (float)i / (num_points - 1) * width - width / 2;
		float y = center_y;
		y += amp1 * sinf(x * freq1);
		y += amp2 * sinf(x * freq2 + freq_offset);
		landscape_points[i] = (b2Vec2){x, y};
	}

	b2ChainDef chainDef = b2DefaultChainDef();
	chainDef.points = landscape_points;
	chainDef.count = num_points;

	b2SurfaceMaterial material = b2DefaultSurfaceMaterial();
	material.customColor = b2_colorGreen;

	chainDef.materials = &material;

	b2BodyDef groundBodyDef = b2DefaultBodyDef();
	b2BodyId groundBodyId = b2CreateBody(worldId, &groundBodyDef);
	b2CreateChain(groundBodyId, &chainDef);
}

void game_init()
{
	if (b2World_IsValid(world.worldId)) {
		b2DestroyWorld(world.worldId);
	}

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = (b2Vec2){0.0f, 10.0f};
	world.worldId = b2CreateWorld(&worldDef);

	world.debugDraw = (b2DebugDraw){0};
	world.debugDraw.context = NULL;
	world.debugDraw.DrawPolygonFcn = DrawPolygonCallback;
	world.debugDraw.DrawSolidPolygonFcn = DrawSolidPolygonCallback;
	world.debugDraw.DrawCircleFcn = DrawCircleCallback;
	world.debugDraw.DrawSolidCircleFcn = DrawSolidCircleCallback;
	world.debugDraw.DrawSegmentFcn = DrawSegmentCallback;
	world.debugDraw.DrawTransformFcn = DrawTransformCallback;
	world.debugDraw.DrawStringFcn = DrawStringCallback;
	world.debugDraw.DrawPointFcn = DrawPointCallback;
	world.debugDraw.DrawSolidCapsuleFcn = DrawSolidCapsuleCallback;
	world.debugDraw.drawShapes = true;

	// bodies outside of these bounds will not be drawn
	b2AABB bounds;
	bounds.lowerBound = (b2Vec2){-100.0f, -100.0f};
	bounds.upperBound = (b2Vec2){100.0f, 100.0f};
	world.debugDraw.drawingBounds = bounds;

	create_landscape(world.worldId, 14, 0, -10.0f, -25.0f, 1.0f, 0.8f, 0.5f, 1.6f, 0.5f);

	b2BodyDef staticBodyDef = b2DefaultBodyDef();
	staticBodyDef.position.y = 15;
	b2BodyId groundBodyId = b2CreateBody(world.worldId, &staticBodyDef);
	b2Polygon staticBox = b2MakeBox(10.0f, 1.0f);
	b2ShapeDef staticBodyShapeDef = b2DefaultShapeDef();
	staticBodyShapeDef.material.customColor = b2_colorMagenta;
	b2CreatePolygonShape(groundBodyId, &staticBodyShapeDef, &staticBox);

	b2BodyDef rectBodyDef = b2DefaultBodyDef();
	rectBodyDef.type = b2_dynamicBody;
	b2BodyId rectBodyId = b2CreateBody(world.worldId, &rectBodyDef);
	b2Polygon rectBodyBox = b2MakeSquare(1.0f);
	b2ShapeDef rectBodyShapeDef = b2DefaultShapeDef();
	rectBodyShapeDef.material.customColor = b2_colorRed;
	b2CreatePolygonShape(rectBodyId, &rectBodyShapeDef, &rectBodyBox);

	b2BodyDef circleBodyDef = b2DefaultBodyDef();
	circleBodyDef.type = b2_dynamicBody;
	circleBodyDef.position.x = 1.0f;
	circleBodyDef.position.y = 8.0f;
	circleBodyDef.enableSleep = false;
	b2BodyId circleBodyId = b2CreateBody(world.worldId, &circleBodyDef);
	b2Circle circleShape;
	circleShape.center = (b2Vec2){0.0f, 0.0f};
	circleShape.radius = 1.0f;
	b2ShapeDef circleShapeDef = b2DefaultShapeDef();
	circleShapeDef.material.restitution = 2.0f;
	circleShapeDef.material.customColor = b2_colorBlue;
	b2CreateCircleShape(circleBodyId, &circleShapeDef, &circleShape);
}

void game_update()
{
	int32_t subStepCount = 8;
	b2World_Step(world.worldId, TICK_TIME / 1000.0f, subStepCount);
}

void game_draw()
{
	clear(&screen_context);
	b2World_Draw(world.worldId, &world.debugDraw);
	sys_start_refresh();
	sys_wait_refresh();
}

int handle_events()
{
	int key, type;
	type = sys_event(&key);
	if (type == EVENT_KEYDOWN) {
		switch (key) {
			case KEY_LSOFT: game_init(); break;
			case KEY_RSOFT: game_state.pause = !game_state.pause; break;
			case KEY_STAR: case KEY_PLUS: return 1;
			case KEY_UP:
				b2World_SetGravity(world.worldId, (b2Vec2){0.0f, -10.0f});
				b2World_EnableSleeping(world.worldId, false); // if a body is sleeping, it won't be affected by gravity
				break;
			case KEY_DOWN:
				b2World_SetGravity(world.worldId, (b2Vec2){0.0f, 10.0f});
				b2World_EnableSleeping(world.worldId, false);
				break;
			case KEY_LEFT:
				b2World_SetGravity(world.worldId, (b2Vec2){-10.0f, 0.0f});
				b2World_EnableSleeping(world.worldId, false);
				break;
			case KEY_RIGHT:
				b2World_SetGravity(world.worldId, (b2Vec2){10.0f, 0.0f});
				b2World_EnableSleeping(world.worldId, false);
				break;
			case KEY_CENTER:
				b2World_SetGravity(world.worldId, (b2Vec2){0.0f, 0.0f});
				break;
		}
	} else if (type == EVENT_KEYUP) {
		b2World_EnableSleeping(world.worldId, true);
	} else if (type == EVENT_QUIT) {
		return 1;
	}
	return 0;
}

void game_loop()
{
	world.worldId = b2_nullWorldId;
	game_init();
	uint32_t last_time = sys_timer_ms();
	while (1) {
		if (handle_events()) break;
		if (!game_state.pause) game_update();
		game_draw();
		uint32_t current_time = sys_timer_ms();
		uint32_t sleep = TICK_TIME - (current_time - last_time);
		if (sleep < TICK_TIME) {
			sys_wait_ms(sleep);
			last_time = sys_timer_ms();
		}
	}
	if (b2World_IsValid(world.worldId)) b2DestroyWorld(world.worldId);
}

int main(int argc, char **argv)
{
	(void)argc; (void)argv;
	b2SetAllocator(custom_aligned_alloc, custom_aligned_free);
	framebuf_alloc();
	sys_framebuffer(screen_context.framebuf);
	sys_start();
	game_loop();
	free(framebuf_mem);
	return 0;
}

void keytrn_init(void)
{
	uint8_t keymap[64];
	int i, flags = sys_getkeymap(keymap);
	(void)flags;
#define FILL_KEYTRN(j) for (i = 0; i < 64; i++) { sys_data.keytrn[j][i] = keymap[i]; }
	FILL_KEYTRN(0); FILL_KEYTRN(1);
#undef FILL_KEYTRN
}
