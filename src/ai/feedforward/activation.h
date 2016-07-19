#ifndef ACTIVATION_H
#define ACTIVATION_H

#include <stdint.h>

#include "network.h"

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
