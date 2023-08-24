#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "odinlib.h"
#include "odinscan.h"
#include "options.h"

static char rcsid[] = "$Id$";

#ifndef WORDS_BIGENDIAN
#define DEFAULTTYPE 0
#else
#define DEFAULTTYPE 1000
#endif

#define STRING (DEFAULTTYPE + 1)
#define NUMBER (DEFAULTTYPE)

/* define names and arrays for MATLAB variables */
static char stw_name[] = "STW";
static char freq_name[] = "f";
static char data_name[] = "d";
static char type_name[] = "phase";

void logname(char *);

int freqsort(Scan *, double *);
static int scanlist = 0;
static char filename[MAXNAMLEN+1];    /* name for new MatLab file */
static char listname[MAXNAMLEN+1];    /* name for list of scan file(s) */

char PROGRAM_NAME[] = "wmlab";
char description[] = "store scans as binary MatLab file";
char required[] = "-file name scan(s)";
struct _options options[] = {
  { "-file name",      "specify output file name" },
  { "-list name",      "specify file name for list of scans" },
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

/*
 * savemat - C language routine to save a matrix in a MAT-file.
 *
 * Here is an example that uses 'savemat' to save two matrices to disk,
 * the second of which is complex:
 *
 *	FILE *fp;
 *	double xyz[1000], ar[1000], ai[1000];
 *	fp = fopen("foo.mat","wb");
 *	savemat(fp, 2000, "xyz", 2, 3, 0, xyz, (double *)0);
 *	savemat(fp, 2000, "a", 5, 5, 1, ar, ai);
 *      fclose(fp);
 *
 * Author J.N. Little 11-3-86
 */

typedef struct {
    long type;   /* type */
    long mrows;  /* row dimension */
    long ncols;  /* column dimension */
    long imagf;  /* flag indicating imag part */
    long namlen; /* name length (including NULL) */
} Fmatrix;

void savemat(FILE *fp, int type, char *pname, int mrows, int ncols, 
	int imagf, double *preal, double *pimag)
/* FILE *fp;       File pointer */
/* int type;       Type flag: Normally 0 for PC, 1000 for Sun, Mac, and	 */
/*		   Apollo, 2000 for VAX D-float, 3000 for VAX G-float    */
/*		   Add 1 for text variables.	 */
/*		   See LOAD in reference section of guide for more info. */
/* int mrows;      row dimension */ 
/* int ncols;      column dimension */
/* int imagf;	   imaginary flag (0: real array, 1: complex array) */
/* char *pname;    pointer to matrix name */
/* double *preal;  pointer to real data */
/* double *pimag;  pointer to imag data */
{
    int mn;
    Fmatrix x;
	
    x.type = type;
    x.mrows = mrows;
    x.ncols = ncols;
    x.imagf = imagf;
    x.namlen = strlen(pname) + 1;
    mn = x.mrows * x.ncols;

    fwrite(&x, sizeof(Fmatrix), 1, fp);
    fwrite(pname, sizeof(char), (int)x.namlen, fp);
    fwrite(preal, sizeof(double), mn, fp);
    if (imagf) {
	fwrite(pimag, sizeof(double), mn, fp);
    }
}

int main(int argc, char *argv[])
{
  static struct OdinScan scan;
  FILE *sl, *matlab, *tmp;
  int i, j, k, n, backend, nc, mode;
  double *stw, *phase;
  double *f, *d;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (filename[0] == '\0') {
    fprintf(stderr, "wmlab -- output filename required\n");
    exit(1);
  }

  sl = NULL;
  if (scanlist) {
    if (!strcmp(listname, "-")) sl = stdin;
    else {
      sl = fopen(listname, "r");
      if (sl == NULL) {
	fprintf(stderr, "wmlab -- can't open input list '%s'\n", listname);
	exit(1);
      }
    }
  }

  /* open a MatLab file */
  matlab = fopen(filename, "w");
  if (matlab == NULL) {
    fprintf(stderr, "wmlab -- can't open MatLab file '%s'\n", filename);
    exit(1);
  }
  /* open a temporary file */
  tmp = fopen("/tmp/wmlab.lst", "w");
  if (tmp == NULL) {
    fprintf(stderr, "wmlab -- can't open temporary file '/tmp/wmlab.lst'\n");
    exit(1);
  }

  /* find out how many scans we have */
  n = 0;
  if (scanlist) {
    while (fscanf(sl, "%s", listname)) {
      if (feof(sl)) break;
      if (n == 0) {
	if (!readODINscan(listname, &scan)) {
	  fprintf(stderr, "wmlab -- failed to read '%s'\n", listname);
	  exit(1);
	}
      }
      fprintf(tmp, "%s\n", listname);
      n++;
    }
  } else {
    n = argc-3;
    if (!readODINscan(argv[3], &scan)) {
      fprintf(stderr, "wmlab -- failed to read '%s'\n", argv[3]);
      exit(1);
    }
    for (i = 0; i < n; i++) {
      fprintf(tmp, "%s\n", argv[3+i]);
    }
  }
  /*    ODINinfo("read %d scans", n); */
  fclose(tmp);

  nc = scan.Channels;
  backend = scan.Backend;
  mode = scan.IntMode;
  /*   printf("number of channels: %d\n", scan.Channels); */
  /*   printf("backend:            %d\n", scan.Backend ); */
  /*   printf("integration mode:   %d\n", scan.IntMode ); */
  
  stw = calloc(n, sizeof(double));
  if (stw == NULL) {
    fprintf(stderr, "wmlab -- failed to allocate STW vector\n");
    exit(1);
  }

  phase = calloc(n, sizeof(double));
  if (phase == NULL) {
    fprintf(stderr, "wmlab -- failed to allocate phase vector\n");
    exit(1);
  }

  f = calloc(MAXCHANNELS, sizeof(double));
  if (f == NULL) {
    fprintf(stderr, "wmlab -- failed to allocate frequency vector\n");
    exit(1);
  }
  nc = freqsort(&scan, f);
  /*    printf("freqsort: %d\n", nc); */
  /*    for (k = 0; k < 5; k++) { */
  /*      printf("f[%d] = %f\n", k, f[k]); */
  /*    } */
  /* (void)frequency(&scan, f); */

  d = calloc(n*nc, sizeof(double));
  if (d == NULL) {
    fprintf(stderr, "wmlab -- failed to allocate data array\n");
    exit(1);
  }
  for (k = 0; k < nc; k++) d[k] = scan.data[k];
  
  /* reopen the temporary file */
  tmp = fopen("/tmp/wmlab.lst", "r");
  if (tmp == NULL) {
    fprintf(stderr, "wmlab -- can't reopen temporary file '/tmp/wmlab.lst'\n");
    exit(1);
  }
  for (j = 0; j < n; j++) {
    fscanf(tmp, "%s", listname);
    if (ferror(tmp)) {
      fprintf(stderr, "wmlab -- failed to read filename\n");
    } else {
      /*  if (j == 0) continue; */ /* we already have this scan */
      if (!readODINscan(listname, &scan)) {
	fprintf(stderr, "wmlab -- failed to read file '%s'\n", listname);
	exit(1);
      }

      /*     printf("number of channels: %d\n", scan.Channels); */
      /*     printf("backend:            %d\n", scan.Backend ); */
      /*     printf("integration mode:   %d\n", scan.IntMode ); */
      if (scan.Backend  != backend) 
	fprintf(stderr, "wmlab -- backend mismatch\n");
      if (scan.IntMode  != mode)    
	fprintf(stderr, "wmlab -- mode mismatch\n");
      if (freqsort(&scan, f) != nc) 
	fprintf(stderr, "wmlab -- number of channels mismatch\n");
      /*     (void)frequency(&scan, f); */
      stw[j] = (double)scan.STW;
      phase[j] = (double)scan.Type;
      /*        printf("%4d: %08x %d\n", j, scan.STW, scan.Type); */
      for (k = 0; k < nc; k++) d[k+j*nc] = scan.data[k];
    }
  }
  fprintf(stderr, "wmlab -- read %d scans\n", j);

  savemat(matlab, NUMBER, stw_name,  n,  1, 0, stw, (double *)0);
  savemat(matlab, NUMBER, type_name, n,  1, 0, phase, (double *)0);
  savemat(matlab, NUMBER, freq_name, nc, 1, 0, f,   (double *)0);
  savemat(matlab, NUMBER, data_name, nc, n, 0, d,   (double *)0);
  
  free(stw);
  free(phase);
  free(f);
  free(d);

  fclose(matlab);

  exit(0);
}
