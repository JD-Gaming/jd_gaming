#include <stdio.h>

#include "game.h"
#include "arkanoid.h"

int main( void )
{
  game_t *game = createArkanoid( -1, 1 );
  if( game == NULL ) {
    fprintf( stderr, "Unable to initialise game\n" );
    return -1;
  }

  printf( "Game created\n" );
  while( game->game_over == false ) {
    input_t input = {0, };
    
    game->_update( game, input );
  }

  printf( "Destroying game\n" );

  destroyArkanoid( game );

  return 0;
}
