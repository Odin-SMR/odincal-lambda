#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "utility.h"

#ifndef TINY
#define TINY 1.0e-20
#endif

static char rcsid[] = "$Id$";

/*
  Function 'planck' returns the brightness temperature of a
  black body at frequency f (Hz) and temperature T (K)
*/
double planck(double T, double f)
{
  const double h = 6.626176e-34;     /* Planck constant (Js)     */
  const double k = 1.380662e-23;     /* Boltzmann constant (J/K) */
  double Tb, T0;

  T0 = h*f/k;
  if (T > 0.0) Tb = T0/(exp(T0/T)-1.0);
  else         Tb = 0.0;

  return Tb;
}
