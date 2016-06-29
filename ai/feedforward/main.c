#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>
#include <sys/time.h>

#include "neurons.h"
#include "network.h"

float sigmoid( float val )
{
  return 1.0 / (1.0 + expf(-val));
}

float step( float val )
{
  return val >= 0 ? 1.0 : 0.0;
}

float relu( float val )
{
  return val > 0 ? val : 0.0;
}

int main( void )
{
  const size_t numInputs = 2;
  const size_t numHidden = 2;
  const size_t numOutputs = 1;

  size_t i, j;

#if 0
  neuron_t *hidden[numHidden];
  neuron_t *output[numOutputs];
  
  for( i = 0; i < numHidden; i++ ) {
    hidden[i] = createSigmoid( numInputs, NULL, NULL );
  }

  for( i = 0; i < numOutputs; i++ ) {
    output[i] = createSigmoid( numHidden, NULL, NULL );
  }
#else
  // Get some better randomness going
  srand((unsigned)(time(NULL)));

  // An array of hidden neurons, each of which has <numInputs> weights and a bias
  float hiddenWeights[numHidden][numInputs+1];
  float hiddenVal[numHidden];
  for( i = 0; i < numHidden; i++ ) {
    for( j = 0; j < numInputs+1; j++ ) {
      hiddenWeights[i][j] = 2 * (rand() / (float)RAND_MAX - 0.5) / numInputs;
    }
  }
  bzero( hiddenVal, sizeof(float) * numHidden );

  // An array of output neurons, each of which has <numHidden> weights and a bias
  float outputWeights[numOutputs][numHidden+1];
  float outputVal[numOutputs];
  for( i = 0; i < numOutputs; i++ ) {
    for( j = 0; j < numHidden+1; j++ ) {
      outputWeights[i][j] = 2 * (rand() / (float)RAND_MAX - 0.5) / numHidden;
    }
  }
  bzero( outputVal, sizeof(float) * numOutputs );

  /*
  // Load a pre-configured network
  // A NAND network, provided the output uses step function and the hidden use sigmoid
  hiddenWeights[0][0] = -2.985651;
  hiddenWeights[0][1] = -4.935944;
  hiddenWeights[0][2] =  5.373516; // Bias

  hiddenWeights[1][0] =  3.971738;
  hiddenWeights[1][1] =  1.763571;
  hiddenWeights[1][2] = -3.553529; // Bias
 
  outputWeights[0][0] =  -0.717844;
  outputWeights[0][1] = -10.556825;
  outputWeights[0][2] =   7.640961; // Bias
  */

  size_t inp;
  float inputs[4][2];
  inputs[0][0] = 0;  inputs[0][1] = 0;
  inputs[1][0] = 0;  inputs[1][1] = 1;
  inputs[2][0] = 1;  inputs[2][1] = 0;
  inputs[3][0] = 1;  inputs[3][1] = 1;

  for( inp = 0; inp < 4; inp++ ) {
    // Set input
    float *inputVal = inputs[inp];

    // Calculate first hidden layer
    for( i = 0; i < numHidden; i++ ) {
      float tmpVal = hiddenWeights[i][numInputs]; // Bias
      for( j = 0; j < numInputs; j++ ) {
	tmpVal += inputVal[j] * hiddenWeights[i][j];
      }
      hiddenVal[i] = sigmoid(tmpVal);
    }

    // Add more hidden layers here if necessary

    // Calculate output layer
    for( i = 0; i < numOutputs; i++ ) {
      float tmpVal = outputWeights[i][numHidden]; // Bias
      for( j = 0; j < numHidden; j++ ) {
	tmpVal += hiddenVal[j] * outputWeights[i][j];
      }
      printf( "Setting output %d\n", (int)i );
      outputVal[i] = tmpVal;
    }

    printf( "Input: {" );
    for( i = 0; i < numInputs; i++ ) {
      if( i+1 < numInputs )
	printf( "%f, ", inputVal[i] );
      else
	printf( "%f", inputVal[i] );
    }
    printf( "} -> {" );
    for( i = 0; i < numOutputs; i++ ) {
      if( i+1 < numOutputs )
	printf( "%f, ", outputVal[i] );
      else
	printf( "%f", outputVal[i] );
    }
    printf( "}\n" );
  }
#endif

  return 0;
}
