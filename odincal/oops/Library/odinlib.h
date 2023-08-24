#ifndef ODINLIB_H
#define ODINLIB_H

/**@name General routines */
/*@{*/

/* $Id$ */

#include "odinscan.h"
#include "odinfilter.h"

typedef struct OdinScan   Scan;
typedef struct OdinFilter Bank;

/** Get long integer.
    Returns one 32 bit long integer (e.g. the satellite time word)
    from a level 0 data file. This
    is done in a manner, which is independent of the underlying 
    computer architecture (big endian or little endian). 

    @param w a pointer to the first of two consecutive 16-bit words
*/
unsigned long LongWord(unsigned short int w[]);

/** Get valid flag from science index.
    Returns status of valid flag in science index word of level 0 files.

    @param iword science index word of level 0 file
*/
int SIvalid(unsigned short int iword);


/** Get discipline from science index.
    Returns discipline (ASTRO or AERO) from science index word of 
    level 0 files, implicit check of valid flag.

    @param iword science index word of level 0 file
*/
int SIdiscipline(unsigned short int iword);


/** Get ACDC mode from science index.
    Returns ACDC mode from science index word of level 0 files.

    @param iword science index word of level 0 file
*/
int SIacdc(unsigned short int iword);


/** Get index count from science index.
    Returns index count from science index word of level 0 files.

    @param iword science index word of level 0 file
*/
int SIindex(unsigned short int iword);

/** Get STW reset counter.
    Returns the value of the satellite time word reset counter from 
    a given level 0 data file name. The counter is represented by the
    first character of the file name, after leading directory information 
    has been removed.

    @param filename name of the level 0 data file
*/
int STWreset(char *filename);

/** Get STW range.
    Given the name of a level 0 data file name, find the lowest and highest 
    value of the satellite time word present in that data file.

    @param filename name of the level 0 data file
    @param start will return the first STW found
    @param stop will return the last STW found
*/
void STWrange(FILE *pdcfile, unsigned long *start, unsigned long *stop);

/** Print scan header.
    Produce a pretty dump of the main header paramters for a given OdinScan
    structure.

    @param s pointer to an OdinScan structure
*/
void PrintScan(Scan *s);

/** Initialise scan header.
    Set main header paramters for a given OdinScan structure to reasonable
    initial values.

    @param s pointer to an OdinScan structure
*/
void InitHead(Scan *);

/** Set program signature.
    This will let you set a 6 character long name for the program to be used
    with error and status logging (see below), so that the origin of messages 
    becomes clear.

    @param name name under which messages should be logged
*/
void logname(char *name);

/** Set log file name.
    This will let specify the name of a file to receive all info and
    warn messages, which would otherwise go to stderr.

    @param name file name to be used 
*/
void logfile(char *name);

/** Log error.
    Log an error message and exit program. This uses the variable paramter
    {\tt vprintf} of ANSI C.

    @param fmt print format string
*/
void ODINerror(char *fmt, ...);

/** Log warning.
    Log a warning message and continue program execution. 
    This uses the variable paramter {\tt vprintf} of ANSI C.

    @param fmt print format string
*/
void ODINwarning(char *fmt, ...);

/** Log message.
    Log an informational message and continue program. 
    This uses the variable paramter {\tt vprintf} of ANSI C.

    @param fmt print format string
*/
void ODINinfo(char *fmt, ...);

/** Create file name.
    Compose a useful name for storing an OdinScan in its binary form
    on disk. The name will be composed of the backend (3 letters), the
    satellite time word in hexadecimal notation (8 letters) and the 
    spectrum type (3 letters), e.g. {\tt AOS.01234567.REF}

    @param s pointer to an OdinScan structure
*/    
char *SaveName(Scan *s);

/** Read scan.
    Read one OdinScan in its binary form from disk.
    Returns 0 on error, 1 on success.

    @param filename name of the file holding the scan
    @param s pointer to an OdinScan structure which will hold the scan
    on return
*/    
int readODINscan(char *filename, Scan *s);

/** Write scan.
    Write one OdinScan in its binary form to disk.
    Returns 0 on error, 1 on success.

    @param filename name of the file to be written to
    @param s pointer to an OdinScan structure which holds the scan
*/    
int writeODINscan(char *filename, Scan *s);

/*  void CalcTsys(Scan *, Scan *, double , double ); */
/*  void Calibrate(Scan *, Scan *, Scan *); */

/** Clear scan.
    Fill one OdinScan structure to all zeros, except member {\tt Version}
    which will be set to the proper version number.

    @param s pointer to an OdinScan structure which holds the scan
*/    
void clearscan(Scan *s);

/** Copy scan.
    Copy contents of one OdinScan structure to another structure.

    @param s pointer to an OdinScan structure which holds the destination
    @param t pointer to an OdinScan structure which holds the source
*/    
void copyscan(Scan *s, Scan *t);

/** New Mode.
    For correlator spectra there are two ways of coding the
    ADC sequence and therefor the frequency order of the channels. The
    traditional way simply enumerated the modes from AC_XHIRES to
    AC_LOWRES. The new way interprets the mode stored by the
    correlator bit by bit and leads to much cleaner code. This hadn't
    really been understood properly due to a misinterpretation of the
    bit order and due to missing documentation when lab tests were
    performed.
    This routine updates the header of a scan by replacing the old coding 
    by the new one. This involves some rearrangement of data and the 
    LO settings stored in FreqCal.
    The routine will only work on spectra which haven't been frequency 
    sorted or split, yet.
    Returns 1 on success, 0 on failure.

    @param s pointer to an OdinScan structure which holds the destination */
int newMode(Scan *s);
    
/** Split scan.
    For correlator spectra in split mode, this routine will split the spectrum
    into its two halves and replace the scan by one of them, depending on
    parameter {\tt upper}.
    Returns 1 on success, 0 on failure.

    @param s pointer to an OdinScan structure which holds the destination
    @param upper if true, return upper half, else return lower half */    
int splitcorr(Scan *s, int upper);

/** Extract band.  
    For correlator spectra, this routine will return a spectrum where all
    channels not belonging to the selected band have been dropped. This is
    meant to aid in reducing data one band at a time. The spectrum must
    not have been frequency sorted, yet. 
    Returns 1 on success, 0 on failure.

    @param s pointer to an OdinScan structure which holds the destination
    @param n an integer between 1 and 8, the number of band to extract  
    @param dsb if true, treat LSB/USB of one LO as one band
*/
int getband(Scan *s, int n, int dsb);

/** Get frequencies.
    Retrieve an array of frequency values in Hz. The array corresponds
    one by one to the array of channel data stored in the OdinScan structure.
    Returns 1 on success, 0 on failure.

    @param s pointer to an OdinScan structure which holds the destination
    @param f will hold array of frequencies on return.
*/    
int frequency(Scan *s, double f[]);

/** Drop channels.
    This routine will turn a correlator spectrum into one contiguous band,
    dropping overlapping frequencies and possibly filling in dummy values
    at intermediate frequencies which weren't covered. Array {\tt f} should
    be filled via a call to {\tt frequency} prior to using this routine.

    @param s pointer to an OdinScan structure which holds the destination
    @param f an array of frequencies.
*/    
int drop(Scan *s, double f[]);

/** Sort frequencies.
    Retrieve an array of sorted frequency values in Hz. Data channels in
    the OdinScan structure will be sorted the same way on return.
    Returns 1 on success, 0 on failure.

    @param s pointer to an OdinScan structure which holds the destination
    @param f will hold array of frequencies on return.
*/    
int freqsort(Scan *s, double f[]);

/** Change resolution.
    Reinterpolate spectral data onto a grid of frequencies seperated by 
    given frequency resolution, but still being centered on the same centre 
    frequency.

    @param s pointer to an OdinScan structure which holds the destination
    @param f will hold array of frequencies on return.
    @param df wanted frequency resolution.
*/    
void redres(Scan *s, double f[], double df);

/** Fix correlator bands.
    Determine if any of the individual correlator bands lies significantly
    above or below the others. If yes, make the deviating band line up 
    with the rest.
    Returns 1 if at least one band was adjusted.

    @param s pointer to an OdinScan structure which holds the destination
    @param f will hold array of frequencies on return.
*/
int fixband(Scan *s, double f[]);

/** Doppler shift.
    Calculate Doppler shifts due to the satellite's orbit around the
    Earth, given that GPS information from the attitude files has already
    been filled in.

    @param s pointer to an OdinScan structure where velocity results will
    be filled in
    @param ra true RA of date for which velocities should be calculated
    @param dec true Dec of date for which velocities should be calculated
    @param vsat will hold rectangular velocity components of satellite on 
    return, including Sun's LSR velocity, Earth's orbital motion
    and satellite's orbital motion around the Earth.
*/    
void Doppler(Scan *s, double ra, double dec, double vsat[]);

/** Set ACS alignment parameters.
    Supply new set of ACS alignment parameters. All further calls to the 
    'skybeams' routine will use these values to calculate attitude 
    information.

    @param version new version number to be stored in the upper nibble of 
    the 'Level' struct member of an Odin scan.
    @t an array of three doubles to pass the new alignment Euler angles.
*/
void setAlignment(int version, double t[3]);

/** Sky beam calculations.
    Calculate RA, Dec of main and sky beams, given that attitude information 
    from the attitude files has already been filled in. Also, member 
    {\tt SkyBeamHit} will be set accordingly. The positions of the
    Moon and Sun are calculated and filled in.

    @param s pointer to an OdinScan structure which holds the destination
    @param placs time dependent PLACS_ALIGN adjustment around SC_z in degrees
*/    
void skybeams(Scan *s, double placs);

/*@}*/

#endif
