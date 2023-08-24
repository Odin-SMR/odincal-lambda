#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>

#include "level0.h"
#include "odinlib.h"
#include "shklib.h"

#define PLLFREQS
#define CALLOAD
#include "smrcdb.h"

#define MHZ 1.0e6

static HKBlock *shk;
static int nBlocks;

static char rcsid[] = "$Id$";

/* 
   Function 'OpenHKLevel0' reads a level 0 file (STW.SHK) of sampled 
   house keeping data.
   
   function variables:
   - char *filename    a pointer to the name of the file to be read

   return value:
   - the number of HKBlock found
*/
int OpenHKLevel0(char *filename)
{
  static struct stat buf;
  int n, i;
  FILE *tmdata;
  
  /* Reset number of HKBlock counter */
  nBlocks = 0;

  /* Get file status to retrieve file size */
  if (stat(filename, &buf) == -1) return -1;

  /* Is file size a multiple of the size of one HKBlock ? */
  if (buf.st_size % sizeof(HKBlock)) return -1;
  
  /* Calculate number of HKBlocks */
  nBlocks = buf.st_size/sizeof(HKBlock);

  /* Allocate required array of HK blocks */
  if (shk) free(shk);
  shk = NULL;
  shk = (HKBlock *)calloc(nBlocks, sizeof(HKBlock));
  if (shk == NULL) {
    ODINwarning("memory allocation error");
    nBlocks = 0;
    return -1;
  } 

  /* Open the file for reading */
  tmdata = fopen(filename, "r");
  if (tmdata == NULL) {
    ODINwarning("can't open data file '%s'", filename);
    CloseHKLevel0();
    return -1;
  }
  
  /* Read all HK blocks */
  n = fread(shk, sizeof(HKBlock), nBlocks, tmdata);
  if (n != nBlocks) {
    ODINwarning("read error");
    CloseHKLevel0();
    return -1;
  }
  for (i = 0; i < nBlocks; i++) {
    if (shk[i].head.SYNC != SYNCWORD) {
      ODINwarning("wrong sync word");
      CloseHKLevel0();
      return -1;
    }
  }
  fclose(tmdata);

  /* Set return values */
  /* ODINinfo("%d blocks in file '%s'", nBlocks, filename); */
  return nBlocks;
}

void CloseHKLevel0()
{
  if (shk) free(shk);
  shk = NULL;
  nBlocks = 0;
}

/*
  Get the STW from house keeping block.
*/
unsigned long shkSTW(int i)
{
  if (i < 0) i += nBlocks;

  if (i < 0)         return 0L;
  if (i > nBlocks-1) return 0L;

  return LongWord(shk[i].head.STW);
}

/* 
   Retrieve all values of specific word of house keeping data.
*/
int shkWord(int index, int sub, unsigned long *STW, 
	    unsigned short *word, unsigned short bad)
{
  int i, n;
  static unsigned short hkword, hkindex;

  n = 0;
  for (i = 0; i < nBlocks; i++) {
    hkword = shk[i].data[index];
    if (hkword == 0xffff) continue;
    if (hkword == bad)    continue;
    if ((sub > -1) && (sub < 16)) {
      hkindex = hkword & 0x000f;
      hkword  >>= 4;
      if (hkindex == sub) {
	STW[n] = shkSTW(i);
	word[n] = hkword;
	n++;
      }
    } else {
      STW[n] = shkSTW(i);
      word[n] = hkword;
      n++;
    }
  }
  return n;
}

int shkArray(int index, int sub, double (*convert)(unsigned short), 
	     unsigned short bad, unsigned long *STW, double *array)
{
  int n, i;

  unsigned short *w = calloc(nBlocks, sizeof(unsigned short));
  n = shkWord(index, sub, STW, w, bad);
  for (i = 0; i < n; i++) {
    array[i] = convert(w[i]);
  }
  free(w);
  return n;
}

int mechArray(int index, int sub, unsigned long *STW, double *array)
{
  int n, i;
  static unsigned short hkword, hkindex;

  n = 0;
  for (i = 0; i < nBlocks; i++) {
    hkword = shk[i].data[index];
    if (hkword == 0xffff) {
      /* if mechanism A produced 0xffff, try mechanism B */
      hkword = shk[i].data[index+MECH_A_HK];
      /* if mechanism B produced 0xffff, forget it */
      if (hkword == 0xffff) continue;
    }

    if ((sub > -1) && (sub < 16)) {
      hkindex = hkword & 0x000f;
      hkword  >>= 4;
      if (hkindex == sub) {
	STW[n] = shkSTW(i);
	array[n] = (double)hkword;
	n++;
      }
    } else {
      STW[n] = shkSTW(i);
      array[n] = (double)hkword;
      n++;
    }
  }

  return n;
}

static int locate(unsigned long STW0, int n, 
		  unsigned long STW[], double array[])
{
  int last;

  if (STW0 < STW[0]) return 0;
  last = 1;
  while ((last < n) && (STW0 > STW[last])) {
    last++;
  }
  return last;
}

double LookupHK(unsigned long STW0, int n, 
	      unsigned long STW[], double array[])
{
  int i;

  i = locate(STW0, n, STW, array);
  if (i == 0) return 0.0;

  return array[i-1];
}

double Interp1HK(unsigned long STW0, int n, 
		   unsigned long STW[], double array[])
{
  int i;
  double dt;

  i = locate(STW0, n, STW, array); 
  if (i == 0) return 0.0;
  if (i == n) return array[n-1];

  dt = (double)(STW0-STW[i-1]);
  return array[i-1]+(array[i]-array[i-1])/(STW[i]-STW[i-1])*dt;
}

double HK2Etrip(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return -1.2195*x;
}

double HK2Edoub(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return -1.6129*x;
}

double HK2Ehmix(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 1.2195*x;
}

double HK2Egunn(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 80.0*x;
}

double HK2Evar(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 4.8*x-12.0;
}

double HK2Emix(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return x/1.22;
}

double HK2Ehemt(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 2.0*x-5.0;
}

double HK2Ewarm(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 20.0*(x-1.16);
}

double HK2Ecold(unsigned short reading)
{
  double x;
  x = (double)reading;
  x *= 5.0/4095.0;

  return 70.0*(x-3.86)+273.15;
}

double HK2Ehro(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (x+4000.0)*MHZ;
}

double HK2Epro(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (x/32.0+100.0)*MHZ;
}

double HK2Edro(unsigned short reading)
{
  double x;
  x = (double)reading;

  x = -5.01 + 10.0*x/4095.0;
  return (69.8979 -(53.28-(14.764-1.8817*x)*x)*x);
}

/* conversion function for AOS laser temperature */
double HK2Eaos1(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (0.01141*x+7.3);
}

/* conversion function for AOS laser current */
double HK2Eaos2(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (0.0388*x);
}

/* conversion function for AOS structure temperature */
double HK2Eaos3(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (0.01167*x+0.764);
}

/* conversion function for AOS continuum power */
double HK2Eaos4(unsigned short reading)
{
  double x;
  x = (double)reading;

  return ((5.7718e-6*x+6.929e-3)*x-2.1);
}

/* conversion function for AOS IF processor temperature */
double HK2Eaos5(unsigned short reading)
{
  double x;
  x = (double)reading;

  return (0.01637*x-6.54);
}

int Varactor(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(17, 0, HK2Evar, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(18, 0, HK2Evar, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(25, 0, HK2Evar, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(26, 0, HK2Evar, 0xfeaf, STW, array);
  return 0;
}

int Gunn(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(17, 1, HK2Egunn, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(18, 1, HK2Egunn, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(25, 1, HK2Egunn, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(26, 1, HK2Egunn, 0xfeaf, STW, array);
  return 0;
}

int HMixer(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(17, 2, HK2Ehmix, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(18, 2, HK2Ehmix, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(25, 2, HK2Ehmix, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(26, 2, HK2Ehmix, 0xfeaf, STW, array);
  return 0;
}

int Doubler(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(17, 3, HK2Edoub, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(18, 3, HK2Edoub, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(25, 3, HK2Edoub, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(26, 3, HK2Edoub, 0xfeaf, STW, array);
  return 0;
}

int Tripler(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(17, 4, HK2Etrip, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(18, 4, HK2Etrip, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(25, 4, HK2Etrip, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(26, 4, HK2Etrip, 0xfeaf, STW, array);
  return 0;
}

int MixerC(int rx,  unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(19, 0, HK2Emix, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(19, 1, HK2Emix, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(27, 0, HK2Emix, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(27, 1, HK2Emix, 0xfeaf, STW, array);
  return 0;
}

int HemtBias(int rx, int hemt, unsigned long *STW, double *array)
{
  if (hemt == 1) {
    if (rx == REC_495) return shkArray(19, 2, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_549) return shkArray(19, 4, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_572) return shkArray(27, 2, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_555) return shkArray(27, 4, HK2Emix, 0xfeaf, STW, array);
  }
  if (hemt == 2) {
    if (rx == REC_495) return shkArray(19, 3, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_549) return shkArray(19, 5, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_572) return shkArray(27, 3, HK2Emix, 0xfeaf, STW, array);
    if (rx == REC_555) return shkArray(27, 5, HK2Emix, 0xfeaf, STW, array);
  }
  return 0;
}

int warmIF(char side, unsigned long *STW, double *array)
{
  if (side == 'A') return shkArray(20, 0, HK2Ewarm, 0xfeaf, STW, array);
  if (side == 'B') return shkArray(28, 0, HK2Ewarm, 0xfeaf, STW, array);
  return 0;
}

int CLoad(char side, unsigned long *STW, double *array)
{
  int n, i;

  if (side == 'A') {
    n = shkArray(20, 1, HK2Ewarm, 0xfeaf, STW, array);
    for (i = 0; i < n; i++) array[i] -= dtcal[0];
    return n;
  }
  if (side == 'B') {
    n = shkArray(28, 1, HK2Ewarm, 0xfeaf, STW, array);
    for (i = 0; i < n; i++) array[i] -= dtcal[1];
    return n;
  }
  return 0;
}

int ILoad(char side, unsigned long *STW, double *array)
{
  if (side == 'A') return shkArray(20, 2, HK2Ewarm, 0xfeaf, STW, array);
  if (side == 'B') return shkArray(28, 2, HK2Ewarm, 0xfeaf, STW, array);
  return 0;
}

int MixerT(char side, unsigned long *STW, double *array)
{
  if (side == 'A') return shkArray(20, 3, HK2Ecold, 0xfeaf, STW, array);
  if (side == 'B') return shkArray(28, 3, HK2Ecold, 0xfeaf, STW, array);
  return 0;
}

int LNA(char side, unsigned long *STW, double *array)
{
  if (side == 'A') return shkArray(20, 4, HK2Ecold, 0xfeaf, STW, array);
  if (side == 'B') return shkArray(28, 4, HK2Ecold, 0xfeaf, STW, array);
  return 0;
}

int Mix119(char side, unsigned long *STW, double *array)
{
  if (side == 'A') return shkArray(20, 5, HK2Ecold, 0xfeaf, STW, array);
  if (side == 'B') return shkArray(28, 5, HK2Ecold, 0xfeaf, STW, array);
  return 0;
}

int HROfreq(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(21, 0, HK2Ehro, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(21, 2, HK2Ehro, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(29, 0, HK2Ehro, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(29, 2, HK2Ehro, 0xfeaf, STW, array);
  return 0;
}

int PROfreq(int rx, unsigned long *STW, double *array)
{
  if (rx == REC_495) return shkArray(21, 1, HK2Epro, 0xfeaf, STW, array);
  if (rx == REC_549) return shkArray(21, 3, HK2Epro, 0xfeaf, STW, array);
  if (rx == REC_572) return shkArray(29, 1, HK2Epro, 0xfeaf, STW, array);
  if (rx == REC_555) return shkArray(29, 3, HK2Epro, 0xfeaf, STW, array);
  return 0;
}

int LOfreq(int rx, unsigned long *STW, double *array)
{
  double hro, pro, m, f;
  int n, i, index, sub;
  unsigned short hkword[2], hkindex[2];

  switch (rx) {
   case REC_495:
    index = 21;
    sub   =  0;
    m = 17.0;
    break;
   case REC_549:
    index = 21;
    sub   =  2;
    m = 19.0;
    break;
   case REC_555:
    index = 29;
    sub   =  2;
    m = 19.0;
    break;
   case REC_572:
    index = 29;
    sub   =  0;
    m = 20.0;
    break;
   default:
    ODINwarning("invalid receiver %d", rx);
    return 0;
  }
  n = 0;
  for (i = 0; i < nBlocks-1; i++) {
    hkword[0] = shk[i].data[index];
    hkword[1] = shk[i+1].data[index];

    hkindex[0] = hkword[0] & 0x000f;
    if (hkindex[0] != sub) continue;
    hkindex[1] = hkword[1] & 0x000f;
    if (hkindex[1] != sub+1) continue;

    hkword[0] >>= 4;
    hkword[1] >>= 4;

    STW[n] = shkSTW(i);
    hro = ((double)hkword[0]+4000.0)*MHZ;
    pro = ((double)hkword[1]/32.0+100.0)*MHZ;

    f = (hro*m+pro)*6.0;
    array[n] = f;
    n++;
  }
  return n;
}

int TuneMechanism(int rx, unsigned long *STW, double *array)
{
  int index, sub;
  
  switch (rx) {
   case REC_495:
    index = 35; 
    sub   =  0;
    break;
   case REC_549:
    index = 36;
    sub   =  0;
    break;
   case REC_555:
    index = 36;
    sub   =  2;
    break;
   case REC_572:
    index = 35;
    sub   =  2;
    break;
   default:
    ODINwarning("invalid receiver %d", rx);
    return 0;
  }

  return mechArray(index, sub, STW, array);
}

double LOdrift(char side, double T)
{
  double c;

  c = 1.0;
  if (side == 'A') c = 1.0+(pllc0[0]+pllc1[0]*T)*1.0e-6;
  if (side == 'B') c = 1.0+(pllc0[1]+pllc1[1]*T)*1.0e-6;
  return c;
}

double IFreq(int rx, double LOfreq, double mech)
{
  double C1, C2, sf, s3900, d;
  int i;

  switch (rx) {
   case REC_495:
    C1 = 61600.36;
    C2 = 104188.89;
    sf = 0.0002977862;
    break;
   case REC_549:
    C1 = 57901.86;
    C2 = 109682.58;
    sf = 0.0003117128;
    break;
   case REC_555:
    C1 = 60475.43;
    C2 = 116543.50;
    sf = 0.0003021341;
    break;
   case REC_572:
    C1 = 58120.92;
    C2 = 115256.73;
    sf = 0.0003128605;
    break;
  }
   
  d = 0.0;
  for (i = -2; i <= 2; i++) {
    s3900 = 299.79/(mech + C1)*(C2 + (double)i/sf)-LOfreq/1.0e9;
    if (fabs(fabs(s3900)-3.9) < fabs(fabs(d)-3.9)) {
      d = s3900;
    }
  }

  return (d > 0.0 ? 3900.0e6 : -3900.0e6 );
}

int AOSlaserT(unsigned long *STW, double *array)
{
  return shkArray(1, 0, HK2Eaos1, 0xffff, STW, array);
}

int AOSlaserC(unsigned long *STW, double *array)
{
  return shkArray(1, 1, HK2Eaos2, 0xffff, STW, array);
}

int AOSstruct(unsigned long *STW, double *array)
{
  return shkArray(1, 2, HK2Eaos3, 0xffff, STW, array);
}

int AOScontinuum(unsigned long *STW, double *array)
{
  return shkArray(1, 3, HK2Eaos4, 0xffff, STW, array);
}

int AOSprocessor(unsigned long *STW, double *array)
{
  return shkArray(1, 4, HK2Eaos5, 0xffff, STW, array);
}

int DROtemp(unsigned long *STW, double *array)
{
  return shkArray(13, 1, HK2Edro, 0xffff, STW, array);
}
