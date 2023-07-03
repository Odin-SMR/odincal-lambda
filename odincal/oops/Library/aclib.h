#ifndef ACLIB_H
#define ACLIB_H

/**@name Correlator routines */
/*@{*/

/* $Id$ */

#include "level0.h"
#include "odinscan.h"

#define CLOCKFREQ    224.0e6

/* conversion factor between sample counter and integration time */
/*  #define SAMPLEFREQ   (10.0e6/4096.0) */
#define SAMPLEFREQ   10.0e6

#define MAXCHIPS     8
#define LAGSPERCHIP  96
#define MAXACCHAN   (MAXCHIPS*LAGSPERCHIP)

#define LEVELS 2
#define POS    0
#define NEG    1

/** Read level 0 data.
    Returns a pointer to an array of structures holding one AC block each.

    @param filename name of the level 0 file to be read
    @param n will contain number of blocks found on return
 */
ACSBlock *ReadACSLevel0(char *filename, int *n);

/** Get number of samples.
    Returns the number of samples for this integration.

    @param acs a pointer to a structure holding one AC block
 */
unsigned long GetSamples(ACSBlock *acs);

/** Get zero lag.
    Returns the value of the zero lag.

    @param acs a pointer to a structure holding one AC block
    @param chip the index of the correlator chip for which the
    zero lag is wanted
*/
unsigned long GetZLag(ACSBlock *acs, int chip);

/** Get commanded integration time.
    Returns the commanded integration time in seconds.

    @param acs a pointer to a structure holding one AC block
*/
float GetCmdIntTime(ACSBlock *acs);

/** Get prescaler.
    Returns the value of the prescaler for this integration.

    @param acs a pointer to a structure holding one AC block
*/
int GetPrescaler(ACSBlock *acs);

/** Get SSB attanuator value.
    Returns the value of the attenuator.

    @param acs a pointer to a structure holding one AC block
    @param ssb the index of the correlator band for which the
    attenuator value is wanted
*/
int GetSSBattenuator(ACSBlock *acs, int ssb, int *err);

/** Get SSB frequency.
    Returns the frequency in MHz.

    @param acs a pointer to a structure holding one AC block
    @param ssb the index of the correlator band for which the
    frequency is wanted
*/
double GetSSBfrequency(ACSBlock *acs, int ssb);

/** Get monitor value.
    Returns the value of monitor channel.

    @param acs a pointer to a structure holding one AC block
    @param chip the index of the correlator chip for which the
    monitor channel is wanted
    @param sign choses between positive and negative monitor channel
*/
unsigned long GetMonitor(ACSBlock *acs, int chip, int sign);

/** Describe ADC setup and sidebands used.

    The routine returns a sequence of 16 integers, whose
    meaning is as follows:

    n1 ssb1 n2 ssb2 n3 ssb3 ... n8 ssb8

    n1 ... n8 are the numbers of chips that are cascaded to form a band
    ssb1 ... ssb8 are +1 or -1 for USB or SSB, respectively.
    Unused ADCs are represented by zeros.

    examples (the "classical" modes):

    1 band/8 chips  0x00:   8  1  0  0  0  0  0  0  0  0  0  0  0  0  0  0
    2 band/4 chips  0x08:   4  1  0  0  0  0  0  0  4 -1  0  0  0  0  0  0
    4 band/2 chips  0x2A:   2  1  0  0  2  1  0  0  2 -1  0  0  2 -1  0  0
    8 band/1 chips  0x7F:   1  1  1 -1  1  1  1 -1  1 -1  1  1  1 -1  1  1

    @param mode  the number returned by GetACMode (new version)
*/
int *GetACSequence(int mode);

/** Get mode of correlator.
    Returns mode of correlator as reported in level 0 data file.

    @param acs a pointer to a structure holding one AC block
*/
int GetACMode(ACSBlock *acs);

/** Get the position of the internal calibration mirror.

    Note that this info is not normally contained in AC1/AC2 level 0
    files. We use program 'mirror' to copy the two mechanism words from
    the FBA level 0 files to words 9 and 10 of each first block of 
    correlator data. These words are otherwise zero and the mirror position
    would be returned as undefined in that case. That is just what we want.

    Returns 0 = undefined, 1 = sky beam 1, 2 = hot load, 3 = sky beam 2.

    @param acs a pointer to a structure holding one AC block
*/
int GetRefBeam(ACSBlock *acs);

/** Get input channel of correlator.
    Returns the selected input channel as reported in level 0 data file.

    @param acs a pointer to a structure holding one AC block
*/
int GetACInput(ACSBlock *acs);

/** Get type of correlator spectrum.
    Returns type of integration (SIG or REF).

    @param acs a pointer to a structure holding one AC block
*/
int GetACType(ACSBlock *acs);

/** Get observing mode.
    Returns 1 for switching, 0 for no-switching

    @param acs a pointer to a structure holding one AC block
*/
int GetACSwitch(ACSBlock *acs);

/** Get data values for this integration.
    Returns the number of channels read.

    @param data an array where the read values will be stored
    @param acs a pointer to a structure holding one AC block
*/
int GetACChannels(float *data, ACSBlock *acs);

/** Turn one set of AC data blocks into one spectrum.
    Returns 1 on success, 0 otherwise.

    @param next a pointer to an OdinScan structure
    @param aos a pointer to an array of AC blocks
*/
int ACSBlock2Scan(struct OdinScan *next, ACSBlock *acs);

/** Turn AC data blocks into OdinScan structures.
    Returns pointer to an array of OdinScan structures.
    This routine loops over all blocks in a level 0 files and
    calls 'ACSBlock2Scan' for each spectrum it finds.

    @param acs a pointer to an array of AC blocks
    @param n will hold number of spectra found on return
*/
struct OdinScan *GetACSscan(ACSBlock *acs, int *n);

/** Calculate total power from zero lag.
    Returns the calculated total power.

    @param zlag the value of the zero lag
    @param v the value of the threshold voltage
*/
double ZeroLag(double zlag, double v);

/** Perform quantisation correction.
    Returns 0 on error.

    @param c the mean of the positive and negative threshold values.
    @param f a pointer to the array of values to be corrected
    @param n the length of the array
*/
int QCorrect(double c, double f[], int n);

/** Calculate threshold value.
    Returns the resulting normalised threshold value.

    @param monitor value of the normalised monitor channel
*/
double Threshold(double monitor);

/** Perform Fourier transformation.

    @param data a pointer the data values to be transformed
    @param n the length of the data array
*/
void Transform(double data[], int n);

/** Reintroduce power.

    @param power the value of total power by which data will be scaled
    @param data a pointer the data values to be transformed
    @param n the length of the data array
*/
void PowerScale(double power, double data[], int n);

/** Normalise raw lags.
    Returns 0 on failure, 1 on success.

    Normalise raw data following Omnisys prescription

    @param s a pointer to an OdinScan structure
*/
int Normalise(struct OdinScan *s);

/** Reduce one correlator band.
    Returns 0 on failure, 1 on success.

    @param data a pointer the data values to be transformed
    @param n the length of the data array
    @param monitor an array holding the two monitor channels for
    this band
*/
int Reduce1Band(double data[], int n, double monitor[]);

/** Reduce one correlator spectrum.
    Returns 0 on failure, 1 on success.

    @param s a pointer to an OdinScan structure
*/
int ReduceAC(struct OdinScan *s);

/*@}*/

#endif
