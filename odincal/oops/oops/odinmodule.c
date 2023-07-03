#include "Python.h"
#include "structmember.h"
#include "numpy/arrayobject.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "odinmodule.h"

#include "odinlib.h"
#include "accessor.h"
#include "level0.h"
#include "fbalib.h"
#include "aoslib.h"
#include "aclib.h"
#include "odintime.h"
#include "astrolib.h"
#include "drplib.h"
#include "recipes.h"

#define SOURCENAMELEN    32                  
#define MAXCHANNELS    1728                  

static struct OdinScan s, t, r;
static int speccount = 0;

/************************* OdinScan **************************/

static void headC2Py(struct OdinScan *s, PoOdinScan *obj)
{
    int i;

    obj->Version   = s->Version;
    obj->Level     = s->Level;
    obj->Quality   = s->Quality;
    obj->STW       = s->STW;
    obj->MJD       = s->MJD;
    obj->Orbit     = s->Orbit;
    obj->LST       = s->LST;
    obj->Source    = PyString_FromString(Source(s));
    obj->Discipline= PyString_FromString(Discipline(s));
    obj->Topic     = PyString_FromString(Topic(s));
    obj->Spectrum  = s->Spectrum;
    obj->ObsMode   = PyString_FromString(ObsMode(s));
    obj->Type      = PyString_FromString(Type(s));
    obj->Frontend  = PyString_FromString(Frontend(s));
    obj->Backend   = PyString_FromString(Backend(s));
    obj->SkyBeamHit= s->SkyBeamHit;
    obj->RA2000    = s->RA2000;
    obj->Dec2000   = s->Dec2000;
    obj->VSource   = s->VSource;               
    obj->Longitude = s->u.tp.Longitude;
    obj->Latitude  = s->u.tp.Latitude;
    obj->Altitude  = s->u.tp.Altitude;
    obj->SunZD     = s->SunZD;
    obj->Vgeo      = s->Vgeo;
    obj->Vlsr      = s->Vlsr;
    obj->Tcal      = s->Tcal;
    obj->Tsys      = s->Tsys;
    obj->SBpath    = s->SBpath;
    obj->LOFreq    = s->LOFreq;
    obj->SkyFreq   = s->SkyFreq;
    obj->RestFreq  = s->RestFreq;
    obj->MaxSup    = s->MaxSuppression;
    obj->SodaVers  = s->SodaVersion;
    obj->FreqRes   = s->FreqRes;
    obj->IntMode   = s->IntMode;
    obj->IntTime   = s->IntTime;
    obj->EffTime   = s->EffTime;
    obj->Channels  = s->Channels;
    for (i = 0; i < 4; i++) {
	PyList_SetItem(obj->Qtarget,   i, PyFloat_FromDouble(s->Qtarget[i]));
	PyList_SetItem(obj->Qachieved, i, PyFloat_FromDouble(s->Qachieved[i]));
	PyList_SetItem(obj->FreqCal,   i, PyFloat_FromDouble(s->FreqCal[i]));
    }
    for (i = 0; i < 3; i++) {
	PyList_SetItem(obj->Qerror,  i, PyFloat_FromDouble(s->Qerror[i]));
	PyList_SetItem(obj->GPSpos,  i, PyFloat_FromDouble(s->GPSpos[i]));
	PyList_SetItem(obj->GPSvel,  i, PyFloat_FromDouble(s->GPSvel[i]));
	PyList_SetItem(obj->SunPos,  i, PyFloat_FromDouble(s->SunPos[i]));
	PyList_SetItem(obj->MoonPos, i, PyFloat_FromDouble(s->MoonPos[i]));
    }
}

static int dataC2Py(struct OdinScan *s, PoOdinScan *obj)
{
    double *d;
    int i;
    int dimensions[1];
    PyArrayObject *array;

    
/*      printf("C -> Python: %08lX (%d) %d, %d [%d:%d]\n", */
/*  	   obj->Data, obj->Data->ob_refcnt, */
/*  	   obj->Data->nd, obj->Data->dimensions[0], */
/*  	   obj->Data->descr->type_num, */
/*  	   PyArray_ISCONTIGUOUS(obj->Data)); */
    
    array = obj->Data;
    if (array->nd != 1) {
/*  	printf("data not in vector form\n"); */
	dimensions[0] = s->Channels;
	array = (PyArrayObject *)PyArray_FromDims(1, dimensions, PyArray_DOUBLE);
	if (array == NULL) {
	    PyErr_SetString(PyExc_RuntimeError, "can't create new vector");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }
    if (s->Channels != array->dimensions[0]) {
/*  	printf("data vector has wrong length\n"); */
	dimensions[0] = s->Channels;
	array = (PyArrayObject *)PyArray_FromDims(1, dimensions, PyArray_DOUBLE);
	if (array == NULL) {
	    PyErr_SetString(PyExc_RuntimeError, "can't resize");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }
    if (!PyArray_ISCONTIGUOUS(array)) {
/*  	printf("data not contiguous\n"); */
	array = (PyArrayObject *)
	    PyArray_ContiguousFromObject((PyObject *)obj->Data, PyArray_DOUBLE, 1, 1);
	if (array == NULL) {
/*  	    printf("can't create contiguous vector\n"); */
	    PyErr_SetString(PyExc_RuntimeError, "can't create contiguous vector");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }
    if (array->descr->type_num != PyArray_DOUBLE) {
/*  	printf("data is not a vector of doubles\n"); */
	array = (PyArrayObject *)
	    PyArray_ContiguousFromObject((PyObject *)obj->Data, PyArray_DOUBLE, 1, 1);
	if (array == NULL) {
	    PyErr_SetString(PyExc_RuntimeError, "can't create vector of doubles");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }
    obj->Channels = s->Channels;
    d = (double *)array->data;
/*      printf("dataC2Py [%08X]: channels: %d\n", d, s->Channels); */
/*      printf("C -> Python: %08lX (%d) %d, %d [%d:%d]\n", */
/*    	   array, array->ob_refcnt, */
/*    	   array->nd, array->dimensions[0], */
/*    	   array->descr->type_num, */
/*    	   PyArray_ISCONTIGUOUS(array)); */
    for (i = 0; i < s->Channels; i++) {
	// d[i] = 0.0;
	d[i] = s->data[i];
    }
/*      printf("dataC2Py: %08X =?= %08X\n", array, obj->Data); */

    if (array != obj->Data) obj->Data = array;
    /* printf("C -> Python: %08lX (%d)\n", obj->Data, obj->Data->ob_refcnt); */
    return 1;
}

static void headPy2C(PoOdinScan *obj, struct OdinScan *s)
{
    int i;

    s->Version        = obj->Version;
    s->Level          = obj->Level;
    s->Quality        = obj->Quality;
    s->STW            = obj->STW;
    s->MJD            = obj->MJD;
    s->Orbit          = obj->Orbit;
    s->LST            = obj->LST;
    setSource(s, PyString_AsString(obj->Source));
    setDiscipline(s, PyString_AsString(obj->Discipline));
    setTopic(s, PyString_AsString(obj->Topic));
    s->Spectrum       = obj->Spectrum;
    setObsMode(s, PyString_AsString(obj->ObsMode));
    setType(s, PyString_AsString(obj->Type));
    setFrontend(s, PyString_AsString(obj->Frontend));
    setBackend(s, PyString_AsString(obj->Backend));
    s->SkyBeamHit     = obj->SkyBeamHit;
    s->RA2000         = obj->RA2000;
    s->Dec2000        = obj->Dec2000;
    s->VSource        = obj->VSource;
    s->u.tp.Longitude = obj->Longitude;
    s->u.tp.Latitude  = obj->Latitude;
    s->u.tp.Altitude  = obj->Altitude;
    s->SunZD          = obj->SunZD;
    s->Vgeo           = obj->Vgeo;
    s->Vlsr           = obj->Vlsr;
    s->Tcal           = obj->Tcal;
    s->Tsys           = obj->Tsys;
    s->SBpath         = obj->SBpath;
    s->LOFreq         = obj->LOFreq;
    s->SkyFreq        = obj->SkyFreq;
    s->RestFreq       = obj->RestFreq;
    s->MaxSuppression = obj->MaxSup;
    s->SodaVersion    = obj->SodaVers;
    s->FreqRes        = obj->FreqRes;
    s->IntMode        = obj->IntMode;
    s->IntTime        = obj->IntTime;
    s->EffTime        = obj->EffTime;
    s->Channels       = obj->Channels;

    for (i = 0; i < 4; i++) {
	s->Qtarget[i]   = PyFloat_AsDouble(PyList_GetItem(obj->Qtarget,   i));
	s->Qachieved[i] = PyFloat_AsDouble(PyList_GetItem(obj->Qachieved, i));
	s->FreqCal[i]   = PyFloat_AsDouble(PyList_GetItem(obj->FreqCal,   i));
    }
    for (i = 0; i < 3; i++) {
	s->Qerror[i]  = PyFloat_AsDouble(PyList_GetItem(obj->Qerror,  i));
	s->GPSpos[i]  = PyFloat_AsDouble(PyList_GetItem(obj->GPSpos,  i));
	s->GPSvel[i]  = PyFloat_AsDouble(PyList_GetItem(obj->GPSvel,  i));
	s->SunPos[i]  = PyFloat_AsDouble(PyList_GetItem(obj->SunPos,  i));
	s->MoonPos[i] = PyFloat_AsDouble(PyList_GetItem(obj->MoonPos, i));
    }
}

static int dataPy2C(PoOdinScan *obj, struct OdinScan *s)
{
    PyArrayObject *array;
    double *d;
    int i;

    /*
      printf("Python -> C: %08lX (%d) %d, %d [%d:%d]\n",
      obj->Data, obj->Data->ob_refcnt,
      obj->Data->nd, obj->Data->dimensions[0],
      obj->Data->descr->type_num,
      PyArray_ISCONTIGUOUS(obj->Data));
    */
    s->Channels = obj->Channels;
    array = obj->Data;
    if (!PyArray_ISCONTIGUOUS(array)) {
	/* printf("data not contiguous\n"); */
	array = (PyArrayObject *)
	    PyArray_ContiguousFromObject((PyObject *)obj->Data, PyArray_DOUBLE, 1, 1);
	if (array == NULL) {
	    /* printf("can't create contiguous vector\n"); */
	    PyErr_SetString(PyExc_RuntimeError, "can't create contiguous vector");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }
    if (array->descr->type_num != PyArray_DOUBLE) {
	/* printf("data is not a vector of doubles\n"); */
	array = (PyArrayObject *)
	    PyArray_ContiguousFromObject((PyObject *)obj->Data, PyArray_DOUBLE, 1, 1);
	if (array == NULL) {
	    /* printf("can't create vector of doubles\n"); */
	    PyErr_SetString(PyExc_RuntimeError, "can't create vector of doubles");
	    return 0;
	}
	Py_XDECREF(obj->Data);
    }

    d = (double *)array->data;
    for (i = 0; i < obj->Channels; i++) {
	s->data[i] = d[i];
    }
    if (array != obj->Data) { Py_XDECREF(array); }
    /* printf("Python -> C: %08lX (%d)\n", obj->Data, obj->Data->ob_refcnt); */
    return 1;
}

#define OFF(x) offsetof(PoOdinScan, x)

static struct memberlist spec_memberlist[] = {
    { "version",   T_USHORT, OFF(Version)   },
    { "level",     T_USHORT, OFF(Level)     },
    { "quality",   T_ULONG,  OFF(Quality)   },
    { "stw",       T_ULONG,  OFF(STW)       },
    { "mjd",       T_DOUBLE, OFF(MJD)       },
    { "orbit",     T_DOUBLE, OFF(Orbit)     },
    { "lst",       T_FLOAT,  OFF(LST)       },
    { "source",    T_OBJECT, OFF(Source)    },
    { "discipline",T_OBJECT, OFF(Discipline)},
    { "topic",     T_OBJECT, OFF(Topic)     },
    { "spectrum",  T_SHORT,  OFF(Spectrum)  },
    { "obsmode",   T_OBJECT, OFF(ObsMode)   },
    { "type",      T_OBJECT, OFF(Type)      },
    { "frontend",  T_OBJECT, OFF(Frontend)  },
    { "backend",   T_OBJECT, OFF(Backend)   },
    { "skybeamhit",T_SHORT,  OFF(SkyBeamHit)},
    { "ra2000",    T_FLOAT,  OFF(RA2000)    },
    { "dec2000",   T_FLOAT,  OFF(Dec2000)   },
    { "vsource",   T_FLOAT,  OFF(VSource)   },               
    { "longitude", T_FLOAT,  OFF(Longitude) },
    { "latitude",  T_FLOAT,  OFF(Latitude)  },
    { "altitude",  T_FLOAT,  OFF(Altitude)  },
    { "xoff",      T_FLOAT,  OFF(Longitude) },
    { "yoff",      T_FLOAT,  OFF(Latitude)  },
    { "tilt",      T_FLOAT,  OFF(Altitude)  },
    { "qtarget",   T_OBJECT, OFF(Qtarget)   },
    { "qachieved", T_OBJECT, OFF(Qachieved) },
    { "qerror",    T_OBJECT, OFF(Qerror)    },
    { "gpspos",    T_OBJECT, OFF(GPSpos)    },
    { "gpsvel",    T_OBJECT, OFF(GPSvel)    },
    { "sunpos",    T_OBJECT, OFF(SunPos)    },
    { "moonpos",   T_OBJECT, OFF(MoonPos)   },
    { "sunzd",     T_FLOAT,  OFF(SunZD)     },
    { "vgeo",      T_FLOAT,  OFF(Vgeo)      },
    { "vlsr",      T_FLOAT,  OFF(Vlsr)      },
    { "tcal",      T_FLOAT,  OFF(Tcal)      },
    { "tsys",      T_FLOAT,  OFF(Tsys)      },
    { "sbpath",    T_FLOAT,  OFF(SBpath)    },
    { "lofreq",    T_DOUBLE, OFF(LOFreq)    },
    { "skyfreq",   T_DOUBLE, OFF(SkyFreq)   },
    { "restfreq",  T_DOUBLE, OFF(RestFreq)  },
    { "maxsup",    T_DOUBLE, OFF(MaxSup)    },
    { "soda",      T_DOUBLE, OFF(SodaVers)  },
    { "freqres",   T_DOUBLE, OFF(FreqRes)   },
    { "freqcal",   T_OBJECT, OFF(FreqCal)   },
    { "intmode",   T_INT,    OFF(IntMode)   },
    { "inttime",   T_FLOAT,  OFF(IntTime)   },
    { "efftime",   T_FLOAT,  OFF(EffTime)   },
    { "channels",  T_INT,    OFF(Channels)  },
    { "data",      T_OBJECT, OFF(Data)      },
    { NULL }
};

static PyObject *Load(PoOdinScan *self, PyObject *args)
{
    char *filename;

    if (!PyArg_ParseTuple(args, "s:Load", &filename)) return NULL;

    if (filename) {
	if (!readODINscan(filename, &s)) {
	    PyErr_SetString(PyExc_IOError, "can't read file");
	    return NULL;
	}
	headC2Py(&s, self);
	if (dataC2Py(&s, self) == 0) return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *Save(PoOdinScan *self, PyObject *args)
{
    char *filename = NULL;
    PyObject *obj;

    if (!PyArg_ParseTuple(args, "|s:Save", &filename)) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;
    if (filename == NULL) filename = SaveName(&s);

    if (filename) {
	if (!writeODINscan(filename, &s)) {
	    PyErr_SetString(PyExc_IOError, "can't write file");
	    return NULL;
	}
    }

    obj = PyString_FromString(filename);
    return obj;
}

static double f[MAXCHANNELS];

static PyArrayObject *PoFrequency(PoOdinScan *self, PyObject *args)
{
    PyArrayObject *farray;
    double v, *fdata;
    int i, n, m;
    int dimensions[1];
  
    if (!PyArg_ParseTuple(args, ":Frequency")) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    /* if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL; */

    n = s.Channels;
    if ((m = frequency(&s, f)) != n) {
	PyErr_SetString(PyExc_IOError, "can't extract frequencies");
	return NULL;
    }

    dimensions[0] = n;
    farray = (PyArrayObject *)PyArray_FromDims(1, dimensions, PyArray_DOUBLE);

    if (farray) {
	fdata = (double *)farray->data;
	if (s.SkyFreq >= s.LOFreq) {
	    for (i = 0; i < n; i++) {
		v = f[i];
		v = s.LOFreq + v;
		v /= 1.0e9;
		fdata[i] = v;
	    }
	} else {
	    for (i = 0; i < n; i++) {
		v = f[i];
		v = s.LOFreq - v;
		v /= 1.0e9;
		fdata[i] = v;
	    }
	}
    }

    return farray;
}

static PyObject *PoFreqSort(PoOdinScan *self, PyObject *args)
{
    int n, m;
  
    if (!PyArg_ParseTuple(args, ":FreqSort")) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    m = s.Channels;

    /* printf("before: (%10.2f,%10.2f)\n", 
       s.FreqCal[0]/1.0e6, s.SkyFreq/1.0e6); */

    n = drop(&s, f);
    /* printf("after:  (%10.2f,%10.2f)\n", 
       s.FreqCal[0]/1.0e6, s.SkyFreq/1.0e6); */

    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoRedres(PoOdinScan *self, PyObject *args)
{
    double df;
  
    if (!PyArg_ParseTuple(args, "d:Redres", &df)) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!(s.Quality & ISORTED)) freqsort(&s, f);
    redres(&s, f, df);

    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoFixBands(PoOdinScan *self, PyObject *args)
{
    int i;
  
    if (!PyArg_ParseTuple(args, ":FixBands")) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    i = fixband(&s, f);
    if (i == 0) {
	PyErr_SetString(PyExc_RuntimeError, "can't adjust AC bands");
	return NULL;
    }
    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoGetBand(PoOdinScan *self, PyObject *args)
{
    int i;
    int band, dsp;
  
    dsp = 0;
    if (!PyArg_ParseTuple(args, "i|i:GetBand", &band, &dsp)) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    i = getband(&s, band, dsp);
    if (i == 0) {
	PyErr_SetString(PyExc_RuntimeError, "can't extract AC bands");
	return NULL;
    }
    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoFixMode(PoOdinScan *self, PyObject *args)
{
    int i;
  
    if (!PyArg_ParseTuple(args, ":FixMode")) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    i = newMode(&s);
    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoACS(PoOdinScan *self, PyObject *args)
{
    double theta[3];
    int version;

    version = 0;
    theta[0] = theta[1] = theta[2] = 0.0;

    if (!PyArg_ParseTuple(args, "|iddd:AcsAlign", &version, 
			  &theta[0], &theta[1], &theta[2])) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (version) {
	if (theta[0] == 0.0 && theta[1] == 0.0 && theta[2] == 0.0) {
	    s.Level = (s.Level & 0x00ff) | version;
	} else {
	    setAlignment(version, theta);
	    printf("level: %04X\n", version);
	    printf("theta: %10.6f %10.6f %10.6f\n", 
		   theta[0], theta[1], theta[2]);
	}
    } else {
	/* we just want re-calculation of tangent point */
	s.Level = (s.Level & 0x00ff);
    }
    /* printf("before: %10.4f %10.4f\n", s.RA2000, s.Dec2000); */
    skybeams(&s, 0.0);
    /* printf("after : %10.4f %10.4f\n", s.RA2000, s.Dec2000); */

    headC2Py(&s, self);
    if (dataC2Py(&s, self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
    
}

static PyObject *PoCopy(PoOdinScan *self, PyObject *args);
static PyObject *PoCopyHead(PoOdinScan *self, PyObject *args);
static PyObject *PoCopyData(PoOdinScan *self, PyObject *args);
static PyObject *PoSplit(PoOdinScan *self, PyObject *args);
static PyObject *PoMoment(PoOdinScan *self, PyObject *args);
static PyObject *PoAccum(PoOdinScan *self, PyObject *args);
static PyObject *PoGain(PoOdinScan *self, PyObject *args);
static PyObject *PoCalibrate(PoOdinScan *self, PyObject *args);
static PyObject *PoCalcTsys(PoOdinScan *self, PyObject *args);
static PyObject *PoAttitude(PoOdinScan *self, PyObject *args);
static PyObject *PoFitComb(PoOdinScan *self, PyObject *args);
/* static PyObject *PoFreqCut(PoOdinScan *self, PyObject *args); */

static PyMethodDef spec_functions[] = {
    { "Load",      (PyCFunction)Load,        METH_VARARGS },
    { "Save",      (PyCFunction)Save,        METH_VARARGS },
    { "Copy",      (PyCFunction)PoCopy,      METH_VARARGS },
    { "CopyHead",  (PyCFunction)PoCopyHead,  METH_VARARGS },
    { "CopyData",  (PyCFunction)PoCopyData,  METH_VARARGS },
    { "Split",     (PyCFunction)PoSplit,     METH_VARARGS },
    { "Frequency", (PyCFunction)PoFrequency, METH_VARARGS },
    { "FreqSort",  (PyCFunction)PoFreqSort,  METH_VARARGS },
    { "FixBands",  (PyCFunction)PoFixBands,  METH_VARARGS },
    { "GetBand",   (PyCFunction)PoGetBand,   METH_VARARGS },
    { "FixMode",   (PyCFunction)PoFixMode,   METH_VARARGS },
    { "Moment",    (PyCFunction)PoMoment,    METH_VARARGS },
    { "Accum",     (PyCFunction)PoAccum,     METH_VARARGS },
    { "Gain",      (PyCFunction)PoGain,      METH_VARARGS },
    { "Calibrate", (PyCFunction)PoCalibrate, METH_VARARGS },
    { "CalcTsys",  (PyCFunction)PoCalcTsys,  METH_VARARGS },
    { "Attitude",  (PyCFunction)PoAttitude,  METH_VARARGS },
    { "FitComb",   (PyCFunction)PoFitComb,   METH_VARARGS },
/*    { "FreqCut",   (PyCFunction)PoFreqCut,   METH_VARARGS }, */
    { "Redres",    (PyCFunction)PoRedres,    METH_VARARGS },
    { "AcsAlign",  (PyCFunction)PoACS,       METH_VARARGS },
    { NULL, NULL }
};

static void spec_dealloc(PoOdinScan *self)
{
    if (self) {
  	Py_XDECREF(self->Source);
  	Py_XDECREF(self->Discipline);
  	Py_XDECREF(self->Topic);
  	Py_XDECREF(self->ObsMode);
  	Py_XDECREF(self->Type);
  	Py_XDECREF(self->Frontend);
  	Py_XDECREF(self->Backend);
  	Py_XDECREF(self->Qtarget);
  	Py_XDECREF(self->Qachieved);
  	Py_XDECREF(self->Qerror);
  	Py_XDECREF(self->GPSpos);
  	Py_XDECREF(self->GPSvel);
  	Py_XDECREF(self->SunPos);
  	Py_XDECREF(self->MoonPos);
  	Py_XDECREF(self->FreqCal);
  	Py_XDECREF(self->Data);
	if (speccount > 0) {
	    speccount--;
	    /* printf("spectrum %08X deallocated, new count = %d\n", 
	       self, speccount); */
	} else {
	    printf("delloc: invalid spectrum count = %d\n", speccount);
	}
    }
}

static int spec_print(PoOdinScan *self, FILE *fp, int n)
{
    headPy2C((PoOdinScan *)self, &s);

    PrintScan(&s);

    return 0;
}

static PyObject *spec_getattr(PoOdinScan *self, char *attr)
{
    PyObject *res;

    res = Py_FindMethod(spec_functions, (PyObject *)self, attr);
    if (res != NULL) return res;
    else {
	PyErr_Clear();
	return PyMember_Get((char *)self, spec_memberlist, attr);
    }
}

static int spec_setattr(PoOdinScan *self, char *attr, PyObject *value)
{
    unsigned long stw;
    int n;

    if (value == NULL) {
	PyErr_SetString(PyExc_AttributeError, "can't delete attributes");
	return -1;
    }

    /* kludge to handle stw between 0x80000000 and 0xffffffff */
    if (strcmp(attr, "stw") == 0) {
	/* n = PyLong_CheckExact(value); */
	stw = PyLong_AsUnsignedLong(value);
	/* the following appears to be needed, 
	   even if we set self->STW explicitely in the statement after. */
 	n = PyMember_Set((char *)self, spec_memberlist, attr, value);
	self->STW = stw;
	return n;
    } else return PyMember_Set((char *)self, spec_memberlist, attr, value);
}

PyTypeObject spec_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "Spectrum",
    sizeof(PoOdinScan),
    0,
    (destructor)  spec_dealloc,
    (printfunc)   spec_print,
    (getattrfunc) spec_getattr,
    (setattrfunc) spec_setattr
};

static PoOdinScan *NewPoOdinScan(int n)
{
    PoOdinScan *obj;
    int dimensions[1];

    obj = PyObject_NEW(PoOdinScan, &spec_Type);
    if (obj) {
	obj->Channels = 0;
	if ((obj->Qtarget   = PyList_New(4)) == NULL) return NULL;
	if ((obj->Qachieved = PyList_New(4)) == NULL) return NULL;
	if ((obj->Qerror    = PyList_New(3)) == NULL) return NULL;
	if ((obj->GPSpos    = PyList_New(3)) == NULL) return NULL;
	if ((obj->GPSvel    = PyList_New(3)) == NULL) return NULL;
	if ((obj->SunPos    = PyList_New(3)) == NULL) return NULL;
	if ((obj->MoonPos   = PyList_New(3)) == NULL) return NULL;
	if ((obj->FreqCal   = PyList_New(4)) == NULL) return NULL;
	dimensions[0] = n;
	obj->Data = (PyArrayObject *)PyArray_FromDims(1, dimensions, PyArray_DOUBLE);
	if (obj->Data == NULL) return NULL;
    }
    speccount++;
/*      printf("spectrum %08X allocated, new count   = %d\n", obj, speccount); */

    return obj;
}

static PyObject *PoCopy(PoOdinScan *self, PyObject *args)
{
    PoOdinScan *obj = NULL;

    if (!PyArg_ParseTuple(args, "|O:Copy", &obj)) return NULL;

    if (obj) {
	/* An OdinScan was passed, copy onto self */
	if (self->Channels != obj->Channels) {
	    PyErr_SetString(PyExc_IndexError, "failed to copy spectrum");
	    return NULL;
	}
	headPy2C(obj, &s);
	if (dataPy2C(obj, &s) == 0) return NULL;
	headC2Py(&s, self);
	if (dataC2Py(&s, self) == 0) return NULL;
	return Py_None;
    } else {
	/* No argument was passed, copy self onto new odinscan */
	headPy2C(self, &s);
	if (dataPy2C(self, &s) == 0) return NULL;
	obj = NewPoOdinScan(s.Channels);
	if (obj) {
	    headC2Py(&s, obj);
	    if (dataC2Py(&s, obj) == 0) return NULL;
	}
	return (PyObject *)obj;
    }
}

static PyObject *PoCopyHead(PoOdinScan *self, PyObject *args)
{
    PoOdinScan *obj = NULL;
    int n;

    if (!PyArg_ParseTuple(args, "O:Copy", &obj)) return NULL;

    if (obj) {
	/* An OdinScan was passed, copy onto self */
	n = self->Channels;
	headPy2C(obj, &s);
	headC2Py(&s, self);
	self->Channels = n;

	return Py_None;
    }
    PyErr_SetString(PyExc_RuntimeError, "failed to retrieve scan object");
    return NULL;
}

static PyObject *PoCopyData(PoOdinScan *self, PyObject *args)
{
    PoOdinScan *obj = NULL;

    if (!PyArg_ParseTuple(args, "O:Copy", &obj)) return NULL;

    if (obj) {
	/* An OdinScan was passed, copy onto self */
	if (self->Channels != obj->Channels) {
	    PyErr_SetString(PyExc_IndexError, "failed to copy spectrum");
	    return NULL;
	}
	if (dataPy2C(obj, &s) == 0) return NULL;
	if (dataC2Py(&s, self) == 0) return NULL;
	return Py_None;
    }
    PyErr_SetString(PyExc_RuntimeError, "failed to retrieve scan object");
    return NULL;
}

static PyObject *PoSplit(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;
    PoOdinScan *half[2];
    int i;

    if (!PyArg_ParseTuple(args, ":Split")) return NULL;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    obj = PyList_New(2);
    if (obj) {
	for (i = 0; i < 2; i++) {
	    memcpy(&r, &s, sizeof(struct OdinScan));
	    if (!splitcorr(&r, i)) {
		PyErr_SetString(PyExc_IOError, "failed to split spectrum");
		return NULL;
	    }
	    half[i] = NewPoOdinScan(r.Channels);
	    if (half[i] == NULL) return NULL;
	    headC2Py(&r, half[i]);
	    if (dataC2Py(&r, half[i]) == 0) return NULL;
	    PyList_SetItem(obj, i, (PyObject *)half[i]);
	}
    }

    return obj;
}

static PyObject *PoMoment(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;
    int from, to, i;
    double *moment;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    from = 0;
    to = s.Channels;

    if (!PyArg_ParseTuple(args, "|ii:Moment", &from, &to)) return NULL;

    obj = PyList_New(4);
    if (obj) {
	moment = Moment(&s, from , to);
	for (i = 0; i < 4; i++) {
	    PyList_SetItem(obj, i, PyFloat_FromDouble(moment[i]));
	}
    }

    return obj;
}

static PyObject *PoAccum(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, "O:Accum", &obj)) return NULL;

    headPy2C((PoOdinScan *)obj, &r);
    if (dataPy2C((PoOdinScan *)obj, &r) == 0) return NULL;
  
    Accum(&s, &r);
    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoGain(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, "O:Gain", &obj)) return NULL;

    headPy2C((PoOdinScan *)obj, &r);
    if (dataPy2C((PoOdinScan *)obj, &r) == 0) return NULL;
  
    Gain(&s, &r);

    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoCalcTsys(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;
    float Th, Tc;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, "Off:CalcTsys", &obj, &Th, &Tc)) return NULL;

    headPy2C((PoOdinScan *)obj, &r);
    if (dataPy2C((PoOdinScan *)obj, &r) == 0) return NULL;
  
    CalcTsys(&s, &r, (double)Th, (double)Tc);

    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoAttitude(PoOdinScan *self, PyObject *args)
{
    PyObject *o;
    int i;
    unsigned long stw;
    double orbit, JD;
    double qt[4], qa[4], qe[3];
    double gps[6], acs;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, 
			  "(dOd(dddd)(dddd)(ddd)(dddddd)d):Attitude", 
			  &JD, &o, &orbit,
			  &qt[0], &qt[1], &qt[2], &qt[3], 
			  &qa[0], &qa[1], &qa[2], &qa[3], 
			  &qe[0], &qe[1], &qe[2], 
			  &gps[0], &gps[1], &gps[2], 
			  &gps[3], &gps[4], &gps[5], &acs)) return NULL;

    stw = PyLong_AsUnsignedLong(o);

    if (s.STW != stw) {
	PyErr_SetString(PyExc_RuntimeError, "attitude refers to wrong STW");
	return NULL;
    }

    s.MJD = jd2mjd(JD);
    s.Orbit = orbit;
    for (i = 0; i < 4; i++) {
	s.Qtarget[i]   = qt[i];
	s.Qachieved[i] = qa[i];
	if (i < 3) s.Qerror[i] = qe[i];
    }

    for (i = 0; i < 3; i++) {
	s.GPSpos[i]   = gps[i];
	s.GPSvel[i]   = gps[i+3];
    }

    skybeams(&s, acs);

    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *PoFitComb(PoOdinScan *self, PyObject *args)
{
    PyObject *obj;
    double *coeff;
    int i;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, ":FitComb")) return NULL;
    if ((s.Backend != AOS) || (s.Type != CMB)) {
	PyErr_SetString(PyExc_RuntimeError, "not an AOS comb spectrum");
	return NULL;
    }

    obj = PyList_New(4);
    if (obj) {
	coeff = FitAOSFreq(&s);
	for (i = 0; i < 4; i++) {
	    PyList_SetItem(obj, i, PyFloat_FromDouble(coeff[i]));
	}
    }

    return obj;
}

/*
static PyObject *PoFreqCut(PoOdinScan *self, PyObject *args)
{
    double lower, upper;
    static int dn;
    int n1, n2, nm;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    if (!PyArg_ParseTuple(args, "dd:FreqCut", &lower, &upper)) return NULL;
    printf("cut from %f to %f (%d)\n", lower, upper, nn);
    if (lower < upper) {
        n1 = FChannel(&s, lower);
        n2 = FChannel(&s, upper);
        nm = (n1+n2)/2;
        if (nn == 0) dn = abs(n2-n1)/2;
        n1 = nm-dn+1;
        n2 = nm+dn-1;
        Drop(&s, n1, n2);
        printf("spectrum cut to %d %d %d\n", n1, n2, n2-n1+1);
    }
    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}
*/

static PyObject *PoCalibrate(PoOdinScan *self, PyObject *args)
{
    PyObject *obj0, *obj1;
    double eta;

    headPy2C((PoOdinScan *)self, &s);
    if (dataPy2C((PoOdinScan *)self, &s) == 0) return NULL;

    eta = 0.0;
    if (!PyArg_ParseTuple(args, "OO|d:Calibrate", &obj0, &obj1, &eta)) 
	return NULL;
  
    headPy2C((PoOdinScan *)obj0, &r);
    if (dataPy2C((PoOdinScan *)obj0, &r) == 0) return NULL;

    headPy2C((PoOdinScan *)obj1, &t);
    if (dataPy2C((PoOdinScan *)obj1, &t) == 0) return NULL;
  
    Calibrate(&s, &r, &t, eta);

    headC2Py(&s, (PoOdinScan *)self);
    if (dataC2Py(&s, (PoOdinScan *)self) == 0) return NULL;

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *NewSpectrum(PyObject *self, PyObject *args)
{
    PoOdinScan *obj;
    char *filename = NULL;

    if (!PyArg_ParseTuple(args, "|s:Spectrum", &filename)) return NULL;
  
    memset(&s, 0, sizeof(struct OdinScan));
    if (filename) {
	if (!readODINscan(filename, &s)) {
	    PyErr_SetString(PyExc_IOError, "can't open file");
	    return NULL;
	}
    }
    obj = NewPoOdinScan(s.Channels);
    if (obj) {
	headC2Py(&s, obj);
	if (dataC2Py(&s, obj) == 0) return NULL;
    }
  
    return (PyObject *)obj;
}

PyObject *LogAs(PyObject *self, PyObject *args)
{
    char *filename = NULL;
    char *logas = NULL;

    if (!PyArg_ParseTuple(args, "s|s:LogAs", &logas, &filename)) return NULL;

    if (logas)    logname(logas);
    if (filename) logfile(filename);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *Polyfit(PyObject *self, PyObject *args)
{
    PyObject *x, *y, *o;
    double X, Y, **Array, *Vector, *D;
    int i, j, k, l, nx, ny, order;

    if (!PyArg_ParseTuple(args, "OOi:Polyfit", &x, &y, &order)) return NULL;

    nx = ny = 0;
    if (PySequence_Check(x)) nx = PySequence_Length(x);
    if (PySequence_Check(y)) ny = PySequence_Length(y);

    if (nx != ny) {
	PyErr_SetString(PyExc_IndexError, "unequal length of vectors");
	return NULL;
    }

    order = order+1;
    Vector = (double *)calloc(order, sizeof(double));
    if (Vector == NULL) {
	PyErr_SetString(PyExc_MemoryError, "memory allocation failure");
	return NULL;
    }

    D = (double *)calloc(order, sizeof(double));
    if (D == NULL) {
	PyErr_SetString(PyExc_MemoryError, "memory allocation failure");
	return NULL;
    }

    Array = (double **)calloc(order*order, sizeof(double));
    if (Array == NULL) {
	PyErr_SetString(PyExc_MemoryError, "memory allocation failure");
	free(Vector);
	free(D);
	return NULL;
    }

    for (i = 0; i < order; i++) {
	Array[i] = (double *)calloc(order, sizeof(double));
	if (Array[i] == NULL) {
	    PyErr_SetString(PyExc_MemoryError, "memory allocation failure");
	    free(Vector);
	    free(D);
	    free(Array);
	    return NULL;
	}
    }
    
    for (i = 0; i < order; i++) {
	Vector[i] = 0.0;
	for (j = i; j < order; j++) {
	    Array[i][j] = 0.0;
	    Array[j][i] = 0.0;
	}
    }
  
    for (k = 0; k < nx; k++) {
	D[0] = 1.0;
	X = PyFloat_AsDouble(PySequence_GetItem(x, k));
	Y = PyFloat_AsDouble(PySequence_GetItem(y, k));

	for (j = 1; j < order; j++) D[j] = D[j-1]*X;
	for (j = 0; j < order; j++) {
	    Vector[j] += Y*D[j];
	    for (l = j; l < order; l++) {
		Array[j][l] += D[j]*D[l];
		Array[l][j] = Array[j][l];
	    }
	}
    }

    if (solve(Array, Vector, order) == 0) {
	PyErr_SetString(PyExc_RuntimeError, "singular matrix");
	free(Vector);
	free(D);
	free(Array);
	return NULL;
    }

    o = PyList_New(order);
    if (o) {
	for (j = 0; j < order; j++) {
	    PyList_SetItem(o, j, PyFloat_FromDouble(Vector[j]));
	}
    }

    free(Vector);
    free(D);
    for (i = 0; i < order; i++) free(Array[i]);
    free(Array);

    return o;
}

PyObject *Warn(PyObject *self, PyObject *args)
{
    char *msg = NULL;

    if (!PyArg_ParseTuple(args, "s:Warn", &msg)) return NULL;

    ODINwarning(msg);

    Py_INCREF(Py_None);
    return Py_None;
}

PyObject *Info(PyObject *self, PyObject *args)
{
    char *msg = NULL;

    if (!PyArg_ParseTuple(args, "s:Info", &msg)) return NULL;

    ODINinfo(msg);

    Py_INCREF(Py_None);
    return Py_None;
}

/************************* FBAfile **************************/

#define FBABLOCKS 1

typedef struct {
    PyObject_HEAD
    FILE *fp;
    PyObject *name;
    int rstcnt;
    int count;
    unsigned long start;
    unsigned long stop;
    unsigned short data[FBABLOCKS*sizeof(FBABlock)/sizeof(short)];
} PyFBAfile;

#define FBAOFF(x) offsetof(PyFBAfile, x)

static struct memberlist fba_memberlist[] = {
    { "name",  T_OBJECT, FBAOFF(name)   },
    { "rstcnt",T_INT,    FBAOFF(rstcnt) },
    { "count", T_INT,    FBAOFF(count) },
    { "start", T_LONG,   FBAOFF(start)  },
    { "stop",  T_LONG,   FBAOFF(stop)   },
    { NULL }
};

static PyObject *FBANext(PyFBAfile *self, PyObject *args)
{
    PoOdinScan *obj;
    FILE *fp;
    FBABlock *fba;
    int k, n, nChannels, sync;
    time_t clock;
    unsigned long stw;
    struct tm *now;
    double JD, UTC;

    if (!PyArg_ParseTuple(args, ":NextSpectrum")) return NULL;

    fp = self->fp;

    sync = 0;
    while (!sync) {
	n = fread(self->data, sizeof(FBABlock), 1, fp);
	if (n != 1) {
	    if (feof(fp)) {
		PyErr_SetString(PyExc_EOFError, "end of level 0 file");
		return NULL;
	    } else {
		PyErr_SetString(PyExc_IOError, "can't read file");
		return NULL;
	    }
	}
	if (self->data[0] != SYNCWORD) {
	    PyErr_SetString(PyExc_IOError, "wrong sync word");
	    return NULL;
	}
	sync = 1;
	sync &= (self->data[3] == FBAUSER);
	sync &= (self->data[4] != 0xffff);
	sync &= ((self->data[4] & 0xfff0) != 0xb9b0);
	sync &= (self->data[7] != self->count);
    }
  
    fba = (FBABlock *)self->data;
    nChannels = GetFBAChannels(s.data, fba);
    self->count = (int)self->data[7];
    if (nChannels == -1) {
	PyErr_SetString(PyExc_IOError, "can't extract channels");
	return NULL;
    } else {
	stw   = LongWord(fba->head.STW);
	clock = stw2utc(stw, self->rstcnt);
	now   = gmtime(&clock);
	JD    = djl(now->tm_year+1900, now->tm_mon+1, now->tm_mday);
	UTC   = (double)(now->tm_hour*3600 + now->tm_min*60 + now->tm_sec)/86400.0;

	s.Version  = ODINSCANVERSION;
	s.Level    = 0x0000;
	s.Quality  = (0x0000000f & self->rstcnt);
	s.Channels = nChannels;
	s.STW      = stw;
	s.MJD      = jd2mjd(JD)+UTC;
	s.Orbit    = mjd2orbit(s.MJD);
	s.Spectrum = 0;
	s.Discipline = AERO;
	s.Backend  = FBA;
	s.Frontend = REC_119;
	s.IntTime  = GetFBAIntTime(fba);
	s.Type     = GetCalMirror(fba);
	s.SkyFreq  = 118.749860e9;
	s.LOFreq   = s.SkyFreq-3900.0e6;
	s.FreqRes  = 100.0e6;
	s.FreqCal[0] = 3900.0e6+LOWFREQ;
	s.FreqCal[1] = 3900.0e6+MIDFREQ;
	s.FreqCal[2] = 3900.0e6+HIFREQ;
	s.FreqCal[3] = 0.0;
	for (k = 0; k < FILTERCHANNELS; k++) {
	    s.data[k] /= s.IntTime;
	}

    }

    obj = NewPoOdinScan(s.Channels);
    if (obj) {
	headC2Py(&s, obj);
	if (dataC2Py(&s, obj) == 0) return NULL;
    } 

    return (PyObject *)obj;
}

static PyMethodDef fba_functions[] = {
    { "NextSpectrum", (PyCFunction)FBANext, METH_VARARGS },
    { NULL, NULL }
};

static void fba_dealloc(PyFBAfile *self)
{
    if (self) {
	if (self->fp) fclose(self->fp);
	Py_XDECREF(self->name);
        Py_XDECREF(self);
	/* free(self); */
    }
}

static PyObject *fba_getattr(PyFBAfile *self, char *attr)
{
    PyObject *res;

    res = Py_FindMethod(fba_functions, (PyObject *)self, attr);
    if (res != NULL) return res;
    else {
	PyErr_Clear();
	return PyMember_Get((char *)self, fba_memberlist, attr);
    }
}

static int fba_setattr(PyFBAfile *self, char *attr, PyObject *value)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_AttributeError, "can't delete attributes");
	return -1;
    }
    return PyMember_Set((char *)self, fba_memberlist, attr, value);
}

PyTypeObject fba_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "FBAfile",
    sizeof(PyFBAfile),
    0,
    (destructor)  fba_dealloc,
    0,
    (getattrfunc) fba_getattr,
    (setattrfunc) fba_setattr
};

PyObject *FBAfile(PyObject *self, PyObject *args)
{
    PyFBAfile *obj;
    unsigned long STW0, STW1;
    FILE *fp;
    char *filename = NULL;

    if (!PyArg_ParseTuple(args, "s:FBAfile", &filename)) return NULL;

    obj = PyObject_NEW(PyFBAfile, &fba_Type);
    if (filename) {
	if ((fp = fopen(filename, "r")) == NULL) {
	    PyErr_SetString(PyExc_IOError, "can't open file");
	    return NULL;
	}
    
	obj->fp = fp;
	obj->name  = PyString_FromString(filename);
	STWrange(fp, &STW0, &STW1);
	obj->rstcnt = STWreset(filename);
	obj->count = -1;
	obj->start = STW0;
	obj->stop  = STW1;
    }
  
    return (PyObject *)obj;
}

/************************* AOSfile **************************/

#define AOSBLOCKS 32

typedef struct {
    PyObject_HEAD
    FILE *fp;
    PyObject *name;
    int rstcnt;
    unsigned long start;
    unsigned long stop;
    unsigned short data[AOSBLOCKS*sizeof(AOSBlock)/sizeof(short)];
} PyAOSfile;

#define AOSOFF(x) offsetof(PyAOSfile, x)

static struct memberlist aos_memberlist[] = {
    { "name",  T_OBJECT, AOSOFF(name)   },
    { "rstcnt",T_INT,    AOSOFF(rstcnt) },
    { "start", T_LONG,   AOSOFF(start)  },
    { "stop",  T_LONG,   AOSOFF(stop)   },
    { NULL }
};

#define ALIGNED    1
#define NOTALIGNED 2

static PyObject *AOSNext(PyAOSfile *self, PyObject *args)
{
    PoOdinScan *obj;
    FILE *fp;
    AOSBlock *aos;
    int i, n, sync, mode, formats;

    if (!PyArg_ParseTuple(args, ":NextSpectrum")) return NULL;

    fp = self->fp;
    sync = 0;
    while (!sync) {
	aos = (AOSBlock *)self->data;
	n = fread(aos, sizeof(AOSBlock), 1, fp);
	if (n != 1) {
	    if (feof(fp)) {
		PyErr_SetString(PyExc_EOFError, "end of level 0 file");
		return NULL;
	    } else {
		PyErr_SetString(PyExc_IOError, "can't read file");
		return NULL;
	    }
	}
	if (self->data[0] != SYNCWORD) {
	    PyErr_SetString(PyExc_IOError, "wrong sync word");
	    return NULL;
	}
	sync = 1;
	sync &= (self->data[3] == AOSUSER);
	mode = AOSformat(aos);
    }
  
    switch (mode) {
      case 111:
      case 211:
	formats = 16;
	break;
      case 312:
	formats = 32;
	break;
      case 411:
	formats = 8;
	break;
      case 511:
	formats = 3;
	break;
      case 611:
      case 711:
	formats = 5;
	break;
      default:
	formats = 0;
	break;
    }

    for (i = 1; i < formats; i++) {
	n = fread(aos+i, sizeof(AOSBlock), 1, fp);
	if (n != 1) {
	    if (feof(fp)) {
		PyErr_SetString(PyExc_EOFError, "end of level 0 file");
		return NULL;
	    } else {
		PyErr_SetString(PyExc_IOError, "can't read file");
		return NULL;
	    }
	}
    }

    if (AOSBlock2Scan(&s, aos) == 0) {
	Py_INCREF(Py_None);
	return Py_None;
    }
    //    PrintScan(&s);
    // Py_INCREF(Py_None);
    // return Py_None;

    s.Quality |= (0x0000000f & self->rstcnt);
    obj = NewPoOdinScan(s.Channels);
    if (obj) {
	headC2Py(&s, obj);
	if (dataC2Py(&s, obj) == 0) return NULL;
    } 
    return (PyObject *)obj;
}

static PyMethodDef aos_functions[] = {
    { "NextSpectrum", (PyCFunction)AOSNext, METH_VARARGS },
    { NULL, NULL }
};

static void aos_dealloc(PyAOSfile *self)
{
    if (self) {
	if (self->fp) fclose(self->fp);
	Py_XDECREF(self->name);
	free(self);
    }
}

static PyObject *aos_getattr(PyAOSfile *self, char *attr)
{
    PyObject *res;

    res = Py_FindMethod(aos_functions, (PyObject *)self, attr);
    if (res != NULL) return res;
    else {
	PyErr_Clear();
	return PyMember_Get((char *)self, aos_memberlist, attr);
    }
}

static int aos_setattr(PyAOSfile *self, char *attr, PyObject *value)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_AttributeError, "can't delete attributes");
	return -1;
    }
    return PyMember_Set((char *)self, aos_memberlist, attr, value);
}

PyTypeObject aos_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "AOSfile",
    sizeof(PyAOSfile),
    0,
    (destructor)  aos_dealloc,
    0,
    (getattrfunc) aos_getattr,
    (setattrfunc) aos_setattr
};

PyObject *AOSfile(PyObject *self, PyObject *args)
{
    PyAOSfile *obj;
    unsigned long STW0, STW1;
    FILE *fp;
    char *filename = NULL;

    if (!PyArg_ParseTuple(args, "s:AOSfile", &filename)) return NULL;

    obj = PyObject_NEW(PyAOSfile, &aos_Type);
    if (filename) {
	if ((fp = fopen(filename, "r")) == NULL) {
	    PyErr_SetString(PyExc_IOError, "can't open file");
	    return NULL;
	}
    
	obj->fp = fp;
	obj->name  = PyString_FromString(filename);
	STWrange(fp, &STW0, &STW1);
	obj->rstcnt = STWreset(filename);
	obj->start = STW0;
	obj->stop  = STW1;
    }
  
    return (PyObject *)obj;
}

/************************* ACfile **************************/

#define ACBLOCKS 13

typedef struct {
    PyObject_HEAD
    FILE *fp;
    PyObject *name;
    int rstcnt;
    unsigned long start;
    unsigned long stop;
    unsigned short data[ACBLOCKS*sizeof(ACSBlock)/sizeof(short)];
} PyACfile;

#define ACOFF(x) offsetof(PyACfile, x)

static struct memberlist ac_memberlist[] = {
    { "name",  T_OBJECT, ACOFF(name)   },
    { "rstcnt",T_INT,    ACOFF(rstcnt) },
    { "start", T_LONG,   ACOFF(start)  },
    { "stop",  T_LONG,   ACOFF(stop)   },
    { NULL }
};

static PyObject *ACNext(PyACfile *self, PyObject *args)
{
    PoOdinScan *obj;
    FILE *fp;
    ACSBlock *ac;
    char *option = NULL;
    int i, n, sync, formats;

    if (!PyArg_ParseTuple(args, "|s:NextSpectrum", &option)) return NULL;

    fp = self->fp;
    sync = 0;
    while (!sync) {
	ac = (ACSBlock *)self->data;
	n = fread(ac, sizeof(ACSBlock), 1, fp);
	if (n != 1) {
	    if (feof(fp)) {
		PyErr_SetString(PyExc_EOFError, "end of level 0 file");
		return NULL;
	    } else {
		PyErr_SetString(PyExc_IOError, "can't read file");
		return NULL;
	    }
	}
	if (self->data[0] != SYNCWORD) {
	    PyErr_SetString(PyExc_IOError, "wrong sync word");
	    return NULL;
	}
	sync = 1;
	sync &= ((self->data[3] & 0xff0f) == 0x7300);
    }
  
    formats = 13;

    for (i = 1; i < formats; i++) {
	n = fread(ac+i, sizeof(ACSBlock), 1, fp);
	if (n != 1) {
	    if (feof(fp)) {
		PyErr_SetString(PyExc_EOFError, "end of level 0 file");
		return NULL;
	    } else {
		PyErr_SetString(PyExc_IOError, "can't read file");
		return NULL;
	    }
	}
    }

    if (ACSBlock2Scan(&s, ac) == 0) {
	Py_INCREF(Py_None);
	return Py_None;
    }

    s.Quality |= (0x0000000f & self->rstcnt);
    Normalise(&s);
    if (option == NULL || strcmp(option, "raw") != 0) {
	if (!ReduceAC(&s)) {
	    PyErr_SetString(PyExc_IOError, "can't reduce spectrum");
	    return NULL;
	}
    }

    obj = NewPoOdinScan(s.Channels);
    if (obj) {
	headC2Py(&s, obj);
	if (dataC2Py(&s, obj) == 0) return NULL;
    } 

    return (PyObject *)obj;
}

static PyObject *ACZlag(PyACfile *self, PyObject *args)
{
    PyObject *obj;
    ACSBlock *ac;
    int k, mode, bands = 0;
    unsigned long lag0[8], samples;
    double zlag[8];

    obj = PyList_New(8);
    if (obj) {
	ac = (ACSBlock *)self->data;
	mode = GetACMode(ac);
	switch (mode) {
	  case AC_XHIRES: bands = 1; break;
	  case AC_HIRES:  bands = 2; break;
	  case AC_MEDRES: bands = 4; break;
	  case AC_LOWRES: bands = 8; break;
	}

	samples = GetSamples(ac);
	for (k = 0; k < 8; k++) {
	    if (k < bands) {
		lag0[k] = GetZLag(ac, k*MAXCHIPS/bands);
		zlag[k] = (double)lag0[k];
		if (samples > 0L) {
		    zlag[k] *= 2048.0*(SAMPLEFREQ/(double)samples)/(CLOCKFREQ/2.0);
		} else {
		    zlag[k] = 0.0;
		}
	    } else {
		zlag[k] = 0.0;
	    }
	    PyList_SetItem(obj, k, PyFloat_FromDouble(zlag[k]));
	}
    }

    return obj;
}

static PyMethodDef ac_functions[] = {
    { "NextSpectrum", (PyCFunction)ACNext, METH_VARARGS },
    { "ZeroLags",     (PyCFunction)ACZlag, METH_VARARGS },
    { NULL, NULL }
};

static void ac_dealloc(PyACfile *self)
{
    if (self) {
	if (self->fp) fclose(self->fp);
	Py_XDECREF(self->name);
	free(self);
    }
}

static PyObject *ac_getattr(PyACfile *self, char *attr)
{
    PyObject *res;

    res = Py_FindMethod(ac_functions, (PyObject *)self, attr);
    if (res != NULL) return res;
    else {
	PyErr_Clear();
	return PyMember_Get((char *)self, ac_memberlist, attr);
    }
}

static int ac_setattr(PyACfile *self, char *attr, PyObject *value)
{
    if (value == NULL) {
	PyErr_SetString(PyExc_AttributeError, "can't delete attributes");
	return -1;
    }
    return PyMember_Set((char *)self, ac_memberlist, attr, value);
}

PyTypeObject ac_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,
    "ACfile",
    sizeof(PyACfile),
    0,
    (destructor)  ac_dealloc,
    0,
    (getattrfunc) ac_getattr,
    (setattrfunc) ac_setattr
};

PyObject *ACfile(PyObject *self, PyObject *args)
{
    PyACfile *obj;
    unsigned long STW0, STW1;
    FILE *fp;
    char *filename = NULL;

    if (!PyArg_ParseTuple(args, "s:ACfile", &filename)) return NULL;

    obj = PyObject_NEW(PyACfile, &ac_Type);
    if (filename) {
	if ((fp = fopen(filename, "r")) == NULL) {
	    PyErr_SetString(PyExc_IOError, "can't open file");
	    return NULL;
	}
    
	obj->fp = fp;
	obj->name  = PyString_FromString(filename);
	STWrange(fp, &STW0, &STW1);
	obj->rstcnt = STWreset(filename);
	obj->start = STW0;
	obj->stop  = STW1;
    }
  
    return (PyObject *)obj;
}

/************************** odin functions **************************/

static PyMethodDef odin_functions[] = {
    { "Spectrum", NewSpectrum, METH_VARARGS },
    { "FBAfile",  FBAfile,     METH_VARARGS },
    { "AOSfile",  AOSfile,     METH_VARARGS },
    { "ACfile",   ACfile,      METH_VARARGS },
    { "Polyfit",  Polyfit,     METH_VARARGS },
    { "LogAs",    LogAs,       METH_VARARGS },
    { "Warn",     Warn,        METH_VARARGS },
    { "Info",     Info,        METH_VARARGS },
    { NULL, NULL }
};

void initodin()
{
    PyObject *m;
    /*    static void *PyOdin_API[PyOdin_API_pointers]; */
    /*    PyObject *m, *d, *c_api_object; */

    m = Py_InitModule("odin", odin_functions);
    import_array();
    logname("python");
    /*    PyOdin_API[PyOdin_Spectrum_NUM] = (void *)NewSpectrum; */
    /*    c_api_object = PyCObject_FromVoidPtr((void *)PyOdin_API, NULL); */
  
    /*    d = PyModule_GetDict(m); */
    /*    PyDict_SetItemString(d, "_C_API", c_api_object); */
}
