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
  ffn_layer_params_t layerParams[] = {
    (ffn_layer_params_t) {2, 2, activation_any},
    (ffn_layer_params_t) {1, 2, activation_any},
  };

  ffn_network_t *net1 = ffnNetworkCreate( numInputs, numLayers, layerParams, true );
  ffn_network_t *net2, *net3;

  if( net1 == NULL ) {
    printf( "Unable to create network\n" );
    return 0;
  }

  ffnNetworkSaveFile( net1, "/tmp/test.net" );
  net2 = ffnNetworkLoadFile( "/tmp/test.net" );
  if( net2 == NULL ) {
    fprintf( stderr, "Unable to read network from file\n" );
    return -1;
  }

  ffnNetworkSaveFile( net2, "/tmp/test2.net" );
  net3 = ffnNetworkLoadFile( "/tmp/test2.net" );

  int i;
  for( i = 0; i < 3; i++ ) {
    float inputArray[2];
    inputArray[0] = rand() / (float)RAND_MAX;
    inputArray[1] = rand() / (float)RAND_MAX;

    ffnNetworkRun( net1, inputArray );
    ffnNetworkRun( net2, inputArray );
    ffnNetworkRun( net3, inputArray );

    printf( "Old:  {%6.4f, %6.4f} -> %6.4f\n", inputArray[0], inputArray[1], ffnNetworkGetOutputValue( net1, 0 ) );
    printf( "New:  {%6.4f, %6.4f} -> %6.4f\n", inputArray[0], inputArray[1], ffnNetworkGetOutputValue( net2, 0 ) );
    printf( "New2: {%6.4f, %6.4f} -> %6.4f\n\n", inputArray[0], inputArray[1], ffnNetworkGetOutputValue( net3, 0 ) );
  }

  /*
  ffnNetworkPrint( net1 );
  ffnNetworkPrint( net2 );
  ffnNetworkPrint( net3 );
  */

  ffnNetworkDestroy(net3);
  ffnNetworkDestroy(net2);
  ffnNetworkDestroy(net1);

  return 0;
}
