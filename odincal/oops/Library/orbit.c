#include <stdio.h>
#include <math.h>

#include "attlib.h"
#include "odinscan.h"

static char rcsid[] = "$Id";

static double osc[6];
static double sat[6];

/* G * M(earth) in m^3/s^2 (Montenbruck & Gill, Satellite Orbits, 2000) */
static const double Gm =  3.986004415e14;

/*
  Given ECI GPS position and velocity, calculate corresponding
  orbital elements.
*/
double *gps2osc(double gps[])
{
  double h[3], h2, v2;
  double rx, ry, rz, vx, vy, vz;
  double r, a, O, i, w, e, E, M, n, u, v;

  rx = gps[0];
  ry = gps[1];
  rz = gps[2];
  vx = gps[3];
  vy = gps[4];
  vz = gps[5];

  v2 = vx*vx+vy*vy+vz*vz;
  r = sqrt(rx*rx+ry*ry+rz*rz);

  a = Gm*r/(2*Gm - r*v2);
  n = sqrt(Gm/pow(a,3.0));

  h[0] = ry*vz-vy*rz;
  h[1] = vx*rz-rx*vz;
  h[2] = rx*vy-vx*ry;

  O = atan2(h[0],-h[1]);
  i = atan2(sqrt(h[0]*h[0]+h[1]*h[1]),h[2]);

  h2 =  h[0]*h[0]+h[1]*h[1]+h[2]*h[2];
  e = sqrt(1.0-h2/Gm/a);
  E = atan2((rx*vx+ry*vy+rz*vz)/(a*a*n), 1-r/a);
  M = E-e*sin(E);
  if (M < 0.0) M += 2.0*M_PI;

  u = atan2(rz/sin(i), rx*cos(O)+ry*sin(O));
  v = atan2(sqrt(1-e*e)*sin(E), cos(E)-e);
  w = u-v;
  if (w < 0.0) w += 2.0*M_PI;

  osc[0] = a;
  osc[1] = e;
  osc[2] = i;
  osc[3] = O;
  osc[4] = w;
  osc[5] = M;

  return osc;
}

/*
  Given 6 orbital elements, calculate corresponding ECI 
  GPS position and velocity.
*/
double *osc2gps(double orbit[])
{
  int iter;
  double Q[3], P[3];
  double a, e, i, O, w, M, E0, E1, E, n;
  double rx, ry, vx, vy, fac, R, V;

  a = orbit[0];
  e = orbit[1];
  i = orbit[2];
  O = orbit[3];
  w = orbit[4];
  M = orbit[5];
  n = sqrt(Gm/pow(a,3.0));
  fac = sqrt((1.0-e)*(1.0+e));

  iter = 0;
  E0 = 0.0;
  E1 = M;
  if (e > 0.8) E1 = M_PI;
  while (fabs(E1-E0) > 1.0e-10) {
    E0 = E1;
    E1 = E0 - (E0-e*sin(E0)-M)/(1.0-e*cos(E0));
    iter++;
    if (iter > 10) {
      fprintf(stderr, " eccentric anomaly did not converge\n");
      break;
    }
  }
  E = E1;

  P[0] =  cos(w)*cos(O)-sin(w)*cos(i)*sin(O);
  P[1] =  cos(w)*sin(O)+sin(w)*cos(i)*cos(O);
  P[2] =  sin(w)*sin(i);

  Q[0] = -sin(w)*cos(O)-cos(w)*cos(i)*sin(O);
  Q[1] = -sin(w)*sin(O)+cos(w)*cos(i)*cos(O);
  Q[2] =  cos(w)*sin(i);

  R = a*(1.0-e*cos(E));
  V = sqrt(Gm*a)/R;

  rx = a*(cos(E)-e);
  ry = a*fac*sin(E);
  vx = -V*sin(E);
  vy = V*fac*cos(E);

  sat[0] = rx*P[0] + ry*Q[0];
  sat[1] = rx*P[1] + ry*Q[1];
  sat[2] = rx*P[2] + ry*Q[2];
  sat[3] = vx*P[0] + vy*Q[0];
  sat[4] = vx*P[1] + vy*Q[1];
  sat[5] = vx*P[2] + vy*Q[2];

  return sat;
}
