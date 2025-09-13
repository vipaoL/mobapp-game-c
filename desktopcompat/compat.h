#include <stdint.h>
#include "SDL2/SDL_timer.h"

#ifndef COMPAT_H
#define COMPAT_H

inline uint32_t sys_timer_ms(void) {
	return SDL_GetTicks();
}

#endif
