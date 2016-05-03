#ifndef MY_GAME_H
#define MY_GAME_H

#include "game.h"

#include <stdint.h>

game_t *createArkanoid( int32_t max_rounds );
void destroyArkanoid( game_t *game );

#endif // MY_GAME_H
