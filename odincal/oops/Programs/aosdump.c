#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <time.h>

#include "options.h"
#include "odinlib.h"
#include "aoslib.h"

static char rcsid[] = "$Id$";

char PROGRAM_NAME[] = "aosdump";
char description[] = "retrieve level 0 data and dump info";
char required[] = "";
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
  AOSBlock *block;

  unsigned long STW, format[2], checksum;
  unsigned int syncword[5];
  float samples, inttime;
  int i, k, n, mode;
  int sync, backend, frontend, spectra;
  static char *Frontend[] = { "???", "555", "495", "572", "549", "119" };
  static char *Backend[] = { "???", "AC1", "AC2", "AOS" };

  GetOpts(argc, argv);
  logname(PROGRAM_NAME);

  if (argc == 1) ODINerror("filename required as argument");

  block = ReadAOSLevel0(argv[argc-1], &n);
  if (block == NULL) ODINerror("not a valid level 0 AOS file");

  spectra = 0;
  for (i = 0; i+16 <= n; i++) {
    sync = block[i].head.UserHead;

    if ((sync & 0xff0f) == 0x7300) { /* first block */
      mode = AOSformat(block+i);
      if (mode == 322) continue;
      
      if (spectra) printf("\n");
      spectra++;
      STW = LongWord(block[i].head.STW);
      printf("spectrum:         %08lx\n", STW /* spectra */);

      switch (sync) {
       case 0x7360:
	backend = AOS;
	break;
       default:
	ODINwarning("unknown backend");
	backend = 0;
	break;
      }
      frontend = GetAOSInput(block+i);
      /*      12345678901234567890 */
      printf("backend/frontend: %s/%s\n", 
	     Backend[backend], Frontend[frontend]);
/*        printf("backend:        %d\n", backend); */
/*        printf("mode:             %d\n", mode); */

      printf("mode:             %d ", mode);
      if (mechAmask(block+i) || mechBmask(block+i)) {
	printf("switched ");
	if (mechAmask(block+i)) printf("%04x ",(mechAsync(block+i) & 0xffff));
	else                    printf("%04x ",(mechBsync(block+i) & 0xffff));
      } else {
	printf("unswitched ");
      }
      printf("%04x\n", (CalMirror(block+i) & 0xffff));

      samples = (float)CCDreadouts(block+i);
      inttime  = samples*(1760.0/3.0e5) /* 0.00528 */;
      printf("int.time:         %.2f (%d)\n", inttime, (int)samples);

      format[0] = StartFormat(block+i);
      format[1] = EndFormat(block+i);
      /*      12345678901234567890 */
      printf("Formats:          %08lx %08lx\n", format[0], format[1]);

      syncword[0] = (unsigned int)ACDC1sync(block+i) & 0xffff;
      syncword[1] = (unsigned int)ACDC2sync(block+i) & 0xffff;
      syncword[2] = (unsigned int)mechAsync(block+i) & 0xffff;
      syncword[3] = (unsigned int)mechBsync(block+i) & 0xffff;
      syncword[4] = (unsigned int)F119sync(block+i)  & 0xffff;
      printf("sync words:      ");
      for (k = 0; k < 5; k++) {
	printf(" %04x", syncword[k]);
      }
      printf("\n");

      syncword[0] = (unsigned int)ACDC1mask(block+i) & 0xffff;
      syncword[1] = (unsigned int)ACDC2mask(block+i) & 0xffff;
      syncword[2] = (unsigned int)mechAmask(block+i) & 0xffff;
      syncword[3] = (unsigned int)mechBmask(block+i) & 0xffff;
      syncword[4] = (unsigned int)F119mask(block+i)  & 0xffff;
      printf("mask words:      ");
      for (k = 0; k < 5; k++) {
	printf(" %04x", syncword[k]);
      }
      printf("\n");

      /*        printf("ACDC sync 1/2: "); */
      /*        printf("mech sync A/B: "); */
      /*        printf("F119 sync:     "); */

      printf("functioning:     ");
      printf(" %04x", FuncMode(block+i));
      printf(" %04x", NSec(block+i));
      printf(" %04x", Nswitch(block+i));
      printf(" %04x", AOSoutput(block+i));
      printf("\n");

      printf("commands control:");
      for (k = 0; k < 5; k++) {
	printf(" %04x", cmdReadback(block+i, k));
      }
      printf("\n");

      checksum = CheckSum(block+i);
      printf("check sum:        %lu\n", checksum);
    }
  }
  ODINinfo("file '%s' contained %d spectra", argv[argc-1], spectra);
/*    ODINinfo("STW range: [%08x:%08x]", ac[0].STW, ac[n-1].STW); */
  free(block);

  exit(0);
}


