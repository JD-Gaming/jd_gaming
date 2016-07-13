#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>


/*******************************************
 *             Type definitions            *
 *******************************************/
typedef enum activation_type_e {
  activation_linear    = 1 <<  0, // y = x
  activation_relu      = 1 <<  1, // y = x > 0 ? x : 0
  activation_step      = 1 <<  2, // y = x >= 0 ? 1 : 0
  activation_sigmoid   = 1 <<  3, // y = 1 / (1 + exp(-x))
  activation_tanh      = 1 <<  4, // y = 2 / (1 + exp(-2x)) - 1
  activation_atan      = 1 <<  5, // y = atan(x)
  activation_softsign  = 1 <<  6, // y = x / (1 + abs(x))
  activation_softplus  = 1 <<  7, // y = ln(1 + exp(x))
  activation_gaussian  = 1 <<  8, // y = exp(-(x*x))
  activation_sinc      = 1 <<  9, // y = x == 0 ? 1 : sin(x)/x
  activation_sin       = 1 << 10, // y = sin(x)

  // Add any new functions above this value.
  activation_max_bit   = 1 << 11,
  activation_max_shift = 11,
  activation_any       = 0x000
} activation_type_t;

typedef struct network_layer_s {
  // A bit mask of the allowed activation functions used for
  //  initialisation and mutations.
  uint32_t           allowedActivations;
  // Number of neurons in layer.
  uint64_t           numNeurons;
  // Number of connections each neuron has to previous layer.
  uint64_t           numConnections;
  // Seed used for generating connections.
  uint64_t          *seeds;
  // List of connections to previous layer.
  uint64_t         **connections;
  // Weights associated with the above connections.
  float            **weights;
  // Activatiion functions of all neurons.
  activation_type_t *activations;
  // Temporary calculation results.
  float             *values;
} network_layer_t;

typedef struct network_s {
  // Size of network.
  uint64_t          numInputs;
  uint64_t          numLayers;
  network_layer_t **layers;
} network_t;

typedef struct network_layer_params_s {
  uint64_t numNeurons;
  uint64_t numConnections;
  // A bit mask of the allowed activation functions used for
  //  initialisation and mutations.
  uint32_t allowedActivations;
} network_layer_params_t;


/*******************************************
 *        Creation and destruction         *
 *******************************************/
// Create a non-initialized network with the specified dimensions.
//  If number of weights equals number of inputs, the connections will
//  be initalised linearly rather than randomly.
network_t *networkCreate( uint64_t inputs, uint64_t layers, network_layer_params_t *layerParameters, bool initialise );

// Create a duplicate of another network, but with its own memory.
network_t *networkCopy( network_t *network );

// Free memory used by a network.
void networkDestroy( network_t *network );

// Generate a network from a specification file.
network_t *networkLoadFile( char *filename );

// Save a network to a specification file.
bool networkSaveFile( network_t *network, char *filename );

// Generate a network from a byte stream.
network_t *networkUnserialise( uint64_t len, uint8_t *data );

// Generate a byte stream from a network, will allocate memory for *data
//  and return the number of bytes used.  Allocation may not be the same
//  size as the number of bytes used.  Caller is responsible for freeing
//  memory when done.
uint64_t networkSerialise( network_t *network, uint8_t **data );

// Generate a network layer parameter list from an existing network.
network_layer_params_t *networkGetLayerParams( network_t *network );


/*******************************************
 *               Genetics                  *
 *******************************************/
// Randomly combine two networks.  Their dimensions must be identical.
network_t *networkCombine( network_t *mother, network_t *father );

// Randomly change some weight/bias or connection seed in the network.
//  <mutateRate> is a value between 0.0 and 1.0, inclusive
void networkMutate( network_t *network, double mutateRate );


/*******************************************
 *                Running                  *
 *******************************************/
// Run the network once with the specified input array.
void networkRun( network_t *network, float *inputs );

// Get the output value for the specified output neuron.
float networkGetOutputValue( network_t *network, uint64_t idx );

// Get information about dimensions.
uint64_t networkGetNumInputs( network_t *network );
uint64_t networkGetNumLayers( network_t *network );
uint64_t networkGetLayerNumNeurons( network_t *network, uint64_t layer );
uint64_t networkGetNumOutputs( network_t *network );

// Get the number of connections each neuron in a layer has.
uint64_t networkGetLayerNumConnections( network_t *network, uint64_t layer );
uint64_t networkGetOutputNumConnections( network_t *network );

// Manipulate a layer.  If a seed is set to 0, the connections will be linear rather than random.
void              networkSetLayerSeed(       network_t *network, uint64_t layer, uint64_t neuron, uint64_t seed );
uint64_t          networkGetLayerSeed(       network_t *network, uint64_t layer, uint64_t neuron );
void              networkSetLayerBias(       network_t *network, uint64_t layer, uint64_t neuron, float bias );
float             networkGetLayerBias(       network_t *network, uint64_t layer, uint64_t neuron );
void              networkSetLayerWeight(     network_t *network, uint64_t layer, uint64_t neuron, uint64_t source, float weight );
float             networkGetLayerWeight(     network_t *network, uint64_t layer, uint64_t neuron, uint64_t source );
void              networkSetLayerActivation( network_t *network, uint64_t layer, uint64_t neuron, activation_type_t activation );
activation_type_t networkGetLayerActivation( network_t *network, uint64_t layer, uint64_t neuron );

// These functions are merely aliases for the above functions with the layer index set to the last layer.
// Manipulate output layer.  If a seed is set to 0, the connections will be linear rather than random.
void              networkSetOutputSeed(       network_t *network, uint64_t neuron, uint64_t seed );
uint64_t          networkGetOutputSeed(       network_t *network, uint64_t neuron );
void              networkSetOutputBias(       network_t *network, uint64_t neuron, float bias );
float             networkGetOutputBias(       network_t *network, uint64_t neuron );
void              networkSetOutputWeight(     network_t *network, uint64_t neuron, uint64_t source, float weight );
float             networkGetOutputWeight(     network_t *network, uint64_t neuron, uint64_t source );
void              networkSetOutputActivation( network_t *network, uint64_t neuron, activation_type_t activation );
activation_type_t networkGetOutputActivation( network_t *network, uint64_t neuron );

// Print entire network in a JSON like format
void networkPrint( network_t *network );

#endif
