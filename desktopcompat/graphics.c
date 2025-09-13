#include "graphics.h"
#include <SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL_render.h>

extern SDL_Renderer* g_renderer;

static void ColorFrom565(uint16_t color565, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a) {
	*r = (uint8_t)(((color565 & 0xF800) >> 11) << 3);
	*g = (uint8_t)(((color565 & 0x07E0) >> 5) << 2);
	*b = (uint8_t)((color565 & 0x001F) << 3);
	*a = 255;
}

void clear(GraphicsContext* ctx) {
	(void)ctx;
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
	SDL_RenderClear(g_renderer);
}

void fill(GraphicsContext* ctx, uint16_t color) {
	(void)ctx;
	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	SDL_SetRenderDrawColor(g_renderer, r, g, b, a);
	SDL_RenderClear(g_renderer);
}

void draw_pixel(GraphicsContext* ctx, int x, int y, uint16_t color) {
	(void)ctx;
	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	pixelRGBA(g_renderer, x, y, r, g, b, a);
}

void draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, float thickness, uint16_t color) {
	(void)ctx;
	if (thickness < 1) thickness = 1;

	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);

	if (thickness == 1) {
		lineRGBA(g_renderer, x0, y0, x1, y1, r, g, b, a);
		return;
	}

	thickLineRGBA(g_renderer, x0, y0, x1, y1, (uint8_t)thickness, r, g, b, a);

	int radius = thickness / 2;
	filledCircleRGBA(g_renderer, x0, y0, radius, r, g, b, a);
	filledCircleRGBA(g_renderer, x1, y1, radius, r, g, b, a);
}

void draw_hline(GraphicsContext* ctx, int x1, int x2, int y, uint16_t color) {
	(void)ctx;
	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	hlineRGBA(g_renderer, x1, x2, y, r, g, b, a);
}

void fill_rect(GraphicsContext* ctx, int x, int y, int w, int h, uint16_t color) {
	(void)ctx;
	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	boxRGBA(g_renderer, x, y, x + w - 1, y + h - 1, r, g, b, a);
}

void draw_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, float thickness, uint16_t color) {
	(void)ctx;
	if (radius <= 0 || thickness < 1) return;

	int outerRadius = radius;
	int innerRadius = radius - thickness;

	if (innerRadius < 0) innerRadius = 0;

	const int outerRadiusSq = outerRadius * outerRadius;
	const int innerRadiusSq = innerRadius * innerRadius;

	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	SDL_SetRenderDrawColor(g_renderer, r, g, b, a);

	for (int y = -outerRadius; y <= outerRadius; ++y)
	{
		for (int x = -outerRadius; x <= outerRadius; ++x)
		{
			const int distanceSq = x * x + y * y;
			if (distanceSq <= outerRadiusSq && distanceSq >= innerRadiusSq)
			{
				SDL_RenderDrawPoint(g_renderer, center_x + x, center_y + y);
			}
		}
	}
}

void draw_solid_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color) {
	(void)ctx;
	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	filledCircleRGBA(g_renderer, center_x, center_y, radius, r, g, b, a);
}

void draw_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, float thickness, uint16_t color) {
	(void)ctx;

	if (vertexCount < 2) {
		return;
	}

	for (int i = 0; i < vertexCount; ++i) {
		const vec2d p1 = vertices[i];
		const vec2d p2 = vertices[(i + 1) % vertexCount];
		draw_line(ctx, p1.x, p1.y, p2.x, p2.y, thickness, color);
	}
}

void fill_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color) {
	(void)ctx;
	if (vertexCount < 3) return;
	Sint16* vx = (Sint16*)malloc(vertexCount * sizeof(Sint16));
	Sint16* vy = (Sint16*)malloc(vertexCount * sizeof(Sint16));
	if (!vx || !vy) { free(vx); free(vy); return; }

	for(int i = 0; i < vertexCount; i++) {
		vx[i] = vertices[i].x;
		vy[i] = vertices[i].y;
	}

	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	filledPolygonRGBA(g_renderer, vx, vy, vertexCount, r, g, b, a);

	free(vx);
	free(vy);
}

void draw_text(GraphicsContext* ctx, const char* text, int x, int y, uint16_t color, int anchor) {
	(void)ctx;
	const int char_w = 8;
	const int char_h = 8;
	int text_w = strlen(text) * char_w;

	if (anchor & ANCHOR_HCENTER) x -= text_w / 2;
	if (anchor & ANCHOR_RIGHT)   x -= text_w;
	if (anchor & ANCHOR_VCENTER) y -= char_h / 2;
	if (anchor & ANCHOR_BOTTOM)  y -= char_h;

	uint8_t r, g, b, a;
	ColorFrom565(color, &r, &g, &b, &a);
	stringRGBA(g_renderer, x, y, text, r, g, b, a);
}
