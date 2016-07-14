#ifndef FFN_NETWORK_H
#define FFN_NETWORK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "layer.h"
#include "neurons.h"
#include "activation.h"

/*******************************************
 *             Type definitions            *
 *******************************************/
typedef struct ffn_network_s ffn_network_t;
typedef struct ffn_layer_s ffn_layer_t;
typedef struct ffn_neuron_s ffn_neuron_t;

typedef struct ffn_network_s {
  // Size of network.
  uint64_t      numInputs;
  uint64_t      numLayers;
  ffn_layer_t **layers;
} ffn_network_t;

/*******************************************
 *        Creation and destruction         *
 *******************************************/
// Create a non-initialized network with the specified dimensions.
//  If number of weights equals number of inputs, the connections will
//  be initalised linearly rather than randomly.
ffn_network_t *ffnNetworkCreate( uint64_t inputs, uint64_t layers, ffn_layer_params_t *layerParameters, bool initialise );

// Create a duplicate of another network, but with its own memory.
ffn_network_t *ffnNetworkCopy( ffn_network_t *network );

// Free memory used by a network.
void ffnNetworkDestroy( ffn_network_t *network );

// Generate a network from a specification file.
ffn_network_t *ffnNetworkLoadFile( char *filename );

// Save a network to a specification file.
bool ffnNetworkSaveFile( ffn_network_t *network, char *filename );

// Generate a network from a byte stream.
ffn_network_t *ffnNetworkUnserialise( uint64_t len, uint8_t *data );

// Generate a byte stream from a network, will allocate memory for *data
//  and return the number of bytes used.  Allocation may not be the same
//  size as the number of bytes used.  Caller is responsible for freeing
//  memory when done.
uint64_t ffnNetworkSerialise( ffn_network_t *network, uint8_t **data );

// Generate a network layer parameter list from an existing network.
ffn_layer_params_t *ffnNetworkGetLayerParams( ffn_network_t *network );


/*******************************************
 *               Genetics                  *
 *******************************************/
// Randomly combine two networks.  Their dimensions must be identical.
ffn_network_t *ffnNetworkCombine( ffn_network_t *mother, ffn_network_t *father );

// Randomly change some weight/bias or connection seed in the network.
//  <mutateRate> is a value between 0.0 and 1.0, inclusive
void ffnNetworkMutate( ffn_network_t *network, double mutateRate );


/*******************************************
 *                Running                  *
 *******************************************/
// Run the network once with the specified input array.
void ffnNetworkRun( ffn_network_t *network, float *inputs );

// Get the output value for the specified output neuron.
float ffnNetworkGetOutputValue( ffn_network_t *network, uint64_t idx );

// Get information about dimensions.
uint64_t ffnNetworkGetNumInputs( ffn_network_t *network );
uint64_t ffnNetworkGetNumLayers( ffn_network_t *network );
uint64_t ffnNetworkGetLayerNumNeurons( ffn_network_t *network, uint64_t layer );
uint64_t ffnNetworkGetNumOutputs( ffn_network_t *network );

// Get the number of connections each neuron in a layer has.
uint64_t ffnNetworkGetLayerNumConnections( ffn_network_t *network, uint64_t layer );
uint64_t ffnNetworkGetOutputNumConnections( ffn_network_t *network );

// Manipulate a layer.  If a seed is set to 0, the connections will be linear rather than random.
void              ffnNetworkSetLayerNeuronSeed(       ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t seed );
uint64_t          ffnNetworkGetLayerNeuronSeed(       ffn_network_t *network, uint64_t layer, uint64_t neuron );
void              ffnNetworkSetLayerNeuronBias(       ffn_network_t *network, uint64_t layer, uint64_t neuron, float bias );
float             ffnNetworkGetLayerNeuronBias(       ffn_network_t *network, uint64_t layer, uint64_t neuron );
void              ffnNetworkSetLayerNeuronWeight(     ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t source, float weight );
float             ffnNetworkGetLayerNeuronWeight(     ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t source );
void              ffnNetworkSetLayerNeuronActivation( ffn_network_t *network, uint64_t layer, uint64_t neuron, activation_type_t activation );
activation_type_t ffnNetworkGetLayerNeuronActivation( ffn_network_t *network, uint64_t layer, uint64_t neuron );

// These functions are merely aliases for the above functions with the layer index set to the last layer.
// Manipulate output layer.  If a seed is set to 0, the connections will be linear rather than random.
void              ffnNetworkSetOutputNeuronSeed(       ffn_network_t *network, uint64_t neuron, uint64_t seed );
uint64_t          ffnNetworkGetOutputNeuronSeed(       ffn_network_t *network, uint64_t neuron );
void              ffnNetworkSetOutputNeuronBias(       ffn_network_t *network, uint64_t neuron, float bias );
float             ffnNetworkGetOutputNeuronBias(       ffn_network_t *network, uint64_t neuron );
void              ffnNetworkSetOutputNeuronWeight(     ffn_network_t *network, uint64_t neuron, uint64_t source, float weight );
float             ffnNetworkGetOutputNeuronWeight(     ffn_network_t *network, uint64_t neuron, uint64_t source );
void              ffnNetworkSetOutputNeuronActivation( ffn_network_t *network, uint64_t neuron, activation_type_t activation );
activation_type_t ffnNetworkGetOutputNeuronActivation( ffn_network_t *network, uint64_t neuron );

// Print entire network in a JSON like format
void ffnNetworkPrint( ffn_network_t *network );

#endif
