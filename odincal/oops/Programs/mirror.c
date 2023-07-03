#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "level0.h"
#include "options.h"
#include "odinlib.h"
#include "aclib.h"
#include "fbalib.h"

static char rcsid[] = "$Id$";

#define BLOCKSIZE 15

static char oldname[MAXNAMLEN+1];    /* name for old AC file */
static char newname[MAXNAMLEN+1];    /* name for new AC file */

char PROGRAM_NAME[] = "mirror";
char description[] = "copy info on calibration mirror from fba to ac data";
char required[] = "ac-filename fba-filename";
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
  FILE *new;
  ACSBlock *ac;
  FBABlock *fba;

  int j, i, nac, nfba;
  unsigned short sync;
  unsigned long acSTW, fbaSTW;
  
  GetOpts(argc, argv);
  logname("mirror");

  if (argc < 3) ODINerror("filenames required as argument");

  strcpy(oldname, argv[argc-2]);
  strcpy(newname, oldname);
  i = strlen(newname);
  newname[i] = '~';
  newname[i+1] = '\0';

  ac = ReadACSLevel0(oldname, &nac);
  if (ac == NULL) ODINerror("not a valid level 0 AC file");

  fba = ReadFBALevel0(argv[argc-1], &nfba);
  if (fba == NULL) ODINerror("not a valid level 0 FBA file");

  j = 0;
  fbaSTW = LongWord(fba[j].head.STW);

  /*    printf("rename '%s' to '%s'\n", oldname, newname); */
  if (rename(oldname, newname) != 0) ODINerror("failed to rename file");

  new = fopen(oldname, "w");
  if (new == NULL) ODINerror("can't open file '%s' for writing");
  
  for (i = 0; i < nac; i++) {
    sync = ac[i].head.UserHead;
    if ((sync & 0xff0f) == 0x7300) { /* first block */
      acSTW = LongWord(ac[i].head.STW);
      while (fbaSTW < acSTW) {
	j++;
	if (j >= nfba) {
	  ODINwarning("STW too large");
	  break;
	}
	fbaSTW = LongWord(fba[j].head.STW);
      }
      if (j < 1) {
	ODINwarning("STW too small");
      } else {
	ac[i].data[ 9] = fba[j-1].mech[0];
	ac[i].data[10] = fba[j-1].mech[1];
	/*        printf("%4d %4d %08x %08x %04x %04x\n",  */
	/*  	     i, j, acSTW, LongWord(fba[j-1].head.STW), */
	/*  	     (ac[i].data[9] & 0xffff), (ac[i].data[10] & 0xffff)); */
      }
    }
  }

  if (fwrite(ac, sizeof(ACSBlock), nac, new) != nac) {
    ODINerror("failed to write AC data");
  }
  ODINinfo("added mirror info to file '%s'", oldname);
  fclose(new);

  free(ac);
  free(fba);

  exit(0);
}
