#ifndef SHKLIB_H
#define SHKLIB_H

/**@name HK routines */
/*@{*/

/* $Id$ */

#include "level0.h"

/** Open HK file.
    Open a level 0 house-keeping data file and return the number 
    of HK data blocks found.

    @param filename name of the level 0 file to be read
 */
int OpenHKLevel0(char *filename);

/** Close HK file.
    Closes the previously opened file.
*/
void CloseHKLevel0();

/** Get STW.
    Returns the satellite time word of given block.

    @param i index of block for which STW should be read.
*/
unsigned long shkSTW(int i);

/** Get HK words.
    Retrieve array of house-keeping words at given index. Subcommuted values
    can be selected via non-zero parameter {\tt sub}. 
    Returns the number of readings found.

    @param index index of wanted HK word
    @param sub if non-zero, select only words with this sub-commutor value
    @param STW array to hold satellite time words for all values on return
    @param w array to hold retrieved values on return
    @param bad if non-zero, skip data with this value
*/
int shkWord(int index, int sub, unsigned long STW[], 
	    unsigned short *w, unsigned short bad);

/** Get engineering data.
    Retrieve array of house-keeping words at given index and convert
    to engineering units. Subcommuted values
    can be selected via non-zero parameter {\tt sub}.
    Returns the number of readings found.

    @param index index of wanted HK word
    @param sub if non-zero, select only words with this sub-commutor value
    @param convert pointer to function to convert short integers to engineering
    data
    @param bad if non-zero, skip data with this value
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved engineering data on return
*/
int shkArray(int index, int sub, double (*convert)(unsigned short), 
	     unsigned short bad,unsigned long STW[],double y[]);

/** Get mechanism data.
    Retrieve array of mechanism readings at given index. Will try both
    mechanism A and B if necessary.
    Subcommuted values can be selected via non-zero parameter {\tt sub}.
    Returns the number of readings found.

    @param index index of wanted HK word
    @param sub if non-zero, select only words with this sub-commutor value
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int mechArray(int index, int sub, unsigned long STW[],double y[]);

/** Look-up STW.
    Given a table of satellite time words and readings, returns the reading 
    of prior or equal to the given STW.
    Returns the looked-up value.

    @param STW0 satellite time word to be looked up in table
    @param n length of the passed arrays
    @param STW array holding satellite time words
    @param y array holding data
*/
double LookupHK(unsigned long STW0, int n, unsigned long STW[], double y[]);

/** Interpolate STW.
    Given a table of satellite time words and readings, returns an
    linearly interpolated reading for the given STW.
    Returns the interpolated value.

    @param STW0 satellite time word to be looked up in table
    @param n length of the passed arrays
    @param STW array holding satellite time words
    @param y array holding data
*/
double Interp1HK(unsigned long STW0, int n, unsigned long STW[], double y[]);

double HK2Etrip(unsigned short hkword);
double HK2Edoub(unsigned short hkword);
double HK2Ehmix(unsigned short hkword);
double HK2Egunn(unsigned short hkword);
double HK2Evar(unsigned short hkword);
double HK2Emix(unsigned short hkword);
double HK2Ehemt(unsigned short hkword);
double HK2Ewarm(unsigned short hkword);
double HK2Ecold(unsigned short hkword);
double HK2Ehro(unsigned short hkword);
double HK2Epro(unsigned short hkword);
double HK2Edro(unsigned short hkword);
double HK2Eaos1(unsigned short hkword);
double HK2Eaos2(unsigned short hkword);
double HK2Eaos3(unsigned short hkword);
double HK2Eaos4(unsigned short hkword);
double HK2Eaos5(unsigned short hkword);

/** Get varactor voltages.
    Retrieve array of varactor voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int Varactor(int rx, unsigned long STW[], double y[]);

/** Get gunn bias.
    Retrieve array of gunn bias voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int Gunn(int rx, unsigned long STW[], double y[]);

/** Get harmonic mixer voltages.
    Retrieve array of harmonic mixer voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int HMixer(int rx, unsigned long STW[], double y[]);

/** Get doubler voltages.
    Retrieve array of doubler voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int Doubler(int rx, unsigned long STW[], double y[]);

/** Get tripler voltages.
    Retrieve array of tripler voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int Tripler(int rx, unsigned long STW[], double y[]);

/** Get mixer current.
    Retrieve array of mixer currents for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int MixerC(int rx,  unsigned long STW[], double y[]);

/** Get HEMT bios.
    Retrieve array of HEMT bias voltages for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param hemt wanted HEMT
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int HemtBias(int rx, int hemt, unsigned long STW[], double y[]);

/** Get warm IF temperature.
    Retrieve array of temperatures for the warm IF for given side (A or B).
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int warmIF(char side, unsigned long STW[], double y[]);

/** Get calibration load temperature.
    Retrieve array of temperatures for the calibration load for 
    given side (A or B).
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int CLoad(char side, unsigned long STW[], double y[]);

/** Get image load temperature.
    Retrieve array of temperatures for the image load for 
    given side (A or B). Note, that for side B this will retrieve the
    temperature of the PLL box.
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int ILoad(char side, unsigned long STW[], double y[]);

/** Get mixer temperature.
    Retrieve array of mixer temperatures for given side (A or B).
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int MixerT(char side, unsigned long STW[], double y[]);

/** Get LNA temperatures.
    Retrieve array of LNA temperatures for given side (A or B).
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int LNA(char side, unsigned long STW[], double y[]);

/** Get 119GHz mixer temperature.
    Retrieve array of temperatures for the 119GHz mixer for 
    given side (A or B).
    Returns the number of readings found.

    @param side wanted side
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int Mix119(char side, unsigned long STW[], double y[]);

/** Get HRO frequency.
    Retrieve array of HRO frequencies for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int HROfreq(int rx, unsigned long STW[], double y[]);

/** Get PRO frequency.
    Retrieve array of PRO frequencies for given receiver.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int PROfreq(int rx, unsigned long STW[], double y[]);

/** Get LO frequencies.
    Retrieve array of LO frequencies for given receiver.
    Uses {\tt HROfreq} and {\tt PROfreq} internally.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int LOfreq(int rx, unsigned long STW[], double y[]);

/** Get SSB tunings.
    Retrieve array of SSB tuning mechanism readings for given receiver,
    which can be used to distinguish between upper and lower sideband tuning.
    Returns the number of readings found.

    @param rx wanted receiver
    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int TuneMechanism(int rx, unsigned long STW[], double y[]);

/** Get AOS laser temperature.
    Retrieve array of AOS laser temperatures.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int AOSlaserT(unsigned long STW[], double y[]);

/** Get AOS laser current.
    Retrieve array of AOS laser currents.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int AOSlaserC(unsigned long STW[], double y[]);

/** Get AOS structure temperature.
    Retrieve array of AOS structure temperatures.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int AOSstruct(unsigned long STW[], double y[]);

/** Get AOS continuum level.
    Retrieve array of AOS continuum level readings.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int AOScontinuum(unsigned long STW[], double y[]);

/** Get AOS processor temperature.
    Retrieve array of AOS processor temperatures.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int AOSprocessor(unsigned long STW[], double y[]);

/** Get 119 GHz DRO temperature.
    Retrieve array of DRO temperatures.
    Returns the number of readings found.

    @param STW array to hold satellite time words for all values on return
    @param y array to hold retrieved data on return
*/
int DROtemp(unsigned long STW[], double y[]);

/** Calculate frequency drift.
    Returns a correction factor to be applied to a LO frequency in
    order to correct for temperature drifts. The two sides, A and B, are
    described by different fitted coefficients.

    @param side wanted side
    @param T temperature in degrees Celius 
*/
double LOdrift(char side, double T);

/** Calculate IF frequency.
    Returns the value of the first IF, either -3900MHz or +3900MHz.

    @param rx wanted receiver
    @param LOfreq local oscillator frequency system was tuned to
    @param mech reading of the SSB tuning mechanism for the given receiver
*/
double IFreq(int rx, double LOfreq, double mech);

/*@}*/

#endif
