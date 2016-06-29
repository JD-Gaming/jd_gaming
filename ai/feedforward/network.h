#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <stdbool.h>

typedef enum activation_type_e {
  activation_linear,   // y = x
  activation_relu,     // y = x > 0 ? x : 0
  activation_step,     // y = x >= 0 ? 1 : 0
  activation_sigmoid,  // y = 1 / (1 + exp(-x))
  activation_tanh,     // y = 2 / (1 + exp(-2x)) - 1
  activation_atan,     // y = atan(x)
  activation_softsign, // y = x / (1 + abs(x))
  activation_softplus, // y = ln(1 + exp(x))
  activation_gaussian, // y = exp(-(x*x))
  activation_sinc,     // y = x == 0 ? 1 : sin(x)/x
  activarion_sin,      // y = sin(x)
} activation_type_t;

typedef struct network_s {
  size_t numInputs;
  size_t numHidden; // Extend to add more layers later
  size_t numOutputs;

  float **hiddenWeights;
  activation_type_t *hiddenActivations;
  float *hiddenVals;
  float **outputWeights;
  activation_type_t *outputActivations;
  float *outputVals;
} network_t;

// Create a non-initialized network with the specified dimensions
network_t *networkCreate( size_t inputs, size_t hidden, size_t outputs );
// Free memory used by a network
void networkDestroy( network_t *network );

// Run the network once with the specified input array
bool networkRun( network_t *network, float *inputs );

// Get the output value for the specified output neuron
float networkGetOutputValue( network_t *network, size_t idx );

// Generate a network from a specification file
network_t *networkLoadFile( char *filename );
// Save a network to a specification file
bool networkSaveFile( network_t *network, char *filename );

// Generate a network from a byte stream
network_t *networkUnserialise( size_t len, unsigned char *data );
// Generate a byte stream from a network, will allocate memory for *data
//  and return the number of bytes used.  Allocation may not be the same
//  size as the number of bytes used.  Caller is responsible for freeing
//  memory when done.
size_t networkSerialise( network_t *network, unsigned char **data );

// Get information about dimensions
size_t networkGetNumInputs( network_t *network );
size_t networkGetNumHidden( network_t *network );
size_t networkGetNumOutputs( network_t *network );

// Manipulate hidden layer
void networkSetHiddenBias( network_t *network, size_t idx, float bias );
float networkGetHiddenBias( network_t *network, size_t idx );
void networkSetHiddenWeight( network_t *network, size_t idx, size_t source, float weight );
float networkGetHiddenWeight( network_t *network, size_t idx, size_t source );
void networkSetHiddenActivation( network_t *network, size_t idx, activation_type_t activation );
activation_type_t networkGetHiddenActivation( network_t *network, size_t idx );

// Manipulate output layer
void networkSetOutputBias( network_t *network, size_t idx, float bias );
float networkGetOutputBias( network_t *network, size_t idx );
void networkSetOutputWeight( network_t *network, size_t idx, size_t source, float weight );
float networkGetOutputWeight( network_t *network, size_t idx, size_t source );
void networkSetOutputActivation( network_t *network, size_t idx, activation_type_t activation );
activation_type_t networkGetOutputActivation( network_t *network, size_t idx );


#endif
