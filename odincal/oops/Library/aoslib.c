#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#include "odinscan.h"
#include "odinlib.h"
#include "aoslib.h"
#include "odintime.h"
#include "astrolib.h"

static char rcsid[] = "$Id$";

static int rstcnt;

/*
 * The following routines retrieve AOS ancillary information
 */
short AOSformat(AOSBlock *aos)
{
  return aos->data[1];
}

unsigned long StartFormat(AOSBlock *aos)
{
  return LongWord(&(aos->data[2]));
}

unsigned long EndFormat(AOSBlock *aos)
{
  return LongWord(&(aos->data[4]));
}

short ACDC1sync(AOSBlock *aos)
{
  return aos->data[6];
}

short ACDC2sync(AOSBlock *aos)
{
  return aos->data[7];
}

short mechAsync(AOSBlock *aos)
{
  return aos->data[8];
}

short mechBsync(AOSBlock *aos)
{
  return aos->data[9];
}

short F119sync(AOSBlock *aos)
{
  return aos->data[10];
}

short CalMirror(AOSBlock *aos)
{
  short mirror, sync;

  mirror = aos->data[11];
  if (mechAmask(aos)) {
    mirror &= 0x000f;
  } else if (mechBmask(aos)) {
    mirror >>= 8;
    mirror &= 0x000f;
  } else {
    sync = mechAsync(aos);
    if (sync == (short)0xffff) {
      mirror >>= 8;
      mirror &= 0x000f;
    } else {
      mirror &= 0x000f;
    }
  }
  return mirror;
}

short FuncMode(AOSBlock *aos)
{
  return aos->data[12];
}

short NSec(AOSBlock *aos)
{
  return aos->data[13];
}

short Nswitch(AOSBlock *aos)
{
  return aos->data[14];
}

short AOSoutput(AOSBlock *aos)
{
  return aos->data[15];
}

short ACDC1mask(AOSBlock *aos)
{
  return aos->data[17];
}

short ACDC2mask(AOSBlock *aos)
{
  return aos->data[18];
}

short mechAmask(AOSBlock *aos)
{
  return aos->data[19];
}

short mechBmask(AOSBlock *aos)
{
  return aos->data[20];
}

short F119mask(AOSBlock *aos)
{
  return aos->data[21];
}

short HKinfo(AOSBlock *aos, int index)
{
  return aos->data[22+index];
}

short cmdReadback(AOSBlock *aos, int index)
{
  return aos->data[30+index];
}

short CCDreadouts(AOSBlock *aos)
{
  return aos->data[35];
}

unsigned long CheckSum(AOSBlock *aos)
{
  return LongWord(&(aos->data[36]));
}

/* 
   Get input channel used by AOS
   
   input:
   - AOSBlock *aos  pointer to aos block
*/
int GetAOSInput(AOSBlock *aos)
{
  switch (cmdReadback(aos, 2)) {
   case 1: 
    return REC_555;
    break;
   case 2:
    return REC_572;
    break;
   case 4:
    return REC_495;
    break;
   case 8:
    return REC_549;
    break;
   case 16:
    return REC_119;
    break;
   default:
    return 0;
    break;
  }
}

#define ALIGNED    1
#define NOTALIGNED 2

/* 
   Get spectrum type for AOS
   
   input:
   - AOSBlock *aos  pointer to aos block
*/
int GetAOSType(AOSBlock *aos)
{
  int type = 0;
  short sync;

  if (mechAmask(aos)) {
    type = (mechAsync(aos) & 0x0080) == 0 ? NOTALIGNED : ALIGNED;
  } else if (mechBmask(aos)) {
    type = (mechBsync(aos) & 0x0080) == 0 ? NOTALIGNED : ALIGNED;
  } else {
    sync = mechAsync(aos);
    if (sync == (short)0xffff) sync = mechBsync(aos);
    type = (sync & 0x0080) == 0 ? NOTALIGNED : ALIGNED;
  }
  return type;
}

AOSBlock *ReadAOSLevel0(char *filename, int *fcnt)
/* 
   Function 'ReadLevel0' reads a level 0 file (STW.AOS) containing AOS 
   science data.
   The data type 'Format' is declared in 'tm.h'.
   
   function variables:
   - char *filename    a pointer to the name of the file to be read
   - int *fcnt         pointer to an integer which will receive 
                       the number of formats found
   return value:
   - a pointer to the array of AOSBlock found
*/
{
  static struct stat buf;
  int nBlocks, n, i;
  AOSBlock *aos;
  FILE *tmdata;

  /* Reset number of AOSBlock counter */
  *fcnt = 0;

  /* Get file status to retrieve file size */
  if (stat(filename, &buf) == -1) {
    ODINwarning("can't get file status for '%s'", filename);
    return (AOSBlock *)NULL;
  }
  
  /* Is file size a multiple of the size of one AOSBlock ? */
  if (buf.st_size % sizeof(AOSBlock)) {
    ODINwarning("file size error for '%s'", filename);
    return (AOSBlock *)NULL;
  }
  
  /* Calculate number of AOSBlocks */
  nBlocks = buf.st_size/sizeof(AOSBlock);

  /* Allocate required array of AOS blocks */
  aos = (AOSBlock *)calloc(nBlocks, sizeof(AOSBlock));
  if (aos == NULL) 
    ODINerror("memory allocation error");

  /* Open the file for reading */
  tmdata = fopen(filename, "r");
  if (tmdata == NULL)
    ODINerror("can't open data file '%s'", filename);
  
  /* Read all AOS blocks */
  n = fread(aos, sizeof(AOSBlock), nBlocks, tmdata);
  if (n != nBlocks)
    ODINerror("read error");

  for (i = 0; i < nBlocks; i++) {
    if (aos[i].head.SYNC != SYNCWORD) {
      ODINwarning("wrong sync word");
      fclose(tmdata);
      return (AOSBlock *)NULL;
    }
  }

  /* Set return values */
  /* ODINinfo("%d AOS formats in file '%s'", nBlocks, filename); */
  fclose(tmdata);
  *fcnt = nBlocks;
  rstcnt = STWreset(filename);
  return aos;
}

int GetAOSChannels(int mode, float *data, AOSBlock *aos)
/*
   Function 'GetAOSChannels' extracts AOS channel contents from 
   one set (16 or 32) of AOSBlock's.
   
   function variables:
   - int mode          the mode in which the data were taken
   - float *data       a pointer to an array to receive the data
   - AOSBlock *aos     a pointer to the first format holding the data

   return value:
   - the number of channels extracted or -1 in case of error
*/
{
  int i, k, n, block, word, shift, bit, mode2;
  unsigned long readout, chksum, sum, stw;

  chksum = CheckSum(aos);
  shift = (int)log((double)CCDreadouts(aos)) + 1;
  /* Reset channel counter */
  k = 0;
  sum = 0;

  /* Data are encoded depending on mode */
  switch (mode) {
   case 111:
    /* Mode 111 has 1728 16-bit data distributed over 16 formats.
       The data are preceeded by 38 words of service data 
       in the first block. 
       The first 14 words are unused pixels and are skipped */
    for (block = 0; block < 16; block++) {
      for (word = (block == 0 ? 38+14 : 0); word < AOSBLOCKSIZE; word++) {
	/* data[k++] = (float)(aos[block].data[word]<<shift); */
	/* printf("data[%d] 0x%04x (<< %d)\n", k, aos[block].data[word], shift); */
	readout = aos[block].data[word]<<shift;
	sum += readout;
	data[k++] = (float)readout;
	if (k == MAXCHANNELS) {
	  /* sum <<= shift; */
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;

   case 211:
    /* Mode 211 has 864 32-bit data distributed over 16 formats.
       The data are preceeded by 38 words of service data in the first block.
       The first 7 long words (14 words) are unused pixels and are skipped.
       We fill two consecutive channels with the same value to yield a 
       total of 1728 channels */
    for (block = 0; block < 16; block++) {
      for (word = (block == 0 ? 38+14 : 0); word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	/* printf("data[%d] %10d\n", k, readout); */
	for (i = 0; i < 2; i++) data[k++] = (float)readout;
	if (k == MAXCHANNELS) {
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;

  case 312:
    /* Mode 312 has 1728 32-bit data distributed over 32 formats.
       The data are preceeded by 38 words of service data
       in the first format and another 38 words of service data
       in the 16th format, which should have mode = 322
       The first 14 long words (28 words) are unused pixels and are skipped */
    mode2 = AOSformat(aos+16); /* location of mode information 2nd half */
    if (mode2 != 322) {
      ODINwarning("mode 312 not followed by mode 322");
      return (-1);
    }
    for (block = 0; block < 32; block++) {
      word = ((block%16) == 0 ? 38 : 0);
      if (block == 0) word += 28;
      for (; word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	/* 	data[k] = (float)LongWord((aos[block].data)+word); */
	data[k] = (float)readout;
	k++;
	if (k == MAXCHANNELS) {
	  /*  	  if (sum != chksum) */
	    /* ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;
   case 411:
    /* Mode 411 has 426 32-bit data distributed over 8 formats.
       The data are preceeded by 38 words of service data in the first block.
       The first 2 long words (4 words) are unused pixels and are skipped.
       After that 3 long words follow representing sums over 8 pixels, which
       will be stored at channels 0 ... 23.
       After that 420 long words follow representing sums over 4 pixels, which
       will be stored at channels 24 ... 1703.
       After that 3 long words follow representing sums over 8 pixels, which
       will be stored at channels 1704 ... 1727.
       We fill 4(8) consecutive channels with the same value to yield a 
       total of MAXCHANNELS channels */
    for (block = 0; block < 8; block++) {
      for (word = (block == 0 ? 38+4 : 0); word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	if ((k < 24) || (k >= 1704)) {
	  for (i = 0; i < 8; i++) data[k++] = (float)readout/8.0;
	} else {
	  for (i = 0; i < 4; i++) data[k++] = (float)readout/4.0;
	}
	if (k == MAXCHANNELS) {
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;
   case 511:
    /* Mode 511 has 146 32-bit data distributed over 3 formats.
       The data are preceeded by 38 words of service data in the first block.
       The first 2 long words (4 words) are unused pixels and are skipped.
       After that 25 long words follow representing sums over 32 pixels, which
       will be stored at channels 0 ... 799.
       After that 16 long words follow representing sums over 2 pixels, which
       will be stored at channels 800 ... 831.
       After that 64 long words follow representing sums over 1 pixel, which
       will be stored at channels 832 ... 895.
       After that 16 long words follow representing sums over 2 pixels, which
       will be stored at channels 896 ... 927.
       After that 25 long words follow representing sums over 32 pixels, which
       will be stored at channels 928 ... 1727.
       We fill 32/2/1 consecutive channels with the same value to yield a 
       total of MAXCHANNELS channels */
    for (block = 0; block < 3; block++) {
      for (word = (block == 0 ? 38+4 : 0); word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	if ((k < 800) || (k >= 928)) {
	  for (i = 0; i < 32; i++) data[k++] = (float)readout/32.0;
	} else if (((k >= 800) && (k < 832)) || ((k >= 896) && (k < 928))) {
	  for (i = 0; i < 2; i++) data[k++] = (float)readout/2.0;
	} else if ((k >= 832) && (k < 896)) {
	  data[k++] = (float)readout;
	}
	if (k == MAXCHANNELS) {
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;
   case 611:
    /* Mode 611 has 257 32-bit data distributed over 5 formats.
       The data are preceeded by 38 words of service data in the first block.
       The first 2 long words (4 words) are unused pixels and are skipped.
       After that 98 long words follow representing sums over 1 pixel, which
       will be stored at channels 0 ... 97.
       After that 7 long words follow representing sums over 2 pixels, which
       will be stored at channels 98 ... 111.
       After that 47 long words follow representing sums over 32 pixel, which
       will be stored at channels 112 ... 1615.
       After that 7 long words follow representing sums over 2 pixels, which
       will be stored at channels 1616 ... 1629.
       After that 98 long words follow representing sums over 1 pixel, which
       will be stored at channels 1630 ... 1727.
       We fill 32/2/1 consecutive channels with the same value to yield a 
       total of MAXCHANNELS channels */
    for (block = 0; block < 5; block++) {
      for (word = (block == 0 ? 38+4 : 0); word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	if ((k < 98) || (k >= 1630)) {
	  data[k++] = (float)readout;
	} else if (((k >= 98) && (k < 112)) || ((k >= 1616) && (k < 1630))) {
	  for (i = 0; i < 2; i++) data[k++] = (float)readout/2.0;
	} else if ((k >= 112) && (k < 1616)) {
	  for (i = 0; i < 32; i++) data[k++] = (float)readout/32.0;
	}
	if (k == MAXCHANNELS) {
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;
   case 711:
    /* Mode 711 has 258 32-bit channels distributed over 5 formats.
       The data are preceeded by 38 words of service data in the first block.
       The first 2 channels (4 words) are unused pixels and are skipped.
       After that n*8 long words follow representing sums over 24 pixels.
       After that 192 channels follow representing sums over 1 pixel.
       After that (9-n-1)*8 channels follow representing sums over 24 pixels.
       The number n is extracted from the telecommand at channel 9.
       We fill 24/1 consecutive channels with the same value to yield a 
       total of MAXCHANNELS channels */
    /* start testing bits at bit 3. Lowest set bit determines band (n) */
    bit = (1 << 3);
    for (n = 0; n < 8; n++) {
      if (AOSoutput(aos) & bit) break;
      bit <<= 1; /* else test next higher bit */
    }
    for (block = 0; block < 5; block++) {
      for (word = (block == 0 ? 38+4 : 0); word < AOSBLOCKSIZE; word += 2) {
	readout = LongWord((aos[block].data)+word);
	sum += readout;
	if ((k >= n*192) && (k < (n+1)*192)) data[k++] = (float)readout;
	else for (i = 0; i < 24; i++) data[k++] = (float)readout/24.0;
	if (k == MAXCHANNELS) {
/*  	  if (sum != chksum)  */
/*  	    ODINwarning("checksum error %lu <> %lu in mode %d", sum, chksum, mode); */
	  return k;
	}
      }
    }
    break;
  }
  return (-1);
}
 
/*
  Turn blocks of AOS level 0 data into an OdinScan structure.
*/
int AOSBlock2Scan(struct OdinScan *next, AOSBlock *aos)
{
  float samples;
  int k;
  int nChannels, mode;
  unsigned long stw;
  time_t clock;
  struct tm *now;
  double JD, UTC;

  double *c;
  static double fc[4] = { 2100.000e+06,  6.200000e+05, 
			  2.000000e+00,  0.000000e+00 };

  stw = LongWord(aos->head.STW);
  mode = AOSformat(aos);
  /* printf("GetAOSscan: AOS data block %3d in mode %d\n", i, mode); */
  nChannels = GetAOSChannels(mode, next->data, aos);
  if (nChannels == -1) {
    ODINwarning("can't extract channels");
    return 0;
  }

  next->Version  = ODINSCANVERSION;
  next->Level    = 0x0000;
  next->Quality  = rstcnt;
  next->Channels = nChannels;
  next->STW      = stw;
  next->Backend  = AOS;
  next->Discipline = SIdiscipline(aos->index);

  next->Frontend = GetAOSInput(aos);
  switch (GetAOSType(aos)) {
   case NOTALIGNED:
    switch (next->Frontend) {
     case REC_495:
     case REC_549:
      next->Type = REF;
      break;
     case REC_555:
     case REC_572:
      next->Type = SIG;
      break;
     default:
      next->Type = SIG;
      break;
    }
    break;
   case ALIGNED:
    switch (next->Frontend) {
     case REC_495:
     case REC_549:
      next->Type = SIG;
      break;
     case REC_555:
     case REC_572:
      next->Type = REF;
      break;
     default:
      next->Type = REF;
      break;
    }
    break;
  }
  if (next->Type == REF) {
    switch (CalMirror(aos) & 0x000f) {
     case 1:
      next->Type = SK1;
      break;
     case 2:
      next->Type = CAL;
      break;
     case 3:
      next->Type = SK2;
      break;
     default:
      ODINwarning("undefined position of calibration mirror");
      break;
    }
  }

  next->ObsMode = ((mechAmask(aos) || mechBmask(aos)) ? SSW : TPW);

  if (F119mask(aos) != 0) {
    next->ObsMode = FSW;
    next->Type = (F119sync(aos) & 0x0080) == 0 ? REF : SIG;
  }
      
  if (cmdReadback(aos, 2) == 0) next->Type = DRK;
  if (cmdReadback(aos, 4) == 1) next->Type = CMB;

  switch (mode) {
   case 111:
    next->IntMode = AOS_SHORT;
    break;
   case 211:
    next->IntMode = AOS_HALF;
    break;
   case 312:
    next->IntMode = AOS_LONG;
    break;
   case 411:
    next->IntMode = AOS_FOUR;
    break;
   case 511:
    next->IntMode = AOS_CENTRE;
    break;
   case 611:
    next->IntMode = AOS_WINGS;
    break;
   case 711:
    next->IntMode = AOS_WINDOW;
    break;
   default:
    next->IntMode = UNDEFINED;
    break;
  }
  if (next->IntMode == UNDEFINED) {
    ODINwarning("invalid integration mode");
    return 0;
  }
      
  samples = (float)CCDreadouts(aos);
  for (k = 0; k < nChannels; k++) next->data[k] /= samples;
  next->IntTime = samples*(1760.0/3.0e5) /* was 0.00528 */;
  
  /* rstcnt = next->Quality & STWRSTMASK; */
  if (next->STW == 0L) {
    ODINwarning("zero satellite time word");
    return 0;
  }
  clock = stw2utc(next->STW, rstcnt);
  now = gmtime(&clock);
  JD = djl(now->tm_year+1900, now->tm_mon+1, now->tm_mday);
  UTC = (double)(now->tm_hour*3600 + now->tm_min*60 + now->tm_sec)/SECPERDAY;

  next->MJD = jd2mjd(JD)+UTC;
  next->Orbit = mjd2orbit(next->MJD);
  next->Spectrum = 0;

  next->SkyFreq = 0.0;
  next->LOFreq = 0.0;

  /* So far, both frequency and intensity are unreliable */
  next->Quality |= WFREQUENCY;
  next->Quality |= WAMPLITUDE;

  if (next->Type == CMB) {
      c = FitAOSFreq(next);
      if (c) {
	  for (k = 0; k < 4; k++) fc[k] = c[k];
	  ODINinfo("comb: %10.4e %10.4e %10.4e %10.4e", 
		   fc[0], fc[1], fc[2], fc[3]);
      } else {
	  ODINwarning("fitting of polynomial failed");
      }
  }

  /* Initialise with recent values */
  next->FreqCal[0] =  fc[0];
  next->FreqCal[1] =  fc[1];
  next->FreqCal[2] =  fc[2];
  next->FreqCal[3] =  fc[3];
  next->FreqRes = next->FreqCal[1];
  
  return 1;
}

struct OdinScan *GetAOSscan(AOSBlock *block, int *fcnt)
/*
   Function 'GetAOSscan' extracts AOS data information from an
   array of AOSBlock's. (cf. with 'ExtractHK')
   
   function variables:
   - AOSBlock *block   a pointer to an array of AOSBlocks
   - int *fcnt         a pointer to the number of AOSBlock's in the array
                       will be updated to the number of spectra found

   return value:
   - a pointer to the array of OdinScan structures
*/
{
  struct OdinScan *scan;
  AOSBlock *aos;
  int i, j, n;
  int nAOS, mode, sync, blocks, formats;
  int *index;

  /* Allocate an array of integers to hold the index of those formats
     which contain valid data. */
  blocks = *fcnt;
  index = (int *)calloc(blocks, sizeof(int));
  if (index == NULL)
    ODINerror("memory allocation error");

  /* reset counter of valid formats */
  nAOS = 0;

  /* loop through all formats */
  for (i = 0; i < blocks; i++) {
    /* look for start of sequence:
       sync word should contain 0x73 in upper byte and a block number 
       of zero in the lowest 4 bits */
    sync = block[i].head.UserHead;
    if (sync == AOSUSER) {  /* first block */
      mode = AOSformat(&block[i]);
       /* mode 322 is a submode of 321 and needs to be skipped here. */
      if (mode == 322) continue;
      /* determine the number of formats needed for current mode */
      switch (mode) {
       case 111:
       case 211:
	formats = 16;
	break;
       case 312:
	formats = 32;
	break;
       case 411:
	formats = 8;
	break;
       case 511:
	formats = 3;
	break;
       case 611:
       case 711:
	formats = 5;
	break;
      }
      /* check if all formats are stored in current file */
      if (i + formats <= blocks) {
	index[nAOS] = i;
	nAOS++;
      }
    }
  }

  /* Did we find any data at all ? */
  if (nAOS == 0) {
    ODINwarning("no valid data found");
    free(index);
    *fcnt = 0;
    return (struct OdinScan *)NULL;
  }

  ODINinfo("%d AOS data blocks found", nAOS);

  /* Allocate array of OdinScan structures to hold data */
  scan = (struct OdinScan *)calloc(nAOS, sizeof(struct OdinScan));
  if (scan == NULL) {
    ODINwarning("memory allocation error");
    free(index);
    *fcnt = 0;
    return (struct OdinScan *)NULL;
  }  

  /* Extract data values */
  n = 0;
  for (i = 0; i < nAOS; i++) {
    j = index[i];
    aos = &block[j];
    if (AOSBlock2Scan(&scan[n], aos)) {
      n++;
    }
  }

  free(index);
  *fcnt = n;
  return scan;
}

#define MAXPEAKS 10     /* number of spikes in a frequency comb (AOS) */
#define ORDER     3     /* order of polynomial to fit                 */
#define DIM (ORDER+1)

extern int solve(double **a, double *v, int n); 

double *FitAOSFreq(struct OdinScan *aos)
{
    static double ch[MAXPEAKS], freq[MAXPEAKS];
    static double matrix[DIM][DIM], *array[DIM], vector[DIM], d[DIM];
    double X, Y, maxx;
    int i, j, l, n;

    maxx = 0.0;
    for (i = 0; i < aos->Channels; i++) 
	if (aos->data[i] > maxx) maxx = aos->data[i];

    n = 0;
    for (i = 0; i < aos->Channels; i++) {
	if (aos->data[i] > maxx/5.0) {
	    if ((aos->data[i] > aos->data[i-1]) 
		&& (aos->data[i] >= aos->data[i+1])) {
		freq[n] = 1600.0 + 100.0*n;
		ch[n]   = (double)i-aos->Channels/2;
		/* printf("[%2d]: %4d %8.3f %8.3f\n", n, i, ch[n], freq[n]); */
		n++;
		i += 100;
		if (n == MAXPEAKS) break;
	    }
	}
    }

    for (i = 0; i < DIM; i++) array[i] = &matrix[i][0];
    for (i = 0; i < DIM; i++) {
	vector[i] = 0.0;
	for (j = i; j < DIM; j++) {
	    array[i][j] = 0.0;
	    array[j][i] = 0.0;
	}
    }
  
    for (i = 0; i < MAXPEAKS; i++) {
	X = ch[i];
	Y = freq[i];
	d[0] = 1.0;
	for (j = 1; j < DIM; j++) d[j] = d[j-1]*X;
	for (j = 0; j < DIM; j++) {
	    vector[j] += Y*d[j];
	    for (l = j; l < DIM; l++) {
		array[j][l] += d[j]*d[l];
		array[l][j] = array[j][l];
	    }
	}
    }
  
    /*    for (i = 0; i < DIM; i++) { */
    /*      for (j = 0; j < DIM; j++) { */
    /*        printf("%12.4e ", array[i][j]); */
    /*      } */
    /*      printf("= %12.4e\n", vector[i]); */
    /*    } */

    if (solve(array, vector, DIM) == 0) {
	return ((double *)NULL);
    }

    /* return coefficients in Hz */
    for (i = 0; i < DIM; i++) vector[i] *= 1.0e6;
    return (vector);
}

