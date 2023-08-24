#include <stdio.h>
#include <math.h>

#include "odinscan.h"
#include "astrolib.h"
#include "attlib.h"

static char rcsid[] = "$Id";

/*
  Calculate Doppler correction based on target RA, Dec and current
  satellite position and velocity.
  - members Vhel and Vlsr of the scan structure are filled in

  input:
    scan    - pointer to OdinScan structure

  output:
    vsat    - rectangular components of velocity vector for Odin,
              including Sun's LSR velocity, Earth's orbital motion
              and satellite's orbital motion around the Earth.

  Side effects:
  - the LST (local sidereal time) member is calculated and filled in.
*/
void Doppler(struct OdinScan *scan, double ra, double dec, double vsat[]) {
  /*  'ssm' are the rectangular components of a unit vector
      in the direction of the standard solar motion of 1900.0
      taken from Meth.of Exp.Physics, vol.12, part C, page 281
      ( vs = 20.0 km/s  RAAPX = 270.0 deg  DCAPX = 30.0 deg )    */
  static cVector ssm = { 0.0, -0.8660254, 0.5 };
  static cVector vsun, vlun, vtel, tel;
  pVector tp;
  static double vs = 20.0e3;
  static rotMatrix PreMatrix;          /* precession matrix              */
  static double OrbElems[9];           /* orbital elements of sun & moon */
  double along, phi, radius, height, vgeo, vhel, vlsr;
  double JD, Gmst, utc, lst, ip;
  int i;

  height = geodetic(scan->GPSpos, &along, &phi);

  JD = mjd2jd(scan->MJD);

  /* Our GPS coordinates are in an inertial frame, 
     so LST is just equal to the longitude calculated above. */
  lst = along;
  scan->LST = lst*86400.0/(2.0*PI);

  /* Transform longitude into an earth fixed system. */
  utc = modf(scan->MJD, &ip);
  Gmst = GMST(JD-utc); 
  along /= 2.0*PI;   /* convert from radians to revolutions */
  along -= modf(Gmst + ROTATE*utc, &ip);

  radius = sqrt(scan->GPSpos[0]*scan->GPSpos[0]
	       +scan->GPSpos[1]*scan->GPSpos[1]
	       +scan->GPSpos[2]*scan->GPSpos[2]);

  /* we need to precess the standard solar motion from 1900.0 to today */
  pre(B1900, JD, PreMatrix);
  rotate(PreMatrix, ssm, vsun);
  
  /* start coordinate transformation ra, al -> RA, Dec of date         */
  tp.l = ra;        /* true RA of date */
  tp.b = dec;       /* true Dec of date */
  /* printf("%12.4f %12.4f\n", ra*180.0/M_PI, dec*180.0/M_PI); */
  cuv(&tp, tel);    /* convert into cartesian unit vector */

  /* pre(J2000, JD, PreMatrix);   */
  /* rotate(PreMatrix, tel, tel); */

  /* we'll need orbital elements of sun & moon for vearth & vmoon      */
  elements(JD, OrbElems);

  /* the motion of the earth (only annual rotation)                    */
  vorbit(OrbElems, vtel);

  /* the motion of the earth around the earth-moon barycenter          */
  vmoon(OrbElems, vlun);

  for (i = 0; i < 3; i++) {
    vlun[i] /= -81.3;     /* 81.30 is the mass ratio earth/moon        */
    vtel[i] += scan->GPSvel[i];   /* add GPS component                 */
    vtel[i] += vlun[i];   /* add lunar velocity component              */
    vsun[i] *= vs;        /* scale standard solar motion vector        */
    /* printf("%10.4f %12.3f %12.3f %12.3f %12.3f\n", */
    /*   tel[i], vlun[i], vtel[i], vsun[i], scan->GPSvel[i]); */
    vsat[i] = vsun[i]+vtel[i];
  }
 
  vhel =      -vtel[0]*tel[0]-vtel[1]*tel[1]-vtel[2]*tel[2];
  vlsr =  vhel-vsun[0]*tel[0]-vsun[1]*tel[1]-vsun[2]*tel[2];
  vgeo = -scan->GPSvel[0]*tel[0]-scan->GPSvel[1]*tel[1]-scan->GPSvel[2]*tel[2];
  /* printf("vhel = %10.1f vlsr = %10.1f vgeo = %10.1f\n", vhel, vlsr, vgeo); */
  /* printf("%12.6f %12.3f %12.3f %12.3f\n", scan->Orbit, vhel, vlsr, vgeo);  */

  scan->Vgeo = vgeo;
  scan->Vlsr = vlsr;
}
