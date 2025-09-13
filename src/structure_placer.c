#include "structure_placer.h"
#include "element_placer.h"
#include "worldgen.h"
#include <math.h>

EndPoint place_arc1_struct(int x, int y, int r)
{
	int r2 = r * 3/2;
	float cursor_x = x + r;

	place_arc(cursor_x - r, y - r2, r2, 60, 30);

	place_arc(cursor_x + r/2, y - r*2, r, 300, 120);

	float cos30 = cosf(30.0f * M_PI / 180.0f);
	int offset = (1 - cos30) * 2 * r2;
	place_arc(cursor_x + r*2 - offset, y - r2, r2, 60, 90);

	int l = r2 + r2 - offset;

	return (EndPoint){x + l, y};
}

EndPoint place_sin_struct(int x, int y, int l, int half_periods, int offset, int amp)
{
	place_sin(x, y, l, half_periods, offset, amp);

	return (EndPoint){x + l, y + amp * sinf(M_PI * (offset + 180 * half_periods) / 180.0f)};
}

EndPoint place_horizontal_floor_struct(int x, int y, int l)
{
	place_line(x, y, x + l, y);
	return (EndPoint){x + l, y};
}

EndPoint place_arc2_struct(int x, int y, int r, int sn)
{
	// stub
	return (EndPoint){x, y};
}

EndPoint place_abyss_struct(int x, int y, int l)
{
	int end_x = x;

	int pr_length = 1000;
	place_line(x, y, x + pr_length, y);
	x += pr_length;
	end_x += pr_length;
	int ang = 60; // springboard angle
	int r = l / 8;
	place_scaled_arc(x, y-r, r, ang, 90 - ang, 15, 10);
	place_line(x+l - l / 5, y - r * cosf(ang), x+l, y - r * cosf(ang));
	end_x += l;
	y -= r * cosf(ang);
	return (EndPoint){end_x, y};
}

EndPoint place_slanted_dotted_line_struct(int x, int y, int n)
{
	int offset_l = 600;
	for (int i = 0; i < n; i++) {
		place_line(x + i*offset_l, y + i * 300/n, x + i*offset_l + 300, y + i * 300/n - 300);
	}
	return (EndPoint){x + n * offset_l, y};
}

EndPoint place_floor_struct(int x, int y, int l, int y2)
{
	int amp = (y2 - y) / 2;
	return place_sin_struct(x, y + amp, l, 1, 270, amp);
}
