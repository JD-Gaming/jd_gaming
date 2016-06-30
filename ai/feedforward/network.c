#include "network.h"

#include <stdio.h>
#include <stdint.h>
#include <math.h>

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

network_t *networkCreate( uint64_t inputs, uint64_t hidden, uint64_t outputs )
{
  uint64_t i, last;

  network_t *tmp = malloc(sizeof(network_t));
  if( tmp == NULL ) {
    goto err_object;
  }

  tmp->numInputs = inputs;
  tmp->numHidden = hidden;
  tmp->numOutputs = outputs;

  // Set up memory for hidden layer
  tmp->hiddenWeights = malloc(sizeof(float*) * tmp->numHidden);
  if( tmp->hiddenWeights == NULL ) {
    goto err_hidden_weights;
  }

  for( i = 0; i < tmp->numHidden; i++ ) {
    tmp->hiddenWeights[i] = malloc(sizeof(float) * (tmp->numInputs + 1));
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
  tmp->outputWeights = malloc(sizeof(float*) * tmp->numOutputs);
  if( tmp->outputWeights == NULL ) {
    goto err_output_weights;
  }

  for( i = 0; i < tmp->numOutputs; i++ ) {
    tmp->outputWeights[i] = malloc(sizeof(float) * (tmp->numHidden + 1));
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
  free(tmp); 

 err_object:
  return NULL;
}

void networkDestroy( network_t *network )
{
  uint64_t i;
  free(network->outputActivations);
  for( i = 0; i < network->numOutputs; i++ ) {
    free(network->outputWeights[i]);
  }
  free(network->outputWeights);
  free(network->hiddenVals);
  free(network->hiddenActivations);
  for( i = 0; i < network->numHidden; i++ ) {
    free(network->hiddenWeights[i]);
  }
  free(network->hiddenWeights);
  free(network); 
}

bool networkRun( network_t *network, float *inputs )
{
  uint64_t i, j;
  // Calc hidden layer
  for( i = 0; i < network->numHidden; i++ ) {
    float tmpVal = network->hiddenWeights[i][network->numInputs]; // Bias
    for( j = 0; j < network->numInputs; j++ ) {
      tmpVal += inputs[j] * network->hiddenWeights[i][j];
    }
    network->hiddenVals[i] = activationToFunction(network->hiddenActivations[i])(tmpVal);
  }

  // Calc output layer
  for( i = 0; i < network->numOutputs; i++ ) {
    float tmpVal = network->outputWeights[i][network->numHidden]; // Bias
    for( j = 0; j < network->numHidden; j++ ) {
      tmpVal += network->hiddenVals[j] * network->outputWeights[i][j];
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
  return NULL;
}

bool networkSaveFile( network_t *network, char *filename )
{
  return false;
}

network_t *networkUnserialise( uint64_t len, uint8_t *data )
{
  if( len < 4 * sizeof(uint64_t) ) {
    return NULL;
  }

  network_t *tmp;
  uint64_t i, h, o, w;

  uint64_t numInputs, numHiddenLayers, numHidden, numOutputs;

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

  printf( "Num inputs:  %llu\n", (unsigned long long)numInputs );
  printf( "Num hidden:  %llu\n", (unsigned long long)numHidden );
  printf( "Num outputs: %llu\n", (unsigned long long)numOutputs );

  tmp = networkCreate( numInputs, numHidden, numOutputs );
  if( tmp == NULL ) {
    return NULL;
  }

  for( h = 0; h < tmp->numHidden; h++ ) {
    for( w = 0; w < tmp->numInputs + 1; w++ ) {
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

  for( o = 0; o < tmp->numOutputs; o++ ) {
    for( w = 0; w < tmp->numHidden + 1; w++ ) {
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
  // Hidden weights
  length += sizeof(float) * (network->numHidden * (network->numInputs + 1));
  // Hidden activations
  length += network->numHidden;
  // Output weights
  length += sizeof(float) * (network->numOutputs * (network->numHidden + 1));
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

  // Hidden
  for( h = 0; h < network->numHidden; h++ ) {
    for( w = 0; w < network->numInputs + 1; w++ ) {
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
    for( w = 0; w < network->numHidden + 1; w++ ) {
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

  printf( "Serialisation done, i = %llu, length = %llu\n",
	  (unsigned long long)i, (unsigned long long) length);

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

void networkSetHiddenBias( network_t *network, uint64_t idx, float bias )
{
  network->hiddenWeights[idx][network->numInputs] = bias;
}

float networkGetHiddenBias( network_t *network, uint64_t idx )
{
  return network->hiddenWeights[idx][network->numInputs];
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

void networkSetOutputBias( network_t *network, uint64_t idx, float bias )
{
  network->outputWeights[idx][network->numHidden] = bias;
}

float networkGetOutputBias( network_t *network, uint64_t idx )
{
  return network->outputWeights[idx][network->numHidden];
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


