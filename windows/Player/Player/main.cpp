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
			(((unsigned int)(__val__ * 0xff) & 0xff)) | \
			(((unsigned int)(__val__ * 0xff) << 8) & 0xff00) | \
			(((unsigned int)(__val__ * 0xff) << 16) & 0xff0000) | \
			0xff000000 \
		)

int main() {
	srand((unsigned)(time(NULL)));

	game_t *arkanoid = createArkanoid(-1);
	input_t inputs = { 0, };

	Screen screen(arkanoid->screen_width, arkanoid->screen_height, "AI world");

	int leftState = 0;
	int rightState = 0;
	int upState = 0;
	int downState = 0;

	while (GetAsyncKeyState(VK_ESCAPE) == 0 && arkanoid->game_over != true) {
		leftState = GetAsyncKeyState(VK_LEFT);
		rightState = GetAsyncKeyState(VK_RIGHT);

		for (unsigned int y = 0; y < arkanoid->screen_height; y++) {
			for (unsigned int x = 0; x < arkanoid->screen_width; x++) {
				// Windows screens are upside down...
				float val = arkanoid->screen[y * arkanoid->screen_width + x];
				screen.pixels[(arkanoid->screen_height - y - 1) * arkanoid->screen_width + x] = FLOAT_TO_PIXEL(val * 0xff);
			}
		}

		// Draw new stuff
		screen.swap();

		inputs.left  = leftState ? 1 : 0;
		inputs.right = rightState ? 1 : 0;
		inputs.up    = upState ? 1 : 0;
		inputs.down  = downState ? 1 : 0;

		// Send input to game
		arkanoid->_update(arkanoid, inputs);
	}

	return 0;
}