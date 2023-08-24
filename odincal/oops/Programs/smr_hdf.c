#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <hdf.h>
#include "smr_hdf_structs.h"
#include "smr_hdf_defs.h"
#include "smr_hdf.h"

static char rcsid[] = "$Id";

/*****************************************************************
**
** SMR structures to HDF software. Version 0.3
**
** Created by:
**   Frank Merino
**
** The following software was written to write an OdinScan
** an OdinFilter structure to a HDF file using the Vdata model interface.
** The format that the data takes within the Vdata are based
** on Nick Lloyd's software ONYX.
**

** Version 0.4: modified by Michael Olberg

*****************************************************************/

/*----------------------------------------------------
-- These are the Vdata names for each container     --
-- They are needed when calling the HDF routine     --
-- VSsetname. Sets the Vdata/Container name.        --
----------------------------------------------------*/

static char *vdata_name_tags[NUM_VDATA_TYPES] =
{
  "SMR",
  "R4:R4:01728",
  "R4:R4:00896",
  "R4:R4:00448",
  "SMR FILTER BANK"
};

/*----------------------------------------------------
-- The first column gives the HDF vdata_id for each --
-- container. "-1" means it is closed.              --
----------------------------------------------------*/
static int32 vdata_id_num[NUM_VDATA_TYPES] =
{
  -1 ,    /* CONT_SMR_HEADER */
  -1 ,    /* CONT_AOS_1728 */
  -1 ,    /* CONT_SMR_DATA_896 */
  -1 ,    /* CONT_SMR_DATA_448 */
  -1 ,    /* CONT_FILTER_DATA */
};

struct ODIN_FILTER_STRUCT *smrfilt;
struct ODIN_SMR_STRUCT    *smrtmpl;

#define OFFSET(base, field) (((char *)&(base##->##field)) - ((char *)(base)))

static const char *smr_fields[NUM_FIELDS] = 
{
  "Version",
  "Level",
  "Quality",
  "STW",
  "MJD",
  "Orbit",
  "LST",
  "Source",
  "Discipline",
  "Topic",
  "Spectrum",
  "ObsMode",
  "Type",
  "Frontend",
  "Backend",
  "SkyBeamHit",
  "RA2000",
  "Dec2000",
  "VSource",
  "Longitude",
  "Latitude",
  "Altitude",
  "Qtarget",
  "Qachieved",
  "Qerror",
  "GPSpos",
  "GPSvel",
  "SunPos",
  "MoonPos",
  "SunZD",
  "Vhel",
  "Vlsr",
  "Tcal",
  "Tsys",
  "SBpath",
  "LOFreq",
  "SkyFreq",
  "RestFreq",
  "MaxSuppression",
  "FreqThrow",
  "FreqRes",
  "FreqCal",
  "IntMode",
  "IntTime",
  "EffTime",
  "Channels",
  "TBD",
  "pointer"
};

static const uint16 smr_offsets[NUM_FIELDS_EXPANDED][2] =
{
  { OFFSET(smrtmpl, scan.Version), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Level), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Quality), sizeof(long)   },
  { OFFSET(smrtmpl, scan.STW), sizeof(long)   },
  { OFFSET(smrtmpl, scan.MJD), sizeof(double) },
  { OFFSET(smrtmpl, scan.Orbit), sizeof(double) },
  { OFFSET(smrtmpl, scan.LST), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Source[0]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[1]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[2]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[3]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[4]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[5]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[6]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[7]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[8]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[9]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[10]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[11]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[12]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[13]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[14]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[15]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[16]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[17]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[18]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[19]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[20]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[21]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[22]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[23]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[24]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[25]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[26]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[27]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[28]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[29]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[30]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Source[31]), sizeof(char) },
  { OFFSET(smrtmpl, scan.Discipline), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Topic), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Spectrum), sizeof(short)  },
  { OFFSET(smrtmpl, scan.ObsMode), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Type), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Frontend), sizeof(short)  },
  { OFFSET(smrtmpl, scan.Backend), sizeof(short)  },
  { OFFSET(smrtmpl, scan.SkyBeamHit), sizeof(short)  },
  { OFFSET(smrtmpl, scan.RA2000), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Dec2000), sizeof(float)  },
  { OFFSET(smrtmpl, scan.VSource), sizeof(float)  },
  { OFFSET(smrtmpl, scan.u.tp.Longitude), sizeof(float)  },
  { OFFSET(smrtmpl, scan.u.tp.Latitude), sizeof(float)  },
  { OFFSET(smrtmpl, scan.u.tp.Altitude), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Qtarget[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qtarget[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qtarget[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qtarget[3]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qachieved[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qachieved[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qachieved[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qachieved[3]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qerror[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qerror[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.Qerror[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSpos[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSpos[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSpos[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSvel[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSvel[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.GPSvel[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.SunPos[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.SunPos[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.SunPos[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.MoonPos[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.MoonPos[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.MoonPos[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.SunZD), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Vhel), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Vlsr), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Tcal), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Tsys), sizeof(float)  },
  { OFFSET(smrtmpl, scan.SBpath), sizeof(float)  },
  { OFFSET(smrtmpl, scan.LOFreq), sizeof(double) },
  { OFFSET(smrtmpl, scan.SkyFreq), sizeof(double) },
  { OFFSET(smrtmpl, scan.RestFreq), sizeof(double) },
  { OFFSET(smrtmpl, scan.MaxSuppression), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqThrow), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqRes), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqCal[0]), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqCal[1]), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqCal[2]), sizeof(double) },
  { OFFSET(smrtmpl, scan.FreqCal[3]), sizeof(double) },
  { OFFSET(smrtmpl, scan.IntMode), sizeof(int)    },
  { OFFSET(smrtmpl, scan.IntTime), sizeof(float)  },
  { OFFSET(smrtmpl, scan.EffTime), sizeof(float)  },
  { OFFSET(smrtmpl, scan.Channels), sizeof(int)    },
  { OFFSET(smrtmpl, TBD.version[0]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, TBD.version[1]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, TBD.version[2]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, TBD.version[3]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, pointer.key[0]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, pointer.key[1]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, pointer.key[2]), sizeof(unsigned long) },
  { OFFSET(smrtmpl, pointer.key[3]), sizeof(unsigned long) }
};

static const int32 smr_types[NUM_FIELDS][2] =
{
  { DFNT_UINT16, 1 },
  { DFNT_UINT16, 1 },
  { DFNT_UINT32, 1 },
  { DFNT_UINT32, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_CHAR8, 32 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT64, 4 },
  { DFNT_FLOAT64, 4 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 4 },
  { DFNT_INT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_INT32, 1 },
  { DFNT_UINT32, NUM_VERSION_ENTRY},
  { DFNT_UINT32, NUM_HOPTR_ENTRY}
};

static const char *smr_filter_fields[NUM_FILTER_FIELDS] = 
{
  "Version",
  "Level",
  "Quality",
  "STW",
  "MJD",
  "Orbit",
  "LST",
  "Discipline",
  "Topic",
  "Spectrum",
  "ObsMode",
  "Type",
  "Longitude",
  "Latitude",
  "Altitude",
  "Qtarget",
  "Qachieved",
  "Qerror",
  "GPSpos",
  "GPSvel",
  "Tcal",
  "Tsys",
  "SBpath",
  "LOFreq",
  "SkyFreq",
  "RestFreq",
  "FreqCal",
  "IntTime",
  "EffTime",
  "Channels",
  "Array",
  "TBD"
};

static const uint16 smr_filter_offsets[NUM_FILTER_FIELDS_EXPANDED][2] =
{
  { OFFSET(smrfilt, scan.Version), sizeof(short)  },
  { OFFSET(smrfilt, scan.Level), sizeof(short)  },
  { OFFSET(smrfilt, scan.Quality), sizeof(long)   },
  { OFFSET(smrfilt, scan.STW), sizeof(long)   },
  { OFFSET(smrfilt, scan.MJD), sizeof(double) },
  { OFFSET(smrfilt, scan.Orbit), sizeof(double) },
  { OFFSET(smrfilt, scan.LST), sizeof(float)  },
  { OFFSET(smrfilt, scan.Discipline), sizeof(short)  },
  { OFFSET(smrfilt, scan.Topic), sizeof(short)  },
  { OFFSET(smrfilt, scan.Spectrum), sizeof(short)  },
  { OFFSET(smrfilt, scan.ObsMode), sizeof(short)  },
  { OFFSET(smrfilt, scan.Type), sizeof(short)  },
  { OFFSET(smrfilt, scan.Longitude), sizeof(float)  },
  { OFFSET(smrfilt, scan.Latitude), sizeof(float)  },
  { OFFSET(smrfilt, scan.Altitude), sizeof(float)  },
  { OFFSET(smrfilt, scan.Qtarget[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qtarget[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qtarget[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qtarget[3]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qachieved[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qachieved[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qachieved[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qachieved[3]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qerror[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qerror[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.Qerror[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSpos[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSpos[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSpos[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSvel[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSvel[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.GPSvel[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.Tcal), sizeof(float)  },
  { OFFSET(smrfilt, scan.Tsys), sizeof(float)  },
  { OFFSET(smrfilt, scan.SBpath), sizeof(float)  },
  { OFFSET(smrfilt, scan.LOFreq), sizeof(double) },
  { OFFSET(smrfilt, scan.SkyFreq), sizeof(double) },
  { OFFSET(smrfilt, scan.RestFreq), sizeof(double) },
  { OFFSET(smrfilt, scan.FreqCal[0]), sizeof(double) },
  { OFFSET(smrfilt, scan.FreqCal[1]), sizeof(double) },
  { OFFSET(smrfilt, scan.FreqCal[2]), sizeof(double) },
  { OFFSET(smrfilt, scan.IntTime), sizeof(float)  },
  { OFFSET(smrfilt, scan.EffTime), sizeof(float)  },
  { OFFSET(smrfilt, scan.Channels), sizeof(int)    },
  { OFFSET(smrfilt, scan.data[0]), sizeof(float) },
  { OFFSET(smrfilt, scan.data[1]), sizeof(float) },
  { OFFSET(smrfilt, scan.data[2]), sizeof(float) },
  { OFFSET(smrfilt, TBD.version[0]), sizeof(unsigned long) },
  { OFFSET(smrfilt, TBD.version[1]), sizeof(unsigned long) },
  { OFFSET(smrfilt, TBD.version[2]), sizeof(unsigned long) },
  { OFFSET(smrfilt, TBD.version[3]), sizeof(unsigned long) }
};

static const int32 smr_filter_types[NUM_FILTER_FIELDS][2] =
{
  { DFNT_UINT16, 1 },
  { DFNT_UINT16, 1 },
  { DFNT_UINT32, 1 },
  { DFNT_UINT32, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_INT16, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT64, 4 },
  { DFNT_FLOAT64, 4 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 1 },
  { DFNT_FLOAT64, 3 },
  { DFNT_FLOAT32, 1 },
  { DFNT_FLOAT32, 1 },
  { DFNT_INT32, 1 },
  { DFNT_UINT32, NUM_VERSION_ENTRY},
  { DFNT_UINT32, NUM_HOPTR_ENTRY}
};

/*****************************************************************
**                 read_filter_hdf                              **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
** Output parameters:                                           **
**      smr        :    ODIN_FILTER_STRUCTURE                   **
**                                                              **
*****************************************************************/
int32 read_filter_hdf(int32 file_id, struct OdinFilter *filter1)
{
  int32 istat;
  struct ODIN_FILTER_STRUCT *filter;
 
  if ((filter = (struct ODIN_FILTER_STRUCT *) malloc(sizeof(struct ODIN_FILTER_STRUCT))) == NULL)
    se("Out of memory allocating the structure filter in read_filter_hdf. \n");
 
  /* Read the structure */
  istat = read_filter(file_id, filter, 1, CONT_FILTER_DATA);          
 
  memcpy(filter1, &filter->scan, sizeof(struct OdinFilter));
 
  (void)free(filter);
 
  return(istat);
}

/*****************************************************************
**                 container_records                            **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      cont_id    :    Vdata identifier of the container we    **
**                      wish to read from.                      **
**                                                              **
** Output parameters:                                           **
**      n_records  : number of records in a given Vdata         **
*****************************************************************/
 
int32 container_records(int32 file_id,int32 cont_id)
{
  int32 vdata_id,n_records,interlace,vdata_size;
  char  vdata_name[500],fields[300];
 
  vdata_id = container_id(file_id,cont_id,"r");
  VSinquire(vdata_id,&n_records,&interlace,fields,&vdata_size,vdata_name);
  return(n_records);
}

/*****************************************************************
**                 read_smr_hdf                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
** Output parameters:                                           **
**      smr        :    ODIN_SMR_STRUCTURE                      **
**                                                              **
*****************************************************************/
int32 read_smr_hdf(int32 file_id, struct OdinScan *smr1)
{
  int32 istat;
  struct ODIN_SMR_STRUCT *smr;

  if ((smr = (struct ODIN_SMR_STRUCT  *) malloc(sizeof(struct ODIN_SMR_STRUCT))) == NULL)
    se("Out of memory allocating the structure smr in read_smr_hdf. \n");
 
  /* Read the structure */
  istat = read_smr(file_id,smr,1,CONT_SMR_HEADER);          
  /* Read the spectra   */
  istat &= read_spectra(file_id,smr->pointer,&smr->scan.data[0],1);  

  memcpy(smr1,&smr->scan,sizeof(struct OdinScan));

  (void) free(smr);
 
  return(istat);
}

/*****************************************************************
**                 write_smr_hdf                                **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
** smr1    :    OdinScan structure as defined by **
**   Olberg.     **
** Output parameters:                                           **
** istat    :    flag indicating whether the write was   **
**   successful. -1 means fail.  **
**                                                              **
*****************************************************************/
int32 write_smr_hdf(int32 file_id,struct OdinScan *smr1)
{
  int32 istat,mode,nch,backend,split = 0;
  struct ODIN_SMR_STRUCT *smr;


  if ((smr = (struct ODIN_SMR_STRUCT  *) malloc(sizeof(struct ODIN_SMR_STRUCT))) == NULL)
    se("Out of memory allocating the structure smr in write_smr_hdf. \n");

  memcpy(&smr->scan,smr1,sizeof(struct OdinScan));

  mode = smr->scan.IntMode; 
  backend = smr->scan.Backend;

  if ((backend == AC1 || backend == AC2) && (mode & AC_SPLIT)) split = 1;
  mode &= 0x000f; /* keep lowest 4 bits only to test for split mode */

  if (backend == AC1) {
    nch = split ? SIZE_SMR_DATA : 2*SIZE_SMR_DATA;

    switch (mode) {
     case AC_XHIRES :
      if (split) (void)se("\nNo split mode in extra high resolution mode. \n");
      istat = write_spectra(file_id,&smr->pointer,&smr->scan.data[0],
			    sizeof(float)*nch,nch,1);
      break;
     case AC_HIRES :
     case AC_MEDRES :
     case AC_LOWRES :
      istat = write_spectra(file_id,&smr->pointer,&smr->scan.data[0],
			    sizeof(float)*nch,nch,1);
      break;
     default :
      se("\nError in write_smr_hdf :\nunknown switch for the AC1 Backend mode . \n");
    }
  } else if (backend == AOS) {
    nch = SIZE_AOS_1728;
    switch (mode) {
     case AOS_LONG :
     case AOS_SHORT :
     case AOS_HALF :
      istat = write_spectra(file_id,&smr->pointer,&smr->scan.data[0],
			    sizeof(float)*nch,nch,1);
      break;
     default :
      se("\nError in write_smr_hdf : \nunknown switch for the AOS. \n");
    }
  } else se("\nError in write_smr_hdf : \n unknown backend. \n");

  istat = write_smr(file_id,smr,1,CONT_SMR_HEADER);
  (void) free(smr);

  return(istat);
}

/*****************************************************************
**                 write_filter_hdf                             **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      filter1       :    OdinFilter structure          **
**        **
** Output parameters:                                           **
**      istat      :    flag indicating whether the write was   **
**                      successful. -1 means fail.              **
**                                                              **
*****************************************************************/
int32 write_filter_hdf(int32 file_id,struct OdinFilter *filter1)
{
  int32 istat;
  struct ODIN_FILTER_STRUCT *filter;
 
  if ((filter = (struct ODIN_FILTER_STRUCT  *) malloc(sizeof(struct ODIN_FILTER_STRUCT))) == NULL)
    se("Out of memory allocating the structure filter in write_filter_hdf. \n");
 
  memcpy(&filter->scan,filter1,sizeof(struct OdinFilter));
 
  istat = write_filter(file_id,filter,1,CONT_FILTER_DATA);
  (void) free(filter);
 
  return(istat);
}

/*****************************************************************
**                 write_filter                                 **
**                                                              **
** Input parameters:                                            **
**      file_id         : Vdata file identifier.                **
**      ODIN_FILTER_STRUCT : The Odin filter bank scan struct.  **
**      databuf         : Buffer for records to be stored.      **
**      nrec            : Number of records to be stored.       **
** Output parameters:                                           **
**      Returns a boolean indicating whether the HDF routine    **
**      VSwrite was successful.                                 **
**                                                              **
*****************************************************************/
int32 write_filter(int32 file_id,struct ODIN_FILTER_STRUCT *filterdata,int32 nrec,int32 cont_id)
{
  int32         istat;
  int32         vdata_id;
  uint16        sum = 0;
  uint16        offset;
  void          *pntr;
  void          *databuf;
  void          *start;
  int           i;
 
  vdata_id = container_id(file_id,cont_id,"w");
 
  databuf = NULL;
  sum = 0;
  for (i = 0; i < NUM_FILTER_FIELDS_EXPANDED; i++)
    sum += smr_filter_offsets[i][1];
  if ((databuf = (char *) malloc(sum*nrec)) == NULL)
    se("\nError in write_filter : \nwhen allocating memory for databuf. \n");
 
  filterdata->TBD.version[0] = 0;
  filterdata->TBD.version[1] = 0;
  filterdata->TBD.version[2] = 0;
  filterdata->TBD.version[3] = 0;
 
  pntr = databuf;
  start = filterdata;
 
  for (i = 0; i < NUM_FILTER_FIELDS_EXPANDED; i++) {
    offset = smr_filter_offsets[i][1];
    memcpy(pntr, start + smr_filter_offsets[i][0], offset);
    pntr += offset;
  }
 
  istat = VSwrite(vdata_id, databuf, nrec, FULL_INTERLACE);
 
  free(databuf);
 
  return(istat);
}

/*****************************************************************
**                 close_HDF_vdata                              **
**                                                              **
** Input parameters:                                            **
**  file_id    : File identifier.   **
** Output parameters:   NONE.            **
**                                                              **
*****************************************************************/
void close_HDF_vdata(int32 file_id)
{
  if (close_containers() == -1)
    se("\nError in Close_HDF_Vdata : \nwhen calling close_containers. \n"); 
  if (Vend(file_id) == -1)
    se("\nError in Close_HDF_Vdata : \nwhen calling Vend. \n");
  if ((Hclose(file_id)) == -1)
    se("\nError in Close_HDF_Vdata : \nwhen closing the HDF. \n");
}

/*****************************************************************
**                 close_containers                             **
**                                                              **
** Input parameters:    NONE                                    **
** Output parameters:   Returns a boolean.          **
**                                                              **
*****************************************************************/
int32 close_containers()
{
  int32 istat = 0;
  int i;
  
  for (i = 0; i < NUM_VDATA_TYPES; i++) {
    if (vdata_id_num[i] != -1) {
      istat = VSdetach(vdata_id_num[i]);
      vdata_id_num[i] = -1;
    }
  }
  return(istat);
}

/*****************************************************************
**                 open_HDF_vdata                               **
**                                                              **
** Input parameters:                                            **
** filename   :  name of the HDF file to be opened. **
** access_mode: access mode definition. (read/write).   **
** n_dds    : number of data descriptors in a block   **
**   if a new file is to be created.  **
** Output parameters:           **
**      file_id    :    File identifier.                        **
**                                                              **
*****************************************************************/
int32 open_HDF_vdata(char *filename,int32 access_mode, int16 n_dds)        
{
  int32 file_id,istat;
  
  if ((file_id = Hopen(filename, access_mode, n_dds)) == -1)
    se("\nError in Open_HDF_Vdata : \nwhen Opening the HDF file %s. \n",filename);
  
  if ((istat = Vstart(file_id)) == -1)
    se("\nError in Open_HDF_Vdata : \nwhen calling Vstart. \n");
  
  return(file_id);
}

/*****************************************************************
**                 read_spectra                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.          **
** data    : HOPTR.     **
** nrec    :    number of records to read from the Vdata**
** Output parameters:                                           **
** buffer    :    buffer for the retrieved data.  **
**                                                              **
*****************************************************************/
int32 read_spectra(int32 file_id,struct HOPTR pointer,void *buffer,int32 nrec)
{
  int32         istat;
  int32         vdata_id;
  char          *databuf;
  void          *pntr;
  unsigned long ncols,nrows;
  static unsigned long   crosscheck;

  ncols = pointer.key[1];
  nrows = 1;

  /* Use the HOPTR information to select the correct Vdata */
  vdata_id = container_id(file_id,pointer.key[1],"r");

  databuf = NULL;
  if ((databuf = (char *) malloc(ncols*HOPTR_FIELD2_SIZE+HOPTR_FIELD1_SIZE*nrec)) == NULL)
    se("\nError in read_spectra : \nwhen allocating memory for databuf. \n");
  
  istat = VSseek(vdata_id,pointer.key[0]);
  istat = VSread(vdata_id,(uchar8 *) databuf,nrec,FULL_INTERLACE);
 
  pntr = databuf;
  memcpy(&crosscheck,pntr,HOPTR_FIELD1_SIZE);
  if (crosscheck != pointer.key[3])
    se("\nError in read_spectra : \nthe crosscheck values do not match. \n");
  pntr += HOPTR_FIELD1_SIZE;
  memcpy(buffer, pntr, ncols*HOPTR_FIELD2_SIZE);
  
  free(databuf);
 
  return(istat);
}

/*****************************************************************
**                 read_smr                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.          **
**      nrec       :    number of records to read from the Vdata**
** cont_id    : Vdata identifier of the container we    **
**   wish to read from.   **
** Output parameters:                                           **
**      smrdata    :    OdinScan structure for the retrieved  **
**   data.                          **
*****************************************************************/
int32 read_smr(int32 file_id,struct ODIN_SMR_STRUCT *smrdata,int32 nrec,int32 cont_id)
{
  int32         istat;
  char          *databuf;
  uint16        offset;
  uint16        sum;
  void          *pntr;
  int           i;
  void          *start;
  int32         vdata_id;
  
  vdata_id = container_id(file_id,cont_id,"r"); 

  /* Allocate the memory */
  databuf = NULL;
  sum = 0;
  for (i = 0; i < NUM_FIELDS_EXPANDED; i++)
    sum += smr_offsets[i][1];

  if ((databuf = (char *) malloc(sum*nrec)) == NULL)
    se("\nError in read_smr : \nwhen allocating memory for databuf. \n");
 
  istat = VSread(vdata_id, databuf,nrec,FULL_INTERLACE);

  pntr = databuf;
  start = smrdata; 

  for(i=0;i<NUM_FIELDS_EXPANDED;i++) {
    offset = smr_offsets[i][1];
    memcpy(start+smr_offsets[i][0],pntr,offset);
    pntr += offset;
  }

  free(databuf);

  return(istat);
}

/*******************************************************************
**                 read_filter                                    **
**                                                                **
** Input parameters:                                              **
**      file_id    :    File identifier.                          **
**      nrec       :    number of records to read from the Vdata  **
**      cont_id    :    Vdata identifier of the container we      **
**                      wish to read from.                        **
** Output parameters:                                             **
**      filterdata    :    OdinFilter structure for the retrieved **
**                      data.                                     **
*******************************************************************/
int32 read_filter(int32 file_id,struct ODIN_FILTER_STRUCT *filterdata,int32 nrec,int32 cont_id)
{
  int32         istat;
  char          *databuf;
  uint16        offset;
  uint16        sum;
  void          *pntr;
  int           i;
  void          *start;
  int32         vdata_id;
 
  vdata_id = container_id(file_id,cont_id,"r");
 
  /* Allocate the memory */
  databuf = NULL;
  sum = 0;
  for (i = 0; i < NUM_FILTER_FIELDS_EXPANDED; i++)
    sum += smr_filter_offsets[i][1];
 
  if ((databuf = (char *) malloc(sum*nrec)) == NULL)
    se("\nError in read_filter : \nwhen allocating memory for databuf. \n");
  
  istat = VSread(vdata_id, databuf,nrec,FULL_INTERLACE);
  
  pntr = databuf;
  start = filterdata;
  
  for(i = 0; i < NUM_FILTER_FIELDS_EXPANDED; i++) {
    offset = smr_filter_offsets[i][1];
    memcpy(start+smr_filter_offsets[i][0],pntr,offset);
    pntr += offset;
  }
 
  free(databuf);
 
  return(istat);
}

/*****************************************************************
**                 write_spectra                                **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      data       :    HOPTR.                                  **
**      buffer     :    buffer containing the data to be stored.**
**      nrec       :    number of records to store in the Vdata.**
** Output parameters:   NONE.                                   **
**                                                              **
*****************************************************************/
int32 write_spectra(int32 file_id,struct HOPTR *pointer,void* buffer,int32 size,int32 size_id,int32 nrec)
{
  int32 istat,vdata_id_hoptr,n_records;
  char *databuf;
  void *pntr;

  /* Get the vdata_id */
  vdata_id_hoptr = container_id(file_id,size_id,"w");

  pointer->key[3] = next_cross_checkID(); /* exposure id */

  databuf = NULL;
  if ((databuf = (char *) malloc(nrec*(size+HOPTR_FIELD1_SIZE))) == NULL)
    se("\nError in write_spectra : \nwhen allocating memory for databuf. \n");

  pntr = databuf;
  memcpy(pntr, &(pointer->key[3]), HOPTR_FIELD1_SIZE);
  pntr += HOPTR_FIELD1_SIZE;
  memcpy(pntr, buffer, size);
  
  istat = VSwrite(vdata_id_hoptr, databuf,nrec,FULL_INTERLACE);
  istat &= VSinquire(vdata_id_hoptr,&n_records,NULL,NULL,NULL,NULL);

  pointer->key[0] = n_records-1;          
  pointer->key[1] = size_id;              /* number of columns */
  pointer->key[2] = nrec;                 /* number of rows, which for the smr is always 1 */

  free(databuf);

  return(istat);
}

/*****************************************************************
**                 container_id                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      cont_id    :    Vdata identifier of the container we    **
**                      wish to read from.                      **
** access_mode: access mode definition.   **
*****************************************************************/
int32 container_id(int32 file_id,int32 cont_id,char *access_mode)
{
  int32 vdata_id = -1;

 switch (cont_id) {
  case  CONT_SMR_HEADER :
   if (vdata_id_num[0] == -1) {
     vdata_id = header_vdata(file_id,vdata_name_tags[0],access_mode);
     vdata_id_num[0] = vdata_id;
   } else {
     vdata_id = vdata_id_num[0];
   }
   break;
  case  SIZE_AOS_1728 :
   if (vdata_id_num[1] == -1) {
     vdata_id = hoptr_vdata(file_id,vdata_name_tags[1],cont_id,access_mode);
     vdata_id_num[1] = vdata_id;
   } else
     vdata_id = vdata_id_num[1];
   break;
  case 2*SIZE_SMR_DATA :
   if (vdata_id_num[2] == -1) {
     vdata_id = hoptr_vdata(file_id,vdata_name_tags[2],cont_id,access_mode);
     vdata_id_num[2] = vdata_id;
   } else
     vdata_id = vdata_id_num[2];
   break;
  case SIZE_SMR_DATA :
   if (vdata_id_num[3] == -1) {
     vdata_id = hoptr_vdata(file_id,vdata_name_tags[3],cont_id,access_mode);
     vdata_id_num[3] = vdata_id;
   } else
     vdata_id = vdata_id_num[3];
   break;
  case CONT_FILTER_DATA :
   if (vdata_id_num[4] == -1) {
     vdata_id = filter_vdata(file_id,vdata_name_tags[4],access_mode);
     vdata_id_num[4] = vdata_id;
   } else
     vdata_id = vdata_id_num[4];
   break;
  default :
   se("\nError in container_id : \nunknown switch %d. \n",cont_id);
 }
 return(vdata_id);
}

/*****************************************************************
**                 header_vdata                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
** vdata_name : Vdata name.    **
**      access_mode:    access mode definition.                 **
*****************************************************************/
int32 header_vdata(int32 file_id,char *vdata_name,char *access_mode)
{
  int32 vdata_id,istat,vdata_ref;
  int i;
  char       *fields = NULL;       
  char     dummy_fields[500];
  char       s[500];

  fields = get_field_names();
 
  if (strcmp(access_mode,"w") == 0) {
    vdata_id = VSattach(file_id,-1,"w");
    /* Define the SMR fields for the HDF file */
    for (i = 0; i < NUM_FIELDS; i++)
      istat &= VSfdefine(vdata_id, smr_fields[i], smr_types[i][0], smr_types[i][1]);
    istat &= VSfdefine(vdata_id, "", -1, 0);
    
    VSsetname(vdata_id, vdata_name);                
    (void) sprintf(s,"%12d",CONT_SMR_HEADER);
    VSsetclass(vdata_id,s);       

    istat = VSsetfields(vdata_id, fields);   

  } else /* We must be reading */ {    
    vdata_ref = VSfind(file_id,vdata_name);
    vdata_id = VSattach(file_id,vdata_ref,"r");
    istat = VSinquire(vdata_id,NULL,NULL,dummy_fields,NULL,NULL);
    if (strcmp(dummy_fields,fields) != 0)
      se("\nError in header_vdata : \nthe header fields in the HDF 
file and those from the software do not agree.");
    istat &= VSsetfields(vdata_id, dummy_fields);       
  }

  if (fields != NULL)
    free(fields);
 
  return(vdata_id);
}

/*****************************************************************
**                 filter_vdata                                 **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      vdata_name :    Vdata name.                             **
**      access_mode:    access mode definition.                 **
*****************************************************************/
int32 filter_vdata(int32 file_id,char *vdata_name,char *access_mode)
{
  int32 vdata_id,istat,vdata_ref;
  int i;
  char       *fields = NULL;
  char       dummy_fields[500];
  char    s[500];
 
  fields = get_filter_field_names();
 
  if (strcmp(access_mode,"w") == 0) {
    vdata_id = VSattach(file_id,-1,"w");
    /* Define the filter fields for the HDF file */
    for (i = 0; i < NUM_FILTER_FIELDS; i++)
      istat &= VSfdefine(vdata_id, smr_filter_fields[i], smr_filter_types[i][0], smr_filter_types[i][1]);
    istat &= VSfdefine(vdata_id, "", -1, 0);
 
    VSsetname(vdata_id, vdata_name);
    (void) sprintf(s,"%12d",CONT_FILTER_DATA);
    VSsetclass(vdata_id,s);
    
    istat = VSsetfields(vdata_id, fields);
  } else /* We must be reading */ {
    vdata_ref = VSfind(file_id,vdata_name);
    vdata_id = VSattach(file_id,vdata_ref,"r");
    istat = VSinquire(vdata_id,NULL,NULL,dummy_fields,NULL,NULL);
    if (strcmp(dummy_fields,fields) != 0)
      se("\nError in header_vdata : \nthe header fields in the HDF file and those from the software do not agree.");
    istat &= VSsetfields(vdata_id, dummy_fields);
  }
 
  if (fields != NULL) free(fields);
 
  return(vdata_id);
}

/*****************************************************************
**                 hoptr_vdata                                  **
**                                                              **
** Input parameters:                                            **
**      file_id    :    File identifier.                        **
**      vdata_name :    Vdata name.                             **
** order    : Vdata filed order.   **
**      access_mode:    access mode definition.                 **
*****************************************************************/
int32 hoptr_vdata(int32 file_id, char *vdata_name,int32 order,char *access_mode)
{
  int32 vdata_id,istat,vdata_ref;
  char       dummy_fields[500];
  char    s[500];

  if (strcmp(access_mode,"w") == 0) {
    vdata_id = VSattach(file_id,-1,"w");
    istat = VSfdefine(vdata_id,HOPTR_FIELD1,  HOPTR_FIELD1_TYPE, 1);
    istat &= VSfdefine(vdata_id, HOPTR_FIELD2, HOPTR_FIELD2_TYPE, order);
    istat &= VSfdefine(vdata_id, "", -1, 0);
    istat &= VSsetname(vdata_id, vdata_name);
    (void) sprintf(s,"%12d",0);
    istat &= VSsetclass(vdata_id,s);
    istat &= VSsetfields(vdata_id,HOPTR_FIELDS);
  } else  /* We must be reading */ {
    vdata_ref = VSfind(file_id,vdata_name);
    vdata_id = VSattach(file_id,vdata_ref,"r");
    istat = VSinquire(vdata_id,NULL,NULL,dummy_fields,NULL,NULL);
    if (strcmp(dummy_fields,HOPTR_FIELDS) != 0)
      se("\nError in hoptr_vdata : \nthe hoptr fields in the HDF file and those from the software do not agree.");
    istat &= VSsetfields(vdata_id, dummy_fields);        
  }

  return(vdata_id);
}

/*****************************************************************
**                 write_smr                                 **
**                                                              **
** Input parameters:                                            **
** file_id         : Vdata file identifier.  **
** ODIN_SMR_STRUCT : The Odin scan structure.  **
** databuf  : Buffer for records to be stored. **
** nrec  : Number of records to be stored. **
** Output parameters:        **
** Returns a boolean indicating whether the HDF routine **
** VSwrite was successful.     **
**                                                              **
*****************************************************************/
int32 write_smr(int32 file_id,struct ODIN_SMR_STRUCT *smrdata,int32 nrec,int32 cont_id)
{
  int32         istat;
  int32       vdata_id;
  uint16        sum = 0;               
  uint16        offset;
  void          *pntr;
  void          *databuf;
  void          *start;
  int           i;

  vdata_id = container_id(file_id,cont_id,"w");

  databuf = NULL;
  sum = 0;
  for (i = 0; i < NUM_FIELDS_EXPANDED; i++)
    sum += smr_offsets[i][1];
  if ((databuf = (char *) malloc(sum*nrec)) == NULL)
    se("\nError in write_smr : \nwhen allocating memory for databuf. \n");

  smrdata->TBD.version[0] = 0;
  smrdata->TBD.version[1] = 0;
  smrdata->TBD.version[2] = 0;
  smrdata->TBD.version[3] = 0;

  pntr = databuf;
  start = smrdata;

  for (i = 0; i < NUM_FIELDS_EXPANDED; i++) {
    offset = smr_offsets[i][1];
    memcpy(pntr, start + smr_offsets[i][0], offset);
    pntr += offset;
  }

  istat = VSwrite(vdata_id, databuf, nrec, FULL_INTERLACE);

  free(databuf);

  return(istat);
}


/*****************************************************************
**                 get_filter_field_names                       **
**                                                              **
** Input parameters:    NONE                                    **
** Output parameters:   Returns a one dimensional string.       **
**                                                              **
*****************************************************************/
char *get_filter_field_names()
{
  char *databuf;
  int sum,i;
  sum = 0;
 
  for (i = 0; i < NUM_FILTER_FIELDS; i++)
    sum += strlen(smr_filter_fields[i]);
 
  if ((databuf = (char *) malloc(sum*sizeof(char)+NUM_FILTER_FIELDS)) == NULL)
    se("\nError in get_filter_field_names : \nwhen allocating memory for databuf. \n");
  
  strcpy(databuf,smr_filter_fields[0]);
  for (i = 1; i < NUM_FILTER_FIELDS; i++) {
    strcat(databuf,",");
    strcat(databuf,smr_filter_fields[i]);
  }
 
  return (databuf);
}

/*****************************************************************
**                 get_field_names    **
**        **
** Input parameters: NONE     **
** Output parameters:   Returns a one dimensional string. **
**        **
*****************************************************************/
char *get_field_names()
{
  char *databuf;
  int sum,i;
  sum = 0;

  for (i = 0; i < NUM_FIELDS; i++) sum += strlen(smr_fields[i]);

  if ((databuf = (char *) malloc(sum*sizeof(char)+NUM_FIELDS)) == NULL)
    se("\nError in get_field_names : \nwhen allocating memory for databuf. \n"); 

  strcpy(databuf,smr_fields[0]);
  for (i = 1; i < NUM_FIELDS; i++) {
    strcat(databuf,",");
    strcat(databuf,smr_fields[i]);
  }

  return(databuf);
}

/*****************************************************************
**  next_cross_checkID    **
**                                                              **
** Input parameters:    NONE                                    **
** Output/Description:                                          **
**      Generates a number which is going to be unique for this **
**      application.                                            **
**      It is used to tag each HOPTR                            **
**      structure with an ID code that allows the header        **
**      container and the HOPTR                                 **
**      container to verify they are properly synchronized.     **
**                                                              **
**      The following function was taken from Nick Lloyds       **
**      ONYX software.                                          **
*****************************************************************/
unsigned long next_cross_checkID()
{
  static unsigned long exposureid = 0;
  
  if (exposureid == 0) {
    unsigned long thetime;
    thetime = time(&thetime);
    exposureid = (unsigned long)thetime;
  }
  return exposureid++;
}

/*****************************************************************
**              se                                              **
**      se : Screen Error handling routine.                     **
*****************************************************************/
void se(char *format, ...)
{
  va_list ptr; /* get an arg pointer */
 
  /* initialise ptr to point to the first argument after the format string */
  
  va_start(ptr,format);
  
  /* print out the message */
  vprintf(format,ptr);
 
  va_end(ptr);
  exit(1); /* exit to the shell */
}
