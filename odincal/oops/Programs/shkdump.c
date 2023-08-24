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

int subcom[HKBLOCKSIZE] = {
    1,
    1, 1, 0, 0,
    1, 1, 1,
    1, 1, 1, 1, 1,
    1, 1, 1, 1,
    1, 1, 1, 1, 
    1, 1, 1, 1,
    1, 1, 1, 1, 
    1, 1, 1, 1,
    0, 1, 1, 1, 1, 1,
    0, 1, 1, 1, 1, 1,
    0,
    1,
    0, 0,
    1,
    1, 1, 1,
    1, 1,
    1,
    1,
    0, 0, 0,
    1, 1, 1,
    0, 0
};

char *SHKname[HKBLOCKSIZE] = {
    "HK47",
    "AOS[0]", "AOS[1]", "AOS[2]", "AOS[3]",
    "OS[0]", "OS[1]", "OS[2]",
    "HK46", "HK4C", "HK48", "HK4D", "HK49",
    "HK40", "HK41", "HK42", "HK43",
    "PLLA[0]", "PLLA[1]", "PLLA[2]", "PLLA[3]", 
    "PLLA[4]", "PLLA[5]", "PLLA[6]", "PLLA[7]",
    "PLLB[0]", "PLLB[1]", "PLLB[2]", "PLLB[3]", 
    "PLLB[4]", "PLLB[5]", "PLLB[6]", "PLLB[7]",
    "MECHA[0]", "MECHA[1]", "MECHA[2]", "MECHA[3]", "MECHA[4]", "MECHA[5]",
    "MECHB[0]", "MECHB[1]", "MECHB[2]", "MECHB[3]", "MECHB[4]", "MECHB[5]",
    "HKpyro",
    "HK4A",
    "ACDC1sync", "ACDC2sync",
    "ACS",
    "HK20", "HK21", "HK22",
    "HK44", "HK45",
    "ACDC1slow", "ACDC2slow",
    "HK30", "HK31", "HK32",
    "Gy1", "Gy2", "Gy3",
    "HK4E", "HK4F"
};

static char shkfile[MAXNAMLEN+1] = "";

char PROGRAM_NAME[] = "shkdump";
char description[] = "retrieve house-keeping data";
char required[] = "filename";
struct _options options[] = {
{ "-file filename",    "specify house-keeping data file" },
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
  static struct stat buf;
  HKBlock hk;
  /* unsigned short sync, user; */
  unsigned short hknext;
  unsigned long stw, sum;
  static unsigned short hkword[8], hkindex[8];
  int nBlocks, i, j, k, n, index;
  FILE *shk;

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (argc == 1) ODINerror("filename required as argument");

  if (shkfile[0] == '\0') strcpy(shkfile, argv[argc-1]);

  /* Get file status to retrieve file size */
  if (stat(shkfile, &buf) == -1) 
    ODINerror("can't stat data file '%s'", shkfile);
  
  /* Is file size a multiple of the size of one HKBlock ? */
  if (buf.st_size % sizeof(HKBlock)) 
    ODINerror("file size error");
  
  /* Calculate number of HKBlocks */
  nBlocks = buf.st_size/sizeof(HKBlock);

  shk = fopen(shkfile, "r");
  if (shk == NULL) ODINerror("can't open HK data file '%s'", shkfile);
  
  for (i = 0; i < nBlocks; i++) {
    /* printf("block %d\n", i); */
    n = fread(&hk, sizeof(HKBlock), 1, shk);
    if (n != 1) ODINerror("read error");

    if (hk.head.SYNC != SYNCWORD) ODINwarning("wrong sync word");
    stw = LongWord(hk.head.STW);

    for (k = 0; k < HKBLOCKSIZE; k++) {
      printf("%d", stw);
      printf(":%s", SHKname[k]);
      if (subcom[k]) {
	printf("[%d]", hk.data[k] & 0x000f);
	hk.data[k] >>= 4;
      }
      printf(":%d\n", hk.data[k]);
    }
  }

  fclose(shk);

  exit(0);
}
