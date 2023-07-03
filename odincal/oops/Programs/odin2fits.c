#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <time.h>

#include "odinlib.h"
#include "drplib.h"
#include "odinscan.h"
#include "options.h"
#include "vector.h"
#include "accessor.h"

#include "fitsio.h"

static char rcsid[] = "$Id$";

static int scanlist = 0;
static char listname[MAXNAMLEN+1];    /* name for list of scan file(s) */

char PROGRAM_NAME[] = "odin2fits";
char description[] = "convert Odin scans to FITS files";
char required[] = "scan(s)";
struct _options options[] = {
  { "-list name",      "specify file name for list of scans" },
  { "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;
  optarg = NULL;

  if (!strcmp(opt, "list")) {
    scanlist = 1;
    GetOption(&optarg, pargc, pargv);
    strcpy(listname, optarg);
    return;
  }
  if (!strcmp(opt, "help")) {
    Help();
    exit(0);
  }
  Syntax(**pargv);
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

int writefits(struct OdinScan *scan, char *sname)
{
  static double f[MAXCHANNELS];
  fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
  long  fpixel, nelements;
  int bitpix;
  static char filename[MAXNAMLEN+1];    /* name for new FITS file */
  char  cvar[8], *ptr;
  char  cvarlong[16];
  double rvar, elevation;
  cVector earth, mb, e;
  pVector beam;
  double vres;
  int i, j, n, status, simple, extend, nchan;
  float chanmin,chanmax, bscale, bzero;
  long pcount, gcount, naxis, naxes[3];
  char tstring[80];
  char cut[10];
  struct tm *lt;
  time_t utsec;
  time_t lst_daysec;

  if (!(scan->Quality & ISORTED)) freqsort(scan, f);
  if (scan->Backend == AOS) {
      if (!(scan->Quality & ILINEAR)) redres(scan, f, 0.62e6);
  }
  status = 0;

  /*    strcpy(filename, SaveName(scan)); */
  /* If filename has directory information, strip that off. */
  ptr = strrchr(sname, '/');
  if (ptr != NULL) ptr++;
  else             ptr = sname;
  strcpy(filename, ptr);

  /*    printf("SCAN filename: '%s'\n", filename); */
  /* If filename has extension, replace it with 'fits' */
  /* else just append '.fits'                          */
  ptr = strrchr(filename, '.');
  if (ptr) *ptr = '\0';
  strcat(filename, ".fits");
  printf("FITS filename: '%s'\n", filename);

  remove(filename); /* Delete old file if it already exists */
  fits_create_file(&fptr, filename, &status);
  if (status) return status;

  bitpix =  -32;
  naxis =  3;
  naxes[0] = scan->Channels;
  naxes[1] = 1;
  naxes[2] = 1;
  pcount = 0;
  gcount = 1;
  simple = TRUE;
  extend = FALSE;

  /* scale data */
  nchan = scan->Channels;
  chanmax = scan->data[0];
  chanmin = scan->data[0];
  for (j=1 ; j<nchan ; j++) {
    /* printf (" %8.6f",scan->data[j]); */
    if (scan->data[j] < chanmin)  chanmin = scan->data[j];
    if (scan->data[j] > chanmax)  chanmax = scan->data[j];
  } 
  bscale = chanmax-chanmin;
  bzero  = (chanmax-chanmin)/2;

  /* debug prints
     printf ("Min/Max channel values: %8.6f %8.6f\n",chanmin,chanmax);
     printf ("bscale: %8.6f\n",bscale);
     printf ("bzero : %8.6f\n",bzero);
     printf ("source : %s\n",scan->Source);
  */
  utsec = mjd2utc(scan->MJD);
  lst_daysec = (int)scan->LST;
  /*
    printf ("utsec : %10d\n",utsec);
    printf ("localt : %s\n",asctime(lt)); 
  */
  

  /* for lower sideband observations we want negative FreqRes */
  if (scan->SkyFreq < scan->LOFreq) scan->FreqRes = -fabs(scan->FreqRes);
  else                              scan->FreqRes =  fabs(scan->FreqRes);

  /* start writing the keywords */
  fits_write_grphdr(fptr, simple, bitpix, naxis, naxes, 
		    pcount, gcount, extend, &status);
  fits_write_key(fptr, TSTRING, "ORIGIN  ", "OSO-ODIN", 
		     "at Onsala Space Observatory, Sweden           ", 
		 &status);
  sprintf (cvar,"%d",scan->Level);
  fits_write_key_str(fptr, "LEVEL   ", cvar, 
		     "level of data reduction applied               ", 
		 &status);
  sprintf (cvar,"%d",scan->Topic);
  fits_write_key_str(fptr, "OBSERVER", cvar, 
		     "observer(topical team)                        ", 
		 &status);
  fits_write_key_str(fptr, "OBJECT  ", scan->Source, 
		     "object of observation                         ", 
		 &status);
  sprintf (cvar,"%s",Backend(scan));
  fits_write_key_str(fptr, "INSTRUME", cvar, 
		     "backend receiver (AC1,AC2,AOS)                ", 
		 &status);
  sprintf (cvar,"%s",Frontend(scan));
  fits_write_key_str(fptr, "FRONTEND", cvar, 
		     "frontend receiver (REC_555,495,572,549,119)   ", 
		 &status);
  sprintf (cvarlong, "ODIN %s/%s", Frontend(scan), Backend(scan));
  fits_write_key_str(fptr, "TELESCOP", cvarlong, 
		     "satellite frontend/backend (for CLASS)        ", 
		 &status);

  sprintf (cvar,"%s",ObsMode(scan));
  fits_write_key_str(fptr, "OBSMODE ", cvar, 
		     "mode of observation (TPW,SSW,LSW,FSW)         ", 
		 &status);
  sprintf (cvar,"%s",Type(scan));
  fits_write_key_str(fptr, "OBSTYPE ", cvar, 
		     "type of obs. (SIG,REF,CAL,CMB,DRK,SK1,SK2)    ", 
		 &status);
  sprintf (cvar,"%d",scan->Spectrum);
  fits_write_key_str(fptr, "SPECTRUM", cvar, 
		     "spectrum number in this orbit                 ", 
		 &status);
  fits_write_key_flt(fptr, "BSCALE  ", bscale, 6, 
		     "real = tape*BSCALE + BZERO                    ", 
		     &status);
  fits_write_key_flt(fptr, "BZERO   ", bzero, 6, 
		     "bias added                                    ", 
		     &status);
  fits_write_key_str(fptr, "BUNIT   ", "K", 
		     "brightness unit                               ", 
		 &status);
  fits_write_key_flt(fptr, "DATAMAX ", chanmax, 6, 
		     "maximum amplitude of data                     ", 
		     &status);
  fits_write_key_flt(fptr, "DATAMIN ", chanmin, 6, 
		     "minimum amplitude of data                     ", 
		     &status);
  lt = gmtime(&utsec); 
  fits_date2str(lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday, 
		cut, &status);
  fits_write_key_str(fptr, "DATE-OBS", cut, 
		     "date of data acquisition          (yyyy-mm-dd)", 
		 &status);
  fits_time2str(0, 0, 0, lt->tm_hour, lt->tm_min, lt->tm_sec, 1,
		cut, &status);
  fits_write_key_str(fptr, "UTC     ", cut, 
		     "universal time at end of observation          ", 
		 &status);
  lt = gmtime(&lst_daysec); 
  fits_time2str(0, 0, 0, lt->tm_hour, lt->tm_min, lt->tm_sec, 1,
		cut, &status);
  fits_write_key_str(fptr, "LST     ", cut,  
		     "sidereal time at end of observation           ", 
		 &status);
  fits_write_key_fixdbl(fptr, "STW     ", (float) scan->STW, 0, 
		     "satellite time word                           ", 
		 &status);
  rvar = scan->MJD + 2400000.5;
  fits_write_key_fixdbl(fptr, "JDATE   ", rvar, 6, 
		     "julian date at end of observation             ", 
		     &status);
  
  fits_write_key_fixdbl(fptr, "ORBIT  ", scan->Orbit, 6, 
		     "number of orbit plus fraction                 ", 
		     &status);
  rvar = 2000.0;
  fits_write_key_fixdbl(fptr, "EQUINOX ", rvar, 4, 
		     "equinox of coordinates                        ", 
		     &status);
  fits_write_key(fptr, TSTRING, "CTYPE1  ", "FREQ    ", 
		     "f(i) = RESTFREQ +                         (Hz)", 
		 &status);
  rvar = 0.0;
  fits_write_key_fixdbl(fptr, "CRVAL1  ", rvar, 4, 
		     "    CRVAL1 +             offset frequency (Hz)", 
		     &status);
  fits_write_key_dbl(fptr, "CDELT1  ", scan->FreqRes,  6, 
		     "    CDELT1 *         frequency resolution (Hz)", 
		     &status);
  fits_write_key_fixdbl(fptr, "CRPIX1  ", (double)CenterCh(scan)+1.0, 1, 
		     "    (i-CRPIX1)               reference channel", 
			&status);
  fits_write_key(fptr, TSTRING, "CTYPE2  ", "RA      ", 
		     "longitude at reference (epoch) =         (deg)", 
		 &status);
  rvar = scan->RA2000 - scan->u.map.Xoff/cos(scan->Dec2000*M_PI/180.0);
  fits_write_key_flt(fptr, "CRVAL2  ", rvar, 6, 
		     "    CRVAL2 +                reference position", 
		     &status);
  rvar = scan->u.map.Xoff;
  fits_write_key_fixdbl(fptr, "CDELT2  ", rvar, 4, 
		     "                                              ", 
			&status);
  rvar = 0.0;     /* was 1.0, but CLASS wants 0.0 */
  fits_write_key_fixdbl(fptr, "CRPIX2  ", rvar, 4, 
		     "                                              ", 
			&status);
  fits_write_key(fptr, TSTRING, "CTYPE3  ", "DEC", 
		     "latitude at reference (epoch) =          (deg)", 
		 &status);
  rvar = scan->Dec2000 - scan->u.map.Yoff;
  fits_write_key_flt(fptr, "CRVAL3  ", rvar, 6, 
		     "    CRVAL3 +                reference position", 
		     &status);
  rvar = scan->u.map.Yoff;
  fits_write_key_fixdbl(fptr, "CDELT3  ", rvar, 4, 
		     "                                              ", 
			&status);
  rvar = 0.0;     /* was 1.0, but CLASS wants 0.0 */
  fits_write_key_fixdbl(fptr, "CRPIX3  ", rvar, 4, 
		     "                                              ", 
			&status);
  fits_write_key_fixflt(fptr, "RA      ", scan->RA2000, 6, 
			"object apparent right ascension          (deg)", 
			&status);
  fits_write_key_fixflt(fptr, "DEC     ", scan->Dec2000, 6, 
			"object apparent declination              (deg)", 
			&status);
  fits_write_key_fixflt(fptr, "RAOFF   ", scan->u.map.Xoff, 6, 
			"offset in right ascension                (deg)", 
			&status);
  fits_write_key_fixflt(fptr, "DECOFF  ", scan->u.map.Yoff, 6, 
			"offset in declination                    (deg)", 
			&status);
  fits_write_key_fixflt(fptr, "OBSTIME ", scan->IntTime, 3, 
			"true observed integration time           (sec)", 
			&status);
  fits_write_key_fixflt(fptr, "INTTIME ", scan->EffTime, 3, 
			"effective integration time               (sec)", 
			&status);
  fits_write_key_fixflt(fptr, "TSYS    ", scan->Tsys, 3, 
			"chopper wheel system noise temperature     (K)", 
			&status);
  fits_write_key_fixflt(fptr, "TCAL    ", scan->Tcal, 3, 
			"calibration temperature                    (K)", 
			&status);
  fits_write_key_fixdbl(fptr, "VLSR   ", scan->VSource,  1, 
		     "lsr velocity of source                   (m/s)", 
		     &status);
  fits_write_key_fixdbl(fptr, "VSATLSR", scan->Vlsr,  1, 
		     "velocity of satellite w.r.t. LSR         (m/s)", 
		     &status);
  fits_write_key_fixdbl(fptr, "VSATGEO", scan->Vgeo,  1, 
		     "velocity of satellite w.r.t. Earth       (m/s)", 
		     &status);

  quat2eul(scan->Qachieved, e);
  beam.l =  e[0];
  beam.b = -e[1];
  cuv(&beam, mb);
  for (i = 0; i < 3; i++) earth[i] = -scan->GPSpos[i];
  normalise(earth);
  normalise(mb);
  elevation = distance(earth, mb)*180.0/M_PI-90.0;
  fits_write_key_fixdbl(fptr, "ELEVATIO", elevation,  2, 
		     "elevation                                (deg)", 
		     &status);
  rvar = 0.0;
  for (i = 0; i < 3; i++) {
    rvar += scan->Qerror[i]*scan->Qerror[i];
  }
  rvar = sqrt(rvar)/3600.0;
  fits_write_key_fixdbl(fptr, "POINTACC", rvar,  6, 
		     "pointing accuracy                        (deg)", 
		     &status);

  rvar = VelRes(scan);
  fits_write_key_fixdbl(fptr, "DELTAV  ", rvar,  1, 
		     "velocity spacing of channels             (m/s)", 
		     &status);
  rvar = scan->SkyFreq/(1.0-(scan->Vlsr+scan->VSource)/2.99792456e8);
  fits_write_key_fixdbl(fptr, "RESTFREQ", rvar,  1, 
		     "rest frequency                            (Hz)", 
		     &status);
  fits_write_key_fixdbl(fptr, "CENTFREQ", scan->SkyFreq,  1, 
		     "sky frequency                             (Hz)", 
		     &status);
  fits_write_key_fixdbl(fptr, "LOFREQ  ", scan->LOFreq,  1, 
		     "LO frequency                              (Hz)", 
		     &status);
  fits_write_key_fixdbl(fptr, "FTHROW  ", scan->SodaVersion,  1, 
		     "frequency throw                               ", 
		     &status);


  fpixel = 1;                /* first pixel to write      */
  nelements = naxes[0];      /* number of pixels to write */
  fits_write_img(fptr, TFLOAT, fpixel, nelements, scan->data, &status);

  fits_close_file(fptr, &status);

  return status;
}

int main(int argc, char *argv[])
{
  static struct OdinScan scan;
  FILE *sl;
  int i, j, status;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);


  sl = NULL;
  if (scanlist) {
    if (!strcmp(listname, "-")) sl = stdin;
    else {
      sl = fopen(listname, "r");
      if (sl == NULL) ODINerror("can't open input list '%s'", listname);
    }
  }

  j = 0;
  if (scanlist) {
    while (fscanf(sl, "%s", listname)) {
      if (feof(sl)) break;
      ODINinfo("next scan '%s'", listname);
      if (!readODINscan(listname, &scan))
	ODINerror("failed to read '%s'", argv[i]);
      writefits(&scan, listname);
      j++;
    };
  } else {
    for (i = 1; i < argc; i++) {
      ODINinfo("next scan '%s'", argv[i]);
      if (!readODINscan(argv[i], &scan))
	ODINerror("failed to read '%s'", argv[i]);
      writefits(&scan, argv[i]);
      j++;
    }
  }
  ODINinfo("read %d scans", j);

  exit(0);
}
