#include "neurons.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "pcg_variants.h"

#include "activation.h"

/*******************************************
 *               Local types               *
 *******************************************/
typedef struct ffn_neuron_s {
  // Number of inputs, not necessarily the same as number of connections
  uint64_t numInputs;

  // What type of activation function to use for neuron
  activation_type_t activation;

  // Seed used to randomly connect neuron to inputs
  uint64_t seed;
  // Number of connections to previous layer
  uint64_t numConnections;
  // The array of connections
  uint64_t *connections;

  // The bias value of the neuron, duh
  float bias;

  // Weights, duh x 2
  float *weights;
} ffn_neuron_t;

/*******************************************
 *             Local functions             *
 *******************************************/
// Returns a uniformly distributed random value between low and high inclusive.
static float randomVal( float low, float high )
{
  return (rand() / (float)RAND_MAX) * (high - low) + low;
}

static void mutateValue( float *target )
{
  switch( rand() % 31 ) {
  case 0 ... 9:
    // Add a little
    *target += randomVal( 0, 1 );
    break;

  case 10 ... 19:
    // Remove a little
    *target -= randomVal( 0, 1 );
    break;

  case 20 ... 24:
    // Multiply a little
    *target *= randomVal( 1, 3 );
    break;

  case 25 ... 29:
    // Divide a little
    *target /= randomVal( 1, 3 );
    break;

  case 30:
    // Replace completely
    *target = randomVal( -5, 5 );
  }
}

// A seed = 0 creates a linear mapping rather than a random one
static void createConnections( uint64_t seed, uint64_t sourceSize, uint64_t numConnections, uint64_t positions[] )
{
  if( seed != 0 ) {
    pcg64_random_t rng;

    pcg64_srandom_r(&rng, seed, seed * sourceSize);

    uint64_t i;
    for( i = 0; i < numConnections; i++ ) {
      positions[i] = pcg64_boundedrand_r(&rng, sourceSize);
    }
  } else {
    uint64_t i;
    for( i = 0; i < numConnections; i++ ) {
      positions[i] = i;
    }
  }
}

/*******************************************
 *           Exported functions            *
 *******************************************/
// Creates a new neuron.  <bias> and <weights> are optional arguments that can be used to initialise a previously trained neuron.
ffn_neuron_t *ffnNeuronCreate( uint64_t numInputs, activation_type_t activationType, uint64_t numConnections, uint64_t seed, bool initialise )
{
  uint32_t i;
  ffn_neuron_t *tmp = malloc(sizeof(ffn_neuron_t));
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->numInputs = numInputs;

  tmp->numConnections = numConnections;
  tmp->connections = malloc( sizeof(uint64_t) * numConnections );
  if( tmp->connections == NULL ) {
    free( tmp );
    return NULL;
  }

  tmp->seed = seed;
  createConnections( tmp->seed,
		     numInputs,
		     tmp->numConnections,
		     tmp->connections );

  tmp->weights = malloc( sizeof(float) * numConnections );
  if( tmp->weights == NULL ) {
    fprintf( stderr, "ffnNeuronCreate() - Unable to allocate memory\n" );
    free( tmp->connections );
    free( tmp );
    return NULL;
  }

  tmp->activation = activationType;
  tmp->bias = randomVal( -1, 1 );
  for( i = 0; i < numConnections; i++ ) {
    tmp->weights[i] = randomVal( -1, 1 );
  }

  return tmp;
}

void ffnNeuronDestroy( ffn_neuron_t *neuron )
{
  assert( neuron != NULL );
  free( neuron->connections );
  free( neuron->weights );
  free( neuron );
}

// Run a neuron and return its result
float ffnNeuronRun( ffn_neuron_t *neuron, float *inputs )
{
  assert( neuron != NULL );
  assert( inputs != NULL );

  int i;
  float sum = neuron->bias;

  if( neuron->seed != 0 ) {
    for( i = 0; i < neuron->numConnections; i++ ) {
      sum += inputs[neuron->connections[i]] * neuron->weights[i];
    }
  } else {
    // No point in going through a connection redirection layer if there's no redirection
    for( i = 0; i < neuron->numConnections; i++ ) {
      sum += inputs[i] * neuron->weights[i];
    }
  }

  return activationToFunction( neuron->activation ) ( sum );
}

void ffnNeuronMutate( ffn_neuron_t *neuron, double mutateRate, uint32_t allowedActivations )
{
  int i;

  // Mutate bias
  if( rand() / (RAND_MAX + 1.0) < mutateRate ) {
    mutateValue( &(neuron->bias) );
  }
		 
  // Alter weights a little
  for( i = 0; i < neuron->numConnections; i++ ) {
    if( rand() / (RAND_MAX + 1.0) < mutateRate ) {
      mutateValue( &(neuron->weights[i]) );
    }
  }

  // Change a few activation functions
  if( rand() / (RAND_MAX + 1.0) < mutateRate  / 10.0 ) {
    neuron->activation = randomActivation( allowedActivations );
  }
}

void ffnNeuronSetSeed( ffn_neuron_t *neuron, uint64_t seed )
{
  assert( neuron != NULL );

  if( seed != neuron->seed ) {
    neuron->seed = seed;
    createConnections( neuron->seed,
		       neuron->numInputs,
		       neuron->numConnections,
		       neuron->connections );
  }
}

uint64_t ffnNeuronGetSeed( ffn_neuron_t *neuron )
{
  assert( neuron != NULL );

  return neuron->seed;
}

void ffnNeuronSetBias( ffn_neuron_t *neuron, float bias )
{
  assert( neuron != NULL );

  neuron->bias = bias;
}

float ffnNeuronGetBias( ffn_neuron_t *neuron )
{
  assert( neuron != NULL );

  return neuron->bias;
}

void ffnNeuronSetWeight( ffn_neuron_t *neuron, uint64_t source, float weight )
{
  assert( neuron != NULL );
  assert( source < neuron->numConnections );

  neuron->weights[source] = weight;
}

float ffnNeuronGetWeight( ffn_neuron_t *neuron, uint64_t source )
{
  assert( neuron != NULL );
  assert( source < neuron->numConnections );

  return neuron->weights[source];
}

void ffnNeuronSetActivation( ffn_neuron_t *neuron, activation_type_t activation )
{
  assert( neuron != NULL );

  neuron->activation = activation;
}

activation_type_t ffnNeuronGetActivation( ffn_neuron_t *neuron )
{
  assert( neuron != NULL );

  return neuron->activation;
}

uint64_t ffnNeuronGetConnection( ffn_neuron_t *neuron, uint64_t index )
{
  assert( neuron != NULL );
  assert( index < neuron->numConnections );

  return neuron->connections[index];
}
