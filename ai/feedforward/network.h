#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum activation_type_e {
  activation_linear   = 0x00, // y = x
  activation_relu     = 0x01, // y = x > 0 ? x : 0
  activation_step     = 0x02, // y = x >= 0 ? 1 : 0
  activation_sigmoid  = 0x03, // y = 1 / (1 + exp(-x))
  activation_tanh     = 0x04, // y = 2 / (1 + exp(-2x)) - 1
  activation_atan     = 0x05, // y = atan(x)
  activation_softsign = 0x06, // y = x / (1 + abs(x))
  activation_softplus = 0x07, // y = ln(1 + exp(x))
  activation_gaussian = 0x08, // y = exp(-(x*x))
  activation_sinc     = 0x09, // y = x == 0 ? 1 : sin(x)/x
  activarion_sin      = 0x0a, // y = sin(x)
} activation_type_t;

typedef struct network_s {
  uint64_t           numInputs;
  uint64_t           numHidden; // Extend to add more layers later
  uint64_t           numOutputs;

  uint64_t           numHiddenConnections;
  uint64_t           numOutputConnections;

  uint64_t          *hiddenSeeds;
  uint64_t         **hiddenConnections;
  float            **hiddenWeights;
  activation_type_t *hiddenActivations;
  float             *hiddenVals;

  uint64_t          *outputSeeds;
  uint64_t         **outputConnections;
  float            **outputWeights;
  activation_type_t *outputActivations;
  float             *outputVals;
} network_t;

// Create a non-initialized network with the specified dimensions.
//  If number of weights equals number of inputs, the connections will
//  be initalised linearly rather than randomly.
network_t *networkCreate( uint64_t inputs, uint64_t hidden, uint64_t hWeights, uint64_t outputs, uint64_t oWeights, bool initialise );
// Free memory used by a network
void networkDestroy( network_t *network );

// Run the network once with the specified input array
bool networkRun( network_t *network, float *inputs );

// Get the output value for the specified output neuron
float networkGetOutputValue( network_t *network, uint64_t idx );

// Generate a network from a specification file
network_t *networkLoadFile( char *filename );
// Save a network to a specification file
bool networkSaveFile( network_t *network, char *filename );

// Generate a network from a byte stream
network_t *networkUnserialise( uint64_t len, uint8_t *data );
// Generate a byte stream from a network, will allocate memory for *data
//  and return the number of bytes used.  Allocation may not be the same
//  size as the number of bytes used.  Caller is responsible for freeing
//  memory when done.
uint64_t networkSerialise( network_t *network, uint8_t **data );

// Get information about dimensions
uint64_t networkGetNumInputs( network_t *network );
uint64_t networkGetNumHidden( network_t *network );
uint64_t networkGetNumOutputs( network_t *network );

// Get the number of connections each neuron in a layer has
uint64_t networkGetNumHiddenConnections( network_t *network );
uint64_t networkGetNumOutputConnections( network_t *network );

// Manipulate hidden layer.  If a seed is set to 0, the connections will be linear rather than random
void networkSetHiddenSeed( network_t *network, uint64_t idx, uint64_t seed );
uint64_t networkGetHiddenSeed( network_t *network, uint64_t idx );
void networkSetHiddenBias( network_t *network, uint64_t idx, float bias );
float networkGetHiddenBias( network_t *network, uint64_t idx );
void networkSetHiddenWeight( network_t *network, uint64_t idx, uint64_t source, float weight );
float networkGetHiddenWeight( network_t *network, uint64_t idx, uint64_t source );
void networkSetHiddenActivation( network_t *network, uint64_t idx, activation_type_t activation );
activation_type_t networkGetHiddenActivation( network_t *network, uint64_t idx );

// Manipulate output layer.  If a seed is set to 0, the connections will be linear rather than random
void networkSetOutputSeed( network_t *network, uint64_t idx, uint64_t seed );
uint64_t networkGetOutputSeed( network_t *network, uint64_t idx );
void networkSetOutputBias( network_t *network, uint64_t idx, float bias );
float networkGetOutputBias( network_t *network, uint64_t idx );
void networkSetOutputWeight( network_t *network, uint64_t idx, uint64_t source, float weight );
float networkGetOutputWeight( network_t *network, uint64_t idx, uint64_t source );
void networkSetOutputActivation( network_t *network, uint64_t idx, activation_type_t activation );
activation_type_t networkGetOutputActivation( network_t *network, uint64_t idx );


#endif
