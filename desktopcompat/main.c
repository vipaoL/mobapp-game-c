#include <SDL2/SDL.h>
#include <stdbool.h>
#include <stddef.h>
#include "graphics.h"
#include "game.h"
#include "compat.h"
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL.h>

SDL_Window* g_window = NULL;
SDL_Renderer* g_renderer = NULL;

GraphicsContext screen_context;

void handle_key_event(SDL_KeyboardEvent* key) {
	switch(key->keysym.scancode) {
		case SDL_SCANCODE_R:
			if (key->type == SDL_KEYDOWN && key->repeat == 0) game_init();
			break;
		case SDL_SCANCODE_ESCAPE:
			if (key->type == SDL_KEYDOWN && key->repeat == 0) g_is_paused = !g_is_paused;
			break;
		case SDL_SCANCODE_4:
			if (key->type == SDL_KEYDOWN && key->repeat == 0) game_print_debug();
			break;
		default:
			if (key->type == SDL_KEYDOWN && key->repeat == 0) {
				game_handle_keydown_default();
			} else if (key->type == SDL_KEYUP) {
				game_handle_keyup_default();
			}
			break;
	}
}

int main(int argc, char* argv[]) {
	(void)argc; (void)argv;

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
		return 1;
	}

	g_window = SDL_CreateWindow("FPBox2D Demo",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		800, 600,
		SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!g_window) {
		fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}

	g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (!g_renderer) {
		fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
		SDL_DestroyWindow(g_window);
		SDL_Quit();
		return 1;
	}

	uint32_t last_time = sys_timer_ms();
	game_init();

	bool running = true;
	SDL_Event event;

	while (running) {
		while (SDL_PollEvent(&event)) {
			switch(event.type) {
				case SDL_QUIT:
					running = false;
					break;
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					handle_key_event(&event.key);
					break;
			}
		}

		SDL_GetRendererOutputSize(g_renderer, &screen_context.width, &screen_context.height);
		screen_context.framebuf = NULL;

		uint32_t current_time = sys_timer_ms();
		uint32_t delta_ms = current_time - last_time;
		last_time = current_time;
		if (delta_ms > 250) delta_ms = 250;
		if (!g_is_paused) game_update(delta_ms);

		clear(&screen_context);
		game_draw(&screen_context);
		SDL_RenderPresent(g_renderer);
	}

	game_destroy();
	SDL_DestroyRenderer(g_renderer);
	SDL_DestroyWindow(g_window);
	SDL_Quit();

	return 0;
}
