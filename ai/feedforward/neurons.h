#ifndef NEURONS_H
#define NEURONS_H

#include <stdint.h>
#include <stdbool.h>

#include "activation.h"

typedef struct ffn_neuron_s {
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

// Creates a new neuron
ffn_neuron_t *ffnNeuronCreate( activation_type_t activationType, uint64_t numConnections, uint64_t seed, bool initialise );

// Free memory et c.
void ffnNeuronDestroy( ffn_neuron_t *neuron );

// Run a neuron and return its result.
float ffnNeuronRun( ffn_neuron_t *neuron, float *inputs );

// Perform random mutations in the neuron.
void ffnNeuronMutate( ffn_neuron_t *neuron, double mutateRate );

#endif
