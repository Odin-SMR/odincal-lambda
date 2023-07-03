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

char PROGRAM_NAME[] = "acdump";
char description[] = "retrieve level 0 data and dump info";
char required[] = "";
struct _options options[] = {
{ "-table",	       "Produce data in table format" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

static int table = 0;

void ParseOpts(int *pargc, char ***pargv)
{
    char *opt, *optarg;

    opt = (*pargv)[0] + 1;
    optarg = NULL;

    if (!strcmp(opt, "table")) {
	table = 1;
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
    ACSBlock *block;
    static float data[MAXCHANNELS];

    unsigned long STW;
    unsigned long mon[2][8], lag0[8];
    float monitor[2][8], zlag[8];
    unsigned long samples;
    float inttime, tcmd;
    int i, k, n, bands, ssb, att, mode, omode, ps, ch, err, *seq;
    int sync, backend, frontend, spectra, nChannels;
    static char *Frontend[] = { "???", "555", "495", "572", "549", "119", "SPL" };
    static char *Backend[] = { "???", "AC1", "AC2", "AOS" };

    /*    unsigned short msb, lsb; */
    /*    int z; */

    GetOpts(argc, argv);
    logname(PROGRAM_NAME);

    if (argc == 1) ODINerror("filename required as argument");

    block = ReadACSLevel0(argv[argc-1], &n);
    if (block == NULL) ODINerror("not a valid level 0 AC file");

    /*    ac = GetACSscan(block, &n); */
   
    spectra = 0;
    for (i = 0; i+13 <= n; i++) {
	sync = block[i].head.UserHead;

	if ((sync & 0xff0f) == 0x7300) { /* first block */
	    if (table) {
	    } else {
		if (spectra) printf("\n");
	    }
	    spectra++;
	    STW = LongWord(block[i].head.STW);
	    if (table) {
		printf("%10d", STW /* spectra */);
	    } else {
		printf("spectrum:         %08lx\n", STW /* spectra */);
	    }

	    switch (sync) {
	      case 0x7380:
		backend = AC1;
		break;
	      case 0x73b0:
		backend = AC2;
		break;
	      default:
		ODINwarning("unknown backend");
		backend = 0;
		break;
	    }
	    /*      12345678901234567890 */
	    frontend = GetACInput(block+i);
	    if (table) {
		printf(" %2d %2d", backend, frontend);
	    } else {
		printf("backend/frontend: %s/%s\n", 
		       Backend[backend], Frontend[frontend]);
	    }

	    omode = (block[i].data[35]>>8) & 0xff;
	    mode = GetACMode(block+i);
	    if (table) {
		printf(" %2d%4d", mode, omode);
	    } else {
		printf("mode:             %d [%02X] ", mode, omode);
	    }
	    if (block[i].data[36] & 0x01) {
		if (table) {
		    printf(" %5d", block[i].data[8] & 0xffff);
		} else {
		    printf("switched ");
		    printf("%04x\n", (block[i].data[8] & 0xffff));
		}
	    } else {
		if (table) {
		    printf(" %5d", block[i].data[8] & 0xffff);
		} else {
		    printf("unswitched ");
		    printf("%04x\n", (block[i].data[8] & 0xffff));
		}
	    }

	    ps = GetPrescaler(block+i);
	    if (table) {
		printf(" %2d", ps);
	    } else {
		printf("prescaler:        %d\n", ps);
	    }

	    /*        samples = (float)(GetSamples(block+i) >> (ps-2)); */
	    samples = GetSamples(block+i);
	    inttime  = (float)samples/SAMPLEFREQ;
	    tcmd = GetCmdIntTime(block+i);
	    if (table) {
		printf(" %5.2f %5.2f %10lu", inttime, tcmd, samples);
	    } else {
		printf("int.time:        %5.4f %5.2f (%lu)\n", 
		       inttime, tcmd, samples);
	    }

	    /*      12345678901234567890 */
	    if (table) {
	    } else {
		printf("LO frequencies:  ");
	    }
	    for (k = 0; k < 4; k++) {
		ssb = (int)(GetSSBfrequency(block+i, k)/1.0e6);
		printf(" %4d", ssb);
	    }
	    if (table) {
	    } else {
		printf("\n");
	    }

	    /*        printf("SSB att:       "); */
	    if (table) {
	    } else {
		printf("attenuators:     ");
	    }
	    for (k = 0; k < 4; k++) {
		att = GetSSBattenuator(block+i, k, &err);
		printf(" %4d", att);
	    }
	    if (table) {
	    } else {
		printf("\n");
	    }

	    seq = GetACSequence(mode);
	    bands = 0;
	    for (k = 0; k < 8; k++) {
		if (seq[2*k]) bands++;
	    }

	    nChannels = GetACChannels(data, block+i);
	    for (k = 0; k < 8; k++) {
		if (seq[2*k]) {
		    lag0[k] = GetZLag(block+i, k);	
		    zlag[k] = (float)lag0[k];
		    if (samples > 0L) {
			zlag[k] *= 2048.0*(SAMPLEFREQ/(float)samples)/(CLOCKFREQ/2.0);
			if (zlag[k] > 1.0) zlag[k] = 9.9999;
		    } else {
			zlag[k] = 0.0;
		    }
		    mon[POS][k] = GetMonitor(block+i, k, POS);
		    mon[NEG][k] = GetMonitor(block+i, k, NEG);
		    monitor[POS][k] = (float)mon[POS][k];
		    monitor[NEG][k] = (float)mon[NEG][k];		    
		    if (samples > 0.0) {
		       	monitor[POS][k] *= 1024.0*((float)SAMPLEFREQ/(float)samples)/((float)CLOCKFREQ/2.0);
			monitor[NEG][k] *= 1024.0*(SAMPLEFREQ/samples)/(CLOCKFREQ/2.0);
			if (monitor[POS][k] > 1.0) monitor[POS][k] = 9.9999;
			if (monitor[NEG][k] > 1.0) monitor[NEG][k] = 9.9999;
		    } else {
			monitor[POS][k] = 0.0;
			monitor[NEG][k] = 0.0;
		    }
		} else {
		    lag0[k] = 0;
		    zlag[k] = 0.0;
		    mon[POS][k] = 0;
		    mon[NEG][k] = 0;
		    monitor[POS][k] = 0.0;
		    monitor[NEG][k] = 0.0;
		}
	    }

	    if (table) {
	    } else {
		printf("monitor (pos):   ");
		for (k = 0; k < MAXCHIPS; k++) {
		    if (seq[2*k] == 0) printf(" ------");
		    else               printf(" %06lx", mon[POS][k]);
		}
		printf("\n                 ");
		for (k = 0; k < MAXCHIPS; k++) 
		    printf(" %6.4f", monitor[POS][k]);
		printf("\n");
		printf("monitor (neg):   ");
		for (k = 0; k < MAXCHIPS; k++) {
		    if (seq[2*k] == 0) printf(" ------");
		    else               printf(" %06lx", mon[NEG][k]);
		}
		printf("\n                 ");
		for (k = 0; k < MAXCHIPS; k++) 
		    printf(" %6.4f", monitor[NEG][k]);
		printf("\n");
	    }
	    
	    if (table) {
	    } else {
		printf("zero lags:       ");
		for (k = 0; k < MAXCHIPS; k++) {
		    if (seq[2*k] == 0) printf(" ------");
		    else               printf(" %06lx", lag0[k]);
		}
		printf ("\n                 ");
		for (k = 0; k < MAXCHIPS; k++) 
		    printf(" %6.4f", zlag[k]);	    
		printf("\n");
	    }
	    
	    if (table) {
	    } else {
		printf ("lags 1-8:        ");
		if (nChannels > 0) {
		    for (k = 1; k <= 8; k++) {
			ch = (int)data[2*MAXCHIPS+k];
			if (k == 2 && ch > 0) {
			    ch -= 65536;
			}
			printf(" %06x", ch & 0xffffff);
		    }
		} else {
		    for (k = 1; k <= 8; k++) {
			printf(" 000000");
		    }
		}
	    }
	    printf ("\n");
	}
    }
    ODINinfo("file '%s' contained %d spectra", argv[argc-1], spectra);
    /* ODINinfo("STW range: [%08x:%08x]", ac[0].STW, ac[n-1].STW); */
    free(block);

    exit(0);
}
