#ifndef AOSLIB_H
#define AOSLIB_H

/**@name AOS routines */
/*@{*/

/* $Id$ */

#include "level0.h"
#include "odinscan.h"

/** Get AOS format.
    Returns the mode (data format) the AOS was in during the integration.
*/
short AOSformat(AOSBlock *aos);

/** Get starting format.
    Returns the format number at the beginning of the integration.

    @param aos pointer to a block of AOS data
*/
unsigned long StartFormat(AOSBlock *aos);

/** Get ending format.
    Returns the format number at the end of the integration.

    @param aos pointer to a block of AOS data
*/
unsigned long EndFormat(AOSBlock *aos);

/** Get ACDC1 sync word.
    Returns a copy of the ACDC controller 1 synchronisation word.

    @param aos pointer to a block of AOS data
*/
short ACDC1sync(AOSBlock *aos);

/** Get ACDC2 sync word.
    Returns a copy of the ACDC controller 2 synchronisation word.

    @param aos pointer to a block of AOS data
*/
short ACDC2sync(AOSBlock *aos);

/** Get mech A sync word.
    Returns a copy of the A mechanism synchronisation word.

    @param aos pointer to a block of AOS data
*/
short mechAsync(AOSBlock *aos);

/** Get mech B sync word.
    Returns a copy of the B mechanism synchronisation word.

    @param aos pointer to a block of AOS data
*/
short mechBsync(AOSBlock *aos);

/** Get 119 GHz sync word.
    Returns a copy of the synchronisation word used for frequency switch
    at 119 GHz.

    @param aos pointer to a block of AOS data
*/
short F119sync(AOSBlock *aos);

/** Get calibration mirror position.
    Returns a copy of the mechanism word describing the position of
    the calibration mirror.

    @param aos pointer to a block of AOS data
*/
short CalMirror(AOSBlock *aos);

short FuncMode(AOSBlock *aos);
short NSec(AOSBlock *aos);
short Nswitch(AOSBlock *aos);

/** Get submode.
    Returns information on the location of the high resolution window, 
    if data have been taken in {\tt AOS_WINDOW} mode.

    @param aos pointer to a block of AOS data
*/
short AOSoutput(AOSBlock *aos);

/** Get ACDC1 mask.
    Returns a copy of the ACDC controller 1 mask word.

    @param aos pointer to a block of AOS data
*/
short ACDC1mask(AOSBlock *aos);

/** Get ACDC2 mask.
    Returns a copy of the ACDC controller 2 mask word.

    @param aos pointer to a block of AOS data
*/
short ACDC2mask(AOSBlock *aos);

/** Get mech A mask.
    Returns a copy of the A mechanism mask word.

    @param aos pointer to a block of AOS data
*/
short mechAmask(AOSBlock *aos);

/** Get mech B mask.
    Returns a copy of the B mechanism mask word.

    @param aos pointer to a block of AOS data
*/
short mechBmask(AOSBlock *aos);

/** Get 119 GHz mask.
    Returns a copy of the frequency switch mask word.

    @param aos pointer to a block of AOS data
*/
short F119mask(AOSBlock *aos);

/** Get HK info.
    Returns one housekeeping data word.

    @param aos pointer to a block of AOS data
    @param i   index of word to be retrieved
*/
short HKinfo(AOSBlock *aos, int i);

/** Get readback command.
    Returns one of the configuration command words.

    @param aos pointer to a block of AOS data
    @param i   index of word to be retrieved
*/
short cmdReadback(AOSBlock *aos, int i);

/** Get CCD readouts.
    Returns the total number of CCD readouts used for this integration.

    @param aos pointer to a block of AOS data
*/
short CCDreadouts(AOSBlock *aos);

/** Get check sum.
    Returns the check sum for the retrieved integration.

    @param aos pointer to a block of AOS data
*/
unsigned long CheckSum(AOSBlock *aos);

/** Get input channel of AOS.
    Returns the selected input channel as reported in level 0 data file.

    @param aos a pointer to a structure holding one AOS block
*/
int GetAOSInput(AOSBlock *aos);

/** Get type of AOS spectrum.
    Returns type of integration (SIG or REF).

    @param aos a pointer to a structure holding one AOS block
*/
int GetAOSType(AOSBlock *aos);

/** Read level 0 data.
    Returns a pointer to an array of structures holding one AOS block each.

    @param filename name of the level 0 file to be read
    @param n will contain number of blocks found on return
 */
AOSBlock *ReadAOSLevel0(char *filename, int *n);

/** Get data values for this integration.
    Returns the number of channels read.

    @param data an array where the read values will be stored
    @param aos a pointer to a structure holding one AOS block
*/
int GetAOSChannels(int mode, float *data, AOSBlock *aos);

/** Turn one set of AOS data blocks into one spectrum.
    Returns 1 on success, 0 otherwise.

    @param next a pointer to an OdinScan structure
    @param aos a pointer to an array of AOS blocks
*/
int AOSBlock2Scan(struct OdinScan *next, AOSBlock *aos);

/** Turn AOS data blocks into OdinScan structures.
    Returns pointer to an array of OdinScan structures.
    This routine loops over all blocks in a level 0 files and
    calls 'AOSBlock2Scan' for each spectrum it finds.

    @param next a pointer to an array of OdinScan structures
    @param n will hold number of spectra found on return
*/
struct OdinScan *GetAOSscan(AOSBlock *aos, int *n);

/** Fit comb spectrum.
    Returns a pointer to an array of fitted coefficients in Hz.

    @param s pointer to an OdinScan structure
*/
double *FitAOSFreq(struct OdinScan *s);

/*@}*/

#endif
