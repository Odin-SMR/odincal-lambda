#ifndef DRPLIB_H
#define DRPLIB_H

/**@name Data reduction routines.
   Note that the routines make use of a
   \begin{verbatim}
   typedef struct OdinScan Scan;
   \end{verbatim}
 */
/*@{*/

/* $Id$ */

/** Center channel.
    Returns channel number corresponding to the centre of the spectrum.

    @param s pointer to Odin scan structure
*/
int CenterCh(Scan *s);

/** Velocity resoultion.
    Returns velocity resolution of spectrum in m/s.

    @param s pointer to Odin scan structure
*/
double VelRes(Scan *s);

/** Bandwidth.
    Returns bandwidth of spectrum in Hz.

    @param s pointer to Odin scan structure
*/
double Bandwidth(Scan *s);

/** Velocity at given channel.
    Returns velocity at given channel in m/s.

    @param s pointer to Odin scan structure
    @param ch channel number
*/
double Velocity(Scan *s, int ch);

/** Frequency at given channel.
    Returns frequency at given channel in Hz.

    @param s pointer to Odin scan structure
    @param ch channel number
*/
double Frequency(Scan *s, int ch);

/** Frequency channel.
    Returns channel number corresponding to given frequency.

    @param s pointer to Odin scan structure
    @param freq frequency of interest in Hz
*/
int FChannel(Scan *s, double freq);

/** Velocity channel.
    Returns channel number corresponding to given velocity.

    @param s pointer to Odin scan structure
    @param vel velocity of interest in m/s
*/
int VChannel(Scan *s, double vel);

/** Test for compatibility.
    Returns true or false depending on whether or not the two scans are
    compatible, i.e. can be combined to form an average.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure 
*/
int compatible(Scan *s1, Scan *s2);

/** Align in frequency.
    Align two scans in frequency (and velocity) dropping all channels outside
    of common frequency (velocity) range. Returns 1 if successful, 0 otherwise.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure 
*/
int align(Scan *s1, Scan *s2);

/** Accumulate.

    Add scan s2 to scan s1, weighted by system temperature and time.
    and store result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Accum(Scan *ave, Scan *s);

/** Add.

    Add s1 to s2 channel by channel, result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Add(Scan *s1, Scan *s2);

/** Subtract.

    Subtract s2 from s1 channel by channel, result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Subtract(Scan *s1, Scan *s2);

/** Multiply.

    Multiply s1 by s2 channel by channel, result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Multiply(Scan *s1, Scan *s2);

/** Divide.

    Divide s1 by s2 channel by channel, result in s1. Channels for which 
    s2 contains 0 will be set to 0 in the resulting spectrum.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Divide(Scan *s1, Scan *s2);

/** Gain.

    Calculate gain curve from given hot load and cold sky spectrum.
    Efficiencies and spillover are taken care off according to
    calibration parameters in 'smrcdb.h'.

    @param cal pointer to Odin scan structure (spectrum on hot load)
    @param ref pointer to Odin scan structure (spectrum on cold sky) 
*/
void Gain(Scan *cal, Scan *ref);



/** Calibrate.

    Calculate (sig-ref)/cal, result in sig. Cal is supposed to contain
    a spectrum describing a gain curve (counts per K), use function
    'Gain' to calculate it. Efficiencies and spillover are taken care
    off according to calibration parameters in 'smrcdb.h'.

    @param sig pointer to Odin scan structure
    @param ref pointer to Odin scan structure
    @param cal pointer to Odin scan structure 
    @param eta spill-over efficiency of main beam, if zero a hard-coded 
    value will be used.
*/
void Calibrate(Scan *sig, Scan *ref, Scan *cal, double eta);

/** Calculate system temperature.

    Calculate system temperature using y-factor method.

    @param hot  pointer to Odin scan structure
    @param cold pointer to Odin scan structure
    @param Th physical temperature corresponding to hot spectrum
    @param Tc physical temperature corresponding to cold spectrum
*/
void CalcTsys(Scan *hot, Scan *cold, double Th, double Tc);

/** Integrate intensity.
    Returns sum of intensities over defined region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Area(Scan *s, int from, int to);

/** Add bias.

    @param s pointer to Odin scan structure
    @param v value to be added to all channels
*/
void Bias(Scan *s, double v);

/** Drop channels.

    Drop all channels outside of defined region.

    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
void Drop(Scan *s, int from, int to);

/** Find maximum intensity.
    Returns the maximum intensity in given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Maximum(Scan *s, int from, int to);

/** Calculate mean intensity.
    Returns the mean intensity in given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Mean(Scan *s, int from, int to);

/** Calculate median intensity.
    Returns the median intensity in given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Median(Scan *s, int from, int to);


/** Find minimium intensity.
    Returns the minimum intensity in given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Minimum(Scan *s, int from, int to);

/** Calculate standard deviation.
    Returns rms value over given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Rms(Scan *s, int from, int to);

/** Calcuate skew.
    Returns skewness over given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Skew(Scan *s, int from, int to);

/** Calculate kurtosis.
    Returns kurtosis over given region.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double Kurtosis(Scan *s, int from, int to);

/** Calculate moments.
    Returns pointer to array holding moments over given region.

    Calculate mean, rms, skewness and kurtosis over given region. The 
    results are stored in an array of 4 double values, a pointer to which
    is returned by the routine.
    
    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
double *Moment(Scan *s, int from, int to);

/** Scale spectrum.

    @param s pointer to Odin scan structure
    @param v value by which all channels will be multiplied
*/
void Scale(Scan *s, double v);

/** Set intensities.

    @param s pointer to Odin scan structure
    @param v value to which all channels in given region will be set
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
void setValue(Scan *s, double v, int from, int to);

/** Set maximum intensity.

    @param s pointer to Odin scan structure
    @param v maximum value to which all channels in given region 
    will be restricted
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
void setHigh(Scan *s, double v, int from, int to);

/** Set minimum intensity.

    @param s pointer to Odin scan structure
    @param v minimum value to which all channels in given region 
    will be restricted
    @param from channel number defining start of region
    @param to   channel number defining end of region
*/
void setLow(Scan *s, double v, int from, int to);

/** Shift frequency and velocity scale.

    @param s pointer to Odin scan structure
    @param n number of channels by which spectrum should be shifted
*/
void Shift(Scan *s, int n);

/** Fold a frequency switched spectrum.

    @param s pointer to Odin scan structure
    @param df frequency throw in Hz
    @param shift true or false indicating if switching was symmetric or not
    @return channel number corresponding to the centre of the spectrum 
*/
void Fold(Scan *s, double ft, int shift);

/** Convolve scans.

    Convolve scan s1 by s2, result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Convolve(Scan *s1, Scan *s2);

/** Correlate scans.

    Correlate scan s1 with s2, result in s1.

    @param s1 pointer to Odin scan structure
    @param s2 pointer to Odin scan structure
*/
void Correlate(Scan *s1, Scan *s2);

/** Reverse order of channels.

    @param s pointer to Odin scan structure
*/
void Reverse(Scan *s);

/** Smooth resolution.

    @param s pointer to Odin scan structure
    @param red factor by which resolution will be reduced.
*/
void Smooth(Scan *s, double red);

/** Apply digital filter.

    @param s pointer to Odin scan structure
    @param a pointer to array with filter coefficients
    @param n number of filter coefficients, i.e. dimension of a
    @return channel number corresponding to the centre of the spectrum 
*/
void Filter(Scan *s, double a[], int n);

/** Boxcar filter.

    @param s pointer to Odin scan structure
    @param n width of boxcar filter in channels
*/
void Boxcar(Scan *s, int n);

/** Hanning filter.

    @param s pointer to Odin scan structure
*/
void Hanning(Scan *s);

/** Savitzky-Golay smoothing filter.

    @param s pointer to Odin scan structure
    @param o order of filter
    @param n width of filter in channels
*/
void Dispo(Scan *s, int o, int n);

/** Fit baseline.

    @param s pointer to Odin scan structure
    @param o order of polynomial to be fitted
    @param w array of integers defining (pairwise) regions of spectrum 
    which should be used for the fit
    @param n number of values passed in w, must be even
*/
double *Baseline(Scan *s, int o, int w[], int n);

/** Replace spectrum with polynomial.

    @param s pointer to Odin scan structure
    @param c pointer to array of polynomial coefficients
    @param n number of coefficients
*/
void Polynom(Scan *s, double c[], int n);

/** Fit Gaussian.
    Returns pointer to array of doubles containing fit values for
    amplitude, centre and width.

    Fit a gaussian profile to given part of spectrum. Note that centre and 
    width are passed and returned in terms of channels, no conversion to
    frequency or velocity is performed.

    @param s pointer to Odin scan structure
    @param from channel number defining start of region
    @param to   channel number defining end of region
    @param guess pointer to array of doubles containing start values for
    amplitude, centre and width
*/
double *Gaussfit(Scan *s, int from ,int to, double guess[]);

/** Replace spectrum with Gaussian fit.

    @param s pointer to Odin scan structure
    @param c pointer to array of fitted polynomial coefficients
*/
void Gaussian(Scan *s, double c[]);

/*@}*/

#endif
