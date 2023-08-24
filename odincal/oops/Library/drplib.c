#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "odinlib.h"

#include "drplib.h"
#include "accessor.h"
#include "recipes.h"

#define CALIBRATION
#include "smrcdb.h"

static void drpqsort(float a[], int n)
{
    float swap;
    int i;
    int sorted = 0;
    
    while (!sorted) {
        sorted = 1;
        for (i = 1; i < n; i++) {
            if (a[i] < a[i-1]) {
                swap = a[i];
                a[i] = a[i-1];
                a[i-1] = swap;
                sorted = 0;
            }
        }
    }
}

static void drpsort(int w[], int n)
{
    int swap, i, j;

    for (i = n-1; i > 0; i--) {
        for (j = 0; j < i ; j++) {
            if (w[j] > w[j+1]) {
                swap = w[j+1];
                w[j+1] = w[j];
                w[j] = swap;
            }
        }
    }
}

static void drpcheck(int *i1, int *i2, int n)
{
    int swap;

    if (*i1 < 0) *i1 = 0;
    if (*i2 < 0) *i2 = 0;
    if (*i1 > n) *i1 = n;
    if (*i2 > n) *i2 = n;
    if (*i2 < *i1) {
        swap = *i1;
        *i1 = *i2;
        *i2 = swap;
    }
}

static int drpint(double x)
{
    return (int)floor(x+0.5);
}

void Noise(Scan *s)
{
    int i, n, idum;

    idum = (int)time(NULL);
    idum = -idum;
    n = Channels(s);
    for (i = 0; i < n; i++) {
        s->data[i] = gasdev(&idum);
    }
}

int CenterCh(Scan *s)
{
    return (Channels(s)-1)/2;
}
  
#define LIGHTSPEED 2.997924562e8   /* c in meter per second      */

double VelRes(Scan *s)
{
    return -(FreqRes(s)/RestFreq(s)) * LIGHTSPEED;
}

double Bandwidth(Scan *s)
{
    return (FreqRes(s)*Channels(s));
}
  
double Velocity(Scan *s, int ch)
{
    double Vel;

    /*    if (ch < 0)             ch = 0; */
    /*    if (ch > Channels(s)-1) ch = Channels(s)-1; */
    Vel = (double)(ch-CenterCh(s))*VelRes(s)+VSource(s);
    return (Vel);
}
  
double Frequency(Scan *s, int ch)
{
    double Freq;
  
    /*    if (ch < 0)             ch = 0; */
    /*    if (ch > Channels(s)-1) ch = Channels(s)-1; */
    Freq = (double)(ch-CenterCh(s))*FreqRes(s)+RestFreq(s);
    return (Freq);
}
  
int FChannel(Scan *s, double freq)
{
    int ch, n;
  
    n = Channels(s);
    ch = CenterCh(s)+drpint((freq-RestFreq(s))/FreqRes(s));
    /*    if (ch < 0) ch = 0; */
    /*    if (ch > n-1) ch = n-1; */
    if ((ch < 0) || (ch >= n)) ch = -1;
    return (ch);
}
  
int VChannel(Scan *s, double vel)
{
    int ch, n;
  
    n = Channels(s);
    ch = CenterCh(s)+drpint((vel-VSource(s))/VelRes(s));
    /*    if (ch < 0) ch = 0; */
    /*    if (ch > n-1) ch = n-1; */
    if ((ch < 0) || (ch >= n)) ch = -1;
    return (ch);
}

int compatible(Scan *one, Scan *two)
{
    /* frequencies should agree within 1 kHz */
    static const double EPS = 1.0e3;

    /* we need positive effective integration times */
    /*    if (one->EffTime <= 0.0)                             return 0; */
    /*    if (two->EffTime <= 0.0)                             return 0; */

    /* backends should be the same */
    if (one->Backend  != two->Backend) {
        ODINwarning("incompatible backends");
        return 0;
    }

    /* frontends should be the same */
    if (one->Frontend != two->Frontend) {
        ODINwarning("incompatible frontends");
        return 0;
    }

    /* integration modes should agree */
    if (one->IntMode  != two->IntMode) {
        ODINwarning("incompatible modes");
        return 0;
    }

    /* number of channels should agree */
    /* if (one->Channels  != two->Channels) {
        ODINwarning("incompatible number of channels");
        return 0;
    } */

    /* same frequency resolution ? */
    /* if (fabs(one->FreqRes - two->FreqRes) > EPS) { */
    /*   ODINwarning("incompatible frequencies"); */
    /*   return 0; */
    /* } */

    /* if (fabs((double)(one->Longitude - two->Longitude)) > EPS) return 0; */
    /* if (fabs((double)(one->Latitude  - two->Latitude )) > EPS) return 0; */
    return 1;
}

int align(Scan *one, Scan *two)
{
    double f1, f2, f3, f4, swap;
    int i1, i2, i3, i4, n1, n2, m;

    if (!compatible(one, two)) return 0;

    /* make sure both spectra are frequency sorted */
    if (!(one->Quality & ISORTED)) {
        ODINwarning("spectrum 1 not frequency sorted");
        return 0;
    }
    if (!(two->Quality & ISORTED)) {
        ODINwarning("spectrum 2 not frequency sorted");
        return 0;
    }

    /* We only align spectra with linear frequency scale */
    if (!(one->Quality & ILINEAR)) {
        ODINwarning("spectrum 1 not linearised");
        return 0;
    }
    if (!(two->Quality & ILINEAR)) {
        ODINwarning("spectrum 2 not linearised");
        return 0;
    }
  
    n1 = Channels(one)-1;
    n2 = Channels(two)-1;
    f1 = Frequency(one, 0);
    f2 = Frequency(one, n1);
    f3 = Frequency(two, 0);
    f4 = Frequency(two, n2);
  
    if (f1 > f2) {
        swap = f1;
        f1 = f2;
        f2 = swap;
    }
    if (f3 > f4) {
        swap = f3;
        f3 = f4;
        f4 = swap;
    }

    if (f1 < f3) f1 = f3;
    if (f2 > f4) f2 = f4;

    i1 = FChannel(one, f1);
    i2 = FChannel(one, f2);
    i3 = FChannel(two, f1);
    i4 = FChannel(two, f2);

    if (i1 == i2) {
        ODINwarning("no channels found for spectrum 1");
        return 0;
    }
    if (i3 == i4) {
        ODINwarning("no channels found for spectrum 2");
        return 0;
    }

    if (i1 > i2) {
        m = i1;
        i1 = i2;
        i2 = m;
    }
    if (i3 > i4) {
        m = i3;
        i3 = i4;
        i4 = m;
    }

    if (i2-i1 != i4-i3) {
        ODINwarning("channel range mismatch");
        return 0;
    }

    if ((i1 > 0) || (i2 < n1)) {
        Drop(one, i1, i2);
    }
    if ((i3 > 0) || (i4 < n1)) {
        Drop(two, i3, i4);
    }

    return 1;
}

void Accum(Scan *ave, Scan *s)
{
    double t1, t2, total, T1, T2, w1, w2, wtotal;
    int i;

    if (IntTime(ave) == 0.0) {
        memcpy(ave, s, sizeof(Scan));
        return;
    }

    if (!align(ave, s)) {
        ODINwarning("failed to align");
        return;
    }

    t1 = IntTime(ave);
    t2 = IntTime(s);
    if (t2 == 0.0 || t1 == 0.0) return;
    total = t1 + t2;

    T1 = Tsys(ave);
    T2 = Tsys(s);
    if (T2 == 0.0 || T1 == 0.0) {
        T1 = T2 = 1.0;
    }

    w1 = t1/(T1*T1);
    w2 = t2/(T2*T2);

    wtotal = w2+w1;
    for (i = 0; i < Channels(s); i++) {
        ave->data[i] = (ave->data[i]*w1 + s->data[i]*w2)/wtotal;
    }
    setIntTime(ave, total);
    setTsys(ave, sqrt(total/wtotal));
    setEffTime(ave, EffTime(ave)+EffTime(s));
}

void Add(Scan *s1, Scan *s2)
{
    int i;

    for (i = 0; i < Channels(s1); i++) {
        s1->data[i] += s2->data[i];
    }
}

void Subtract(Scan *s1, Scan *s2)
{
    int i;

    for (i = 0; i < Channels(s1); i++) {
        s1->data[i] -= s2->data[i];
    }
}

void Multiply(Scan *s1, Scan *s2)
{
    int i;

    for (i = 0; i < Channels(s1); i++) {
        s1->data[i] *= s2->data[i];
    }
}

void Divide(Scan *s1, Scan *s2)
{
    int i;

    for (i = 0; i < Channels(s1); i++) {
        if (s2->data[i] != 0.0) s1->data[i] /= s2->data[i];
        else                    s1->data[i] = 0.0;
    }
}

extern double planck(double, double);

void CalcTsys(Scan *hot, Scan *cold, double Th, double Tc)
{
    int i;
    double y, Tr, f;

    if (!compatible(hot, cold)) {
        ODINwarning("HOT and COLD incompatible");
        return;
    }

    f = SkyFreq(hot);
    if (f != 0.0) {
        Th = etaMT*planck(Th, f)+(1.0-etaMT)*Tspill;
        Tc = etaMS*planck(Tc, f)+(1.0-etaMS)*Tspill;
    }

    for (i = 0; i < hot->Channels; i++) {
        if (cold->data[i] == 0.0) {
            hot->data[i] = 0.0;
        } else {
            y = (double)(hot->data[i]/cold->data[i]);
            if (y > 1.0) {
                Tr = (Th - y*Tc)/(y - 1.0);
                hot->data[i] = Tr;
            } else {
                hot->data[i] = 0.0;
            }
        }
    }
}

void Gain(Scan *cal, Scan *ref)
{
    int i;
    double f, Tbg, Thot, dT;

    if (!compatible(cal, ref)) {
        ODINwarning("CAL and REF incompatible");
        return;
    }
    f = SkyFreq(cal);
    Tbg = planck(2.7, f);
    Thot = planck(Tcal(cal), f);
    if (Thot == 0.0) {
        Thot = 275.0;
        ODINwarning("using default calibration temperature");
    }
    dT = epsr*etaMT*Thot - etaMS*Tbg + (etaMS-etaMT)*Tspill;
    for (i = 0; i < cal->Channels; i++) {
        if (ref->data[i] > 0.0) {
            if (cal->data[i] > ref->data[i]) {
                cal->data[i] = ref->data[i]/(cal->data[i]-ref->data[i]);
                cal->data[i] *= dT;
            } else {
                cal->data[i] = 0.0;
            }
        } else {
            cal->data[i] = 0.0;
        }
    }
}

void Calibrate(Scan *sig, Scan *ref, Scan *cal, double eta)
{
    int i, rx;
    double f, Tbg, spill;

    if (!compatible(sig, ref)) {
        ODINwarning("SIG and REF incompatible");
        return;
    }
    if (!compatible(sig, cal)) {
        ODINwarning("SIG and CAL incompatible");
        return;
    }
    if (eta == 0.0) {
        rx = sig->Frontend - 1;
        if ((rx < 0) || (rx > 4)) {
            ODINwarning("invalid receiver");
            return;
        }
        eta = etaML[rx];
    }
    f = SkyFreq(sig);
    Tbg = planck(2.7, f);
    spill = etaMS*Tbg - (etaMS-eta)*Tspill;
    for (i = 0; i < sig->Channels; i++) {
        sig->data[i] -= ref->data[i];
        if (ref->data[i] > 0.0) {
            sig->data[i] /= ref->data[i];
            sig->data[i] *= cal->data[i];
            sig->data[i] += spill;
            sig->data[i] /= eta;
        } else {
            sig->data[i] = 0.0;
        }
    }
    sig->Quality &= ~WAMPLITUDE;
    sig->Type = SPE;
}

double Area(Scan *s, int from, int to)
{
    int i;
    double sum;

    drpcheck(&from, &to, Channels(s));
    for (i = from; i < to; i++) {
        sum += (double)s->data[i];
    }

    return sum;
}

void Bias(Scan *s, double value)
{
    int i;

    for (i = 0; i < Channels(s); i++) {
        s->data[i] += value;
    }
}

void Drop(Scan *s, int from, int to)
{
    int i, n, shift;

    drpcheck(&from, &to, Channels(s));

    for (i = from, n = 0; i < to; i++, n++) {
        s->data[n] = s->data[i];
    }

    shift = (from+to-1)/2 - CenterCh(s);
    setChannels(s, n);

    Shift(s, shift);
    return;
}

double Maximum(Scan *s, int from, int to)
{
    int i;
    double max;

    drpcheck(&from, &to, Channels(s));
    max = (double)s->data[from];
    for (i = from+1; i < to; i++) {
        if (max < (double)s->data[i]) max = (double)s->data[i];
    }

    return max;
}

double Mean(Scan *s, int from, int to)
{
    double *moment;

    moment = Moment(s, from, to);

    return moment[0];
}

static float tmp[MAXCHANNELS];

double Median(Scan *s, int from, int to)
{
    float med;
    int i, j, n;

    drpcheck(&from, &to, Channels(s));
    for (i = from, j; i < to; i++, j++) tmp[j] = s->data[i];
    n = to-from;
    drpqsort(tmp, n);
    if (n % 2) med = tmp[(n-1)/2];
    else       med = (tmp[n/2]+tmp[n/2-1])/2.0;

    return (double)med;
}

double Minimum(Scan *s, int from, int to)
{
    int i;
    double min;

    drpcheck(&from, &to, Channels(s));
    min = s->data[from];
    for (i = from+1; i < to; i++) {
        if (min > (double)s->data[i]) min = (double)s->data[i];
    }

    return min;
}

double *Moment(Scan *s, int from, int to)
{
    static double moment[4];
    double mean, var, rms, skew, kurt, y;
    int i;

    drpcheck(&from, &to, Channels(s));
    mean = 0.0;
    for (i = from; i < to; i++) {
        mean += (double)s->data[i];
    }
    mean /= (double)(to-from);
    moment[0] = mean;

    var = skew = kurt = 0.0;
    for (i = from; i < to; i++) {
        y = (double)s->data[i]-mean;
        var  += y*y;
        skew += y*y*y;
        kurt += y*y*y*y;
    }

    if (to > from) {
        var /= (double)(to-from-1);
        rms  = sqrt(var);
    } else {
        rms = var = 0.0;
    }

    if (var != 0.0) {
        skew /= pow(rms*(to-from), 3.0);
        kurt /= pow(var*(to-from), 2.0)-3.0;
    } else {
        skew = 0.0;
        kurt = 0.0;
    }
    moment[1] = rms;
    moment[2] = skew;
    moment[3] = kurt;

    return moment;
}

double Rms(Scan *s, int from, int to)
{
    double *moment;

    moment = Moment(s, from, to);

    return moment[1];
}

double Skew(Scan *s, int from, int to)
{
    double *moment;

    moment = Moment(s, from, to);

    return moment[2];
}

double Kurtosis(Scan *s, int from, int to)
{
    double *moment;

    moment = Moment(s, from, to);

    return moment[3];
}

void Scale(Scan *s, double value)
{
    int i;

    for (i = 0; i < Channels(s); i++) {
        s->data[i] *= value;
    }
}

void setValue(Scan *s, double value, int from, int to)
{
    int i;

    drpcheck(&from, &to, Channels(s));
    for (i = from; i < to; i++) {
        s->data[i] = value;
    }
}

void setHigh(Scan *s, double value, int from, int to)
{
    int i;

    drpcheck(&from, &to, Channels(s));
    for (i = from; i < to; i++) {
        if (value < (double)s->data[i]) s->data[i] = value;
    }
}

void setLow(Scan *s, double value, int from, int to)
{
    int i;

    drpcheck(&from, &to, Channels(s));
    for (i = from; i < to; i++) {
        if (value > s->data[i]) s->data[i] = value;
    }
}

void Shift(Scan *s, int n)
{
    setRestFreq(s, RestFreq(s) + FreqRes(s) * n);
    setSkyFreq(s, SkyFreq(s) + FreqRes(s) * n);
    setVSource(s, VSource(s) + VelRes(s) * n);
    return;
}

void Fold(Scan *s, double ft, int shift)
{
    int i, n;

    ft = -ft;
    n = (int)(ft/FreqRes(s));
    if (n > 0) {
        for (i=0; i<Channels(s)-n; i++) {
            s->data[i] -= s->data[i+n];
            s->data[i] /= 2.0;
        }
        if (shift) {
            n /= 2;
            for (i=Channels(s)-1; i>=n; i--) {
                s->data[i] = s->data[i-n];
            }
        }
    } else {
        n = -n;
        for (i=Channels(s)-1; i>=n; i--) {
            s->data[i] -= s->data[i-n];
            s->data[i] /= 2.0;
        }
        if (shift) {
            n /= 2;
            for (i=0; i<Channels(s)-n; i++) {
                s->data[i+n] = s->data[i];
            }
        }
    }
}

void Convolve(Scan *s1, Scan *s2)
{
    float Re, Im;
    int i, ii, n;

    n = 2;
    while (n < Channels(s1)) n *= 2;

    for (i = Channels(s1); i < n; i++) {
        s1->data[i] = s2->data[i] = 0.0;
    }

    /* Fourier transform both arrays */
    realft(s1->data, n/2, 1);
    realft(s2->data, n/2, 1);

    /* (complex) multiply arrays */
    for (i = 1; i < n/2; i++) {
        ii = 2*i;
        Re = s1->data[ii]*s2->data[ii] + s1->data[ii+1]*s2->data[ii+1];
        Im = s1->data[ii+1]*s2->data[ii] - s1->data[ii]*s2->data[ii+1];
        s1->data[ii] = Re; s1->data[ii+1] = Im;
    }

    s1->data[0] = s1->data[0]*s2->data[0];
    s1->data[1] = s1->data[1]*s2->data[1];

    /* do inverse transform */
    realft(s1->data, n/2, -1);
}

void Correlate(Scan *s1, Scan *s2)
{
    float Re, Im;
    int i, ii, n;

    n = 2;
    while (n < Channels(s1)) n *= 2;

    for (i = Channels(s1); i < n; i++) {
        s1->data[i] = s2->data[i] = 0.0;
    }

    /* Fourier transform both arrays */
    realft(s1->data, n/2, 1);
    realft(s2->data, n/2, 1);

    /* (complex) multiply arrays */
    for (i = 1; i < n/2; i++) {
        ii = 2*i;
        Re = s1->data[ii]*s2->data[ii] - s1->data[ii+1]*s2->data[ii+1];
        Im = s1->data[ii+1]*s2->data[ii] + s1->data[ii]*s2->data[ii+1];
        s1->data[ii] = Re; s1->data[ii+1] = Im;
    }

    s1->data[0] = s1->data[0]*s2->data[0];
    s1->data[1] = s1->data[1]*s2->data[1];

    /* do inverse transform */
    realft(s1->data, n/2, -1);
}

void Reverse(Scan *s)
{
    float swap;
    int i, n;
  
    n = Channels(s)-1;
    for (i = 0; i < Channels(s)/2; i++) {
        swap = s->data[i];
        s->data[i] = s->data[n-i];
        s->data[n-i] = swap;
    }  
    setFreqRes(s, -FreqRes(s));
}

void Smooth(Scan *s, double red)
{
/*      Scan *t; */
    int i, j, n, l, u, nc;
    double freq, delta, sum, sumw, weight;
    double v, f, Df, df, rf;

    v = Velocity(s, 0);
    f = Frequency(s, 0);
    Df = RestFreq(s) - SkyFreq(s);

    v += VelRes(s)*(red-1.0)/2.0;
    f += FreqRes(s)*(red-1.0)/2.0;
  
/*      t = (Scan *)calloc(1, sizeof(Scan)); */
/*      if (t == (Scan *)NULL) return; */

/*      setRestFreq(t, RestFreq(s)); */
    rf = RestFreq(s);
    df = FreqRes(s)*red;
    n = CenterCh(s);
    nc = (int)(floor((Channels(s)-n-0.5)/red-0.5) + floor((n+0.5)/red-0.5)+1);
    /*      setFreqRes(t, FreqRes(s)*red); */
    n = (int)red;
  
    freq = RestFreq(s)-df*((nc-1)/2);
    for (i = 0; i < nc; i++) {
        l = FChannel(s, freq)-n; u = l+2*n+1;
        if (l < 0) l = 0; 
        if (u > Channels(s)) u = Channels(s);
        sum = sumw = 0.0;
        for (j = l; j < u; j++) { 
            delta = (Frequency(s, j)-freq)/FreqRes(s)/red;
            weight = exp(-2.0*PI*delta*delta);
            sum += s->data[j]*weight;
            sumw += weight;
        }
        tmp[i] = sum/sumw;
        freq += df; 
    }

    for (i = 0; i < nc; i++) s->data[i] = tmp[i];

    setFreqRes(s, df);
    setChannels(s, nc);

    /*    Bandwidth(s) = FreqRes(s)*Channels(s); */
    setVSource(s, v + CenterCh(s)*VelRes(s));
    setRestFreq(s, f + CenterCh(s)*FreqRes(s));
    setSkyFreq(s, RestFreq(s)-Df);
}

void Filter(Scan *s, double a[], int adim)
{
    float *fft1, *fft2, re, im;
    int i, m, n, k;
    n = 2;

    k = Channels(s);
    while (n < k) n *= 2;

    tmp[0] = a[0];
    for (i = 1; i < adim; i++) tmp[i] = tmp[n-i] = a[i];
    for (i = adim; i <= n-adim; i++) tmp[i] = 0.0;

    fft1 = (float *)calloc(2*n, sizeof(float));
    if (fft1 == NULL) return;

    fft2 = (float *)calloc(2*n, sizeof(float));
    if (fft1 == NULL) return;

    twofft(s->data, tmp, fft1, fft2, n);

    m = n/2;
    for (i = 0; i <= n; i += 2) {
        re = fft1[i]*fft2[i] - fft1[i+1]*fft2[i+1];
        im = fft1[i]*fft2[i+1] + fft1[i+1]*fft2[i];
        fft1[i]   = re/m;
        fft1[i+1] = im/m;
    }
    fft1[1] = fft1[n];

    realft(fft1, m, -1);
    for (i = 0; i < k; i++) s->data[i] = fft1[i];

    free(fft1);
    free(fft2);
}

#define MAXDIM 64
static double a[MAXDIM];
/* static double b[MAXDIM]; */

void Boxcar(Scan *s, int n)
{
    int i;

    if (n > MAXDIM) n = MAXDIM;
    for (i = 0; i < n; i++) a[i] = 1.0;
    for (i = n; i < MAXDIM; i++) a[i] = 0.0;

    Filter(s, a, n);
}

void Hanning(Scan *s)
{
    int i;

    a[0] = 0.5;
    a[1] = 0.25;
    for (i = 2; i < MAXDIM; i++) a[i] = 0.0;

    Filter(s, a, 2);
}

void Dispo(Scan *s, int order, int n)
{
    if (n > MAXDIM) return;
    savgol(a, 2*n+1, n, n, 0, order);
    n++;

    Filter(s, a, n);
}

void MedFilter(Scan *s, int width, double tol, int from, int to)
{
    int i, i1, i2;
    double med;

    drpcheck(&from, &to, Channels(s));
    for (i = from; i < to; i++) {
        i1 = i-width/2;
        if (i1 < from) i1 = from;
        i2 = i1+width;
        if (i2 > to)   i2 = to;
        i1 = i2-width;
        if (i1 < from) i1 = from;
        med = Median(s, i1, i2);
        if (fabs(s->data[from]-med) > tol) s->data[i] = med;
    }
}

#define MAXORDER   20

double *Baseline(Scan *s, int order, int *w, int n)
{
    double X, Y, nc;
    static double Matrix[MAXORDER*MAXORDER];
    static double *Array[MAXORDER], Vector[MAXORDER], D[MAXORDER];
    int i, j, k, l;

    if (n % 2) {
        fprintf(stderr, "DRP -- number of window limits must be even\n");
        return NULL;
    }

    if (order < 1 || order >= MAXORDER) {
        fprintf(stderr, "DRP -- baseline order out of range\n");
        return NULL;
    }
  
    drpsort(w, n);
    order++;
    for (i = 0; i < MAXORDER; i++) Array[i] = &Matrix[i*MAXORDER];

    for (i = 0; i < order; i++) {
        Vector[i] = 0.0;
        for (j = i; j < order; j++) {
            Array[i][j] = 0.0;
            Array[j][i] = 0.0;
        }
    }
  
    nc = (double)Channels(s);
    for (k = 0; k < n; k += 2) {
        for (i = w[k]; i <= w[k+1]; i++) {
            X = (double)i/nc-0.5;
            Y = (double)s->data[i];
            D[0] = 1.0;
            for (j = 1; j < order; j++) D[j] = D[j-1]*X;
            for (j = 0; j < order; j++) {
                Vector[j] += Y*D[j];
                for (l = j; l < order; l++) {
                    Array[j][l] += D[j]*D[l];
                    Array[l][j] = Array[j][l];
                }
            }
        }
    }
  
    if (solve(Array, Vector, order) == 0) {
        fprintf(stderr, "DRP -- singular matrix");
        return NULL;
    }

    return Vector;
}

void Polynom(Scan *s, double c[], int n)
{
    double p, X, nc;
    int i, j;

    nc = (double)Channels(s);
    for (i = 0; i < Channels(s); i++) {
        p = c[n-1];
        X = (double)i/nc-0.5;
        for (j = 1; j < n; j++) p = p*X+c[n-j-1];
        s->data[i] = (float)p;
    }
}

#define DIM    3
#define NLINES 1

double *Gaussfit(Scan *s, int from ,int to, double guess[])
{
    static double *X, *Y, *sig, max;
    static double a[NLINES*DIM], da[NLINES*DIM] /*, dy[NLINES*DIM] */;
    static double **covar, **alpha;
    static double chisq, ochisq, alamda;
    static int ia[NLINES*DIM];
    int dof, itst, i, j, iter, cc, ma, n;
    double Amplitude, Centre, Width;

    drpcheck(&from, &to, Channels(s));
    Amplitude = guess[0];
    Centre    = guess[1];
    Width     = guess[2];

    n = Channels(s);
    max = s->data[0];
    for (i = 1; i < n; i++) {
        if (s->data[i] > max) {
            max = s->data[i];
            cc = i;
        }
    }

    if (Amplitude == 0.0) Amplitude = max;
    if (Centre    == 0.0) Centre    = (double)cc;
    if (Width     == 0.0) Width     = 10.0;
 
    ma = DIM*NLINES;
    dof = to-from;
    if (dof <= ma) {
        fprintf(stderr, "too few points to fit\n");
        return NULL;
    }

    /*    printf("guess values [channels]:\n"); */
    /*    printf (" Amplitude = %f\n", Amplitude); */
    /*    printf (" Centre    = %f\n", Centre); */
    /*    printf (" Width     = %f\n", Width ); */
    /*    printf (" fit between channels %d and %d\n", from, to-1); */

    covar = (double **)calloc(ma, sizeof(double *));
    if (covar == NULL) {
        fprintf(stderr, "memory allocation failure\n");
        return NULL;
    }
    for (i = 0; i < ma; i++) {
        covar[i] = (double *)calloc(ma, sizeof(double));
        if (covar[i] == NULL) {
            fprintf(stderr, "memory allocation failure\n");
            return NULL;
        }
    }

    alpha = (double **)calloc(ma, sizeof(double *));
    if (alpha == (double **)NULL) {
        fprintf(stderr, "memory allocation failure\n");
        return NULL;
    }
    for (i = 0; i < ma; i++) {
        alpha[i] = (double *)calloc(ma, sizeof(double));
        if (alpha[i] == (double *)NULL) {
            fprintf(stderr, "memory allocation failure\n");
            return NULL;
        }
    }

    alamda = -1.0;

    X = (double *)calloc(dof, sizeof(double));
    if (X == (double *)NULL) {
        fprintf(stderr, "memory allocation failure\n");
        return NULL;
    }

    Y = (double *)calloc(dof, sizeof(double));
    if (Y == (double *)NULL) {
        fprintf(stderr, "memory allocation failure\n");
        return NULL;
    }

    sig = (double *)calloc(dof, sizeof(double));
    if (sig == (double *)NULL) {
        fprintf(stderr, "memory allocation failure\n");
        return NULL;
    }

    for (j = 0, i = from; i < to; j++, i++) {
        X[j] = (double)i;
        Y[j] = (double)s->data[i];
        sig[j] = 1.0;
    }

    a[0] = Amplitude;    ia[0] = 1;
    a[1] = Centre;       ia[1] = 1;
    a[2] = Width;        ia[2] = 1;

    alamda = -1;
    mrqmin(X, Y, sig, dof, a, ia, ma, covar, alpha, &chisq, fgauss, &alamda);
    iter = 1;
    itst = 0;
    while (itst < 4) {
        /*
          printf("\n%s %2d %17s %10.4f %10s %9.2e\n","Iteration #", iter,
          "chi-squared:",chisq,"alamda:",alamda);
          printf("%8s %8s %8s\n",
          "a[0]","a[1]","a[2]");
          for (i = 0; i < 3; i++) printf("%9.4f", a[i]);
          printf("\n"); 
        */
        iter++;
        ochisq = chisq;
        mrqmin(X,Y,sig,dof,a,ia,ma,covar,alpha,&chisq,fgauss,&alamda);
        if (chisq > ochisq)                 itst = 0;
        else if (fabs(ochisq-chisq) < 0.1)  itst++;
    }

    alamda = 0.0;
    mrqmin(X, Y, sig, dof, a, ia, ma, covar, alpha, &chisq, fgauss, &alamda);
    for (i = 0; i < 3; i++) da[i] = sqrt(covar[i][i]);

    return a;
}

void Gaussian(Scan *s, double c[])
{
    double x, dy[3];
    int i;

    for (i = 0; i < Channels(s); i++) {
        x = (double)i;
        s->data[i] = (float)fgauss(x, c, dy, 3);
    }
}
