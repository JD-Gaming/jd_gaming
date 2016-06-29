#include "network.h"

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

network_t *networkCreate( size_t inputs, size_t hidden, size_t outputs )
{
  size_t i, last;

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
  size_t i;
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
  size_t i, j;
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

float networkGetOutputValue( network_t *network, size_t idx )
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

network_t *networkUnserialise( size_t len, unsigned char *data )
{
  return NULL;
}

size_t networkSerialise( network_t *network, unsigned char **data )
{
  return 0;
}

size_t networkGetNumInputs( network_t *network )
{
  return network->numInputs;
}

size_t networkGetNumHidden( network_t *network )
{
  return network->numHidden;
}

size_t networkGetNumOutputs( network_t *network )
{
  return network->numOutputs;
}

void networkSetHiddenBias( network_t *network, size_t idx, float bias )
{
  network->hiddenWeights[idx][network->numInputs] = bias;
}

float networkGetHiddenBias( network_t *network, size_t idx )
{
  return network->hiddenWeights[idx][network->numInputs];
}

void networkSetHiddenWeight( network_t *network, size_t idx, size_t source, float weight )
{
  network->hiddenWeights[idx][source] = weight;
}

float networkGetHiddenWeight( network_t *network, size_t idx, size_t source )
{
  return network->hiddenWeights[idx][source];
}

void networkSetHiddenActivation( network_t *network, size_t idx, activation_type_t activation )
{
  network->hiddenActivations[idx] = activation;
}

activation_type_t networkGetHiddenActivation( network_t *network, size_t idx )
{
  return network->hiddenActivations[idx];
}

void networkSetOutputBias( network_t *network, size_t idx, float bias )
{
  network->outputWeights[idx][network->numHidden] = bias;
}

float networkGetOutputBias( network_t *network, size_t idx )
{
  return network->outputWeights[idx][network->numHidden];
}

void networkSetOutputWeight( network_t *network, size_t idx, size_t source, float weight )
{
  network->outputWeights[idx][source] = weight;
}

float networkGetOutputWeight( network_t *network, size_t idx, size_t source )
{
  return network->outputWeights[idx][source];
}

void networkSetOutputActivation( network_t *network, size_t idx, activation_type_t activation )
{
  network->outputActivations[idx] = activation;
}

activation_type_t networkGetOutputActivation( network_t *network, size_t idx )
{
  return network->outputActivations[idx];
}


