#include "games/arkanoid/arkanoid.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#define _USE_MATH_DEFINES
#include <math.h>

#include "games/arkanoid/geometry.h"

#ifndef M_PI
#  define M_PI 3.14159265359
#endif
#define SQRT_2_HALF 0.70710678118

#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define BLOCK_ROWS 5
#define BLOCK_COLS 10
// Try to make this divide exactly or it'll be weird
#define BLOCK_WIDTH (SCREEN_WIDTH / BLOCK_COLS)
#define BLOCK_HEIGHT 15
// Purely cosmetic
#define BLOCK_MARGIN 1
#define BLOCK_HEALTH 100
#define BLOCK_DAMAGE 20

#define PADDLE_MAX_WIDTH 80
#define PADDLE_MIN_WIDTH 10
#define PADDLE_WIDTH_DECREASE 2
#define PADDLE_HEIGHT 10
#define PADDLE_Y_POS (SCREEN_HEIGHT - 2 * PADDLE_HEIGHT)
#define PADDLE_X_POS ((SCREEN_WIDTH - PADDLE_MAX_WIDTH) / 2)
#define PADDLE_MAX_SPEED 15

#define BALL_SIZE 10
#define BALL_SPEED 5
#define BALL_START_X ((SCREEN_WIDTH - BALL_SIZE) / 2)
#define BALL_START_Y (PADDLE_Y_POS - BALL_SIZE)
#define BALL_MIN_ANGLE (M_PI/3.0)
#define BALL_MAX_ANGLE (2.0*M_PI/3.0)

// Number of points per struck block initially
#define POINTS_BASE 10
// How much the number of points increase with every strike before being
//  reset by hitting the paddle
#define POINTS_INCREASE 1

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
	// Add more stuff here, obv
	int counter;

	unsigned int seed;

	point_t player_pos;
	float   player_speed;
	uint32_t paddle_width;

	point_t ball_pos;
	point_t ball_direction; // Not true coordinates, use as a vector

	int points_per_hit;

	int num_blocks;
	block_t *blocks;
} game_state_t;

typedef struct local_sensor_s {
	char *name;
	uint32_t height;
	uint32_t width;
	uint32_t depth;
	float *data;
} local_sensor_t;


typedef struct local_game_s {
	uint32_t num_sensors;
	local_sensor_t *sensors;
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

		// Reset points per hit when the paddle strikes
		state->points_per_hit = POINTS_BASE;

		float dist = (iPoint.x - paddleLine.p1.x) / (width);

		if (ballLine.p1.x < ballLine.p2.x) {
			// Going right
			float angle = (float)acos(dist * SQRT_2_HALF);

			state->ball_direction.x = (float)cos(angle) * BALL_SPEED;
			state->ball_direction.y = (float)-sin(angle) * BALL_SPEED;
		}
		else if (ballLine.p1.x > ballLine.p2.x) {
			// Going left
			float angle = (float)acos((1.0 - dist) * SQRT_2_HALF);

			state->ball_direction.x = (float)-cos(angle) * BALL_SPEED;
			state->ball_direction.y = (float)-sin(angle) * BALL_SPEED;
		}
		else {
			// Straight down
			float angle = (float)acos(2 * (dist - 0.5) * SQRT_2_HALF);

			state->ball_direction.x = (float)cos(angle) * BALL_SPEED;
			state->ball_direction.y = (float)-sin(angle) * BALL_SPEED;
		}
	}
}

void drawGame(local_game_t *game)
{
	assert(game);
	game_state_t *state = (game_state_t*)game->_internal_game_state;

	memset(game->sensors[0].data, 0, sizeof(float) * game->sensors[0].width * game->sensors[0].height);

	int i;
	uint32_t x, y;
	for (i = 0; i < state->num_blocks; i++) {
		if (state->blocks[i].health > 0) {
			uint32_t block_top = (int)state->blocks[i].top_left.y;
			uint32_t block_left = (int)state->blocks[i].top_left.x;
			for (y = block_top + BLOCK_MARGIN; y < block_top + BLOCK_HEIGHT - BLOCK_MARGIN; y++) {
				for (x = block_left + BLOCK_MARGIN; x < block_left + BLOCK_WIDTH - BLOCK_MARGIN; x++) {
					game->sensors[0].data[y * game->sensors[0].width + x] =
						(float)(state->blocks[i].health / (float)BLOCK_HEALTH);
				}
			}
		}
	}

	uint32_t player_top = (int)state->player_pos.y;
	uint32_t player_left = (int)state->player_pos.x;
	for (y = player_top; y < player_top + PADDLE_HEIGHT; y++) {
		for (x = player_left; x < player_left + state->paddle_width; x++) {
			game->sensors[0].data[y * game->sensors[0].width + x] = 0.75;
		}
	}

	uint32_t ball_top = (int)state->ball_pos.y;
	uint32_t ball_left = (int)state->ball_pos.x;
	for (y = ball_top; y < ball_top + BALL_SIZE && y < (int)game->sensors[0].height; y++) {
		for (x = ball_left; x < ball_left + BALL_SIZE && x < (int)game->sensors[0].width; x++) {
			game->sensors[0].data[y * game->sensors[0].width + x] = 0.5;
		}
	}
}

void hit(local_game_t *l_game, int block)
{
	game_state_t *state = l_game->_internal_game_state;
	state->blocks[block].health -= BLOCK_DAMAGE;
	l_game->score += state->points_per_hit;
	state->points_per_hit += POINTS_INCREASE;
}

void initBlocks(game_state_t *state)
{
	int row, col;
	for (row = 0; row < BLOCK_ROWS; row++) {
		for (col = 0; col < BLOCK_COLS; col++) {
			state->blocks[row * BLOCK_COLS + col].top_left.y = (float)row * BLOCK_HEIGHT;
			state->blocks[row * BLOCK_COLS + col].top_left.x = (float)col * BLOCK_WIDTH;
			state->blocks[row * BLOCK_COLS + col].health = BLOCK_HEALTH;
		}
	}
}

bool areBlocksDead(game_state_t *state)
{
	int row, col;
	for (row = 0; row < BLOCK_ROWS; row++) {
		for (col = 0; col < BLOCK_COLS; col++) {
			if (state->blocks[row * BLOCK_COLS + col].health > 0)
				return false;
		}
	}

	return true;
}

void blockCollision(local_game_t *l_game, game_state_t *state, point_t last_ball_pos, int block)
{
	// Ignore dead blocks obviously
	if (state->blocks[block].health <= 0) {
		return;
	}

	// Intersecting a block
	direction_t bounce_direction = intersects(l_game, block, last_ball_pos, state->ball_pos, state->blocks[block].top_left, BLOCK_WIDTH, BLOCK_HEIGHT);
	// If block still alive, bounce
	// If block died, continue in the same direction
	switch (bounce_direction) {
	case direction_up:
	case direction_down:
		hit(l_game, block);
		if (state->blocks[block].health > 0)
			state->ball_direction.y = -state->ball_direction.y;
		break;

	case direction_left:
	case direction_right:
		hit(l_game, block);
		if (state->blocks[block].health > 0)
			state->ball_direction.x = -state->ball_direction.x;
		break;

	default: // Miss
		break;
	}
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
	if (state->player_pos.x >= game->sensors[0].width - state->paddle_width) {
		state->player_pos.x = (float)game->sensors[0].width - state->paddle_width - 1;
	}

	// Update ball position
	point_t last_ball_pos = state->ball_pos;
	state->ball_pos.x += state->ball_direction.x;
	state->ball_pos.y += state->ball_direction.y;

	// Figure out if anything's happened with the blocks this round
	int b;

	if (state->ball_direction.y < 0) {
		// Going upwards, test blocks in reverse order
		for (b = state->num_blocks-1; b >= 0; b--) {
			blockCollision(l_game, state, last_ball_pos, b);
		}

	}
	else {
		// Going downwards
		for (b = 0; b < state->num_blocks; b++) {
			blockCollision(l_game, state, last_ball_pos, b);
		}
	}

	// Bounce on walls
	if (state->ball_pos.x <= 0) {
		state->ball_pos.x = -state->ball_pos.x;
		state->ball_direction.x = -state->ball_direction.x;
	}
	if (state->ball_pos.x + BALL_SIZE >= game->sensors[0].width - 1) {
		state->ball_pos.x = 2 * (game->sensors[0].width - 1 - BALL_SIZE) - state->ball_pos.x;
		state->ball_direction.x = -state->ball_direction.x;
	}
	if (state->ball_pos.y <= 0) {
		state->ball_pos.y = -state->ball_pos.y;
		state->ball_direction.y = -state->ball_direction.y;
	}

	// Die at the bottom
	if (state->ball_pos.y + BALL_SIZE >= game->sensors[0].height - 1)
		l_game->game_over = true;

	// Bounce on paddle
	direction_t bounce_direction = intersects(l_game, -1, last_ball_pos, state->ball_pos, state->player_pos, state->paddle_width, PADDLE_HEIGHT);
	if (bounce_direction == direction_down) {
		strikeBall(l_game, last_ball_pos, state->ball_direction, state->player_pos, state->paddle_width);

		// Reset blocks and increase difficulty
		if (areBlocksDead(state)) {
			initBlocks(state);
			if (state->paddle_width > PADDLE_MIN_WIDTH) {
				state->paddle_width -= PADDLE_WIDTH_DECREASE;
				state->player_pos.x += PADDLE_WIDTH_DECREASE / 2;
			}
		}
	}

	drawGame(l_game);

	state->counter++;
	if (l_game->max_rounds != -1 &&
		state->counter > l_game->max_rounds) {
		l_game->game_over = true;
	}
}

game_t *createArkanoid(int32_t max_rounds, unsigned int seed)
{
	local_game_t *tmp = malloc(sizeof(local_game_t));
	game_state_t *state = NULL;
	local_sensor_t *sensor = NULL;
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

	sensor = malloc(sizeof(local_sensor_t));
	if (sensor == NULL) {
		free(pixels);
		free(tmp);
		free(state);
		return NULL;
	}

	tmp->sensors = sensor;
	tmp->sensors[0].name = SENSOR_SCREEN;
	tmp->sensors[0].width = SCREEN_WIDTH;
	tmp->sensors[0].height = SCREEN_HEIGHT;
	tmp->sensors[0].depth = 1;
	tmp->sensors[0].data = pixels;
	tmp->score = 0;
	tmp->game_over = false;
	tmp->_update = updateArkanoid;
	tmp->_internal_game_state = state;
	tmp->max_rounds = max_rounds;

	state->counter = 0;
	state->seed = seed;

	// Set up player
	state->player_pos.x = PADDLE_X_POS;
	state->player_pos.y = PADDLE_Y_POS;
	state->player_speed = 0;
	state->paddle_width = PADDLE_MAX_WIDTH;
	state->points_per_hit = POINTS_BASE;

	// Set up ball
	state->ball_pos.x = rand_r( &(state->seed) ) % (SCREEN_WIDTH - PADDLE_MAX_WIDTH);
	state->ball_pos.y = BALL_START_Y;
	float startAngle = BALL_MIN_ANGLE + (rand_r( &(state->seed) ) / (float)RAND_MAX) * (BALL_MAX_ANGLE - BALL_MIN_ANGLE);
	state->ball_direction.x = (float)cos(startAngle) * BALL_SPEED;
	state->ball_direction.y = (float)-sin(startAngle) * BALL_SPEED;

	// Generate blocks
	state->num_blocks = BLOCK_ROWS * BLOCK_COLS;
	state->blocks = malloc(sizeof(block_t) * state->num_blocks);

	initBlocks(state);

	// Set the first pixel state
	drawGame(tmp);

	return (game_t*)tmp;
}

void destroyArkanoid(game_t *game)
{
	if (game) {
		local_game_t *l_game = (local_game_t*)game;

		if (l_game->_internal_game_state) {
			game_state_t *state = l_game->_internal_game_state;
			if (state->blocks)
				free(state->blocks);

			free(l_game->_internal_game_state);
		}
		if (l_game->sensors) {
			if (l_game->sensors[0].data)
				free(l_game->sensors[0].data);
			free(l_game->sensors);
		}
		free(game);
	}
}
