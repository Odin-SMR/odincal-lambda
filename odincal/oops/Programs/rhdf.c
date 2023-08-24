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

char PROGRAM_NAME[] = "rhdf";
char description[] = "retrieve spectra from HDF file";
char required[] = "-file name spectra";
struct _options options[] = {
  { "-file name",      "specify output file name" },
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
  static struct OdinScan scan;
  FILE *sl;
  int status, file_id, i, j, n;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (filename[0] == '\0') ODINerror("input filename required");

  /* Open the HDF file and initialize the Vdata interface */
  file_id = open_HDF_vdata(filename, DFACC_RDONLY, 0);
  
  n = container_records(file_id, CONT_SMR_HEADER);
  ODINinfo("found %d spectra", n);

  status = 0;

  for(i = 0; i < n; i++) {
    status = read_smr_hdf(file_id, &scan);
    strcpy(filename, SaveName(&scan));
    printf("%s\n", filename);
    if (!writeODINscan(filename ,&scan))
      ODINerror("failed to write '%s'", filename);
  }
 
  close_HDF_vdata(file_id);

  exit(0);
}
