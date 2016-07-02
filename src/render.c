#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "arkanoid.h"
#include "canvas.h"
#include "network.h"

#define FILENAME_LEN 100

void update(game_t* game, input_t input) {
  game->_update(game, input);
}

void createFolderFromFilename( char *filename )
{
  struct stat st = {0};

  if (stat("/some/directory", &st) == -1) {
    mkdir("/some/directory", 0700);
  }
}

int main( int argc, char *argv[] )
{
  char *networkFilename = argv[1];
  char *strGeneration   = argv[2];
  char *strRunningSeed  = argv[3];

  game_t *game = createArkanoid( -1 );
  input_t inputs = {0, };

  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }

  // Get some better randomness going
  srand((unsigned)(time(NULL)));

  Canvas *c;
  canvasInit();
  c = canvasCreate( game->sensors[0].width, game->sensors[0].height, RGB_888 );
  char imageFilename[FILENAME_LEN];

  // Create neural network
  network_t *net = networkLoadFile( networkFilename );
  if( net == NULL ) {
    fprintf( stderr, "Unable to load neural network\n" );
    return -1;
  }

  uint64_t numInputs = networkGetNumInputs( net );
  uint64_t numRandom = numInputs - 2 * game->sensors[0].width * game->sensors[0].height;

  float *ffwData = malloc( sizeof(float) * numInputs );
  bzero( ffwData, sizeof(float) * numInputs );

  printf( "Inputs: %llu\n", (unsigned long long)numInputs );

  unsigned long runningSeed = strtoul( strRunningSeed, NULL, 16 );
  unsigned long generation   = strtoul( strGeneration, NULL, 16 );
  srand( runningSeed + generation );
  int count = 0;
  while( game->game_over == false ) {
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

    int x, y;
    for (y = 0; y < game->sensors[0].height; y++) {
      for (x = 0; x < game->sensors[0].width; x++) {
	float value = game->sensors[0].data[y * game->sensors[0].width + x];

	ffwData[i++] = value;
	uint8_t col = (uint8_t)round( value * 0xff );
	canvasSetRGB( c, x, y, col, col, col );
      }
    }

    // Add AI here
    networkRun( net, ffwData );

    inputs.up         = networkGetOutputValue( net, 0 );
    inputs.down       = networkGetOutputValue( net, 1 );
    inputs.left       = networkGetOutputValue( net, 2 );
    inputs.right      = networkGetOutputValue( net, 3 );
    inputs.actions[0] = networkGetOutputValue( net, 4 );
    inputs.actions[1] = networkGetOutputValue( net, 5 );
    inputs.actions[2] = networkGetOutputValue( net, 6 );
    inputs.actions[3] = networkGetOutputValue( net, 7 );


    // Send input to game
    update( game, inputs );

    snprintf( imageFilename, FILENAME_LEN, "brains/%s_%010d.jpg", strGeneration, count );
    canvasSaveJpeg( c, imageFilename, 255 );

    count++;
  }

  destroyArkanoid(game);
  
  return 0;
}
