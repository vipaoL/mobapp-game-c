#include <stdint.h>

typedef struct GraphicsContext {
	uint16_t* framebuf;
	int width;
	int height;
} GraphicsContext;

typedef struct vec2d {
	int x;
	int y;
} vec2d;

void clear(GraphicsContext* ctx);

void fill(GraphicsContext* ctx, uint16_t color);

void draw_pixel(GraphicsContext* ctx, int x, int y, uint16_t color);

void draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, uint16_t color);

void draw_hline(GraphicsContext* ctx, int x1, int x2, int y, uint16_t color);

void draw_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color);

void fill_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color);

void circle_draw_8_points(GraphicsContext* ctx, int xc, int yc, int x, int y, uint16_t color);

void draw_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color);

void draw_solid_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color);
