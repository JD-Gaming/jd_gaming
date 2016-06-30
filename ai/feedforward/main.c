#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <time.h>

#include "network.h"

int main( void )
{
  const uint64_t numInputs = 2;
  const uint64_t numHidden = 2;
  const uint64_t numHiddenConnections = numInputs;
  const uint64_t numOutputs = 1;
  const uint64_t numOutputConnections = numHidden;
  /*
  const uint64_t numInputs = 2*(640*480) + 5;
  const uint64_t numHidden = 500; //numInputs/10;
  const uint64_t numHiddenConnections = numInputs * 0.05;
  const uint64_t numOutputs = 8;
  const uint64_t numOutputConnections = numHidden * 0.5;
  */
  size_t i;

  // Get some better randomness going
  srand((unsigned)(time(NULL)));

  network_t *netCopy = NULL;
  network_t *netFile = NULL;
  network_t *net = networkCreate( numInputs, numHidden, numHiddenConnections, numOutputs, numOutputConnections, false );
  if( net == NULL ) {
    printf( "Unable to create network\n" );
    return 0;
  }

  // Load a pre-configured network
  // A NAND network, provided the output uses step function and the hidden use sigmoid
  networkSetHiddenBias( net, 0, 5.373516 );
  networkSetHiddenWeight( net, 0, 0, -2.985651 );
  networkSetHiddenWeight( net, 0, 1, -4.935944 );
  networkSetHiddenActivation( net, 0, activation_sigmoid );

  networkSetHiddenBias( net, 1, -3.553529 );
  networkSetHiddenWeight( net, 1, 0, 3.971738 );
  networkSetHiddenWeight( net, 1, 1, 1.763571 );
  networkSetHiddenActivation( net, 1, activation_sigmoid );

  networkSetOutputBias( net, 0, 7.640961 );
  networkSetOutputWeight( net, 0, 0, -0.717844 );
  networkSetOutputWeight( net, 0, 1, -10.556825 );
  networkSetOutputActivation( net, 0, activation_step );

  networkSaveFile( net, "test.net" );

  // Copy from buffer
  uint8_t *data;
  uint64_t dataLen = networkSerialise( net, &data );
  printf( "Size: %llu\n", (unsigned long long)dataLen );
  if( dataLen != 0 ) {
    netCopy = networkUnserialise( dataLen, data );
    free( data );
  }
  if( netCopy == NULL ) {
    printf( "Unable to unserialise to copy\n" );
    return 0;
  }

  // Read from a file
  netFile = networkLoadFile( "test.net" );
  if( netFile == NULL ) {
    printf( "Unable to read from file\n" );
    return 0;
  }

  size_t inp;
  float inputs[4][2];
  inputs[0][0] = 0;  inputs[0][1] = 0;
  inputs[1][0] = 0;  inputs[1][1] = 1;
  inputs[2][0] = 1;  inputs[2][1] = 0;
  inputs[3][0] = 1;  inputs[3][1] = 1;

  for( inp = 0; inp < 4; inp++ ) {
    // Set input
    float *inputVal = inputs[inp];

    // Run networks
    networkRun( net, inputVal );
    networkRun( netCopy, inputVal );
    networkRun( netFile, inputVal );

    printf( "Input: {" );
    for( i = 0; i < numInputs; i++ ) {
      if( i+1 < numInputs )
	printf( "%f, ", inputVal[i] );
      else
	printf( "%f", inputVal[i] );
    }
    printf( "} -> {" );
    for( i = 0; i < numOutputs; i++ ) {
      if( i+1 < numOutputs )
	printf( "%f (%f, %f), ",
		networkGetOutputValue( net, i ),
		networkGetOutputValue( netCopy, i ),
		networkGetOutputValue( netFile, i ) );
      else
	printf( "%f (%f, %f)",
		networkGetOutputValue( net, i ),
		networkGetOutputValue( netCopy, i ),
		networkGetOutputValue( netFile, i ) );
    }
    printf( "}\n" );
  }

  networkDestroy(net);
  networkDestroy(netCopy);

  return 0;
}
