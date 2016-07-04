#ifndef MY_GAME_H
#define MY_GAME_H

#include "game.h"

#include <stdint.h>

// Set max_rounds to -1 in order to continue playing until death
game_t *createArkanoid( int32_t max_rounds, unsigned int seed );
void destroyArkanoid( game_t *game );

#endif // MY_GAME_H
