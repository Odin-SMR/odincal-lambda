#undef VERBOSE
#include <stdio.h>
#include <math.h>

#include "odinscan.h"
#include "odinlib.h"
#include "astrolib.h"
#include "attlib.h"
#include "vector.h"

#define POINTLIMIT  60                       /* 60 arcsec */

#define SUNLIMIT    (30.0/60.0/180.0*PI)     /* 30 arcmin */
#define MOONLIMIT   (30.0/60.0/180.0*PI)     /* 30 arcmin */
#define PLANETLIMIT ( 5.0/60.0/180.0*PI)     /*  5 arcmin */

static char rcsid[] = "$Id";

/* sky beam 1 corresponds to 
   a rotation of a = -39.6 degrees around the y-axis, followed by 
   a rotation of b =  15.9 degrees around the z-axis.

   sky beam 2 corresponds to 
   a rotation of a = -37.1 degrees around the y-axis, followed by 
   a rotation of b = -19.9 degrees around the z-axis.

   Given a and b the components of the unit vector pointing in the 
   direction of the sky beam, in a coordinate system where the telescope
   axis has components (1,0,0), is given as

             {    cos(a)*cos(b)      sin(a)*cos(b)        -sin(b)     }
*/
static cVector s1 = { 0.74103441513220,  0.21108920598303,  0.63742398974869 };
static cVector s2 = { 0.74995869856869, -0.27148125895069,  0.60320798774528 };

/* the following three Euler angles describe the orientation of the SMR
   signal beam with respect to the satellite frame.
*/

struct alignment {
    double start;
    int version;
    double theta[3];
} ACS[] = {
    /* 2001-02-20 - 2001-08-31 ACSALIGN 2.3 */
    {     0.0, 0x0000, {  0.033682, -0.090285, -0.14340 } },

    /* 2001-09-01 - 2001-12-31 */
    {  2859.0, 0x0100, { -0.025654, -0.096412, -0.13990 } },

    /* 2002-01-01 - 2002-07-31 */
    {  4671.0, 0x0200, {  0.046613, -0.098859, -0.14722 } },

    /* 2002-08-01 - 2002-12-31 ACSALIGN 2.8 */
    {  7826.0, 0x0300, { -0.025654, -0.096412, -0.13990 } },

    /* 2003-01-01 - 2003-04-30 ACSALIGN 2.9 */
    { 10106.0, 0x0400, { -0.026454, -0.10713, -0.1448  } },

    /* 2003-05-01 - 2003-07-31 ACSALIGN 2.10 */
    { 11895.0, 0x0500, { -0.022094, -0.12258, -0.14559 } },

    /* 2003-08-01 - 2004-01-31 ACSALIGN 2.11 */
    { 13267.0, 0x0600, { -0.022358, -0.11899, -0.14052 } }, 

    /* 2004-02-01 - 2004-03-01 ACSALIGN 2.13 old */
    { 16013.0, 0x0700, { -0.022564, -0.12345, -0.14466 } },

    /* 2004-03-02 - 2004-07-31 ACSALIGN 2.13 */ /* also new ST alignment! */
    { 16461.0, 0x0800, { -0.022263, -0.12109, -0.14731 } },

    /* 2004-08-01 - 2004-09-14 ACSALIGN 2.14 */
    { 18730.0, 0x0900, { -0.022776, -0.11423, -0.14347 } },

    /* 2004-09-15 - 2009-12-31 ACSALIGN 2.15 */
    { 19402.0, 0x0a00, { -0.02227,  -0.11975, -0.14285 } }
};

/* As of 2001-09-18: */
/* version number of beam offset to be stored in spectra */

/********** ACSALIGN 2 3 **********/
/* static int BEAMOFFVERSION = 0x0100; */
/* static double theta[3] = {  0.033682, -0.090285, -0.143402 }; */

/********** ACSALIGN 2 7 **********/
/* static int BEAMOFFVERSION = 0x0200; */
/* static double theta[3] = {  0.030946, -0.083915, -0.142990 }; */

/********** ACSALIGN 2 8 **********/
/* static int BEAMOFFVERSION = 0x0300; */
/* static double theta[3] = { -0.025654, -0.096412, -0.13990  }; */

/********** ACSALIGN 2 9 **********/
/* static int BEAMOFFVERSION = 0x0400; */
/* static double theta[3] = { -0.026454, -0.107130, -0.14480  }; */

/********** ACSALIGN 2 10 **********/
/* static int BEAMOFFVERSION = 0x0500; */
/* static double theta[3] = { -0.022094, -0.12258,  -0.14559  }; */

/********** ACSALIGN 2 11 **********/
/* static int BEAMOFFVERSION = 0x0600; */
/* static double theta[3] = { -0.022358, -0.11899,  -0.14052 }; */

/********** ACSALIGN 2 12 **********/ /* new ST alignment */
/* static int BEAMOFFVERSION = 0x0700; */
/* static double theta[3] = { -0.022001, -0.11468,  -0.14135 }; */

/********** ACSALIGN 2 13 **********/
static int BEAMOFFVERSION = 0x0800;
static double theta[3] = { -0.022263, -0.12109,  -0.14731 };

/*
   Supply new ACS alignment parameters.
*/
void setAlignment(int version, double t[3])
{
    BEAMOFFVERSION = version;
    theta[0] = t[0];
    theta[1] = t[1];
    theta[2] = t[2];
}

static Quat qsmr = { 0.0, 0.0, 0.0, 0.0 };

/* 
   Transform vector from earth centred to Odin centred system.
   Vector v holds the coordinates to be transformed (m), earth is
   a vector holding the position of Earth as seen from the satellite (m).
 */
static void transform(cVector v, cVector earth)
{
    int i;

    for (i = 0; i < 3; i++) v[i] += earth[i];
}

/*
  The following routine tests for the various avoidance zones
  and sets the 'SkyBeamHit' structure member accordingly.

  The Qtarget reference attitude needs to be filled in before calling
  this routine.

  Side effects: 
  - the RA2000 and Dec2000 members are calculated and filled in. 
  - the positions of Sun and Moon are calculated and filled in. 
*/
void skybeams(struct OdinScan *scan, double placs)
{
    cVector e, sky1, sky2, mb, earth, tp, vsat;
    rotMatrix m, n;
    pVector beam;
    double EARTHLIMIT, R, d1, d2, d3, d4, dv, o;
    Quat smr, acs, soda;
    double JD, *sol, *moon, *jupiter, *saturn;
    double day, f, gmst;
    double lon, lat, h, err;
    int i, j, nacs;

    if ((scan->Level & 0xff00) == 0) {
	nacs = sizeof(ACS)/sizeof(struct alignment);
	o = scan->Orbit;
	for (i = 1; i < nacs; i++) {
	    if (o < ACS[i].start) break;
	}

	i--;
	BEAMOFFVERSION = ACS[i].version;
	for (j = 0; j < 3; j++) theta[j] = ACS[i].theta[j];
#ifdef VERBOSE
	printf("using alignment: %04X %10.6f %10.6f %10.6f\n", 
	       BEAMOFFVERSION, theta[0],  theta[1],  theta[2]);
#endif
    }

    JD = mjd2jd(scan->MJD);
    f = modf(JD, &day);
    gmst = modf(GMST(day+0.5)+ROTATE*(f-0.5), &day);

    for (i = 0; i < 4; i++) acs[i] = scan->Qachieved[i];

    if (placs != 0.0) {
	ODINinfo("picked up PLACS: %5.1f arcsec", placs*3600.0);
	e[0] = placs*M_PI/180.0;
	e[1] = 0.0;
	e[2] = 0.0;
	eul2quat(e, soda);
	quatprod(acs, soda, acs);
    }

    if (qsmr[0] + qsmr[1] + qsmr[2] + qsmr[3] == 0.0) {
	e[0] = theta[2]*M_PI/180.0;
	e[1] = theta[1]*M_PI/180.0;
	e[2] = theta[0]*M_PI/180.0;
	eul2quat(e, qsmr);
    }
    quatprod(acs, qsmr, smr);
    quat2eul(smr, e);

    /* calculate RA, Dec 2000 from Euler angles e */
    beam.l =  e[0];
    beam.b = -e[1];
    cuv(&beam, mb);
#ifdef VERBOSE
    printf("Julian date:  %15.4f\n", JD);
    printf("qt(acs): %15.6e %15.6e %15.6e %15.6e\n",acs[0],acs[1],acs[2],acs[3]);
    printf("corr.:   %15.6e %15.6e %15.6e %15.6e\n",qsmr[0],qsmr[1],qsmr[2],qsmr[3]);
    printf("qt(smr): %15.6e %15.6e %15.6e %15.6e\n",smr[0],smr[1],smr[2],smr[3]);
    printf("Euler:   %15.6e %15.6e %15.6e\n", e[0], e[1], e[2]);
    printf("unit:    %15.6e %15.6e %15.6e\n", mb[0], mb[1], mb[2]);
#endif

    /* Remove effect of aberation */
    Doppler(scan, e[0], -e[1], vsat);
    dv = dot(mb, vsat)/LIGHTSPEED;
    for (i = 0; i < 3; i++) {
	e[i] = (mb[i]-vsat[i]/LIGHTSPEED)/(1.0-dv);
    }
    normalise(e);

    /* Remove effect of nutation */
    nut(JD, n);
    inv(n);
    rotate(n, e, e);
#ifdef VERBOSE
    printf("nutated: %15.6e %15.6e %15.6e\n", e[0], e[1], e[2]);
#endif

    /* Precess to J2000 */
    pre(JD, J2000, m);
    rotate(m, e, e);
#ifdef VERBOSE
    printf("precess: %15.6e %15.6e %15.6e\n", e[0], e[1], e[2]);
#endif
  
    uvc(e, &beam);
#ifdef VERBOSE
    printf("RA, Dec: %15.6e %15.6e\n", beam.l, beam.b);
#endif

    scan->RA2000  = beam.l*180.0/PI;
    scan->Dec2000 = beam.b*180.0/PI;

    sol     = Sun(JD);
    moon    = Moon(JD);
    jupiter = Jupiter(JD);
    saturn  = Saturn(JD);
#ifdef VERBOSE
    printf("Sun:     %15.6e %15.6e %15.6e\n", sol[0],sol[1],sol[2]);
    printf("Moon:    %15.6e %15.6e %15.6e\n", moon[0],moon[1],moon[2]);
    printf("Jupiter: %15.6e %15.6e %15.6e\n", jupiter[0],jupiter[1],jupiter[2]);
    printf("Saturn:  %15.6e %15.6e %15.6e\n", saturn[0],saturn[1],saturn[2]);
#endif

    for (i = 0; i < 3; i++) {
	scan->SunPos[i]  = sol[i];
	scan->MoonPos[i] = moon[i];
    }
    /* precess to standard epoch J2000 */
    rotate(m, scan->SunPos, scan->SunPos);
    rotate(m, scan->MoonPos, scan->MoonPos);

    /* retrieve and normalise vector pointing to Earth */
    for (i = 0; i < 3; i++) earth[i] = -scan->GPSpos[i];
#ifdef VERBOSE
    printf("Odin:    %15.6e %15.6e %15.6e\n", -earth[0], -earth[1], -earth[2]);
#endif

    /* We want to mark all spectra with a beam closer than h to the surface
       of the Earth. */
    h = 120.0e3;                     /* h = 120 km               */
    R = (6378.14e3+6356.755e3)/2.0;  /* mean radius of the Earth */
    /* EARTHLIMIT = asin((R+h)/norm(earth)); */
    EARTHLIMIT = 82.0*M_PI/180.0;
#ifdef VERBOSE
    printf("heigth:  %15.6e %15.6e\n", norm(earth), norm(earth)-R);
    printf("earth limit:  %7.3f\n", EARTHLIMIT*180/M_PI);
#endif

    /* Transform position vectors for Sun, Moon, Jupiter and Saturn
       to a satellite centred frame and normalise. */
    transform(sol, earth);
    normalise(sol);

    transform(moon, earth);
    normalise(moon);

    transform(jupiter, earth);
    normalise(jupiter);

    transform(saturn, earth);
    normalise(saturn);

    normalise(earth);
    scan->SkyBeamHit = 0;
    quat2mat(acs, m);
    /* 
       Multiply vector for main beam with inverse (=transpose) rotation matrix 
    */
    /*    mb[0] = m[0][0]; */
    /*    mb[1] = m[0][1]; */
    /*    mb[2] = m[0][2]; */
    /*    normalise(mb); */
    uvc(mb, &beam);
    /* d1 is elevation + 90 degrees */
    if ((d1 = distance(mb, earth))  < EARTHLIMIT)   scan->SkyBeamHit |= EARTHMB;
    if ((d2 = distance(mb, moon))   < MOONLIMIT)    scan->SkyBeamHit |= MOONMB;
    if ((d3 = distance(mb, jupiter)) < PLANETLIMIT) scan->SkyBeamHit |= JUPITERMB;
    if ((d4 = distance(mb, saturn)) < PLANETLIMIT)  scan->SkyBeamHit |= SATURNMB;
#ifdef VERBOSE
    printf("main:    %15.6e %15.6e %15.6e\n", mb[0], mb[1], mb[2]);
    printf("main to earth:   %7.3f\n", d1*180.0/M_PI);
    printf("main to moon:    %7.3f\n", d2*180.0/M_PI);
    printf("main to jupiter: %7.3f\n", d3*180.0/M_PI);
    printf("main to saturn:  %7.3f\n", d4*180.0/M_PI);
    printf("sky beam hit:    %04X\n", scan->SkyBeamHit);
#endif

    /*    printf("%10ld ", scan->STW); */
    /*    printf("%10.4f ", scan->Orbit); */
    /*    printf("%7.3f ", d1*180.0/M_PI - 90.0); */
  
    /* For the sky beas we increase EARTHLIMIT by half the sky beam's FWHM */
    /*    EARTHLIMIT += (4.5/2.0)*M_PI/180.0; */

    /* 
       Multiply vector for sky beam 1 with inverse (=transpose) rotation matrix 
    */
    sky1[0] = m[0][0]*s1[0]+m[1][0]*s1[1]+m[2][0]*s1[2];
    sky1[1] = m[0][1]*s1[0]+m[1][1]*s1[1]+m[2][1]*s1[2];
    sky1[2] = m[0][2]*s1[0]+m[1][2]*s1[1]+m[2][2]*s1[2];
    normalise(sky1);
    if ((d1 = distance(sky1, earth)) < EARTHLIMIT)  scan->SkyBeamHit |= EARTH1;
    if ((d2 = distance(sky1, sol))   < SUNLIMIT)    scan->SkyBeamHit |= SUN1;
    if ((d3 = distance(sky1, moon))  < MOONLIMIT)   scan->SkyBeamHit |= MOON1;
#ifdef VERBOSE
    printf("sky1:    %15.6e %15.6e %15.6e\n", sky1[0], sky1[1], sky1[2]);
    printf("sky1 to earth:   %7.3f\n", d1*180.0/M_PI);
    printf("sky1 to sun:     %7.3f\n", d2*180.0/M_PI);
    printf("sky1 to moon:    %7.3f\n", d3*180.0/M_PI);
    printf("sky beam hit:    %04X\n", scan->SkyBeamHit);
#endif
    /*    printf("%7.3f ", d1*180.0/M_PI - 90.0); */
    rotate(n, sky1, sky1);
    rotate(m, sky1, sky1);
    uvc(sky1, &beam);
    if (avoid(beam.l, beam.b))               scan->SkyBeamHit |= GALAX1;
    /*    if (scan->Type == SK1) { */
    /*      scan->RA2000  = beam.l*180.0/PI; */
    /*      scan->Dec2000 = beam.b*180.0/PI; */
    /*    } */

    /* 
       Multiply vector for sky beam 1 with inverse (=transpose) rotation matrix 
    */
    sky2[0] = m[0][0]*s2[0]+m[1][0]*s2[1]+m[2][0]*s2[2];
    sky2[1] = m[0][1]*s2[0]+m[1][1]*s2[1]+m[2][1]*s2[2];
    sky2[2] = m[0][2]*s2[0]+m[1][2]*s2[1]+m[2][2]*s2[2];
    normalise(sky2);
    if ((d1 = distance(sky2, earth)) < EARTHLIMIT)  scan->SkyBeamHit |= EARTH2;
    if ((d2 = distance(sky2, sol))   < SUNLIMIT)    scan->SkyBeamHit |= SUN2;
    if ((d3 = distance(sky2, moon))  < MOONLIMIT)   scan->SkyBeamHit |= MOON2;
#ifdef VERBOSE
    printf("sky2:    %15.6e %15.6e %15.6e\n", sky2[0], sky2[1], sky2[2]);
    printf("sky2 to earth:   %7.3f\n", d1*180.0/M_PI);
    printf("sky2 to sun:     %7.3f\n", d2*180.0/M_PI);
    printf("sky2 to moon:    %7.3f\n", d3*180.0/M_PI);
    printf("sky beam hit:    %04X\n", scan->SkyBeamHit);
#endif
    /*    printf("%7.3f\n", d1*180.0/M_PI - 90.0); */
    rotate(n, sky2, sky2);
    rotate(m, sky2, sky2);
    uvc(sky2, &beam);
    if (avoid(beam.l, beam.b))               scan->SkyBeamHit |= GALAX2;
    /*    if (scan->Type == SK2) { */
    /*      scan->RA2000  = beam.l*180.0/PI; */
    /*      scan->Dec2000 = beam.b*180.0/PI; */
    /*    } */

    /*    lon = scan->u.tp.Longitude*M_PI/180.0;  */
    /*    lat = scan->u.tp.Latitude*M_PI/180.0;  */
    /*    h = scan->u.tp.Altitude;  */

    att2tp(scan->GPSpos, mb, &lon, &lat, &h);

    geocentric(lon, lat, h, tp);
    normalise(tp);
    scan->SunZD = distance(sol,tp)*180.0/M_PI;

    lon = lon-gmst*2.0*M_PI;
    lon *= 180.0/M_PI;
    if (lon > 360.0) lon -= 360.0;
    if (lon <   0.0) lon += 360.0;
    lat *= 180.0/M_PI;
    if (lat >  90.0) lat -= 180.0;
    if (lat < -90.0) lat += 180.0;

    scan->u.tp.Longitude = lon;
    scan->u.tp.Latitude  = lat;
    scan->u.tp.Altitude  = h;

    err = 0.0;
    for (i = 0; i < 3; i++) {
	err += scan->Qerror[i]*scan->Qerror[i];
    }
    err = sqrt(err);
    if (err > POINTLIMIT) scan->Quality |= WPOINTING;
    scan->Level = BEAMOFFVERSION;
}
