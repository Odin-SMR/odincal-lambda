#ifndef LEVEL0_H
#define LEVEL0_H

/* $Id$ */

#define SYNCWORD 0x2bd3
#define AOSUSER  0x7360
#define AC1USER  0x7380
#define AC2USER  0x73b0
#define HKUSER   0x732c
#define FBAUSER  0x73ec
#define TAILWORD 0x5c5c

#define ROWSIZE   15

struct BlockHeader{
  unsigned short SYNC;
  unsigned short STW[2];
  unsigned short UserHead;
};

#define BLOCKHEAD (sizeof(struct BlockHeader)/sizeof(unsigned short))
#define INDEX 1

#define AC1BLOCKSIZE 64
#define AC2BLOCKSIZE 64
#define ACSBLOCKSIZE 64
#define AOSBLOCKSIZE 112
#define AOSTAILERLEN (ROWSIZE-((BLOCKHEAD+AOSBLOCKSIZE+INDEX)%ROWSIZE))
#define AC1TAILERLEN (ROWSIZE-((BLOCKHEAD+AC1BLOCKSIZE+INDEX)%ROWSIZE))
#define AC2TAILERLEN (ROWSIZE-((BLOCKHEAD+AC2BLOCKSIZE+INDEX)%ROWSIZE))
#define ACSTAILERLEN (ROWSIZE-((BLOCKHEAD+AC2BLOCKSIZE+INDEX)%ROWSIZE))

#define AOSHK       4
#define OSHK        3
#define PLATFORMHK 24
#define PLL_A_HK    8
#define PLL_B_HK    8
#define MECH_A_HK   6
#define MECH_B_HK   6
#define ACDCHK      4
#define GYROHK      3
#define ACSHK       1
#define HKBLOCKSIZE (AOSHK+OSHK+PLATFORMHK+PLL_A_HK+PLL_B_HK+ \
                     MECH_A_HK+MECH_B_HK+ACDCHK+GYROHK+ACSHK)
#define HKTAILERLEN (ROWSIZE-((BLOCKHEAD+HKBLOCKSIZE+INDEX)%ROWSIZE))

#define FBABLOCKSIZE 5
#define FBATAILERLEN (ROWSIZE-((BLOCKHEAD+FBABLOCKSIZE+2+INDEX)%ROWSIZE))

typedef struct {
  struct BlockHeader head;
  unsigned short data[HKBLOCKSIZE];
  unsigned short index;
  unsigned short tail[HKTAILERLEN];
} HKBlock;

typedef struct {
  struct BlockHeader head;
  unsigned short data[FBABLOCKSIZE];
  unsigned short mech[2];
  unsigned short index;
  unsigned short tail[FBATAILERLEN];
} FBABlock;

typedef struct {
  struct BlockHeader head;
  unsigned short data[AC1BLOCKSIZE];
  unsigned short index;
  unsigned short tail[AC1TAILERLEN];
} AC1Block;

typedef struct {
  struct BlockHeader head;
  short data[AC2BLOCKSIZE];
  unsigned short index;
  unsigned short tail[AC2TAILERLEN];
} AC2Block;

typedef struct {
  struct BlockHeader head;
  short data[ACSBLOCKSIZE];
  unsigned short index;
  unsigned short tail[ACSTAILERLEN];
} ACSBlock;

typedef struct {
  struct BlockHeader head;
  unsigned short data[AOSBLOCKSIZE];
  unsigned short index;
  unsigned short tail[AOSTAILERLEN];
} AOSBlock;

#endif
