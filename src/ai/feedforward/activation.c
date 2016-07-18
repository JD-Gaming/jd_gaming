#include "activation.h"

#include <math.h>
#include <stdlib.h>

/*******************************************
 *             Local functions             *
 *******************************************/

/*******************************************
 *           Exported functions            *
 *******************************************/
float act_linear( float val )
{
  return val;
}

float act_relu( float val )
{
  return val > 0 ? val : 0.0;
}

float act_step( float val )
{
  return val >= 0 ? 1.0 : 0.0;
}

float act_sigmoid( float val )
{
  return 1.0 / (1.0 + expf(-val));
}

float act_tanh( float val )
{
  return 2.0 / (1.0 + expf(-2 * val)) - 1.0;
}

float act_atan( float val )
{
  return atanf(val);
}

float act_softsign( float val )
{
  return val / (1.0 + fabsf(val));
}

float act_softplus( float val )
{
  return logf(1 + expf(val));
}

float act_gaussian( float val )
{
  return expf(-(val*val));
}

float act_sinc( float val )
{
  return val == 0 ? 1.0 : sinf(val)/val;
}

float act_sin( float val )
{
  return sinf(val);
}

act_func activationToFunction( activation_type_t activation )
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
  case activation_sin:
    return act_sin;

  default:
    return act_linear;
  }
}

activation_type_t randomActivation( uint32_t allowedActivations )
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
