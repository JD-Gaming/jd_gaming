#include <stdio.h>

#include "network.h"

int main( int argc, char *argv[] )
{
  printf( "%d\n", argc );
  if( argc < 2 ) {
    fprintf( stderr, "Please provide a neural network definition file.\n" );
    return -1;
  }

  ffn_network_t *net = ffnNetworkLoadFile( argv[1] );
  if( net == NULL ) {
    fprintf( stderr, "File is not a neural network definition file: \"%s\".\n", argv[1] );
    return -2;
  }

  ffnNetworkPrint( net );

  return 0;
}
