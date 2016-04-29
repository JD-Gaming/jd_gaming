#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "mygame.h"
#include "canvas.h"

#define DRAW_IMAGES 1

void update(game_t* game, input_t input) {
  game->_update(game, input);
}

int main( void )
{
  game_t *game = createMyGame( -1 );
  input_t inputs = {0, };

  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }

#ifdef DRAW_IMAGES
  Canvas *c;
  canvasInit();
  c = canvasCreate( game->screen_width, game->screen_height, RGB_888 );
  char filename[50];
#endif

  int count = 0;

  while (game->game_over == false) {
#ifdef DRAW_IMAGES
    int x, y;
    for (y = 0; y < game->screen_height; y++) {
      for (x = 0; x < game->screen_width; x++) {
	uint8_t col = (uint8_t)round( game->screen[y * game->screen_width + x] * 0xff );
	canvasSetRGB( c, x, y, col, col, col );
      }
    }
#endif

    // Add AI here
    inputs.up = rand() / (float)RAND_MAX;
    inputs.down = rand() / (float)RAND_MAX;
    inputs.left = rand() / (float)RAND_MAX;
    inputs.right = rand() / (float)RAND_MAX;

    int i;
    for (i = 0; i < sizeof(inputs.actions)/sizeof(inputs.actions[0]); i++) {
      inputs.actions[i] = rand() / (float)RAND_MAX;
    }

    // Send input to game
    update( game, inputs );

#ifdef DRAW_IMAGES
    snprintf( filename, 50, "game_%010d.jpg", count++ );
    canvasSaveJpeg( c, filename, 255 );
#endif
  }
  
  return 0;
}
