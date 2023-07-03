#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>

#include "odinscan.h"
#include "odinlib.h"
#include "fbalib.h"

static char rcsid[] = "$Id$";

static int rstcnt;

/* 
   Function 'ReadFBALevel0' reads a level 0 file (STW.FBA) containing FBA 
   science data.
   The data type 'Format' is declared in 'tm.h'.
   
   function variables:
   - char *filename    a pointer to the name of the file to be read
   - int *fcnt         pointer to an integer which will receive 
                       the number of formats found
   return value:
   - a pointer to the array of ACSBlock found
*/
FBABlock *ReadFBALevel0(char *filename, int *fcnt)
{
  static struct stat buf;
  int nBlocks, n, i;
  FBABlock *fba;
  FILE *tmdata;
  
  /* Reset number of FBABlock counter */
  *fcnt = 0;

  /* Get file status to retrieve file size */
  if (stat(filename, &buf) == -1) return (FBABlock *)NULL;
  
  /* Is file size a multiple of the size of one FBABlock ? */
  if (buf.st_size % sizeof(FBABlock)) return (FBABlock *)NULL;
  
  /* Calculate number of FBABlocks */
  nBlocks = buf.st_size/sizeof(FBABlock);

  /* Allocate required array of FBA blocks */
  fba = (FBABlock *)calloc(nBlocks, sizeof(FBABlock));
  if (fba == NULL) {
    ODINwarning("memory allocation error");
    return (FBABlock *)NULL;
  } 

  /* Open the file for reading */
  tmdata = fopen(filename, "r");
  if (tmdata == NULL) {
    ODINwarning("can't open data file '%s'", filename);
    *fcnt = 0;
    return (FBABlock *)NULL;
  }
  
  /* Read all FBA blocks */
  n = fread(fba, sizeof(FBABlock), nBlocks, tmdata);
  if (n != nBlocks) {
    ODINwarning("read error");
    *fcnt = 0;
    return (FBABlock *)NULL;
  }
  for (i = 0; i < nBlocks; i++) {
    if (fba[i].head.SYNC != SYNCWORD) {
      ODINwarning("wrong sync word");
      *fcnt = 0;
      return (FBABlock *)NULL;
    }
  }

  /* Set return values */
  ODINinfo("%d blocks in file '%s'", nBlocks, filename);
  *fcnt = nBlocks;
  rstcnt = STWreset(filename);
  return fba;
}

/* 
   Get the integration time from a FBA level 0 block.
   
   input:
   - FBABlock *fba  pointer to fba block
*/
float GetFBAIntTime(FBABlock *fba)
{
  float samples;

  samples = (float)fba->data[4];
  samples *= 25.6e-6;  /* scale factor according to Omnisys documentation */

  return samples;
}

/* 
   Get the value of the counter from a FBA level 0 block.
   
   input:
   - FBABlock *fba  pointer to fba block
*/
int GetFBACounter(FBABlock *fba)
{
  int counter;

  counter = (int)fba->data[3];

  return counter;
}

/*
  Get the position of the calibration mirror.

  This is stored in FBA level 0 files since the Toulouse solar simulation
  tests. Mechanisms A and B are sampled at 1 second intervals and stored in
  two extra words after the FBA science data.
*/
int GetCalMirror(FBABlock *fba)
{
  int mirror, pos;
  unsigned short mechanism;
  unsigned long STW;

  STW = LongWord(fba->head.STW);
  mechanism = fba->data[FBABLOCKSIZE+0];
  if (mechanism == 0xffff) {
    mechanism = fba->data[FBABLOCKSIZE+1];
    if (mechanism == 0xffff) {
      ODINwarning("calibration mirror position undefined at STW = %08x", STW);
      return 0;
    }
  }
  mirror = (mechanism>>13) & 0x0003;

  switch (mirror) {
   case 1:
    pos = SK1;
    break;
   case 2:
    pos = CAL;
    break;
   case 3:
    pos = SK2;
    break;
   default:
    pos = 0;
    break;
  }
  return pos;
}

/*
   Function 'GetFBAChannels' extracts FBA channel contents from 
   one FBABlock.
   
   function variables:
   - float *data       a pointer to an array to receive the data
   - FBABlock *fba     a pointer to the first format holding the data

   On exit extracted channels are stored in array data.
   
   return value:
   - the number of channels extracted or -1 in case of error
*/
int GetFBAChannels(float *data, FBABlock *fba)
{
  int block, word;

  block = 0;
  /* Note, that we rearrange channels in increasing frequency order,
     they are returned in decreasing order by the filterbank. */
  for (word = 0; word < FILTERCHANNELS; word++) {
    data[FILTERCHANNELS-1-word] = (float)fba[block].data[word];
  }
  return word;
}

/*
   Function 'GetFBAscan' extracts FBA data information from an
   array of FBABlock's.
   
   function variables:
   - FBABlock *block   a pointer to an array of FBABlocks
   - int *fcnt         a pointer to the number of FBABlock's in the array
                       will be updated to the number of spectra found

   return value:
   - a pointer to the array of OdinScan structures
*/
struct OdinScan *GetFBAscan(FBABlock *block, int *fcnt)
{
  struct OdinScan *scan, *next;
  FBABlock *fba;
  int i, j, k;
  int nChannels, nFBA, sync, blocks, count;
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
  nFBA = 0;
  /* loop through all formats */
  count = -1;
  for (i = 0; i < blocks; i++) {
    /* look for FBA sync word */
    sync = block[i].head.UserHead;
    if (sync == FBAUSER) {
      if (block[i].data[0] != 0xffff) {
	if ((block[i].data[0] & 0xfff0) != 0xb9b0) {
    	  if (block[i].data[3] != count) { 
    	    count = block[i].data[3];
	    index[nFBA] = i;
	    nFBA++;
    	  }
	}
      }
    }
  }

  /* Did we find any data at all ? */
  if (nFBA == 0) {
    ODINwarning("no valid data found");
    *fcnt = 0;
    free(index);
    return (struct OdinScan *)NULL;
  }

  ODINinfo("%d FBA data blocks found", nFBA);

  /* Allocate array of OdinFilter structures to hold data */
  scan = (struct OdinScan *)calloc(nFBA, sizeof(struct OdinScan));
  if (scan == NULL) {
    ODINwarning("memory allocation error");
    free(index);
    return (struct OdinScan *)NULL;
  }  

  /* Extract data values */
  next = scan;
  for (i = 0; i < nFBA; i++) {
    j = index[i];
    fba = &block[j];
    nChannels = GetFBAChannels(next->data, fba);
    if (nChannels == -1) {
      ODINwarning("can't extract channels");
      continue;
    } else {
      next->Version  = ODINSCANVERSION;
      next->Level    = 0x0000;
      next->Quality  = rstcnt;
      next->Channels = nChannels;
      next->STW      = LongWord(fba->head.STW);
      next->Backend  = FBA;
      next->Frontend = REC_119;
      next->IntTime  = GetFBAIntTime(fba);
      next->Type     = GetCalMirror(fba);
      next->SkyFreq  = 118.749860e9;
      next->LOFreq   = next->SkyFreq-3900.0e6;
      next->FreqRes  = 100.0e6;
      next->FreqCal[0] = 3900.0e6+LOWFREQ;
      next->FreqCal[1] = 3900.0e6+MIDFREQ;
      next->FreqCal[2] = 3900.0e6+HIFREQ;
      next->FreqCal[3] = 0.0;
      for (k = 0; k < FILTERCHANNELS; k++) {
	next->data[k] /= next->IntTime;
      }
      next++;
    }
  }

  free(index);
  *fcnt = nFBA;
  return scan;
}
