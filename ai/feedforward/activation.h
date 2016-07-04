#ifndef ACTIVATION_H
#define ACTIVATION_H
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
  case activation_sin:
    return act_sin;

  default:
    return act_linear;
  }
}
#endif
