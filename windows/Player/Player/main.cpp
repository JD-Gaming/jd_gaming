#include <time.h>
#include <stdio.h>

#include <stdint.h>
#include <math.h>
#include <windows.h>
#pragma comment( lib, "opengl32.lib" )
#include <synchapi.h>

#include "screen.h"

extern "C" {
#include "../../../game.h"
#include "../../../arkanoid.h"
}

#define FLOAT_TO_PIXEL(__val__) \
		( \
			(((unsigned int)(__val__ * 0xff) << 0) & 0xff) | \
			(((unsigned int)(__val__ * 0xff) << 8) & 0xff00) | \
			(((unsigned int)(__val__ * 0xff) << 16) & 0xff0000) | \
			(((unsigned int)(__val__ * 0xff) << 24) & 0xff000000) \
		)

int main() {
	srand((unsigned)(time(NULL)));

	game_t *game = createArkanoid(-1);
	input_t inputs = { 0, };

	Screen screen(game->screen_width, game->screen_height, "AI world");

	int leftState = 0;
	int rightState = 0;
	int upState = 0;
	int downState = 0;

	int count = 0;

	while (GetAsyncKeyState(VK_ESCAPE) == 0 && game->game_over != true) {
		leftState = GetAsyncKeyState(VK_LEFT);
		rightState = GetAsyncKeyState(VK_RIGHT);

		if (GetAsyncKeyState(VK_SPACE)) {
			screen.toggleVSync(true);
		}

		for (unsigned int y = 0; y < game->screen_height; y++) {
			for (unsigned int x = 0; x < game->screen_width; x++) {
				// Windows screens are upside down...
				uint32_t gVal = (uint32_t)(0xff * game->screen[y * game->screen_width + x]);
				uint32_t color = gVal | (gVal << 8) | (gVal << 16) | (gVal << 24);

				screen.pixels[(game->screen_height - y - 1) * game->screen_width + x] = color;
			}
		}

		// Draw new stuff
		screen.swap();

		inputs.left = (float)(leftState ? 1.0 : 0.0);
		inputs.right = (float)(rightState ? 1.0 : 0.0);
		inputs.up = (float)(upState ? 1.0 : 0.0);
		inputs.down = (float)(downState ? 1.0 : 0.0);

		// Send input to game
		game->_update(game, inputs);

		if (!(count % 100)) {
			fprintf(stderr, "Current fps: %f\n", screen.getFps());
		}
		count++;
	}

	fprintf(stderr, "Score: %d\n", game->score);

	while (GetAsyncKeyState(VK_ESCAPE) == 0) {
	}

	return 0;
}