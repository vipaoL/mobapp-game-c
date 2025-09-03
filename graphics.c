#include "graphics.h"
#include <stdlib.h>
#include <string.h>

void clear(GraphicsContext* ctx)
{
	memset(ctx->framebuf, 0, ctx->width * ctx->height * 2);
}

void fill(GraphicsContext* ctx, uint16_t color)
{
	int total_pixels = ctx->width * ctx->height;

	for (int i = 0; i < total_pixels; i++) {
		ctx->framebuf[i] = color;
	}
}

void draw_pixel(GraphicsContext* ctx, int x, int y, uint16_t color) {
	if (x >= 0 && x < ctx->width && y >= 0 && y < ctx->height) {
		ctx->framebuf[y * ctx->width + x] = color;
	}
}

void draw_line(GraphicsContext* ctx, int x0, int y0, int x1, int y1, uint16_t color) {
	int dx = abs(x1 - x0);
	int sx = x0 < x1 ? 1 : -1;
	int dy = -abs(y1 - y0);
	int sy = y0 < y1 ? 1 : -1;
	int err = dx + dy;
	int e2;

	while (1) {
		draw_pixel(ctx, x0, y0, color);
		if (x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

void draw_hline(GraphicsContext* ctx, int x1, int x2, int y, uint16_t color)
{
	if (x1 > x2) {
		int temp = x1;
		x1 = x2;
		x2 = temp;
	}
	for (int x = x1; x <= x2; x++) {
		draw_pixel(ctx, x, y, color);
	}
}

void draw_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color)
{
	for (int i = 0; i < vertexCount; ++i) {
		vec2d p1 = vertices[i];
		vec2d p2 = vertices[(i + 1) % vertexCount];
		draw_line(ctx, p1.x, p1.y, p2.x, p2.y, color);
	}
}

void fill_polygon(GraphicsContext* ctx, const vec2d* vertices, int vertexCount, uint16_t color)
{
	if (vertexCount == 0) return;

	float min_y = vertices[0].y;
	float max_y = vertices[0].y;
	for (int i = 1; i < vertexCount; i++) {
		if (vertices[i].y < min_y) min_y = vertices[i].y;
		if (vertices[i].y > max_y) max_y = vertices[i].y;
	}

	for (int y = (int)ceilf(min_y); y <= (int)floorf(max_y); y++) {
		float intersections[vertexCount];
		int intersection_count = 0;

		for (int i = 0; i < vertexCount; i++) {
			vec2d p1 = vertices[i];
			vec2d p2 = vertices[(i + 1) % vertexCount];

			if (p1.y == p2.y) continue;

			if (y >= fminf(p1.y, p2.y) && y < fmaxf(p1.y, p2.y)) {
				float x = p1.x + (y - p1.y) * (p2.x - p1.x) / (float)(p2.y - p1.y);
				intersections[intersection_count++] = x;
			}
		}

		for (int i = 0; i < intersection_count - 1; i++) {
			for (int j = 0; j < intersection_count - i - 1; j++) {
				if (intersections[j] > intersections[j + 1]) {
					float temp = intersections[j];
					intersections[j] = intersections[j + 1];
					intersections[j + 1] = temp;
				}
			}
		}

		for (int i = 0; i < intersection_count; i += 2) {
			if (i + 1 < intersection_count) {
				draw_hline(ctx, (int)ceilf(intersections[i]), (int)floorf(intersections[i+1]), y, color);
			}
		}
	}
}

void circle_draw_8_points(GraphicsContext* ctx, int xc, int yc, int x, int y, uint16_t color)
{
	draw_pixel(ctx, xc+x, yc+y, color);
	draw_pixel(ctx, xc-x, yc+y, color);
	draw_pixel(ctx, xc+x, yc-y, color);
	draw_pixel(ctx, xc-x, yc-y, color);
	draw_pixel(ctx, xc+y, yc+x, color);
	draw_pixel(ctx, xc-y, yc+x, color);
	draw_pixel(ctx, xc+y, yc-x, color);
	draw_pixel(ctx, xc-y, yc-x, color);
}

void draw_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color)
{
	int x = 0, y = radius;
	int d = 3 - 2 * radius;
	circle_draw_8_points(ctx, center_x, center_y, x, y, color);
	while (y >= x) {
		if (d > 0) {
			d = d + 4 * (x - y) + 10;
			y--;
		}
		else {
			d = d + 4 * x + 6;
		}
		x++;
		circle_draw_8_points(ctx, center_x, center_y, x, y, color);
	}
}

void draw_solid_circle(GraphicsContext* ctx, int center_x, int center_y, int radius, uint16_t color)
{
	int x = 0, y = radius;
	int d = 3 - 2 * radius;

	while (y >= x) {
		draw_hline(ctx, center_x - x, center_x + x, center_y + y, color);
		draw_hline(ctx, center_x - x, center_x + x, center_y - y, color);
		draw_hline(ctx, center_x - y, center_x + y, center_y + x, color);
		draw_hline(ctx, center_x - y, center_x + y, center_y - x, color);

		if (d > 0) {
			d = d + 4 * (x - y) + 10;
			y--;
		}
		else {
			d = d + 4 * x + 6;
		}
		x++;
	}
}
