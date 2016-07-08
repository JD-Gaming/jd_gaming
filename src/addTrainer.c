#include <getopt.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <strings.h>
#include <signal.h>

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
static const int maxBits = 8;

#define START_BITS 256

static const struct option *getOptlist()
{
  static struct option optlist[] = {
    {"tasks",       required_argument, NULL, 't'},
    {"seed",        required_argument, NULL, 's'},
    {"generations", required_argument, NULL, 'g'},
    {"first-gen",   required_argument, NULL, 'f'},
    {"networks",    required_argument, NULL, 'n'},
    {"rounds",      required_argument, NULL, 'r'},
    {"bits",        required_argument, NULL, 'b'},
    {"start-bits",  required_argument, NULL, START_BITS},

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

static void usage( char *progname )
{
  printf( "Usage: %s [OPTION]... [FILE]...\n", progname );
  printf( "Generate a population of neural networks that try to learn how to add two\n"
	  "32 bit integers properly\n\n" );  
  printf( "File arguments will be loaded as neural networks and used to initialise\n"
	  "the first generation of the population.\n\n" );
  printf( "Mandatory arguments to long options are mandatory for short options too.\n" );
  printf( "  -t, --threads=INT          number of parallel threads to run\n" );
  printf( "  -s, --seed=HEX             seed value for random generator. \n"
	  "                             Useful to replicate previous results\n" );
  printf( "  -g, --generations=INT      number of generations to train for\n" );
  printf( "  -f, --first-gen=INT        generation to begin at, useful for resuming\n" );
  printf( "  -n, --networks=INT         networks per population\n" );
  printf( "  -r, --rounds=INT           number of additions to perform per generation\n" );

  printf( "  -b, --bits=INT             number of bits in the addition\n" );
  printf( "      --start-bits=INT       number of bits to compare in the beginning, defaults\n" );
  printf( "                             to all bits in numbers to add\n" );

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
#define SAVE_NET_FORMAT "bits/0x%08x_0x%08x_%d_%f.ffw"
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

static float calcScore( uint32_t first, uint32_t second, network_t *network, int numBits )
{
  float score = 0;
  uint32_t result = first + second;
  int i;

  for( i = 0; i < numBits; i++ ) {
    float resBit = (result & (1 << i)) ? 1.0 : 0.0;

    // Set score to distance between correct and calculated value.  Might 
    //  change to square of distance layer to make sure large errors are 
    //  attacked more aggressively.
    float tmpScore = fabsf( networkGetOutputValue( network, i ) - resBit );
    score += tmpScore;
  }

  return score;
}

static double playNetwork( network_t *network,
			   unsigned int generation, unsigned int seed,
			   unsigned int numRounds, int numBits )
{
  float ffwData[64];
  double netScore = 0;

  unsigned int localSeed = seed + generation;

  int round;
  for( round = 0; round < numRounds; round++ ) {
    uint32_t first, second;
    first = rand_r( &localSeed );
    second = rand_r( &localSeed );

    int i;
    for( i = 0; i < maxBits; i++ ) {
      ffwData[i] = (first & (1 << i)) ? 1.0 : 0.0;
    }
    for( i = 0; i < maxBits; i++ ) {
      ffwData[i+maxBits] = (second & (1 << i)) ? 1.0 : 0.0;
    }

    // Create output
    networkRun( network, ffwData );

    // Score network
    netScore += calcScore( first, second, network, numBits );
  }

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
  // How many bits to calculate scores for, should allow networks to learn one bit at a time
  int numBits = 1;
  float bitIncreaseLimit = 0.15;

  // Register a signal handler that'll save networks when we quit
  signal( SIGINT, sigintHandler );

  int c;
  while( (c = getopt_long (argc, argv, "s:t:g:f:n:r:b:h",
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
    case 'b': // Optional
      fprintf( stderr, "This is actually not setting the number of bits to compare, sorry...\n" );
      numBits = strtoul(optarg, NULL, 10);
      break;
    case START_BITS: // Optional
      //startBit = strtoul(optarg, NULL, 10);
      break;
    case 'h': // Special
      usage( argv[0] );
      return 0;
    }
  }

  // Number of total inputs in the network
  const uint64_t numInputs = maxBits * 2;
  // Number of layers, including output layer, used by the networks
  const uint64_t numLayers = 3;
  // Description of the layers
  network_layer_params_t layerParams[] = {
    (network_layer_params_t) {64, numInputs, activation_sigmoid},
    (network_layer_params_t) {32, 64,        activation_sigmoid},
    (network_layer_params_t) { 8, 32,        activation_sigmoid}
  };

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

  double  bestScore;
  int     bestNet;

  bool minimise = true;

  started = true;
  unsigned long generation;

  for( generation = firstGeneration; generation < numGenerations; generation++ ) {
    bestScore = minimise ? DBL_MAX : -DBL_MAX;
    bestNet = -1;

    printf( "Generation %lu\n", generation );

    populationClearScores( population );
    int n;
    for( n = 0; n < population->size; n++ ) {
      double netScore = 0;
      printf( "  Network %d", n ); fflush(stdout);

      netScore = playNetwork( populationGetIndividual( population, n ), generation, runningSeed, numRounds, numBits );

      // If two nets have the same score, let the last one win
      if( !minimise && netScore >= bestScore ) {
	bestScore = netScore;
	bestNet = n;
      } else if( minimise && netScore <= bestScore && netScore >= 0 ) {
	bestScore = netScore;
	bestNet = n;
      }

      populationSetScore( population, n, netScore );

      if( saveAllNetsAndQuit ) {
	printf( "\nSaving all networks and quitting\n" );
	savePopulation( population, -1, generation, runningSeed, numRounds );
	return 0;
      }

      printf( " - %f / %u (%f)\n", netScore, numRounds, netScore / (double)numRounds );
    } // End of population loop

    printf( "  Best score: %f (%f)\n", bestScore, bestScore / (double)numRounds);

    // Save the best net here
    savePopulation( population, bestNet, generation, runningSeed, numRounds );

    // Increase how many bits to practice on if network is good enough
    if( (bestScore / (double)numRounds) / numBits < bitIncreaseLimit && numBits < maxBits ) {
      numBits++;
    }

    populationRespawn( population, minimise );
  }

  populationDestroy( population );
  
  return 0;
}
