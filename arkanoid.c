#include "arkanoid.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "geometry.h"

#define SQRT_2_HALF 0.70710678118

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define BLOCK_WIDTH 64
#define BLOCK_HEIGHT 15
#define BLOCK_MARGIN 1

#define PADDLE_WIDTH 180
#define PADDLE_HEIGHT 10
#define PADDLE_MAX_SPEED 15

#define BALL_SIZE 10
#define BALL_SPEED 4

#define BLOCK_ROWS 4
#define BLOCK_COLS 10

#ifndef min
#  define min(__a__, __b__) ((__a__) < (__b__) ? (__a__) : (__b__))
#endif
#ifndef max
#  define max(__a__, __b__) ((__a__) > (__b__) ? (__a__) : (__b__))
#endif

typedef struct block_s {
  point_t top_left;
  int      health;
} block_t;

typedef struct game_state_s {
	float *pixels;

	// Add more stuff here, obv
	int counter;

	point_t player_pos;
	float    player_speed;

	point_t ball_pos;
	point_t ball_direction; // Not true coordinates, use as a vector

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
	void(*_update)(game_t* game, input_t input);

	// Local stuff here
	void* _internal_game_state;
	int32_t max_rounds;
} local_game_t;

typedef enum direction_e {
	direction_none,
	direction_up,
	direction_down,
	direction_left,
	direction_right
} direction_t;

direction_t intersects(local_game_t *game, int block, point_t last_pos, point_t next_pos, point_t block_pos, uint32_t width, uint32_t height)
{
	// We know that the angle can't be low enough to go entirely sideways, so !up = down, but !left != right.
	int up_down, left_right;
	up_down = (int)(last_pos.y - next_pos.y);
	left_right = (int)(last_pos.x - next_pos.x);

	// Up and down check
	if (up_down > 0) {
		// Going up
		if (// Top of ball was lower than bottom of block
			(last_pos.y >= block_pos.y + height) &&
			// Top of ball is higher than bottom of block
			(next_pos.y <= block_pos.y + height) &&
			// Ball is within the width of the block
			(next_pos.x < block_pos.x + width) &&
			(next_pos.x + BALL_SIZE >= block_pos.x)) {
			return direction_up;
		}
	}
	else {
		// Going down
		if (// Bottom of ball was higher than top of block
			// There might be a = missing somewhere here, test when a ball actually bounces on top
			(last_pos.y + BALL_SIZE <= block_pos.y) &&
			// Bottom of ball is lower than top of block
			(next_pos.y + BALL_SIZE >= block_pos.y) &&
			// Ball is within the width of the block
			(next_pos.x < block_pos.x + width) &&
			(next_pos.x + BALL_SIZE >= block_pos.x)) {
			return direction_down;
		}
	}

	if (left_right > 0) {
		// Going left
	}
	else if (left_right < 0) {
		// Going right
	}
	else {
		// Going straight up or down
	}

	// Temporary check, to be replaced
	if ((next_pos.x + BALL_SIZE >= block_pos.x &&
		next_pos.x < block_pos.x + width) &&
		(next_pos.y + BALL_SIZE >= block_pos.y &&
		next_pos.y < block_pos.y + height)) {
		return direction_left;
	}

	return direction_none;
}

void strikeBall(local_game_t *l_game, point_t ball, point_t direction, point_t paddle, uint32_t width)
{
	line_t ballLine = {
		ball,
		(point_t) {
			ball.x + direction.x,
				ball.y + direction.y + BALL_SIZE
		}
	};

	line_t paddleLine = {
		paddle,
		(point_t) {
			paddle.x + width,
				paddle.y
		}
	};

	point_t iPoint;
	if (intersectSegment(ballLine, paddleLine, &iPoint)) {
		game_state_t *state = l_game->_internal_game_state;
		float dist = (iPoint.x - paddleLine.p1.x) / PADDLE_WIDTH;
		fprintf(stderr, "Intersect at point {%f, %f}\n", iPoint.x, iPoint.y);
		fprintf(stderr, "Distance from middle: %f\n", dist);

		if (ballLine.p1.x < ballLine.p2.x) {
			// Going right
			float angle = (float)acos(dist * SQRT_2_HALF);

			state->ball_direction.x = (float)cos(angle) * BALL_SPEED;
			state->ball_direction.y = (float)sin(angle) * BALL_SPEED;

			printf("Right, angle: %f, x: %f, y: %f\n", angle * 180 / 3.14159267, state->ball_direction.x, state->ball_direction.y );
		}
		else if (ballLine.p1.x > ballLine.p2.x) {
			// Going left
			float angle = (float)acos((1.0 - dist) * SQRT_2_HALF);

			state->ball_direction.x = (float)-cos(angle) * BALL_SPEED;
			state->ball_direction.y = (float)sin(angle) * BALL_SPEED;

			printf("Left, angle: %f, x: %f, y: %f\n", angle * 180 / 3.14159267, state->ball_direction.x, state->ball_direction.y);
		}
		else {
			// Straight down
			float angle = (float)acos(2 * (dist - 0.5) * SQRT_2_HALF);

			state->ball_direction.x = (float)sin(angle) * BALL_SPEED;
			state->ball_direction.y = (float)cos(angle) * BALL_SPEED;
		}

		// Should do some nifty angle calculations here
		state->ball_direction.y = -state->ball_direction.y;
	}
}

void drawGame(local_game_t *game)
{
	assert(game);
	game_state_t *state = (game_state_t*)game->_internal_game_state;

	memset(game->screen, 0, sizeof(float) * game->screen_width * game->screen_height);

	int i;
	int x, y;
	for (i = 0; i < state->num_blocks; i++) {
		if (state->blocks[i].health > 0) {
			int block_top = (int)state->blocks[i].top_left.y;
			int block_left = (int)state->blocks[i].top_left.x;
			for (y = block_top + BLOCK_MARGIN; y < block_top + BLOCK_HEIGHT - BLOCK_MARGIN; y++) {
				for (x = block_left + BLOCK_MARGIN; x < block_left + BLOCK_WIDTH - BLOCK_MARGIN; x++) {
					game->screen[y * game->screen_width + x] =
						(float)(state->blocks[i].health / 100.0);
				}
			}
		}
	}

	int player_top = (int)state->player_pos.y;
	int player_left = (int)state->player_pos.x;
	for (y = player_top; y < player_top + PADDLE_HEIGHT; y++) {
		for (x = player_left; x < player_left + PADDLE_WIDTH; x++) {
			game->screen[y * game->screen_width + x] = 0.75;
		}
	}

	int ball_top = (int)state->ball_pos.y;
	int ball_left = (int)state->ball_pos.x;
	for (y = ball_top; y < ball_top + BALL_SIZE && y < (int)game->screen_height; y++) {
		for (x = ball_left; x < ball_left + BALL_SIZE && x < (int)game->screen_width; x++) {
			game->screen[y * game->screen_width + x] = 0.5;
		}
	}
}

void hit(local_game_t *l_game, int block)
{
	game_state_t *state = l_game->_internal_game_state;
	state->blocks[block].health -= 20;
	l_game->score += 1;
}

void updateArkanoid(game_t *game, input_t input)
{
	assert(game);
	local_game_t *l_game = (local_game_t*)game;
	game_state_t *state = l_game->_internal_game_state;

	// Update player position
	state->player_pos.x += (float)min(max(input.right, 0.0), 1.0) * PADDLE_MAX_SPEED;
	state->player_pos.x -= (float)min(max(input.left, 0.0), 1.0) * PADDLE_MAX_SPEED;
	if (state->player_pos.x < 0) {
		state->player_pos.x = 0;
	}
	if (state->player_pos.x >= game->screen_width - PADDLE_WIDTH) {
		state->player_pos.x = (float)game->screen_width - PADDLE_WIDTH - 1;
	}

	// Update ball position
	point_t last_ball_pos = state->ball_pos;
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
		direction_t bounce_direction = intersects(l_game, b, last_ball_pos, state->ball_pos, state->blocks[b].top_left, BLOCK_WIDTH, BLOCK_HEIGHT);
		if (bounce_direction != direction_none) {
			// If block still alive, bounce
			// If block died, continue in the same direction
			switch (bounce_direction) {
			case direction_up:
			case direction_down:
				// TODO: Replace the magic number, possibly with a function taking
				//  time since last bounce into account
				hit(l_game, b);
				if (state->blocks[b].health > 0)
					state->ball_direction.y = -state->ball_direction.y;
				break;

			default: // Sideways
				hit(l_game, b);
				if (state->blocks[b].health > 0)
					state->ball_direction.x = -state->ball_direction.x;
				break;
			}

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

	// Bounce on paddle
	direction_t bounce_direction = intersects(l_game, -1, last_ball_pos, state->ball_pos, state->player_pos, PADDLE_WIDTH, PADDLE_HEIGHT);
	if (bounce_direction == direction_down) {
		strikeBall(l_game, last_ball_pos, state->ball_direction, state->player_pos, PADDLE_WIDTH);
	}

	drawGame(l_game);

	state->counter++;
	if (l_game->max_rounds != -1 &&
		state->counter > l_game->max_rounds) {
		l_game->game_over = true;
	}
}

game_t *createArkanoid(int32_t max_rounds)
{
	local_game_t *tmp = malloc(sizeof(local_game_t));
	game_state_t *state = NULL;
	float *pixels = NULL;

	if (tmp == NULL) {
		return NULL;
	}

	pixels = malloc(sizeof(float) * SCREEN_WIDTH * SCREEN_HEIGHT);
	if (pixels == NULL) {
		free(tmp);
		return NULL;
	}

	state = malloc(sizeof(game_state_t));
	if (state == NULL) {
		free(pixels);
		free(tmp);
		return NULL;
	}

	tmp->screen_width = SCREEN_WIDTH;
	tmp->screen_height = SCREEN_HEIGHT;
	tmp->screen = pixels;
	tmp->score = 0;
	tmp->game_over = false;
	tmp->_update = updateArkanoid;
	tmp->_internal_game_state = state;
	tmp->max_rounds = max_rounds;

	state->counter = 0;

	// Set up player
	state->player_pos.x = (SCREEN_WIDTH - PADDLE_WIDTH) / 2;
	state->player_pos.y = SCREEN_HEIGHT - PADDLE_HEIGHT * 2;
	state->player_speed = 0;

	// Set up ball
	state->ball_pos.x = (SCREEN_WIDTH - BALL_SIZE) / 2;
	state->ball_pos.y = SCREEN_HEIGHT / 2;
	state->ball_direction.x = 2;
	state->ball_direction.y = -BALL_SPEED;

	// Generate blocks
	state->num_blocks = BLOCK_ROWS * BLOCK_COLS;
	state->blocks = malloc(sizeof(block_t) * state->num_blocks);

	int row, col;
	for (row = 0; row < BLOCK_ROWS; row++) {
		for (col = 0; col < BLOCK_COLS; col++) {
			state->blocks[row * BLOCK_COLS + col].top_left.y = (float)row * BLOCK_HEIGHT;
			state->blocks[row * BLOCK_COLS + col].top_left.x = (float)col * BLOCK_WIDTH;
			state->blocks[row * BLOCK_COLS + col].health = 100;
		}
	}

	// Set the first pixel state
	drawGame(tmp);

	return (game_t*)tmp;
}

void destroyArkanoid(game_t *game)
{
	if (game) {
		local_game_t *l_game = (local_game_t*)game;

		if (l_game->_internal_game_state)
			free(l_game->_internal_game_state);
		if (l_game->screen)
			free(l_game->screen);
	}
}
