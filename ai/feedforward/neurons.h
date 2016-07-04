#ifndef NEURONS_H
#define NEURONS_H

#include <stdint.h>

typedef struct neuron_s neuron_t;

typedef struct neuron_s {
  // Calculates the next value using synapses et c, inputs must be the same size as num_synapses
  float (*calc) ( neuron_t *neuron, neuron_t **inputs );
  // The bias value of the neuron, duh
  float bias;
  // The array of synapses
  uint32_t num_synapses;
  float *synapses;
  // The last calculated value
  float value;
} neuron_t;

// Creates a new neuron.  <bias> and <synapses> are optional arguments that can be used to initialise a previously trained neuron.
neuron_t *createSigmoid( uint32_t num_synapses, float *bias, float *synapses );

#endif
