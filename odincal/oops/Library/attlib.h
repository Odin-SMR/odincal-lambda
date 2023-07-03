#ifndef ATTLIB_H
#define ATTLIB_H

#include "vector.h"

/**@name Attitude related routines */
/*@{*/

/* $Id$ */

typedef double Quat[4];
typedef double Triple[3];

/** Get geodetic coordinates.
    Calculate geodetic coordinates from given GPS position.
    Returns height in meters, and longitude and latitude through
    paramters 2 and 3, respectively.

    @param X 3 element GPS position in meters
    @param lon will hold geodetic longitude in radian on return
    @param lat will hold geodetic latitude in radian on return
*/
double geodetic(double X[], double *lon, double *lat);

/** Get geocentric coordinates.
    Calculate GPS position from given geodetic coordinates.

    @param lon geodetic longitude in radian
    @param lat geodetic latitude in radian
    @param h geodetic height in meter
*/
void geocentric(double lon, double lat, double h, double *X);

/** Caclulate tangent point. 
    Calculation of tangent point location from satellite position
    and line of sight vector.

    Algorithm based on Craig Haley's MatLab code, which in turn is 
    based on Nick Lloyd's ONYX software.

    @param X 3 element GPS position in meters
    @param los 3 element vector describing line of sight
    @param lon geodetic longitude in radian
    @param lat geodetic latitude in radian
    @param h geodetic height in meter
*/
void att2tp(double X[], double los[], double *lon, double *lat, double *h);

/** Quaternion to Euler angles.
    Transform set of quaternions (4 component vector) to corresponding
    Euler angles (3 component vector). When used to calculate R.A., Dec
    from given attitude, note that {\tt e[0] = R.A.} and {\tt e[1] = -Dec}.

    @param q quaternion to be transformed
    @param e resulting Euler angles
*/
void quat2eul(Quat q, Triple e);

/** Euler angles to quaternion.
    Transform set of Euler angles (3 component vector) to corresponding
    quaternions (4 component vector).

    @param e Euler angles to be transformed
    @param q resulting quaternion
*/
void eul2quat(Triple e, Quat q);

/** Euler angles to rotation matrix.
    Transform set of Euler angles (3 component vector) to corresponding
    3 by 3 rotation matrix.

    @param e Euler angles to be transformed
    @param m resulting rotation matrix
*/
void eul2mat(Triple e, rotMatrix m);

/** Quaternion to rotation matrix.
    Transform set of quaternions (4 component vector) to corresponding
    3 by 3 rotation matrix.

    @param q quaternion to be transformed
    @param m resulting rotation matrix
*/
void quat2mat(Quat q, rotMatrix m);

/** Rotation matrix to Euler angles.
    Transform 3 by 3 rotation matrix to corresponding set of 
    Euler angles (3 component vector).

    @param m rotation matrix to be transformed
    @param e resulting Euler angles
*/
void mat2eul(rotMatrix m, Triple e);

/** Multiply quaternions.
    
    @param q1 first quaternion to be multiplied 
    @param q2 second quaternion to be multiplied 
    @param r  resulting quaternion product
*/
void quatprod(Quat q1, Quat q2, Quat r);

/** Rotate vector by quaternion.

    @param v vector to be rotated
    @param q quaternion describing rotation
    @param w  resulting vector
 */
void qrotate(cVector v, Quat q, cVector w);

/** GPS to orbitual elements.
    Transform GPS position and velocity (passed as 6 component vector)
    to a set of osculating orbital elements. Returns a pointer to an array
    holding the elements.

    @param gps pointer to an array holding GPS position and velocity 
*/
double *gps2osc(double gps[]);


/** Orbitual elements to GPS.
    Transform set of osculating orbital elements (passed as 6 component vector)
    to corresponding GPS position and velocity. Returns a pointer to an array
    holding the GPS information.

    @param osc pointer to an array holding orbital elements.
*/
double *osc2gps(double orbit[]);

/** Position of the Sun.
    Calculate position of the Sun for given Julian date.
    Based on low precision formulae given in 
    T. C. Van Flandern and K. F. Pulkkinen, Ap.J. (Suppl.) 41, 391-411 (1979).
    Returns pointer to 3 element vector holding geocentric
    position vector (X,Y,Z) for the Sun in meter.

    @param JD Julian date
*/
double *Sun(double JD);

/** Position of the Moon.
    Calculate position of the Moon for given Julian date.
    Based on low precision formulae given in 
    T. C. Van Flandern and K. F. Pulkkinen, Ap.J. (Suppl.) 41, 391-411 (1979).
    Returns pointer to 3 element vector holding geocentric
    position vector (X,Y,Z) for the Moon in meter.

    @param JD Julian date
*/
double *Moon(double JD);

/** Position of Jupiter.
    Calculate position of Jupiter for given Julian date.
    Based on low precision formulae given in 
    T. C. Van Flandern and K. F. Pulkkinen, Ap.J. (Suppl.) 41, 391-411 (1979).
    Returns pointer to 3 element vector holding geocentric
    position vector (X,Y,Z) for Jupiter in meter.

    @param JD Julian date
*/
double *Jupiter(double JD);

/** Position of Saturn.
    Calculate position of Saturn for given Julian date.
    Based on low precision formulae given in 
    T. C. Van Flandern and K. F. Pulkkinen, Ap.J. (Suppl.) 41, 391-411 (1979).
    Returns pointer to 3 element vector holding geocentric
    position vector (X,Y,Z) for Saturn in meter.

    @param JD Julian date
*/
double *Saturn(double JD);

/** Galactic avoidance zone.
    Returns 1 if given R.A., Dec corresponds to a position in the Galaxy
    which should be avoided as a reference position based on the Columbia
    map of integrated CO 1-0 intensities. Returns 0, if reference is clear.

    @param ra right ascension of reference beam in decimal degrees
    @param dec declination of reference beam in decimal degrees
*/
int avoid(double ra, double dec);

/*@}*/

#endif
