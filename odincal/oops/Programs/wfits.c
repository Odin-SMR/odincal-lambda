#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "odinlib.h"
#include "odinscan.h"
#include "options.h"

#include "bintable.h"
#include "fitsio.h"

static char rcsid[] = "$Id$";

void logname(char *);

static int scanlist = 0;
static char filename[MAXNAMLEN+1];    /* name for new FITS file */
static char listname[MAXNAMLEN+1];    /* name for list of scan file(s) */

char PROGRAM_NAME[] = "wfits";
char description[] = "store spectra as FITS binary table";
char required[] = "-file name spectra";
struct _options options[] = {
  { "-file name",      "specify output file name" },
  { "-list name",      "specify file name for list of spectra" },
  { "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;
  optarg = NULL;

  if (!strcmp(opt, "file")) {
    GetOption(&optarg, pargc, pargv);
    strcpy(filename, optarg);
    return;
  }
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

int main(int argc, char *argv[])
{
  static char buffer[8192];
  static struct OdinScan scan;
  FILE *sl;
  int status, i, j;
  fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
  long  fpixel, nelements;
  int bitpix, n;
  long naxis;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (filename[0] == '\0') ODINerror("output filename required");

  status = 0;
  remove(filename);               /* Delete old file if it already exists */

  n = rowSize();
  if (sizeof(buffer) < n) {
    ODINerror("buffer size too small");
  }
  sl = NULL;
  if (scanlist) {
    if (!strcmp(listname, "-")) sl = stdin;
    else {
      sl = fopen(listname, "r");
      if (sl == NULL) ODINerror("can't open input list '%s'", listname);
    }
  }

  /* open a new FITS file */
  if (fits_create_file(&fptr, filename, &status)) /* create new FITS file */
    printerror(status);           /* call printerror if error occurs */

  status = 0;         /* initialize status before calling fitsio routines */

  /* write the required keywords for the primary array image */
  bitpix =  8;
  naxis =  0;
  if (fits_create_img(fptr, bitpix, naxis, NULL, &status))
    printerror(status);          
  
  fpixel = 1;         /* first pixel to write      */
  nelements = 0;      /* number of pixels to write */

  writebintable(fptr);

  j = 0;
  if (scanlist) {
    while (fscanf(sl, "%s", listname)) {
      if (feof(sl)) break;
      ODINinfo("next spectrum '%s'", listname);
      if (!readODINscan(listname, &scan))
	ODINerror("failed to read '%s'", argv[i]);
      decodescan(&scan, buffer);
      j++;
      fits_write_tblbytes(fptr, j, 1, n, buffer, &status); 
      if (status) printerror(status);
    };
  } else {
    for (i = 3; i < argc; i++) {
      ODINinfo("next spectrum '%s'", argv[i]);
      if (!readODINscan(argv[i], &scan))
	ODINerror("failed to read '%s'", argv[i]);
      decodescan(&scan, buffer);
      j++;
      fits_write_tblbytes(fptr, j, 1, n, buffer, &status); 
      if (status) printerror(status);
    }
  }
  ODINinfo("read %d spectra", j);

  
  if (fits_close_file(fptr, &status))       /* close the FITS file */
    printerror(status);

  exit(0);
}
