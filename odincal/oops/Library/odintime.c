#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "astrolib.h"
#include "odintime.h"

static char rcsid[] = "$Id$";

static struct {
  unsigned long STW0, CLK0;
} STWreset[] = {
  /* { 0x01cf3600, 944660100L }, STW on 1999-12-08 13:35 UTC.           0 */
  /* { 0x0113e400, 947081460L }, STW on 2000-01-05 14:11 UTC.           1 */
  /* { 0x00000000, 950787903L }, STW on 2000-01-17 11:45 UTC.           2 */
  /* { 0x0091ca2a, 962965817L }, STW on 2000-07-07 10:30:17 (Toulouse)  3 */
  /* { 0x030d9733, 975690082L }, STW on 2000-12-01 17:01:22 (Linkoping) 4 */
  /* { 0x004d2e5f, 980815386L }, STW on 2001-01-30 00:43:06 (Svobodny)  5 */
  /* { 0x0014bd34, 981591480L }, STW on 2001-02-08 00:18:00 (Svobodny)  6 */
  { 0x001aeecc, 982752996 }, /* STW on 2001-02-21 10:56:36.90 (Flight)  0 */
};

static unsigned long launch = 2451961L;   /* JD on launch date 12:00 UTC   */

/*static char stw_pln[] = "/archive/time/STW_PLN.DAT";*/
static char stw_pln[] = "/home/bengt/Downloads/stw_pln.dat";
/*static char eqx_mat[] = "/archive/time/eqx.mat";*/
static char eqx_mat[] = "/home/bengt/test/eqx.mat";


struct STWtable {
  unsigned long stw;
  double JD;
  double rate;
};

static int stwnum, eqxnum;
static struct STWtable *stwtbl;
static double *eqx;

static void readEQX_MAT()
{
  FILE *f;
  char *ptr;
  static char line[256];
  int i, n, need;
  double JD;
  int orbit;
  struct stat buf;
 
  f = fopen(eqx_mat, "r");
  if (f == NULL) {
    fprintf(stderr, "can't open '%s'\n", eqx_mat);
    exit(1);
  }
  

  if (stat(eqx_mat, &buf) == -1) {
    fprintf(stderr, "can't stat '%s'\n", eqx_mat);
    exit(1);
  }

  need = (int)buf.st_size;
  
  need = need/101;
  eqx = NULL;
  if ((eqx = calloc(need, sizeof(double))) == NULL) {
    fprintf(stderr, "memory allocation failure\n");
    exit(1);
  }
  eqxnum = need;
  for (orbit = 0; orbit < eqxnum; orbit++) {
    eqx[orbit] = 0.0;
  }
  
  n = 0;
  while (fgets(line, 255, f) != NULL) {
    i = strlen(line);
    if ((ptr = rindex(line, '\n')) != NULL) *ptr = '\0';
    if ((ptr = rindex(line, '\r')) != NULL) *ptr = '\0';
    ptr = strtok(line, " \t\n\0");
    i = 0;
    orbit = -1;
    JD = 0.0;
    while (ptr) {
      if (i == 0) {
	orbit = (int)floor(atof(ptr));
      }
      if (i == 3) {
	JD = atof(ptr);
      }
      ptr = strtok(NULL, " \t\n\0");
      i++;
    }
    if ((orbit < 0) || (orbit >= eqxnum) || (JD == 0.0)) {
      fprintf(stderr, "orbit table failure\n");
      exit(1);
    }
    eqx[orbit] = JD;
  }
  fclose(f);
}

static void readSTW_PLN()
{
  FILE *f;
  char *ptr;
  static char line[256];
  int i, n, need;
  unsigned long stw, date;
  int year, month, day;
  double secs;
  double rate;
  struct stat buf;

  f = fopen(stw_pln, "r");
  if (f == NULL) {
    fprintf(stderr, "can't open '%s'\n", stw_pln);
    exit(1);
  }
  
  if (stat(stw_pln, &buf) == -1) {
    fprintf(stderr, "can't stat '%s'\n", stw_pln);
    exit(1);
  }

  need = (int)buf.st_size;
  need = need/153;
  if ((stwtbl = calloc(need, sizeof(struct STWtable))) == NULL) {
    fprintf(stderr, "memory allocation failure\n");
    exit(1);
  }
  stwnum = need;

  n = 0;
  while (fgets(line, 255, f) != NULL) {
    if (line[0] == '!') continue;
    i = strlen(line);
    if ((ptr = rindex(line, '\n')) != NULL) *ptr = '\0';
    if ((ptr = rindex(line, '\r')) != NULL) *ptr = '\0';
    ptr = strtok(line, " \t\n\0");
    i = 0;
    while (ptr) {
      if ((i == 1) && strncmp(ptr, "0000", 4)) break;
      if (i == 7) {
	stw = strtoul(ptr, NULL, 16);
	stwtbl[n].stw = stw;
	stwtbl[n+1].stw = 0L;
      }
      if (i == 8) {
	date = strtoul(ptr, NULL, 10);
	year = date/10000;
	month = (date/100) % 100;
	day = date % 100;
	stwtbl[n].JD = djl(year, month, day);
      }
      if (i == 9) {
	secs = atof(ptr);
	stwtbl[n].JD += secs/86400.0;
      }
      if (i == 11) {
	rate = atof(ptr);
	stwtbl[n].rate = rate;
	n++;
	if (n == stwnum) {
	  fprintf(stderr, "table size exceeded\n");
	  exit(1);
	}
      }
      ptr = strtok(NULL, " \t\n\0");
      i++;
    }
  }
  stwnum = n;
  fclose(f);
}

static double orbit2JD(double orbit)
{
  double r, o, JD;
  int i;

  if (orbit < 0.0) return 0.0;
  if (eqxnum == 0) readEQX_MAT();
  r = modf(orbit, &o);
  i = (int)o;
  if (i < eqxnum-1) {
    JD = eqx[i]+r*(eqx[i+1]-eqx[i]);
  } else {
    r = orbit-(double)eqxnum+1.0;
    JD = eqx[eqxnum-1]+r*(eqx[eqxnum-1]-eqx[eqxnum-2]);
  }

  return JD;
}

static double JD2orbit(double JD)
{
  int i;
  double orbit, r;

  if (JD < 2451960.8604086721) return 0.0;
  if (eqxnum == 0) readEQX_MAT();

  for (i = 1; i < eqxnum; i++) {
    if (eqx[i] >= JD) {
      r = (JD-eqx[i-1])/(eqx[i]-eqx[i-1]);
      orbit = (double)(i-1)+r;
      break;
    }
  }
  if (i == eqxnum) {
    r = (JD-eqx[eqxnum-2])/(eqx[eqxnum-1]-eqx[eqxnum-2]);
    orbit = (double)eqxnum+r-2.0;
  }
  return orbit;
}

static double stw2JD(unsigned long stw)
{
  int n0, n1, n2;
  unsigned long stw0;
  double rate, JD, JD0;

  if (stwnum == 0) readSTW_PLN();

  n0 = 0;
  n1 = stwnum-1;
  if (stw <= stwtbl[n0].stw) {
    stw0 = stwtbl[n0].stw;
    JD0  = stwtbl[n0].JD;
    rate = stwtbl[n0].rate;
  } else if (stw >= stwtbl[n1].stw) {
    stw0 = stwtbl[n1].stw;
    JD0  = stwtbl[n1].JD;
    rate = stwtbl[n1].rate;
  } else {
    while ((n1-n0) > 1) {
      n2 = (n1+n0)/2;
      stw0 = stwtbl[n2].stw;
      if (stw <= stw0) n1 = n2;
      else             n0 = n2;
    }
    stw0 = stwtbl[n0].stw;
    JD0  = stwtbl[n0].JD;
    rate = stwtbl[n0].rate;
  }
  if (stw >= stw0) JD = JD0 + (stw-stw0)*rate/86400.0;
  else             JD = JD0 - (stw0-stw)*rate/86400.0;

  return JD;
}

static unsigned long JD2stw(double JD)
{
  int n0, n1, n2;
  double rate, JD0;
  unsigned long stw, stw0;

  if (stwnum == 0) readSTW_PLN();
  n0 = 0;
  n1 = stwnum-1;
  if (JD <= stwtbl[n0].JD) {
    stw0 = stwtbl[n0].stw;
    JD0  = stwtbl[n0].JD;
    rate = stwtbl[n0].rate;
  } else if (JD >= stwtbl[n1].JD) {
    stw0 = stwtbl[n1].stw;
    JD0  = stwtbl[n1].JD;
    rate = stwtbl[n1].rate;
  } else {
    while ((n1-n0) > 1) {
      n2 = (n1+n0)/2;
      JD0 = stwtbl[n2].JD;
      if (JD <= JD0) n1 = n2;
      else           n0 = n2;
    }
    stw0 = stwtbl[n0].stw;
    JD0  = stwtbl[n0].JD;
    rate = stwtbl[n0].rate;
  }
  stw = stw0 + (long)floor((JD-JD0)*86400.0/rate+0.5);
  return stw;
}

static int validreset(int reset)
{
  int n;
  
  n = sizeof(STWreset)/sizeof(STWreset[0]);
  if (reset < 0 || reset >= n) reset = n-1;
  
  return reset;
}

/*
  Calculate modified Julian date from given date and time.
*/
double mjd(int year, int month, int day, int hour, int min, double secs)
{
  double JD, mjd;

  JD = djl(year, month, day);
  secs += (double)(hour*3600 + min*60);
  JD += secs/86400.0;
  mjd = jd2mjd(JD);

  return mjd;
}

/* 
   The following routine converts STW to mjd. 
 */
double stw2mjd(unsigned long stw, int reset)
{
  double JD, mjd;

  JD = stw2JD(stw);
  mjd = jd2mjd(JD);

  return mjd;
}

/* 
   The following routine converts mjd to STW. 
 */
unsigned long mjd2stw(double mjd, int reset)
{
  double JD;
  unsigned long stw;

  JD = mjd2jd(mjd);
  stw = JD2stw(JD);

  return stw;
}

/* 
   The following routine converts STW to UTC. 
   It returns number of seconds since 1970-01-01.
 */
unsigned long stw2utc(unsigned long STW, int reset)
{
  unsigned long utc; 
  double mjd;

  mjd = stw2mjd(STW, reset);
  utc = mjd2utc(mjd);

  return utc;
}

/* 
 * The following routine converts UTC to STW. 
 * It takes number of seconds since 1970-01-01.
 */
unsigned long utc2stw(unsigned long utc, int reset)
{
  unsigned long STW;
  double mjd;

  mjd = utc2mjd(utc);
  STW = mjd2stw(mjd, reset);

  return STW;
}

/* 
 * The following routine converts UTC to MJD. 
 * It takes number of seconds since 1970-01-01.
 */
double utc2mjd(unsigned long utc)
{
  struct tm *gmt;
  double secs;
  int year, month, day, hour, min;

  gmt = gmtime(&utc);
  year  = gmt->tm_year+1900;
  month = gmt->tm_mon+1;
  day   = gmt->tm_mday;
  hour  = gmt->tm_hour;
  min   = gmt->tm_min;
  secs  = (double)gmt->tm_sec;
  
  return mjd(year, month, day, hour, min, secs);
}

/* 
 * Inverse of the above.
 */
unsigned long mjd2utc(double mjd)
{
  struct tm gmt;
  unsigned long now;
  double secs, JD;

  JD = mjd2jd(mjd);
  cld(JD, &gmt.tm_year,&gmt.tm_mon,&gmt.tm_mday,
      &gmt.tm_hour,&gmt.tm_min,&secs);
  gmt.tm_sec = (int)floor(secs);
  gmt.tm_year -= 1900;
  gmt.tm_mon  -= 1;
  gmt.tm_wday = 0;
  gmt.tm_yday = 0;
  
  gmt.tm_isdst = 0;
  now = mktime(&gmt)+3600;

  return now;
}

/* 
 * The following routine converts MJD to orbit number.
 * Will return 0.0 for days before 2001-02-20.
 */
double mjd2orbit(double mjd)
{
  double orbit, JD;

  JD = mjd2jd(mjd);
  orbit = JD2orbit(JD);

  return orbit;
}

/* 
 * Inverse of the above.
 */
double orbit2mjd(double orbit)
{
  double JD, mjd;

  JD = orbit2JD(orbit);
  mjd = jd2mjd(JD);

  return mjd;
}

/* 
 * The following routine converts UTC to orbit number. 
 * It takes number of seconds since 1970-01-01.
 */
double utc2orbit(unsigned long utc)
{
  return mjd2orbit(utc2mjd(utc));
}

/* 
 * Inverse of the above.
 */
unsigned long orbit2utc(double orbit)
{
  return mjd2utc(orbit2mjd(orbit));
}

/* 
 * Produce an ASCII representation of UTC.
 */
char *asciitime(unsigned long utc)
{
  static char zeit[32], *at;
  struct tm *gmt;
  int n;

  gmt = gmtime(&utc);
  at = asctime(gmt);
  strcpy(zeit, at);
  n = strlen(zeit);
  /* remove EOL at end of string */
  zeit[n-1] = '\0';

  return zeit;
}

#ifdef MAIN
int main()
{
    unsigned long stw0 = 0x80000000;
    unsigned long stw1, utc;
    double JD;

    JD = stw2JD(stw0);
    stw1 = JD2stw(JD);
    printf("%10u %15.6f %10u\n", stw0, JD, stw1);

    stw0 = 0x7fffffff;
    utc = stw2utc(stw0, -1);
    stw1 = utc2stw(utc, -1);
    printf("%10u %10d %10u\n", stw0, utc, stw1);

    stw0 = 0x80000000;
    utc = stw2utc(stw0, -1);
    stw1 = utc2stw(utc, -1);
    printf("%10u %10d %10u\n", stw0, utc, stw1);

    exit(0);
}

#endif
