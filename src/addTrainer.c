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
#include <pthread.h>

#include "network.h"
#include "population.h"
#include "jobhandler.h"
#include "progress.h"

#define FILENAME_LEN 100

typedef struct neuron_job_s {
  // Network to run
  network_t    *network;
  // Number of bits to look at
  int           numBits;
  // Number of game rounds to play
  unsigned int  numRounds;
  // Which generation this is
  unsigned int  generation;
  // Seed to initialise random number generation with
  unsigned int  seed;

  // Place to save the score of the network
  unsigned int *score;

  // Signal indicating if the task should stop running a network and pick a new job
  bool         *stop;

  // Set this to true when done
  bool         *done;
} neuron_job_t;

static bool saveAllNetsAndQuit = false;
static bool started = false;
static const int maxBits = 8;

static pthread_mutex_t net_mutex;

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

    {"help",        no_argument,       NULL, 'h'},
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
#define SAVE_NET_FORMAT "allbits/0x%08x_0x%08x_%d_%f.ffw"
  char filename[FILENAME_LEN];
  if( individual == -1 ) {
    int i;
    for( i = 0; i < population->size; i++ ) {
      sprintf( filename, SAVE_NET_FORMAT,
	       generation, seed, i, populationGetScore( population, i ) / (double)(rounds) );
      networkSaveFile( populationGetIndividual( population, i ), filename );
    }
  } else {
    sprintf( filename, SAVE_NET_FORMAT,
	     generation, seed, individual, populationGetScore( population, individual ) / (double)(rounds) );
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
  // Assume that any untrained bits are half wrong
  for( ; i < maxBits; i++ ) {
    score += 0.5;
  }

  return score;
}

static double playNetwork( network_t *network,
			   unsigned int generation, unsigned int seed,
			   unsigned int numRounds, int numBits,
			   bool *stopFlag )
{
  float ffwData[64];
  double netScore = 0;

  unsigned int localSeed = seed + generation;

  int round;
  uint32_t first, second;
  uint32_t max = (1 << maxBits);
  for( first = 0; first < max; first++ ) {
    for( second = 0; second < max; second++ ) {
      // Stop and clear score to avoid partial results
      if( *stopFlag ) {
	printf( "Stopping job\n" );
	netScore = 0;
	break;
      }

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
  }

  return netScore;
}


static void *train_thread( void *arg )
{
  jobHandler *jh = arg;

  printf( "Thread started!\n" );

  while( 1 ) {
    neuron_job_t *job = jobHandlerGetJob( jh );
    if( job != NULL ) {
      double score = playNetwork( job->network, job->generation, job->seed, job->numRounds, job->numBits, job->stop );
      pthread_mutex_lock( &net_mutex );
      *(job->score) = score;
      *(job->done) = true;
      pthread_mutex_unlock( &net_mutex );
    } else if( saveAllNetsAndQuit == true ) {
      // If no job left and quit flag is set, quit
      break;
    }
  }

  printf( "Thread stopping!\n" );
  return NULL;
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

  int i = 0;

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
  if( population == NULL ) {
    fprintf( stderr, "Can't create population\n" );
    return -2;
  }

  // Set up threads and stuff
  jobHandler *jh = jobHandlerCreate( JH_RANDOM, population->size, sizeof(neuron_job_t), NULL );

  pthread_t *threads = malloc( sizeof(*threads) * numThreads );
  if( !threads ) {
    free( jh );
    return -3;
  }
  for( i = 0; i < numThreads; i++ ) {
    if( pthread_create( &threads[i], NULL, train_thread, jh) != 0 ) {
      free( threads );
      free( jh );
      return -4;
    }
  }

  neuron_job_t *threadJobs = malloc( numNets * sizeof(neuron_job_t) );
  if( threadJobs == NULL ) {
    free( threads );
    free( jh );
    return -5;
  }
  for( i = 0; i < numNets; i++ ) {
    threadJobs[i].score      = malloc(sizeof(float));
    threadJobs[i].stop       = malloc(sizeof(bool));
    threadJobs[i].done       = malloc(sizeof(bool));
  }

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

  pthread_mutex_init( &net_mutex, NULL );

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
      // Take mutex and create jobs here
      pthread_mutex_lock( &net_mutex );
      for( n = 0; n < population->size; n++ ) {
	threadJobs[n].network    = populationGetIndividual( population, n );
	threadJobs[n].numBits    = numBits;
	threadJobs[n].numRounds  = numRounds;
	threadJobs[n].generation = generation;
	threadJobs[n].seed       = runningSeed;
	*(threadJobs[n].score)   = 0;
	*(threadJobs[n].stop)    = false;
	*(threadJobs[n].done)    = false;

	jobHandlerAddJob( jh, &(threadJobs[n]) );
      }
      pthread_mutex_unlock( &net_mutex );
    }

    // Wait for nets here
    int numReady = 0;
    while( 1 ) {
      int readyCount = 0;

      // Tell threads to quit
      if( saveAllNetsAndQuit ) {
	printf( "Stopping networks\n" );
	for( n = 0; n < population->size; n++ ) {
	  pthread_mutex_lock( &net_mutex );
	  *(threadJobs[n].stop) = true;
	  pthread_mutex_unlock( &net_mutex );
	}

	// Wait for threads to stop
	for( n = 0; n < population->size; n++ ) {
	  if( *(threadJobs[n].done) == false ) {
	    printf( "Waiting for network %d\n", n );
	  }
	  while( *(threadJobs[n].done) == false ) {
	    sched_yield();
	  }
	}

	printf( "Saving all networks and quitting\n" );
	savePopulation( population, -1, generation, runningSeed, numRounds );
	return 0;
      }

      // See if there are any nets that aren't ready yet and wait for them to complete
      for( n = 0; n < population->size; n++ ) {
	pthread_mutex_lock( &net_mutex );
	if( *(threadJobs[n].done) == true ) {
	  readyCount++;
	}
	pthread_mutex_unlock( &net_mutex );
      }

      if( readyCount > numReady ) {
	for( ; numReady < readyCount; numReady++ ) {
	  printf( "." ); fflush(stdout);
	}
      }

      if( readyCount == numNets )
	break;

      // Let other threads do useful stuff
      sched_yield();
    }

    printf( "\n" );

    // All threads are done, tally up results and evolve
    for( n = 0; n < population->size; n++ ) {
      double netScore = *(threadJobs[n].score);

      // If two nets have the same score, let the last one win
      if( !minimise && netScore >= bestScore ) {
	bestScore = netScore;
	bestNet = n;
      } else if( minimise && netScore <= bestScore && netScore >= 0 ) {
	bestScore = netScore;
	bestNet = n;
      }

      populationSetScore( population, n, netScore );

      printf( " - %f / %u (%f)\n", netScore, numRounds, netScore / (double)(numRounds) );
    } // End of population loop

    printf( "  Best score: %f (%f)\n", bestScore, bestScore / (double)(numRounds) );

    // Save the best net here
    savePopulation( population, bestNet, generation, runningSeed, numRounds );

    // Increase how many bits to practice on if network is good enough
    if( (bestScore / (double)(numRounds)) / numBits < bitIncreaseLimit && numBits < maxBits ) {
      numBits++;
    }

    populationRespawn( population, minimise );
  }

  populationDestroy( population );
  
  return 0;
}
