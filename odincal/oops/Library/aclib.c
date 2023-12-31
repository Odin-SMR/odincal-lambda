#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/stat.h>

#include "aclib.h"
#include "odinfft.h"
#include "odinlib.h"
#include "odintime.h"
#include "astrolib.h"

static char rcsid[] = "$Id$";

static int rstcnt;

/* 
   Function 'ReadACSLevel0' reads a level 0 file (STW.ACS) containing ACS 
   science data.
   The data type 'Format' is declared in 'tm.h'.
   
   function variables:
   - char *filename    a pointer to the name of the file to be read
   - int *fcnt         pointer to an integer which will receive 
                       the number of formats found
   return value:
   - a pointer to the array of ACSBlock found
*/
ACSBlock *ReadACSLevel0(char *filename, int *fcnt)
{
    static struct stat buf;
    int nBlocks, n, i;
    ACSBlock *acs;
    FILE *tmdata;
  
    /* Reset number of ACSBlock counter */
    *fcnt = 0;

    /* Get file status to retrieve file size */
    if (stat(filename, &buf) == -1) {
	ODINwarning("can't get file status for '%s'", filename);
	return (ACSBlock *)NULL;
    }
  
    /* Is file size a multiple of the size of one ACSBlock ? */
    if (buf.st_size % sizeof(ACSBlock)) {
	ODINwarning("file size error for '%s'", filename);
	return (ACSBlock *)NULL;
    }
  
    /* Calculate number of ACSBlocks */
    nBlocks = buf.st_size/sizeof(ACSBlock);

    /* Allocate required array of ACS blocks */
    acs = (ACSBlock *)calloc(nBlocks, sizeof(ACSBlock));
    if (acs == NULL)
	ODINerror("memory allocation error");

    /* Open the file for reading */
    tmdata = fopen(filename, "r");
    if (tmdata == NULL)
	ODINerror("can't open data file '%s'", filename);
  
    /* Read all ACS blocks */
    n = fread(acs, sizeof(ACSBlock), nBlocks, tmdata);
    if (n != nBlocks) 
	ODINerror("read error");

    for (i = 0; i < nBlocks; i++) {
	if (acs[i].head.SYNC != SYNCWORD) {
	    ODINwarning("wrong sync word");
	    *fcnt = 0;
	    fclose(tmdata);
	    return (ACSBlock *)NULL;
	}
    }

    fclose(tmdata);
    /* Set return values */
    ODINinfo("%d blocks in file '%s'", nBlocks, filename);
    *fcnt = nBlocks;
    rstcnt = STWreset(filename);
    return acs;
}

/* 
   Get the number of samples from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
*/
unsigned long GetSamples(ACSBlock *acs)
{
    unsigned long samples;
    /* unsigned long stw; */
    int ps;

    ps = (int)acs->data[49];            /* prescaler       */
    if (ps < 2 || ps > 6) {
	ODINwarning("prescaler out of range: %d", ps);
	return 0L;
    }
    samples = acs->data[12] & 0x0000ffff;
    /* temporary cludge */
    /* samples |= 0x8000; */
    samples <<= 14-ps;
    return samples;
}

/* 
   Get zero lag for given chip (0-7)
   
   input:
   - ACSBlock *acs  pointer to acs block
   - int chip       number of chip
*/
unsigned long GetZLag(ACSBlock *acs, int chip)
{
    unsigned short msb, lsb;
    unsigned long zlag;
    /* unsigned long stw; */
    int z, block, offset;

    /* calculate the position of zerolag data for given chip */
    z = chip*LAGSPERCHIP;
    block = (z/ACSBLOCKSIZE)+1;
    offset = z%ACSBLOCKSIZE;

    msb = acs[0].data[50+chip];         /* bits 0...15 */
    lsb = acs[block].data[offset];      /* bits 4...19 */
    /* make sure bits 4...15 agree */
    if ((lsb>>4) != (msb & 0x0fff)) {
	ODINwarning("zerolag msb/lsb mismatch %04x:%04x", 
		    (msb & 0x0fff), (lsb>>4));
	/*      return 0L; */
    }
    zlag = (msb<<4)|lsb;

    return zlag;
}

/* 
   Get the commanded integration time from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
*/
float GetCmdIntTime(ACSBlock *acs)
{
    return ((float)(acs->data[35] & 0xff)/16.0);
}

/* 
   Get the prescaler value from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
*/
int GetPrescaler(ACSBlock *acs)
{
    /*    double ct, it, r; */

    /*    ct = (double)GetCmdIntTime(acs); */
    /*    it = (double)GetSamples(acs)/SAMPLEFREQ; */
    /*    r = (log(it)-log(ct))/log(sqrt(2.0)); */

    /*    return ((int)floor(r+0.5)); */
    return ((int)acs->data[49]);
}

/* 
   Get the internal attenuator setting from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
   - int ssb        number 0-3 of SSB module
*/
int GetSSBattenuator(ACSBlock *acs, int ssb, int *err)
{
    int att;
    /* unsigned long stw; */

    att =  (int)acs->data[37+ssb];

    *err = 0;
    if (att <=  95) {
	ODINwarning("SSB module %d at maximum level", ssb);
	*err = 1;
    } else if (att >= 145) {
	/* ODINwarning("SSB module %d at minimum level", ssb); */
	*err = 1;
    }

    return (att);
}

/* 
   Get the SSB frequency setting in Hz from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
   - int ssb        number 0-3 of SSB module
*/
double GetSSBfrequency(ACSBlock *acs, int ssb)
{
    int ssbfreq;
    /* unsigned long stw; */

    ssbfreq = (int)acs->data[44-ssb];
    if ((ssbfreq < 3000) || (ssbfreq > 5000)) {
	ODINwarning("SSB LO %d out of range %d", ssb, ssbfreq);
	return 0.0;
    }
    return (double)ssbfreq*1.0e6;
}

/* 
   Get content of monitor channel from a ACS level 0 block.
   
   input:
   - ACSBlock *acs  pointer to acs block
   - int chip       chip number 0-7
   - int sign       0 or 1, meaning positive or negative, respectively
*/
unsigned long GetMonitor(ACSBlock *acs, int chip, int sign)
{
    unsigned long monitor, zlag;
  
    zlag = GetZLag(acs, chip);
    if (zlag == 0L) return 0L;

    monitor = acs->data[16+2*chip+sign] & 0x0000ffff;
    /* Get the highest nibble from the corresponding zero lag value */
    monitor |= zlag & 0x000f0000;
    if (abs(monitor-zlag) > 0x8000) {
	if (monitor > zlag) monitor -= 0x10000;
	else                monitor += 0x10000;
    }

    return monitor;
}

/* 
   Analyse the correlator mode by calculating a sequence of 16 integers
   whose meaning is a follows:

   n1 ssb1 n2 ssb2 n3 ssb3 ... n8 ssb8

   n1 ... n8 are the numbers of chips that are cascaded to form a band
   ssb1 ... ssb8 are +1 or -1 for USB or SSB, respectively.
   Unused ADCs are represented by zeros.

   examples (the "classical" modes):

   1 band/8 chips  0x00:   8  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0
   2 band/4 chips  0x08:   4  1  0  0  0  0  0  0  4 -1  0  0  0  0  0  0
   4 band/2 chips  0x2A:   2  1  0  0  2  1  0  0  2 -1  0  0  2 -1  0  0
   8 band/1 chips  0x7F:   1  1  1 -1  1  1  1 -1  1 -1  1  1  1 -1  1  1
*/
int *GetACSequence(int mode)
{
    static int seq[16];
    /* the sequence of USB/LSB employed by the correlators */
    static int ssb[8] = { 1, -1, 1, -1, -1, 1, -1, 1 };
    int i, m;

    /* 
       To indicate the new way of storing the mode, 
       they are stored with the ADC_SEQ bit set.
    */
    if (!(mode & ADC_SEQ)) return NULL;

    mode = (mode & 0xff);
    
    m = -1;
    /* reset our sequence */
    for (i = 0; i < 16; i++) seq[i] = 0;

    /* printf("newadc: mode = %02X: ", mode); */
    /*      for (i = 0; i < 8; i++) { */
    /*  	if ((mode >> i) & 1) printf("1"); */
    /*  	else                 printf("0"); */
    /*      } */
    /*      printf("\n"); */
    
    for (i = 0; i < 8; i++) {
	if (mode & 1) m = i;   /* if bit is set, ADC is used */
	seq[2*m]++;            /* count chips                */
	mode >>= 1;            /* move on to next bit        */
    }

    for (i = 0; i < 8; i++) {
	if (seq[2*i]) {
	    if (ssb[i] < 0) seq[2*i+1] = -1;
	    else            seq[2*i+1] =  1;
	}   else            seq[2*i+1] =  0;
    }
    return seq;
}


/* 
   Get the correlator configuration
   
   input:
   - ACSBlock *acs  pointer to acs block
*/
int GetACMode(ACSBlock *acs)
{
    int mode;
    /* unsigned long stw; */
  
    mode = (acs->data[35]>>8) & 0xff;
    /* 
       Each bit represents one ADC from LSB to MSB, 
       the first ADC is assumed to always be on and is not stored. 
       Add the extra bit:
    */
    mode = (mode << 1) | 1;
    return (mode | ADC_SEQ);
}

int GetACOldMode(ACSBlock *acs)
{
    int mode;
    /* unsigned long stw; */
  
    mode = (acs->data[35]>>8) & 0xff;
    /* Old code */
    switch (mode) {
      case 0x7f:
	return AC_LOWRES;
	break;
      case 0x2a:
	return AC_MEDRES;
	break;
      case 0x08:
	return AC_HIRES;
	break;
      case 0x00:
	return AC_XHIRES;
	break;
      case 0x40:
	/* new non-standard mode for high resolution with 7 chips */
	/* ODINwarning("non-standard configuration %02x", mode); */
	return AC_YHIRES;
	break;
      default:
	ODINwarning("unknown configuration %02x", mode);
	return 0;
	break;
    }
}

/* 
   Get the position of the internal calibration mirror

   Note that this info is not normally contained in AC1/AC2 level 0
   files. We use program 'mirror' to copy the two mechanism words from
   the FBA level 0 files to words 9 and 10 of each first block of 
   correlator data. These words are otherwise zero and the mirror position
   would be returned as undefined in that case. That is just what we want.

   input:
   - ACSBlock *acs  pointer to acs block
   output:
   - reference beam position: 
   - 0 = undefined, 1 = sky beam 1, 2 = hot load, 3 = sky beam 2.
*/
int GetRefBeam(ACSBlock *acs)
{
    int beam;
    unsigned short mechanism;

    if ((acs->data[9] == 0) && (acs->data[10] == 0)) return 0;

    mechanism = (unsigned short)acs->data[9];
    if (mechanism == 0xffff) {
	mechanism = (unsigned short)acs->data[10];
	if (mechanism == 0xffff) return 0;
    }

    beam = (mechanism>>13) & 0x0003;
    return beam;
}

/* 
   Get input channel used by correlator
   
   input:
   - ACSBlock *acs  pointer to acs block
*/
int GetACInput(ACSBlock *acs)
{
    int input;
    /* unsigned long stw; */
  
    if (acs->data[36] & 0x02) {
	ODINwarning("simulation mode");
	return 0;
    }
    input = (acs->data[36]>>8) & 0x0f;
    switch (input) {
      case 1: 
	return REC_549;
	break;
      case 2:
	return REC_495;
	break;
      case 3:
	return REC_572;
	break;
      case 4:
	return REC_555;
	break;
      case 5:
	return REC_SPLIT;         /* power combiner */
	break;
      case 6:
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
   Get spectrum type
   
   input:
   - ACSBlock *acs  pointer to acs block

   output:
   returns value ALIGNED if chopper wheel was aligned with optics plate,
   returns value NOTALIGNED if chopper wheel was not aligned and
   returns 0 if chopper wheel position is undefined
*/
int GetACType(ACSBlock *acs)
{
    int type;

    /*    if (acs->data[36] & 0x01) { */
    type = acs->data[8] & 0xffff;
    if (type == 0xaaaa) return NOTALIGNED;
    if (type == 0xbbbb) return ALIGNED;
    /*    }  */
    return 0;
}

/* 
   Get observing mode
   
   input:
   - ACSBlock *acs  pointer to acs block

   output:
   returns 1 for switching
   returns 0 for no-switching
*/
int GetACSwitch(ACSBlock *acs)
{
    return (acs->data[36] & 0x01);
}

/*
   Function 'GetACChannels' extracts ACS channel contents from 
   one set (13) of ACSBlock's.
   
   function variables:
   - float *data       a pointer to an array to receive the data
   - ACSBlock *acs     a pointer to the first format holding the data

   On exit extracted channels are stored in array data.
   They are preceeded by 16 monitor channels.
   
   return value:
   - the number of channels extracted or -1 in case of error
*/
int GetACChannels(float *data, ACSBlock *acs)
{
    int i, k, bad, block, word;
    unsigned long pos, neg;

    k = 0;
    bad = 0;
    for (i = 0; i < MAXCHIPS; i++) {
	pos = GetMonitor(acs, i, POS);
	neg = GetMonitor(acs, i, NEG);
	if ((pos == 0L) || (neg == 0L)) bad++;
	data[k++] = (float)pos;
	data[k++] = (float)neg;
    }
    /* simply return if all bands had zero signal */
    /* ODINwarning("all bands zero"); */
    if (bad == MAXCHIPS) return -1;

    for (block = 1; block < 13; block++) {
	for (word = 0; word < ACSBLOCKSIZE; word++) {
	    data[k++] = (float)acs[block].data[word];
	    if (k == MAXACCHAN+16) return k;
	}
    }
    return -1;
}

/*
  Turn blocks of AC1/AC2 level 0 data into an OdinScan structure.
*/
int ACSBlock2Scan(struct OdinScan *next, ACSBlock *acs)
{
    int i, k, z, att, err, *seq, max;
    int nChannels, backend, bands;
    static float samples;
    unsigned long stw;
    time_t clock;
    struct tm *now;
    double JD, UTC;

    switch (acs->head.UserHead) {
      case 0x7380:
	backend = AC1;
	break;
      case 0x73b0:
	backend = AC2;
	break;
      default:
	ODINwarning("unknown backend");
	return 0;
    }

    stw = LongWord(acs->head.STW);
    nChannels = GetACChannels(next->data, acs);
    if (nChannels == -1) {
	/* We expect the correlators to deliver zeros during times which are 
	   blanked by the ACDC mask word. No need for the following warning. */
	/* ODINwarning("can't extract channels (%08X)", stw); */
	return 0;
    }

    samples = (float)GetSamples(acs);
    if (samples == 0.0) {
	ODINwarning("zero integration time");
	return 0;
    }

    next->Version  = ODINSCANVERSION;
    next->Level    = 0x0000;
    next->Quality  = rstcnt;
    next->Channels = nChannels;
    next->STW      = stw;
    next->Backend  = backend;
    next->Discipline = SIdiscipline(acs->index);

    next->IntMode  = GetACMode(acs);
    if (next->IntMode == AC_YHIRES) {
	k = MAXCHIPS*2+LAGSPERCHIP*7;
	/* printf("set last band to zero (%d)\n", k); */
	/* clear last band */
	for (i = 0; i < LAGSPERCHIP; i++) next->data[k++] = 0.0;
    }

    /*    if (!next->IntMode) { */
    /*      ODINwarning("invalid integration mode"); */
    /*      return 0; */
    /*    } */
    next->Frontend = GetACInput(acs);

    /* 
       The following code assumes AC1 hooked up to 495/549 receiver
       and  AC2 hooked up to 555/572 receiver.
    */
    next->Type = 0;
    switch (GetACType(acs)) {
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
	  case REC_SPLIT:
	    if (next->Backend == AC1) next->Type = REF;
	    else                      next->Type = SIG;
	    break;
	  case REC_119:
	    next->Type = SIG;
	    break;
	  default:
	    next->Type = UNDEFINED;
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
	  case REC_SPLIT:
	    if (next->Backend == AC1) next->Type = SIG;
	    else                      next->Type = REF;
	    break;
	  case REC_119:
	    next->Type = REF;
	    break;
	  default:
	    next->Type = UNDEFINED;
	    break;
	}
	break;
    }
    if (next->Type == UNDEFINED) {
	ODINwarning("undefined phase");
	return 0;
    }
    if (next->Type == REF) {
	switch (GetRefBeam(acs)) {
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
	    /* ODINwarning("undefined position of calibration mirror"); */
	    next->Quality |= ECALMIRROR;
	    break;
	}
    }

    next->ObsMode = (GetACSwitch(acs) ? SSW : TPW);
    next->IntTime = samples/SAMPLEFREQ;

    if (next->IntMode & ADC_SEQ) {
	next->FreqCal[0] = GetSSBfrequency(acs, 0);
	next->FreqCal[1] = GetSSBfrequency(acs, 1);
	next->FreqCal[2] = GetSSBfrequency(acs, 2);
	next->FreqCal[3] = GetSSBfrequency(acs, 3);
    } else {
	/* old version:
	   Note, that we store the SSB frequencies in the order
	   in which subbands are returned by the correlator: 0, 2, 1, 3 */
	next->FreqCal[0] = GetSSBfrequency(acs, 0);
	next->FreqCal[1] = GetSSBfrequency(acs, 2);
	next->FreqCal[2] = GetSSBfrequency(acs, 1);
	next->FreqCal[3] = GetSSBfrequency(acs, 3);
    }

    for (i = 0; i < 4; i++) {
	if (next->FreqCal[i] == 0.0) {
	    ODINwarning("failed to retrieve SSB [%d] frequency", i);
	    return 0;
	}
    }

    /* set number of bands and check attenuator levels */
    if (next->IntMode & ADC_SEQ) {
	seq = GetACSequence(next->IntMode);
	bands = 0;
	for (i = 0; i < 8; i++) {
	    if (seq[2*i]) bands++;
	    if (i%2) continue;
	    if ((att = GetSSBattenuator(acs, i/2, &err)) == 0) return 0;
	    if (err) next->Quality |= ESIGLEVEL;
	}
/*  	printf("newadc: number of bands: %d\n", bands); */
/*  	printf("newadc: signal level: %d\n", (next->Quality & ESIGLEVEL ? 1 : 0)); */
    } else {
	switch (next->IntMode) {
	  case AC_XHIRES:
	  case AC_YHIRES:
	    if ((att = GetSSBattenuator(acs, 0, &err)) == 0) return 0;
	    if (err) next->Quality |= ESIGLEVEL;
	    bands = 1;
	    break;
	  case AC_HIRES:
	    for (k = 0; k < 2; k += 2) {
		if ((att = GetSSBattenuator(acs, k, &err)) == 0) return 0;
		if (err) next->Quality |= ESIGLEVEL;
	    }
	    bands = 2;
	    break;
	  case AC_MEDRES:
	    for (k = 0; k < 4; k++) {
		if ((att = GetSSBattenuator(acs, k, &err)) == 0) return 0;
		if (err) next->Quality |= ESIGLEVEL;
	    }
	    bands = 4;
	    break;
	  case AC_LOWRES:
	    for (k = 0; k < 4; k++) {
		if ((att = GetSSBattenuator(acs, k, &err)) == 0) return 0;
		if (err) next->Quality |= ESIGLEVEL;
	    }
	    bands = 8;
	    break;
	  default:
	    bands = 1;
	    break;
	}
    }
    if (next->IntMode & ADC_SEQ) {
	z = MAXCHIPS*2;
	for (k = 0; k < 8; k++) {
	    if (seq[2*k]) {
		next->data[z] = (float)GetZLag(acs, k);
/*    		printf("newadc: get zlag %d: %d %d %f %f %f\n", */
/*    		       k, z, z-MAXCHIPS*2, next->data[z], next->data[z+1], next->data[z+2]); */
		if (next->data[z+2] > 0.0) next->data[z+2] -= 65536.0;
		z += seq[2*k]*LAGSPERCHIP;
	    }
	}
    } else {
	for (k = 0; k < bands; k++) {
	    z = MAXCHIPS*2 + k*MAXACCHAN/bands;
	    next->data[z] = (float)GetZLag(acs, k*MAXCHIPS/bands);
	    /* potential lag[2] underflow */
	    if (next->data[z+2] > 0.0) {
		/* ODINwarning("potential lag[2] underflow"); */
		next->data[z+2] -= 65536.0;
	    }
	}

	if (next->IntMode == AC_YHIRES) {
	    z = MAXCHIPS*2;
	    next->data[z] = (float)GetZLag(acs, 7);
	}
    }

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

    if (next->IntMode & ADC_SEQ) {
	max = 0;
	for (i = 0; i < 8; i++) {
	    if (seq[2*i] > max) max = seq[2*i];
	}
/*  	printf("newadc: max no. chips: %d\n", max); */
	if (max) next->FreqRes = 1.0e6/(double)max;
/*  	printf("newadc: freq.res: %6.3f\n", next->FreqRes/1.0e6); */
    } else {
	switch (next->IntMode) {
	  case AC_LOWRES:   next->FreqRes = 1.000e6; break;
	  case AC_MEDRES:   next->FreqRes = 0.500e6; break;
	  case AC_HIRES:    next->FreqRes = 0.250e6; break;
	  case AC_XHIRES:   next->FreqRes = 0.125e6; break;
	  case AC_YHIRES:   next->FreqRes = 0.125e6; break;
	}
    }

    return 1;
}

/*
   Function 'GetACSscan' extracts ACS data information from an
   array of ACSBlock's.
   
   function variables:
   - ACSBlock *block   a pointer to an array of ACSBlocks
   - int *fcnt         a pointer to the number of ACSBlock's in the array
                       will be updated to the number of spectra found

   return value:
   - a pointer to the array of OdinScan structures
*/
struct OdinScan *GetACSscan(ACSBlock *block, int *fcnt)
{
    struct OdinScan *scan;
    ACSBlock *acs;
    int i, j, n;
    int nACS, sync, blocks, formats, backend;
    int *index;

    /* Allocate an array of integers to hold the index of those formats
       which contain valid data. */
    blocks = *fcnt;
    index = (int *)calloc(blocks, sizeof(int));
    if (index == NULL) {
	ODINwarning("memory allocation error");
	return (struct OdinScan *)NULL;
    }  

    /* reset counter of valid formats */
    backend = 0;
    nACS = 0;
    /* number of formats for one complete spectrum */
    formats = 13;
    /* loop through all formats */
    for (i = 0; i < blocks; i++) {
	/* look for start of sequence:
	   sync word should contain 0x73 in upper byte and a block number 
	   of zero in the lowest 4 bits */
	sync = block[i].head.UserHead;
	/* if (sync == ACSUSER) { */ /* first block */
	if ((sync & 0xff0f) == 0x7300) { /* first block */
	    /* check if all formats are stored in current file */
	    switch (sync) {
	      case 0x7380:
		backend = AC1;
		break;
	      case 0x73b0:
		backend = AC2;
		break;
	      default:
		ODINwarning("unknown backend");
		continue;
		break;
	    }

	    if (i + formats <= blocks) {
		index[nACS] = i;
		nACS++;
	    }
	}
    }

    /* Did we find any data at all ? */
    if (nACS == 0) {
	ODINwarning("no valid data found");
	free(index);
	*fcnt = 0;
	return (struct OdinScan *)NULL;
    }

    ODINinfo("%d ACS data blocks found", nACS);

    /* Allocate array of OdinScan structures to hold data */
    scan = (struct OdinScan *)calloc(nACS, sizeof(struct OdinScan));
    if (scan == NULL) {
	ODINwarning("memory allocation error");
	free(index);
	return (struct OdinScan *)NULL;
    }  

    /* Extract data values */
    n = 0;
    for (i = 0; i < nACS; i++) {
	j = index[i];
	acs = &block[j];
	if (ACSBlock2Scan(&scan[n], acs)) {
	    n++;
	}
    }

    free(index);
    *fcnt = n;
    return scan;
}

/*
 * Approximation for Inverse Complementary Error Function (taken from
 * D'Addario et.al. 1984, Radio Science, vol 19, 3, p.931) 
 */
double inv_erfc(double z)
{
    static double p[3] = { 1.591863138, -2.442326820, 0.37153461 };
    static double q[3] = { 1.467751692, -3.013136362, 1.00000000 };
    double x, y;
 
    x = 1.0-z;
    y = (x*x-0.5625);
    y = x*(p[0]+(p[1]+p[2]*y)*y)/(q[0]+(q[1]+q[2]*y)*y);
    return (y);
}
 
/* 
 * Determine input noise power from zero lag.
 */
double ZeroLag(double zlag, double v)
{
    double x;
 
    if (zlag >= 1.0 || zlag <= 0.0) return(0.0);
    x = v/inv_erfc(zlag);
    return (x*x/2.0);
}

/*
 * Remove any bias in the tail of the correlation lags
 */
void Equalize(double data[], int n)
{
    int i;
    double mean;
 
    mean = 0.0;
    for (i = n-1; i >= n/2; i--)  mean += data[i];
    mean /= (double)n/2.0;
    for (i = 0; i < n; i++)  data[i] -= mean;
}

/* 
 * Reintroduce power into filter shapes.
 */
void PowerScale(double power, double data[], int n)
{
    int i;
 
    for (i = 0; i < n; i++)  data[i] *= power;
}

/* 
 * Reverse sequence of channels
 */
static void reverse(double data[], int n)
{
    int i, j;
    double swap;
 
    for (i = 0, j = n-1; i < n/2; i++, j--) {
	swap    = data[i];
	data[i] = data[j];
	data[j] = swap;
    }
}

/* 
 * Complementary error function.
 */
double erfc(double x)
{
    double z, t;
 
    z = fabs(x);
    t = 1.0/(1.0+z/2.0);
    z = t*exp(-z*z-1.26551223+t*(1.00002368+t*(0.37409196+
    t*(0.09678418+t*(-0.18628806+t*(0.27886807+t*(-1.13520398+
    t*(1.48851587+t*(-0.82215223+t*0.17087277)))))))));
    return (x > 0.0 ? z : 2.0-z);
}

/* 
 * Evaluation of the integrand for the Odin 3 level correlation.
 */
double o3lcc(double a, double c, double x)
{
    double x1, x2, z;
 
    x1 = 1.0+x;
    x2 = 1.0-x*x;
    z = (exp(-c*c/x1)+exp(-a*a/x1)+2.0*exp((2.0*c*a*x-a*a-c*c)/(2.0*x2)))
	/sqrt(x2);
    return (z);
}
 
/* 
 * Integrate over 'o3lcc' function using trapezoidal rule.
 * Resulting table is stored in array 'y'. 
 */
void Trapez(double t1, double t2, double x[], double y[], int n)
{
    double delta, x0, y0, y1, Ez, v1, v2;
    int i;
 
    delta = 1.0/n;

    Ez = 2.0*PI;
    v1 = erf(t1/sqrt(2.0));
    v2 = erf(t2/sqrt(2.0));
    x[0] = y[0] = 0.0;
    x0 = 0.0;
    y0 = o3lcc(t1, -t2, x0)/Ez;
    for (i = 1; i < n; i++) {
        x0 += delta;
        x[i] = x0;
        y1 = o3lcc(t1, -t2, x0)/Ez;
        y[i] = y[i-1]+0.5*(y0+y1)*delta;
        y0 = y1;
    }
    x[n] = 1.0;
    y[n] = 1.0-0.5*v1-0.5*v2;
}

/* 
 * Find true autocorrelation value for given quantized autocorrelation
 * by backward linear interpolation in precalculated table 'y'.
 */
double Lookup(double f, double x[], double y[])
{
    double p;
    int index;

    index = 1;
    while (y[index] < f) index++;
    p = (f-y[index-1])/(y[index]-y[index-1]);
    return (x[index-1]+p*(x[index]-x[index-1]));
}

#define MAXTABLE 20
static double rho[MAXTABLE+1], cc[MAXTABLE+1];
 
int QCorrect1(double thr[], double f[], int n)
{
    int i;
    double corrected, fa;

    Trapez(thr[0], thr[1], rho, cc, MAXTABLE);
    f[0] = 1.0;
    for (i = 1; i < n; i++) {
	fa = fabs(f[i]);
	corrected = Lookup(fa, rho, cc);
	if (f[i] < 0.0) f[i] = -corrected;
	else            f[i] =  corrected;
    }
    return 1;
}

/*
  Calculate normalised threshold values based on the value of
  a (normalised) monitor channel.
*/
double Threshold(double monitor)
{
    double thr;

    if (monitor < 0.0 || monitor > 1.0) return(0.0);
    thr = sqrt(2.0)*inv_erfc(2.0*monitor);

    return(thr);
}

/*
  Perform quantisation correction using Kulkarni & Heiles approximation.
  (taken from Kulkarni, S.R., Heiles, C., 1980, AJ, 85, 1413.
*/
int QCorrect(double c, double f[], int n)
{
    int i;
    double fa, A, B;
 
    A = (PI/2.0)*exp(c*c);
    B = -A*A*A*(pow((c*c-1), 2.0)/6.0);

    f[0] = 1.0;
    for (i = 1; i < n; i++) {
	fa = f[i];
	if (fabs(fa) > 0.86) {
	    ODINwarning("level too high in QCorrect");
	    return 0;
	}
	f[i] = (A + B*fa*fa)*fa;
    }
    return 1;
}

/*
  Normalise raw data following Omnisys prescription

  Note, that we expect the monitor values in the first 16 channels
  followed by n data values.
*/
int Normalise(struct OdinScan *s)
{
    int k;
    float samples;
 
    samples = s->IntTime*SAMPLEFREQ;

    /* normalise monitor channels */
    for (k = 0; k < MAXCHIPS*2; k++) {
	s->data[k] *= 1024.0*(SAMPLEFREQ/samples)/(CLOCKFREQ/2.0);
    }
  
    /* normalise data channels */
    for (; k < s->Channels; k++) {
	s->data[k] *= 2048.0*(SAMPLEFREQ/samples)/(CLOCKFREQ/2.0);
    }
    return 1;
}
  
/*
  Reduce one band.

  input:
  - double data[]      the data channels belonging to this band
  - int n              number of channels
  - double monitor[]   two monitor channels belonging to this band

  output:
  - 1                  success
  - 0                  failure
*/
int Reduce1Band(double data[], int n, double monitor[])
{
    double zlag, c[2], cmean, dc, power;
  
    if (data == (double *)NULL) {
	ODINwarning("invalid band");
	return 0;
    }

    zlag = data[0];
    if (zlag == 0.0) {
	ODINwarning("missing band");
	return 0;
    }
    if (zlag < 0.0) {
	ODINwarning("this is not the start of a band");
	return 0;
    }
    if (zlag > 1.0) {
	ODINwarning("data out of range (%.4f)", zlag);
	return 0;
    }
   
    power = ZeroLag(zlag, 1.0);
    c[POS] = Threshold(monitor[POS]);
    c[NEG] = Threshold(monitor[NEG]);
    if (c[POS] == 0.0 || c[NEG] == 0.0) {
	ODINwarning("thresholds out of range (%.4f,%.4f)", c[POS], c[NEG]);
	/* cmean = Threshold(zlag/2.0); */
	return 0;
    } else {
	cmean = (c[POS]+c[NEG])/2.0;
	dc = fabs((c[POS]-c[NEG])/2.0/cmean);
	if (dc > 0.1) {
	    ODINwarning("asymmetric thresholds (%.4f,%.4f)", c[POS], c[NEG]);
	    /* cmean = Threshold(zlag/2.0); */
	    return 0;
	}
    }

    /* Equalize(data, n); */
    if (QCorrect(cmean, data, n) == 0) {
	ODINwarning("quantisation correction failed");
	return 0;
    }
    /* QCorrect1(c, data, n); */
    odinfft(data, n);
    PowerScale(power, data, n);

    /* indicate success */
    return 1;
}

#define MAXACMHZ   (MAXACCHAN*112/96)

/*
  Reduce one autocorrelator scan.

  input:
  - struct OdinScan *s       pointer to the scan to be reduced

  output:
  - 1                  success
  - 0                  failure
*/
int ReduceAC(struct OdinScan *s)
{
    static double data[MAXCHANNELS];
    static double monitor[MAXCHIPS][LEVELS];
    float *bandhead;
    int nchan, mode, index, got, *seq;
    int i, j, k, band, nbands, nraw, nred, adc;

    int sort[4][8] = {
	{  1,  0,  0,  0,  0,  0,  0,  0 },
	{  1, -2,  0,  0,  0,  0,  0,  0 },
	{  2,  4, -1, -3,  0,  0,  0,  0 },
	{  2, -1,  6, -5, -3,  4, -7,  8 }
    };
    

    /* 
       For reducing the data, we first extract the 8*2 monitor channels
       which are stored as the first 16 channels, and move all data channels
       downwards correspondingly.
    */
    for (k = 0; k < MAXCHIPS; k++) {
	monitor[k][POS] = (double)s->data[2*k];
	monitor[k][NEG] = (double)s->data[2*k+1];
    }

    nchan = s->Channels-2*MAXCHIPS; 
    for (j = 0, k = 2*MAXCHIPS; j < nchan; j++, k++) { 
	s->data[j] = s->data[k]; 
    } 

    /* now perform the reduction depending on mode */
    if (s->IntMode & ADC_SEQ) {
/*  	printf("newadc: new version of ReduceAC\n"); */
	seq = GetACSequence(s->IntMode);
	nbands = 0;
	for (i = 0; i < 8; i++) {
	    if (seq[2*i]) nbands++;
	}
/*    	printf("newadc: number of bands: %d\n", nbands); */

	/* 
	   copy data to temporary array (of type 'double') 
	   append zeros to get multiples of 112 channels, which is the 
	   size required by our fft7 routine. 
	*/
	k = 0;
	for (adc = 0; adc < 8; adc++) {
	    if (seq[2*adc]) {
		nraw = seq[2*adc]*LAGSPERCHIP;
		nred = seq[2*adc]*112;
/*    		printf("newadc: band length %d %d:%d\n", adc, nraw, nred); */
		for (j = 0; j < nraw; j++) 
		    data[k++] = (double)s->data[adc*LAGSPERCHIP+j]; 
		for (     ; j < nred; j++) 
		    data[k++] = 0.0;
	    }
	} 

	/* Reduce each band. */
	k = 0;
	for (adc = 0; adc < 8; adc++) {
	    if (seq[2*adc]) {
		nred = seq[2*adc]*112;
/*      		printf("newadc: reduce band @ %d, length %d [%d:%d]\n", adc, nred, k, k+nred); */
		got = Reduce1Band(&data[k], nred, monitor[adc]);
		if (got) {
		    /* if (seq[2*adc+1] < 0) printf("newadc: reverse %d\n", adc); */
		    /* if (seq[2*adc+1] < 0) reverse(&data[k], nred);     */
		    for (j = k; j < k+nred; j++) s->data[j] = data[j];
		} else {
		    for (j = k; j < k+nred; j++) s->data[j] = 0.0;
/*  		    printf("newadc: reducing band @ %d failed\n", adc); */
		    s->Quality |= ESIGLEVEL;
		}
		k += nred;
	    }
	}
/*    	printf("newadc: total number of channels: %d\n", k); */
	s->Channels = k; 
    } else {
	switch (s->IntMode) {
	  case AC_XHIRES:
	  case AC_YHIRES:
	    nbands = 1;
	    mode = 0;
	    break;

	  case AC_HIRES:
	    nbands = 2;
	    mode = 1;
	    break;

	  case AC_MEDRES:
	    nbands = 4;
	    mode = 2;
	    break;

	  case AC_LOWRES:
	    nbands = 8;
	    mode = 3;
	    break;

	  default:
	    ODINwarning("unknown mode %d\n", s->IntMode);
	    return 0;
	    break;
	}

	nraw = MAXACCHAN/nbands;
	nred = MAXACMHZ/nbands;

	/* 
	   copy data to temporary array (of type 'double') 
	   append zeros to get multiples of 112 channels, which is the 
	   size required by our fft7 routine. 
	*/
	k = 0;
	for (band = 0; band < nbands; band++) { 
	    for (j = 0; j < nraw; j++) data[k++] = (double)s->data[band*nraw+j]; 
	    for (     ; j < nred; j++) data[k++] = 0.0; 
	} 

	k = 0;
	for (band = 0; band < nbands; band++) { 
	    if (s->IntMode == AC_YHIRES) {
		got = Reduce1Band(&data[band*nred], nred, 
				  monitor[7]);
	    } else {
		got = Reduce1Band(&data[band*nred], nred, 
				  monitor[band*(MAXCHIPS/nbands)]);
	    }
	    if (sort[mode][band] < 0) {
		reverse(&data[band*nred], nred); 
	    }

	    /* copy reduced data back to scan structure into right slot */
	    index = abs(sort[mode][band])-1;
	    bandhead = &(s->data[index*nred]);
	    if (got) {
		for (j = 0; j < nred; j++) {
		    bandhead[j] = data[k];
		    k++;
		}
	    } else {
		for (j = 0; j < nred; j++) {
		    bandhead[j] = 0.0;
		    k++;
		}
		s->Quality |= ESIGLEVEL;
	    }
	}
	s->Channels = nred*nbands; 
    }

    return 1;
}
