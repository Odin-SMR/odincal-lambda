#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "odinlib.h"
#include "options.h"

#include <hdf.h>
#include "odinhdf.h"

static char rcsid[] = "$Id$";

void logname(char *);

static int scanlist = 0;
static char filename[MAXNAMLEN+1];    /* name for new HDF file */
static char listname[MAXNAMLEN+1];    /* name for list of scan file(s) */

char PROGRAM_NAME[] = "whdf";
char description[] = "store spectra in HDF";
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
  static struct OdinScan scan;
  FILE *sl;
  int status, file_id, i, j;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (filename[0] == '\0') ODINerror("output filename required");

  sl = NULL;
  if (scanlist) {
    if (!strcmp(listname, "-")) sl = stdin;
    else {
      sl = fopen(listname, "r");
      if (sl == NULL) ODINerror("can't open input list '%s'", listname);
    }
  }

  /* Open the HDF file and initialize the Vdata interface */
  file_id = open_HDF_vdata(filename, DFACC_CREATE, 0);

  status = 0;

  j = 0;
  if (scanlist) {
    while (fscanf(sl, "%s", listname)) {
      if (feof(sl)) break;
      ODINinfo("next spectrum '%s'", listname);
      if (!readODINscan(listname, &scan))
	ODINerror("failed to read '%s'", listname);
      j++;
      status = write_smr_hdf(file_id, &scan);
    };
  } else {
    for (i = 3; i < argc; i++) {
      ODINinfo("next spectrum '%s'", argv[i]);
      if (!readODINscan(argv[i], &scan))
	ODINerror("failed to read '%s'", argv[i]);
      j++;
      status = write_smr_hdf(file_id, &scan);
    }
  }
  ODINinfo("read %d spectra", j);

  /* Close the HDF file and close the Vdata interface */
  close_HDF_vdata(file_id); 

  exit(0);
}
