#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <signal.h>

#include "arkanoid.h"
#include "network.h"
#include "population.h"
#include "jobhandler.h"
#include "progress.h"

#define FILENAME_LEN 100

typedef struct neuron_job_s {
  // Network to run
  network_t    *network;
  // Input to network
  float        *input;
  // Number of game rounds to play
  unsigned int  numRounds;
  // Which generation this is
  unsigned int  generation;
  // Seed to initialise random number generation with
  unsigned int  seed;

  // Place to save the score of the network
  unsigned int  score;

  // Signal indicating if the task should stop running a network and pick a new job
  bool          stop;

  // Set this to true when done
  bool          done;
} neuron_job_t;

static bool saveAllNetsAndQuit = false;
static bool started = false;

static const struct option *getOptlist()
{
  static struct option optlist[] = {
    {"tasks",       required_argument, NULL, 't'},
    {"seed",        required_argument, NULL, 's'},
    {"generations", required_argument, NULL, 'g'},
    {"first-gen",   required_argument, NULL, 'f'},
    {"networks",    required_argument, NULL, 'n'},
    {"rounds",      required_argument, NULL, 'r'},

    {"help",     no_argument,       NULL, 'h'},
    {0, 0, 0, 0}
  };

  return optlist;
}

static void sigintHandler( int signal )
{
  if( started ) {
    printf( "\nSave and quit after this network is done\n" );
    saveAllNetsAndQuit = true;
  } else {
    exit(0);
  }
}

static void update(game_t* game, input_t input) {
  game->_update(game, input);
}

static void usage( char *progname )
{
  printf( "Usage: %s [OPTION]... [FILE]...\n", progname );
  printf( "Generate a population of neural networks that play Arkanoid and compete\n"
	  "against each other in an attempt to learn how to play the game properly.\n\n" );
  printf( "File arguments will be loaded as neural networks and used to initialise\n"
	  "the first generation of the population.\n\n" );
  printf( "Mandatory arguments to long options are mandatory for short options too.\n" );
  printf( "  -t, --threads=INT          number of parallel threads to run\n" );
  printf( "  -s, --seed=HEX             seed value for random generator. \n"
	  "                             useful to replicate previous results\n" );
  printf( "  -g, --generations=INT      number of generations to train for\n" );
  printf( "  -f, --first-gen=INT        generation to begin at, useful for resuming\n" );
  printf( "  -n, --networks=INT         networks per population\n" );
  printf( "  -r, --rounds=INT           game rounds each network should play per generation.\n" );

  printf( "  -h, --help                 display this message and exit\n" );
}

static bool isNetworkCorrect( network_t *net, network_layer_params_t *lp )
{
  int i;
  for( i = 0; i < networkGetNumLayers( net ); i++ ) {
    if( networkGetLayerNumNeurons( net, i ) != lp[i].numNeurons )
      return false;

    if( networkGetLayerNumConnections( net, i ) != lp[i].numConnections )
      return false;
  }

  return true;
}

// individual == -1 means save all, otherwise save only specified individual
static void savePopulation( population_t *population, int individual, unsigned int generation, unsigned int seed, unsigned int rounds )
{
#define SAVE_NET_FORMAT "brains/0x%08x_0x%08x_%d_%f.ffw"
  char filename[FILENAME_LEN];
  if( individual == -1 ) {
    int i;
    for( i = 0; i < population->size; i++ ) {
      sprintf( filename, SAVE_NET_FORMAT,
	       generation, seed, i, populationGetScore( population, i ) / rounds );
      networkSaveFile( populationGetIndividual( population, i ), filename );
    }
  } else {
    sprintf( filename, SAVE_NET_FORMAT,
	     generation, seed, individual, populationGetScore( population, individual ) / rounds );
    networkSaveFile( populationGetIndividual( population, individual ), filename );
  }
}

static int64_t playNetwork( network_t *network,
			    uint64_t numFrames,  uint64_t numInputs,
			    unsigned int generation, unsigned int seed,
			    unsigned int numRounds, uint64_t numRandom )
{
  game_t *game;
  input_t inputs = {0, };
  float *ffwData = malloc((numRandom + numFrames * numInputs) * sizeof(float));
  double netScore = 0;

  unsigned int localSeed = seed + generation;

  int round;
  for( round = 0; round < numRounds; round++ ) {
    // Create a new game for this player
    game = createArkanoid( -1, rand_r( &localSeed ) );
    if (game == NULL) {
      fprintf( stderr, "Can't create game\n" );
      free( ffwData );
      return -1;
    }

    while (game->game_over == false) {
      uint64_t i;

      // Give network some random values to play with
      for( i = 0; i < numRandom; i++ ) {
	ffwData[i] = rand_r( &localSeed ) / (float)RAND_MAX;
      }

      // Copy last frame in order to track movement
      unsigned int size = game->sensors[0].height * game->sensors[0].width;
      for( i = 0; i+1 < numFrames; i++ ) {
	memcpy( &ffwData[numRandom + i * size], &ffwData[numRandom + (i+1) * size], size );
      }
      // Fetch new frame
      memcpy( &ffwData[numRandom + i * size], game->sensors[0].data, size );

      // Add AI here
      networkRun( network, ffwData );

      float tmpOutput = networkGetOutputValue( network, 0 );
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

  free(ffwData);
  return netScore;
}

int main( int argc, char *argv[] )
{
  // Get some better randomness going
  srand((unsigned)(time(NULL)));
  unsigned long runningSeed = rand();
  // Number of concurrent threads to run
  int numThreads = 1;
  // Number of networks in a population
  unsigned int numNets = 75;
  // Number of games played by each network in a generation
  unsigned int numRounds = 20;
  // Number of generations to play before stopping
  unsigned int numGenerations = 2000;
  // Which generation to begin with, useful when resuming training
  unsigned int firstGeneration = 0;

  // Register a signal handler that'll save networks when we quit
  signal( SIGINT, sigintHandler );

  int c;
  while( (c = getopt_long (argc, argv, "s:t:g:f:n:r:h",
			   getOptlist(), NULL)) != -1 ) {
    switch(c) {
    case 's': // Optional
      runningSeed = strtoul(optarg, NULL, 16);
      break;
    case 't': // Optional
      numThreads = atoi(optarg);
      break;
    case 'g': // Optional
      numGenerations = strtoul(optarg, NULL, 10);
      break;
    case 'f': // Optional
      firstGeneration = strtoul(optarg, NULL, 10);
      break;
    case 'n': // Optional
      numNets = strtoul(optarg, NULL, 10);
      break;
    case 'r': // Optional
      numRounds = strtoul(optarg, NULL, 10);
      break;
    case 'h': // Special
      usage( argv[0] );
      return 0;
    }
  }

  // Temporary game used to get meta data
  game_t *game = createArkanoid( -1, 0 );
  if (game == NULL) {
    fprintf( stderr, "Can't create game\n" );
    return -1;
  }

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

  int i = 0;
  // Get the rest of the arguments since they might be networks
  for( ; optind < argc; optind++ ) {
    // Regular arguments, network definition files to seed with
    printf( "Using file %s\n", argv[optind] );

    // Add networks to population
    network_t *tmp = networkLoadFile( argv[optind] );
    if( tmp != NULL ) {
      // Only add networks that can successfully mate with the ones we already have
      if( (networkGetNumInputs( tmp ) == numInputs) &&
	  (networkGetNumLayers( tmp ) == numLayers) &&
	  isNetworkCorrect( tmp, layerParams ) ) {
	printf( "Adding network\n" );
	populationReplaceIndividual( population, i++, tmp );
      } else {
	printf( "Not adding network\n" );
	networkDestroy( tmp );
      }
    }
  }

  double bestScore;
  int    bestNet;

  started = true;
  unsigned long generation;
  for( generation = firstGeneration; generation < numGenerations; generation++ ) {
    bestScore = 0;
    bestNet = -1;

    printf( "Generation %lu\n", generation );

    populationClearScores( population );
    int n;
    for( n = 0; n < population->size; n++ ) {
      double netScore = 0;
      printf( "  Network %d", n ); fflush(stdout);

      netScore = playNetwork( populationGetIndividual( population, n ), numFrames, numInputs, generation, runningSeed, numRounds, numRandom );

      // If two nets have the same score, let the last one win
      if( netScore >= bestScore ) {
	bestScore = netScore;
	bestNet = n;
      }

      populationSetScore( population, n, netScore );

      if( saveAllNetsAndQuit ) {
	printf( "\nSaving all networks and quitting\n" );
	savePopulation( population, -1, generation, runningSeed, numRounds );
	return 0;
      }

      printf( " - %llu / %u (%f)\n", (unsigned long long)netScore, numRounds, netScore / (double)numRounds );
    } // End of population loop

    printf( "  Best score: %f\n", bestScore );

    // Save the best net here
    savePopulation( population, bestNet, generation, runningSeed, numRounds );

    populationRespawn( population, false );
  }

  populationDestroy( population );
  
  return 0;
}
