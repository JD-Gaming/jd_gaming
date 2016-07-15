#ifndef NEURONS_H
#define NEURONS_H

#include <stdint.h>
#include <stdbool.h>

#include "activation.h"

typedef struct ffn_neuron_s ffn_neuron_t;

// Creates a new neuron
ffn_neuron_t *ffnNeuronCreate( uint64_t numInputs, activation_type_t activationType, uint64_t numConnections, uint64_t seed, bool initialise );

// Free memory et c.
void ffnNeuronDestroy( ffn_neuron_t *neuron );

// Run a neuron and return its result.
float ffnNeuronRun( ffn_neuron_t *neuron, float *inputs );

// Perform random mutations in the neuron.
void ffnNeuronMutate( ffn_neuron_t *neuron, double mutateRate, uint32_t allowedActivations );

// Neuron manipulation functions
void              ffnNeuronSetSeed(       ffn_neuron_t *neuron, uint64_t seed );
uint64_t          ffnNeuronGetSeed(       ffn_neuron_t *neuron );
void              ffnNeuronSetBias(       ffn_neuron_t *neuron, float bias );
float             ffnNeuronGetBias(       ffn_neuron_t *neuron );
void              ffnNeuronSetWeight(     ffn_neuron_t *neuron, uint64_t source, float weight );
float             ffnNeuronGetWeight(     ffn_neuron_t *neuron, uint64_t source );
void              ffnNeuronSetActivation( ffn_neuron_t *neuron, activation_type_t activation );
activation_type_t ffnNeuronGetActivation( ffn_neuron_t *neuron );

uint64_t          ffnNeuronGetConnection( ffn_neuron_t *neuron, uint64_t index );

#endif
