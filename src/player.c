#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <strings.h>

#include "arkanoid.h"
#include "canvas.h"
#include "network.h"

#define DRAW_IMAGES 1
#define FILENAME_LEN 100

void update(game_t* game, input_t input) {
  game->_update(game, input);
}

int main( void )
{
  game_t *game = createArkanoid( -1 );
  input_t inputs = {0, };

  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }

  // Get some better randomness going
  srand((unsigned)(time(NULL)));

#ifdef DRAW_IMAGES
  Canvas *c;
  canvasInit();
  c = canvasCreate( game->sensors[0].width, game->sensors[0].height, RGB_888 );
  char filename[FILENAME_LEN];
#endif

  const uint64_t numRandom = 5;
  const uint64_t numInputs = 2*(game->sensors[0].width * game->sensors[0].height) + numRandom;
  const uint64_t numHidden = 500; //numInputs/10;
  const uint64_t numHiddenConnections = numInputs * 0.05;
  const uint64_t numOutputs = 8;
  const uint64_t numOutputConnections = numHidden;

  // Create neural network
  network_t *net = networkLoadFile( "player.ffw" );
  if( net == NULL ) {
    net = networkCreate( numInputs, numHidden, numHiddenConnections, numOutputs, numOutputConnections, true );
    if( net == NULL ) {
      fprintf( stderr, "Unable to load or generate neural network\n" );
      return -1;
    }

    networkSaveFile( net, "player.ffw" );
  }

  float ffwData[numInputs];
  bzero( ffwData, sizeof(float) * numInputs );

  int count = 0;
  while (game->game_over == false) {
    printf( "Frame %d\n", count );
    uint64_t i;

    // Give network some random values to play with
    for( i = 0; i < numRandom; i++ ) {
      ffwData[i] = rand() / (float)RAND_MAX;
    }

    // Copy last frame in order to track movement
    for( i = numRandom; i < numRandom + game->sensors[0].height * game->sensors[0].width; i++ ) {
      ffwData[i] = ffwData[i + game->sensors[0].height * game->sensors[0].width];
    }

#ifdef DRAW_IMAGES
    int x, y;
    for (y = 0; y < game->sensors[0].height; y++) {
      for (x = 0; x < game->sensors[0].width; x++) {
	float value = game->sensors[0].data[y * game->sensors[0].width + x];
	ffwData[i++] = value;

	uint8_t col = (uint8_t)round( value * 0xff );
	canvasSetRGB( c, x, y, col, col, col );
      }
    }
#endif

    // Add AI here
    /*
    inputs.up = rand() / (float)RAND_MAX;
    inputs.down = rand() / (float)RAND_MAX;
    inputs.left = rand() / (float)RAND_MAX;
    inputs.right = rand() / (float)RAND_MAX;

    int i;
    for (i = 0; i < sizeof(inputs.actions)/sizeof(inputs.actions[0]); i++) {
      inputs.actions[i] = rand() / (float)RAND_MAX;
    }
    */

    networkRun( net, ffwData );

    inputs.up         = networkGetOutputValue( net, 0 );
    inputs.down       = networkGetOutputValue( net, 1 );
    inputs.left       = networkGetOutputValue( net, 2 );
    inputs.right      = networkGetOutputValue( net, 3 );
    inputs.actions[0] = networkGetOutputValue( net, 4 );
    inputs.actions[1] = networkGetOutputValue( net, 5 );
    inputs.actions[2] = networkGetOutputValue( net, 6 );
    inputs.actions[3] = networkGetOutputValue( net, 7 );

    /*
    printf( "{" );
    for( i = 0; i < 8; i++ ) {
      printf( "%f%s", networkGetOutputValue( net, i ), i == 7 ? "" : ", " );
    }
    printf( "}\n" );
    */

    // Send input to game
    update( game, inputs );

#ifdef DRAW_IMAGES
    snprintf( filename, FILENAME_LEN, "game_%010d.jpg", count );
    canvasSaveJpeg( c, filename, 255 );
#endif

    count++;
  }

  destroyArkanoid(game);
  
  return 0;
}
