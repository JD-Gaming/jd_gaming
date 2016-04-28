#include "mygame.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define BLOCK_WIDTH 51
#define BLOCK_HEIGHT 15
#define BLOCK_MARGIN 2

#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 10

#define BALL_SIZE 10

typedef struct coords_s {
  float x, y;
} coords_t;

typedef struct block_s {
  coords_t top_left;
  int      health;
} block_t;

typedef struct game_state_s {
  float *pixels;
  int32_t score;

  // Add more stuff here, obv
  int counter;
  coords_t player_pos;

  coords_t ball_pos;
  coords_t ball_direction; // Not true coordinates, use as a vector

  int num_blocks;
  block_t *blocks;
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

void drawGame(local_game_t *game)
{
  assert(game);
  game_state_t *state = (game_state_t*)game->_internal_game_state;

  int i;
  int x, y;
  for (i = 0; i < state->num_blocks; i++) {
    if (state->blocks[i].health > 0) {
      int block_top = state->blocks[i].top_left.y;
      int block_left = state->blocks[i].top_left.x;
      for (y = block_top; y < block_top + BLOCK_HEIGHT; y++) {
	for (x = block_left; x < block_left + BLOCK_WIDTH; x++) {
	  game->screen[y * game->screen_width + x] =
	    state->blocks[i].health / 100.0;
	}
      }
    }
  }

  int player_top = state->player_pos.y;
  int player_left = state->player_pos.x;
  for (y = player_top; y < player_top + PADDLE_HEIGHT; y++) {
    for (x = player_left; x < player_left + PADDLE_WIDTH; x++) {
      game->screen[y * game->screen_width + x] = 0.75;
    }
  }

  int ball_top = state->ball_pos.y;
  int ball_left = state->ball_pos.x;
  for (y = ball_top; y < ball_top + BALL_SIZE; y++) {
    for (x = ball_left; x < ball_left + BALL_SIZE; x++) {
      game->screen[y * game->screen_width + x] = 0.5;
    }
  }
}

void updateMyGame(game_t *game, input_t input)
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

  // Set up player
  state->player_pos.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
  state->player_pos.y = SCREEN_HEIGHT - PADDLE_HEIGHT*2;

  // Set up ball
  state->ball_pos.x = (SCREEN_WIDTH - BALL_SIZE) / 2;
  state->ball_pos.y = SCREEN_HEIGHT / 2;
  state->ball_direction.x = 0;
  state->ball_direction.y = 4;

  // Generate blocks
  state->num_blocks = 4 * 12;
  state->blocks = malloc( sizeof(block_t) * state->num_blocks );

  int row, col;
  for (row = 0; row < 4; row++) {
    for (col = 0; col < 12; col++ ) {
      state->blocks[row * 12 + col].top_left.y = BLOCK_MARGIN + row * (BLOCK_HEIGHT + BLOCK_MARGIN);
      state->blocks[row * 12 + col].top_left.x = BLOCK_MARGIN + col * (BLOCK_WIDTH + BLOCK_MARGIN);
      state->blocks[row * 12 + col].health = 100;
    }
  }

  // Set the first pixel state
  drawGame( tmp );

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
