#include "network.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "pcg_variants.h"

// List of static functions used for squashing values
#include "activation.h"

/*******************************************
 *             Local functions             *
 *******************************************/
static float randomVal( float min, float max )
{
  return (rand() / (float)RAND_MAX) * (max - min) + min;
}

static activation_type_t randomActivation( int32_t allowedActivations )
{
  if( allowedActivations != activation_any ) {
    activation_type_t tmp;
    do {
      int shift = rand() % activation_max_shift;
      tmp = 1 << shift;
    } while( !((int32_t)tmp & allowedActivations) );
    return tmp;
  }

  int shift = rand() % activation_max_shift;
  return 1 << shift;
}

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

static network_layer_t *createLayer( uint64_t size, uint64_t inputs, uint64_t connections, uint32_t allowedActivations, bool initialise )
{
  uint64_t i, j, last;
  network_layer_t *layer;

  layer = malloc( sizeof(network_layer_t) );
  if( layer == NULL ) {
    goto layer_err_object;
  }

  layer->allowedActivations = allowedActivations;
  layer->numNeurons = size;
  layer->numConnections = connections;

  // Seeds
  layer->seeds = malloc( layer->numNeurons * sizeof(uint64_t) );
  if( layer->seeds == NULL ) {
    goto layer_err_seeds;
  }

  // If number of hidden connections is the same as number of inputs, make a linear connection
  if( initialise &&
      layer->numConnections != inputs ) {
    for( i = 0; i < layer->numNeurons; i++ ) {
      layer->seeds[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
  } else {
    for( i = 0; i < layer->numNeurons; i++ ) {
      layer->seeds[i] = 0;
    }
  }

  // Connections
  layer->connections = malloc( sizeof(uint64_t*) * layer->numNeurons );
  if( layer->connections == NULL ) {
    goto layer_err_connections;
  }

  for( i = 0; i < layer->numNeurons; i++ ) {
    layer->connections[i] = malloc( sizeof(uint64_t) * layer->numConnections );
    if( layer->connections[i] == NULL ) {
      last = i;
      goto layer_err_connections_arr;
    }
  }

  // Create connections even if initialise isn't set, seeds will be 0 so
  //  initialisation will be linear
  for( i = 0; i < layer->numNeurons; i++ ) {
    createConnections( layer->seeds[i],
		       inputs,
		       layer->numConnections,
		       layer->connections[i] );
  }

  // Weights
  layer->weights = malloc( sizeof(float*) * layer->numNeurons );
  if( layer->weights == NULL ) {
    goto layer_err_weights;
  }
  for( i = 0; i < layer->numNeurons; i++ ) {
    layer->weights[i] = malloc( sizeof(float) * (layer->numConnections + 1) );
    if( layer->weights[i] == NULL ) {
      last = i;
      goto layer_err_weights_arr;
    }
    if( initialise ) {
      for( j = 0; j < layer->numConnections + 1; j++ ) {
	layer->weights[i][j] = randomVal( -1.0 / layer->numConnections, 1.0 / layer->numConnections );
      }
    }
  }

  // Activations
  layer->activations = malloc( sizeof(activation_type_t) * layer->numNeurons );
  if( layer->activations == NULL ) {
    goto layer_err_activations;
  }
  for( i = 0; i < layer->numNeurons; i++ ) {
    layer->activations[i] = randomActivation( layer->allowedActivations );
  }

  // Values
  layer->values = malloc( sizeof(float) * layer->numNeurons );
  if( layer->values == NULL ) {
    goto layer_err_values;
  }


  // Done
  return layer;


  // Error handling
 layer_err_values:
  free( layer->activations );

 layer_err_activations:
  last = layer->numNeurons;

 layer_err_weights_arr:
  for( i = 0; i < last; i++ ) {
    free( layer->weights[i] );
  }
  free( layer->weights );

 layer_err_weights:
  last = layer->numNeurons;

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

static void destroyLayer( network_layer_t *layer )
{
  assert( layer != NULL );

  uint64_t i;
  free( layer->values );
  free( layer->activations );
  for( i = 0; i < layer->numNeurons; i++ ) {
    free( layer->weights[i] );
  }
  free( layer->weights );
  for( i = 0; i < layer->numNeurons; i++ ) {
    free( layer->connections[i] );
  }
  free( layer->connections );
  free( layer->seeds );
  free( layer );
}

// mutateRate is a value between 0 and 1
static void networkMutateLayer( network_layer_t *layer, double mutateRate )
{
  uint64_t i, j;
  for( i = 0; i < layer->numNeurons; i++ ) {
    // Don't mess with seed values because they change too much

    // Alter weights a little
    for( j = 0; j < layer->numConnections+1; j++ ) {
      if( rand() / (RAND_MAX + 1.0) < mutateRate ) {
	switch( rand() % 31 ) {
	case 0 ... 9:
	  // Add a little
	  layer->weights[i][j] += randomVal( 0, 1 );
	  break;

	case 10 ... 19:
	  // Remove a little
	  layer->weights[i][j] -= randomVal( 0, 1 );
	  break;

	case 20 ... 24:
	  // Multiply a little
	  layer->weights[i][j] *= randomVal( 1, 3 );
	  break;

	case 25 ... 29:
	  // Divide a little
	  {
	    layer->weights[i][j] /= randomVal( 1, 3 );
	    break;
	  }

	case 30:
	  // Replace completely
	  layer->weights[i][j] = randomVal( -10, 10 );
	}
      }
    }

    // Change a few activation functions
    if( rand() / (RAND_MAX + 1.0) < mutateRate ) {
      layer->activations[i] = randomActivation( layer->allowedActivations );
    }
  }
}

static bool layerRun( network_layer_t *layer, float *inputs )
{
  assert( layer != NULL );
  assert( inputs != NULL );

  uint64_t neur, src;

  for( neur = 0; neur < layer->numNeurons; neur++ ) {
    float tmpVal = layer->weights[neur][layer->numConnections]; // Bias
    for( src = 0; src < layer->numConnections; src++ ) {
      tmpVal += inputs[layer->connections[neur][src]] * layer->weights[neur][src];
    }
    layer->values[neur] = activationToFunction(layer->activations[neur])(tmpVal);
  }
  return true;
}


/*******************************************
 *           Exported functions            *
 *******************************************/
network_t *networkCreate( uint64_t inputs, uint64_t layers, network_layer_params_t *layerParameters, bool initialise )
{
  assert( inputs >= 1 );
  assert( layers >= 1 );
  assert( layerParameters != NULL );

  network_t *tmp = malloc(sizeof(network_t));
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->numInputs = inputs;
  tmp->numLayers = layers;

  tmp->layers = malloc( layers * sizeof(network_layer_t) );
  if( tmp->layers == NULL ) {
    free( tmp );
    return NULL;
  }

  // Special handling of first layer
  tmp->layers[0] = createLayer( layerParameters[0].numNeurons,
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
    tmp->layers[i] = createLayer( layerParameters[i].numNeurons,
				  layerParameters[i-1].numNeurons, layerParameters[i].numConnections,
				  layerParameters[0].allowedActivations, initialise );
    if( tmp->layers[i] == NULL ) {
      do {
	destroyLayer( tmp->layers[i] );
      } while( i-- );
      free( tmp->layers );
      free( tmp );
    }
  }

  // Done
  return tmp;
}

network_t *networkCopy( network_t *network )
{
  assert( network != NULL );

  network_layer_params_t *layerParams = networkGetLayerParams( network );
  if( layerParams == NULL ) {
    return NULL;
  }

  network_t *tmp = networkCreate( network->numInputs,
				  network->numLayers,
				  layerParams,
				  false );
  free( layerParams );
  if( tmp == NULL ) {
    return NULL;
  }

  uint64_t lay, neur, src;

  for( lay = 0; lay < tmp->numLayers; lay++ ) {
    for( neur = 0; neur < networkGetLayerNumNeurons( network, lay ); neur++ ) {
      networkSetLayerSeed( tmp, lay, neur, networkGetLayerSeed( network, lay, neur ) );

      networkSetLayerBias( tmp, lay, neur, networkGetLayerBias( network, lay, neur ) );

      for( src = 0; src < networkGetLayerNumConnections( network, lay ); src++ ) {
	networkSetLayerWeight( tmp, lay, neur, src, networkGetLayerWeight( network, lay, neur, src ) );
      }

      networkSetLayerActivation( tmp, lay, neur, networkGetLayerActivation( network, lay, neur ) );
    }
  }

  return tmp;
}

void networkDestroy( network_t *network )
{
  assert( network != NULL );

  uint64_t lay;
  for( lay = 0; lay < networkGetNumLayers( network ); lay++ ) {
    destroyLayer( network->layers[lay] );
  }
  free( network->layers );
  free( network );
}

network_t *networkCombine( network_t *mother, network_t *father )
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

  network_layer_params_t *layerParams = networkGetLayerParams( mother );
  if( layerParams == NULL ) {
    return NULL;
  }

  network_t *tmp = networkCreate( mother->numInputs,
				  mother->numLayers,
				  layerParams,
				  false );
  free( layerParams );
  if( tmp == NULL ) {
    return NULL;
  }

  for( lay = 0; lay < tmp->numLayers; lay++ ) {
    for( neur = 0; neur < tmp->layers[lay]->numNeurons; neur++ ) {
      networkSetLayerSeed( tmp, lay, neur,
			    rand() & 1 ?
			    networkGetLayerSeed( mother, lay, neur ) :
			    networkGetLayerSeed( father, lay, neur ) );

      networkSetLayerBias( tmp, lay, neur,
			    rand() & 1 ?
			    networkGetLayerBias( mother, lay, neur ) :
			    networkGetLayerBias( father, lay, neur ) );

      for( src = 0; src < tmp->layers[lay]->numConnections; src++ ) {
	networkSetLayerWeight( tmp, lay, neur, src,
			       rand() & 1 ?
			       networkGetLayerWeight( mother, lay, neur, src ) :
			       networkGetLayerWeight( father, lay, neur, src ) );
      }

      networkSetLayerActivation( tmp, lay, neur,
				  rand() & 1 ?
				  networkGetLayerActivation( mother, lay, neur ) :
				  networkGetLayerActivation( father, lay, neur ) );
    }
  }

  return tmp;
}


void networkMutate( network_t *network, double mutateRate )
{
  assert( network != NULL );

  uint64_t lay;

  networkMutateLayer( network->layers[0], mutateRate );
  for( lay = 1; lay < network->numLayers; lay++ ) {
    networkMutateLayer( network->layers[lay], mutateRate );
  }
}

void networkRun( network_t *network, float *inputs )
{
  assert( network != NULL );
  assert( inputs != NULL );

  uint64_t lay;

  // Special treatment for first layer
  layerRun( network->layers[0], inputs );

  // Any remaining layers
  for( lay = 1; lay < network->numLayers; lay++ ) {
    layerRun( network->layers[lay], network->layers[lay-1]->values );
  }
}

float networkGetOutputValue( network_t *network, uint64_t idx )
{
  assert( network != NULL );
  assert( network->numLayers > 0 );
  assert( idx < network->layers[network->numLayers-1]->numNeurons );

  return network->layers[network->numLayers-1]->values[idx];
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
    fprintf( stderr, "Unable to read entire network definition\n" );
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
  assert( data != NULL );

  if( len < 2 * sizeof(uint64_t) ) {
    return NULL;
  }

  network_t *tmp;
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

  network_layer_params_t *layerParams = malloc( sizeof(network_layer_params_t) * numLayers );
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
  tmp = networkCreate( numInputs, numLayers, layerParams, false );
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
      networkSetLayerSeed( tmp, lay, neur, seed );

      for( src = 0; src < tmp->layers[lay]->numConnections + 1; src++ ) {
	uint32_t val = 0;
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];
	val <<= 8; val |= data[i++];

	// Implicitly handles the bias
	float *unPunned = (float*)(&val);
	networkSetLayerWeight( tmp, lay, neur, src, *unPunned );
      }

      networkSetLayerActivation( tmp, lay, neur, (activation_type_t)data[i++] );
    }
  }

  return tmp;
}

uint64_t networkSerialise( network_t *network, uint8_t **data )
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
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 56) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 48) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 40) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 32) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 24) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >> 16) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >>  8) & 0xff;
      bytes[i++] = (network->layers[lay]->seeds[neur] >>  0) & 0xff;

      // WTF HERE?
      for( src = 0; src < network->layers[lay]->numConnections + 1; src++ ) {
	uint32_t tmp = *((uint32_t*)(&(network->layers[lay]->weights[neur][src])));
	bytes[i++] = (tmp >> 24) & 0xff;
	bytes[i++] = (tmp >> 16) & 0xff;
	bytes[i++] = (tmp >>  8) & 0xff;
	bytes[i++] = (tmp >>  0) & 0xff;
      }

      bytes[i++] = (uint8_t)network->layers[lay]->activations[neur];
    }
  }

  *data = bytes;
  return length;
}

network_layer_params_t *networkGetLayerParams( network_t *network )
{
  assert( network != NULL );

  network_layer_params_t *tmp = malloc( sizeof(network_layer_params_t) * network->numLayers );
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

uint64_t networkGetNumInputs( network_t *network )
{
  assert( network != NULL );
  return network->numInputs;
}

uint64_t networkGetNumLayers( network_t *network )
{
  assert( network != NULL );
  return network->numLayers;
}

uint64_t networkGetLayerNumNeurons( network_t *network, uint64_t layer )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  return network->layers[layer]->numNeurons;
}

uint64_t networkGetNumOutputs( network_t *network )
{
  assert( network != NULL );
  return networkGetLayerNumNeurons( network, network->numLayers - 1 );
}

uint64_t networkGetLayerNumConnections( network_t *network, uint64_t layer )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  return network->layers[layer]->numConnections;
}

uint64_t networkGetNumOutputConnections( network_t *network )
{
  assert( network != NULL );
  return networkGetLayerNumConnections( network, network->numLayers - 1 );
}

void networkSetLayerSeed( network_t *network, uint64_t layer, uint64_t idx, uint64_t seed )
{
  assert( network != NULL );
  if( seed != network->layers[layer]->seeds[idx] ) {
    network->layers[layer]->seeds[idx] = seed;
    createConnections( network->layers[layer]->seeds[idx],
		       layer == 0 ? network->numInputs : network->layers[layer-1]->numNeurons,
		       network->layers[layer]->numConnections,
		       network->layers[layer]->connections[idx] );
  }
}

uint64_t networkGetLayerSeed( network_t *network, uint64_t layer, uint64_t idx )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );

  return network->layers[layer]->seeds[idx];
}

void networkSetLayerBias( network_t *network, uint64_t layer, uint64_t idx, float bias )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );

  network->layers[layer]->weights[idx][network->layers[layer]->numConnections] = bias;
}

float networkGetLayerBias( network_t *network, uint64_t layer, uint64_t idx )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );

  return network->layers[layer]->weights[idx][network->layers[layer]->numConnections];
}

void networkSetLayerWeight( network_t *network, uint64_t layer, uint64_t idx, uint64_t source, float weight )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );
  assert( source < network->layers[layer]->numConnections + 1 ); // Allow bias

  network->layers[layer]->weights[idx][source] = weight;
}

float networkGetLayerWeight( network_t *network, uint64_t layer, uint64_t idx, uint64_t source )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );
  assert( source < network->layers[layer]->numConnections + 1 ); // Allow bias

  return network->layers[layer]->weights[idx][source];
}

void networkSetLayerActivation( network_t *network, uint64_t layer, uint64_t idx, activation_type_t activation )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );

  network->layers[layer]->activations[idx] = activation;
}

activation_type_t networkGetLayerActivation( network_t *network, uint64_t layer, uint64_t idx )
{
  assert( network != NULL );
  assert( layer < network->numLayers );
  assert( idx < network->layers[layer]->numNeurons );

  return network->layers[layer]->activations[idx];
}

void networkSetOutputSeed( network_t *network, uint64_t idx, uint64_t seed )
{
  networkSetLayerSeed( network, network->numLayers - 1, idx, seed );
}

uint64_t networkGetOutputSeed( network_t *network, uint64_t idx )
{
  return networkGetLayerSeed( network, network->numLayers - 1, idx );
}

void networkSetOutputBias( network_t *network, uint64_t idx, float bias )
{
  networkSetLayerBias( network, network->numLayers - 1, idx, bias );
}

float networkGetOutputBias( network_t *network, uint64_t idx )
{
  return networkGetLayerBias( network, network->numLayers - 1, idx );
}

void networkSetOutputWeight( network_t *network, uint64_t idx, uint64_t source, float weight )
{
  networkSetLayerWeight( network, network->numLayers - 1, idx, source, weight );
}

float networkGetOutputWeight( network_t *network, uint64_t idx, uint64_t source )
{
  return networkGetLayerWeight( network, network->numLayers - 1, idx, source );
}

void networkSetOutputActivation( network_t *network, uint64_t idx, activation_type_t activation )
{
  networkSetLayerActivation( network, network->numLayers - 1, idx, activation );
}

activation_type_t networkGetOutputActivation( network_t *network, uint64_t idx )
{
  return networkGetLayerActivation( network, network->numLayers - 1, idx );
}

void networkPrint( network_t *network )
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
      printf( "        0x%016llx,\n", (unsigned long long)(network->layers[i]->seeds[j]) );
    }
    printf( "      ],\n" );
    printf( "      connections: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        [\n" );
      for( k = 0; k < network->layers[i]->numConnections; k++ ) {
	printf( "          %llu,\n", (unsigned long long)(network->layers[i]->connections[j][k]) );
      }
      printf( "        ],\n" );
    }
    printf( "      ],\n" );
    printf( "      weights: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        [\n" );
      for( k = 0; k < network->layers[i]->numConnections; k++ ) {
	printf( "          %f,\n", network->layers[i]->weights[j][k] );
      }
      printf( "        ],\n" );
    }
    printf( "      ],\n" );
    printf( "      activations: [\n" );
    for( j = 0; j < network->layers[i]->numNeurons; j++ ) {
      printf( "        0x%04x,\n", network->layers[i]->activations[j] );
    }
    printf( "      ],\n" );
    printf( "    },\n" );
  }
  printf( "  ]\n" );
  printf( "}\n" );
}
