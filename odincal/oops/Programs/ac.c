#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "options.h"
#include "odinlib.h"
#include "aclib.h"

static char rcsid[] = "$Id$";

/* #define MAXACCHAN  (MAXCHIPS*CHPERCHIP) */
#define MAXACMHZ   (MAXACCHAN*112/96)

static int normalise = 0;
static int reduce = 0;
static int header = 0;

char PROGRAM_NAME[] = "acs";
char description[] = "retrieve level 0 data and store as level 1a";
char required[] = "filename";
struct _options options[] = {
{ "-norm",	       "normalise raw data" },
{ "-fft",	       "process data" },
{ "-head",	       "print scan header" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
    char *opt, *optarg;

    opt = (*pargv)[0] + 1;
    optarg = NULL;

    if (!strcmp(opt, "norm")) {
	normalise = 1;
	return;
    }
    if (!strcmp(opt, "fft")) {
	normalise = 1;
	reduce = 1;
	return;
    }
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
    static double f[MAXCHANNELS];
    static char filename[32];

    ACSBlock *block;
    struct OdinScan *ac;
  
    int i, k, n;

    GetOpts(argc, argv);
    logname("acs");

    if (argc == 1) ODINerror("filename required as argument");

    block = ReadACSLevel0(argv[argc-1], &n);
    if (block == NULL) ODINerror("not a valid level 0 AC file");

    ac = GetACSscan(block, &n);
    free(block);
   
    if (ac == (struct OdinScan *)NULL) exit(0);

    /* save scans as individual files */
    for (i = 0; i < n; i++) {
	if (ac[i].data[0] == 0) {
	    ODINwarning("no data in spectrum %d", i);
	    continue;
	}

	InitHead(ac+i);
	/* 291 385 511 */
/*  	if (ac[i].IntMode == 511) { */
	    if (header)    PrintScan(ac+i);
	    if (normalise) Normalise(ac+i);
	    if (reduce)    ReduceAC(ac+i);

	    strcpy(filename, SaveName(ac+i));

	    if (!writeODINscan(filename, ac+i)) 
		ODINerror("failed to write '%s'", filename);

	    ODINinfo("backend = %d, mode = %d, time = %.1f", 
		     ac[i].Backend, ac[i].IntMode, ac[i].IntTime);
	    ODINinfo("scan written to file '%s'", filename);
/*  	    break; */
/*  	} */
    }
    ODINinfo("file '%s' contained %d spectra", argv[argc-1], n);
    if (n) ODINinfo("STW range: [%08x:%08x]", ac[0].STW, ac[n-1].STW);
    free(ac);

    exit(0);
}
