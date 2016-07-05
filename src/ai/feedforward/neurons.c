#include "neurons.h"

#include <stdlib.h>
#include <math.h>

static float sigmoid( neuron_t *neuron, neuron_t **inputs )
{
  uint32_t i;

  float sum = neuron->bias;
  for( i = 0; i < neuron->num_synapses; i++ ) {
    sum += neuron->synapses[i] * inputs[i]->value;
  }

  neuron->value = 1 / (1 + exp(-sum));
  
  return neuron->value;
}

// Returns a uniformly distributed random value between low and high inclusive.
static float randomVal( float low, float high )
{
  return (rand() / (float)RAND_MAX) * (high - low) + low;
}

neuron_t *createSigmoid( uint32_t num_synapses, float *bias, float *synapses )
{
  uint32_t i;
  neuron_t *tmp = malloc(sizeof(neuron_t));

  tmp->calc = sigmoid;
 
  if( bias ) {
    tmp->bias = *bias;
  } else {
    tmp->bias = randomVal( -1, 1 );
  }

  tmp->num_synapses = num_synapses;
  tmp->synapses = malloc( sizeof(float) * num_synapses );
  if( synapses ) {
    for( i = 0; i < num_synapses; i++ ) {
      tmp->synapses[i] = synapses[i];
    }
  } else {
    for( i = 0; i < num_synapses; i++ ) {
      tmp->synapses[i] = randomVal( -1, 1 );
    }
  }
  
  tmp->value = 0;
  
  return tmp;
}
