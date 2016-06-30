#include "network.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "pcg_variants.h"

typedef float (*act_func) ( float val );

static float act_linear( float val )
{
  return val;
}

static float act_relu( float val )
{
  return val > 0 ? val : 0.0;
}

static float act_step( float val )
{
  return val >= 0 ? 1.0 : 0.0;
}

static float act_sigmoid( float val )
{
  return 1.0 / (1.0 + expf(-val));
}

static float act_tanh( float val )
{
  return 2.0 / (1.0 + expf(-2 * val)) - 1.0;
}

static float act_atan( float val )
{
  return atanf(val);
}

static float act_softsign( float val )
{
  return val / (1.0 + fabsf(val));
}

static float act_softplus( float val )
{
  return logf(1 + expf(val));
}

static float act_gaussian( float val )
{
  return expf(-(val*val));
}

static float act_sinc( float val )
{
  return val == 0 ? 1.0 : sinf(val)/val;
}

static float act_sin( float val )
{
  return sinf(val);
}

static act_func activationToFunction( activation_type_t activation )
{
  switch( activation ) {
  case activation_linear:
    return act_linear;
  case activation_relu:
    return act_relu;
  case activation_step:
    return act_step;
  case activation_sigmoid:
    return act_sigmoid;
  case activation_tanh:
    return act_tanh;
  case activation_atan:
    return act_atan;
  case activation_softsign:
    return act_softsign;
  case activation_softplus:
    return act_softplus;
  case activation_gaussian:
    return act_gaussian;
  case activation_sinc:
    return act_sinc;
  case activarion_sin:
    return act_sin;
  }

  return act_linear;
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

network_t *networkCreate( uint64_t inputs, uint64_t hidden, uint64_t hWeights, uint64_t outputs, uint64_t oWeights, bool initialise )
{
  uint64_t i, last;

  network_t *tmp = malloc(sizeof(network_t));
  if( tmp == NULL ) {
    goto err_object;
  }

  tmp->numInputs = inputs;
  tmp->numHidden = hidden;
  tmp->numOutputs = outputs;

  tmp->numHiddenConnections = hWeights;
  tmp->numOutputConnections = oWeights;

  // Set up memory for hidden layer
  tmp->hiddenSeeds = malloc(sizeof(uint64_t) * tmp->numHidden);
  if( tmp->hiddenSeeds == NULL ) {
    goto err_hidden_seeds;
  }
  if( initialise ) {
    for( i = 0; i < tmp->numHidden; i++ ) {
      tmp->hiddenSeeds[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
  } else {
    for( i = 0; i < tmp->numHidden; i++ ) {
      tmp->hiddenSeeds[i] = 0;
    }
  }

  tmp->hiddenConnections = malloc(sizeof(uint64_t*) * tmp->numHidden);
  if( tmp->hiddenConnections == NULL ) {
    goto err_hidden_connections;
  }

  for( i = 0; i < tmp->numHidden; i++ ) {
    tmp->hiddenConnections[i] = malloc(sizeof(uint64_t) * tmp->numHiddenConnections);
    if( tmp->hiddenConnections[i] == NULL ) {
      last = i;
      goto err_hidden_connections_arr;
    }
  }
  // If number of hidden connections is the same as number of inputs, make a linear connection
  if( tmp->numHiddenConnections != tmp->numInputs &&
      initialise ) {
    for( i = 0; i < tmp->numHidden; i++ ) {
      createConnections( tmp->hiddenSeeds[i],
			 tmp->numInputs,
			 tmp->numHiddenConnections,
			 tmp->hiddenConnections[i] );
    }
  } else {
    // Create a linear mapping
    for( i = 0; i < tmp->numHidden; i++ ) {
      createConnections( 0,
			 tmp->numInputs,
			 tmp->numHiddenConnections,
			 tmp->hiddenConnections[i] );
    }
  }

  tmp->hiddenWeights = malloc(sizeof(float*) * tmp->numHidden);
  if( tmp->hiddenWeights == NULL ) {
    goto err_hidden_weights;
  }

  for( i = 0; i < tmp->numHidden; i++ ) {
    tmp->hiddenWeights[i] = malloc(sizeof(float) * (tmp->numHiddenConnections + 1));
    if( tmp->hiddenWeights[i] == NULL ) {
      last = i;
      goto err_hidden_weights_arr;
    }
  }

  tmp->hiddenActivations = malloc(sizeof(activation_type_t) * tmp->numHidden);
  if( tmp->hiddenActivations == NULL ) {
    goto err_hidden_activations;
  }

  tmp->hiddenVals = malloc(sizeof(float) * tmp->numHidden);
  if( tmp->hiddenVals == NULL ) {
    goto err_hidden_vals;
  }


  // Set up memory for output layer
  tmp->outputSeeds = malloc(sizeof(uint64_t) * tmp->numOutputs);
  if( tmp->outputSeeds == NULL ) {
    goto err_output_seeds;
  }
  if( initialise ) {
    for( i = 0; i < tmp->numOutputs; i++ ) {
      tmp->outputSeeds[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
  } else {
    for( i = 0; i < tmp->numOutputs; i++ ) {
      tmp->outputSeeds[i] = 0;
    }
  }
  tmp->outputConnections = malloc(sizeof(uint64_t*) * tmp->numOutputs);
  if( tmp->outputConnections == NULL ) {
    goto err_output_connections;
  }

  for( i = 0; i < tmp->numOutputs; i++ ) {
    tmp->outputConnections[i] = malloc(sizeof(uint64_t) * tmp->numOutputConnections);
    if( tmp->outputConnections[i] == NULL ) {
      last = i;
      goto err_output_connections_arr;
    }
  }
  // If number of output connections is the same as number of hidden neurons, make a linear connection
  if( tmp->numOutputConnections != tmp->numHidden &&
      initialise ) {
    for( i = 0; i < tmp->numOutputs; i++ ) {
      createConnections( tmp->outputSeeds[i],
			 tmp->numHidden,
			 tmp->numOutputConnections,
			 tmp->outputConnections[i] );
    }
  } else {
    // Create a linear mapping
    for( i = 0; i < tmp->numOutputs; i++ ) {
      createConnections( 0,
			 tmp->numHidden,
			 tmp->numOutputConnections,
			 tmp->outputConnections[i] );
    }
  }

  tmp->outputWeights = malloc(sizeof(float*) * tmp->numOutputs);
  if( tmp->outputWeights == NULL ) {
    goto err_output_weights;
  }

  for( i = 0; i < tmp->numOutputs; i++ ) {
    tmp->outputWeights[i] = malloc(sizeof(float) * (tmp->numOutputConnections + 1));
    if( tmp->outputWeights[i] == NULL ) {
      last = i;
      goto err_output_weights_arr;
    }
  }

  tmp->outputActivations = malloc(sizeof(activation_type_t) * tmp->numOutputs);
  if( tmp->outputActivations == NULL ) {
    goto err_output_activations;
  }

  tmp->outputVals = malloc(sizeof(float) * tmp->numOutputs);
  if( tmp->outputVals == NULL ) {
    goto err_output_vals;
  }


  // Finally done
  return tmp;


  // Error handling
 err_output_vals:
  free(tmp->outputActivations);

 err_output_activations:
  last = tmp->numOutputs;

 err_output_weights_arr:
  for( i = 0; i < last; i++ ) {
    free(tmp->outputWeights[i]);
  }
  free(tmp->outputWeights);

 err_output_weights:
  last = tmp->numOutputs;

 err_output_connections_arr:
  for( i = 0; i < last; i++ ) {
    free(tmp->outputConnections[i]);
  }
  free(tmp->outputConnections);

 err_output_connections:
  free(tmp->outputSeeds);

 err_output_seeds:
  free(tmp->hiddenVals);

 err_hidden_vals:
  free(tmp->hiddenActivations);

 err_hidden_activations:
  last = tmp->numHidden;

 err_hidden_weights_arr:
  for( i = 0; i < last; i++ ) {
    free(tmp->hiddenWeights[i]);
  }
  free(tmp->hiddenWeights);

 err_hidden_weights:
  last = tmp->numHidden;

 err_hidden_connections_arr:
  for( i = 0; i < last; i++ ) {
    free(tmp->hiddenConnections[i]);
  }
  free(tmp->hiddenConnections);

 err_hidden_connections:
  free(tmp->hiddenSeeds);

 err_hidden_seeds:
  free(tmp); 

 err_object:
  return NULL;
}

void networkDestroy( network_t *network )
{
  uint64_t i;
  free(network->outputVals);
  free(network->outputActivations);
  for( i = 0; i < network->numOutputs; i++ ) {
    free(network->outputWeights[i]);
  }
  free(network->outputWeights);
  for( i = 0; i < network->numOutputs; i++ ) {
    free(network->outputConnections[i]);
  }
  free(network->outputConnections);
  free(network->outputSeeds);
  free(network->hiddenVals);
  free(network->hiddenActivations);
  for( i = 0; i < network->numHidden; i++ ) {
    free(network->hiddenWeights[i]);
  }
  free(network->hiddenWeights);
  for( i = 0; i < network->numHidden; i++ ) {
    free(network->hiddenConnections[i]);
  }
  free(network->hiddenConnections);
  free(network->hiddenSeeds);
  free(network); 
}

bool networkRun( network_t *network, float *inputs )
{
  uint64_t i, j;
  // Calc hidden layer
  for( i = 0; i < network->numHidden; i++ ) {
    float tmpVal = network->hiddenWeights[i][network->numHiddenConnections]; // Bias
    for( j = 0; j < network->numHiddenConnections; j++ ) {
      tmpVal += inputs[network->hiddenConnections[i][j]] * network->hiddenWeights[i][j];
    }
    network->hiddenVals[i] = activationToFunction(network->hiddenActivations[i])(tmpVal);
  }

  // Calc output layer
  for( i = 0; i < network->numOutputs; i++ ) {
    float tmpVal = network->outputWeights[i][network->numOutputConnections]; // Bias
    for( j = 0; j < network->numOutputConnections; j++ ) {
      tmpVal += network->hiddenVals[network->outputConnections[i][j]] * network->outputWeights[i][j];
    }
    network->outputVals[i] = activationToFunction(network->outputActivations[i])(tmpVal);
  }

  return false;
}

float networkGetOutputValue( network_t *network, uint64_t idx )
{
  return network->outputVals[idx];
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
  for( h = 0; h < tmp->numHidden; h++ ) {
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

  for( h = 0; h < tmp->numHidden; h++ ) {
    for( w = 0; w < tmp->numHiddenConnections + 1; w++ ) {
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

  for( h = 0; h < tmp->numHidden; h++ ) {
    networkSetHiddenActivation( tmp, h, (activation_type_t)data[i++] );
  }

  // Read output layer
  for( o = 0; o < tmp->numOutputs; o++ ) {
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

  for( o = 0; o < tmp->numOutputs; o++ ) {
    for( w = 0; w < tmp->numOutputConnections + 1; w++ ) {
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

  for( o = 0; o < tmp->numOutputs; o++ ) {
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
  length += sizeof(uint64_t) * network->numHidden;
  // Hidden weights
  length += sizeof(float) * (network->numHidden * (network->numHiddenConnections + 1));
  // Hidden activations
  length += network->numHidden;
  // Output seeds
  length += sizeof(uint64_t) * network->numOutputs;
  // Output weights
  length += sizeof(float) * (network->numOutputs * (network->numOutputConnections + 1));
  // Output activations
  length += network->numOutputs;

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

  bytes[i++] = (network->numHidden >> 56) & 0xff;
  bytes[i++] = (network->numHidden >> 48) & 0xff;
  bytes[i++] = (network->numHidden >> 40) & 0xff;
  bytes[i++] = (network->numHidden >> 32) & 0xff;
  bytes[i++] = (network->numHidden >> 24) & 0xff;
  bytes[i++] = (network->numHidden >> 16) & 0xff;
  bytes[i++] = (network->numHidden >>  8) & 0xff;
  bytes[i++] = (network->numHidden >>  0) & 0xff;

  bytes[i++] = (network->numOutputs >> 56) & 0xff;
  bytes[i++] = (network->numOutputs >> 48) & 0xff;
  bytes[i++] = (network->numOutputs >> 40) & 0xff;
  bytes[i++] = (network->numOutputs >> 32) & 0xff;
  bytes[i++] = (network->numOutputs >> 24) & 0xff;
  bytes[i++] = (network->numOutputs >> 16) & 0xff;
  bytes[i++] = (network->numOutputs >>  8) & 0xff;
  bytes[i++] = (network->numOutputs >>  0) & 0xff;

  // Connections
  bytes[i++] = (network->numHiddenConnections >> 56) & 0xff;
  bytes[i++] = (network->numHiddenConnections >> 48) & 0xff;
  bytes[i++] = (network->numHiddenConnections >> 40) & 0xff;
  bytes[i++] = (network->numHiddenConnections >> 32) & 0xff;
  bytes[i++] = (network->numHiddenConnections >> 24) & 0xff;
  bytes[i++] = (network->numHiddenConnections >> 16) & 0xff;
  bytes[i++] = (network->numHiddenConnections >>  8) & 0xff;
  bytes[i++] = (network->numHiddenConnections >>  0) & 0xff;

  bytes[i++] = (network->numOutputConnections >> 56) & 0xff;
  bytes[i++] = (network->numOutputConnections >> 48) & 0xff;
  bytes[i++] = (network->numOutputConnections >> 40) & 0xff;
  bytes[i++] = (network->numOutputConnections >> 32) & 0xff;
  bytes[i++] = (network->numOutputConnections >> 24) & 0xff;
  bytes[i++] = (network->numOutputConnections >> 16) & 0xff;
  bytes[i++] = (network->numOutputConnections >>  8) & 0xff;
  bytes[i++] = (network->numOutputConnections >>  0) & 0xff;

  // Hidden
  for( h = 0; h < network->numHidden; h++ ) {
    bytes[i++] = (network->hiddenSeeds[h] >> 56) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >> 48) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >> 40) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >> 32) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >> 24) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >> 16) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >>  8) & 0xff;
    bytes[i++] = (network->hiddenSeeds[h] >>  0) & 0xff;
  }

  for( h = 0; h < network->numHidden; h++ ) {
    for( w = 0; w < network->numHiddenConnections + 1; w++ ) {
      uint32_t tmp = *((uint32_t*)(&(network->hiddenWeights[h][w])));
      bytes[i++] = (tmp >> 24) & 0xff;
      bytes[i++] = (tmp >> 16) & 0xff;
      bytes[i++] = (tmp >>  8) & 0xff;
      bytes[i++] = (tmp >>  0) & 0xff;
    }
  }

  for( h = 0; h < network->numHidden; h++ ) {
    bytes[i++] = (uint8_t)network->hiddenActivations[h];
  }

  // Output
  for( o = 0; o < network->numOutputs; o++ ) {
    bytes[i++] = (network->outputSeeds[o] >> 56) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >> 48) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >> 40) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >> 32) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >> 24) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >> 16) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >>  8) & 0xff;
    bytes[i++] = (network->outputSeeds[o] >>  0) & 0xff;
  }

  for( o = 0; o < network->numOutputs; o++ ) {
    for( w = 0; w < network->numOutputConnections + 1; w++ ) {
      uint32_t tmp = *((uint32_t*)(&(network->outputWeights[o][w])));
      bytes[i++] = (tmp >> 24) & 0xff;
      bytes[i++] = (tmp >> 16) & 0xff;
      bytes[i++] = (tmp >>  8) & 0xff;
      bytes[i++] = (tmp >>  0) & 0xff;
    }
  }

  for( o = 0; o < network->numOutputs; o++ ) {
    bytes[i++] = (uint8_t)network->outputActivations[o];
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
  return network->numHidden;
}

uint64_t networkGetNumOutputs( network_t *network )
{
  return network->numOutputs;
}

uint64_t networkGetNumHiddenConnections( network_t *network )
{
  return network->numHiddenConnections;
}

uint64_t networkGetNumOutputConnections( network_t *network )
{
  return network->numOutputConnections;
}

void networkSetHiddenSeed( network_t *network, uint64_t idx, uint64_t seed )
{
  if( seed != network->hiddenSeeds[idx] ) {
    network->hiddenSeeds[idx] = seed;
    createConnections( network->hiddenSeeds[idx],
		       network->numInputs,
		       network->numHiddenConnections,
		       network->hiddenConnections[idx] );
  }
}

uint64_t networkGetHiddenSeed( network_t *network, uint64_t idx )
{
  return network->hiddenSeeds[idx];
}

void networkSetHiddenBias( network_t *network, uint64_t idx, float bias )
{
  network->hiddenWeights[idx][network->numHiddenConnections] = bias;
}

float networkGetHiddenBias( network_t *network, uint64_t idx )
{
  return network->hiddenWeights[idx][network->numHiddenConnections];
}

void networkSetHiddenWeight( network_t *network, uint64_t idx, uint64_t source, float weight )
{
  network->hiddenWeights[idx][source] = weight;
}

float networkGetHiddenWeight( network_t *network, uint64_t idx, uint64_t source )
{
  return network->hiddenWeights[idx][source];
}

void networkSetHiddenActivation( network_t *network, uint64_t idx, activation_type_t activation )
{
  network->hiddenActivations[idx] = activation;
}

activation_type_t networkGetHiddenActivation( network_t *network, uint64_t idx )
{
  return network->hiddenActivations[idx];
}

void networkSetOutputSeed( network_t *network, uint64_t idx, uint64_t seed )
{
  if( seed != network->outputSeeds[idx] ) {
    network->outputSeeds[idx] = seed;
    createConnections( network->outputSeeds[idx],
		       network->numHidden,
		       network->numOutputConnections,
		       network->outputConnections[idx] );
  }
}

uint64_t networkGetOutputSeed( network_t *network, uint64_t idx )
{
  return network->outputSeeds[idx];
}

void networkSetOutputBias( network_t *network, uint64_t idx, float bias )
{
  network->outputWeights[idx][network->numOutputConnections] = bias;
}

float networkGetOutputBias( network_t *network, uint64_t idx )
{
  return network->outputWeights[idx][network->numOutputConnections];
}

void networkSetOutputWeight( network_t *network, uint64_t idx, uint64_t source, float weight )
{
  network->outputWeights[idx][source] = weight;
}

float networkGetOutputWeight( network_t *network, uint64_t idx, uint64_t source )
{
  return network->outputWeights[idx][source];
}

void networkSetOutputActivation( network_t *network, uint64_t idx, activation_type_t activation )
{
  network->outputActivations[idx] = activation;
}

activation_type_t networkGetOutputActivation( network_t *network, uint64_t idx )
{
  return network->outputActivations[idx];
}


