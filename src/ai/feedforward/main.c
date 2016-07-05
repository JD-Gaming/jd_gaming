#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include "canvas.h"

#include "ai/feedforward/network.h"

int main( void )
{
  const uint64_t numRandom = 5;
  const uint64_t numInputs = 2*(640*480) + numRandom;
  const uint64_t numLayers = 2;
  network_layer_params_t layerParams[] = {
    (network_layer_params_t) {500, (uint64_t)(numInputs * 0.05)},
    (network_layer_params_t) {8, (uint64_t)(500 * 0.5)},
  };
  /*
  const uint64_t numHidden = 500; //numInputs/10;
  const uint64_t numHiddenConnections = numInputs * 0.05;
  const uint64_t numOutputs = 8;
  const uint64_t numOutputConnections = numHidden * 0.5;
  */
  uint64_t i;

  // Get some better randomness going
  srand((unsigned)(time(NULL)));

  network_t *net = networkCreate( numInputs, numLayers, layerParams, true );
  network_t *netFile;

  if( net == NULL ) {
    printf( "Unable to create network\n" );
    return 0;
  }

  canvasInit();

  Canvas *c1, *c2;
  c1 = canvasLoadImage( "images/game_0000000000.jpg" );
  c2 = canvasLoadImage( "images/game_0000000001.jpg" );

  float inputVal[numInputs];
  for( i = 0; i < numRandom; i++ ) {
    inputVal[i] = rand() / (float)RAND_MAX;
  }
  int x, y;
  for( y = 0; y < c1->height; y++ ) {
    for( x = 0; x < c1->width; x++ ) {
      uint8_t col;
      canvasGetRGB( c1, x, y, &col, &col, &col );

      inputVal[i++] = col / 255.0;
    }
  }
  for( y = 0; y < c2->height; y++ ) {
    for( x = 0; x < c2->width; x++ ) {
      uint8_t col;
      canvasGetRGB( c2, x, y, &col, &col, &col );

      inputVal[i++] = col / 255.0;
    }
  }

  networkSaveFile( net, "test.net" );
  netFile = networkLoadFile( "test.net" );
  if( netFile == NULL ) {
    fprintf( stderr, "Unable to read network from file\n" );
    return -1;
  }

  networkRun( net, inputVal );

  printf( "{%f, %f, %f, %f   %f, %f, %f, %f}\n",
	  networkGetOutputValue( net, 0 ),
	  networkGetOutputValue( net, 1 ),
	  networkGetOutputValue( net, 2 ),
	  networkGetOutputValue( net, 3 ),

	  networkGetOutputValue( net, 4 ),
	  networkGetOutputValue( net, 5 ),
	  networkGetOutputValue( net, 6 ),
	  networkGetOutputValue( net, 7 ) );

  networkRun( netFile, inputVal );

  printf( "{%f, %f, %f, %f   %f, %f, %f, %f}\n",
	  networkGetOutputValue( netFile, 0 ),
	  networkGetOutputValue( netFile, 1 ),
	  networkGetOutputValue( netFile, 2 ),
	  networkGetOutputValue( netFile, 3 ),

	  networkGetOutputValue( netFile, 4 ),
	  networkGetOutputValue( netFile, 5 ),
	  networkGetOutputValue( netFile, 6 ),
	  networkGetOutputValue( netFile, 7 ) );

  networkDestroy(netFile);
  networkDestroy(net);

  return 0;
}
