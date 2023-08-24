#ifndef ODINMODULE_H
#define ODINMODULE_H

#define PyOdin_API_pointers 1
#define PyOdin_Spectrum_NUM 0

typedef struct {
    PyObject_HEAD
    unsigned short Version;
    unsigned short Level;
    unsigned long Quality;
    unsigned long STW;
    double MJD;
    double Orbit;
    float  LST;
    PyObject *Source;
    PyObject *Discipline;
    PyObject *Topic;
    short  Spectrum;
    PyObject *ObsMode;
    PyObject *Type;
    PyObject *Frontend;
    PyObject *Backend;
    short  SkyBeamHit;
    float  RA2000;
    float  Dec2000;
    float  VSource;               
    float  Longitude;
    float  Latitude;
    float  Altitude;
    PyObject *Qtarget;
    PyObject *Qachieved;
    PyObject *Qerror;
    PyObject *GPSpos;
    PyObject *GPSvel;
    PyObject *SunPos;
    PyObject *MoonPos;
    float  SunZD;
    float  Vgeo;
    float  Vlsr;
    float  Tcal;
    float  Tsys;
    float  SBpath;
    double LOFreq;
    double SkyFreq;
    double RestFreq;
    double MaxSup;
    double SodaVers;
    double FreqRes;
    PyObject *FreqCal;
    int    IntMode;
    float  IntTime;
    float  EffTime;
    int    Channels;
    PyArrayObject *Data;
} PoOdinScan;

#endif
