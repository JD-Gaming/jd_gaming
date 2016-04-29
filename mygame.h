#ifndef MY_GAME_H
#define MY_GAME_H

#include "game.h"

#include <stdint.h>

game_t *createMyGame( int32_t max_rounds );
void destroyGame( game_t *game );

#endif // MY_GAME_H
