#include <stdio.h>
#include <ctype.h>

static char rcsid[] = "$Id$";

#include "osu.h"

unsigned int GetSTW(Format format)
{
  unsigned int stw;

  stw = (format[2][1]<<16) + format[1][1];
  return stw;
}

int GetFormat(FILE *tm, Format format)
{
  int n;

  n = fread(format, sizeof(Format), 1, tm);
  if (feof(tm) || ferror(tm)) return 0; 
  return n;
}

int PutHKBlock(FILE *shk, HKBlock *block)
{
  int n;

  n = fwrite(block, sizeof(HKBlock), 1, shk);
  if (ferror(shk)) return 0;
  return n;
}

int PutAC1Block(FILE *ac1, AC1Block *block)
{
  int n;

  n = fwrite(block, sizeof(AC1Block), 1, ac1);
  if (ferror(ac1)) return 0;
  return n;
}

int PutAC2Block(FILE *ac2, AC2Block *block)
{
  int n;

  n = fwrite(block, sizeof(AC2Block), 1, ac2);
  if (ferror(ac2)) return 0;
  return n;
}

int PutAOSBlock(FILE *aos, AOSBlock *block)
{
  int n;

  n = fwrite(block, sizeof(AOSBlock), 1, aos);
  if (ferror(aos)) return 0;
  return n;
}

int PutFBABlock(FILE *fba, FBABlock *block)
{
  int n;

  n = fwrite(block, sizeof(FBABlock), 1, fba);
  if (ferror(fba)) return 0;
  return n;
}

FILE *OpenSHKFile(int rstcnt, unsigned int stw, char *blockname)
{
  FILE *fp;
  int k;

  sprintf(blockname, "%x%07x.SHK", rstcnt, stw>>4);
  for (k = 0; k < 8; k++) blockname[k] = toupper(blockname[k]);
  fp = fopen(blockname, "w");
  return fp;
}

FILE *OpenAC1File(int rstcnt, unsigned int stw, char *blockname)
{
  FILE *fp;
  int k;

  sprintf(blockname, "%x%07x.AC1", rstcnt, stw>>4);
  for (k = 0; k < 8; k++) blockname[k] = toupper(blockname[k]);
  fp = fopen(blockname, "w");
  return fp;
}

FILE *OpenAC2File(int rstcnt, unsigned int stw, char *blockname)
{
  FILE *fp;
  int k;

  sprintf(blockname, "%x%07x.AC2", rstcnt, stw>>4);
  for (k = 0; k < 8; k++) blockname[k] = toupper(blockname[k]);
  fp = fopen(blockname, "w");
  return fp;
}

FILE *OpenAOSFile(int rstcnt, unsigned int stw, char *blockname)
{
  FILE *fp;
  int k;

  sprintf(blockname, "%x%07x.AOS", rstcnt, stw>>4);
  for (k = 0; k < 8; k++) blockname[k] = toupper(blockname[k]);
  fp = fopen(blockname, "w");
  return fp;
}

FILE *OpenFBAFile(int rstcnt, unsigned int stw, char *blockname)
{
  FILE *fp;
  int k;

  sprintf(blockname, "%x%07x.FBA", rstcnt, stw>>4);
  for (k = 0; k < 8; k++) blockname[k] = toupper(blockname[k]);
  fp = fopen(blockname, "w");
  return fp;
}

HKBlock *NewHKBlock(unsigned int stw)
{
  static HKBlock shkblock;
  int k;

  shkblock.head.SYNC = SYNCWORD;
  shkblock.head.STW[0] = stw & 0xffff;
  shkblock.head.STW[1] = stw >> 16;
  shkblock.head.UserHead = HKUSER;
  shkblock.index = 0;
  for (k = 0; k < HKBLOCKSIZE; k++) shkblock.data[k] = 0;
  for (k = 0; k < HKTAILERLEN; k++) shkblock.tail[k] = TAILWORD;

  return &shkblock;
}

AC1Block *NewAC1Block(unsigned int stw)
{
  static AC1Block ac1block;
  int k;

  ac1block.head.SYNC = SYNCWORD;
  ac1block.head.STW[0] = stw & 0xffff;
  ac1block.head.STW[1] = stw >> 16;
  ac1block.head.UserHead = AC1USER;
  ac1block.index = 0;
  for (k = 0; k < AC1BLOCKSIZE; k++) ac1block.data[k] = 0;
  for (k = 0; k < AC1TAILERLEN; k++) ac1block.tail[k] = TAILWORD;

  return &ac1block;
}

AC2Block *NewAC2Block(unsigned int stw)
{
  static AC2Block ac2block;
  int k;

  ac2block.head.SYNC = SYNCWORD;
  ac2block.head.STW[0] = stw & 0xffff;
  ac2block.head.STW[1] = stw >> 16;
  ac2block.head.UserHead = AC2USER;
  ac2block.index = 0;
  for (k = 0; k < AC2BLOCKSIZE; k++) ac2block.data[k] = 0;
  for (k = 0; k < AC2TAILERLEN; k++) ac2block.tail[k] = TAILWORD;

  return &ac2block;
}

AOSBlock *NewAOSBlock(unsigned int stw)
{
  static AOSBlock aosblock;
  int k;

  aosblock.head.SYNC = SYNCWORD;
  aosblock.head.STW[0] = stw & 0xffff;
  aosblock.head.STW[1] = stw >> 16;
  aosblock.head.UserHead = AOSUSER;
  aosblock.index = 0;
  for (k = 0; k < AOSBLOCKSIZE; k++) aosblock.data[k] = 0;
  for (k = 0; k < AOSTAILERLEN; k++) aosblock.tail[k] = TAILWORD;

  return &aosblock;
}

FBABlock *NewFBABlock(unsigned int stw)
{
  static FBABlock fbablock;
  int k;

  fbablock.head.SYNC = SYNCWORD;
  fbablock.head.STW[0] = stw & 0xffff;
  fbablock.head.STW[1] = stw >> 16;
  fbablock.head.UserHead = FBAUSER;
  fbablock.index = 0;
  for (k = 0; k < FBABLOCKSIZE; k++) fbablock.data[k] = 0;
  for (k = 0; k < FBATAILERLEN; k++) fbablock.tail[k] = TAILWORD;

  return &fbablock;
}

void GetHKData(Format format, HKBlock *hkblock)
{
  int k, frame, word;

  k = 0;

  /* HK47 word: 1 */
  frame = 2;
  word = 3;
  hkblock->data[k++] = format[frame][word];
  
  /* AOS word: 2,3,4,5 */
  frame = 2;
  for (word = 4; word < 8; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* OS  word: 6,7,8 */
  frame = 5;
  for (word = 5; word < 8; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* HK46 word: 9 */
  frame = 2;
  word = 2;
  hkblock->data[k++] = format[frame][word];
  
  /* HK4C word: 10 */
  frame = 11;
  word = 3;
  hkblock->data[k++] = format[frame][word];
  
  /* HK48 word: 11 */
  frame = 11;
  word = 1;
  hkblock->data[k++] = format[frame][word];
  
  /* HK4D word: 12 */
  frame = 11;
  word = 4;
  hkblock->data[k++] = format[frame][word];
  
  /* HK49 word: 13 */
  frame = 11;
  word = 2;
  hkblock->data[k++] = format[frame][word];
  
  /* HK40-43 word: 14,15,16,17 */
  frame = 11;
  for (word = 7; word < 11; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* PLL A word: 18-25 */
  frame = 11;
  for (word = 11; word < 19; word++) {
    hkblock->data[k++] = format[frame][word];
  }
  /* PLL B word: 26-33 */
  for (word = 19; word < 27; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* MECH A word: 34-39 */
  frame = 11;
  for (word = 27; word < 33; word++) {
    hkblock->data[k++] = format[frame][word];
  }
  /* MECH A word: 40-45 */
  for (word = 33; word < 39; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* HK pyro d5 word: 46 */
  frame = 0;
  word = 3;
  hkblock->data[k++] = format[frame][word];
  
  /* HK4A word: 47*/
  frame = 0;
  word = 4;
  hkblock->data[k++] = format[frame][word];
  
  /* ACDC sync word: 48,49 */
  frame = 0;
  word = 1;
  hkblock->data[k++] = format[frame][word];
  word = 2;
  hkblock->data[k++] = format[frame][word];
  
  /* ACS avail word 50 */
  frame = 0;
  word = 5;
  hkblock->data[k++] = format[frame][word];

  /* HK20-22 word 51,52,53 */
  frame = 9;
  for (word = 1; word < 4; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* HK44-45 word: 54,55 */
  frame = 12;
  for (word = 1; word < 3; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* ACDC slow status word: 56,57 */
  frame = 12;
  word = 15;
  hkblock->data[k++] = format[frame][word];
  word = 16;
  hkblock->data[k++] = format[frame][word];
  
  /* HK30-32 word: 58,59,60 */
  frame = 13;
  for (word = 1; word < 4; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* Gyro 1-3 word: 61,62,63 */
  frame = 13;
  word = 4;
  hkblock->data[k++] = format[frame][word];
  word = 10;
  hkblock->data[k++] = format[frame][word];
  word = 16;
  hkblock->data[k++] = format[frame][word];

  /* HK4E-4F word: 64,65 */
  frame = 14;
  for (word = 39; word < 41; word++) {
    hkblock->data[k++] = format[frame][word];
  }

  /* OSC 11-12 word: 66,67 */
  /*    frame = ??; */
  /*    for (word = ??; word < ??; word++) { */
  /*      hkblock->data[k++] = format[frame][word]; */
  /*    } */
  hkblock->data[k++] = 0xffff;
  hkblock->data[k++] = 0xffff;
}

void GetAC1Data(Format format, AC1Block *ac1block)
{
  int k, frame, word;

  k = 0;
  for (frame = 0; frame < 2; frame++) {
    for (word = (frame == 0 ? 14 : 4); word < 41; word++) {
      ac1block->data[k++] = format[frame][word];
    }
  }
}

void GetAC2Data(Format format, AC2Block *ac2block)
{
  int k, frame, word;

  k = 0;
  for (frame = 9; frame < 11; frame++) {
    for (word = 9; word < 41; word++) {
      ac2block->data[k++] = format[frame][word];
    }
  }
}

void GetAOSData(Format format, AOSBlock *aosblock)
{
  int k, frame, word;

  k = 0;
  for (frame = 2; frame < 5; frame++) {
    for (word = (frame == 2 ? 9 : 1); word < 41; word++) {
      aosblock->data[k++] = format[frame][word];
    }
  }
}

void GetFBAData(Format format, FBABlock *fbablock)
{
  int k, frame, word;

  k = 0;
  for (frame = 14; frame < 15; frame++) {
    for (word = 34; word < 39; word++) {
      fbablock->data[k++] = format[frame][word];
    }
  }
  fbablock->data[k++] = format[11][27];
  fbablock->data[k++] = format[11][33];
}

unsigned short AC1sync(Format format)
{
  unsigned short sync;

  sync = format[0][13];
  if ((sync & 0xff00) != 0x7300) return 0;
  else                           return sync;
}

unsigned short AC2sync(Format format)
{
  unsigned short sync;

  sync = format[9][8];
  if ((sync & 0xff00) != 0x7300) return 0;
  else                           return sync;
}

unsigned short AOSsync(Format format)
{
  unsigned short sync;

  sync = format[2][8];
  if ((sync & 0xff00) != 0x7300) return 0;
  else                           return sync;
}

unsigned short FBAsync(Format format)
{
  unsigned short sync;

  sync = format[14][34];
  if (sync == 0xb9bd) return 0;
  else                return sync;
}

