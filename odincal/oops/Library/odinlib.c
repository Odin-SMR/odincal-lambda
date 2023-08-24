#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>

#include "odinlib.h"
#include "odinscan.h"
#include "astrolib.h"
#include "odintime.h"
#include "accessor.h"
#include "drplib.h"
#include "level0.h"
#include "aclib.h"

static char rcsid[] = "$Id$";

/*
  Get two unsigned shorts from level 0 file 
  and return as unsigned long in a machine independent manner.
*/
unsigned long LongWord(unsigned short int word[])
{
    long result;

    result = ((long)word[1]<<16) + word[0];
    return result;
}

/* Given a level 0 filename, try to extract the STW reset counter. */
int STWreset(char *level0)
{
    int i, rst;
    char *ptr, c;

    if ((ptr = strrchr(level0, '/')) != NULL) ptr++;
    else                                      ptr = level0;

    /* make sure filename starts with 8 hex digits followed by a . */
    for (i = 0; i < 8; i++) {
        if (!isxdigit(ptr[i])) return 0;
    }
    if (ptr[8] != '.') return 0;
  
    c = toupper(ptr[0]);
    if (isdigit(c)) rst = c - '0';
    else            rst = c - 'A' + 10;

    return rst;
}

void STWrange(FILE *pdcfile, unsigned long *STW0, unsigned long *STW1)
{
    static unsigned short int word[15];
    unsigned short user;
    int n;
    long offset, where;
  
    *STW0 = 0L;
    *STW1 = 0L;

    where = ftell(pdcfile);

    /* position file at start */
    n = fseek(pdcfile, 0L, SEEK_SET);
    if (n != 0) {
        ODINwarning("failed to position level 0 file");
        return;
    }
    n = fread(word, sizeof(unsigned short), 15, pdcfile);
    if (n != 15) {
        ODINwarning("failed to read level 0 file");
        return;
    }
    if (word[0] != SYNCWORD) {
        ODINwarning("not a valid level 0 file");
        return;
    }
    *STW0 = LongWord(word+1);
    user = word[3];
    offset = 0;
    if (user == HKUSER ) offset = HKBLOCKSIZE/15+1;
    if (user == FBAUSER) offset = FBABLOCKSIZE/15+1;
    if ((user & 0xfff0) == AOSUSER) offset = AOSBLOCKSIZE/15+1;
    if ((user & 0xfff0) == AC1USER) offset = AC1BLOCKSIZE/15+1;
    if ((user & 0xfff0) == AC2USER) offset = AC2BLOCKSIZE/15+1;
    if (user == FBAUSER) offset = FBABLOCKSIZE/15+1;
    if (offset == 0) {
        ODINwarning("unknown user in level 0 file");
        return;
    }
    n = fseek(pdcfile, -offset*15*sizeof(unsigned short), SEEK_END);
    if (n != 0) {
        ODINwarning("failed to position level 0 file");
        return;
    }
    n = fread(word, sizeof(unsigned short), 15, pdcfile);
    if (n != 15) {
        ODINwarning("failed to read level 0 file");
        return;
    }
    if (word[0] != SYNCWORD) {
        ODINwarning("not a valid level 0 file");
        return;
    }
    *STW1 = LongWord(word+1);
  
    /* reposition file to where we were on entry */
    fseek(pdcfile, where, SEEK_SET);

    return;
}

int readODINscan(char *name, Scan *s)
{
    FILE *sc;
    int l, retval = 0;
  
    sc = fopen (name, "r");
    if (sc != NULL) {
        l = fread((char *)s, 1, HEADLEN ,sc);
        if (l == HEADLEN) {
            if (s->Version < 0x0008) {
                ODINwarning("incompatible OdinScan version (%04x)", s->Version);
            } else {
                if (s->Version != ODINSCANVERSION) {
                    /* ODINwarning("wrong OdinScan version (%04x)", s->Version); */
                    s->Version = ODINSCANVERSION;
                }
                if ((s->Channels > 0) && (s->Channels <= MAXCHANNELS)) {
                    l = fread((char *)s->data, sizeof(float), s->Channels, sc);
                    if (l == s->Channels) retval = 1;
                } else {
                    ODINwarning("can't read data of '%s' (%d)", name, errno);
                }
            }
        } else {
            ODINwarning("can't read header of '%s' (%d)", name, errno);
        }
        
        fclose (sc);
        /* ODINinfo("scan read from file '%s'", name); */
    } else {
        ODINwarning("can't open file '%s' (%d)", name, errno);
    }

    return retval;
}

int writeODINscan(char *name, Scan *s)
{
    FILE *sc;
    int l, retval = 0;
  
    sc = fopen (name, "w");
    if (sc != NULL) {
        l = fwrite((char *)s, 1, HEADLEN ,sc);
        if (l == HEADLEN) {
            if (s->Version != ODINSCANVERSION) {
                /* ODINwarning("correcting OdinScan version (%04x)", s->Version); */
                s->Version = ODINSCANVERSION;
            }
            if ((s->Channels > 0) && (s->Channels <= MAXCHANNELS)) {
                l = fwrite((char *)s->data, sizeof(float), s->Channels, sc);
                if (l == s->Channels) retval = 1;
            } else {
                ODINwarning("can't write data for '%s' (%d)", name, errno);
            }
        } else {
            ODINwarning("can't write header for '%s' (%d)", name, errno);
        }
        fclose (sc);
        /* ODINinfo("scan written to file '%s'", name); */
    } else {
        ODINwarning("can't open file '%s' (%d)", name, errno);
    }

    return retval;
}


#define STRSIZE 12

#ifndef PI
#define PI 3.14159265358979323844
#endif
#define RADTOSEC (3600.0*180.0/PI)

char *angle(double rad, int sys)
{
    double ip;
    long centis;
    int d, m, s, ds, cs;
    static char str[STRSIZE];
    int sign = 0;

    if (fabs(rad) > 2.0*PI) rad = modf(rad/(2.0*PI),&ip)*2.0*PI;
    if (rad < -PI/2.0) rad += 2.0*PI;
    if (rad < 0.0) {
        rad = -rad;
        sign = 1;
    }
    centis = (long)(rad*RADTOSEC*100.0);    /* centiseconds         */
    if (sys == 1) centis /= 15;
    centis += 5;                           /* for correct rounding */
    cs = centis%10;    centis /= 10;
    ds = centis%10;    centis /= 10;
    s  = centis%60;    centis /= 60;
    m  = centis%60;    centis /= 60;
    d  = centis;
    sprintf (str, "%3d:%02d:%02d.%1d", d, m, s, ds);
    if (sign)   str[0] = '-';
    if (d < 10) str[1] = '0'; 
    str[STRSIZE-1] = '\0';

    return (str);
}

void PrintScan(Scan *s)
{
    char c;
    int i;
    int year, month, day, hour, min;
    static char utc[STRSIZE], lst[STRSIZE];
    double JD, ip;
    double r, X[3];
    pVector pos;
    static double au = 1.49597892e11;  /* astronomical unit in m */
    static double er = 6.378140e6;  /* equatorial radius of Earth in m */

    JD = mjd2jd(s->MJD);
    strncpy(utc, angle(modf(s->MJD, &ip)*2.0*PI, 1), STRSIZE);
    utc[9] = '\0';
    strncpy(lst, angle((double)s->LST*2.0*PI/86400.0, 1), STRSIZE);
    lst[9] = '\0';
    cld(JD, &year, &month, &day, &hour, &min, &ip);

    printf("\n");
    /*    printf("- STW: %08lx --------------------", s->STW); */
    /*    printf("-----------------[q:%08lx]", s->Quality); */
    printf("- STW: %08lx -[", s->STW);
    for (i = 0; i < SOURCENAMELEN; i++) {
        c = s->Source[i];
        if (!isprint(c)) break;
        printf("%c", c);
    }
    printf("]");
    for (; i < SOURCENAMELEN; i++) {
        printf("-");
    }
    /*    printf("- STW: %08lx - %-32s --", s->STW, Source(s)); */
    printf("--[q:%08lx]", s->Quality);
    printf("[v:%03d.%03d] -\n", s->Version/256, s->Version%256);
    printf("Date ");
    printf("%04d-%02d-%02d", year, month, day);
    printf(" UTC%s", utc);
    printf(" LST%s", lst);
    printf(" MJD ");
    printf("%8.4f", s->MJD);
    printf("  Orbit ");
    printf("%10.3f\n", s->Orbit);
    printf("RA,Dec 2000.0");
    /*    printf("%13.6f", s->RA2000); */
    /*    printf(" %13.6f", s->Dec2000); */
    printf(" %s ", angle(s->RA2000*PI/180.0,1));
    printf(" %s ", angle(s->Dec2000*PI/180.0,0));
    printf("  Discipline %s", Discipline(s));

    printf("  Topic %s", Topic(s));
    printf("\n");
    printf("L.O. frequency %10.3f GHz  ", s->LOFreq/1.0e9);
    printf(" Bandwidth      %8.3f MHz    ", fabs(s->FreqRes)/1.0e6 * s->Channels);
    printf(" Tcal  %6.1f K\n", s->Tcal);
    printf("Sky frequency  %10.3f GHz  ", s->SkyFreq/1.0e9);
    printf(" Vel. resolution %7.1f km/s   ", 
           LIGHTSPEED*fabs(s->FreqRes/s->SkyFreq)/1.0e3);
    printf(" Tsys  %6.1f K\n", s->Tsys);
    printf("Freq. resolution %8.4f MHz  ", s->FreqRes/1.0e6);
    printf(" Vlsr (telescope) %6.1f km/s   ", s->Vlsr/1.0e3);
    printf(" SBpath%6.1f u\n", s->SBpath/1.0e-6);
    if (s->Backend == AOS) 
        printf("Fit: %6.1fe6 %5.2f %5.2f Hz   ", 
               s->FreqCal[0]/1.0e6,
               s->FreqCal[2], s->FreqCal[3]);
    else if (s->Backend == FBA) 
        printf("Filters    %4.0f %4.0f %4.0f MHz  ", 
               s->FreqCal[0]/1.0e6, s->FreqCal[1]/1.0e6, s->FreqCal[2]/1.0e6);
    else 
        printf("SSB   %4.0f %4.0f %4.0f %4.0f MHz  ", 
               s->FreqCal[0]/1.0e6, s->FreqCal[1]/1.0e6,
               s->FreqCal[2]/1.0e6, s->FreqCal[3]/1.0e6);
    printf(" Vgeo (telescope) %6.1f km/s   ", s->Vgeo/1.0e3);
    printf(" Time %7.2f s\n", s->IntTime);
    printf("Frontend %s", Frontend(s));
    printf("  Backend %s", Backend(s));
    printf("  Channels %4d", s->Channels);
    printf("  Obs.mode %s", ObsMode(s));
    printf("  Type %s", Type(s));
    printf("  level 0x%04x\n", s->Level);

    if (s->Discipline == ASTRO) {
        printf("Map pos:  ");
        printf(" %14.6e", Tilt(s));
        printf(" %14.6e", Xoff(s));
        printf(" %14.6e\n", Yoff(s));
    } else {
        printf("Tangent:  ");
        printf(" %14.6e", Longitude(s));
        printf(" %14.6e", Latitude(s));
        printf(" %14.6e\n", Altitude(s));
    }
    printf("GPS pos:  ");
    for (i = 0; i < 3; i++) printf(" %14.6e", s->GPSpos[i]);
    printf("\n");
    printf("GPS vel:  ");
    for (i = 0; i < 3; i++) printf(" %14.6e", s->GPSvel[i]);
    printf("\n");
    printf("Qtarget:  ");
    for (i = 0; i < 4; i++) printf(" %14.6e", s->Qtarget[i]);
    printf("\n");
    printf("Qachieved:");
    for (i = 0; i < 4; i++) printf(" %14.6e", s->Qachieved[i]);
    printf("\n");
    printf("Qerror:   ");
    for (i = 0; i < 3; i++) printf(" %14.6e", s->Qerror[i]);
    printf("\n");

    printf("Sun pos.: ");
    for (i = 0; i < 3; i++) X[i] = s->SunPos[i];
    r = X[0]*X[0] + X[1]*X[1] + X[2]*X[2];
    if (r != 0.0) {
        r = sqrt(X[0]*X[0] + X[1]*X[1] + X[2]*X[2]);
        for (i = 0; i < 3; i++) X[i] /= r;
    }
    uvc(X, &pos);
    printf("  %s  ", angle(pos.l,1));
    printf("  %s  ", angle(pos.b,0));
    printf("%14.6e\n", r/au);

    printf("Moon pos: ");
    for (i = 0; i < 3; i++) X[i] = s->MoonPos[i];
    r = X[0]*X[0] + X[1]*X[1] + X[2]*X[2];
    if (r != 0.0) {
        r = sqrt(X[0]*X[0] + X[1]*X[1] + X[2]*X[2]);
        for (i = 0; i < 3; i++) X[i] /= r;
    }
    uvc(X, &pos);
    printf("  %s  ", angle(pos.l,1));
    printf("  %s  ", angle(pos.b,0));
    printf("%14.6e\n", r/er);

    printf("---------------------------------------");
    printf("---------------------------------------\n");
    printf("\n");
}

void InitHead(Scan *s)
{
    time_t clock;
    struct tm *now;
    double JD, UTC;
    int rstcnt;

    rstcnt = s->Quality & STWRSTMASK;
    if (rstcnt == 0) rstcnt = -1;
    if (s->STW == 0L) {
        time(&clock);
        s->STW = (unsigned long)clock;
    } else {
        clock = stw2utc(s->STW, rstcnt);
    }
    now = gmtime(&clock);
    JD = djl(now->tm_year+1900, now->tm_mon+1, now->tm_mday);
    UTC = (double)(now->tm_hour*3600 + now->tm_min*60 + now->tm_sec)/SECPERDAY;

    s->MJD = jd2mjd(JD)+UTC;
    s->Orbit = 0;
    s->Spectrum = 0;

    s->SkyFreq = 0.0;
    s->LOFreq = 0.0;

    /* So far, both frequency and intensity are unreliable */
    s->Quality |= WFREQUENCY;
    s->Quality |= WAMPLITUDE;

    switch (s->Backend) {
      case AOS:
        /* Initialise with some reasonable values */
        s->FreqCal[0] =  2.10e+09;
        s->FreqCal[1] =  0.62e+06;
        s->FreqCal[2] =  0.0;
        s->FreqCal[3] =  0.0;
        s->FreqRes = s->FreqCal[1];
        break;
      case AC1:
      case AC2:
        /* This should already have been set when reading level 0 data */
        if (s->FreqRes == 0.0) {
            ODINwarning("zero frequency resolution"); 
            switch (s->IntMode) {
              case AC_LOWRES:
                s->FreqRes = 1.0e6;
                break;
              case AC_MEDRES:
                s->FreqRes = 1.0e6/2.0;
                break;
              case AC_HIRES:
                s->FreqRes = 1.0e6/4.0;
                break;
              case AC_XHIRES:
              case AC_YHIRES:
                s->FreqRes = 1.0e6/8.0;
                break;
            }
            break;
        }
    }
  
    memset(s->Source, 0, SOURCENAMELEN);
}

char *SaveName(Scan *s)
{
    static char scanname[32], stw[9];
    char *ptr;
    time_t clock;

    if (s->STW == 0) {
        time(&clock);
        s->STW = (unsigned long)clock;
    }
    switch (s->Backend) {
      case AC1:
        if (s->IntMode & ADC_SEQ) {
            if (s->IntMode & ADC_SPLIT) {
                if (s->IntMode & ADC_UPPER) strcpy(scanname, "A1U.");
                else                        strcpy(scanname, "A1L.");
            } else                          strcpy(scanname, "AC1.");
        } else {
            if (s->IntMode & AC_SPLIT) {
                if (s->IntMode & AC_UPPER)  strcpy(scanname, "A1U.");
                else                        strcpy(scanname, "A1L.");
            } else                          strcpy(scanname, "AC1.");
        }
        break;
      case AC2:
        if (s->IntMode & ADC_SEQ) {
            if (s->IntMode & ADC_SPLIT) {
                if (s->IntMode & ADC_UPPER) strcpy(scanname, "A2U.");
                else                        strcpy(scanname, "A2L.");
            } else                          strcpy(scanname, "AC2.");
        } else {
            if (s->IntMode & AC_SPLIT) {
                if (s->IntMode & AC_UPPER)  strcpy(scanname, "A2U.");
                else                        strcpy(scanname, "A2L.");
            } else                          strcpy(scanname, "AC2.");
        }
        break;
      case AOS:
        strcpy(scanname, "AOS.");
        break;
      case FBA:
        strcpy(scanname, "FBA.");
        break;
      default:
        return NULL;
        break;
    }

    sprintf(stw, "%08lX.", s->STW);
    strcat(scanname, stw);

    ptr = Type(s);
    if (strncmp(ptr, "   ", 3) == 0) strncat(scanname, "XXX", 3);
    else                             strncat(scanname, ptr, 3);

    return scanname;
}

/* Clear the whole scan, but leave the version number intact */
void clearscan(Scan *s)
{
    memset((char *)s, 0, sizeof(Scan));
    s->Version = ODINSCANVERSION;
}

/* Copy one scan to another */
void copyscan(Scan *s, Scan *t)
{
    memcpy((void *)s, (void *)t, sizeof(Scan));
}

/* 
 * Reverse sequence of channels
 */
static void reverse(float data[], int n)
{
    int i, j;
    float swap;
 
    for (i = 0, j = n-1; i < n/2; i++, j--) {
        swap    = data[i];
        data[i] = data[j];
        data[j] = swap;
    }
}

/* 
 * swap two bands
 */
static void swapbands(float band1[], float band2[], int n)
{
    int i;
    float swap;
 
    for (i = 0; i < n; i++) {
        swap     = band1[i];
        band1[i] = band2[i];
        band2[i] = swap;
    }
}

/*
 * Check if all bands have same number of channels, i.e. same resolution.
 * Call it with the array of integers returned by GetACSequence.
 *
 * The routine will return 0 for modes with mixed or unknown band
 * configurations and m > 0 for clean modes, where m is the number of
 * correlators used per band.
 */
int cleanmode(int *seq)
{
    int adc, m;

    if (seq == NULL) return 0;

    m = 0;
    for (adc = 0; adc < 8; adc++) {
        /* grep printf("%d ", seq[2*adc]); */
        if (seq[2*adc]) {
            if (m == 0) m = seq[2*adc];
            else if (m != seq[2*adc]) return 0;
        }
    }
    printf("\n");
    return m;
}

/*
 * Translate the old way of storing the correlator mode 
 * into the new one.
 */
int newMode(Scan *s)
{
    int n, mode;
    double swap;

    /* Makes only sense for AC1 or AC2 */
    if ((s->Backend != AC1) && (s->Backend != AC2)) return 0;
    
    /* Check if this has already been done. */
    if (s->IntMode & ADC_SEQ) {
        ODINwarning("this is a new mode already");
        return 0;
    }

    /* If spectrum already sorted we no longer allow this */
    if (s->Quality & ISORTED) {
        ODINwarning("can't change mode for frequency sorted spectrum");
        return 0;
    }

    /* If spectrum has been split we no longer allow this */
    if (s->IntMode & AC_SPLIT) {
        printf("can't change mode for split spectrum");
        return 0;
    }

    mode = 0;
    switch (s->IntMode & 0x0f) {
      case AC_XHIRES:
        mode = 0x01;
        break;
      case AC_YHIRES:
        /* The old processing of mode AC_YHIRES (0x40) dropped the
           lags from one chip and padded with zeros to make it look
           like a AC_XHIRES mode. So we return the same as above. */
        mode = 0x01;
        break;
      case AC_HIRES:
        mode = 0x11;
        break;
      case AC_MEDRES:
        mode = 0x55;
        break;
      case AC_LOWRES:
        mode = 0xff;
        break;
      default:
        ODINwarning("unknown correlator mode: %04X\n", s->IntMode);
        return 0;
    }

    mode |= ADC_SEQ;

    /* for the new mode SSB LO 2 and 3 need to be swapped back */
    swap          = s->FreqCal[1];
    s->FreqCal[1] = s->FreqCal[2];
    s->FreqCal[2] = swap;

    /* data need to be reorganised depending on mode */
    switch (s->IntMode & 0x0f) {
      case AC_XHIRES:
      case AC_YHIRES:
        /* nothing to be done */
        break;
      case AC_HIRES:
        /* band 2 needs to be turned around */
        n = s->Channels;
        reverse(&(s->data[n/2]), n/2);
        break;
      case AC_MEDRES:
        /* band 1 and 3 need to be turned around */
        n = s->Channels;
        reverse(&(s->data[0*n/4]), n/4);
        reverse(&(s->data[2*n/4]), n/4);
        /* perform band swapping: 3 1 4 2 -> 1 2 3 4 */
        swapbands(&(s->data[0*n/4]), &(s->data[1*n/4]), n/4);
        swapbands(&(s->data[2*n/4]), &(s->data[3*n/4]), n/4);
        swapbands(&(s->data[1*n/4]), &(s->data[2*n/4]), n/4);
        break;
      case AC_LOWRES:
        /* band 1, 3, 5 and 7 need to be turned around */
        n = s->Channels;
        reverse(&(s->data[0*n/8]), n/8);
        reverse(&(s->data[2*n/8]), n/8);
        reverse(&(s->data[4*n/8]), n/8);
        reverse(&(s->data[6*n/8]), n/8);
        /* perform band swapping:  2 1 5 6 4 3 7 8 -> 1 2 3 4 5 6 7 8 */
        swapbands(&(s->data[2*n/8]), &(s->data[5*n/8]), n/8);
        swapbands(&(s->data[3*n/8]), &(s->data[4*n/8]), n/8);
        swapbands(&(s->data[0*n/8]), &(s->data[1*n/8]), n/8);
        swapbands(&(s->data[4*n/8]), &(s->data[5*n/8]), n/8);
        break;
        break;
      default:
        ODINwarning("unknown correlator mode: %04X\n", s->IntMode);
        return 0;
    }
/*  printf("correlator mode changed: %02X -> %04X\n", s->IntMode, mode); */
    s->IntMode = mode;
    return 1;
}

/*
 * For correlator spectra taken via the power combiner, or for spectra with
 * a non-contiguous frequency band, split the spectrum into two halfs 
 * and return one of them. 
 * If the parameter 'upper' is true, replace spectrum with upper half, 
 * else with lower half.
 * Returns 1 on success, 0 on failure.
 */
int splitcorr(Scan *s, int upper)
{
    int i, n, mode;

    if ((s->Backend != AC1) && (s->Backend != AC2)) return 0;

    /* If spectrum already sorted we no longer allow splitting */
    if (s->Quality & ISORTED)                       return 0;

    if (upper) {
        if (s->Frontend == REC_SPLIT) {
            if (s->Backend == AC1) s->Frontend = REC_549;
            else                   s->Frontend = REC_572;
        }
    } else {
        if (s->Frontend == REC_SPLIT) {
            if (s->Backend == AC1) s->Frontend = REC_495;
            else                   s->Frontend = REC_555;
        }
    }

    if (s->IntMode & ADC_SEQ) {
        mode = s->IntMode & 0xff;
        if (upper) s->IntMode |= ADC_UPPER;
        switch (mode) {
            /* case 0x11: */
            /*   n = s->Channels/2; */
            /*   if (upper) { */
            /*     swapbands(&(s->data[0]), &(s->data[n]), n); */
            /*   } */
            /*  for (i = n; i < 2*n; i++) s->data[i] = 0.0; */
            /*  break; */
          case 0x55:
          case 0xff:
            n = s->Channels/2;
            swapbands(&(s->data[n/2]), &(s->data[n]), n/2);
            if (upper) {
                swapbands(&(s->data[0]), &(s->data[n]), n);
            }
            for (i = n; i < 2*n; i++) s->data[i] = 0.0;
            break;
          default:
            ODINwarning("can't split mode %02X", mode);
            return 0;
        }
        s->IntMode |= ADC_SPLIT;
        s->Channels /= 2;
    } else {
        mode = s->IntMode & 0x0f;
        switch (mode) {
          case AC_MEDRES:
          case AC_LOWRES:
            n = s->Channels/2;
            if (upper) {
                s->IntMode |= AC_UPPER;
                for (i = 0; i < n; i++) {
                    s->data[i] = s->data[i+n];
                    s->data[i+n] = 0.0;
                }
            } else {
                for (i = n; i < 2*n; i++) {
                    s->data[i] = 0.0;
                }
            }
            break;
          default:
            ODINwarning("can't split mode %02X", mode);
            return 0;
        }
        s->IntMode |= AC_SPLIT;
        s->Channels /= 2;
    }

    return 1;
}

/*
 *  For correlator spectra, this routine will return a spectrum where all
 *  channels not belonging to the selected band have been dropped. This is
 *  meant to aid in reducing data one band at a time. The spectrum must
 *  not have been frequency sorted, yet. 
 *  Returns 1 on success, 0 on failure.
 */
int getband(Scan *s, int n, int dsb)
{
    int adc, j, k, c1, c2, m, *seq;
    double df, IFfreq;

    if ((s->Backend != AC1) && (s->Backend != AC2)) return 0;

    /* If spectrum already sorted we no longer allow extracting */
    if (s->Quality & ISORTED)                       return 0;

    /* If spectrum has been split we no longer allow extracting */
    if (s->IntMode & ADC_SPLIT)                     return 0;

    /* For the tradional IntModes, try converting to new mode */
    if (!(s->IntMode & ADC_SEQ)) {
        if (newMode(s) == 0) return 0;
    }
    seq = GetACSequence(s->IntMode);

    if (dsb) n = 2*n-1;
    m = 0;
    c1 = c2 = 0;
    for (adc = 0; adc < 8; adc++) {
        if (seq[2*adc]) {
            m++;
            k = seq[2*adc]*112;
            c1 = c2;
            c2 = c1+k;
        }
        if (m == n) {
            df = 1.0e6/(double)seq[2*adc];
            if (dsb) {
                for (j = 0; j < 2*k; j++) s->data[j] = s->data[j+c1];
                s->Channels = 2*k;
                /* always return bands in increasing frequency order */
                if (seq[2*adc+1] < 0) {
                    reverse(s->data, k);
                } else {
                    reverse(s->data+k, k);
                    swapbands(s->data, s->data+k, k);
                }
                s->FreqCal[0] = s->FreqCal[adc/2];
            } else {
                for (j = 0; j < k; j++) s->data[j] = s->data[j+c1];
                s->Channels = k;
                /* always return bands in increasing frequency order */
                if (seq[2*adc+1] < 0) {
                    reverse(s->data, k);
                    s->FreqCal[0] = s->FreqCal[adc/2] - df*(CenterCh(s)+1);
                } else {
                    s->FreqCal[0] = s->FreqCal[adc/2] + df*CenterCh(s);
                }
            }
            printf("get band %d (%d), %d channels, df = %e\n", 
                   n, dsb, s->Channels, df);
            s->FreqCal[1] = df;
            s->FreqCal[2] = 0.0;
            s->FreqCal[3] = 0.0;
            IFfreq = s->FreqCal[0];
            if (s->SkyFreq > s->LOFreq) {
                s->SkyFreq = s->LOFreq+IFfreq;
            } else {
                s->SkyFreq = s->LOFreq-IFfreq;
            }
            if (s->Discipline == ASTRO) {
                s->RestFreq = s->SkyFreq/(1.0-(s->VSource+s->Vlsr)/LIGHTSPEED);
            } else {
                s->RestFreq = s->SkyFreq/(1.0-s->Vgeo/LIGHTSPEED);
            }
            s->FreqRes = df;
            if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;
            s->Quality |= ISORTED;
            s->Quality |= ILINEAR;
            return 1;
        }
    }
    return 0;
}

/*
 * this routine will return the frequency scale for all channels in an
 * ODIN spectrum.
 *
 * usage:
 *
 * double f[MAXCHANNELS];
 * struct OdinScan odinscan;
 *
 * if (frequency(&odinscan, f)) {
 *     ...
 * }
 *
 * on return, the array f will be filled with the IF frequency values in Hz 
 * for all channels present in the spectrum. The return value of the function
 * will be the number of channels in the spectrum, or 0 in case of an error.
 */

#define RFCENTER  3900.0e6   /* IF band center in Hz */
#define AOSCENTER 2100.0e6   /* IF band center for AOS in Hz */
#define AC1CENTER 3900.0e6   /* IF band center for AC1 in Hz */
#define AC2CENTER 3900.0e6   /* IF band center for AC2 in Hz */

int frequency(Scan *s, double f[])
{
    int i, j, k, m, n, mode, backend, *seq, adc;
    int upper = 0, split = 0;
    double x, df, *c, *LO;

    n = s->Channels;
    backend = s->Backend;
    mode = s->IntMode;
    /* for the two correlators test for split mode */
    if (backend == AC1 || backend == AC2) {
        if (mode & ADC_SEQ) {
            if (mode & ADC_SPLIT) split = 1;
            /* In split mode test for lower or upper half */ 
            if (split) upper = mode & ADC_UPPER;
        } else {
            if (mode & AC_SPLIT) split = 1;
            /* In split mode test for lower or upper half */ 
            if (split) upper = mode & AC_UPPER;
        }
    }

    /* If spectrum is already sorted in frequency, this is easy.     */
    /* For a sorted spectrum we expect FreqCal to contain polynomial */
    /* coefficients to describe our frequency axis, independent of   */
    /* the spectrometer used. See routine 'freqsort' for where this  */
    /* is set up.                                                    */
    if (s->Quality & ISORTED) {
        c = s->FreqCal;             /* point to frequency coefficients */
        for (i = 0; i < n; i++) {
            x = (double)(i-n/2);
            f[i] = c[0] + (c[1] + (c[2] + c[3]*x)*x)*x;
        }

        return n;
    }

    /* Here we get if we have an unprocessed spectrum */

    /* n expected to be                   */
    /* 1728 for the AOS                   */
    /*  896 for the AC1/AC2               */
    /*  448 for the AC1/AC2 in split mode */
    if (n > MAXCHANNELS || n < 0) return 0;

    /* keep lowest 4 bits only so we can use it in a switch statement below */

    switch (backend) {
      case AC1:
      case AC2:
        LO = s->FreqCal;  /* point to SSB LO frequencies */
        if (s->IntMode & ADC_SEQ) {
            seq = GetACSequence(s->IntMode);
            m = 0;
            for (adc = 0; adc < 8; adc++) {
                if (s->IntMode & ADC_SPLIT) {
                    if (s->IntMode & ADC_UPPER) {
                        if (adc == 0) adc += 2;
                        if (adc == 4) adc += 2;
                    } else {
                        if (adc == 2) adc += 2;
                        if (adc == 6) break;
                    }
                }
                if (seq[2*adc]) {
                    k = seq[2*adc]*112;
                    df = 1.0e6/(double)seq[2*adc];
                    if (seq[2*adc+1] < 0) df = -df;
                    /*
                    printf("newadc: adc[%d] LO[%d] %3d:%3d %10.3f %6.3f\n",
                           adc, adc/2, m, m+k, LO[adc/2], df/1.0e6);
                    */
                    for (j = 0; j < k; j++) 
                        f[m+j] = LO[adc/2] + (double)j*df;
                    m += k;
                }
            }
            /* printf("newadc: m = n ? %d = %d\n", m, n); */
            if (m != n) return 0;
        } else {
            mode &= 0x000f;
            switch (mode) {
              case AC_XHIRES:
              case AC_YHIRES:
                df = 1.0e6/8.0;
                if (split) return 0; /* not possible in XHIRES mode       */
                for (j = 0; j < n; j++) 
                    f[j] = LO[0] + (double)j*df;
                break;
              case AC_HIRES:
                df = 1.0e6/4.0;
                if (split) {
                    if (upper) {
                        for (j = 0; j < n; j++) 
                            f[j] = LO[1] - (double)(n-1-j)*df;
                    } else {
                        for (j = 0; j < n; j++) 
                            f[j] = LO[0] + (double)j*df;
                    }
                } else {
                    for (j = 0; j < n/2; j++) 
                        f[j    ] = LO[0] + (double)j*df;
                    for (j = 0; j < n/2; j++) 
                        f[j+n/2] = LO[1] - (double)(n/2-1-j)*df;
                }
                break;
              case AC_MEDRES:
                df = 1.0e6/2.0;
                if (split) {
                    if (upper) {
                        for (j = 0; j < n/2; j++) 
                            f[j    ] = LO[3] - (double)(n/2-1-j)*df;
                        for (j = 0; j < n/2; j++) 
                            f[j+n/2] = LO[2] + (double)j*df;
                    } else {
                        for (j = 0; j < n/2; j++) 
                            f[j    ] = LO[1] - (double)(n/2-1-j)*df;
                        for (j = 0; j < n/2; j++) 
                            f[j+n/2] = LO[0] + (double)j*df;
                    }
                } else {
                    for (j = 0; j < n/4; j++) 
                        f[j+0*n/4] = LO[1] - (double)(n/4-1-j)*df;
                    for (j = 0; j < n/4; j++) 
                        f[j+1*n/4] = LO[0] + (double)j*df;
                    for (j = 0; j < n/4; j++) 
                        f[j+2*n/4] = LO[3] - (double)(n/4-1-j)*df;
                    for (j = 0; j < n/4; j++) 
                        f[j+3*n/4] = LO[2] + (double)j*df;
                }
                break;
              case AC_LOWRES:
                df = 1.0e6;
                if (split) {
                    if (upper) {
                        for (j = 0; j < n/4; j++) 
                            f[j+0*n/4] = LO[2] - (double)(n/4-1-j)*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+1*n/4] = LO[2] + (double)j*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+2*n/4] = LO[3] - (double)(n/4-1-j)*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+3*n/4] = LO[3] + (double)j*df;
                    } else {
                        for (j = 0; j < n/4; j++) 
                            f[j+0*n/4] = LO[0] - (double)(n/4-1-j)*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+1*n/4] = LO[0] + (double)j*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+2*n/4] = LO[1] - (double)(n/4-1-j)*df;
                        for (j = 0; j < n/4; j++) 
                            f[j+3*n/4] = LO[1] + (double)j*df;
                    }
                } else {
                    for (j = 0; j < n/8; j++) 
                        f[j+0*n/8] = LO[0] - (double)(n/8-1-j)*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+1*n/8] = LO[0] + (double)j*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+2*n/8] = LO[1] - (double)(n/8-1-j)*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+3*n/8] = LO[1] + (double)j*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+4*n/8] = LO[2] - (double)(n/8-1-j)*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+5*n/8] = LO[2] + (double)j*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+6*n/8] = LO[3] - (double)(n/8-1-j)*df;
                    for (j = 0; j < n/8; j++) 
                        f[j+7*n/8] = LO[3] + (double)j*df;
                }
                break;
              default:
                return 0;
            }
        }
        break;

      case AOS:
        c = s->FreqCal;  /* point to frequency coefficients */
        switch (mode) {
          case AOS_LONG:
          case AOS_SHORT:
          case AOS_HALF:
          case AOS_FOUR:
          case AOS_CENTRE:
          case AOS_WINGS:
          case AOS_WINDOW:
            for (i = 0; i < n; i++) {
                x = (double)(i-n/2);
                /* typical values for frequency coefficients:
                   c[0] =  2.100e+09
                   c[1] =  6.200e+05
                   c[2] =  4.000e+00
                   c[3] = -1.000e-02
                */
                f[i] = c[0] + (c[1] + (c[2] + c[3]*x)*x)*x;
                f[i] = RFCENTER + (AOSCENTER - f[i]);
            }
            break;
          default:
            return 0;
        }
        break;

      case FBA:
        f[0] = s->FreqCal[0];
        f[1] = s->FreqCal[1];
        f[2] = s->FreqCal[2];
        n = FILTERCHANNELS;
        break;
      default:
        return 0;
    }

    return n;
}

/*
 * this routine will sort frequencies for a spectrum, possibly dropping 
 * channels in overlapping bands for a correlator spectrum.
 *
 * usage:
 *
 * double f[MAXCHANNELS];
 * struct OdinScan odinscan;
 *
 * if (drop(&odinscan, f)) {
 *     ...
 * }
 *
 * f is an array of frequencies to be used by routine 'frequency' above.
 * The function will return the new number of channels or 0 in case of
 * an error.
 *
 * This routine may be used as an alternative to freqsort below.
 */

int drop(Scan *s, double *f)
{
    int m, n, i, j, jm;
    int backend, mode, nbands, swapped, dead[8];
    double fmin, fmax, df, IFfreq;
    float *dp, dswap;
    double *fp, fswap;

    /* call the frequency routine first */
    n = s->Channels;
    frequency(s, f);
    /* for (j = 0; j < n; j++) printf("%15.7e %15.7e\n", f[j], s->data[j]); */

    backend = s->Backend;

    /* nothing to be done for FBA */
    if (backend == FBA) return s->Channels;
  
    /* check if this is already done */
    if (s->Quality & ISORTED) return s->Channels;

    if (backend == AOS) {
        /* AOS may need to be reversed */
        if (f[0] > f[n-1]) {
            for (i = 0; i < n/2; i++) {
                dswap = s->data[i];
                s->data[i] = s->data[n-1-i];
                s->data[n-1-i] = dswap;
                fswap = f[i];
                f[i] = f[n-1-i];
                f[n-1-i] = fswap;
            }
            s->FreqCal[0] = RFCENTER + AOSCENTER - s->FreqCal[0];
            s->FreqCal[2] = -s->FreqCal[2];
            df = s->FreqCal[1];
            s->FreqRes = df;
            if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;
        }
        s->Quality |= ISORTED;
        return n;
    }

    mode = s->IntMode;
    if (s->IntMode & ADC_SEQ) {
        /* new mode descriptor */
        mode &= 0x00ff;
        switch (mode) {
          case 0x01:
            df = 1.0e6/8.0;
            nbands = 1;
            break;
          case 0x11:
            df = 1.0e6/4.0;
            nbands = 2;
            break;
          case 0x55:
            df = 1.0e6/2.0;
            nbands = 4;
            break;
          case 0xff:
            df = 1.0e6;
            nbands = 8;
            break;
          default:
            /* here we get for modes which use different number of
               chips per band */
            ODINwarning("not yet implemented");
            return 0;
        }
        if (s->IntMode & ADC_SPLIT) nbands /= 2;
    } else {
        /* old mode descriptor */
        mode &= 0x000f;

        switch (mode) {
          case AC_XHIRES:
          case AC_YHIRES:
            df = 1.0e6/8.0;
            nbands = 1;
            break;
          case AC_HIRES:
            df = 1.0e6/4.0;
            nbands = 2;
            break;
          case AC_MEDRES:
            df = 1.0e6/2.0;
            nbands = 4;
            break;
          case AC_LOWRES:
            df = 1.0e6;
            nbands = 8;
            break;
        }
        if (s->IntMode & AC_SPLIT) nbands /= 2;
    }

    m = n/nbands;

    /* turn inverted bands around */
    /* this may be needed for new mode descriptions */
    for (j = 0; j < nbands; j++) {
        if (f[j*m] > f[(j+1)*m-1]) {
            /* ODINwarning("turning band %d (%6.1f-%6.1f) around\n", 
                        j+1, f[j*m]/1.0e6, f[(j+1)*m-1]/1.0e6); */
            dp = &s->data[j*m];
            fp = &f[j*m];
            for (i = 0; i < m/2; i++) {
                dswap = dp[i];
                dp[i] = dp[m-1-i];
                dp[m-1-i] = dswap;
                fswap = fp[i];
                fp[i] = fp[m-1-i];
                fp[m-1-i] = fswap;
            }
        }
    }

    if (nbands > 1) {
        /* bring bands into frequency order */
        do {
            swapped = 0;
            for (j = 0; j < nbands-1; j++) {
                if (f[j*m] > f[(j+1)*m]) {
                    dp = &s->data[j*m];
                    fp = &f[j*m];
                    for (i = 0; i < m; i++) {
                        dswap = dp[i];
                        dp[i] = dp[i+m];
                        dp[i+m] = dswap;
                        fswap = fp[i];
                        fp[i] = fp[i+m];
                        fp[i+m] = fswap;
                    }
                    swapped = 1;
                }
            }
        } while (swapped);

        /* check for dead bands */
        for (j = 0; j < nbands; j++) {
            dead[j] = 1;
            for (i = 0; i < m; i++) {
                if (s->data[j*m+i] != 0.0) {
                    dead[j] = 0;
                    break;
                }
            }
        }

        /*      for (j = 0; j < nbands; j++) { */
        /*        if (dead[j]) { */
        /*      printf("band %d is dead\n", j); */
        /* if there are any more bands after the dead one, move them. */
        /*      if (j < nbands-1) { */
        /*        for (i = 0; i < (nbands-1-j)*m; i++) { */
        /*          k = j*m+i; */
        /*          s->data[k] = s->data[k+m]; */
        /*          f[k] = f[k+m]; */
        /*        } */
        /*      } */
        /* adjust new number of channels and bands */
        /*      n -= m; */
        /*      nbands--; */
        /*      j--; */
        /*      printf("number of bands now %d (%d)\n", nbands, n); */
        /*        } */
        /*      } */

        for (j = 1; j < nbands; j++) {
            if (dead[j-1] ^ dead[j]) {
                if (dead[j-1]) {
                    jm = (j-1)*m;
                    for (i = 0; i < m; i++) f[jm+i] = 0.0;
                } else if (dead[j]) {
                    jm = j*m;
                    for (i = 0; i < m; i++) f[jm+i] = 0.0;
                }
            } else {
                jm = j*m;
                /* fmin is beginning of band j, fmax is end of band j-1 */
                fmin = f[jm];
                fmax = f[jm-1];
                /* if (fmin-fmax > df) { */
                /*     ODINwarning("non-contiguous bands"); */
                /* } */
                i = 0;
                while (fmax >= fmin) {
                    f[jm+i] = 0.0;
                    fmin = f[jm+i+1];
                    if (fmax >= fmin) {
                        f[jm-i-1] = 0.0;
                        fmax = f[jm-i-2];
                        i++;
                    }
                }
            }
        }
    
        j = 0;
        while (j < n) {
            if (f[j] == 0.0) {
                /* remove channels with zero frequencies */
                m = 1;
                while (m+j < n && f[j+m] == 0.0) m++;
                /* ODINwarning("remove %d zero channels", m); */
                for (i = j+m; i < n; i++) {
                    f[i-m] = f[i];
                    s->data[i-m] = s->data[i];
                }
                n -= m;
            } 
            j++;
        } 

        j = 1;
        while (j < n) {
            if (f[j]-f[j-1] > df) {
                /* fill in gaps with zero data channels */
                m = 1;
                while (f[j]-f[j-1] > df*(m+1)) m++;
                /* ODINwarning("gap of %d channels", m); */
                if (n+m > MAXCHANNELS) {
                    ODINwarning("maximum number of channels exceeded");
                    return 0;
                }
                for (i = n-1; i >= j; i--) {
                    f[i+m] = f[i];
                    s->data[i+m] = s->data[i];
                }
                n += m;
                for (i = 0; i < m; i++) {
                    f[j] = f[j-1]+df;
                    s->data[j] = 0.0;
                    j++;
                }
            } else {
                j++;
            }
        }
    }

    s->Channels = n;
    s->FreqCal[0] = f[0] + df*CenterCh(s);
    s->FreqCal[1] = df;
    s->FreqCal[2] = 0.0;
    s->FreqCal[3] = 0.0;
    IFfreq = f[n/2];
    if (s->SkyFreq > s->LOFreq) {
        s->SkyFreq = s->LOFreq+IFfreq;
        s->MaxSuppression  = s->SkyFreq - 2.0*RFCENTER;
    } else {
        s->SkyFreq = s->LOFreq-IFfreq;
        s->MaxSuppression  = s->SkyFreq + 2.0*RFCENTER;
    }
    if (s->Discipline == ASTRO) {
        s->RestFreq = s->SkyFreq/(1.0-(s->VSource+s->Vlsr)/LIGHTSPEED);
    } else {
        s->RestFreq = s->SkyFreq/(1.0-s->Vgeo/LIGHTSPEED);
    }
    s->FreqRes = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->Quality |= ISORTED;
    s->Quality |= ILINEAR;

    return n;
}

/* The following structure and function are used by 'freqsort' below */
static struct Xaxis {
    int index;
    int weight;
    double freq;
} xaxis[MAXCHANNELS];

static float data[MAXCHANNELS];

int freqcmp(struct Xaxis *one, struct Xaxis *two)
{
    if (one->freq < two->freq) return -1;
    if (one->freq > two->freq) return  1;
    else                       return  0;
}

/*
 * this routine will turn a spectrum into one contiguous band,
 * dropping overlapping frequencies and possibly filling in dummy values
 * at intermediate frequencies which weren't covered.
 *
 * usage:
 *
 * double f[MAXCHANNELS];
 * struct OdinScan odinscan;
 *
 * if (freqsort(&odinscan, f)) {
 *     ...
 * }
 *
 * 'freqsort' will call routine 'frequency' first.
 * The function will return the (probably new) number of channels 
 * or 0 in case of an error.
 */
int freqsort(Scan *s, double *f)
{
    int adc, m, n, i, j, k, *seq;
    int backend, mode, split = 0, upper = 0;
    double ip, frac;
    double fmin, fmax, df;
    float swap;

    /* Call the frequency routine first. */
    frequency(s, f);

    /* nothing to be done for FBA */
    if (backend == FBA) return s->Channels;
  
    /* check if this is already done */
    if (s->Quality & ISORTED) return s->Channels;

    df = 0.0;
    n = s->Channels;
    backend = s->Backend;

    if (backend == AOS) {
        /* AOS may need to be reversed */
        if (f[0] > f[n-1]) {
            for (i = 0; i < n/2; i++) {
                swap = s->data[i];
                s->data[i] = s->data[n-1-i];
                s->data[n-1-i] = swap;
                ip = f[i];
                f[i] = f[n-1-i];
                f[n-1-i] = ip;
            }
            s->FreqCal[0] = RFCENTER + AOSCENTER - s->FreqCal[0];
            s->FreqCal[2] = -s->FreqCal[2];
        }
        s->Quality |= ISORTED;
        return n;
    }

    /* nothing to be done for FBA */
    if (backend == FBA) return n;

    mode = s->IntMode;
    if (mode & ADC_SEQ) {
        /* for the two correlators test for split mode */
        if (mode & ADC_SPLIT) split = 1;
        /* In split mode test for lower or upper half */ 
        if (split) upper = mode & ADC_UPPER;
        /* keep lowest 4 bits only for the switch statement below */
        mode &= 0x000f;
    } else {
        /* for the two correlators test for split mode */
        if (mode & AC_SPLIT) split = 1;
        /* In split mode test for lower or upper half */ 
        if (split) upper = mode & AC_UPPER;
        /* keep lowest 4 bits only for the switch statement below */
        mode &= 0x000f;
    }

    for (i = 0; i < n; i++) {
        xaxis[i].index = i;
        xaxis[i].freq = f[i];
    }

    /* 
       We set up a weight for each channel which will allow us to decide
       which channels to keep from two bands which overlap.
       The weights as a function of channel number look like this: /\/\/\/\
       with one ramp (/ or \) per band.
       The weights have their highest values close to the SSB local oscillators.
    */
    if (s->IntMode & ADC_SEQ) {
        if (s->IntMode & ADC_SPLIT) {
            ODINwarning("not yet implemented");
            return n;
        }
        seq = GetACSequence(s->IntMode);
        m = 0;
        for (adc = 0; adc < 8; adc++) {
            if (seq[2*adc]) {
                k = seq[2*adc]*112;
                df = 1.0e6/(double)seq[2*adc];
                for (j = 0; j < k; j++) xaxis[m+j].weight = k-j;
                m += k;
            }
        }
        if (m != n) {
            ODINwarning("number of channel mismatch");
            return 0;
        }
    } else {
        switch (mode) {
          case AC_XHIRES:
          case AC_YHIRES:
            df = 1.0e6/8.0;
            for (i = 0; i < n; i++) {
                xaxis[i].weight = n-i;
            }
            break;
          case AC_HIRES:
            df = 1.0e6/4.0;
            if (split) {
                for (i = 0; i < n; i++) {
                    xaxis[i].weight     = n-i;
                }
            } else {
                for (i = 0; i < n/2; i++) {
                    xaxis[i].weight     = n/2-i;
                    xaxis[n-1-i].weight = n/2-i;
                }
            }
            break;
          case AC_MEDRES:
            df = 1.0e6/2.0;
            if (split) {
                for (i = 0; i < n/2; i++) {
                    xaxis[i].weight     = i;
                    xaxis[n-1-i].weight = i;
                }
            } else {
                for (i = 0; i < n/4; i++) {
                    xaxis[1*n/4-1-i].weight = n/4-i;
                    xaxis[1*n/4+i].weight   = n/4-i;
                    xaxis[3*n/4-1-i].weight = n/4-i;
                    xaxis[3*n/4+i].weight   = n/4-i;
                }
            }
            break;
          case AC_LOWRES:
            df = 1.0e6;
            if (split) {
                for (i = 0; i < n/4; i++) {
                    xaxis[1*n/4-1-i].weight = n/4-i;
                    xaxis[1*n/4+i].weight   = n/4-i;
                    xaxis[3*n/4-1-i].weight = n/4-i;
                    xaxis[3*n/4+i].weight   = n/4-i;
                }
            } else {
                for (i = 0; i < n/8; i++) {
                    xaxis[1*n/8-1-i].weight = n/8-i;
                    xaxis[1*n/8+i].weight   = n/8-i;
                    xaxis[3*n/8-1-i].weight = n/8-i;
                    xaxis[3*n/8+i].weight   = n/8-i;
                    xaxis[5*n/8-1-i].weight = n/8-i;
                    xaxis[5*n/8+i].weight   = n/8-i;
                    xaxis[7*n/8-1-i].weight = n/8-i;
                    xaxis[7*n/8+i].weight   = n/8-i;
                }
            }
            break;
        }
    }
    qsort(xaxis, n, sizeof(struct Xaxis), (void *)freqcmp);

    /* frequencies are now sorted in structure xaxis, member freq */
    fmin = xaxis[0].freq;
    fmax = xaxis[n-1].freq;

    /* the frequency range needs to be divisible by the resolution */
    if (df == 0.0) return 0;
    frac = modf((double)((fmax - fmin)/df), &ip);
    if (frac != 0.0) return 0;

    m = (int)ip+1;
    if (m > MAXCHANNELS) {
        ODINwarning("maximum number of channels exceeded");
        return 0;
    }

    j = 0;
    for (i = 0; i < m; i++) {
        /* generate and look up next frequency */
        f[i] = fmin + df*i;
        /* do we already have the right index j? */
        if (f[i] != xaxis[j].freq) {
            /* search for the frequency in the array */
            for (j = 0; j < n; j++) {
                if (xaxis[j].freq == f[i]) break;
            }
        }
        /* if frequency not found, set channel to zero */
        if (j == n) data[i] = 0.0;
        else {
            /* the index has the data channel */
            k = xaxis[j].index;
            /* do we have a frequency covered by two bands? */
            if (f[i] == xaxis[j+1].freq) {
                /* take the channel with the higher weight */
                if (xaxis[j+1].weight > xaxis[j].weight) {
                    k = xaxis[j+1].index;
                }
            }
            data[i] = s->data[k];
            j++;
        }
    }

    for (i = 0; i < m; i++) s->data[i] = data[i];

    s->Channels = m;
    s->FreqCal[0] = fmin + df*CenterCh(s);
    s->FreqCal[1] = df;
    s->FreqCal[2] = 0.0;
    s->FreqCal[3] = 0.0;

    s->FreqRes = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->Quality |= ISORTED;
    s->Quality |= ILINEAR;
    return m;
}

/* 
 * A vector big enough to hold channel values for one spectrum.
 * It will be used by routines below this point.
 */
static float u[MAXCHANNELS];

/*
 * Resample a spectrum onto a new grid of frequencies.
 * May be used to transform an AOS spectrum to a linear frequency scale.
 */
void redres(Scan *s, double *f, double df)
{
    int i, k, n;
    float p, q, sig, a, b, h, rf;

    n = Channels(s);
    data[0] = u[0] = 0.0;

    if (!freqsort(s, f)) {
        ODINwarning("can't sort frequencies");
        return;
    }
    for (i = 1; i <= n-2; i++) {
        sig = (f[i]-f[i-1])/(f[i+1]-f[i-1]);
        p = sig*data[i-1]+2.0;
        data[i] = (sig-1.0)/p;
        u[i] = (s->data[i+1]-s->data[i])/(f[i+1]-f[i]) 
            - (s->data[i]-s->data[i-1])/(f[i]-f[i-1]);
        u[i] = (6.0*u[i]/(f[i+1]-f[i-1])-sig*u[i-1])/p;
    }

    data[n-1] = 0.0;

    for (i = n-2; i >= 0; i--) data[i] = data[i]*data[i+1]+u[i];

    for (i = 0; i < n; i++) u[i] = s->data[i];
  
    rf = f[CenterCh(s)];
    k = 0;
    for (i = 0; i < n; i++) {
        p = rf + (i - CenterCh(s))*df;
        if ((p < f[0]) || (p > f[n-1])) {
            s->data[i] = 0.0;
            continue;
        }
        q = f[k];
        while (p > f[k+1]) k++;
    
        h = f[k+1] - f[k];
        a = (f[k+1] - p)/h;
        b = (p - f[k])/h;
        s->data[i] = a*u[k]+b*u[k+1]
            + ((a*a*a-a)*data[k] + (b*b*b-b)*data[k+1])*(h*h)/6.0;
    }
    s->FreqRes = s->FreqCal[1] = df;
    if (s->SkyFreq < s->LOFreq) s->FreqRes = -df;

    s->FreqCal[2] = s->FreqCal[3] = 0.0;
    s->Quality |= ILINEAR;
}

static void adjust(float d[], double f[], double df, 
                   int b1, int b2, int n, int nbands)
{
    int j, k, m, i, i1, i2, order, nmin = 20;
    float bias;
    double f1min, f2min, f1max, f2max;
    static double X, Y, S, SX, SY, SXX, SXY, c1[2], c2[2], D;

    m = n/nbands;

    /* force band b1 (bad) to line up with band b2 (good) */
    f1min = f[b1*m];
    f1max = f[(b1+1)*m-1];
    f2min = f[b2*m];
    f2max = f[(b2+1)*m-1];

    /* printf("band %d from %f to %f\n", b1, f1min, f1max); */
    /* printf("band %d from %f to %f\n", b2, f2min, f2max); */

    order = 2;

    if ((f1min >= f2min) && (f1min <= f2max)) {
        n = (int)((f2max-f1min)/df);
        if (n < nmin) n = nmin;
        for (j = 0; j < order; j++) {
            c1[j] = 0.0;
        }
        S = SX = SY = SXX = SXY = 0.0;
        for (k = 0; k < n; k++) {
            i1 = b1*m+k;
            X = (f[i1]-f2max)/1.0e6;
            Y = (double)d[i1];
            S += 1.0;
            SX += X;
            SY += Y;
            SXX += X*X;
            SXY += X*Y;
        }

        D = S*SXX-SX*SX;
        c1[0] = (SXX*SY-SX*SXY)/D;
        c1[1] = (S*SXY-SX*SY)/D;
        /* printf("coeff(1) = %f %f\n", c1[0], c1[1]); */

        for (j = 0; j < order; j++) {
            c2[j] = 0.0;
        }
        S = SX = SY = SXX = SXY = 0.0;
        for (k = 0; k < n; k++) {
            i2 = (b2+1)*m-n+k;
            X = (f[i2]-f2max)/1.0e6;
            Y = (double)d[i2];
            S += 1.0;
            SX += X;
            SY += Y;
            SXX += X*X;
            SXY += X*Y;
        }

        D = S*SXX-SX*SX;
        c2[0] = (SXX*SY-SX*SXY)/D;
        c2[1] = (S*SXY-SX*SY)/D;
        /* printf("coeff(2) = %f %f\n", c2[0], c2[1]); */

        bias = 0.0;
        for (k = 0; k < n; k++) {
            i1 = b1*m+k;
            X = (f[i1]-f2max)/1.0e6;
            bias += c1[0]-c2[0]+(c1[1]-c2[1])*X;
        }
        bias /= (float)n;
        printf("adjust: overlap of band %d and %d: %d channels (%f)\n", 
               b1, b2, n, bias);
        for (i = b1*m; i < (b1+1)*m; i++) d[i] -= bias;
    } else if ((f1max >= f2min) && (f1max <= f2max)) {
        n = (int)((f1max-f2min)/df);
        if (n < nmin) n = nmin;
        for (j = 0; j < order; j++) {
            c1[j] = 0.0;
        }
        S = SX = SY = SXX = SXY = 0.0;
        for (k = 0; k < n; k++) {
            i1 = (b1+1)*m-n+k;
            X = (f[i1]-f2min)/1.0e6;
            Y = (double)d[i1];
            S += 1.0;
            SX += X;
            SY += Y;
            SXX += X*X;
            SXY += X*Y;
        }

        D = S*SXX-SX*SX;
        c1[0] = (SXX*SY-SX*SXY)/D;
        c1[1] = (S*SXY-SX*SY)/D;
        /* printf("coeff(1) = %f %f\n", c1[0], c1[1]); */

        for (j = 0; j < order; j++) {
            c2[j] = 0.0;
        }
        S = SX = SY = SXX = SXY = 0.0;
        for (k = 0; k < n; k++) {
            i2 = b2*m+k;
            X = (f[i2]-f2min)/1.0e6;
            Y = (double)d[i2];
            S += 1.0;
            SX += X;
            SY += Y;
            SXX += X*X;
            SXY += X*Y;
        }

        D = S*SXX-SX*SX;
        c2[0] = (SXX*SY-SX*SXY)/D;
        c2[1] = (S*SXY-SX*SY)/D;

        bias = 0.0;
        for (k = 0; k < n; k++) {
            i1 = (b1+1)*m-n+k;
            X = (f[i1]-f2min)/1.0e6;
            bias += c1[0]-c2[0]+(c1[1]-c2[1])*X;
        }
        bias /= (float)n;
        printf("adjust: overlap of band %d and %d: %d channels (%f)\n", b1, b2, n, bias);
        for (i = b1*m; i < (b1+1)*m; i++) d[i] -= bias;
    }
}

int fixband(Scan *s, double f[])
{
    int m;
    int mode, nbands;
    double df;

    /* We only deal with correlator spectra here. */
    if (s->Backend != AC1 && s->Backend != AC2) {
        ODINwarning("can't fix bands for backend %s", Backend(s));
        return 0;
    }

    /* Skip spectra which are already frequency sorted. */
    if (s->Quality & ISORTED) {
        ODINwarning("can't fix bands for frequency sorted data");
        return 0;
    }

    /* Only accept spectra before splitting */
    if (s->IntMode & AC_SPLIT) {
        ODINwarning("can't fix bands after splitting");
        return 0;
    }

    /* Determine number of bands for spectrometer mode. */
    mode = (s->IntMode & 0x000f);
    switch (mode) {
      case AC_XHIRES:
      case AC_YHIRES:
        nbands = 1;
        break;
      case AC_HIRES:
        nbands = 2;
        break;
      case AC_MEDRES:
        nbands = 4;
        break;
      case AC_LOWRES:
        nbands = 8;
        break;
    }

    /* For 8 band mode we treat two sidebands as one large band. */
    if (nbands == 8) nbands /= 2;

    m = s->Channels/nbands;
    frequency(s, f);
    df = fabs(s->FreqRes);
    if (mode == AC_MEDRES) {
        adjust(s->data, f, df, 1, 0, s->Channels, nbands);
        adjust(s->data, f, df, 3, 2, s->Channels, nbands);
        adjust(s->data, f, df, 0, 1, s->Channels, nbands/2);
    } else {
        adjust(s->data, f, df, 0, 1, s->Channels, nbands);
        adjust(s->data, f, df, 3, 2, s->Channels, nbands);
        adjust(s->data, f, df, 3, 0, s->Channels, nbands);
    }

    s->Quality |= WBANDADJUST;
    return 1;
}
