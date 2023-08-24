#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "odinscan.h"
#include "bintable.h"
#include "fitsio.h"

static char rcsid[] = "$Id";

Column fitstable[] = {
  { "Version"       , "1I"   , ""       ,    1, sizeof(short) },
  { "Level"         , "1I"   , ""       ,    1, sizeof(short) },
  { "Quality"       , "1J"   , ""       ,    1, sizeof(long)  },
  { "STW"           , "1J"   , ""       ,    1, sizeof(long)  },
  { "MJD"           , "1D"   , ""       ,    1, sizeof(double)},
  { "Orbit"         , "1D"   , ""       ,    1, sizeof(double)},
  { "LST"           , "1E"   , ""       ,    1, sizeof(float) },
  { "Source"        , "32A"  , ""       ,   32, sizeof(char)  },
  { "Discipline"    , "1I"   , ""       ,    1, sizeof(short) },
  { "Topic"         , "1I"   , ""       ,    1, sizeof(short) },
  { "Spectrum"      , "1I"   , ""       ,    1, sizeof(short) },
  { "ObsMode"       , "1I"   , ""       ,    1, sizeof(short) },
  { "Type"          , "1I"   , ""       ,    1, sizeof(short) },
  { "Frontend"      , "1I"   , ""       ,    1, sizeof(short) },
  { "Backend"       , "1I"   , ""       ,    1, sizeof(short) },
  { "SkyBeamHit"    , "1I"   , ""       ,    1, sizeof(short) },
  { "RA2000"        , "1E"   , "degrees",    1, sizeof(float) },
  { "Dec2000"       , "1E"   , "degrees",    1, sizeof(float) },
  { "VSource"       , "1E"   , "m/s"    ,    1, sizeof(float) },
  { "MapXoff"       , "1E"   , "degrees",    1, sizeof(float) },
  { "MapYoff"       , "1E"   , "degrees",    1, sizeof(float) },
  { "MapTilt"       , "1E"   , "degrees",    1, sizeof(float) },
  { "Qtarget"       , "4D"   , ""       ,    4, sizeof(double)},
  { "Qachieved"     , "4D"   , ""       ,    4, sizeof(double)},
  { "Qerror"        , "3D"   , "degrees",    3, sizeof(double)},
  { "GPSpos"        , "3D"   , "m"      ,    3, sizeof(double)},
  { "GPSvel"        , "3D"   , "m/s"    ,    3, sizeof(double)},
  { "SunPos"        , "3D"   , "m"      ,    3, sizeof(double)},
  { "MoonPos"       , "3D"   , "m"      ,    3, sizeof(double)},
  { "SunZD"         , "1E"   , "degrees",    1, sizeof(float) },
  { "Vgeo"          , "1E"   , "m/s"    ,    1, sizeof(float) },
  { "Vlsr"          , "1E"   , "m/s"    ,    1, sizeof(float) },
  { "Tcal"          , "1E"   , "K"      ,    1, sizeof(float) },
  { "Tsys"          , "1E"   , "K"      ,    1, sizeof(float) },
  { "SBpath"        , "1E"   , "m"      ,    1, sizeof(float) },
  { "LOFreq"        , "1D"   , "Hz"     ,    1, sizeof(double)},
  { "SkyFreq"       , "1D"   , "Hz"     ,    1, sizeof(double)},
  { "RestFreq"      , "1D"   , "Hz"     ,    1, sizeof(double)},
  { "MaxSuppression", "1D"   , "Hz"     ,    1, sizeof(double)},
  { "SodaVersion"   , "1D"   , ""       ,    1, sizeof(double)},
  { "FreqRes"       , "1D"   , "Hz"     ,    1, sizeof(double)},
  { "FreqCal"       , "4D"   , "Hz"     ,    4, sizeof(double)},
  { "IntMode"       , "1J"   , ""       ,    1, sizeof(int)   },
  { "IntTime"       , "1E"   , "s"      ,    1, sizeof(float) },
  { "EffTime"       , "1E"   , "s"      ,    1, sizeof(float) },
  { "Channels"      , "1J"   , ""       ,    1, sizeof(int)   },
  { "data"          , "1728E", ""       , 1728, sizeof(float) }
};

#ifndef WORDS_BIGENDIAN
#define BYTESWAP
#endif

#ifdef BYTESWAP
void swapbytes(char *p, int n)
{
  register char swap;
  register int i, j;

  for (i = 0, j = n-1; i < j; i++, j--) {
    swap = p[i];
    p[i] = p[j];
    p[j] = swap;
  }
}

void swapscan(char *scan)
{
  int i, j, offset, size;
  char *ptr;

  offset = 0;
  ptr = scan;
  for (i = 0; i < ODINSCANMEMBERS; i++) {
    size = fitstable[i].size;
    for (j = 0; j < fitstable[i].dim; j++) {
      /* printf("[%d] swap %d bytes starting at 0x%08x\n",
	 j,size,ptr+offset); */
      if (size > 1) swapbytes(ptr+offset, size);
      offset += size;
    }
  }
}
#endif /* BYTESWAP */

int rowSize()
{
  int i, size;

  size = 0;
  for (i = 0; i < ODINSCANMEMBERS; i++) {
    size += fitstable[i].size * fitstable[i].dim;
  }

  return size;
}

void getColumn(char *member, char *buffer, int i)
{
  int total = 0;
  int size, dim, k;
  char *ptr;

  total = 0;
  for (k = 0; k < i; k++) {
    size = fitstable[k].size;
    dim  = fitstable[k].dim;
    total += size*dim;
  }
  size = fitstable[i].size;
  dim  = fitstable[i].dim;
  ptr = buffer+total;
  memcpy(member, ptr, size*dim);
}

void putColumn(char *member, char *buffer, int i)
{
  int total;
  int size, dim, k;
  char *ptr;

  total = 0;
  for (k = 0; k < i; k++) {
    size = fitstable[k].size;
    dim  = fitstable[k].dim;
    total += size*dim;
  }
  size = fitstable[i].size;
  dim  = fitstable[i].dim;
  ptr = buffer+total;
  memcpy(ptr, member, size*dim);
}

void encodescan(struct OdinScan *scan, char *buffer)
{
  int i = 0;

  memset((char *)scan, '\0', sizeof(struct OdinScan));
#ifdef BYTESWAP
  swapscan(buffer);
#endif
  getColumn((char *)&(scan->Version), buffer, i);        i++;
  getColumn((char *)&(scan->Level), buffer, i);          i++;
  getColumn((char *)&(scan->Quality), buffer, i);        i++;
  getColumn((char *)&(scan->STW), buffer, i);            i++;
  getColumn((char *)&(scan->MJD), buffer, i);            i++;
  getColumn((char *)&(scan->Orbit), buffer, i);          i++;
  getColumn((char *)&(scan->LST), buffer, i);            i++;
  getColumn((char *)&(scan->Source), buffer, i);         i++;
  getColumn((char *)&(scan->Discipline), buffer, i);     i++;
  getColumn((char *)&(scan->Topic), buffer, i);          i++;
  getColumn((char *)&(scan->Spectrum), buffer, i);       i++;
  getColumn((char *)&(scan->ObsMode), buffer, i);        i++;
  getColumn((char *)&(scan->Type), buffer, i);           i++;
  getColumn((char *)&(scan->Frontend), buffer, i);       i++;
  getColumn((char *)&(scan->Backend), buffer, i);        i++;
  getColumn((char *)&(scan->SkyBeamHit), buffer, i);     i++;
  getColumn((char *)&(scan->RA2000), buffer, i);         i++;
  getColumn((char *)&(scan->Dec2000), buffer, i);        i++;
  getColumn((char *)&(scan->VSource), buffer, i);        i++;
  getColumn((char *)&(scan->u.map.Xoff), buffer, i);     i++;
  getColumn((char *)&(scan->u.map.Yoff), buffer, i);     i++;
  getColumn((char *)&(scan->u.map.Tilt), buffer, i);     i++;
  getColumn((char *)&(scan->Qtarget), buffer, i);        i++;
  getColumn((char *)&(scan->Qachieved), buffer, i);      i++;
  getColumn((char *)&(scan->Qerror), buffer, i);         i++;
  getColumn((char *)&(scan->GPSpos), buffer, i);         i++;
  getColumn((char *)&(scan->GPSvel), buffer, i);         i++;
  getColumn((char *)&(scan->SunPos), buffer, i);         i++;
  getColumn((char *)&(scan->MoonPos), buffer, i);        i++;
  getColumn((char *)&(scan->SunZD), buffer, i);          i++;
  getColumn((char *)&(scan->Vgeo), buffer, i);           i++;
  getColumn((char *)&(scan->Vlsr), buffer, i);           i++;
  getColumn((char *)&(scan->Tcal), buffer, i);           i++;
  getColumn((char *)&(scan->Tsys), buffer, i);           i++;
  getColumn((char *)&(scan->SBpath), buffer, i);         i++;
  getColumn((char *)&(scan->LOFreq), buffer, i);         i++;
  getColumn((char *)&(scan->SkyFreq), buffer, i);        i++;
  getColumn((char *)&(scan->RestFreq), buffer, i);       i++;
  getColumn((char *)&(scan->MaxSuppression), buffer, i); i++;
  getColumn((char *)&(scan->SodaVersion), buffer, i);    i++;
  getColumn((char *)&(scan->FreqRes), buffer, i);        i++;
  getColumn((char *)&(scan->FreqCal), buffer, i);        i++;
  getColumn((char *)&(scan->IntMode), buffer, i);        i++;
  getColumn((char *)&(scan->IntTime), buffer, i);        i++;
  getColumn((char *)&(scan->EffTime), buffer, i);        i++;
  getColumn((char *)&(scan->Channels), buffer, i);       i++;
  getColumn((char *)&(scan->data), buffer, i);           i++;
}

void decodescan(struct OdinScan *scan, char *buffer)
{
  int i = 0;

  putColumn((char *)&(scan->Version), buffer, i);        i++;
  putColumn((char *)&(scan->Level), buffer, i);          i++;
  putColumn((char *)&(scan->Quality), buffer, i);        i++;
  putColumn((char *)&(scan->STW), buffer, i);            i++;
  putColumn((char *)&(scan->MJD), buffer, i);            i++;
  putColumn((char *)&(scan->Orbit), buffer, i);          i++;
  putColumn((char *)&(scan->LST), buffer, i);            i++;
  putColumn((char *)&(scan->Source), buffer, i);         i++;
  putColumn((char *)&(scan->Discipline), buffer, i);     i++;
  putColumn((char *)&(scan->Topic), buffer, i);          i++;
  putColumn((char *)&(scan->Spectrum), buffer, i);       i++;
  putColumn((char *)&(scan->ObsMode), buffer, i);        i++;
  putColumn((char *)&(scan->Type), buffer, i);           i++;
  putColumn((char *)&(scan->Frontend), buffer, i);       i++;
  putColumn((char *)&(scan->Backend), buffer, i);        i++;
  putColumn((char *)&(scan->SkyBeamHit), buffer, i);     i++;
  putColumn((char *)&(scan->RA2000), buffer, i);         i++;
  putColumn((char *)&(scan->Dec2000), buffer, i);        i++;
  putColumn((char *)&(scan->VSource), buffer, i);        i++;
  putColumn((char *)&(scan->u.map.Xoff), buffer, i);     i++;
  putColumn((char *)&(scan->u.map.Yoff), buffer, i);     i++;
  putColumn((char *)&(scan->u.map.Tilt), buffer, i);     i++;
  putColumn((char *)&(scan->Qtarget), buffer, i);        i++;
  putColumn((char *)&(scan->Qachieved), buffer, i);      i++;
  putColumn((char *)&(scan->Qerror), buffer, i);         i++;
  putColumn((char *)&(scan->GPSpos), buffer, i);         i++;
  putColumn((char *)&(scan->GPSvel), buffer, i);         i++;
  putColumn((char *)&(scan->SunPos), buffer, i);         i++;
  putColumn((char *)&(scan->MoonPos), buffer, i);        i++;
  putColumn((char *)&(scan->SunZD), buffer, i);          i++;
  putColumn((char *)&(scan->Vgeo), buffer, i);           i++;
  putColumn((char *)&(scan->Vlsr), buffer, i);           i++;
  putColumn((char *)&(scan->Tcal), buffer, i);           i++;
  putColumn((char *)&(scan->Tsys), buffer, i);           i++;
  putColumn((char *)&(scan->SBpath), buffer, i);         i++;
  putColumn((char *)&(scan->LOFreq), buffer, i);         i++;
  putColumn((char *)&(scan->SkyFreq), buffer, i);        i++;
  putColumn((char *)&(scan->RestFreq), buffer, i);       i++;
  putColumn((char *)&(scan->MaxSuppression), buffer, i); i++;
  putColumn((char *)&(scan->SodaVersion), buffer, i);    i++;
  putColumn((char *)&(scan->FreqRes), buffer, i);        i++;
  putColumn((char *)&(scan->FreqCal), buffer, i);        i++;
  putColumn((char *)&(scan->IntMode), buffer, i);        i++;
  putColumn((char *)&(scan->IntTime), buffer, i);        i++;
  putColumn((char *)&(scan->EffTime), buffer, i);        i++;
  putColumn((char *)&(scan->Channels), buffer, i);       i++;
  putColumn((char *)&(scan->data), buffer, i);           i++;

#ifdef BYTESWAP
  swapscan(buffer);
#endif
}

void dump(struct OdinScan *s)
{
  printf("-------------\n");
  printf("%04x\n", s->Version);
  printf("%04x\n", s->Level);
  printf("%08x\n", s->Quality);
  printf("%08x\n", s->STW);
  printf("%f\n", s->MJD);
  printf("%f\n", s->Orbit);
  printf("%f\n", s->LST);
  printf("%s\n", s->Source);
  printf("%d\n", s->Discipline);
  printf("%d\n", s->Topic);
  printf("%d\n", s->Spectrum);
  printf("%d\n", s->ObsMode);
  printf("%d\n", s->Type);
  printf("%d\n", s->Frontend);
  printf("%d\n", s->Backend);
  printf("%08x\n", s->SkyBeamHit);
  printf("%f\n", s->RA2000);
  printf("%f\n", s->Dec2000);
  printf("%f\n", s->VSource);
  printf("%f %f %f\n", s->u.map.Xoff,s->u.map.Yoff,s->u.map.Tilt );
  printf("%f %f %f %f\n", s->Qtarget[0],s->Qtarget[1],s->Qtarget[2],s->Qtarget[3]);
  printf("%f %f %f %f\n", s->Qachieved[0],s->Qachieved[1],s->Qachieved[2],s->Qachieved[3]);
  printf("%f %f %f\n", s->Qerror[0],s->Qerror[1],s->Qerror[2]);
  printf("%f %f %f\n", s->GPSpos[0],s->GPSpos[1],s->GPSpos[2]);
  printf("%f %f %f\n", s->GPSvel[0],s->GPSvel[1],s->GPSvel[2]);
  printf("%f %f %f\n", s->SunPos[0],s->SunPos[1],s->SunPos[2]);
  printf("%f %f %f\n", s->MoonPos[0],s->MoonPos[1],s->MoonPos[2]);
  printf("%f\n", s->SunZD);
  printf("%f\n", s->Vgeo);
  printf("%f\n", s->Vlsr);
  printf("%f\n", s->Tcal);
  printf("%f\n", s->Tsys);
  printf("%f\n", s->SBpath);
  printf("%f\n", s->LOFreq);
  printf("%f\n", s->SkyFreq);
  printf("%f\n", s->RestFreq);
  printf("%f\n", s->MaxSuppression);
  printf("%f\n", s->SodaVersion);
  printf("%f\n", s->FreqRes);
  printf("%f %f %f %f\n", s->FreqCal[0],s->FreqCal[0],s->FreqCal[2],s->FreqCal[3]);
  printf("%d\n", s->IntMode);
  printf("%f\n", s->IntTime);
  printf("%f\n", s->EffTime);
  printf("%d\n", s->Channels);
}

void writebintable(fitsfile *fptr)
{
  int status, nfound, i;
  long firstrow, firstelem;

  int tfields   = ODINSCANMEMBERS;   /* one column per structure member */
  long nrows    = 0;                 /* table will have n rows          */
  long naxes[2];

  char extname[] = "ODIN_Binary";             /* extension name */

  /* define the name, datatype, and physical units for the various columns */
  char *ttype[ODINSCANMEMBERS];
  char *tform[ODINSCANMEMBERS];
  char *tunit[ODINSCANMEMBERS];

  status = 0;
  for (i = 0; i < ODINSCANMEMBERS; i++) {
    ttype[i] = fitstable[i].type;
    tform[i] = fitstable[i].form;
    tunit[i] = fitstable[i].unit;
  }

  /*   for (i = 0; i < ODINSCANMEMBERS; i++) { */
  /*     printf("%-16s %-8s %-8s\n", ttype[i], tform[i], tunit[i]); */
  /*   } */

  /* append a new empty binary table onto the FITS file */
  if (fits_create_tbl(fptr, BINARY_TBL, nrows, tfields, ttype, tform,
			tunit, extname, &status))
    printerror(status);

  firstrow  = 1;  /* first row in table to write   */
  firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
    printerror(status);

  /* printf("NAXIS = %d %d\n", naxes[0], naxes[1]); */
  if (naxes[0] != sizeof(struct OdinScan)) {
    fprintf(stderr,"warning: size of row doesn't match size of structure!\n");
  }

  return;
}

int readbintable(fitsfile *fptr)
{
  int status, nfound;
  long firstrow, firstelem, naxes[2];

  status=0;

  firstrow  = 1;  /* first row in table to write   */
  firstelem = 1;  /* first element in row  (ignored in ASCII tables) */

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status) )
    printerror(status);
  
  return naxes[1];
}

/*****************************************************/
/* Print out cfitsio error messages and exit program */
/*****************************************************/
void printerror(int status)
{
  char status_str[FLEN_STATUS], errmsg[FLEN_ERRMSG];
  
  if (status)
    fprintf(stderr, "\n*** Error occurred during program execution ***\n");

  fits_get_errstatus(status, status_str);   /* get the error description */
  fprintf(stderr, "\nstatus = %d: %s\n", status, status_str);

  /* get first message; null if stack is empty */
  if (fits_read_errmsg(errmsg)) {
    fprintf(stderr, "\nError message stack:\n");
    fprintf(stderr, " %s\n", errmsg);
    
    while (fits_read_errmsg(errmsg))  /* get remaining messages */
      fprintf(stderr, " %s\n", errmsg);
  }

  exit(status);       /* terminate the program, returning error status */
}
