#include <stdio.h>

#include "network.h"

int main( int argc, char *argv[] )
{
  if( argc < 1 ) {
    fprintf( stderr, "Please provide a neural network definition file.\n" );
    return -1;
  }

  network_t *net = networkLoadFile( argv[1] );
  if( net == NULL ) {
    fprintf( stderr, "File is not a neural network definition file: \"%s\".\n", argv[1] );
    return -2;
  }

  networkPrint( net );

  return 0;
}
