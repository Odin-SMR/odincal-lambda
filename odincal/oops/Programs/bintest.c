#include <stdio.h>

#include "odinscan.h"
#include "bintable.h"
#include "fitsio.h"

static char rcsid[] = "$Id";

struct OdinScan scan;

extern Column fitstable[47];

void check(char *member)
{
  static int i = 0, total = 0, offset = 0;
  int size;

  offset = member - (char*)&scan;
  size = fitstable[i].size * fitstable[i].dim;
  printf("%2d: %-16s %3d %c= %3d [%4d]\n", i, fitstable[i].type, 
	 offset, (offset == total? '=' : '!'), total, size);
  total += size;
  i++;
}

void bt()
{
  static char *ttype[ODINSCANMEMBERS];
  static char *tform[ODINSCANMEMBERS];
  static char *tunit[ODINSCANMEMBERS];
  int i;

  for (i = 0; i < ODINSCANMEMBERS; i++) {
    ttype[i] = fitstable[i].type;
    tform[i] = fitstable[i].form;
    tunit[i] = fitstable[i].unit;
  }

  printf("\n");
  for (i = 0; i < ODINSCANMEMBERS; i++) {
    printf("%-16s %8s %-8s\n", ttype[i], tform[i], tunit[i]);
  }
}

int main()
{
  check((char *)&(scan.Version));
  check((char *)&(scan.Level));
  check((char *)&(scan.Quality));
  check((char *)&(scan.STW));
  check((char *)&(scan.MJD));
  check((char *)&(scan.Orbit));
  check((char *)&(scan.LST));
  check((char *)&(scan.Source));
  check((char *)&(scan.Discipline));
  check((char *)&(scan.Topic));
  check((char *)&(scan.Spectrum));
  check((char *)&(scan.ObsMode));
  check((char *)&(scan.Type));
  check((char *)&(scan.Frontend));
  check((char *)&(scan.Backend));
  check((char *)&(scan.SkyBeamHit));
  check((char *)&(scan.RA2000));
  check((char *)&(scan.Dec2000));
  check((char *)&(scan.VSource));
  check((char *)&(scan.u.map.Xoff));
  check((char *)&(scan.u.map.Yoff));
  check((char *)&(scan.u.map.Tilt));
  check((char *)&(scan.Qtarget));
  check((char *)&(scan.Qachieved));
  check((char *)&(scan.Qerror));
  check((char *)&(scan.GPSpos));
  check((char *)&(scan.GPSvel));
  check((char *)&(scan.SunPos));
  check((char *)&(scan.MoonPos));
  check((char *)&(scan.SunZD));
  check((char *)&(scan.Vgeo));
  check((char *)&(scan.Vlsr));
  check((char *)&(scan.Tcal));
  check((char *)&(scan.Tsys));
  check((char *)&(scan.SBpath));
  check((char *)&(scan.LOFreq));
  check((char *)&(scan.SkyFreq));
  check((char *)&(scan.RestFreq));
  check((char *)&(scan.MaxSuppression));
  check((char *)&(scan.FreqThrow));
  check((char *)&(scan.FreqRes));
  check((char *)&(scan.FreqCal));
  check((char *)&(scan.IntMode));
  check((char *)&(scan.IntTime));
  check((char *)&(scan.EffTime));
  check((char *)&(scan.Channels));
  check((char *)&(scan.data));

  printf("    ----------------------------------\n");
  printf("    total size of odinscan      [%4d]\n", sizeof(struct OdinScan));
  printf("    total size of FITS row      [%4d]\n", rowSize());

  bt();

  exit(0);
}
