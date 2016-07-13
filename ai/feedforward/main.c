#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include "canvas.h"

#include "network.h"

int main( void )
{
  const uint64_t numInputs = 2;
  const uint64_t numLayers = 2;
  network_layer_params_t layerParams[] = {
    (network_layer_params_t) {2, 2, activation_any},
    (network_layer_params_t) {1, 2, activation_any},
  };

  network_t *net1 = networkCreate( numInputs, numLayers, layerParams, true );
  network_t *net2, *net3;

  if( net1 == NULL ) {
    printf( "Unable to create network\n" );
    return 0;
  }

  networkSaveFile( net1, "/tmp/test.net" );
  net2 = networkLoadFile( "/tmp/test.net" );
  if( net2 == NULL ) {
    fprintf( stderr, "Unable to read network from file\n" );
    return -1;
  }

  networkSaveFile( net2, "/tmp/test2.net" );
  net3 = networkLoadFile( "/tmp/test2.net" );

  int i;
  for( i = 0; i < 3; i++ ) {
    float inputArray[2];
    inputArray[0] = rand() / (float)RAND_MAX;
    inputArray[1] = rand() / (float)RAND_MAX;

    networkRun( net1, inputArray );
    networkRun( net2, inputArray );
    networkRun( net3, inputArray );

    printf( "Old:  {%6.4f, %6.4f} -> %6.4f\n", inputArray[0], inputArray[1], networkGetOutputValue( net1, 0 ) );
    printf( "New:  {%6.4f, %6.4f} -> %6.4f\n", inputArray[0], inputArray[1], networkGetOutputValue( net2, 0 ) );
    printf( "New2: {%6.4f, %6.4f} -> %6.4f\n\n", inputArray[0], inputArray[1], networkGetOutputValue( net3, 0 ) );
  }

  networkPrint( net1 );
  networkPrint( net2 );
  networkPrint( net3 );

  networkDestroy(net3);
  networkDestroy(net2);
  networkDestroy(net1);

  return 0;
}
