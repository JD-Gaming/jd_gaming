#ifndef FFN_LAYER_H
#define FFN_LAYER_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "activation.h"
#include "neurons.h"

/*******************************************
 *             Type definitions            *
 *******************************************/
typedef struct ffn_layer_s ffn_layer_t;

/*******************************************
 *        Creation and destruction         *
 *******************************************/
// Create a new layer with the given parameters.  If <initialise> is true, neurons are 
//  randomly generated, otherwise they will be created empty.
ffn_layer_t *ffnLayerCreate( uint64_t size, uint64_t inputs, uint64_t connections, uint32_t allowedActivations, bool initialise );

// Destroys a layer and frees its memory.
void ffnLayerDestroy( ffn_layer_t *layer );

/*******************************************
 *           Exported functions            *
 *******************************************/
// Performs random mutations in a single layer. 
//  <mutateRate> is a value between 0 and 1
void ffnLayerMutate( ffn_layer_t *layer, double mutateRate );

// Performs all calculations for a layer.
bool ffnLayerRun( ffn_layer_t *layer, float *inputs );

// Layer manipulation functions
uint64_t ffnLayerGetNumConnections( ffn_layer_t *layer );
uint64_t ffnLayerGetNumNeurons( ffn_layer_t *layer );
uint32_t ffnLayerGetAllowedActivations( ffn_layer_t *layer );

// Returns an array containing the latest values calculated by the layer.
//  The pointer can be used as input for other networks or layers.
float *ffnLayerGetValues( ffn_layer_t *layer );

// Returns a value calculated by the layer.
float ffnLayerGetValue( ffn_layer_t *layer, uint64_t neuron );

// Neuron manipulation functions
void              ffnLayerSetNeuronSeed(       ffn_layer_t *layer, uint64_t neuron, uint64_t seed );
uint64_t          ffnLayerGetNeuronSeed(       ffn_layer_t *layer, uint64_t neuron );
void              ffnLayerSetNeuronBias(       ffn_layer_t *layer, uint64_t neuron, float bias );
float             ffnLayerGetNeuronBias(       ffn_layer_t *layer, uint64_t neuron );
void              ffnLayerSetNeuronWeight(     ffn_layer_t *layer, uint64_t neuron, uint64_t source, float weight );
float             ffnLayerGetNeuronWeight(     ffn_layer_t *layer, uint64_t neuron, uint64_t source );
void              ffnLayerSetNeuronActivation( ffn_layer_t *layer, uint64_t neuron, activation_type_t activation );
activation_type_t ffnLayerGetNeuronActivation( ffn_layer_t *layer, uint64_t neuron );
uint64_t          ffnLayerGetNeuronConnection( ffn_layer_t *layer, uint64_t neuron, uint64_t index );
#endif
