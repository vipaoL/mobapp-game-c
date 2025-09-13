#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "syscode.h"
#include "cmd_def.h"

#include "graphics.h"
#include "game.h"
#include "compat.h"

GraphicsContext screen_context;

static void *framebuf_mem = NULL;

#define CREATE_ENUM_MEMBER(code, name) KEY_##name = code,
enum KeyCodes {
	KEYPAD_ENUM(CREATE_ENUM_MEMBER)
};

#define TICK_TIME_MS 16

int handle_events(void)
{
	int key, type;
	type = sys_event(&key);

	if (type == EVENT_KEYDOWN) {
		switch (key) {
			case KEY_LSOFT: game_init(); break;
			case KEY_RSOFT: g_is_paused = !g_is_paused; break;
			case KEY_STAR: case KEY_PLUS: return 1;
			case KEY_4: game_print_debug();
			default:
				game_handle_keydown_default();
				break;
		}
	} else if (type == EVENT_KEYUP) {
		game_handle_keyup_default();
	} else if (type == EVENT_QUIT) {
		return 1;
	}
	return 0;
}

void lcd_appinit(void)
{
	struct sys_display *disp = &sys_data.display;
	disp->w2 = disp->w1;
	disp->h2 = disp->h1;
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

int main(int argc, char **argv)
{
	(void)argc; (void)argv;
	b2SetAllocator(custom_aligned_alloc, custom_aligned_free);
	framebuf_alloc();
	sys_framebuffer(screen_context.framebuf);
	sys_start();

	uint32_t last_time = sys_timer_ms();
	uint32_t last_sleep_time = sys_timer_ms();

	game_init();

	while (1) {
		if (handle_events()) break;

		uint32_t current_time = sys_timer_ms();
		uint32_t last_tick_ms = current_time - last_time;
		last_time = current_time;

		if (last_tick_ms > 250) {
			last_tick_ms = 250;
		}

		if (!g_is_paused) {
			game_update(last_tick_ms);
		}

		game_draw(&screen_context);

		sys_start_refresh();
		sys_wait_refresh();

		int32_t elapsed = sys_timer_ms() - last_sleep_time;
		if (elapsed < 0) elapsed = 0;

		int32_t sleep = TICK_TIME_MS - elapsed;
		if (sleep > 0) {
			sys_wait_ms(sleep);
		}
		last_sleep_time = sys_timer_ms();
	}
	game_destroy();

	free(framebuf_mem);
	return 0;
}

void keytrn_init(void)
{
	uint8_t keymap[64];
	sys_getkeymap(keymap);
#define FILL_KEYTRN(j) for (int i = 0; i < 64; i++) { sys_data.keytrn[j][i] = keymap[i]; }
	FILL_KEYTRN(0); FILL_KEYTRN(1);
#undef FILL_KEYTRN
}
