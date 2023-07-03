#ifndef BINTABLE_H
#define BINTABLE_H

/* $Id$ */

#include "fitsio.h"

typedef struct {
  char type[16];
  char form[8];
  char unit[8];
  int dim;
  int size;
} Column;

#define ODINSCANMEMBERS (sizeof(fitstable)/sizeof(Column))

void swapscan(char *);
int rowSize();
void getColumn(char *, char *, int );
void putColumn(char *, char *, int );
void encodescan(struct OdinScan *, char *);
void decodescan(struct OdinScan *, char *);
void writebintable(fitsfile *);
int readbintable(fitsfile *);
void printerror(int );

#endif  /* BINTABLE_H */
