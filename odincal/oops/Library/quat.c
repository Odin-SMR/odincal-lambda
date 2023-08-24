#include <math.h>
#include "attlib.h"

static char rcsid[] = "$Id";

/* Normalise a quaternion */
void quatnorm(Quat q)
{
    double norm;

    norm = sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);
    if (norm > 0.0) {
        q[0] /= norm;
        q[1] /= norm;
        q[2] /= norm;
        q[3] /= norm;
    }
}

/* 
   Convert quaternions to Euler angles.
   On output e[0] = RA2000, -e[1] = Dec2000, e[2] = LOSangle.
*/
void quat2eul(Quat q, Triple e)
{
    double q0sq, q1sq, q2sq, q3sq;

    quatnorm(q);
    q0sq = q[0]*q[0];
    q1sq = q[1]*q[1];
    q2sq = q[2]*q[2];
    q3sq = q[3]*q[3];

    e[0] = atan2(2*(q[1]*q[2]+q[3]*q[0]),(q0sq+q1sq-q2sq-q3sq));
    e[1] = asin(-2*(q[1]*q[3]-q[2]*q[0])/(q0sq+q1sq+q2sq+q3sq));
    e[2] = atan2(2*(q[2]*q[3]+q[1]*q[0]),(q0sq-q1sq-q2sq+q3sq));
}

/*
  Conversion from Euler angles to quaternions.
*/
void eul2quat(Triple e, Quat q)
{
    double tx = e[2]/2.0;
    double ty = e[1]/2.0;
    double tz = e[0]/2.0;

    q[0] =  sin(tx)*sin(ty)*sin(tz)+cos(tx)*cos(ty)*cos(tz);
    q[1] = -cos(tx)*sin(ty)*sin(tz)+sin(tx)*cos(ty)*cos(tz);
    q[2] =  cos(tx)*sin(ty)*cos(tz)+sin(tx)*cos(ty)*sin(tz);
    q[3] = -sin(tx)*sin(ty)*cos(tz)+cos(tx)*cos(ty)*sin(tz);
    quatnorm(q);
}

/*
  Calculate rotation matrix corresponding to 3 Euler angles.
  1) rotation by e[0] around z-axis
  2) rotation by e[1] around y-axis
  3) rotation by e[2] around x-axis

  If called with e = (RA, -Dec, LOSangle) the routine returns the 
  inverse (= transpose) of the matrix which transforms the unit vector 
  (1,0,0) (= telescope axis) into the corresponding unit vector in 
  equatorial coordinates.
*/
void eul2mat(Triple e, rotMatrix m)
{
    double sine0 = sin(e[0]);
    double sine1 = sin(e[1]);
    double sine2 = sin(e[2]);
    double cose0 = cos(e[0]);
    double cose1 = cos(e[1]);
    double cose2 = cos(e[2]);

    m[0][0] =  cose0*cose1;
    m[0][1] =  sine0*cose1;
    m[0][2] = -sine1;
    m[1][0] = -cose2*sine0+sine2*cose0*sine1;
    m[1][1] =  cose2*cose0+sine2*sine0*sine1;
    m[1][2] =  sine2*cose1;
    m[2][0] =  sine2*sine0+cose2*cose0*sine1;
    m[2][1] = -sine2*cose0+cose2*sine0*sine1;
    m[2][2] =  cose2*cose1;
}

/*
  Calculate rotation matrix corresponding to quaternions.
*/
void quat2mat(Quat q, rotMatrix m)
{
    double q0sq, q1sq, q2sq, q3sq, norm;

    q0sq = q[0]*q[0];
    q1sq = q[1]*q[1];
    q2sq = q[2]*q[2];
    q3sq = q[3]*q[3];
    norm = sqrt(q[0]*q[0]+q[1]*q[1]+q[2]*q[2]+q[3]*q[3]);

    m[0][0] = (q0sq+q1sq-q2sq-q3sq)/norm;
    m[0][1] = (2*q[1]*q[2]+2*q[0]*q[3])/norm;
    m[0][2] = (2*q[1]*q[3]-2*q[0]*q[2])/norm;
    m[1][0] = (2*q[1]*q[2]-2*q[0]*q[3])/norm;
    m[1][1] = (q0sq-q1sq+q2sq-q3sq)/norm;
    m[1][2] = (2*q[2]*q[3]+2*q[0]*q[1])/norm;
    m[2][0] = (2*q[1]*q[3]+2*q[0]*q[2])/norm;
    m[2][1] = (2*q[2]*q[3]-2*q[0]*q[1])/norm;
    m[2][2] = (q0sq-q1sq-q2sq+q3sq)/norm;
}

/*
  Calculate Euler angles corresponding to given rotation matrix.
*/
void mat2eul(rotMatrix m, Triple e)
{
    e[0] = atan2(m[0][1], m[0][0]);
    e[1] = asin(-m[0][2]);
    e[2] = atan2(m[1][2], m[2][2]);
}

/*
  Quaternion product
*/
void quatprod(Quat q1, Quat q2, Quat r)
{
    Quat qr;
    int i;

    quatnorm(q1);
    quatnorm(q2);
    /* for (i = 0; i < 4; i++) printf("q[%d] = %12.4f\n", i, q1[i]); */
    qr[0] = q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2] - q1[3]*q2[3];
    qr[1] = q1[0]*q2[1] + q1[1]*q2[0] + q1[2]*q2[3] - q1[3]*q2[2];
    qr[2] = q1[0]*q2[2] + q1[2]*q2[0] + q1[3]*q2[1] - q1[1]*q2[3];
    qr[3] = q1[0]*q2[3] + q1[3]*q2[0] + q1[1]*q2[2] - q1[2]*q2[1];
    quatnorm(qr);

    for (i = 0; i < 4; i++) r[i] = qr[i];
    /* for (i = 0; i < 4; i++) printf("r[%d] = %12.4f\n", i, r[i]); */
}

/* 
   Quaternion rotation
*/
void qrotate(cVector v, Quat q, cVector w)
{
    Quat qr, qinv;

    qr[0] =  0.0;
    qr[1] = v[0];
    qr[2] = v[1];
    qr[3] = v[2];

    qinv[0] =  q[0];
    qinv[1] = -q[1];
    qinv[2] = -q[2];
    qinv[3] = -q[3];

    quatprod(q, qr, qr);
    quatprod(qr, qinv, qr);

    w[0] = qr[1];
    w[1] = qr[2];
    w[2] = qr[3];
}
