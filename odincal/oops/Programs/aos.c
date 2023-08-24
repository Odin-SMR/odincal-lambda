#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "options.h"
#include "odinscan.h"
#include "odinlib.h"
#include "aoslib.h"

static char rcsid[] = "$Id$";

static int header = 0;

char PROGRAM_NAME[] = "aos";
char description[] = "retrieve level 0 data and store as level 1a";
char required[] = "filename";
struct _options options[] = {
{ "-head",	       "print scan header" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;
  optarg = NULL;

  if (!strcmp(opt, "head")) {
    header = 1;
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
  double *c;
  static double fc[4] = { 
    2097.305e+06, 
    6.194040e+05,
    4.003520e+00, 
   -1.019432e-02
  };

  static char filename[32];

  AOSBlock *block;
  struct OdinScan *aos;
  
  int i, j, k, n, orbit, discipline;
  static int type = REF;

  GetOpts(argc, argv);
  logname("aos");

  if (argc == 1) ODINerror("filename required as argument");

  block = ReadAOSLevel0(argv[1], &n);
  if (block == NULL) ODINerror("not a valid level 0 AOS file");

  aos = GetAOSscan(block, &n);
  free(block);
   
  /* save scans as individual files */
  for (i = 0; i < n; i++) {
    if (aos[i].Type == CMB) {
      c = FitAOSFreq(aos+i);
      if (c) {
	for (j = 0; j < 4; j++) {
	  printf("c[%d] = %e\n", j, c[j]);
	  fc[j] = c[j];
	}
      } else {
	ODINwarning("fitting of polynomial failed");
      }
    }
    for (j = 0; j < 4; j++) aos[i].FreqCal[j] = fc[j];

    InitHead(aos+i);
    if (header) PrintScan(aos+i);

    strcpy(filename, SaveName(aos+i));

    if (!writeODINscan(filename, aos+i)) 
      ODINerror("failed to write '%s'", filename);
    ODINinfo("backend = %d, mode = %d, time = %.1f", 
	     aos[i].Backend, aos[i].IntMode, aos[i].IntTime);
    ODINinfo("scan written to file '%s'", filename);
  }
  ODINinfo("file '%s' contained %d spectra", argv[1], n);
  ODINinfo("STW range: [%08x:%08x]", aos[0].STW, aos[n-1].STW);
  free(aos);

  exit(0);
}
