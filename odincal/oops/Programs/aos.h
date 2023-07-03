#ifndef AOS_H
#define AOS_H

/* $Id$ */

typedef struct {
  short N;                       /* number of spectrum */
  short AOSformat;               /* AOS output format */
  long starting_format;          /* number of start format */
  long ending_format;            /* number of end format */
  short ACDC1sync;               /* synchro word #1 */
  short ACDC2sync;               /* synchro word #2 */
  short mechAsync;               /* synchro word #3 */
  short mechBsync;               /* synchro word #4 */
  short f119sync;                /* synchro word #5 */
  short spare1;           
  short obsmode;
  short NSec;                    /* duration of integration */
  short Nswitch;                 /* number of integrations */
  short AOSoutput;               /* AOS output format */
  short spare2;           
  short ACDC1mask;               /* mask word #1 */
  short ACDC2mask;               /* mask word #2 */
  short mechAmask;               /* mask word #3 */
  short mechBmask;               /* mask word #4 */
  short f119mask;                /* mask word #5 */
  short HKinfo[8];               /* engineering data */
  short cmdReadback[5];          /* return of commands */
  short CCDreadouts;             /* number of CCD readouts */
  long sum1728;                  /* sum of 1728 pixels */
} AOSsubHeader;

/* Engineering data: */
typedef struct {   
  int T_laser;                   /* laser diode temperature */
  int C_laser;                   /* laser diode current */
  int T_PAO;                     /* PAO structure temperature */
  int continuum;                 /* continuum measure (FI proc) */
  int T_IFproc;                  /* FI proc temperature */
  int phase_lock;                /* phase lock alarm */
  int spare1;
  int spare2;        
  int cmd_C_laser;               /* return value TC1 */
  int cmd_T_PAO;                 /* return value TC2 */
  int cmd_channel;               /* return value TC3 */
  int cmd_gain;                  /* return value TC4 */
  int cmd_comb;                  /* return value TC5 */
  unsigned version;              /* version number (ed., rev.) */
  int last_TCadress;             /* return address of last TC received */
  int last_TCword;               /* return word of last TC received */
} AOS_HK;

typedef struct {
  int TC0;                       /* autorisation commands 1 to 5 */
  int TC6;                       /* function mode */
  int TC7;                       /* integration time for spectra */
  int TC8;                       /* number of sub-integrations */
  int TC9;                       /* output format of spectra */
  int TC10;                      /* intended value of continuum */
  int TC11;                      /* integrator selection A or B */
  int TC12;                      /* ReadOut current spectrum */
  int TC13;                      /* Reset current integration */
  int TC250;                     /* mask word ACDC 1 */
  int TC251;                     /* mask word ACDC 2 */
  int TC252;                     /* mask word Mech A */
  int TC253;                     /* mask word Mech B */
  int TC254;                     /* mask word Frequency Switch */
  int spare1;
  int spare2;
} AOS_HK2;

#define SUBAOSLEN 16

/* format of scientific data */
typedef struct {
  AOS_HK hk;
  AOS_HK2 hk2;
  long subAOS[SUBAOSLEN];
} AOSHK_DATA;

#define AOSCHANNELS 1754

typedef struct {
  long fmt;
  AOSsubHeader header;
  long data[AOSCHANNELS];
} AOS_DATA;

#endif
