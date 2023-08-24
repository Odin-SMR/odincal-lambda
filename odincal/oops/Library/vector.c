#include <math.h>

#include "vector.h"

double norm(cVector v)
{
  double d;

  d = sqrt(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
  return (d);
}

void normalise(cVector v)
{
  int i;
  double h;

  h = norm(v);
  for (i = 0; i < 3; i++) v[i] /= h;
}

double dot(cVector u, cVector v)
{
  double d;
  
  d = u[0]*v[0]+u[1]*v[1]+u[2]*v[2];
  return d;
}

void cross(cVector a, cVector b, cVector c)
{
  cVector d;

  d[0] = a[1]*b[2]-a[2]*b[1];
  d[1] = b[0]*a[2]-b[2]*a[0];
  d[2] = a[0]*b[1]-a[1]*b[0];
  c[0] = d[0];
  c[1] = d[1];
  c[2] = d[2];
}

double distance(cVector u, cVector v)
{
  double d;

  d = acos(dot(u,v)/norm(u)/norm(v));
  return (d);
}
