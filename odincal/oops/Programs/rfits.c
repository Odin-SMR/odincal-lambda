#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "odinlib.h"
#include "options.h"

#include "bintable.h"
#include "fitsio.h"

static char rcsid[] = "$Id$";

static char filename[MAXNAMLEN+1];    /* name for new FITS file */

char PROGRAM_NAME[] = "rfits";
char description[] = "extract spectra from FITS binary table";
char required[] = "-file name";
struct _options options[] = {
  { "-file name",      "specify input file name" },
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
  int status, nfound, hdutype, i, n, m;
  long naxes[2];
  fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (filename[0] == '\0') ODINerror("input filename required");

  /* initialize FITS image parameters */
  status = 0;

  m = rowSize();
  if (sizeof(buffer) < m) {
    ODINerror("buffer size too small");
  }

  /* open the FITS file */
  if (fits_open_file(&fptr, filename, READONLY, &status)) 
    printerror( status );

  if (fits_movabs_hdu(fptr, 2, &hdutype, &status)) 
    printerror(status);
  
  if (hdutype == BINARY_TBL) ODINinfo("Reading binary table in HDU 2");
  else {
    ODINwarning("this HDU is not a binary table");
    printerror(status);
  }

  /* read the NAXIS1 and NAXIS2 keyword to get table size */
  if (fits_read_keys_lng(fptr, "NAXIS", 1, 2, naxes, &nfound, &status))
    printerror(status);

  n = readbintable(fptr);
  ODINinfo("found %d spectra", n);

  for (i = 0; i < n; i++) {
    fits_read_tblbytes(fptr, i+1, 1, m, buffer, &status); 
    if (status) printerror(status);
    encodescan(&scan, buffer);
    strcpy(filename, SaveName(&scan));
    printf("%s\n", filename);
    if (!writeODINscan(filename ,&scan))
      ODINerror("failed to write '%s'", filename);
  }

  if (fits_close_file(fptr, &status))       /* close the FITS file */
    printerror(status);

  exit(0);
}
