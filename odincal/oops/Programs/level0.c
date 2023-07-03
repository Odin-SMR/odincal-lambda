#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>

#include "options.h"

#include "odinlib.h"
#include "osu.h"

#define MAXFORMATS 32

static char rcsid[] = "$Id$";

int readonly = 0;
int verbose  = 0;
static int rstcnt;

static char osufile[MAXNAMLEN+1] = "";

#define GROUPS 5
#define HKGRP  0
#define AC1GRP 1
#define AC2GRP 2
#define AOSGRP 3
#define FBAGRP 4

#define VALIDBIT 0x8000
#define ASTROBIT 0x4000

int retrieve[GROUPS];
int blocks[GROUPS];
unsigned short SSCindex[GROUPS];

char PROGRAM_NAME[] = "level0";
char description[] = "retrieve telemetry data and store as level0";
char required[] = "filename";
struct _options options[] = {
{ "-scan",	       "only scan of telemetry file, no level 0 data output" },
{ "-hk",	       "retrieve level 0 HK data" },
{ "-aos",	       "retrieve level 0 AOS data" },
{ "-ac1",	       "retrieve level 0 AC1 data" },
{ "-ac2",	       "retrieve level 0 AC2 data" },
{ "-fba",	       "retrieve level 0 FBA data" },
{ "-all",	       "retrieve all level 0 data" },
{ "-file filename",    "specify telemetry data file" },
{ "-verbose",	       "produce verbose output" },
{ "-reset n",          "use STW reset counter n in file names" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
  char *opt, *optarg;

  opt = (*pargv)[0] + 1;

  if (!strcmp(opt, "scan")) {
    readonly = 1;
    return;
  }
  if (!strcmp(opt, "hk")) {
    retrieve[HKGRP] = 1;
    return;
  }
  if (!strcmp(opt, "ac1")) {
    retrieve[AC1GRP] = 1;
    return;
  }
  if (!strcmp(opt, "ac2")) {
    retrieve[AC2GRP] = 1;
    return;
  }
  if (!strcmp(opt, "aos")) {
    retrieve[AOSGRP] = 1;
    return;
  }
  if (!strcmp(opt, "fba")) {
    retrieve[FBAGRP] = 1;
    return;
  }
  if (!strcmp(opt, "all")) {
    retrieve[HKGRP]  = 1;
    retrieve[AC1GRP] = 1;
    retrieve[AC2GRP] = 1;
    retrieve[AOSGRP] = 1;
    retrieve[FBAGRP] = 1;
    return;
  }
  if (!strcmp(opt, "file")) {
    GetOption(&optarg, pargc, pargv);
    strcpy(osufile, optarg);
    return;
  }
  if (!strcmp(opt, "verbose")) {
    verbose = 1;
    return;
  }
  if (!strcmp(opt, "reset")) {
    GetOption(&optarg, pargc, pargv);
    rstcnt = atoi(optarg);
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
  HKBlock  *hkblock;
  AC1Block *ac1block;
  AC2Block *ac2block;
  AOSBlock *aosblock;
  FBABlock *fbablock;

  Format format;
  unsigned short sync;
  unsigned long stw, first, last;
  int nf;

  FILE *tm, *aos, *ac1, *ac2, *shk, *fba;
  static char shkfile[MAXNAMLEN+1], aosfile[MAXNAMLEN+1];
  static char ac1file[MAXNAMLEN+1], ac2file[MAXNAMLEN+1];
  static char fbafile[MAXNAMLEN+1];

  logname("level0");
  GetOpts(argc, argv);

  if (argc == 1) ODINerror("filename required as argument");

  if (!strcmp(osufile, "-")) tm = stdin;
  else {
    if (osufile[0] == '\0') strcpy(osufile, argv[argc-1]);
    tm = fopen(osufile, "r");
    if (tm == NULL) ODINerror("can't open data file '%s'", osufile);
  }

  aos = ac1 = ac2 = shk = fba = (FILE *)NULL;

  /* Read all formats */
  nf = 0;
  while (GetFormat(tm, format)) {
    stw = GetSTW(format);     /* get STW information */
    if (nf == 0) first = stw;

    /* if we don't have an output file yet, open it */
    if (shk == NULL && !readonly && retrieve[HKGRP]) {
      if ((shk = OpenSHKFile(rstcnt, stw, shkfile)) == NULL) 
	ODINerror("can't open HK data file '%s'", shkfile);
    }
    
    hkblock = NewHKBlock(stw);
    hkblock->index = VALIDBIT | ASTROBIT | SSCindex[HKGRP];
    SSCindex[HKGRP]++;

    GetHKData(format, hkblock);
    blocks[HKGRP]++;
    if (shk && !PutHKBlock(shk, hkblock)) 
      ODINerror("error writing HK block");

    /* test for AOS data */
    if ((sync = AOSsync(format))) {
      /* if we don't have an output file yet, open it */
      if (aos == NULL && !readonly && retrieve[AOSGRP]) {
	if ((aos = OpenAOSFile(rstcnt, stw, aosfile)) == NULL) 
	  ODINerror("can't open AOS data file '%s'", aosfile);
      }
      
      if ((sync & 0x000f) == 0) SSCindex[AOSGRP]++;

      aosblock = NewAOSBlock(stw);
      aosblock->head.UserHead = sync;
      aosblock->index = VALIDBIT | ASTROBIT | SSCindex[AOSGRP];
      GetAOSData(format, aosblock);
      blocks[AOSGRP]++;
      if (aos && !PutAOSBlock(aos, aosblock)) 
	ODINerror("error writing AOS block");
    }

    /* test for AC1 data */
    if ((sync = AC1sync(format))) {
      if ((sync & 0xfff0) != AC1USER) {
	if ((sync & 0x000f) == 0) ODINwarning("AC1 has wrong user number");
	sync &= 0xff0f;
	sync |= AC1USER;  /* correct user number */
      }

      /* if we don't have an output file yet, open it */
      if (ac1 == NULL && !readonly && retrieve[AC1GRP]) {
	if ((ac1 = OpenAC1File(rstcnt, stw, ac1file)) == NULL) 
	  ODINerror("can't open AC1 data file '%s'", ac1file);
      }
      
      if ((sync & 0x000f) == 0) SSCindex[AC1GRP]++;

      ac1block = NewAC1Block(stw);
      ac1block->head.UserHead = sync;
      ac1block->index = VALIDBIT | ASTROBIT | SSCindex[AC1GRP];
      blocks[AC1GRP]++;
      GetAC1Data(format, ac1block);
      if (ac1 && !PutAC1Block(ac1, ac1block)) 
	ODINerror("error writing AC1 block");
    }

    /* test for AC2 data */
    if ((sync = AC2sync(format))) {
      if ((sync & 0xfff0) != AC2USER) {
	if ((sync & 0x000f) == 0) ODINwarning("AC2 has wrong user number");
	sync &= 0xff0f;
	sync |= AC2USER;  /* correct user number */
      }

      /* if we don't have an output file yet, open it */
      if (ac2 == NULL && !readonly && retrieve[AC2GRP]) {
	if ((ac2 = OpenAC2File(rstcnt, stw, ac2file)) == NULL) 
	  ODINerror("can't open AC2 data file '%s'", ac2file);
      }
      
      if ((sync & 0x000f) == 0) SSCindex[AC2GRP]++;

      ac2block = NewAC2Block(stw);
      ac2block->head.UserHead = sync;
      ac2block->index = VALIDBIT | ASTROBIT | SSCindex[AC2GRP];
      GetAC2Data(format, ac2block);
      blocks[AC2GRP]++;
      if (ac2 && !PutAC2Block(ac2, ac2block)) 
	ODINerror("error writing AC2 block");
    }

    /* test for FBA data */
    if ((sync = FBAsync(format))) {
      /* if we don't have an output file yet, open it */
      if (fba == NULL && !readonly && retrieve[FBAGRP]) {
	if ((fba = OpenFBAFile(rstcnt, stw, fbafile)) == NULL) 
	  ODINerror("can't open FBA data file '%s'", fbafile);
      }
      fbablock = NewFBABlock(stw);
      /* fbablock->head.UserHead = sync; */
      fbablock->index = VALIDBIT | ASTROBIT | SSCindex[FBAGRP];
      blocks[FBAGRP]++;
      GetFBAData(format, fbablock);
      if (fba && !PutFBABlock(fba, fbablock)) 
	ODINerror("error writing FBA block");
    }

    nf++;    /* count formats */
  }
  last = stw;

  ODINinfo("file '%s' contained %d formats", osufile, nf);
  ODINinfo("STW range: [%08x:%08x]", first, last);
  ODINinfo("number of HK  blocks =%5d", blocks[HKGRP]);
  ODINinfo("number of AC1 blocks =%5d", blocks[AC1GRP]);
  ODINinfo("number of AC2 blocks =%5d", blocks[AC2GRP]);
  ODINinfo("number of AOS blocks =%5d", blocks[AOSGRP]);
  ODINinfo("number of FBA blocks =%5d", blocks[FBAGRP]);

  fclose(tm);
  if (shk) {
    ODINinfo("HK  data written to file '%s'", shkfile);
    printf("%s\n", shkfile);
    fclose(shk);
  }
  if (ac1) {
    ODINinfo("AC1 data written to file '%s'", ac1file);
    printf("%s\n", ac1file);
    fclose(ac1);
  }
  if (ac2) {
    ODINinfo("AC2 data written to file '%s'", ac2file);
    printf("%s\n", ac2file);
    fclose(ac2);
  }
  if (aos) {
    ODINinfo("AOS data written to file '%s'", aosfile);
    printf("%s\n", aosfile);
    fclose(aos);
  }
  if (fba) {
    ODINinfo("FBA data written to file '%s'", fbafile);
    printf("%s\n", fbafile);
    fclose(fba);
  }

  exit(0);
}	
