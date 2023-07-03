#ifndef OSU_H
#define OSU_H

/* $Id$ */

#include "tm.h"
#include "level0.h"

unsigned int GetSTW(Format );
int GetFormat(FILE *, Format );
int PutAOSBlock(FILE *, AOSBlock *);
int PutAC1Block(FILE *, AC1Block *);
int PutAC2Block(FILE *, AC2Block *);
int PutHKBlock(FILE *, HKBlock *);
int PutFBABlock(FILE *, FBABlock *);
FILE *OpenAOSFile(int ,unsigned int ,char *);
FILE *OpenAC1File(int ,unsigned int ,char *);
FILE *OpenAC2File(int ,unsigned int ,char *);
FILE *OpenSHKFile(int ,unsigned int ,char *);
FILE *OpenFBAFile(int ,unsigned int ,char *);
AC1Block *NewAC1Block(unsigned int );
AC2Block *NewAC2Block(unsigned int );
AOSBlock *NewAOSBlock(unsigned int );
HKBlock *NewHKBlock(unsigned int );
FBABlock *NewFBABlock(unsigned int );
void GetHKData(Format , HKBlock *);
void GetFBAData(Format , FBABlock *);
void GetAOSData(Format , AOSBlock *);
void GetAC1Data(Format , AC1Block *);
void GetAC2Data(Format , AC2Block *);
unsigned short AOSsync(Format );
unsigned short AC1sync(Format );
unsigned short AC2sync(Format );
unsigned short FBAsync(Format );

#endif
