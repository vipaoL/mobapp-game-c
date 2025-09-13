#include "element_placer.h"
#include "worldgen.h"
#include "game.h"
#include <stdlib.h>
#include <math.h>

#define DETAIL_LEVEL 1

void place_sin(int x, int y, int l, int half_periods, int start_angle, int amp)
{
	if (amp == 0) {
		b2Vec2 points[] = {
			{x / WORLD_SCALE, y / WORLD_SCALE},
			{(x + l) / WORLD_SCALE, y / WORLD_SCALE}
		};
		world_create_two_sided_landscape(points, 2, x);
	}

	int step = 30 / DETAIL_LEVEL;
	int end_angle = start_angle + half_periods * 180;
	int angle = end_angle - start_angle;
	if (angle == 0) angle = 1;

	int max_points = (abs(angle) / step) + 2;
	b2Vec2 points[max_points];
	int point_count = 0;

	for (int i = start_angle; i < end_angle; i += step) {
		float px = x + (float)(i - start_angle) * l / (float)angle;
		float py = y + amp * sinf(i * M_PI / 180.0f);
		points[point_count++] = (b2Vec2){px / WORLD_SCALE, py / WORLD_SCALE};
	}

	float final_px = x + l;
	float final_py = y + amp * sinf(end_angle * M_PI / 180.0f);
	points[point_count++] = (b2Vec2){final_px / WORLD_SCALE, final_py / WORLD_SCALE};

	if (point_count > 1) {
		world_create_two_sided_landscape(points, point_count, x);
	}
}

void place_scaled_arc(int x, int y, int r, int angle_deg, int start_angle_deg, int kx, int ky)
{
	float scale_x = kx / 10.0f;
	float scale_y = ky / 10.0f;

	float step_rad = (M_PI * 10.0f / (140.0f + r)) / 180.0f / 2 / DETAIL_LEVEL;
	step_rad = CONSTRAIN(M_PI * 10.0f / 180.0f / DETAIL_LEVEL, step_rad, M_PI * 72.0f / 180.0f / DETAIL_LEVEL);

	float start_angle_rad = start_angle_deg * M_PI / 180.0f;
	float end_angle_rad = start_angle_rad + (angle_deg * M_PI / 180.0f);

	int num_segments = (int)(fabsf(angle_deg * M_PI / 180.0f) / step_rad);
	if (num_segments < 1) num_segments = 1;

	b2Vec2 points[num_segments + 2];
	int point_count = 0;

	for (int i = num_segments; i >= 0; i--) {
		float current_angle = start_angle_rad + (float)i * (end_angle_rad - start_angle_rad) / (float)num_segments;
		float px = x + cosf(current_angle) * scale_x * r;
		float py = y + sinf(current_angle) * scale_y * r;
		points[point_count++] = (b2Vec2){px / WORLD_SCALE, py / WORLD_SCALE};
	}

	if (point_count > 1) {
		world_create_two_sided_landscape(points, point_count, x + r);
	}
}

void place_arc(int x, int y, int r, int angle_deg, int start_angle_deg)
{
	place_scaled_arc(x, y, r, angle_deg, start_angle_deg, 10, 10);
}

void place_line(int x1, int y1, int x2, int y2)
{
	b2Vec2 platform_points[] = {
		{x1 / WORLD_SCALE, y1 / WORLD_SCALE},
		{x1 / WORLD_SCALE, y1 / WORLD_SCALE},
		{x2 / WORLD_SCALE, y2 / WORLD_SCALE},
		{x2 / WORLD_SCALE, y2 / WORLD_SCALE},
	};

	world_create_two_sided_landscape(platform_points, 4, fmaxf(x1, x2));
}
