#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <strings.h>

#include "arkanoid.h"
#include "network.h"
#include "population.h"

#define FILENAME_LEN 100

void update(game_t* game, input_t input) {
  game->_update(game, input);
}

int main( void )
{
  // Get some better randomness going
  srand((unsigned)(time(NULL)));

  char filename[FILENAME_LEN];

  // Temporary game used to get meta data
  game_t *game = createArkanoid( -1, 0 );
  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }
  input_t inputs = {0, };

  // Number of networks in a population
  const unsigned int numNets = 75;
  // Number of games played by each network in a generation
  const unsigned int numRounds = 20;
  // Number of generations to play before stopping
  const unsigned int numGenerations = 2000;

  // Number of game frames to send as input to the networks
  const uint64_t numFrames = 2;
  // Number of random values given to the networks as input
  const uint64_t numRandom = 5;
  // Number of total inputs in the network
  const uint64_t numInputs = numFrames*(game->sensors[0].width * game->sensors[0].height) + numRandom;
  // Number of layers, including output layer, used by the networks
  const uint64_t numLayers = 4;
  // Description of the layers
  network_layer_params_t layerParams[] = {
    (network_layer_params_t) {400, numInputs * 0.05},
    (network_layer_params_t) {200, 50},
    (network_layer_params_t) {25, 100},
    (network_layer_params_t) {1, 25},
  };
  // Destroy the temporary game
  destroyArkanoid( game );

  // Create a population of neural networks
  printf( "Creating first generation of %u networks\n", numNets );
  population_t *population = populationCreate( numNets, numInputs, numLayers, layerParams, true );

  float *ffwData = malloc(numInputs * sizeof(float));
  bzero( ffwData, sizeof(float) * numInputs );

  int32_t bestScore;
  int     bestNet;

  unsigned long runningSeed = rand();
  unsigned long generation;
  for( generation = 0; generation < numGenerations; generation++ ) {
    bestScore = 0;
    bestNet = -1;

    printf( "Generation %lu\n", generation );
    srand( runningSeed + generation );

    populationClearScores( population );

    int n;
    for( n = 0; n < numNets; n++ ) {
      printf( "  Network %d", n ); fflush(stdout);

      uint64_t netScore = 0;
      int round;

      for( round = 0; round < numRounds; round++ ) {
	// Create a new game for this player
	game = createArkanoid( -1, rand() );
	if (game == NULL) {
	  fprintf( stderr, "Can't create game\n" );
	  return -1;
	}

	network_t *net = population->networks[n];
	while (game->game_over == false) {
	  uint64_t i;

	  // Give network some random values to play with
	  for( i = 0; i < numRandom; i++ ) {
	    ffwData[i] = rand() / (float)RAND_MAX;
	  }

	  // Copy last frame in order to track movement
	  for( i = numRandom; i < numRandom + (numFrames - 1) * game->sensors[0].height * game->sensors[0].width; i++ ) {
	    ffwData[i] = ffwData[i + game->sensors[0].height * game->sensors[0].width];
	  }

	  int x, y;
	  for (y = 0; y < game->sensors[0].height; y++) {
	    for (x = 0; x < game->sensors[0].width; x++) {
	      ffwData[i++] = game->sensors[0].data[y * game->sensors[0].width + x];
	    }
	  }

	  // Add AI here
	  networkRun( net, ffwData );

	  /*
	  inputs.up         = networkGetOutputValue( net, 0 );
	  inputs.down       = networkGetOutputValue( net, 1 );
	  inputs.left       = networkGetOutputValue( net, 2 );
	  inputs.right      = networkGetOutputValue( net, 3 );
	  inputs.actions[0] = networkGetOutputValue( net, 4 );
	  inputs.actions[1] = networkGetOutputValue( net, 5 );
	  inputs.actions[2] = networkGetOutputValue( net, 6 );
	  inputs.actions[3] = networkGetOutputValue( net, 7 );
	  */
	  float tmpOutput = networkGetOutputValue( net, 0 );
	  if( tmpOutput > 0 ) {
	    inputs.left  = tmpOutput;
	    inputs.right = 0;
	  } else if( tmpOutput < 0 ) {
	    inputs.left  = -tmpOutput;
	    inputs.right = 0;
	  } else {
	    inputs.left  = 0;
	    inputs.right = 0;
	  }

	  // Send input to game
	  update( game, inputs );
	} // End of game loop

	netScore += game->score;
	// Destroy game so we can begin anew with next player
	destroyArkanoid( game );
      }

      // If two nets have the same score, let the last one win
      if( netScore >= bestScore ) {
	bestScore = netScore;
	bestNet = n;
      }

      populationSetScore( population, n, netScore );

      printf( " - %llu / %u (%f)\n", (unsigned long long)netScore, numGenerations, netScore / (double)numGenerations );
    } // End of population loop

    printf( "  Best score: %d\n", bestScore );

    // Print the best net here
    sprintf( filename, "brains/0x%08lx_0x%08lx_%d.ffw", generation, runningSeed, bestScore );
    networkSaveFile( population->networks[bestNet], filename );

    population_t *nextPop = populationSpawn( population );
    populationDestroy( population );
    population = nextPop;
  }

  free(ffwData);
  populationDestroy( population );
  
  return 0;
}
