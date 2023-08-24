#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "options.h"
#include "level0.h"
#include "odinlib.h"

static char rcsid[] = "$Id$";

int readonly = 0;
int verbose  = 0;
int column   = 0;

static int subcom[HKBLOCKSIZE] = {
  1,                              /* HK 47                                 0 */
  1, 1, 0, 0,                     /* AOS                                 1-4 */
  1, 1, 1,                        /* Osiris                              5-7 */
  1, 1, 1, 1, 1,                  /* HK 46, HK 4C, HK 48, HK 4D, HK 49  8-12 */
  1, 1, 1, 1,                     /* HK 40-43                          13-16 */
  1, 1, 1, 1, 1,                  /* PLL A                             17-21 */
  1, 1, 1,
  1, 1, 1, 1, 1,                  /* PLL B                             25-29 */
  1, 1, 1,
  0, 1, 1, 1, 1, 1,               /* Mech A                            33-38 */
  0, 1, 1, 1, 1, 1,               /* Mech B                            39-44 */
  0,                              /* HK pyro                              45 */
  1,                              /* HK 4A                                46 */
  0, 0,                           /* ACDC1 sync, ACDC2 sync            47-48 */
  1,                              /* ACS                                  49 */
  1, 1, 1,                        /* HK 20-22                          50-52 */
  1, 1,                           /* HK 44-45                          53-54 */
  1, 1,                           /* ACDC1 slow, ACDC2 slow            55-56 */
  0, 0, 0,                        /* HK 30-32                          57-59 */
  1, 1, 1,                        /* Gy 1-3                            60-62 */
  0, 0                            /* HK 4E-4F                          63-64 */
};

struct HKgroup {
  char *name;
  int offset;
  int mincol;
  int maxcol;
  int maxindex;
} hkgrp[] = {
  { "hk47",   0, 0, 0, 16 },
  { "aos",    1, 0, 3, 16 },
  { "osiris", 5, 0, 2, 16 },
  { "hk46",   8, 0, 0, 16 },
  { "hk4c",   9, 0, 0, 16 },
  { "hk48",  10, 0, 0, 16 },
  { "hk4d",  11, 0, 0, 16 },
  { "hk49",  12, 0, 0, 16 },
  { "hk40",  13, 0, 0, 16 },
  { "hk41",  14, 0, 0, 16 },
  { "hk42",  15, 0, 0, 16 },
  { "hk43",  16, 0, 0, 16 },
  { "pllA",  17, 0, 4,  6 },
  { "pllB",  25, 0, 4,  6 },
  { "mechA", 33, 0, 5, 16 },
  { "mechB", 39, 0, 5, 16 },
  { "hkpyro",45, 0, 0,  1 },
  { "hk4a",  46, 0, 0, 16 },
  { "acdc",  47, 0, 1,  1 },
  { "acs",   49, 0, 0, 16 },
  { "hk20",  50, 0, 0, 16 },
  { "hk21",  51, 0, 0, 16 },
  { "hk22",  52, 0, 0, 16 },
  { "hk44",  53, 0, 0, 16 },
  { "hk45",  54, 0, 0, 16 },
  { "acdcs", 55, 0, 1, 16 },
  { "hk30",  57, 0, 0,  1 },
  { "hk31",  58, 0, 0,  1 },
  { "hk32",  59, 0, 0,  1 },
  { "gyro",  60, 0, 2, 16 },
  { "hk4e",  63, 0, 0,  1 },
  { "hk4f",  64, 0, 0,  1 }
};

static int which = -1;
static int sub   = -1;

static char shkfile[MAXNAMLEN+1] = "";

char PROGRAM_NAME[] = "shkgrp";
char description[] = "retrieve house-keeping data";
char required[] = "filename";
struct _options options[] = {
{ "-file filename",    "specify house-keeping data file" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;
  int i, n;

  opt = (*pargv)[0] + 1;

  if (!strcmp(opt, "file")) {
    GetOption(&optarg, pargc, pargv);
    strcpy(shkfile, optarg);
    return;
  }
  if (!strcmp(opt, "sub")) {
    GetOption(&optarg, pargc, pargv);
    sub = atoi(optarg);
    return;
  }
  n = (sizeof(hkgrp))/(sizeof(struct HKgroup));
  if (!strcmp(opt, "help")) {
    Help();
    printf("specify the group to dump:\n");
    for (i = 0; i < n; i++) {
      printf("\t-%s", hkgrp[i].name);
      if ((i % 8) == 7) printf("\n");
    }
    printf("\n");
    exit(0);
  }
  which = -1;
  for (i = 0; i < n; i++) {
    if (!strcmp(opt, hkgrp[i].name)) {
      GetOption(&optarg, pargc, pargv);
      if (hkgrp[i].maxcol > 0) {
	column = atoi(optarg);
	column--;
      } else {
	column = 0;
      }
      if (column < hkgrp[i].mincol || column > hkgrp[i].maxcol) {
/*  	ODINerror("column number out of range"); */
	ODINerror("%s needs column between %d and %d",
		  hkgrp[i].name, hkgrp[i].mincol+1, hkgrp[i].maxcol+1);
      }
      which = i;
      return;
    }
  }
  Syntax(**pargv);
}

unsigned long LongWord(unsigned short int word[])
{
  long result;

  result = ((long)word[1]<<16) + word[0];
  return result;
}

int main(int argc, char *argv[])
{
  HKBlock hk;

  unsigned long stw, stw0;
  static unsigned short hkword, hkindex;
  static unsigned int data[16];
  int j, n, index, sync, changed;
  FILE *shk;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (argc == 1) ODINerror("filename required as argument");

  if (!strcmp(shkfile, "-")) shk = stdin;
  else {
    if (shkfile[0] == '\0') strcpy(shkfile, argv[argc-1]);
    shk = fopen(shkfile, "r");
    if (shk == NULL) ODINerror("can't open data file '%s'", shkfile);
  }

  if (which == -1) ODINerror("no such info");
  if (sub >= 0 && sub > hkgrp[which].maxindex) ODINerror("no such subaddress");

  index = hkgrp[which].offset;

  sync    = 0;
  changed = 1;
  stw0    = 0;
  while ((n = fread(&hk, sizeof(HKBlock), 1, shk)) == 1) {
    if (hk.head.SYNC != SYNCWORD) ODINwarning("wrong sync word");
    stw = LongWord(hk.head.STW);

    if (subcom[index+column]) {
      hkindex = hk.data[index+column] & 0x000f;
      hkword  = hk.data[index+column]>>4;
      if (hkindex == 0) {
	sync = 1;
	if (changed && stw0) {
	  printf("%ld", stw0);
	  if (sub >= 0) {
	    printf("\t%d", data[sub]);
	  } else {
	    for (j = 0; j < hkgrp[which].maxindex; j++) {
	      printf("\t%d", data[j]);
	    }
	  }
	  printf("\n");
	  changed = 0;
	}
	stw0 = stw;
      }
    } else {
      hkindex = 0;
      hkword  = hk.data[index+column];
      if (changed && stw0) {
	printf("%ld", stw0);
  	printf("\t%d", data[0]);
	printf("\n");
      }
      changed = 0;
      stw0 = stw;
    }
    if (data[hkindex] != hkword) {
      if (sub == -1 || sub == hkindex) {
	data[hkindex] = hkword;
	changed = 1;
      }
    }
  }

  fclose(shk);

  exit(0);
}
