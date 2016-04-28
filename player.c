#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "mygame.h"
#include "canvas.h"

void update(game_t* game, input_t input) {
  game->_update(game, input);
}

int main( void )
{
  Canvas *c;
  game_t *game = createMyGame();
  input_t inputs = {0, };

  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }

  canvasInit();
  c = canvasCreate( game->screen_width, game->screen_height, RGB_888 );
  
  while (game->game_over == false) {
    int x, y;

    for (y = 0; y < game->screen_height; y++) {
      for (x = 0; x < game->screen_width; x++) {
	uint8_t col = (uint8_t)round( game->screen[y * game->screen_width + x] * 0xff );
	canvasSetRGB( c, x, y, col, col, col );
      }
    }

    update( game, inputs );
  }
  
  canvasSaveJpeg( c, "game.jpg", 255 );
  return 0;
}
