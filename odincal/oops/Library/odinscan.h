/**@name OdinScan */

#ifndef ODINSCAN_H
#define ODINSCAN_H

/* $Id$ */

/**@name OdinScan

    This structure plays a central role for the processing
    of Odin SMR spectral line data. It is in this format that data will
    be stored and handled by the processing computer at Onsala Space 
    Observatory. Many of the routines described further down in this
    documentation require a pointer to such a structure as one of their 
    input and/or output parameters.

    History:
   
    Version 0.1: 
    Original version sent out to members of Odin data working group for
    discussion on March 19, 1997.

    Version 0.2:
    Slightly modified version presented at Odin data working group
    meeting in Toulouse, June 5, 1997.

    Version 0.3:
    Incorporated changes suggested at Odin data working group meeting
    in Toulouse, June 5, 1997 and rearranged and grouped structure
    members following suggestions made by Alain Lecacheux.

    Version 0.4:
    Added source name and topic.
    Added positions of Sun and Moon.
    Added flags indicating if sky beams point at Sun, Moon, Earth 
    or avoidance zone.
    Based on agreements from Odin meetings on April 20, 1998 at CTH
    and on May 15, 1998 at SSC.

    Version 0.5:
    Added sideband IF frequency which has maximum suppression.
    Added solar zenith angle (requested by Eric Le Flochmoen, France).
    Based on e-mail received from Frank Merino, MISU.

    Version 0.6:
    Rearrangements of a few structure members in order to optimize for
    conversion to FITS binary tables.

    Version 0.7:
    Major changes concerning new and removed structure members, following
    Odin scheduling meeting at SSC, Stockholm on March 9, 1999.
    Units of angles changed from radian to degrees.

    Version 0.8:
    Added structure member EffTime, following calibration meeting at MISU.
    Removed structure member Bandwidth. Added union to support tangent point
    in aeronomy mode and map position in astronomy mode.

    Version 0.9:
    Added new spectra types: SK1, SK2, DRK for sky beam 1, sky beam 2 
    and AOS dark spectrum, respectively. 
    Edited comments for frontend types.

    Version 1.0:
    Added backend FBA, after decision at OST 35 to store filter bank data
    as 3 channel spectrometer and abandoning extra file formats for FBA data.
    Added flag SATURNMB and replaced flags EAC1, EAC2 and EAOS by ECALMIRROR
    and ESIGLEVEL.
    Replaced member Vhel by Vgeo, following suggestion by Nicolas Biver.
    Added doc++ style comments.

    Version 1.1
    Added spectrum type SPE for calibrated spectra. Added bit definition
    WBANDADJUST which will be set in correlator spectra which show 
    stair-caseing. Redefined bit definition for ICOMMISSION.

    Version 1.2
    Replaced structure member 'Trec' by 'SBpath', after discussion during
    a meeting of the Odin SMR aeronomy data retrieval group at Onsala, May 
    21-23, 2001.
    Added definitions of constants for storing the aeronomy mode in member
    'Topic'.

    Version 1.3
    Added spectrum type SSB for sideband ratio, after SMR-RG meeting in 
    Bordeaux. Also added type AVE for averaged spectra, mainly meant for
    astronomy.

    Version 1.4
    Corrected definitions of SkyBeamHit related constants (EARTH1, ...)
    to correspond to individual bits.

    Version 1.5
    Introduced new way of describing IntMode for AC1 and AC2. These new
    mode designations are characterised by setting bit ADC_SEQ in header
    variable IntMode. Bits AC_SPLIT and AC_UPPER are replaced by ADC_SPLIT
    and ADC_UPPER. All IntMode dependent code is supposed to handle both
    old and new modes and should check bit ADC_SEQ before processing.

    Version 1.6
    Replaced header variable FreqThrow by SodaVersion, in order to store
    software version used during attitude reconstruction.
*/

//@{
/**@name Constants

   The following macros should be used whenever possible rather than 
   integer constants, to make sure that program code stays portable 
   from one version to another. This means you should use code like
   \begin{verbatim}
   if (scan->Backend == AOS) ...
   \end{verbatim}
   rather than
   \begin{verbatim}
   if (scan->Backend ==   3) ...
   \end{verbatim}
   to test for the backend information in an OdinScan structure.

   The following constants (macros) are defined:
   \begin{verbatim}

   #define ODINSCANVERSION  current version 

   Define the disciplines:
   #define AERO             aeronomy
   #define ASTRO            astronomy

   Define the various topics in astronomy:
   #define SOLSYS           solar system
   #define STARS            late type stars
   #define EXTGAL           extragalactic
   #define LMC              Magellanic clouds
   #define PRIMOL           primordial molecules
   #define SPECTR           spectral scans
   #define CHEM             interstellar chemistry
   #define GPLANE           galactic plane
   #define GCENTR           galactic centre
   #define GMC              giant molecular clouds
   #define SFORM            star formation
   #define DCLOUD           dark clouds
   #define SHOCKS           shocks and outflows
   #define PDR              photon dominated regions
   #define HILAT            high latitude clouds
   #define ABSORB           absorption line studies
   #define ORION            common Orion project
   #define CALOBS           calibration observations
   #define COMMIS           commissioning phase measurement

   Define modes for aeronomy:
   #define STRAT            stratospheric mode (1 or 2)
   #define ODD_N            odd nitrogen mode
   #define ODD_H            odd hydrogen mode (1,2 or 3)
   #define WATER            water mode (1 or 2)
   #define SUMMER           summer mesosphere
   #define DYNA             dynamic ...

   Available backends:
   #define AC1              correlator 1
   #define AC2              correlator 2
   #define AOS              acousto optic spectrometer
   #define FBA              3 channel filter bank

   Backend modes:
   #define AOS_LONG         1728 32-bit data.                   
   #define AOS_SHORT        1728 16-bit data.                   
   #define AOS_HALF          864 32-bit data.                   
   #define AOS_FOUR          432 32-bit data.                   
   #define AOS_CENTRE       high resolution at centre.          
   #define AOS_WINGS        high resolution in wings.           
   #define AOS_WINDOW       high resolution window.             

   #define AC_XHIRES        1 x 100 MHz at  125 kHz resolution. 
   #define AC_HIRES         2 x 100 MHz at  250 kHz resolution. 
   #define AC_MEDRES        4 x 100 MHz at  500 kHz resolution. 
   #define AC_LOWRES        4 x 200 MHz at 1000 kHz resolution. 
   #define AC_YHIRES        1 x 100 MHz at 1000/7 kHz resolution (seven chips!)
   #define AC_SPLIT         correlator split between frontends. 
   #define AC_UPPER         correlator upper half.              
   #define ADC_SEQ          new version for ADC mode coding
   #define ADC_SPLIT        correlator split between frontends (new) 
   #define ADC_UPPER        correlator upper half (new)

   Available receivers (frontends):
   #define REC_555          mixer B2.                           
   #define REC_495          mixer A2.                           
   #define REC_572          mixer B1.                           
   #define REC_549          mixer A1.                           
   #define REC_119          mixer C.                            
   #define REC_SPLIT        power combiner, correlators only.   

   Define the various observing modes:
   #define TPW              total power measurement.                
   #define SSW              sky switching.                          
   #define LSW              load switching.                         
   #define FSW              frequency switching (only for 119 GHz.) 

   Define the various spectrum types:
   #define SIG              on spectrum towards target.             
   #define REF              reference spectrum.                     
   #define CAL              calibration spectrum towards load.      
   #define CMB              frequency comb spectrum (only for AOS.) 
   #define DRK              CCD dark current spectrum (only AOS.)   
   #define SK1              sky 1 reference beam.                   
   #define SK2              sky 2 reference beam.                   
   #define SPE              calibrated spectrum.
   #define SSB              sideband ratio spectrum.
   #define AVE              averaged calibrated spectra.

   Define flags for avoidance objects and zones:
   #define SKYBEAMS         number of sky beams.                   
   #define EARTH1           sky beam 1 points at Earth.            
   #define MOON1            sky beam 1 points at Moon.             
   #define GALAX1           sky beam 1 points at galactic plane.   
   #define SUN1             sky beam 1 points at Sun.              
   #define EARTH2           sky beam 2 points at Earth
   #define MOON2            sky beam 2 points at Moon
   #define GALAX2           sky beam 2 points at galactic plane
   #define SUN2             sky beam 2 points at Sun.              
   #define EARTHMB          main beam points at Earth.             
   #define MOONMB           main beam points at Moon.              
   #define JUPITERMB        main beam points at Jupiter.           
   #define SATURNMB         main beam points at Saturn.           

   A few useful macros to determine the size of the structure below:
   #define SOURCENAMELEN    length of source name.      
   #define MAXCHANNELS      maximum number of channels. 
   #define MAXDATA          maximium length of data.    
   #define SCANLEN          scan length in bytes.       
   #define HEADLEN          length of header in bytes.  

   Error, warning and info codes (preliminary):
   #define STWRSTMASK       nibble will hold reset cnt.           
   #define EPLATFORM        platform error.                       
   #define EPLL             PLL error, LO unlocked.               
   #define ECALMIRROR       position of cal mirror undefined
   #define ESIGLEVEL        signal level in error
   #define WFREQUENCY       warn: frequency unreliable.           
   #define WAMPLITUDE       warn: amplitude unreliable.           
   #define WPOINTING        warn: unreliable pointing
   #define WBANDADJUST      one or more correlator bands adjusted
   #define ILINEAR          spectrum has been liearised
   #define ISORTED          spectrum has been frequency sorted
   #define ICOMMISSION      taken during commissioning.           
   \end{verbatim}
*/

#define ODINSCANVERSION 0x0106

/* 
   Note, that in the various lists below enumerations start at 1 
   and not(!) at 0. 
   This enables proper treatment of uninitialized structure members 
*/
#define UNDEFINED  0

#define AERO       1
#define ASTRO      2

#define SOLSYS     1
#define STARS      2 
#define EXTGAL     3 
#define LMC        4 
#define PRIMOL     5 
#define SPECTR     6 
#define CHEM       7 
#define GPLANE     8 
#define GCENTR     9 
#define GMC       10 
#define SFORM     11 
#define DCLOUD    12 
#define SHOCKS    13 
#define PDR       14 
#define HILAT     15 
#define ABSORB    16 
#define ORION     17 
#define CALOBS    18 
#define COMMIS    19 

#define STRAT      1
#define ODD_N      2
#define ODD_H      3
#define WATER      4
#define SUMMER     5
#define DYNA       6

#define AC1        1
#define AC2        2
#define AOS        3
#define FBA        4

#define AOS_LONG   1 
#define AOS_SHORT  2 
#define AOS_HALF   3 
#define AOS_FOUR   4 
#define AOS_CENTRE 5 
#define AOS_WINGS  6 
#define AOS_WINDOW 7 

#define AC_XHIRES  1 
#define AC_HIRES   2 
#define AC_MEDRES  3 
#define AC_LOWRES  4 
#define AC_YHIRES  5
#define AC_SPLIT (1<<4)
#define AC_UPPER (1<<5)

#define ADC_SEQ   (1<<8)
#define ADC_SPLIT (1<<9)
#define ADC_UPPER (1<<10)

#define REC_555    1    
#define REC_495    2    
#define REC_572    3    
#define REC_549    4    
#define REC_119    5    
#define REC_SPLIT  6    

#define TPW   1      
#define SSW   2      
#define LSW   3      
#define FSW   4      

#define SIG   1      
#define REF   2      
#define CAL   3      
#define CMB   4      
#define DRK   5      
#define SK1   6      
#define SK2   7      
#define SPE   8
#define SSB   9
#define AVE  10

#define SKYBEAMS          2   
#define EARTH1       0x0001   
#define MOON1        0x0002   
#define GALAX1       0x0004   
#define SUN1         0x0008   
#define EARTH2       0x0010   
#define MOON2        0x0020   
#define GALAX2       0x0040   
#define SUN2         0x0080   
#define EARTHMB      0x0100   
#define MOONMB       0x0200   
#define JUPITERMB    0x0400   
#define SATURNMB     0x0800   

#define SOURCENAMELEN    32                  
#define MAXCHANNELS    1728                  
#define MAXDATA MAXCHANNELS*sizeof(float) 
#define SCANLEN sizeof(struct OdinScan)   
#define HEADLEN (SCANLEN-MAXDATA)         

#define STWRSTMASK   0x0000000f 
#define EPLATFORM    0x00000010 
#define EPLL         0x00000020 
#define ESIGLEVEL    0x00000100
#define ECALMIRROR   0x00000800 
#define WFREQUENCY   0x00001000 
#define WAMPLITUDE   0x00002000 
#define WPOINTING    0x00004000 
#define WBANDADJUST  0x00010000
#define ILINEAR      0x01000000 
#define ISORTED      0x02000000 
#define ICOMMISSION  0x10000000 

typedef union {
  /* aeronomy: tangent point parameters */
  struct {
    float  Longitude;
    float  Latitude;
    float  Altitude;
  } tp;
  /* astronomy: map parameters */
  struct {
    float  Xoff;
    float  Yoff;
    float  Tilt;
  } map;
} point;

/**@name Layout */
struct OdinScan {
  /** Version number.
      The version number of {\tt odinscan.h} which was in use when
      the data were created. When reading a spectrum from disk, one should
      check that the version stored in the spectrum header agrees with
      the version your software was compiled with:
      \begin{verbatim}
      if (spectrum->Version == ODINSCANVERSION) {
         // ok to process with this software
         ...
      }
      \end{verbatim}
      When writing data the software should make sure that the version 
      number is set correctly.
      \begin{verbatim}
      spectrum->Version = ODINSCANVERSION;
      \end{verbatim}
      When read as a hexadecimal number, the high nibble will contain the
      major version number, the low nibble the minor version. During
      software development the major version number will be 0 and any
      software using major version 0 shall be considered preliminary.

      Related constant: ODINSCANVERSION
  */
  unsigned short Version;

  /** Level of data reduction applied.
      This word is used to indicate level and version numbers of
      calibration procedures and pointing constants have been used.
  */
  unsigned short Level;

  /** Status info for platform and payload.
      Up to 32 bit of status information for various platform and payload
      components. Macros (see below) starting with {\tt E} indicate 
      error conditions, those starting with {\tt W} indicate warnings and
      those starting with {\tt I} are for informative purposes.

      Related constants:

      STWRSTMASK,
      EPLATFORM,
      EPLL,
      EAOS,
      EAC1,
      EAC2,
      WFREQUENCY,
      WAMPLITUDE,
      WBANDADJUST,
      ICOMMISSION

      @see Constants
  */
  unsigned long Quality;

  /** Satellite time word.
      This is simply a copy of the satellite time word from the first
      block of level 0 data belonging to this spectrum. As this is the
      STW when data were transferred from the spectrometers to the mass
      memory, it corresponds to the end of the measurement.
   */
  unsigned long STW;

  /** Modified Julian date of observation.
      The modified Julian date at the start of the observation.
      The modified Julian date is related to the Julian date {\tt JD}
      in use in astronomy by:
      \begin{verbatim}
      JD = spectrum->MJD + 2400000.5;
      \end{verbatim}
      Note, that the Julian date starts at noon, whereas the modified
      Julian date starts at midnight, e.g. MJD=0.0 corresponds to 1858
      November 17 at 0.0 UTC. The fractional part gives UTC in seconds
      of day via
      \begin{verbatim}
      UTC = modf(spectrum->MJD)*86400.0;
      \end{verbatim}
  */
  double MJD;

  /** Number of orbit plus fraction.
      The orbit number at the start of the observation. The fractional part
      is the phase (position) within the orbit relative
      to the equator crossing. The orbit number is extracted from
      column 6 of block 3 of an attitude file.
  */
  double Orbit;

  /** Local sidereal time of observation.
      The local (mean) sidereal time at the start of the observation in
      seconds. It is calculated based on UTC and the current
      satellite position. The latter in turn is derived from the GPS
      position retrieved from columns 18-20 of block 3 of an attitude file.
  */
  float  LST;

  /** Source name.
      A string of 32 characters holding the name of the source
      (in astronomy) or some other description of the current spectrum
      (in aeronomy).
      
      Related constants:

      SOURCENAMELEN

      The source name is extracted from the unvalidated time line (block 1)
      of an attitude file. The source name will be stored (with 16 characters
      only) in comment fields enclosed in square brackets on the same line as
      the {\tt newompb} keyword. Example:
      \begin{verbatim}
      newompb:   0 [CALOBS W3(OH)             -45.0 POS        ]
                           ----------------                    
      \end{verbatim}
  */
  char   Source[SOURCENAMELEN];

  /** Discipline.
      The discipline for which this spectrum was taken, either aeronomy or
      astronomy.
      
      Related constants:

      AERO,
      ASTRO

      The discipline is indicated by bit ?? in the index word of
      every data block of level 0 AC1, AC2 and AOS files. It may be
      retrieved using the following statement:
      \begin{verbatim}
      scan->Discpline = (index & 0x0000) ? ASTRO : AERO;
      \end{verbatim}
  */
  short  Discipline;
  
  /** Astronomy topic.
      In astronomy, the topical team which requested these observations.
      Not used by aeronomy.
      
      Related constants:

      SOLSYS,
      STARS,
      EXTGAL,
      LMC,
      PRIMOL,
      SPECTR,
      CHEM,
      GPLANE,
      GCENTR,
      GMC,
      SFORM,
      DCLOUD,
      SHOCKS,
      PDR,
      HILAT,
      ABSORB,
      ORION,
      CALOBS,
      COMMIS

      The topical team is extracted from the unvalidated time line (block 1)
      of an attitude file. It will be stored (with 6 characters)
      in comment fields enclosed in square brackets on the same line as
      the {\tt newompb} keyword. Example:
      \begin{verbatim}
      newompb:   0 [CALOBS W3(OH)             -45.0 POS        ]
                    ------
      \end{verbatim}
  */
  short  Topic;

  /** Spectrum number in this orbit.
      An integer counting the individual spectra of one orbit,
      starting at 1. Because it is planned to write data to files grouped
      by orbits, this member will normally be counting spectra within a
      level 1b data file, as well. For averaged spectra it is the
      corresponding number of the first spectrum used for the average.
  */
  short  Spectrum;

  /** Observing mode.
      The observing mode in use when this spectrum was taken.
      
      Related constants:

      TPW,
      SSW,
      LSW,
      FSW
  */
  short  ObsMode;
  
  /** Type of spectrum.
      The type of the current spectrum.

      Related constants:

      SIG,
      REF,
      CAL,
      CMB,
      DRK,
      SK1,
      SK2,
      SPE,
      SSB,
      AVE

      Except for the last two qualifiers (which will be set by the level 1a
      to level 1b data reduction software) this information is retrieved
      mainly from the level 0 files of all spectrometers. All spectrometers
      store the position of the chopper wheel, i.e. SIG or REF. A REF
      spectrum can be qualified as CAL, SK1 or SK2 once the position of the
      calibration mirror is known. This is retrieved either from the
      spectrometer files (in the case of the AOS) or from {\tt STW.FBA}
      files, which record the position of this mirror with 1 second sampling.
  */
  short  Type;

  /** Frontend used.
      The frontend used for this observation.

      Related constants:

      REC_495,
      REC_549,
      REC_555,
      REC_572,
      REC_119,
      REC_SPLIT

      The information is retrieved from the input channel reported by
      each spectrometer. For the split modes and the currently accepted
      frontend configurations, AC1 will hold REC_495 data in its lower half
      and REC_549 in its upper half. AC2 will hold REC_572 data in its
      lower half and REC_555 in its upper half. During the data processing
      from level 1a to level 1b the two halfs of any split mode will be
      broken up in two individual spectra and stored as such.
  */
  short  Frontend;


  /** Backend used.
      The backend used for this observation.

      Related constants:

      AC1,
      AC2,
      AOS
  */
  short  Backend;

  /** Indicates beam(s) on avoidance zones.
      The 16 bits of this word indicate possible hits of major
      sources of submillimetre emission by one of the skybeams or the
      main beam. E.g to test if the main beam was pointing at the Moon
      when a spectrum was taken during a limb scan measurement in aeronomy
      mode:
      \begin{verbatim}
      if (spectrum->SkyBeamHit &amp; MOONMB) {
         // oops! we were looking at the Moon
         ...
      }
      \end{verbatim}

      Related constants:

      SKYBEAMS,
      EARTH1,
      MOON1,
      GALAX1,
      SUN1,
      EARTH2,
      MOON2,
      GALAX2,
      SUN2,
      EARTHMB,
      MOONMB,
      JUPITERMB,
      SATURNMB

      The information is calculated based on the satellite's attitude
      and a low precision ephemeris for the Sun and Moon. The zone of
      avoidance for the galactic plane is based on the map of integrated
      CO 1-0 emission from the Columbia survey.
  */
  short  SkyBeamHit;

  /** Right ascension.
      The right ascension (J2000) of the source (direction of pointing)
      in degrees. Calculated from the quaternions stored in columns
      11-14 of block 3 of an attitude file.
   */
  float  RA2000;

  /** Declination.
      The declination (J2000) of the source (direction of pointing)
      in degrees. Calculated from the quaternions stored in columns
      11-14 of block 3 of an attitude file.
   */
  float  Dec2000;

  /** Source velocity.
      For astronomy, the velocity of source with respect to the local
      standard of rest (LSR) in m/s. It is extracted from the
      unvalidated time line (block 1) of an attitude file. The source
      velocity will be stored (with a {\tt %7.1f} format)
      in comment fields enclosed in square brackets on the same line as
      the {\tt newompb} keyword. Example:
      \begin{verbatim}
      newompb:   0 [CALOBS W3(OH)             -45.0 POS        ]
                                            -------
      \end{verbatim}
      For aeronomy, the relative velocity in m/s between the satellite
      and the tangent point. This is calulated from the satellites GPS
      velocity stored in columns 21-23 of block 3 of an attitude file.
  */
  float  VSource;               

  /** Tangent point or map position.
      The union holds two structures, each consisting of three 
      {\tt float} values with different meanings in astronomy and aeronomy.

      In astronomy the union holds a
      structure {\tt map}, describing map parameters in case the current
      spectrum is part of a mapping observation:
      \begin{verbatim}
      float Xoff // the map offset in the x-direction in degrees.
      float Yoff // the map offset in the x-direction in degrees.
      float Tilt // the position angle of the map in degrees.
      \end{verbatim}

      In aeronomy the union holds a structure {\tt tp}, describing the
      current tangent point:
      \begin{verbatim}
      float Longitude // geographical longitude of the tangent point in degrees.
      float Latitude  // geographical latitude of the tangent point in degrees.
      float Altitude  // geographical altitude of the tangent point in meter.
      \end{verbatim}

      Code to access the various members would look like this:
      \begin{verbatim}
      scan->u.tp.Longitude
      scan->u.map.Yoff
      \end{verbatim}

      The information on the map position is stored in block 1 of an
      attitude file. The map position will be stored 
      in comment fields enclosed in square brackets on the same line as
      the {\tt newompb} keyword. Example:
      \begin{verbatim}
      newompb:   0 [CALOBS W3(OH)             -45.0 MAP    60  -120  180.0]
                                                        ----- ----- ------
      \end{verbatim}
      The information on the tangent point is stored in columns 27-29 of
      block 3 of an attitude file.
  */
  point u;

  /** Reference attitude.
      The reference attitude given as a quaternion (4-vector) as listed in
      columns 8-10 of block 3 of an attitude file.
  */
  double Qtarget[4];

  /** Achieved attitude.
      The achieved attitude given as a quaternion (4-vector) as listed in
      columns 11-14 of block 3 of an attitude file.
  */
  double Qachieved[4];

  /** Attitude error.
      The attitude error in degrees, describing the pointing uncertainty
      around the 3 principle axes of the satellite.
      Listed in columns 15-17 of block 3 of an attitude file.
  */
  double Qerror[3];

  /** Geocentric position.
      The geocentric position $X$,$Y$,$Z$ in meter of the satellite at the 
      end of the observation as reported by the GPS receiver onboard 
      of Odin and as listed in columns 18-20 of block 3 of an attitude file.
  */
  double GPSpos[3];

  /** Geocentric velocity.
      The geocentric velocity $\dot X$, $\dot Y$, $\dot Z$ in meter per 
      second of the satellite at the end of the observation as reported 
      by the GPS receiver onboard of Odin and as listed in columns 21-23 
      of block 3 of an attitude file.
   */
  double GPSvel[3];

  /** Geocentric position of Sun.
      The geocentric position of the Sun in meter. Calculated based on
      current time using a low precision ephemeris. Accuracy is guarantted 
      to be one arcmin or better.
   */
  double SunPos[3];

  /** Geocentric position of Moon.
      The geocentric position of the Moon in meter. Calculated based on
      current time using a low precision ephemeris. Accuracy is guarantted 
      to be one arcmin or better.
  */
  double MoonPos[3];

  /** Solar zenith angle.
      The solar zenith angle in degrees. Calculated from the position of
      the Sun and therefore with the same intrinsic accuracy.
   */
  float  SunZD;

  /** Velocity with respect to the Earth.
      The velocity of the satellite with respect to the Earth in
      meter per second. Calculated from current time and satellite 
      GPS position and velocity.
  */
  float  Vgeo;

  /** Velocity with respect to LSR.
      In astronomy, the velocity of the satellite with respect to the
      local standard of rest (LSR) in meter per second. Calculated from
      current time and satellite GPS position and velocity.
      Not used by aeronomy.
  */
  float  Vlsr;

  /** Calbration temperature.
      The Rayleigh Jeans temperature in Kelvin of the calibration load
      used during the intensity calibration of this spectrum. The physical
      temperature of the load is retrieved from level 0 house keeping
      data files.
  */
  float  Tcal;

  /** System temperature.
      The mean value of the system temperature in Kelvin
      used during the intensity calibration of this spectrum.
      Calculated during calibration.
  */
  float  Tsys;

  /** Diplexer path length.
      The path length in meter of the SSB diplexer. Extracted from house
      keeping data files.
  */
  float  SBpath;

  /** Local oscillator frequency.
      The (first) local oscillator frequency in Hz used for this observation
      in the rest frame of the satellite. Calculated from the information
      on HRO and PRO frequency stored in level 0 house keeping data files.
  */
  double LOFreq;

  /** Sky frequency.
      The frequency in Hz of the centre channel in the rest frame of the
      satellite, i.e not Doppler corrected. Calculated from the LO frequency
      and the value of the IF frequency, which will be either +3900 MHz or
      -3900 MHz. The sign is calculated based on the setting of the SSB
      tuning mechanism, stored in the level 0 house keeping data files.

      Note that the index of the centre channel {\tt cc} is defined as
      \begin{verbatim}
      cc = spectrum->Channels/2;
      \end{verbatim}
      with all the implications of an integer division. This means, that
      for an odd number of channels, the division yields the correct array
      index of the centre channel, for an even number of channels we get
      the index of the channel just above the centre of the band (which falls
      between two channels).
  */
  double SkyFreq;

  /** Rest frequency.
      The frequency in Hz of the centre channel in the rest frame of the
      observed object, i.e Doppler corrected. The same definition of
      centre channel applies as for the previous member. Calculated from
      sky frequency and applicable Doppler correction (see Vlsr).

      For astronomy, the LSR velocity of the source supplied via block 1 
      of an attitude file together with the calculated LSR velocity of 
      the satellite is used to translate the sky frequency to a rest frame
      centred on the source.

      For aeronomy, the relative velocity of the satellite in the direction
      of the tangent point is used to translate the sky frequency to an 
      earth fixed reference frame.

      The frequency correction is calculated using the radio convention:
      \begin{verbatim}
      SkyFreq = RestFreq*(1 - v/c)
      \end{verbatim}
  */
  double RestFreq;

  /** Frequency of max.suppression.
      The sideband frequency in Hz corresponding to the frequency of
      maximum suppression in the rest frame of the satellite,
      i.e not Doppler corrected.
  */
  double MaxSuppression;

  /** Soda version.
      The version number of the attitude reconstruction software (SODA) 
      used at SSC during production of attitiude files.
   */
  double SodaVersion;

  /** Frequency resolution.
      The spacing in Hz between neighbouring channels for this spectrum.
  */
  double FreqRes;

  /** Frequency calibration coefficients.
      Frequency calibration coefficients in Hz for the spectrometer in
      use.

      For the correlators, these are the four local oscillator frequencies
      of the SSB modules, in the order in which individual bands are read
      out from the instrument, i.e. 1,3,2,4 when compared to Omnisys
      documentation.
      Typical values:
      \begin{verbatim}
      FreqCal[0] =  3600.0e+06;
      FreqCal[1] =  3800.0e+06;
      FreqCal[2] =  4000.0e+06;
      FreqCal[3] =  4200.0e+06;
      \end{verbatim}

      For the acousto optic spectrometer, these are the fit results from the
      last frequency comb measurement performed.
      Typical values:
      \begin{verbatim}
      FreqCal[0] =  2.100e+09;
      FreqCal[1] =  6.200e+05;
      FreqCal[2] =  4.000e+00;
      FreqCal[3] = -1.000e-02;
      \end{verbatim}
  */
  double FreqCal[4];

  /** Backend mode.
      The integration mode of the backend during the observation.

      Related constants 

      AC_XHIRES,
      AC_HIRES,
      AC_MEDRES,
      AC_LOWRES,
      AC_YHIRES,
      AC_SPLIT,
      AC_UPPER,
      ADC_SEQ,
      AOS_LONG,
      AOS_SHORT,
      AOS_HALF,
      AOS_FOUR,
      AOS_CENTRE,
      AOS_WINGS,
      AOS_WINDOW

      The information is retrieved from values reported by the
      spectrometers.
  */
  int    IntMode;

  /** Integration time.
      The integration time in seconds, i.e. the duration of this
      observation.
   */
  float  IntTime;

  /** Effective integration time.
      The effective integration time in seconds, i.e. you will get the
      noise level in this spectrum by using this time and the system
      temperature from above and the usual radiometer formula:
      \begin{verbatim}
      dT = Tsys/sqrt(df * EffTime)
      \end{verbatim}
      where {\tt df} is the noise bandwidth of the spectrometer.
  */
  float  EffTime;

  /** Number of channels.
      The number of channels in this spectrum. The maximum number
      of channels that may occur is given by the number of channels in
      the AOS, see below.
  */
  int    Channels;

  /** Channel data.
      The array holding the actual spectrum. To loop through the
      array use:
      \begin{verbatim}
      int channel;
       
      for (channel = 0; channel < spectrum->Channels; channel++) {
         // process each channel
         spectrum->data[channel] = ...;
      }
      \end{verbatim}

      Related constant:

      MAXCHANNELS
  */
  float  data[MAXCHANNELS];
};
//@}

#endif
