#include <math.h>
#ifndef PI
#define PI 3.141592653589796        /* guess what?                */
#endif

#define EPS 1.0e-6

static char rcsid[] = "$Id";

#include "attlib.h"


/* 
   All routines use an equatorial radius a and flattening f as
   specified in SSC document SSAK31-7.
*/
static const double a = 6378140.0;
static const double f = 1.0/298.257;

/* 
   The routine calculates geodetic longitude and latitude (radians) 
   and height (m) from geocentric equatorial rectangular coordinates 
   supplied in X (3 components).

   Longitude and latitude are returned through argument 2 and 3, the
   height is returned as the function value.

   Algorithm taken from Astronomical Ephemeris.
*/
double geodetic(double X[], double *lon, double *lat)
{
  double h, b, r, e2, old, C;

  *lon = atan2(X[1],X[0]);
  if (*lon < 0.0) *lon += 2.0*PI;
  r = sqrt(X[0]*X[0] + X[1]*X[1]);
  e2 = 2*f-f*f;
  b = atan(X[2]/r);
  old = 0.0;
  while (fabs(b-old) > EPS) {
    old = b;
    C = 1.0/sqrt(1-e2*pow(sin(b),2.0));
    b = atan((X[2]+a*C*e2*sin(b))/r);
  }
  *lat = b;
  h = r/cos(b) - a*C;
  return h;
}

/* 
   The routine calculates geocentric equatorial rectangular
   coordinates (m) from supplied geodetic longitude and latitude
   (radians).

   Algorithm taken from Astronomical Ephemeris.
*/
void geocentric(double lon, double lat, double h, double *X)
{
  double C, S;

  C = sqrt(cos(lat)*cos(lat)+(1.0-f*f)*sin(lat)*sin(lat));
  S = (1.0-f*f)*C;
  X[0] = (a*C+h)*cos(lat)*cos(lon);
  X[1] = (a*C+h)*cos(lat)*sin(lon);
  X[2] = (a*S+h)*sin(lat);
}

/* 
   Calculation of tangent point location from satellite position
   and line of sight vector.

   Algorithm based on Craig Haley's MatLab code, which in turn is 
   based on Nick Lloyd's ONYX software.
*/
void att2tp(double X[], double los[], double *lon, double *lat, double *h)
{
  int i;
  double c, a2, c2;
  double xunit[3], yunit[3], zunit[3], ijk[3];
  double w11, w12, w21, w22, w31, w32, xr, yr, A, B, C, x, y, K;
  double factor, dist1, dist2;

  c = a*(1.0-f);
  a2 = a*a;
  c2 = c*c;

  for (i = 0; i < 3; i++) xunit[i] = los[i];
  normalise(xunit);

  cross(los, X, zunit);
  normalise(zunit);

  cross(zunit, xunit, yunit);
  normalise(yunit);

  w11 = xunit[0];
  w12 = yunit[0];
  w21 = xunit[1];
  w22 = yunit[1];
  w31 = xunit[2];
  w32 = yunit[2];
  yr = dot(X, yunit);
  xr = dot(X, xunit);
    
  A = (w11*w11 + w21*w21)/a2 + w31*w31/c2;
  B = 2.0*((w11*w12 + w21*w22)/a2 + (w31*w32)/c2);
  C = (w12*w12 + w22*w22)/a2 + w32*w32/c2;

  if (B == 0.0) x = 0.0;
  else {
    K = -2.0*A/B;
    factor = 1.0/(A+(B+C*K)*K);
    x = sqrt(factor);
    y = K*x;
  }

  dist1 = (xr-x)*(xr-x) + (yr-y)*(yr-y);
  dist2 = (xr+x)*(xr+x) + (yr+y)*(yr+y);

  if (dist1 > dist2) x = -x;

  ijk[0] = w11*x + w12*yr;
  ijk[1] = w21*x + w22*yr;
  ijk[2] = w31*x + w32*yr;

  *h = geodetic(ijk, lon, lat);
}
