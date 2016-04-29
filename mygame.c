#include "mygame.h"

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define BLOCK_WIDTH 51
#define BLOCK_HEIGHT 15
#define BLOCK_MARGIN 2

#define PADDLE_WIDTH 40
#define PADDLE_HEIGHT 10
#define PADDLE_MAX_SPEED 15

#define BALL_SIZE 10
#define BALL_SPEED 4

#define min(__a__, __b__) ((__a__) < (__b__) ? (__a__) : (__b__))
#define max(__a__, __b__) ((__a__) > (__b__) ? (__a__) : (__b__))

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
  float    player_speed;

  coords_t ball_pos;
  coords_t ball_direction; // Not true coordinates, use as a vector

  int num_blocks;
  block_t *blocks;
} game_state_t;

typedef struct local_game_s {
  uint32_t screen_width, screen_height;
  float* screen;
  int32_t score;
  bool game_over;

  // Sends a new input to the game and requests a new state to be written
  //  into <game>.
  void (*_update)(game_t* game, input_t input);

  // Local stuff here
  void* _internal_game_state;
  int32_t max_rounds;
} local_game_t;

void drawGame(local_game_t *game)
{
  assert(game);
  game_state_t *state = (game_state_t*)game->_internal_game_state;

  bzero( game->screen, sizeof(float) * game->screen_width * game->screen_height );

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

  // Update player position
  state->player_pos.x += min(max(input.right, 0.0), 1.0) * PADDLE_MAX_SPEED;
  state->player_pos.x -= min(max(input.left, 0.0), 1.0) * PADDLE_MAX_SPEED;
  if (state->player_pos.x < 0) {
    state->player_pos.x = 0;
  }
  if (state->player_pos.x >= game->screen_width - PADDLE_WIDTH) {
    state->player_pos.x = game->screen_width - PADDLE_WIDTH - 1;
  }

  // Update ball position
  state->ball_pos.x += state->ball_direction.x;
  state->ball_pos.y += state->ball_direction.y;

  // Figure out if anything's happened with the blocks this round
  int b;
  for (b = 0; b < state->num_blocks; b++) {
    // Ignore dead blocks obviously
    if (state->blocks[b].health <= 0) {
      continue;
    }

    // Intersecting a block
    // TODO: replace this with a function taking a block and
    //  two ball positions to make sure a fast ball can't jump beyond a block
    if ((state->ball_pos.x + BALL_SIZE >= state->blocks[b].top_left.x &&
	 state->ball_pos.x < state->blocks[b].top_left.x + BLOCK_WIDTH) &&
	(state->ball_pos.y + BALL_SIZE >= state->blocks[b].top_left.y &&
	 state->ball_pos.y < state->blocks[b].top_left.y + BLOCK_HEIGHT)) {

      // TODO: Replace the magic number, possibly with a function taking
      //  time since last bounce into account
      state->blocks[b].health -= 20;

      // If block still alive, bounce here and stop looking through blocks
      // ...

      // If block died, continue in the same direction
    }
  }

  // Bounce on walls
  if ((state->ball_pos.x <= 0) ||
      (state->ball_pos.x + BALL_SIZE >= game->screen_width - 1))
    state->ball_direction.x = -state->ball_direction.x;
  if ((state->ball_pos.y <= 0))
    state->ball_direction.y = -state->ball_direction.y;

  // Die at the bottom
  if (state->ball_pos.y + BALL_SIZE >= game->screen_height - 1)
    l_game->game_over = true;

  drawGame(l_game);

  state->counter++;
  if ( l_game->max_rounds != -1 &&
       state->counter > l_game->max_rounds ) {
    l_game->game_over = true;
  }
}

game_t *createMyGame( int32_t max_rounds )
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
  tmp->max_rounds = max_rounds;

  // Set up player
  state->player_pos.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
  state->player_pos.y = SCREEN_HEIGHT - PADDLE_HEIGHT*2;
  state->player_speed = 0;

  // Set up ball
  state->ball_pos.x = (SCREEN_WIDTH - BALL_SIZE) / 2;
  state->ball_pos.y = SCREEN_HEIGHT / 2;
  state->ball_direction.x = BALL_SPEED;
  state->ball_direction.y = -BALL_SPEED;

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
