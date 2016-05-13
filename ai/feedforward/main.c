#include <stdio.h>

#include "neurons.h"

int main( void )
{
  const uint32_t numInputs = 2;
  const uint32_t numHidden = 2;
  const uint32_t numOutputs = 1;

  neuron_t *hidden[numHidden];
  neuron_t *output[numOutputs];
  
  uint32_t i;

  for( i = 0; i < numHidden; i++ ) {
    hidden[i] = createSigmoid( numInputs, NULL, NULL );
  }

  for( i = 0; i < numOutputs; i++ ) {
    output[i] = createSigmoid( numHidden, NULL, NULL );
  }
  
  return 0;
}
