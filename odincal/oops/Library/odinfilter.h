#ifndef ODINFILTER_H
#define ODINFILTER_H

/* $Id$ */

/* 
   History:
   
   Version 0.1: 
   original version written by Frank Merino and received by email
   on January 26, 1998 

   Version 0.2:
   modified version to bring it in line with odinscan.h
*/

/* read: version (high byte).(low byte), here: 0.8 */
#define ODINFILTERVERSION 0x0002

/* define frequency offsets in Hz from 3.9e9 Hz for the three channels */
#define LOWFREQ      0.0
#define MIDFREQ     70.0e6
#define HIFREQ     450.0e6  

#define FILTERCHANNELS 3

struct OdinFilter {             /* DESCRIPTION                     BYTE OFFSET */
  /* version and level */
  unsigned short Version;       /* version number                         [] */
  unsigned short Level;         /* level of data reduction applied        [] */
  unsigned long Quality;        /* status info for platform and payload   [] */

  /* spectrum identification */
  unsigned long STW;            /* satellite time word                    [] */
  double MJD;                   /* modified Julian date of observation    [] */
  double Orbit;                 /* number of orbit plus fraction          [] */
  float  LST;                   /* LST of observation (seconds of day)    [] */
  short  Discipline;            /* AERO or ASTRO                          [] */
  short  Topic;                 /* astronomy topic                        [] */
  short  Spectrum;              /* spectrum number in this orbit          [] */
  short  ObsMode;               /* observing mode (TPW, SSW, LSW, FSW)    [] */
  short  Type;                  /* type of spectrum: SIG, REF, CAL ...    [] */
  /* short  SkyBeamHit; */      /* indicates beam(s) on avoidance zones   [] */

  /* antenna parameters */
  float  Longitude;             /* geodetic long. of tangent point [degrees] */
  float  Latitude;              /* geodetic lat. of tangent point  [degrees] */
  float  Altitude;              /* altitude of tangent point             [m] */
  double Qtarget[4];            /* reference attitude                     [] */
  double Qachieved[4];          /* achieved attitude                      [] */
  double Qerror[3];             /* attitude error                  [degrees] */
  double GPSpos[3];             /* geocentric position X,Y,Z             [m] */
  double GPSvel[3];             /* velocity Xdot,Ydot,Zdot             [m/s] */
  /* double SunPos[3]; */       /* geocentric position of Sun            [m] */
  /* double MoonPos[3]; */      /* geocentric position of Moon           [m] */
  /* float  SunZD; */           /* solar zenith angle              [degrees] */

  /* radiometer parameters */
  float  Tcal;                  /* calbration temperature                [K] */
  float  Tsys;                  /* system temperature                    [K] */
  float  Trec;                  /* receiver temperature                  [K] */
  double LOFreq;                /* LO frequency                         [Hz] */
  double SkyFreq;               /* sky frequency                        [Hz] */
  double RestFreq;              /* rest frequency                       [Hz] */

  /* spectrometer parameters */
  double FreqCal[3];            /* frequency calibration coeff.         [Hz] */
  float  IntTime;               /* integration time                      [s] */
  float  EffTime;               /* effective integration time            [s] */

  /* spectrum */
  int    Channels;               /* number of channels                140    */
  float  data[FILTERCHANNELS];   /*                                   144    */
};

#endif
