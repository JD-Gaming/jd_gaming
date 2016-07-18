#ifndef ACTIVATION_H
#define ACTIVATION_H

#include <stdint.h>

typedef enum activation_type_e {
  activation_linear    = 1 <<  0, // y = x
  activation_relu      = 1 <<  1, // y = x > 0 ? x : 0
  activation_step      = 1 <<  2, // y = x >= 0 ? 1 : 0
  activation_sigmoid   = 1 <<  3, // y = 1 / (1 + exp(-x))
  activation_tanh      = 1 <<  4, // y = 2 / (1 + exp(-2x)) - 1
  activation_atan      = 1 <<  5, // y = atan(x)
  activation_softsign  = 1 <<  6, // y = x / (1 + abs(x))
  activation_softplus  = 1 <<  7, // y = ln(1 + exp(x))
  activation_gaussian  = 1 <<  8, // y = exp(-(x*x))
  activation_sinc      = 1 <<  9, // y = x == 0 ? 1 : sin(x)/x
  activation_sin       = 1 << 10, // y = sin(x)

  // Add any new functions above this value.
  activation_max_bit   = 1 << 11,
  activation_max_shift = 11,
  activation_any       = 0x000
} activation_type_t;

typedef float (*act_func) ( float val );

float act_linear( float val );
float act_relu( float val );
float act_step( float val );
float act_sigmoid( float val );
float act_tanh( float val );
float act_atan( float val );
float act_softsign( float val );
float act_softplus( float val );
float act_gaussian( float val );
float act_sinc( float val );
float act_sin( float val );
act_func activationToFunction( activation_type_t activation );
activation_type_t randomActivation( uint32_t allowedActivations );

#endif
