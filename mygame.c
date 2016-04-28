#include "mygame.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

typedef struct coords_s {
  float x, y;
} coords_t;

typedef struct game_state_s {
  float *pixels;
  int32_t score;

  // Add more stuff here, obv
  int counter;
  coords_t player_pos;
} game_state_t;

typedef struct local_game_s {
  int screen_width, screen_height;
  float* screen;
  int32_t score;
  bool game_over;
  
  // Sends a new input to the game and requests a new state to be written
  //  into <game>.
  void (*_update)(game_t* game, input_t input);

  // Local stuff here
  void* _internal_game_state;
} local_game_t;


void updateMyGame(game_t* game, input_t input)
{
  assert(game);
  local_game_t *l_game = (local_game_t*)game;
  game_state_t *state = l_game->_internal_game_state;

  if (state->counter++ > 50) {
    l_game->game_over = true;
  }
}

game_t *createMyGame( void )
{
  local_game_t *tmp = malloc(sizeof(local_game_t));
  game_state_t *state=NULL;
  float *pixels=NULL;

  if (tmp == NULL) {
    return NULL;
  }

  pixels = malloc( sizeof(float) * SCREEN_WIDTH * SCREEN_HEIGHT );
  if (pixels == NULL) {
    free( tmp );
    return NULL;
  }

  state = malloc( sizeof(game_state_t) );
  if (state == NULL) {
    free( pixels );
    free( tmp );
    return NULL;
  }

  tmp->screen_width = SCREEN_WIDTH;
  tmp->screen_height = SCREEN_HEIGHT;
  tmp->screen = pixels;
  tmp->score = 0;
  tmp->game_over = false;
  tmp->_update = updateMyGame;
  tmp->_internal_game_state = state;

  return (game_t*)tmp;
}

void destroyGame( game_t *game )
{
  if (game) {
    local_game_t *l_game = (local_game_t*)game;

    if (l_game->_internal_game_state)
      free(l_game->_internal_game_state);
    if (l_game->screen)
      free(l_game->screen);
  }
}
