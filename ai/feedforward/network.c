#include "network.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>


#include "activation.h"

/*******************************************
 *             Local functions             *
 *******************************************/


/*******************************************
 *           Exported functions            *
 *******************************************/
ffn_network_t *ffnNetworkCreate( uint64_t inputs, uint64_t layers, ffn_layer_params_t *layerParameters, bool initialise )
{
  assert( inputs >= 1 );
  assert( layers >= 1 );
  assert( layerParameters != NULL );

  ffn_network_t *tmp = malloc(sizeof(ffn_network_t));
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->numInputs = inputs;
  tmp->numLayers = layers;

  tmp->layers = malloc( layers * sizeof(ffn_layer_t) );
  if( tmp->layers == NULL ) {
    free( tmp );
    return NULL;
  }

  tmp->layers[0] = ffnLayerCreate( layerParameters[0].numNeurons,
				   inputs, layerParameters[0].numConnections,
				   layerParameters[0].allowedActivations, initialise );
  if( tmp->layers[0] == NULL ) {
    free( tmp->layers );
    free( tmp );
    return NULL;
  }

  // Create any extra layers
  uint64_t i;
  for( i = 1; i < layers; i++ ) {
    tmp->layers[i] = ffnLayerCreate( layerParameters[i].numNeurons,
				     layerParameters[i-1].numNeurons, layerParameters[i].numConnections,
				     layerParameters[0].allowedActivations, initialise );
    if( tmp->layers[i] == NULL ) {
      do {
	ffnLayerDestroy( tmp->layers[i] );
      } while( i-- );
      free( tmp->layers );
      free( tmp );
    }
  }

  // Done
  return tmp;
}

ffn_network_t *ffnNetworkCopy( ffn_network_t *network )
{
  assert( network != NULL );

  ffn_layer_params_t *layerParams = ffnNetworkGetLayerParams( network );
  if( layerParams == NULL ) {
    return NULL;
  }

  ffn_network_t *tmp = ffnNetworkCreate( network->numInputs,
					 network->numLayers,
					 layerParams,
					 false );
  free( layerParams );
  if( tmp == NULL ) {
    return NULL;
  }

  uint64_t lay, neur, src;

  for( lay = 0; lay < tmp->numLayers; lay++ ) {
    for( neur = 0; neur < ffnNetworkGetLayerNumNeurons( network, lay ); neur++ ) {
      ffnNetworkSetLayerNeuronSeed( tmp, lay, neur, ffnNetworkGetLayerNeuronSeed( network, lay, neur ) );

      ffnNetworkSetLayerNeuronBias( tmp, lay, neur, ffnNetworkGetLayerNeuronBias( network, lay, neur ) );

      for( src = 0; src < ffnNetworkGetLayerNumConnections( network, lay ); src++ ) {
	ffnNetworkSetLayerNeuronWeight( tmp, lay, neur, src, ffnNetworkGetLayerNeuronWeight( network, lay, neur, src ) );
      }

      ffnNetworkSetLayerNeuronActivation( tmp, lay, neur, ffnNetworkGetLayerNeuronActivation( network, lay, neur ) );
    }
  }

  return tmp;
}

void ffnNetworkDestroy( ffn_network_t *network )
{
  assert( network != NULL );

  uint64_t lay;
  for( lay = 0; lay < ffnNetworkGetNumLayers( network ); lay++ ) {
    ffnLayerDestroy( network->layers[lay] );
  }
  free( network->layers );
  free( network );
}

ffn_network_t *ffnNetworkCombine( ffn_network_t *mother, ffn_network_t *father )
{
  assert( mother != NULL );
  assert( father != NULL );

  uint64_t lay, neur, src;

  // Networks have to have exactly the same structure
  if( mother->numInputs  != father->numInputs ||
      mother->numLayers  != father->numLayers ) {
    return NULL;
  }
  for( lay = 0; lay < mother->numLayers; lay++ ) {
    if( mother->layers[lay]->numConnections != father->layers[lay]->numConnections ) {
      return NULL;
    }
  }

  ffn_layer_params_t *layerParams = ffnNetworkGetLayerParams( mother );
  if( layerParams == NULL ) {
    return NULL;
  }

  ffn_network_t *tmp = ffnNetworkCreate( mother->numInputs,
					 mother->numLayers,
					 layerParams,
					 false );
  free( layerParams );
  if( tmp == NULL ) {
    return NULL;
  }

  for( lay = 0; lay < tmp->numLayers; lay++ ) {
    for( neur = 0; neur < tmp->layers[lay]->numNeurons; neur++ ) {
      ffnNetworkSetLayerNeuronSeed( tmp, lay, neur,
				    rand() & 1 ?
				    ffnNetworkGetLayerNeuronSeed( mother, lay, neur ) :
				    ffnNetworkGetLayerNeuronSeed( father, lay, neur ) );

      ffnNetworkSetLayerNeuronBias( tmp, lay, neur,
				    rand() & 1 ?
				    ffnNetworkGetLayerNeuronBias( mother, lay, neur ) :
				    ffnNetworkGetLayerNeuronBias( father, lay, neur ) );

      for( src = 0; src < tmp->layers[lay]->numConnections; src++ ) {
	ffnNetworkSetLayerNeuronWeight( tmp, lay, neur, src,
					rand() & 1 ?
					ffnNetworkGetLayerNeuronWeight( mother, lay, neur, src ) :
					ffnNetworkGetLayerNeuronWeight( father, lay, neur, src ) );
      }

      ffnNetworkSetLayerNeuronActivation( tmp, lay, neur,
					  rand() & 1 ?
					  ffnNetworkGetLayerNeuronActivation( mother, lay, neur ) :
					  ffnNetworkGetLayerNeuronActivation( father, lay, neur ) );
    }
  }

  return tmp;
}


void ffnNetworkMutate( ffn_network_t *network, double mutateRate )
{
  assert( network != NULL );

  uint64_t lay;

  for( lay = 0; lay < network->numLayers; lay++ ) {
    ffnLayerMutate( network->layers[lay], mutateRate );
  }
}

void ffnNetworkRun( ffn_network_t *network, float *inputs )
{
  assert( network != NULL );
  assert( inputs != NULL );

  uint64_t lay;

  // Special treatment for first layer
  ffnLayerRun( network->layers[0], inputs );

  // Any remaining layers
  for( lay = 1; lay < network->numLayers; lay++ ) {
    ffnLayerRun( network->layers[lay], network->layers[lay-1]->values );
  }
}

float ffnNetworkGetOutputValue( ffn_network_t *network, uint64_t idx )
{
  assert( network != NULL );
  assert( network->numLayers > 0 );
  assert( idx < network->layers[network->numLayers-1]->numNeurons );

  return network->layers[network->numLayers-1]->values[idx];
}

ffn_network_t *ffnNetworkLoadFile( char *filename )
{
  uint64_t len;
  uint8_t *buf;
  ffn_network_t *tmp;

  FILE *file = fopen( filename, "rb" );
  if( file == NULL ) {
    return NULL;
  }

  fseek( file, 0, SEEK_END );
  len = ftell( file );
  rewind( file );

  buf = malloc(len);
  if( fread( buf, 1, len, file ) != len ) {
    fprintf( stderr, "Unable to read entire network definition\n" );
  }

  tmp = ffnNetworkUnserialise( len, buf );

  free( buf );
  fclose( file );

  return tmp;
}

bool ffnNetworkSaveFile( ffn_network_t *network, char *filename )
{
  uint64_t len;
  uint8_t *buf;

  len = ffnNetworkSerialise( network, &buf );

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

ffn_network_t *ffnNetworkUnserialise( uint64_t len, uint8_t *data )
{
  assert( data != NULL );

  if( len < 2 * sizeof(uint64_t) ) {
    return NULL;
  }

  ffn_network_t *tmp;
  uint64_t i, lay, neur, src;

  uint64_t numInputs, numLayers;

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

  numLayers = 0;
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  numLayers <<= 8; numLayers |= data[i++];
  if( numLayers < 1 ) {
    return NULL;
  }

  ffn_layer_params_t *layerParams = malloc( sizeof(ffn_layer_params_t) * numLayers );
  if( layerParams == NULL ) {
    return NULL;
  }

  for( lay = 0; lay < numLayers; lay++ ) {
    layerParams[lay].allowedActivations = 0;
    layerParams[lay].allowedActivations <<= 8; layerParams[lay].allowedActivations |= data[i++];
    layerParams[lay].allowedActivations <<= 8; layerParams[lay].allowedActivations |= data[i++];
    layerParams[lay].allowedActivations <<= 8; layerParams[lay].allowedActivations |= data[i++];
    layerParams[lay].allowedActivations <<= 8; layerParams[lay].allowedActivations |= data[i++];

    layerParams[lay].numNeurons = 0;
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];
    layerParams[lay].numNeurons <<= 8; layerParams[lay].numNeurons |= data[i++];

    layerParams[lay].numConnections = 0;
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
    layerParams[lay].numConnections <<= 8; layerParams[lay].numConnections |= data[i++];
  }

  // Create a network based on the base parameters, but don't initialise it
  tmp = ffnNetworkCreate( numInputs, numLayers, layerParams, false );
  free( layerParams );

  if( tmp == NULL ) {
    return NULL;
  }

  // Read layers
  for( lay = 0; lay < numLayers; lay++ ) {
    for( neur = 0; neur < tmp->layers[lay]->numNeurons; neur++ ) {
      uint64_t seed = 0;
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      seed <<= 8; seed |= data[i++];
      ffnNetworkSetLayerNeuronSeed( tmp, lay, neur, seed );

      for( src = 0; src < tmp->layers[lay]->numConnections; src++ ) {
	uint32_t val = 0;
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];

	// Implicitly handles the bias
	float *unPunned = (float*)(&val);
	ffnNetworkSetLayerNeuronWeight( tmp, lay, neur, src, *unPunned );
      }

      {
	uint32_t val = 0;
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];

	// Implicitly handles the bias
	float *unPunned = (float*)(&val);
	ffnNetworkSetLayerNeuronBias( tmp, lay, neur,*unPunned );
      }

      ffnNetworkSetLayerNeuronActivation( tmp, lay, neur, (activation_type_t)data[i++] );
    }
  }

  return tmp;
}

uint64_t ffnNetworkSerialise( ffn_network_t *network, uint8_t **data )
{
  assert( network != NULL );
  assert( data != NULL );

  uint64_t length = 0;
  uint8_t *bytes;

  uint64_t i, lay, neur, src;

  // numInputs, numLayers
  length += 2 * sizeof(uint64_t);
  // numLayers * (numNeurons, numConnections)
  length += network->numLayers * 2 * sizeof(uint64_t);

  for( lay = 0; lay < network->numLayers; lay++ ) {
    // Allowed activations
    length += sizeof(uint32_t);
    // Hidden seeds
    length += network->layers[lay]->numNeurons * sizeof(uint64_t);
    // Hidden weights + bias
    length += (network->layers[lay]->numNeurons * (network->layers[lay]->numConnections + 1)) * sizeof(float);
    // Hidden activations
    length += network->layers[lay]->numNeurons;
  }

  bytes = malloc(length);
  if( bytes == NULL ) {
    return 0;
  }

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

  bytes[i++] = (network->numLayers >> 56) & 0xff;
  bytes[i++] = (network->numLayers >> 48) & 0xff;
  bytes[i++] = (network->numLayers >> 40) & 0xff;
  bytes[i++] = (network->numLayers >> 32) & 0xff;
  bytes[i++] = (network->numLayers >> 24) & 0xff;
  bytes[i++] = (network->numLayers >> 16) & 0xff;
  bytes[i++] = (network->numLayers >>  8) & 0xff;
  bytes[i++] = (network->numLayers >>  0) & 0xff;

  for( lay = 0; lay < network->numLayers; lay++ ) {
    bytes[i++] = (network->layers[lay]->allowedActivations >> 24) & 0xff;
    bytes[i++] = (network->layers[lay]->allowedActivations >> 16) & 0xff;
    bytes[i++] = (network->layers[lay]->allowedActivations >>  8) & 0xff;
    bytes[i++] = (network->layers[lay]->allowedActivations >>  0) & 0xff;

    bytes[i++] = (network->layers[lay]->numNeurons >> 56) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >> 48) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >> 40) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >> 32) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >> 24) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >> 16) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >>  8) & 0xff;
    bytes[i++] = (network->layers[lay]->numNeurons >>  0) & 0xff;

    // Connections
    bytes[i++] = (network->layers[lay]->numConnections >> 56) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >> 48) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >> 40) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >> 32) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >> 24) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >> 16) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >>  8) & 0xff;
    bytes[i++] = (network->layers[lay]->numConnections >>  0) & 0xff;
  }

  for( lay = 0; lay < network->numLayers; lay++ ) {
    // Hidden
    for( neur = 0; neur < network->layers[lay]->numNeurons; neur++ ) {
      uint64_t seed = ffnLayerGetNeuronSeed( network->layers[lay], neur );
      bytes[i++] = (seed >> 56) & 0xff;
      bytes[i++] = (seed >> 48) & 0xff;
      bytes[i++] = (seed >> 40) & 0xff;
      bytes[i++] = (seed >> 32) & 0xff;
      bytes[i++] = (seed >> 24) & 0xff;
      bytes[i++] = (seed >> 16) & 0xff;
      bytes[i++] = (seed >>  8) & 0xff;
      bytes[i++] = (seed >>  0) & 0xff;

      for( src = 0; src < network->layers[lay]->numConnections; src++ ) {
	float weight = ffnLayerGetNeuronWeight( network->layers[lay], neur, src );
	uint32_t tmp = *((uint32_t*)(&weight));
	bytes[i++] = (tmp >> 24) & 0xff;
	bytes[i++] = (tmp >> 16) & 0xff;
	bytes[i++] = (tmp >>  8) & 0xff;
	bytes[i++] = (tmp >>  0) & 0xff;
      }

      {
	float bias = ffnLayerGetNeuronBias( network->layers[lay], neur );
	uint32_t tmp = *((uint32_t*)(&bias));
	bytes[i++] = (tmp >> 24) & 0xff;
	bytes[i++] = (tmp >> 16) & 0xff;
	bytes[i++] = (tmp >>  8) & 0xff;
	bytes[i++] = (tmp >>  0) & 0xff;
      }

      bytes[i++] = (uint8_t)ffnLayerGetNeuronActivation( network->layers[lay], neur );
    }
  }

  *data = bytes;
  return length;
}

ffn_layer_params_t *ffnNetworkGetLayerParams( ffn_network_t *network )
{
  assert( network != NULL );

  ffn_layer_params_t *tmp = malloc( sizeof(ffn_layer_params_t) * network->numLayers );
  if( tmp == NULL ) {
    return NULL;
  }

  uint64_t i;
  for( i = 0; i < network->numLayers; i++ ) {
    tmp[i].allowedActivations = network->layers[i]->allowedActivations;
    tmp[i].numNeurons = network->layers[i]->numNeurons;
    tmp[i].numConnections = network->layers[i]->numConnections;
  }

  return tmp;
}

uint64_t ffnNetworkGetNumInputs( ffn_network_t *network )
{
  assert( network != NULL );
  return network->numInputs;
}

uint64_t ffnNetworkGetNumLayers( ffn_network_t *network )
{
  assert( network != NULL );
  return network->numLayers;
}

uint64_t ffnNetworkGetLayerNumNeurons( ffn_network_t *network, uint64_t layer )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  return network->layers[layer]->numNeurons;
}

uint64_t ffnNetworkGetNumOutputs( ffn_network_t *network )
{
  assert( network != NULL );
  return ffnNetworkGetLayerNumNeurons( network, network->numLayers - 1 );
}

uint64_t ffnNetworkGetLayerNumConnections( ffn_network_t *network, uint64_t layer )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  return network->layers[layer]->numConnections;
}

uint64_t ffnNetworkGetNumOutputConnections( ffn_network_t *network )
{
  assert( network != NULL );
  return ffnNetworkGetLayerNumConnections( network, network->numLayers - 1 );
}

void ffnNetworkSetLayerNeuronSeed( ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t seed )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  ffnLayerSetNeuronSeed( network->layers[layer], neuron, seed );
}

uint64_t ffnNetworkGetLayerNeuronSeed( ffn_network_t *network, uint64_t layer, uint64_t neuron )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  return ffnLayerGetNeuronSeed( network->layers[layer], neuron );
}

void ffnNetworkSetLayerNeuronBias( ffn_network_t *network, uint64_t layer, uint64_t neuron, float bias )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  ffnLayerSetNeuronBias( network->layers[layer], neuron, bias );
}

float ffnNetworkGetLayerNeuronBias( ffn_network_t *network, uint64_t layer, uint64_t neuron )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  return ffnLayerGetNeuronBias( network->layers[layer], neuron );
}

void ffnNetworkSetLayerNeuronWeight( ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t source, float weight )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );
  assert( source < network->layers[layer]->numConnections );

  ffnLayerSetNeuronWeight( network->layers[layer], neuron, source, weight );
}

float ffnNetworkGetLayerNeuronWeight( ffn_network_t *network, uint64_t layer, uint64_t neuron, uint64_t source )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );
  assert( source < network->layers[layer]->numConnections );

  return ffnLayerGetNeuronWeight( network->layers[layer], neuron, source );
}

void ffnNetworkSetLayerNeuronActivation( ffn_network_t *network, uint64_t layer, uint64_t neuron, activation_type_t activation )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  ffnLayerSetNeuronActivation( network->layers[layer], neuron, activation );
}

activation_type_t ffnNetworkGetLayerNeuronActivation( ffn_network_t *network, uint64_t layer, uint64_t neuron )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( neuron < network->layers[layer]->numNeurons );

  return ffnLayerGetNeuronActivation( network->layers[layer], neuron );
}

void ffnNetworkSetOutputSeed( ffn_network_t *network, uint64_t neuron, uint64_t seed )
{
  ffnNetworkSetLayerNeuronSeed( network, network->numLayers - 1, neuron, seed );
}

uint64_t ffnNetworkGetOutputSeed( ffn_network_t *network, uint64_t neuron )
{
  return ffnNetworkGetLayerNeuronSeed( network, network->numLayers - 1, neuron );
}

void ffnNetworkSetOutputBias( ffn_network_t *network, uint64_t neuron, float bias )
{
  ffnNetworkSetLayerNeuronBias( network, network->numLayers - 1, neuron, bias );
}

float ffnNetworkGetOutputBias( ffn_network_t *network, uint64_t neuron )
{
  return ffnNetworkGetLayerNeuronBias( network, network->numLayers - 1, neuron );
}

void ffnNetworkSetOutputWeight( ffn_network_t *network, uint64_t neuron, uint64_t source, float weight )
{
  ffnNetworkSetLayerNeuronWeight( network, network->numLayers - 1, neuron, source, weight );
}

float ffnNetworkGetOutputWeight( ffn_network_t *network, uint64_t neuron, uint64_t source )
{
  return ffnNetworkGetLayerNeuronWeight( network, network->numLayers - 1, neuron, source );
}

void ffnNetworkSetOutputActivation( ffn_network_t *network, uint64_t neuron, activation_type_t activation )
{
  ffnNetworkSetLayerNeuronActivation( network, network->numLayers - 1, neuron, activation );
}

activation_type_t ffnNetworkGetOutputActivation( ffn_network_t *network, uint64_t neuron )
{
  return ffnNetworkGetLayerNeuronActivation( network, network->numLayers - 1, neuron );
}

void ffnNetworkPrint( ffn_network_t *network )
{
  uint64_t i, j, k;

  printf( "net: {\n" );
  printf( "  numInputs: %d\n", (int)network->numInputs );
  printf( "  numLayers: %d\n", (int)network->numLayers );
  printf( "  layers: [\n" );
  for( i = 0; i < network->numLayers; i++ ) {
    printf( "    {\n" );
    printf( "      allowedActivations: 0x%08x,\n", network->layers[i]->allowedActivations );
    printf( "      numNeurons:         %llu,\n", (unsigned long long)(network->layers[i]->numNeurons) );
    printf( "      numConnections:     %llu,\n", (unsigned long long)(network->layers[i]->numConnections) );
    printf( "      seeds: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        0x%016llx,\n", (unsigned long long)(ffnLayerGetNeuronSeed( network->layers[i], j )) );
    }
    printf( "      ],\n" );
    /*
      TODO: Fix connections

    printf( "      connections: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        [\n" );
      for( k = 0; k < network->layers[i]->numConnections; k++ ) {
	printf( "          %llu,\n", (unsigned long long)(network->layers[i]->neurons[j]->connections[k]) );
      }
      printf( "        ],\n" );
    }
    printf( "      ],\n" );
    */
    printf( "      bias: %f,\n", ffnLayerGetNeuronBias( network->layers[i], j ) );

    printf( "      weights: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        [\n" );
      for( k = 0; k < network->layers[i]->numConnections; k++ ) {
	printf( "          %f,\n", ffnLayerGetNeuronWeight( network->layers[i], j, k ) );
      }
      printf( "        ],\n" );
    }
    printf( "      ],\n" );
    printf( "      activations: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        0x%04x,\n", ffnLayerGetNeuronActivation( network->layers[i], j ) );
    }
    printf( "      ],\n" );
    printf( "    },\n" );
  }
  printf( "  ]\n" );
  printf( "}\n" );
}
