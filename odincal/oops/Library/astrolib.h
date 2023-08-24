#ifndef ASTRONOMY_H
#define ASTRONOMY_H

/* $Id$ */

#include "vector.h"

/* some useful constants */
#ifndef PI
#define PI 3.141592653589796        /* guess what?                */
#endif
#define LIGHTSPEED 2.997924562e8    /* c in meter per second      */
#define SECPERREV 1296000.0         /* arcsec per full circle     */
#define SECPERDAY 86400.0           /* seconds per day            */
#define ROTATE  1.00273790935       /* solar time/sidereal time   */
#define RPS ROTATE*2.0*PI/SECPERDAY /* rotation per sec. (radian) */
#define J2000 2451545.0             /* standard epoch J2000       */
#define B1900 2415020.31352         /* standard epoch B1900       */

/* function prototypes */

double atan2(double, double);
void cuv(pVector *, cVector);
void uvc(cVector, pVector *);
void rotate(rotMatrix, cVector, cVector);
void trc(rotMatrix, pVector *, pVector *);
double djl(int, int, int);
double jd2mjd(double);
double mjd2jd(double);
void cld(double, int *, int *, int *, int *, int *, double *);
double GMST(double);
void pre(double, double, rotMatrix);
void gal(rotMatrix);
void ecl(double, rotMatrix);
void nut(double, rotMatrix);
void a13(double, double []);
void a46(double, double []);
double eps(double);
double ecc(double);
double ece(double, double);
void elements(double, double []);
void vearth(double, double, double, double [], cVector);
void vorbit(double [], cVector);
void vmoon(double [], cVector);
double DUT(double);
double LMST(int, int, int, double, double, double, double *);
double JEpoch(double);
double BEpoch(double);
double sul(double, double []);
double eqt(double, pVector *);

void mul(rotMatrix , rotMatrix, rotMatrix);
void inv(rotMatrix );
void PrintMatrix(rotMatrix);
void PrintVector(cVector);
void PrintPolar(pVector *);

#endif
