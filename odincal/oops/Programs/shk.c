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

#define AOSINFO   1
#define PLFINFO   2
#define PLLINFO   3
#define MECINFO   4
#define OSINFO    5
#define BEAMINFO  6

static int which = AOSINFO;

static char shkfile[MAXNAMLEN+1] = "";

char PROGRAM_NAME[] = "shk";
char description[] = "retrieve house-keeping data";
char required[] = "filename";
struct _options options[] = {
{ "-file filename",    "specify house-keeping data file" },
{ "-aos",	       "extract AOS housekeeping" },
{ "-beam",	       "extract beam configuration" },
{ "-os",	       "extract Osiris housekeeping" },
{ "-platform",	       "extract platform housekeeping" },
{ "-pll",	       "extract PLL A & B housekeeping" },
{ "-mech",	       "extract mech A & B housekeeping" },
{ "-verbose",	       "produce verbose output" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;

  if (!strcmp(opt, "file")) {
    GetOption(&optarg, pargc, pargv);
    strcpy(shkfile, optarg);
    return;
  }
  if (!strcmp(opt, "aos")) {
    which = AOSINFO;
    return;
  }
  if (!strcmp(opt, "beam")) {
    which = BEAMINFO;
    return;
  }
  if (!strcmp(opt, "os")) {
    which = OSINFO;
    return;
  }
  if (!strcmp(opt, "platform")) {
    which = PLFINFO;
    return;
  }
  if (!strcmp(opt, "pll")) {
    which = PLLINFO;
    return;
  }
  if (!strcmp(opt, "mech")) {
    which = MECINFO;
    return;
  }
  if (!strcmp(opt, "verbose")) {
    verbose = 1;
    return;
  }
  if (!strcmp(opt, "help")) {
    Help();
    exit(0);
  }
  Syntax(**pargv);
}

unsigned long LongWord(unsigned short int word[])
{
  long result;

  /*  printf("%04x:%04x\n", word[1], word[0]); */
  result = ((long)word[1]<<16) + word[0];
  return result;
}

int main(int argc, char *argv[])
{
  HKBlock hk;
  /* unsigned short sync, user; */
  unsigned short hknext;
  unsigned long stw, stw1, sum;
  static unsigned short hkword[8], hkindex[8];
  unsigned short eps1, eps2, pos1, pos2, beam1, beam2, chop1, chop2;
  int j, n, index;
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

  stw1 = 0;
  while ((n = fread(&hk, sizeof(HKBlock), 1, shk)) == 1) {
    if (hk.head.SYNC != SYNCWORD) ODINwarning("wrong sync word");
    stw = LongWord(hk.head.STW);
    
    switch (which) {
     case AOSINFO:
      index = 1;
      for (j = 0; j < AOSHK-2; j++) {
	hkindex[j] = hk.data[index+j] & 0x000f;
	hkword[j] = hk.data[index+j]>>4;
      }
      if (hkindex[0] != hkindex[1]) ODINwarning("HK indices disagree");
      sum = LongWord(&(hk.data[index])+2);

      /* check for limits (Ref: LAS.ODN.PJT.NOT.960646, D.Pouliquen) */
      switch (hkindex[0]) {
       case 0:
	if (hkword[0] > 2459) ODINwarning("laser diode too hot");
	break;
       case 1:
	if (hkword[0] < 1178) ODINwarning("laser diode current too low");
	if (hkword[0] > 3892) ODINwarning("laser diode current too high");
	break;
       case 2:
	if (hkword[0] > 2509) ODINwarning("AOP structure too hot");
	break;
      }

      printf("%10ld %2d %4d %4d %10ld\n",
	     stw, hkindex[0], hkword[0], hkword[1], sum);
      break;
     case BEAMINFO:
      index = 47;
      for (j = 0; j < 2; j++) {
	hkindex[j] = hk.data[index+j] & 0x0003;
	hkword[j] = (hk.data[index+j]>>3) & 0x000f;
      }
      index = 33;
      hkindex[2] = (hk.data[index]>>13) & 0x0003;
      hkindex[3] = (hk.data[index+6]>>13) & 0x0003;
      index = 45;
      hkindex[4] = (hk.data[index]>>7) & 0x0001;
      hkindex[5] = (hk.data[index+1]>>7) & 0x0001;
/*        index = 49; */
/*        hkindex[6] = (hk.data[index]>>7) & 0x0001; */
      if ((eps1 != hkindex[0]) 
	  || (eps2 != hkindex[1])
	  || (pos1 != hkword[0])
	  || (pos2 != hkword[1])
	  || (beam1 != hkindex[2])
	  || (beam2 != hkindex[3])
	  || (chop1 != hkindex[4])
	  || (chop2 != hkindex[5])
	  ) {
	if (stw1 != 0)
	  printf("%10ld %1d %2d %1d %2d %1d %1d %1d %1d\n",
		 stw1, eps1, pos1, eps2, pos2, beam1, beam2, chop1, chop2);
	eps1 = hkindex[0];
	eps2 = hkindex[1];
	pos1 = hkword[0];
	pos2 = hkword[1];
	beam1 = hkindex[2];
	beam2 = hkindex[3];
	chop1 = hkindex[4];
	chop2 = hkindex[5];
	printf("%10ld %1d %2d %1d %2d %1d %1d %1d %1d\n",
	       stw, eps1, pos1, eps2, pos2, beam1, beam2, chop1, chop2);
      }
      stw1 = stw;
      break;
     case PLFINFO:
      ODINerror("not yet implemented");
      break;
     case PLLINFO:
      index = 10+AOSHK+OSHK;
      hknext = hk.data[index];
      if (hknext != 0xfeaf) {
	printf("%10ld ", stw);
	for (j = 0; j < PLL_A_HK; j++) {
	  hknext = hk.data[index+j];
	  hkindex[j] = hknext & 0x000f;
	  hkword[j] = hknext >> 4;
	  printf("%2d %4d ", hkindex[j], hkword[j]);
	}
	printf("\n");
      }

      index += PLL_A_HK;
      hknext = hk.data[index];
      if (hknext != 0xfeaf) {
	printf("%10ld ", stw);
	for (j = 0; j < PLL_A_HK; j++) {
	  hknext = hk.data[index+j];
	  hkindex[j] = (hknext & 0x000f)+16;
	  hkword[j] = hknext >> 4;
	  printf("%2d %4d ", hkindex[j], hkword[j]);
	}
	printf("\n");
      }
      break;
     case MECINFO:
      index = 10+AOSHK+OSHK+PLL_A_HK+PLL_B_HK;
      printf("%10ld ", stw);
      hkword[0] = hk.data[index];
/*        printf("A %5d ", hkword[0]); */
      printf("A  %04x ", hkword[0]);
      for (j = 1; j < MECH_A_HK; j++) {
	hknext = hk.data[index+j];
	hkindex[j] = hknext & 0x000f;
	hkword[j] = hknext >> 4;
	printf("%2d %4d ", hkindex[j], hkword[j]);
      }
      /*        printf("\n           "); */
      printf("\n");
      index += MECH_A_HK;
      printf("%10ld ", stw);
      hkword[0] = hk.data[index];
/*        printf("B %5d ", hkword[0]); */
      printf("B  %04x ", hkword[0]);
      for (j = 1; j < MECH_A_HK; j++) {
	hknext = hk.data[index+j];
	hkindex[j] = (hknext & 0x000f)+16;
	hkword[j] = hknext >> 4;
	printf("%2d %4d ", hkindex[j], hkword[j]);
      }
      printf("\n");
      break;
     case OSINFO:
      index = 1+AOSHK;
      printf("%10ld ", stw);
      for (j = 0; j < OSHK; j++) {
	hknext = hk.data[index+j];
	hkindex[j] = hknext & 0x000f;
	hkword[j] = hknext >> 4;
	printf("%2d %4d ", hkindex[j], hkword[j]);
      }
      printf("\n");
      break;
     default:
      ODINerror("no such info");
    }
  }

  fclose(shk);

  exit(0);
}
