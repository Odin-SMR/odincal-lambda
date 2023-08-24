#ifndef FBALIB_H
#define FBALIB_H

/* $Id$ */

#include "level0.h"
#include "odinscan.h"

FBABlock *ReadFBALevel0(char *, int *);
int      GetCalMirror(FBABlock *fba);
float    GetFBAIntTime(FBABlock *);
int      GetFBACounter(FBABlock *);
int      GetFBAChannels(float *, FBABlock *);
struct OdinScan *GetFBAscan(FBABlock *, int *);

#endif
