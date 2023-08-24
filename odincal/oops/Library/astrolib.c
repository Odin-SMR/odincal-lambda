#include <stdio.h>
#include <math.h>
#include "vector.h"
#include "astrolib.h"
/* #include "constant.h" */

static char rcsid[] = "$Id$";

#ifdef OSK
/* 
 * For systems who don't have a atan2 function (like OS-9)
 */
double atan2(double y, double x)
{
  if (x == 0.0) {
    if (y < 0.0) return(-PI/2.0);
    else         return( PI/2.0);
  } else {
    if (x < 0.0) {
      if (y < 0.0) return(atan(y/x)-PI);
      else         return(atan(y/x)+PI);
    } else return(atan(y/x));
  }
}
#endif

/*
 * convert polar vector to cartesian vector
 */
void cuv(pVector *p, cVector v)
{
  v[0] = cos(p->l)*cos(p->b);
  v[1] = sin(p->l)*cos(p->b);
  v[2] = sin(p->b);
}
 
/*
 * convert cartesian vector to polar vector
 */
void uvc(cVector v, pVector *p)
{
  if (v[0] == 0.0 && v[1] == 0.0) {
    p->l = 0.0;
    if (v[2] > 0.0) p->b =  PI/2.0;
    else            p->b = -PI/2.0;
  } else {
    p->l = atan2(v[1], v[0]);
    if (p->l < 0.0)  p->l += 2.0*PI;
    p->b = atan(v[2]/sqrt(v[0]*v[0]+v[1]*v[1]));
  }
}
 
/*
 * rotate cartesian vector by way of rotation matrix
 */
void rotate(rotMatrix m, cVector vo, cVector vn)
{
  cVector v;

  v[0] = m[0][0]*vo[0]+m[0][1]*vo[1]+m[0][2]*vo[2];
  v[1] = m[1][0]*vo[0]+m[1][1]*vo[1]+m[1][2]*vo[2];
  v[2] = m[2][0]*vo[0]+m[2][1]*vo[1]+m[2][2]*vo[2];
  vn[0] = v[0];
  vn[1] = v[1];
  vn[2] = v[2];
}
 
/*
 * rotate polar vector by way of rotation matrix
 */
void trc(rotMatrix m, pVector *old, pVector *new)
{
  static double vo[3], vn[3];
 
  cuv(old, vo);
  rotate(m, vo, vn);
  uvc(vn, new);
}
 
/*
 * calculate Julian Day from year, month, day
 */
double djl(int Year, int Month, int Day)
{
  long DayNumber;
 
  DayNumber = 367L*Year - 7*(Year+(Month+9)/12)/4
    - 3*((Year+(Month-9)/7)/100+1)/4 + 275*Month/9
    + Day + 1721029L;
  return ((double)DayNumber-0.5);
}

/*
 * calculate modified Julian Day from Julian Day
 */
double jd2mjd(double JD)
{
  return JD - 2400000.5;
}

/*
 * calculate Julian Day from modified Julian Day
 */
double mjd2jd(double MJD)
{
  return MJD + 2400000.5;
}

/*
 * calculate year, month, day, hour, min, secs from Julian Day
 */
void cld(double JD, int *Year, int *Month, int *Day,
	 int *Hour, int *Min, double *Secs)
{
  double ip, f;
  int z, alpha, a, b, c, d, e;

  f = modf(JD+0.5, &ip);
  z = (int)ip;
  if (z < 2299161) {
    a = z;
  } else {
    alpha = (int)((ip - 1867216.25)/36524.25);
    a = z + 1 + alpha - alpha/4;
  }

  b = a + 1524;
  c = (int)(((double)b-122.1)/365.25);
  d = (int)(365.25*c);
  e = (int)((double)(b-d)/30.6001);

  *Day   = b - d - (int)(30.6001*e);
  *Month = (e < 14 ? e-1 : e-13);
  *Year  = (*Month > 2 ? c - 4716 : c - 4715);

  f = modf(24.0*f, &ip);
  *Hour = (int)ip;

  f = modf(60.0*f, &ip);
  *Min = (int)ip;

  *Secs = 60.0*f;
}

/* 
 * Calculation of the components of the earth due to the 
 * motion of the earth around the earth moon barycenter
 * in rectangular, equatorial coordinates for
 * given elements a[0]...a[7] for the sun and the moon.
 *
 * input:
 *     a:           solar and lunar elements
 *
 * output: 
 *     vlun:        velocity vector 
 */
void vmoon(double a[], cVector vlun)
{
  double si1, si2, si3, ece, ecc;
  double tlm, pml, ain;
  double sinc, cinc, somg, comg, seps, ceps;
  double vluna, vxlun, vylun, vzlun, vxecl, vyecl, vzecl;
 
  /* the eccentricity of the lunar orbit */
  ecc = 0.054900489;
  /* equation of the center */
  si1 = sin(a[5]);
  si2 = sin(2.0*a[5]);
  si3 = sin(3.0*a[5]);
  ece = ecc*(2.0*si1+ecc*(1.25*si2+ecc*(1.083333*si3-0.25*si1)));
  /* the true lunar longitude of the moon */
  tlm = a[3]-a[4]+ece;
  /* the lunar longitude of lunar perigee */
  pml = a[3]-a[4]-a[5];
  /* the lunar velocity perpendicular to the radius vector (m/s) */
  vluna = 1023.165604/sqrt(1.0-ecc*ecc);
  /* components with respect to ascending node and pole of orbit */
  vxlun = -vluna*(sin(tlm)+ecc*sin(pml));
  vylun =  vluna*(cos(tlm)+ecc*cos(pml));
  vzlun =  0.0; 
  /* components with respect to eclipse */
  /* the inclination of the lunar orbit to the eclipse */
  ain = 0.089804108;
  sinc = sin(ain);
  cinc = cos(ain);
  /* longitude of ascending node */
  comg = cos(a[4]);
  somg = sin(a[4]);
 
  vxecl = comg*vxlun -somg*cinc*vylun +somg*sinc*vzlun; 
  vyecl = somg*vxlun +comg*cinc*vylun -comg*sinc*vzlun; 
  vzecl =                  sinc*vylun +     cinc*vzlun; 
 
  /* components with respect to equator */
  seps = sin(a[6]);
  ceps = cos(a[6]);
 
  vlun[0] = vxecl;
  vlun[1] = vyecl*ceps-vzecl*seps; 
  vlun[2] = vyecl*seps+vzecl*ceps; 
}
 
/*
 * Calculation of the components of the earth's velocity along its 
 * orbit plus the components of the observer's velocity due the ro-
 * tation of the earth in rectangular, equatorial coordinates
 * for a given local apparent sidereal time (last) in radians and elements 
 * a[0]...a[7] for the sun and the moon which result from routine 'elements'.
 * 
 * input:
 *     last:        local apparent sidereal time
 *     a:           solar and lunar elements
 *     phi:         geographical lattitude
 *     elev:        geographical elevation
 * output:
 *     vorbit:      orbital velocity of earth
 *     vrotat:      rotational velocity of earth
 *     vtel:        velocity vector of earth;
 *
 * side effects:
 *     the true longitude of the sun is calculated and stored in a[8]
 */
void vearth(double last, double rho, double phi, double a[], cVector vtel)
{
  double si1, si2, si3;
  double ece;
  double vorb, vrho;
  double sinfac, cosfac;
 
  /* equation of the center */
  si1 = sin(a[2]);
  si2 = sin(2.0*a[2]);
  si3 = sin(3.0*a[2]);
  ece = a[7]*(2.0*si1+a[7]*(1.25*si2+a[7]*(1.083333*si3-0.25*si1)));
  /* true longitude of the sun */
  a[8] = a[0]+ece;
  /* components of the earth's velocity */
  vorb = 29784.819/sqrt(1.0-a[7]*a[7]);
 
  /* vrho is the velocity due to diurnal rotation (m/s) */
  vrho = 2.0*PI*rho*ROTATE/24.0/3600.0;
  vrho = vrho*cos(phi);
 
  cosfac = cos(a[8])+a[7]*cos(a[1]);
  sinfac = sin(a[8])+a[7]*sin(a[1]);
  vtel[0] =  vorb*sinfac           - vrho*sin(last);
  vtel[1] = -vorb*cosfac*cos(a[6]) + vrho*cos(last);
  vtel[2] = -vorb*cosfac*sin(a[6]);
}

/*
 * Calculation of the components of the earth's velocity along its 
 * orbit around the Sun in rectangular, equatorial coordinates
 * for given elements a[0]...a[7] for the Sun and the Moon which 
 * result from routine 'elements'.
 * 
 * input:
 *     a:           solar and lunar elements
 * output:
 *     vtel:        velocity vector of earth;
 *
 * side effects:
 *     the true longitude of the sun is calculated and stored in a[8]
 *
 * This is a modified version of vearth from above which neglects the
 * diurnal rotation of the earth.
 */
void vorbit(double a[], cVector vtel)
{
  double si1, si2, si3;
  double ece;
  double vorb;
  double sinfac, cosfac;
 
  /* equation of the center */
  si1 = sin(a[2]);
  si2 = sin(2.0*a[2]);
  si3 = sin(3.0*a[2]);
  ece = a[7]*(2.0*si1+a[7]*(1.25*si2+a[7]*(1.083333*si3-0.25*si1)));
  /* true longitude of the sun */
  a[8] = a[0]+ece;
  /* components of the earth's velocity */
  vorb = 29784.819/sqrt(1.0-a[7]*a[7]);
 
  cosfac = cos(a[8])+a[7]*cos(a[1]);
  sinfac = sin(a[8])+a[7]*sin(a[1]);
  vtel[0] =  vorb*sinfac;
  vtel[1] = -vorb*cosfac*cos(a[6]);
  vtel[2] = -vorb*cosfac*sin(a[6]);
}

void a13(double Js, double a[])
{
  double T, ip;
  int i;
 
  T = (Js-J2000)/36525.0;
  /* geometric mean longitude of the sun */
  a[0] = 280.46645 + (36000.76983 + 0.0003032*T)*T;
  /* mean longitude of perigee */
  a[1] = 180.0+102.937348+(1.7195269+(0.00045962+0.000000499*T)*T)*T;
  /* mean anomaly of the sun */
  a[2] = 357.5291092+(35999.0502909-(0.0001536 + T/24490000.0)*T)*T;

  for (i = 0; i < 3; i++) {
    a[i] = modf(a[i]/360.0, &ip);
    if (a[i] < 0.0) a[i] += 1.0;
    a[i] *= (2.0*PI);
  }
}

void a46(double Js, double a[])
{
  double T, ip;
  int i;
 
  T = (Js-J2000)/36525.0;
  /* mean longitude of the moon */
  a[0] = 218.3164591+(481267.88134236-(0.0013268-T/545868.0+T*T/113065000.0)*T)*T;
  /* longitude of the mean ascending node */
  a[1] = 125.044555-(1934.1361849-(0.0020762+T/467410.0-T*T/60616000.0)*T)*T;
  /* mean anomaly of the moon */
  a[2] = 134.9634114+(477198.8676313+(0.008997+T/69699.0-T*T/14712000.0)*T)*T;
  for (i = 0; i < 3; i++) {
    a[i] = modf(a[i]/360.0, &ip);
    if (a[i] < 0.0) a[i] += 1.0;
    a[i] *= (2.0*PI);
  }
}

/* mean obliqity of the ecliptic */
double eps(double Js)
{
  double T, ip; 
  double eps;

  T = (Js-J2000)/36525.0;
  eps = (84381.448-(46.8150+(0.00059-0.001813*T)*T)*T)/3600.0;
  eps = modf(eps/360.0, &ip);
  if (eps < 0.0) eps += 1.0;
  eps *= (2.0*PI);
  return (eps);
}

/* mean eccentricity */ 
double ecc(double Js) 
{ 
  double T; double ecc;

  T = (Js-J2000)/36525.0;
  ecc = 0.016708617 - (0.000042037 + 0.0000001236*T)*T;
  return (ecc);
} 

/* equation of the center */
double ece(double ecc, double a3)
{
  double si1, si2, si3, ece;

  si1 = sin(a3);
  si2 = sin(2.0*a3);
  si3 = sin(3.0*a3);
  ece = ecc*(2.0*si1+ecc*(1.25*si2+ecc*(1.083333*si3-0.25*si1)));
  return (ece);
}

 /*
 * Calculation of certain elements of the sun's and moon's orbit
 * input:
 *     Js:                 Julian Day of epoch
 * output:
 *     a:           resulting orbital elements
 * 
 * side effects:
 *    element a[8] is reserved for true longitude of the sun
 *    which may be calculated by calling routine VEARTH 
 */
void elements(double Js, double a[])
{
  double T, ip;
  int i;
 
  T = (Js-J2000)/36525.0;
  /* geometric mean longitude of the sun */
  a[0] = 280.46645 + (36000.76983 + 0.0003032*T)*T;
  /* mean longitude of perigee */
  a[1] = 180.0+102.937348+(1.7195269+(0.00045962+0.000000499*T)*T)*T;
  /* mean anomaly of the sun */
  a[2] = 357.5291092+(35999.0502909-(0.0001536 + T/24490000.0)*T)*T;
  /* mean longitude of the moon */
  a[3] = 218.3164591+(481267.88134236-(0.0013268-T/545868.0+T*T/113065000.0)*T)*T;
  /* longitude of the mean ascending node */
  a[4] = 125.044555-(1934.1361849-(0.0020762+T/467410.0-T*T/60616000.0)*T)*T;
  /* mean anomaly of the moon */
  a[5] = 134.9634114+(477198.8676313+(0.008997+T/69699.0-T*T/14712000.0)*T)*T;
  /* mean obliqity of the ecliptic */
  a[6] = (84381.448-(46.8150+(0.00059-0.001813*T)*T)*T)/3600.0;
  /* mean eccentricity */
  a[7] = 0.016708617 - (0.000042037 + 0.0000001236*T)*T;
 
  for (i = 0; i < 7; i++) {
    a[i] = modf(a[i]/360.0, &ip);
    if (a[i] < 0.0) a[i] += 1.0;
    a[i] *= (2.0*PI);
  }
}
 
/*
 * Calculation of nutation matrix
 * input:
 *     Js:                 Julian Day of epoch
 * output:
 *     NutMat:             matrix describing nutation at Js
 */
void nut(double Js, rotMatrix NutMat)
{
  double T, eps, l, lp, w, D, F, ip, arg;
  double deps, dPsi;
 
  T = (Js-J2000)/36525.0;
  /* mean obliquity of the ecliptic */
  eps = 84381.448-46.8150*T-0.00059*T*T+0.001813*T*T*T;
  eps = eps/SECPERREV*(2.0*PI);
  /* mean anomaly of the Moon */
  l = 485866.733+(1325.0*1296000.0+715922.633)*T+31.310*T*T+0.064*T*T*T;
  l = modf(l/SECPERREV, &ip)*(2.0*PI);
  /* mean anomaly of the Sun (Earth) */
  lp = 1287099.804+(99.0*1296000.0+1292581.244)*T-0.577*T*T-0.012*T*T*T;
  lp = modf(lp/SECPERREV, &ip)*(2.0*PI);
  /* difference L-w, where L mean longitude of the Moon */
  F = 335778.877+(1342.0*1296000.0+295263.137)*T-13.257*T*T+0.011*T*T*T;
  F = modf(F/SECPERREV, &ip)*(2.0*PI);
  /* mean elongation of the Moon from the Sun */
  D = 1072261.307+(1236.0*1296000.0+1105601.328)*T-6.891*T*T+0.019*T*T*T;
  D = modf(D/SECPERREV, &ip)*(2.0*PI);
  /* longitude of the ascending node of the Moon */
  w = 450160.280-(5.0*1296000.0+482890.539)*T+7.455*T*T+0.008*T*T*T;
  w = modf(w/SECPERREV, &ip)*(2.0*PI);
 
  arg = w;              dPsi = -17.1996*sin(arg);  deps =   9.2025*cos(arg);
  arg = 2.0*w;          dPsi +=  0.2062*sin(arg);  deps += -0.0895*cos(arg);
  arg = 2.0*(F-D+w);    dPsi += -1.3187*sin(arg);  deps +=  0.5736*cos(arg);
  arg = lp;             dPsi +=  0.1426*sin(arg);  deps +=  0.0054*cos(arg);
  arg = lp+2.0*(F-D+w); dPsi += -0.0517*sin(arg);  deps +=  0.0224*cos(arg);
  arg = 2.0*(F-D+w)-lp; dPsi +=  0.0217*sin(arg);  deps += -0.0095*cos(arg);
  arg = 2.0*(F-D)+w;    dPsi +=  0.0129*sin(arg);  deps += -0.0070*cos(arg);
  arg = 2.0*(F+w);      dPsi += -0.2274*sin(arg);  deps +=  0.0977*cos(arg);
  arg = l;              dPsi +=  0.0712*sin(arg);  deps += -0.0007*cos(arg);
  arg = 2.0*F+w;        dPsi += -0.0386*sin(arg);  deps +=  0.0200*cos(arg);
  arg = l+2.0*(F+w);    dPsi += -0.0301*sin(arg);  deps +=  0.0129*cos(arg);
  arg = l-2.0*D;        dPsi += -0.0158*sin(arg);  deps += -0.0001*cos(arg);
  arg = 2.0*(F+w)-l;    dPsi +=  0.0123*sin(arg);  deps += -0.0053*cos(arg);
 
  dPsi *= (2.0*PI)/SECPERREV;
  deps *= (2.0*PI)/SECPERREV;
 
  NutMat[0][0] =  1.0;
  NutMat[0][1] = -dPsi*cos(eps);
  NutMat[0][2] = -dPsi*sin(eps);
  NutMat[1][0] =  dPsi*cos(eps);
  NutMat[1][1] =  1.0;
  NutMat[1][2] = -deps;
  NutMat[2][0] =  dPsi*sin(eps);
  NutMat[2][1] =  deps;
  NutMat[2][2] =  1.0;
}
 
/*
 * Calculation of conversion matrix from galactic to equatorial coordinates
 * for epoch B1950.0
 *
 * input:
 *     (none)
 * output:
 *     GalMat:             matrix describing transformation gal -> B1950.0
 */
void gal(rotMatrix GalMat)
{
  /* 18:49:00 = RA of the ascending node of the galactic plane */
  static double a = (18.0+49.0/60.0)*PI/12.0;   
  /* 327:00:00 = 360 - gal. long. of the asc. node of the galactic plane */
  static double b = 327.0*PI/180.0;
  /* 62:36:00 = inclination of the galactic plane */
  static double c = (62.0+36.0/60.0)*PI/180.0;   
    
  GalMat[0][0] =  cos(a)*cos(b)-sin(a)*sin(b)*cos(c);
  GalMat[0][1] = -cos(a)*sin(b)-sin(a)*cos(b)*cos(c);
  GalMat[0][2] =  sin(a)*sin(c);
  GalMat[1][0] =  sin(a)*cos(b)+cos(a)*sin(b)*cos(c);
  GalMat[1][1] = -sin(a)*sin(b)+cos(a)*cos(b)*cos(c);
  GalMat[1][2] = -cos(a)*sin(c);
  GalMat[2][0] =  sin(b)*sin(c);
  GalMat[2][1] =  cos(b)*sin(c);
  GalMat[2][2] =  cos(c);
}

/*
 * Calculation of conversion matrix from equatorial coordinates 
 * at given Julian date to ecliptic coordinates.
 *
 * input:
 *     (none)
 * output:
 *     EclMat:  matrix describing transformation equ(Js) -> ecl.
 */
void ecl(double Js, rotMatrix EclMat)
{
  double epsilon;
  epsilon = eps(Js);

  EclMat[0][0] =   1.0;
  EclMat[0][1] =   0.0;
  EclMat[0][2] =   0.0;
  EclMat[1][0] =   0.0;
  EclMat[1][1] =   cos(epsilon);
  EclMat[1][2] =   sin(epsilon);
  EclMat[2][0] =   0.0;
  EclMat[2][1] =  -sin(epsilon);
  EclMat[2][2] =   cos(epsilon);
}

/*
 * Calculation of precession matrix
 * input:
 *     Js:                 Julian Day of starting epoch
 *     Je:                 Julian Day of ending epoch
 * output:
 *     PreMat:             matrix describing precession from Js to Je
 */
void pre(double Js, double Je, rotMatrix PreMat)
{
  double t, T;
  double zeta, z, theta;
 
  T = (Js-J2000)/36525.0;
  t = (Je-Js)/36525.0;
  zeta  = ( (2306.2181+1.39656*T-0.000139*T*T)*t
	    + (0.30188-0.000344*T)*t*t + 0.017998*t*t*t )*(2.0*PI)/SECPERREV;
  z     = ( (2306.2181+1.39656*T-0.000139*T*T)*t
	    + (1.09468+0.000066*T)*t*t + 0.018203*t*t*t )*(2.0*PI)/SECPERREV;
  theta = ( (2004.3109-0.85330*T-0.000217*T*T)*t
	    - (0.42665+0.000217*T)*t*t - 0.041833*t*t*t )*(2.0*PI)/SECPERREV;
  PreMat[0][0] =  cos(zeta)*cos(theta)*cos(z)-sin(zeta)*sin(z);
  PreMat[0][1] = -sin(zeta)*cos(theta)*cos(z)-cos(zeta)*sin(z);
  PreMat[0][2] = -sin(theta)*cos(z);
  PreMat[1][0] =  cos(zeta)*cos(theta)*sin(z)+sin(zeta)*cos(z);
  PreMat[1][1] = -sin(zeta)*cos(theta)*sin(z)+cos(zeta)*cos(z);
  PreMat[1][2] = -sin(theta)*sin(z);
  PreMat[2][0] =  cos(zeta)*sin(theta);
  PreMat[2][1] = -sin(zeta)*sin(theta);
  PreMat[2][2] =  cos(theta);
}
 
/*
 * Calculation of Greenwich mean sidereal time at 0h UT for given Julian Day
 * input:
 *     JD:                 Julian Day
 * returns:
 *     GMST:               Greenwich mean sidereal time (revs)
 */
double GMST(double JD)
{
  double Tu, GHA, ip;
 
  Tu = (JD-J2000)/36525.0;
  GHA = 24110.54841+(8640184.812866+(0.093104-6.2e-6*Tu)*Tu)*Tu;
  GHA /= SECPERDAY;
  GHA = modf(GHA, &ip);
  if (GHA < 0.0) GHA += 1.0;
  return (GHA);
}
 
/*
 * Calculation of current difference UT1 - UTC. 
 * Based on IERS Bulletin - A (07 July 1994)
 * input:
 *     JD:                 Julian Day
 * returns:
 *     DUT:                current UT1 - UTC (seconds)
 */
double DUT(double JD)
{
  double T, dUT, MJD;
 
  /* calculate Besselian year */
  T = 1900.0 + (JD-B1900)/365.2421988;

  /* calculate modified Julian Day */
  MJD = jd2mjd(JD);
  dUT = 0.8091 - 0.00227*(MJD - 49536.0);

  /* correct dUT for UT2 - UT1 */
  dUT -=  0.022*sin(2.0*PI*T) - 0.012*cos(2.0*PI*T)
    - 0.006*sin(4.0*PI*T) + 0.007*cos(4.0*PI*T);
  return (dUT);
}
 
/*
 * Calculation of mean local sidereal time
 * input:
 *     Year, Month, Day:   date (yyyy, mm, dd)
 *     ZTime:              zone time (revolutions)
 *     ObsLong:            longitude (revolutions, east is positive) 
 *     ZtLong:             longitude of time zone (revolutions)
 * output:
 *     JD:                 Julian Day for given time zone
 * returns:
 *                         mean sidereal time for given time zone (revs)
 */
double LMST(int Year, int Month, int Day, 
            double ZTime, double ObsLong, double ZtLong, double *JD)
{
  double GHA, SidTime, ip;
 
  *JD = djl(Year, Month, Day);
  GHA = GMST(*JD);
  SidTime = modf(GHA+ObsLong+1.0027379093*(ZTime+ZtLong), &ip);
  *JD += (ZTime+ZtLong);
  return (SidTime);
}
 
/* Julian epoch for given Julian Day */
double JEpoch(double JD)
{
  return (2000.0+(JD-J2000)/365.25);
}
 
/* Besselian epoch for given Julian Day */
double BEpoch(double JD)
{
  return (1900.0+(JD-B1900)/365.242198781);
}

/* calculation of sun's ecliptic longitude */ 
double sul(double Js, double a[])
{
  double sul;
  
  a13(Js, a);
  sul = a[0] + ece(ecc(Js), a[2]);
  if (sul > 2.0*PI) sul -= 2.0*PI;
  return (sul);
}

/* Calculation of the equation of time */
double eqt(double Js, pVector *sol)
{
  static double aber = 9.924136053e-5;
  double a[3], epsilon, aux, sinaux, ramsun, ip;
  cVector r;

  epsilon = eps(Js);
  aux = sul(Js, a) - aber;
  sinaux = sin(aux);
  r[0] = cos(aux);
  r[1] = sinaux*cos(epsilon);
  r[2] = sinaux*sin(epsilon);
  uvc(r, sol);
  ramsun = a[0] - aber;
  return (modf((ramsun-sol->l)/2.0/PI, &ip));
}

/* 
   Multiply two matrices and return result. 
   It's ok to call this routine with output matrix r equal to one of
   the input matrices.
*/
void mul(rotMatrix m, rotMatrix n, rotMatrix r)
{
  static rotMatrix result;
  int row, col;

  for (row = 0; row < 3; row++) {    
    for (col = 0; col < 3; col++) {    
      result[row][col] = m[row][0]*n[0][col] 
	+ m[row][1]*n[1][col] 
	+ m[row][2]*n[2][col];
    }
  }
  memcpy(r, result, 3*3*sizeof(double));
}

/* Invert (i.e. transpose a rotation matrix. */
void inv(rotMatrix m)
{
  double swap;

  swap = m[1][0];
  m[1][0] = m[0][1];
  m[0][1] = swap;

  swap = m[2][0];
  m[2][0] = m[0][2];
  m[0][2] = swap;

  swap = m[1][2];
  m[1][2] = m[2][1];
  m[2][1] = swap;
}
  
void PrintMatrix(rotMatrix m)
{
  int row;

  printf("\n");
  for (row = 0; row < 3; row++) {    
    printf("%10.4f %10.4f %10.4f\n", m[row][0], m[row][1], m[row][2]);
  }
}

void PrintVector(cVector v)
{
  printf("\n%10.4f\n%10.4f\n%10.4f\n", v[0], v[1], v[2]);
}

void PrintPolar(pVector *p)
{
  printf("\n%10.4f %10.4f\n", p->l, p->b);
}
