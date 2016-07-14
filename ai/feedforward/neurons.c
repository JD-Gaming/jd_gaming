#include "neurons.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include "activation.h"

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

/*******************************************
 *           Exported functions            *
 *******************************************/
// Creates a new neuron.  <bias> and <weights> are optional arguments that can be used to initialise a previously trained neuron.
ffn_neuron_t *ffnNeuronCreate( activation_type_t activationType, uint64_t numConnections, uint64_t seed, bool initialise )
{
  uint32_t i;
  ffn_neuron_t *tmp = malloc(sizeof(ffn_neuron_t));
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->seed = seed;

  tmp->numConnections = numConnections;
  tmp->connections = malloc( sizeof(uint64_t) * numConnections );
  if( tmp->connections == NULL ) {
    free( tmp );
    return NULL;
  }

  // TODO: Initialise connections

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

  for( i = 0; i < neuron->numConnections; i++ ) {
    // TODO: Go through connections array
    sum += inputs[i] * neuron->weights[i];
  }

  return activationToFunction( neuron->activation ) ( sum );
}

void ffnNeuronMutate( ffn_neuron_t *neuron, double mutateRate )
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
//  if( rand() / (RAND_MAX + 1.0) < mutateRate ) {
//    layer->activations[i] = randomActivation( layer->allowedActivations );
//  }
}
