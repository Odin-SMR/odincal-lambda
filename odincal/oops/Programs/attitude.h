#ifndef ATT_H
#define ATT_H

/* $Id$ */

#define BLOCKS   3      /* number of blocks in attitude file               */
#define COLUMNS 31      /* number of columns in block 3                    */

struct Timeline {
  unsigned short Index;           /* block index                       */
  int Orbit;                      /* number of orbit plus fraction     */
  int Spectrum;                   /* spectrum number in this orbit     */
  char Source[32];                /* source name                       */
  int Discipline;                 /* AERO or ASTRO                     */
  int Topic;                      /* astronomy topic                   */
  int ObsMode;                    /* observing mode (TPW, SSW, LSW, FSW*/
  int Frontend;                   /* frontend used   (A1, A2, B1, B2, C*/
  int Backend;                    /* backend used    (AOS, AC1, AC2)   */
  float RA2000;                   /* right ascension (J2000) (radian)  */
  float Dec2000;                  /* declination (J2000) (radian)      */
  float LOSangle;                 /* LOS angle (radian)                */
  float VSource;                  /* LSR velocity of source (m/s)      */
};

struct Attitude {
  int    Year;              /* year of observation                         */
  int    Month;             /* month of observation                        */
  int    Day;               /* day of observation                          */
  float  UTC;               /* UTC of observation (seconds of day)         */
  unsigned long STW;        /* satellite time word                         */
  double Orbit;             /* orbit number plus fraction                  */
  double Qtarget[4];        /* reference attitude                          */
  double Qachieved[4];      /* achieved attitude                           */
  double Qerror[3];         /* attitude error                              */
  double GPSpos[3];         /* geocentric position X,Y,Z (m)               */
  double GPSvel[3];         /* velocity Xdot,Ydot,Zdot (m/s)               */
  float  OSLongitude;       /* geodetic longitude of Osiris (radian)       */ 
  float  OSLatitude;        /* geodetic latitude of Osiris (radian)        */
  float  OSAltitude;        /* altitude of Osiris (m)                      */
  float  RMLongitude;       /* geodetic longitude of radiometer (radian)   */ 
  float  RMLatitude;        /* geodetic latitude of radiometer (radian)    */
  float  RMAltitude;        /* altitude of radiometer (m)                  */
  int    GyroData;          /* 1 when gyro data available, 0 otherwise     */
};

int ReadAttitude(FILE *, struct Attitude *);
void PrintAttitude(struct Attitude *);

#endif
