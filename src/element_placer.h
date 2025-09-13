#ifndef ELEMENT_PLACER_H
#define ELEMENT_PLACER_H

void place_sin(int x, int y, int l, int half_periods, int start_angle, int amp);

void place_scaled_arc(int x, int y, int r, int angle_deg, int start_angle_deg, int kx, int ky);

void place_arc(int x, int y, int r, int angle_deg, int start_angle_deg);

void place_line(int x1, int y1, int x2, int y2);

#endif
