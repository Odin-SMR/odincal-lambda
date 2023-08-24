#include <string.h>
#include <ctype.h>

#include "odinscan.h"

typedef struct OdinScan Scan;

#include "accessor.h"

static char rcsid[] = "$Id$";

int Version(Scan *s)
{
  return (int)s->Version;
}

void setVersion(Scan *s, int version)
{
  s->Version = (unsigned short)version;
}

int Level(Scan *s)
{
  return (int)s->Level;
}

void setLevel(Scan *s, int level)
{
  s->Level = (unsigned short)level;
}

int Quality(Scan *s)
{
  return (int)s->Quality;
}

void setQuality(Scan *s, int quality)
{
  s->Quality = (unsigned long)quality;
}

int STW(Scan *s)
{
  return (int)s->STW;
}

void setSTW(Scan *s, int stw)
{
  s->STW = (unsigned long)stw;
}

double MJD(Scan *s)
{
  return s->MJD;
}

void setMJD(Scan *s, double mjd)
{
  s->MJD = mjd;
}

double Orbit(Scan *s)
{
  return s->Orbit;
}

void setOrbit(Scan *s, double orbit)
{
  s->Orbit = orbit;
}

double LST(Scan *s)
{
  return (double)s->LST;
}

void setLST(Scan *s, double lst)
{
  s->LST = (float)lst;
}

static char sourcename[SOURCENAMELEN+1];

char *Source(Scan *s)
{
  static char *ptr;
  int i;

  memset(sourcename, 0, SOURCENAMELEN+1);
  ptr = s->Source;
  for (i = 0; i < SOURCENAMELEN; i++) {
    if (isprint(*ptr)) sourcename[i] = *ptr++;
    else {
      sourcename[i] = '\0';
      break;
    }
  }
  ptr = sourcename;
  return ptr;
}

void setSource(Scan *s, char *source)
{
  static char *ptr;
  int i;

  memset(s->Source, 0, SOURCENAMELEN);
  ptr = source;
  for (i = 0; i < SOURCENAMELEN; i++) {
    if (isprint(*ptr)) s->Source[i] = *ptr++;
    else {
      s->Source[i] = '\0';
      break;
    }
  }
}

char *Discipline(Scan *s)
{
  static char *ptr;

  switch(s->Discipline) {
   case AERO:  ptr = "AERO"; break;
   case ASTRO: ptr = "ASTR"; break;
   default:    ptr = "    "; break;
  }
  return ptr;
}

void setDiscipline(Scan *s, char *disc)
{
  if (strncmp(disc, "AERO", 4) == 0) s->Discipline = AERO;
  else if (strncmp(disc, "ASTR", 4) == 0) s->Discipline = ASTRO;
  else s->Discipline = UNDEFINED;
}

char *Topic(Scan *s)
{
  static char *ptr;

  ptr = "      ";   /* default */
  if (s->Discipline == ASTRO) {
    switch(s->Topic) {
     case SOLSYS:      ptr = "SOLSYS";      break;
     case STARS:       ptr = "STARS ";      break;
     case EXTGAL:      ptr = "EXTGAL";      break;
     case LMC:         ptr = "LMC   ";      break;
     case PRIMOL:      ptr = "PRIMOL";      break;
     case SPECTR:      ptr = "SPECTR";      break;
     case CHEM:        ptr = "CHEM  ";      break;
     case GPLANE:      ptr = "GPLANE";      break;
     case GCENTR:      ptr = "GCENTR";      break;
     case GMC:         ptr = "GMC   ";      break;
     case SFORM:       ptr = "SFORM ";      break;
     case DCLOUD:      ptr = "DCLOUD";      break;
     case SHOCKS:      ptr = "SHOCKS";      break;
     case PDR:         ptr = "PDR   ";      break;
     case HILAT:       ptr = "HILAT ";      break;
     case ABSORB:      ptr = "ABSORB";      break;
     case ORION:       ptr = "ORION ";      break;
     case CALOBS:      ptr = "CALOBS";      break;
     case COMMIS:      ptr = "COMMIS";      break;
    }
  }
  if (s->Discipline == AERO) {
    switch(s->Topic) {
     case  STRAT:      ptr = "STRAT ";      break;
     case  ODD_N:      ptr = "ODD_N ";      break;
     case  ODD_H:      ptr = "ODD_H ";      break;
     case  WATER:      ptr = "WATER ";      break;
     case  SUMMER:     ptr = "SUMMER";      break;
     case  DYNA:       ptr = "DYNA  ";      break;
    }
  }
  return ptr;
}

void setTopic(Scan *s, char *topic)
{
  if (strncmp(topic, "SOLSYS", 6) == 0) s->Topic = SOLSYS;
  else if (strncmp(topic, "STARS",  5) == 0) s->Topic = STARS;
  else if (strncmp(topic, "EXTGAL", 6) == 0) s->Topic = EXTGAL;
  else if (strncmp(topic, "LMC",    3) == 0) s->Topic = LMC;
  else if (strncmp(topic, "PRIMOL", 6) == 0) s->Topic = PRIMOL;
  else if (strncmp(topic, "SPECTR", 6) == 0) s->Topic = SPECTR;
  else if (strncmp(topic, "CHEM",   4) == 0) s->Topic = CHEM ;
  else if (strncmp(topic, "GPLANE", 6) == 0) s->Topic = GPLANE;
  else if (strncmp(topic, "GCENTR", 6) == 0) s->Topic = GCENTR;
  else if (strncmp(topic, "GMC",    3) == 0) s->Topic = GMC;
  else if (strncmp(topic, "SFORM",  5) == 0) s->Topic = SFORM;
  else if (strncmp(topic, "DCLOUD", 6) == 0) s->Topic = DCLOUD;
  else if (strncmp(topic, "SHOCKS", 6) == 0) s->Topic = SHOCKS;
  else if (strncmp(topic, "PDR",    3) == 0) s->Topic = PDR;
  else if (strncmp(topic, "HILAT",  5) == 0) s->Topic = HILAT;
  else if (strncmp(topic, "ABSORB", 6) == 0) s->Topic = ABSORB;
  else if (strncmp(topic, "ORION",  5) == 0) s->Topic = ORION;
  else if (strncmp(topic, "CALOBS", 6) == 0) s->Topic = CALOBS;
  else if (strncmp(topic, "COMMIS", 6) == 0) s->Topic = COMMIS;

  else if (strncmp(topic, "STRAT" , 5) == 0) s->Topic = STRAT;
  else if (strncmp(topic, "ODD_N" , 5) == 0) s->Topic = ODD_N;
  else if (strncmp(topic, "ODD_H" , 5) == 0) s->Topic = ODD_H;
  else if (strncmp(topic, "WATER" , 5) == 0) s->Topic = WATER;
  else if (strncmp(topic, "SUMMER", 6) == 0) s->Topic = SUMMER;
  else if (strncmp(topic, "DYNA"  , 4) == 0) s->Topic = DYNA;

  else s->Topic = UNDEFINED;
}

int Spectrum(Scan *s)
{
  return s->Spectrum;
}

void setSpectrum(Scan *s, int sp)
{
  s->Spectrum = sp;
}

char *ObsMode(Scan *s)
{
  static char *ptr;

  switch(s->ObsMode) {
   case TPW:    ptr = "TPW";    break;
   case SSW:    ptr = "SSW";    break;
   case LSW:    ptr = "LSW";    break;
   case FSW:    ptr = "FSW";    break;
   default:     ptr = "   ";    break;
  }
  return ptr;
}

void setObsMode(Scan *s, char *obsmode)
{
  if (strncmp(obsmode, "TPW", 3) == 0) s->ObsMode = TPW;
  else if (strncmp(obsmode, "SSW", 3) == 0) s->ObsMode = SSW;
  else if (strncmp(obsmode, "LSW", 3) == 0) s->ObsMode = LSW;
  else if (strncmp(obsmode, "FSW", 3) == 0) s->ObsMode = FSW;
  else s->ObsMode = UNDEFINED;
}

char *Type(Scan *s)
{
  static char *ptr;

  switch(s->Type) {
   case SIG:    ptr = "SIG";    break;
   case REF:    ptr = "REF";    break;
   case CAL:    ptr = "CAL";    break;
   case CMB:    ptr = "CMB";    break;
   case DRK:    ptr = "DRK";    break;
   case SK1:    ptr = "SK1";    break;
   case SK2:    ptr = "SK2";    break;
   case SPE:    ptr = "SPE";    break;
   case SSB:    ptr = "SSB";    break;
   case AVE:    ptr = "AVE";    break;
   default:     ptr = "   ";    break;
  }
  return ptr;
}

void setType(Scan *s, char *type)
{
  if (strncmp(type, "SIG", 3) == 0) s->Type = SIG;
  else if (strncmp(type, "SK1", 3) == 0) s->Type = SK1;
  else if (strncmp(type, "SK2", 3) == 0) s->Type = SK2;
  else if (strncmp(type, "REF", 3) == 0) s->Type = REF;
  else if (strncmp(type, "CAL", 3) == 0) s->Type = CAL;
  else if (strncmp(type, "CMB", 3) == 0) s->Type = CMB;
  else if (strncmp(type, "DRK", 3) == 0) s->Type = DRK;
  else if (strncmp(type, "SPE", 3) == 0) s->Type = SPE;
  else if (strncmp(type, "SSB", 3) == 0) s->Type = SSB;
  else if (strncmp(type, "AVE", 3) == 0) s->Type = AVE;
  else s->Type = UNDEFINED;
}

char *Frontend(Scan *s)
{
  static char *ptr;

  switch(s->Frontend) {
   case REC_555:    ptr = "555";    break;
   case REC_495:    ptr = "495";    break;
   case REC_572:    ptr = "572";    break;
   case REC_549:    ptr = "549";    break;
   case REC_119:    ptr = "119";    break;
   case REC_SPLIT:  ptr = "SPL";    break;
   default:         ptr = "   ";    break;
  }
  return ptr;
}

void setFrontend(Scan *s, char *frontend)
{
  if (strncmp(frontend, "555", 3) == 0) s->Frontend = REC_555;
  else if (strncmp(frontend, "495", 3) == 0) s->Frontend = REC_495;
  else if (strncmp(frontend, "572", 3) == 0) s->Frontend = REC_572;
  else if (strncmp(frontend, "549", 3) == 0) s->Frontend = REC_549;
  else if (strncmp(frontend, "119", 3) == 0) s->Frontend = REC_119;
  else if (strncmp(frontend, "SPL", 3) == 0) s->Frontend = REC_SPLIT;
  else s->Frontend = UNDEFINED;
}

char *Backend(Scan *s)
{
  static char *ptr;

  switch(s->Backend) {
   case AC1:    ptr = "AC1";    break;
   case AC2:    ptr = "AC2";    break;
   case AOS:    ptr = "AOS";    break;
   case FBA:    ptr = "FBA";    break;
   default:     ptr = "   ";    break;
  }
  return ptr;
}

void setBackend(Scan *s, char *backend)
{
  if (strncmp(backend, "AC1", 3) == 0) s->Backend = AC1;
  else if (strncmp(backend, "AC2", 3) == 0) s->Backend = AC2;
  else if (strncmp(backend, "AOS", 3) == 0) s->Backend = AOS;
  else if (strncmp(backend, "FBA", 3) == 0) s->Backend = FBA;
  else s->Backend = UNDEFINED;
}

int SkyBeamHit(Scan *s)
{
  return (int)s->SkyBeamHit;
}

void setSkyBeamHit(Scan *s, int hit)
{
  s->SkyBeamHit = (short)hit;
}

double RA2000(Scan *s)
{
  return (double)s->RA2000;
}

void setRA2000(Scan *s, double lon)
{
  s->RA2000 = (float)lon;
}

double Dec2000(Scan *s)
{
  return (double)s->Dec2000;
}

void setDec2000(Scan *s, double lat)
{
  s->Dec2000 = (float)lat;
}

double VSource(Scan *s)
{
  return (double)s->VSource;
}

void setVSource(Scan *s, double vel)
{
  s->VSource = (float)vel;
}

double Longitude(Scan *s)
{
  return (double)s->u.tp.Longitude;
}

void setLongitude(Scan *s, double lon)
{
  s->u.tp.Longitude = (float)lon;
}

double Latitude(Scan *s)
{
  return (double)s->u.tp.Latitude;
}

void setLatitude(Scan *s, double lat)
{
  s->u.tp.Latitude = (float)lat;
}

double Altitude(Scan *s)
{
  return (double)s->u.tp.Altitude;
}

void setAltitude(Scan *s, double alt)
{
  s->u.tp.Altitude = (float)alt;
}

double Xoff(Scan *s)
{
  return (double)s->u.map.Xoff;
}

void setXoff(Scan *s, double xoff)
{
  s->u.map.Xoff = (float)xoff;
}

double Yoff(Scan *s)
{
  return (double)s->u.map.Yoff;
}

void setYoff(Scan *s, double yoff)
{
  s->u.map.Yoff = (float)yoff;
}

double Tilt(Scan *s)
{
  return (double)s->u.map.Tilt;
}

void setTilt(Scan *s, double tilt)
{
  s->u.map.Tilt = (float)tilt;
}

double Qtarget(Scan *s, int i)
{
  if (i >= 0 && i < 4) return s->Qtarget[i];
  else                 return 0.0;
}

void setQtarget(Scan *s, int i, double q)
{
  if (i >= 0 && i < 4) s->Qtarget[i] = q;
}

double Qachieved(Scan *s, int i)
{
  if (i >= 0 && i < 4) return s->Qachieved[i];
  else                 return 0.0;
}

void setQachieved(Scan *s, int i, double q)
{
  if (i >= 0 && i < 4) s->Qachieved[i] = q;
}

double Qerror(Scan *s, int i)
{
  if (i >= 0 && i < 3) return s->Qerror[i];
  else                 return 0.0;
}

void setQerror(Scan *s, int i, double q)
{
  if (i >= 0 && i < 3) s->Qerror[i] = q;
}

double GPSpos(Scan *s, int i)
{
  if (i >= 0 && i < 3) return s->GPSpos[i];
  else                 return 0.0;
}

void setGPSpos(Scan *s, int i, double q)
{
  if (i >= 0 && i < 3) s->GPSpos[i] = q;
}

double GPSvel(Scan *s, int i)
{
  if (i >= 0 && i < 3) return s->GPSvel[i];
  else                 return 0.0;
}

void setGPSvel(Scan *s, int i, double q)
{
  if (i >= 0 && i < 3) s->GPSvel[i] = q;
}

double SunPos(Scan *s, int i)
{
  if (i >= 0 && i < 3) return s->SunPos[i];
  else                 return 0.0;
}

void setSunPos(Scan *s, int i, double q)
{
  if (i >= 0 && i < 3) s->SunPos[i] = q;
}

double MoonPos(Scan *s, int i)
{
  if (i >= 0 && i < 3) return s->MoonPos[i];
  else                 return 0.0;
}

void setMoonPos(Scan *s, int i, double q)
{
  if (i >= 0 && i < 3) s->MoonPos[i] = q;
}

double SunZD(Scan *s)
{
  return (double)s->SunZD;
}

void setSunZD(Scan *s, double sunzd)
{
  s->SunZD = (float)sunzd;
}

double Vgeo(Scan *s)
{
  return (double)s->Vgeo;
}

void setVgeo(Scan *s, double vgeo)
{
  s->Vgeo = (float)vgeo;
}

double Vlsr(Scan *s)
{
  return (double)s->Vlsr;
}

void setVlsr(Scan *s, double vlsr)
{
  s->Vlsr = (float)vlsr;
}

double Tcal(Scan *s)
{
  return (double)s->Tcal;
}

void setTcal(Scan *s, double tcal)
{
  s->Tcal = (float)tcal;
}

double Tsys(Scan *s)
{
  return (double)s->Tsys;
}

void setTsys(Scan *s, double tsys)
{
  s->Tsys = (float)tsys;
}

double SBpath(Scan *s)
{
  return (double)s->SBpath;
}

void setSBpath(Scan *s, double sbpath)
{
  s->SBpath = (float)sbpath;
}

double LOFreq(Scan *s)
{
  return s->LOFreq;
}

void setLOFreq(Scan *s, double f)
{
  s->LOFreq = f;
}

double SkyFreq(Scan *s)
{
  return s->SkyFreq;
}

void setSkyFreq(Scan *s, double f)
{
  s->SkyFreq = f;
}

double RestFreq(Scan *s)
{
  return s->RestFreq;
}

void setRestFreq(Scan *s, double f)
{
  s->RestFreq = f;
}

double MaxSuppression(Scan *s)
{
  return s->MaxSuppression;
}

void setMaxSuppression(Scan *s, double f)
{
  s->MaxSuppression = f;
}

double SodaVersion(Scan *s)
{
  return s->SodaVersion;
}

void setSodaVersion(Scan *s, double v)
{
  s->SodaVersion = v;
}

double FreqRes(Scan *s)
{
  return s->FreqRes;
}

void setFreqRes(Scan *s, double f)
{
  s->FreqRes = f;
}

double FreqCal(Scan *s, int i)
{
  if (i >= 0 && i < 4) return s->FreqCal[i];
  else                 return 0.0;
}

void setFreqCal(Scan *s, int i, double q)
{
  if (i >= 0 && i < 4) s->FreqCal[i] = q;
}

int IntMode(Scan *s)
{
  return s->IntMode;
}

void setIntMode(Scan *s, int m)
{
  s->IntMode = m;
}

double IntTime(Scan *s)
{
  return (double)s->IntTime;
}

void setIntTime(Scan *s, double t)
{
  s->IntTime = (float)t;
}

double EffTime(Scan *s)
{
  return (double)s->EffTime;
}

void setEffTime(Scan *s, double t)
{
  s->EffTime = (float)t;
}

int Channels(Scan *s)
{
  return s->Channels;
}

void setChannels(Scan *s, int n)
{
  s->Channels = n;
}

double Data(Scan *s, int i)
{
  if (i >= 0 && i < s->Channels) return (double)s->data[i];
  else                           return 0.0;
}

void setData(Scan *s, int i, double v)
{
  if (i >= 0 && i < s->Channels) s->data[i] = (float)v;
}
