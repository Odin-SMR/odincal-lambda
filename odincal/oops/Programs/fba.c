#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "options.h"
#include "odinlib.h"
#include "odinscan.h"
#include "fbalib.h"

static char rcsid[] = "$Id$";

char PROGRAM_NAME[] = "fba";
char description[] = "retrieve level 0 data and store as level 1a";
char required[] = "filename";
struct _options options[] = {
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;
  optarg = NULL;

  if (!strcmp(opt, "help")) {
    Help();
    exit(0);
  }
  Syntax(**pargv);
}

int main(int argc, char *argv[])
{
  /* static char filename[32]; */

  FBABlock *block;
  struct OdinScan *filter;
  
  int i, j, n;

  GetOpts(argc, argv);
  logname("fba");

  if (argc == 1) ODINerror("filename required as argument");

  block = ReadFBALevel0(argv[argc-1], &n);
  if (block == NULL) ODINerror("not a valid level 0 FBA file");

  filter = GetFBAscan(block, &n);
  free(block);

  if (filter == (struct OdinScan *)NULL) exit(0);

  /* save scans as individual files */
  for (i = 0; i < n; i++) {
    printf("%10lu", filter[i].STW);
    printf("%5.2f", filter[i].IntTime);
    for (j = 0; j < FILTERCHANNELS; j++) {
      printf("%12.4e", filter[i].data[j]);
    }
    printf("%2d", filter[i].Type);
    printf("\n");
  }
  ODINinfo("file '%s' contained %d spectra", argv[argc-1], n);
  ODINinfo("STW range: [%08x:%08x]", filter[0].STW, filter[n-1].STW);
  free(filter);

  exit(0);
}
