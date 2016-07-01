#include "network.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "pcg_variants.h"

// List of static functions used for squashing values
#include "activation.h"

// A seed = 0 creates a linear mapping rather than a random one
static void createConnections( uint64_t seed, uint64_t sourceSize, uint64_t numConnections, uint64_t positions[] )
{
  if( seed != 0 ) {
    pcg64_random_t rng;

    pcg64_srandom_r(&rng, seed, seed * sourceSize);

    uint64_t i;
    for( i = 0; i < numConnections; i++ ) {
      positions[i] = pcg64_boundedrand_r(&rng, sourceSize);
    }
  } else {
    uint64_t i;
    for( i = 0; i < numConnections; i++ ) {
      positions[i] = i;
    }
  }
}

static network_layer_t *createLayer( uint64_t size, uint64_t inputs, uint64_t connections, bool initialise )
{
  uint64_t i, j, last;
  network_layer_t *layer;

  layer = malloc( sizeof(network_layer_t) );
  if( layer == NULL ) {
    goto layer_err_object;
  }

  layer->width = size;
  layer->numConnections = connections;

  // Seeds
  layer->seeds = malloc( layer->width * sizeof(uint64_t) );
  if( layer->seeds == NULL ) {
    goto layer_err_seeds;
  }

  if( initialise ) {
    for( i = 0; i < layer->width; i++ ) {
      layer->seeds[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
  } else {
    for( i = 0; i < layer->width; i++ ) {
      layer->seeds[i] = 0;
    }
  }

  // Connections
  layer->connections = malloc( sizeof(uint64_t*) * layer->width );
  if( layer->connections == NULL ) {
    goto layer_err_connections;
  }

  for( i = 0; i < layer->width; i++ ) {
    layer->connections[i] = malloc( sizeof(uint64_t) * layer->numConnections );
    if( layer->connections[i] == NULL ) {
      last = i;
      goto layer_err_connections_arr;
    }
  }
  // If number of hidden connections is the same as number of inputs, make a linear connection
  if( layer->numConnections != inputs &&
      initialise ) {
    for( i = 0; i < layer->width; i++ ) {
      createConnections( layer->seeds[i],
			 inputs,
			 layer->numConnections,
			 layer->connections[i] );
    }
  } else {
    // Create a linear mapping
    for( i = 0; i < layer->width; i++ ) {
      createConnections( 0,
			 inputs,
			 layer->numConnections,
			 layer->connections[i] );
    }
  }

  // Weights
  layer->weights = malloc( sizeof(float*) * layer->width );
  if( layer->weights == NULL ) {
    goto layer_err_weights;
  }
  for( i = 0; i < layer->width; i++ ) {
    layer->weights[i] = malloc( sizeof(float) * (layer->numConnections + 1) );
    if( layer->weights[i] == NULL ) {
      last = i;
      goto layer_err_weights_arr;
    }
    if( initialise ) {
      for( j = 0; j < layer->numConnections + 1; j++ ) {
	layer->weights[i][j] = ((rand() / (float)RAND_MAX) * 2.0 - 1.0) / layer->numConnections;
      }
    }
  }

  // Activations
  layer->activations = malloc( sizeof(activation_type_t) * layer->width );
  if( layer->activations == NULL ) {
    goto layer_err_activations;
  }
  for( i = 0; i < layer->width; i++ ) {
    layer->activations[i] = (activation_type_t)(rand() % (int)activation_max);
  }

  // Values
  layer->values = malloc( sizeof(float) * layer->width );
  if( layer->values == NULL ) {
    goto layer_err_values;
  }


  // Done
  return layer;


  // Error handling
 layer_err_values:
  free( layer->activations );

 layer_err_activations:
  last = layer->width;

 layer_err_weights_arr:
  for( i = 0; i < last; i++ ) {
    free( layer->weights[i] );
  }
  free( layer->weights );

 layer_err_weights:
  last = layer->width;

 layer_err_connections_arr:
  for( i = 0; i < last; i++ ) {
    free( layer->connections[i] );
  }
  free( layer->connections );

 layer_err_connections:
  free( layer->seeds );

 layer_err_seeds:
  free( layer );

 layer_err_object:
  return NULL;
}

void destroyLayer( network_layer_t *layer )
{
  uint64_t i;
  free( layer->values );
  free( layer->activations );
  for( i = 0; i < layer->width; i++ ) {
    free( layer->weights[i] );
  }
  free( layer->weights );
  for( i = 0; i < layer->width; i++ ) {
    free( layer->connections[i] );
  }
  free( layer->connections );
  free( layer->seeds );
  free( layer );
}

network_t *networkCreate( uint64_t inputs, uint64_t hidden, uint64_t hWeights, uint64_t outputs, uint64_t oWeights, bool initialise )
{
  network_t *tmp = malloc(sizeof(network_t));
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->numInputs = inputs;
  tmp->numHidden = hidden;
  tmp->numOutputs = outputs;

  // Set up memory for hidden layer
  tmp->hiddenLayer = createLayer( hidden, inputs, hWeights, initialise );
  if( tmp->hiddenLayer == NULL ) {
    free( tmp );
    return NULL;
  }

  // Set up memory for output layer
  tmp->outputLayer = createLayer( outputs, hidden, oWeights, initialise );
  if( tmp->outputLayer == NULL ) {
    destroyLayer( tmp->hiddenLayer );
    free( tmp );
    return NULL;
  }

  // Done
  return tmp;
}

void networkDestroy( network_t *network )
{
  destroyLayer( network->outputLayer );
  destroyLayer( network->hiddenLayer );
  free( network );
}

network_t *networkCombine( network_t *mother, network_t *father )
{
  uint64_t i, j;

  if( mother->numInputs  != father->numInputs  ||
      mother->numHidden  != father->numHidden  ||
      mother->numOutputs != father->numOutputs ||
      mother->hiddenLayer->numConnections != father->hiddenLayer->numConnections ||
      mother->outputLayer->numConnections != father->outputLayer->numConnections ) {
    return NULL;
  }

  network_t *tmp = networkCreate( mother->numInputs,
				  mother->numHidden,
				  mother->hiddenLayer->numConnections,
				  mother->numOutputs,
				  mother->outputLayer->numConnections,
				  false );
  if( tmp == NULL ) {
    return NULL;
  }

  for( i = 0; i < tmp->numHidden; i++ ) {
    networkSetHiddenSeed( tmp, i,
			  rand() & 1 ?
			  networkGetHiddenSeed( mother, i ) :
			  networkGetHiddenSeed( father, i ) );

    networkSetHiddenBias( tmp, i,
			  rand() & 1 ?
			  networkGetHiddenBias( mother, i ) :
			  networkGetHiddenBias( father, i ) );

    for( j = 0; j < tmp->hiddenLayer->numConnections; j++ ) {
      networkSetHiddenWeight( tmp, i,
			      j,
			      rand() & 1 ?
			      networkGetHiddenWeight( mother, i, j ) :
			      networkGetHiddenWeight( father, i, j ) );
    }

    networkSetHiddenActivation( tmp, i,
				rand() & 1 ?
				networkGetHiddenActivation( mother, i ) :
				networkGetHiddenActivation( father, i ) );
  }

  for( i = 0; i < tmp->numOutputs; i++ ) {
    networkSetOutputSeed( tmp, i,
			  rand() & 1 ?
			  networkGetOutputSeed( mother, i ) :
			  networkGetOutputSeed( father, i ) );

    networkSetOutputBias( tmp, i,
			  rand() & 1 ?
			  networkGetOutputBias( mother, i ) :
			  networkGetOutputBias( father, i ) );

    for( j = 0; j < tmp->outputLayer->numConnections; j++ ) {
      networkSetOutputWeight( tmp, i,
			      j,
			      rand() & 1 ?
			      networkGetOutputWeight( mother, i, j ) :
			      networkGetOutputWeight( father, i, j ) );
    }

    networkSetOutputActivation( tmp, i,
				rand() & 1 ?
				networkGetOutputActivation( mother, i ) :
				networkGetOutputActivation( father, i ) );
  }

  return tmp;
}

bool networkRun( network_t *network, float *inputs )
{
  uint64_t i, j;
  // Calc hidden layer
  for( i = 0; i < network->hiddenLayer->width; i++ ) {
    float tmpVal = network->hiddenLayer->weights[i][network->hiddenLayer->numConnections]; // Bias
    for( j = 0; j < network->hiddenLayer->numConnections; j++ ) {
      tmpVal += inputs[network->hiddenLayer->connections[i][j]] * network->hiddenLayer->weights[i][j];
    }
    network->hiddenLayer->values[i] = activationToFunction(network->hiddenLayer->activations[i])(tmpVal);
  }

  // Calc output layer
  for( i = 0; i < network->outputLayer->width; i++ ) {
    float tmpVal = network->outputLayer->weights[i][network->outputLayer->numConnections]; // Bias
    for( j = 0; j < network->outputLayer->numConnections; j++ ) {
      tmpVal += network->hiddenLayer->values[network->outputLayer->connections[i][j]] * network->outputLayer->weights[i][j];
    }
    network->outputLayer->values[i] = activationToFunction(network->outputLayer->activations[i])(tmpVal);
  }

  return false;
}

float networkGetOutputValue( network_t *network, uint64_t idx )
{
  return network->outputLayer->values[idx];
}

network_t *networkLoadFile( char *filename )
{
  uint64_t len;
  uint8_t *buf;
  network_t *tmp;

  FILE *file = fopen( filename, "rb" );
  if( file == NULL ) {
    return NULL;
  }

  fseek( file, 0, SEEK_END );
  len = ftell( file );
  rewind( file );

  buf = malloc(len);
  if( fread( buf, 1, len, file ) != len ) {
    printf( "Unable to read entire network definition\n" );
  }

  tmp = networkUnserialise( len, buf );

  free( buf );
  fclose( file );

  return tmp;
}

bool networkSaveFile( network_t *network, char *filename )
{
  uint64_t len;
  uint8_t *buf;

  len = networkSerialise( network, &buf );

  if( len == 0 ) {
    return false;
  }

  FILE *file = fopen( filename, "wb" );
  if( file == NULL ) {
    free( buf );
    return false;
  }

  fwrite( buf, 1, len, file );

  fclose( file );
  free( buf );

  return true;
}

network_t *networkUnserialise( uint64_t len, uint8_t *data )
{
  if( len < (4 + 2) * sizeof(uint64_t) ) {
    return NULL;
  }

  network_t *tmp;
  uint64_t i, h, o, w;

  uint64_t numInputs, numHiddenLayers, numHidden, numOutputs, numHiddenConnections, numOutputConnections;

  i = 0;
  numInputs = 0;
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];
  numInputs <<= 8; numInputs |= data[i++];

  numHiddenLayers = 0;
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  numHiddenLayers <<= 8; numHiddenLayers |= data[i++];
  // Only support a single hidden layer
  if( numHiddenLayers != 1 ) {
    return NULL;
  }

  numHidden = 0;
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];
  numHidden <<= 8; numHidden |= data[i++];

  numOutputs = 0;
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];
  numOutputs <<= 8; numOutputs |= data[i++];

  numHiddenConnections = 0;
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];
  numHiddenConnections <<= 8; numHiddenConnections |= data[i++];

  numOutputConnections = 0;
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];
  numOutputConnections <<= 8; numOutputConnections |= data[i++];

  // Create a network based on the base parameters, but don't initialise it
  tmp = networkCreate( numInputs, numHidden, numHiddenConnections, numOutputs, numOutputConnections, false );
  if( tmp == NULL ) {
    return NULL;
  }

  // Read hidden layer
  for( h = 0; h < tmp->hiddenLayer->width; h++ ) {
    uint64_t seed = 0;
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    networkSetHiddenSeed( tmp, h, seed );
  }

  for( h = 0; h < tmp->hiddenLayer->width; h++ ) {
    for( w = 0; w < tmp->hiddenLayer->numConnections + 1; w++ ) {
      uint32_t val = 0;
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];

      // Implicitly handles the bias
      float *unPunned = (float*)(&val);
      networkSetHiddenWeight( tmp, h, w, *unPunned );
    }
  }

  for( h = 0; h < tmp->hiddenLayer->width; h++ ) {
    networkSetHiddenActivation( tmp, h, (activation_type_t)data[i++] );
  }

  // Read output layer
  for( o = 0; o < tmp->outputLayer->width; o++ ) {
    uint64_t seed = 0;
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    seed <<= 8; seed |= data[i++];
    networkSetOutputSeed( tmp, o, seed );
  }

  for( o = 0; o < tmp->outputLayer->width; o++ ) {
    for( w = 0; w < tmp->outputLayer->numConnections + 1; w++ ) {
      uint32_t val = 0;
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];
      val <<= 8; val |= data[i++];

      // Implicitly handles the bias
      float *unPunned = (float*)(&val);
      networkSetOutputWeight( tmp, o, w, *unPunned );
    }
  }

  for( o = 0; o < tmp->outputLayer->width; o++ ) {
    networkSetOutputActivation( tmp, o, (activation_type_t)data[i++] );
  }

  return tmp;
}

uint64_t networkSerialise( network_t *network, uint8_t **data )
{
  uint64_t length = 0;
  uint8_t *bytes;

  // numInputs, <reserved>, numHidden, numOutputs
  length += 4 * sizeof(uint64_t);
  // numHiddenConnections, numOutputConnections
  length += 2 * sizeof(uint64_t);
  // Hidden seeds
  length += sizeof(uint64_t) * network->hiddenLayer->width;
  // Hidden weights
  length += sizeof(float) * (network->hiddenLayer->width * (network->hiddenLayer->numConnections + 1));
  // Hidden activations
  length += network->hiddenLayer->width;
  // Output seeds
  length += sizeof(uint64_t) * network->outputLayer->width;
  // Output weights
  length += sizeof(float) * (network->outputLayer->width * (network->outputLayer->numConnections + 1));
  // Output activations
  length += network->outputLayer->width;

  bytes = malloc(length);
  if( bytes == NULL ) {
    return 0;
  }

  uint64_t i, h, o, w;

  i = 0;

  // Dimensions
  bytes[i++] = (network->numInputs >> 56) & 0xff;
  bytes[i++] = (network->numInputs >> 48) & 0xff;
  bytes[i++] = (network->numInputs >> 40) & 0xff;
  bytes[i++] = (network->numInputs >> 32) & 0xff;
  bytes[i++] = (network->numInputs >> 24) & 0xff;
  bytes[i++] = (network->numInputs >> 16) & 0xff;
  bytes[i++] = (network->numInputs >>  8) & 0xff;
  bytes[i++] = (network->numInputs >>  0) & 0xff;

  // Reserved, to be used for future multiple hidden layers
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 0;
  bytes[i++] = 1;

  bytes[i++] = (network->hiddenLayer->width >> 56) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >> 48) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >> 40) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >> 32) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >> 24) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >> 16) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >>  8) & 0xff;
  bytes[i++] = (network->hiddenLayer->width >>  0) & 0xff;

  bytes[i++] = (network->outputLayer->width >> 56) & 0xff;
  bytes[i++] = (network->outputLayer->width >> 48) & 0xff;
  bytes[i++] = (network->outputLayer->width >> 40) & 0xff;
  bytes[i++] = (network->outputLayer->width >> 32) & 0xff;
  bytes[i++] = (network->outputLayer->width >> 24) & 0xff;
  bytes[i++] = (network->outputLayer->width >> 16) & 0xff;
  bytes[i++] = (network->outputLayer->width >>  8) & 0xff;
  bytes[i++] = (network->outputLayer->width >>  0) & 0xff;

  // Connections
  bytes[i++] = (network->hiddenLayer->numConnections >> 56) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >> 48) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >> 40) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >> 32) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >> 24) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >> 16) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >>  8) & 0xff;
  bytes[i++] = (network->hiddenLayer->numConnections >>  0) & 0xff;

  bytes[i++] = (network->outputLayer->numConnections >> 56) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >> 48) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >> 40) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >> 32) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >> 24) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >> 16) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >>  8) & 0xff;
  bytes[i++] = (network->outputLayer->numConnections >>  0) & 0xff;

  // Hidden
  for( h = 0; h < network->hiddenLayer->width; h++ ) {
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 56) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 48) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 40) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 32) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 24) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >> 16) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >>  8) & 0xff;
    bytes[i++] = (network->hiddenLayer->seeds[h] >>  0) & 0xff;
  }

  for( h = 0; h < network->hiddenLayer->width; h++ ) {
    for( w = 0; w < network->hiddenLayer->numConnections + 1; w++ ) {
      uint32_t tmp = *((uint32_t*)(&(network->hiddenLayer->weights[h][w])));
      bytes[i++] = (tmp >> 24) & 0xff;
      bytes[i++] = (tmp >> 16) & 0xff;
      bytes[i++] = (tmp >>  8) & 0xff;
      bytes[i++] = (tmp >>  0) & 0xff;
    }
  }

  for( h = 0; h < network->hiddenLayer->width; h++ ) {
    bytes[i++] = (uint8_t)network->hiddenLayer->activations[h];
  }

  // Output
  for( o = 0; o < network->outputLayer->width; o++ ) {
    bytes[i++] = (network->outputLayer->seeds[o] >> 56) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >> 48) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >> 40) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >> 32) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >> 24) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >> 16) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >>  8) & 0xff;
    bytes[i++] = (network->outputLayer->seeds[o] >>  0) & 0xff;
  }

  for( o = 0; o < network->outputLayer->width; o++ ) {
    for( w = 0; w < network->outputLayer->numConnections + 1; w++ ) {
      uint32_t tmp = *((uint32_t*)(&(network->outputLayer->weights[o][w])));
      bytes[i++] = (tmp >> 24) & 0xff;
      bytes[i++] = (tmp >> 16) & 0xff;
      bytes[i++] = (tmp >>  8) & 0xff;
      bytes[i++] = (tmp >>  0) & 0xff;
    }
  }

  for( o = 0; o < network->outputLayer->width; o++ ) {
    bytes[i++] = (uint8_t)network->outputLayer->activations[o];
  }

  *data = bytes;
  return length;
}

uint64_t networkGetNumInputs( network_t *network )
{
  return network->numInputs;
}

uint64_t networkGetNumHidden( network_t *network )
{
  return network->hiddenLayer->width;
}

uint64_t networkGetNumOutputs( network_t *network )
{
  return network->outputLayer->width;
}

uint64_t networkGetNumHiddenConnections( network_t *network )
{
  return network->hiddenLayer->numConnections;
}

uint64_t networkGetNumOutputConnections( network_t *network )
{
  return network->outputLayer->numConnections;
}

void networkSetHiddenSeed( network_t *network, uint64_t idx, uint64_t seed )
{
  if( seed != network->hiddenLayer->seeds[idx] ) {
    network->hiddenLayer->seeds[idx] = seed;
    createConnections( network->hiddenLayer->seeds[idx],
		       network->numInputs,
		       network->hiddenLayer->numConnections,
		       network->hiddenLayer->connections[idx] );
  }
}

uint64_t networkGetHiddenSeed( network_t *network, uint64_t idx )
{
  return network->hiddenLayer->seeds[idx];
}

void networkSetHiddenBias( network_t *network, uint64_t idx, float bias )
{
  network->hiddenLayer->weights[idx][network->hiddenLayer->numConnections] = bias;
}

float networkGetHiddenBias( network_t *network, uint64_t idx )
{
  return network->hiddenLayer->weights[idx][network->hiddenLayer->numConnections];
}

void networkSetHiddenWeight( network_t *network, uint64_t idx, uint64_t source, float weight )
{
  network->hiddenLayer->weights[idx][source] = weight;
}

float networkGetHiddenWeight( network_t *network, uint64_t idx, uint64_t source )
{
  return network->hiddenLayer->weights[idx][source];
}

void networkSetHiddenActivation( network_t *network, uint64_t idx, activation_type_t activation )
{
  network->hiddenLayer->activations[idx] = activation;
}

activation_type_t networkGetHiddenActivation( network_t *network, uint64_t idx )
{
  return network->hiddenLayer->activations[idx];
}

void networkSetOutputSeed( network_t *network, uint64_t idx, uint64_t seed )
{
  if( seed != network->outputLayer->seeds[idx] ) {
    network->outputLayer->seeds[idx] = seed;
    createConnections( network->outputLayer->seeds[idx],
		       network->hiddenLayer->width,
		       network->outputLayer->numConnections,
		       network->outputLayer->connections[idx] );
  }
}

uint64_t networkGetOutputSeed( network_t *network, uint64_t idx )
{
  return network->outputLayer->seeds[idx];
}

void networkSetOutputBias( network_t *network, uint64_t idx, float bias )
{
  network->outputLayer->weights[idx][network->outputLayer->numConnections] = bias;
}

float networkGetOutputBias( network_t *network, uint64_t idx )
{
  return network->outputLayer->weights[idx][network->outputLayer->numConnections];
}

void networkSetOutputWeight( network_t *network, uint64_t idx, uint64_t source, float weight )
{
  network->outputLayer->weights[idx][source] = weight;
}

float networkGetOutputWeight( network_t *network, uint64_t idx, uint64_t source )
{
  return network->outputLayer->weights[idx][source];
}

void networkSetOutputActivation( network_t *network, uint64_t idx, activation_type_t activation )
{
  network->outputLayer->activations[idx] = activation;
}

activation_type_t networkGetOutputActivation( network_t *network, uint64_t idx )
{
  return network->outputLayer->activations[idx];
}


