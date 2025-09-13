#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <math.h>

#define FONT_W 8
#define FONT_H 16

typedef struct GraphicsContext {
	uint16_t* framebuf;
	int width;
	int height;
} GraphicsContext;

typedef struct vec2d {
	int x;
	int y;
} vec2d;

enum Anchor {
	ANCHOR_HCENTER = 1,
	ANCHOR_VCENTER = 2,
	ANCHOR_LEFT = 4,
	ANCHOR_RIGHT = 8,
	ANCHOR_TOP = 16,
	ANCHOR_BOTTOM = 32
};

void clear(GraphicsContext* ctx);
void fill(GraphicsContext* ctx, uint16_t color);
void draw_pixel(GraphicsContext* ctx, int x, int y, uint16_t color);

void draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, float thickness, uint16_t color);
void draw_hline(GraphicsContext* ctx, int x1, int x2, int y, uint16_t color);
void fill_rect(GraphicsContext* ctx, int x, int y, int w, int h, uint16_t color);
void draw_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, float thickness, uint16_t color);
void draw_solid_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color);
void draw_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, float thickness, uint16_t color);
void fill_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color);

void draw_text(GraphicsContext* ctx, const char* text, int x, int y, uint16_t color, int anchor);

/* 888 -> 565 */
#define RGB565(v) \
	(((v) >> 8 & 0xf800) | ((v) >> 5 & 0x7e0) | ((v) >> 3 & 0x1f))

#endif
