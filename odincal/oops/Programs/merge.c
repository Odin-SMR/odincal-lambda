#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "options.h"
#include "level0.h"
#include "odinlib.h"

static char rcsid[] = "$Id$";

#define WORDSIZE sizeof(unsigned short)
typedef unsigned short UWORD;

static char pdcfile[MAXNAMLEN+1] = "";
static char heapfile[MAXNAMLEN+1] = "";
static int rstcnt;

char PROGRAM_NAME[] = "merge";
char description[] = "filter level 0 data files";
char required[] = "filename";
struct _options options[] = {
{ "-file filename",    "specify level 0 data file" },
{ "-heap filename",    "specify name of heap file" },
{ "-reset n",          "use STW reset counter n in file names" },
{ "-help",	       "Print out this message" },
{NULL, NULL }};

void ParseOpts(int *pargc, char ***pargv)
{
    char *opt, *optarg;

    opt = (*pargv)[0] + 1;
    optarg = NULL;

    if (!strcmp(opt, "file")) {
	GetOption(&optarg, pargc, pargv);
	strcpy(pdcfile, optarg);
	return;
    }
    if (!strcmp(opt, "heap")) {
	GetOption(&optarg, pargc, pargv);
	strcpy(heapfile, optarg);
	return;
    }
    if (!strcmp(opt, "reset")) {
	GetOption(&optarg, pargc, pargv);
	rstcnt = atoi(optarg);
	return;
    }
    if (!strcmp(opt, "help")) {
	Help();
	exit(0);
    }
    Syntax(**pargv);
}

static struct {
    struct BlockHeader header;
    UWORD data[146];
} buffer, *start;

static unsigned long STW; 

int getwords(FILE *tm, UWORD *words, int n)
{
    int i;

    i = fread(words, WORDSIZE, n, tm);
    if (feof(tm)) return (-1);
    else if (i < n) {
	ODINwarning("error reading %d words", n);
	return (0);
    }
    /* printf ("got %d words\n", n); */
    return  n;
}

int checkblock(UWORD user, int blocksize, int tail)
{
    int i, k, next;
    unsigned long lastSTW;

    /* check that we have a sync word */
    if (buffer.header.SYNC != SYNCWORD) {
	ODINwarning("missing sync word");
	return 0;
    }
    
    /* check for correct user */
    if ((buffer.header.UserHead & 0xfff0) != user) {
	ODINwarning("wrong user number");
	return 0;
    }

    /* check for correct number of tail words */
    k = (blocksize - sizeof(struct BlockHeader))/WORDSIZE;
    for (i = 0; i < tail; i++) {
	if (buffer.data[k-i-1] != 0x5c5c) {
	    ODINwarning("tailer word error at STW = %08x", STW);
	    return 0;
	}
    }

    /* make sure STW is increasing */
    lastSTW = STW;
    STW = (buffer.header.STW[1]<<16) + (buffer.header.STW[0]);

    next = buffer.header.UserHead & 0x000f;
    if (next > 0) {
	/* while we are collecting data we require STW to increase by 1 */
	if (STW-lastSTW != 1) {
	    ODINwarning("satellite time word error at STW = %08x", STW);
	    return 0;
	}
    } else {
	/* else we require at least increasing STW */
	if (STW <= lastSTW) {
	    ODINwarning("satellite time word error at STW = %08x", STW);
	    return 0;
	}
    }

    return 1;
}

/* Read forward in file until SYNCWORD is found */
void sync(FILE *tm)
{
    int junk;
    UWORD word;

    junk = 0;
    while (getwords(tm, &word, 1) > 0) {
	if (word == SYNCWORD) {
	    buffer.header.SYNC = word;
	    if (getwords(tm, buffer.header.STW, 
			 sizeof(struct BlockHeader)/WORDSIZE-1) > 0) {
		if (junk) ODINwarning("skipped %d words", junk);
		return;
	    } else {
		ODINerror("can't read block header");
	    }
	} else {
	    junk++;
	}
    }
    /* we should never get here */
    ODINerror("couldn't find sync word");
}

/* Return the number of formats occupied by the various AOS modes */
int AOSformats(int mode)
{
    int formats;

    /* printf("AOS mode = %d\n", mode); */
    switch (mode) {
      case 111:
      case 211:
	formats = 16;
	break;
      case 312:
      case 322:
	formats = 32;
	break;
      case 411:
	formats = 8;
	break;
      case 511:
	formats = 3;
	break;
      case 611:
      case 711:
	formats = 5;
	break;
    }
    return formats;
}

int main(int argc, char *argv[])
{
    static char blockname[32], ext[4];
    FILE *tm, *valid, *heap;
    int need, next, got, i, k, count, aosmode, formats, tail, blocksize;
    unsigned long lastSTW, STW0;
    static UWORD user;
  
    void *stack;
  
    logname("merge");
    GetOpts(argc, argv);
    if (argc == 1) ODINerror("filename required as argument");

    /* initialize variables */
    count = 0;
    tm = valid = heap = NULL;

    if (!strcmp(pdcfile, "-")) tm = stdin;
    else {
	if (pdcfile[0] == '\0') strcpy(pdcfile, argv[argc-1]);
	tm = fopen(pdcfile, "r");
	if (tm == NULL) ODINerror("can't open data file '%s'", pdcfile);
    }

    /* find the next sync word in the level 0 data file */
    sync(tm);
    STW = (buffer.header.STW[1]<<16) + (buffer.header.STW[0]);
    user = buffer.header.UserHead & 0xfff0;

    switch (user) {
      case AOSUSER:
	strcpy(ext, "AOS");
	blocksize = sizeof(AOSBlock);
	aosmode =  0;  /* we don't know this yet       */
	formats = 32;  /* this is the maximum possible */
	tail = AOSTAILERLEN;
	break;
      case AC1USER:
	strcpy(ext, "AC1");
	blocksize = sizeof(AC1Block);
	formats = 13;
	tail = AC1TAILERLEN;
	break;
      case AC2USER:
	strcpy(ext, "AC2");
	blocksize = sizeof(AC2Block);
	tail = AC2TAILERLEN;
	formats = 13;
	break;
      default:
	ODINerror("invalid user number: %04x", user);
	break;
    }

    /* allocate enough memory to hold one complete spectrum */
    stack = calloc(blocksize, formats);
    if (stack == NULL) ODINerror("memory allocation failure");

    need = got = 0;

    /* 
       Check if there is a heap file, i.e. the start of a spectrum
       from a previous file. 
       Read that file and see if it fits in front of the new data.
    */
    if (heapfile[0] != '\0') {
	heap = fopen(heapfile, "r");
	if (heap == NULL) ODINwarning("can't open heap file '%s'", heapfile);
	else {
	    k = blocksize/WORDSIZE;
	    while (getwords(heap, (UWORD *)&buffer, blocksize/WORDSIZE) > 0) {
		STW0 = (buffer.header.STW[1]<<16) + (buffer.header.STW[0]);
		next = buffer.header.UserHead & 0x000f;
		if ((need == 0) && (user == AOSUSER)) {
		    aosmode = (int)buffer.data[1];
		    formats = AOSformats(aosmode);
		}

		if ((buffer.header.UserHead & 0x000f) == need) {
		    memcpy((char *)stack + need*blocksize, &buffer, blocksize);
		    got++;
		    need++;
		}
	    }
	}
	if (STW0 != STW-1) {
	    ODINwarning("heap file doesn't preceed data file");
	    need = got = 0;
	} else {
	    rewind(tm);
	    sync(tm);
	}
    }

    /* we have already read the block header, now read the rest */
    k = (blocksize - sizeof(struct BlockHeader))/WORDSIZE;
    if (getwords(tm, buffer.data, k) <= 0) {
	ODINerror("failed to read first data block");
    }

    /* make sure we are reading block 0 */
    if ((buffer.header.UserHead & 0x000f) == need) {
	memcpy((char *)stack + need*blocksize, &buffer, blocksize);
	got++;
	need++;
	/* For the AOS set the mode, although we may know it from heap */
	if ((user == AOSUSER) && (aosmode == 0)) {
	    aosmode = (int)buffer.data[1];
	    formats = AOSformats(aosmode);
	}
    } else {
	ODINwarning("block out of sequence at STW = %08x", STW);
    }

    while (getwords(tm, (UWORD *)&buffer, blocksize/WORDSIZE) > 0) {
  	lastSTW = STW;
	if (!checkblock(user, blocksize, tail)) continue;

	/* make sure that we are reading the right block */
	next = buffer.header.UserHead & 0x000f;
    
	if (next == 0) {
	    /* For the AOS the mode may have changed */
	    if (user == AOSUSER) {
		aosmode = (int)buffer.data[1];
		formats = AOSformats(aosmode);
	    }
	}

	/* the AOS is counting modulo 16 in case of a 312 mode */
	if (aosmode == 322) next += 16;

	/* check for out of sequence blocks */
	if ((need != next) && (next > 0)) {
	    if (need) ODINwarning("block out of sequence at STW = %08x", STW);
	    need = 0;
	}

	if ((next == 0) && (got > 0)) {
	    ODINwarning("unexpected block zero at STW = %08x", STW);
	    got = 0;
	    need = 0;
	}

	/* if STW out of sequence, look for block 0 */
	if ((next > 0) && ((STW-lastSTW) != 1)) need = 0;
    
	/* copy new data to stack */
	if (need == next) {
	    memcpy((char *)stack+(need%formats)*blocksize, &buffer, blocksize);
	    need++;
	    got++;
	}

	/* save to file if we have enough data */
	if (got == formats) {
	    /* open the output file, if we don't have one. */
	    if (valid == NULL) {
		k = strlen(pdcfile);
		for (i = 0; i < 12; i++) {
		    blockname[i] = pdcfile[k-12+i];
		}
		blockname[12] = '\0';
		valid = fopen(blockname, "w"); 
		if (valid == NULL) 
		    ODINerror("can't open data file '%s'", blockname);
		count = 0;
	    }

	    /* write data to file */
	    if (fwrite(stack, blocksize, formats, valid) < formats) {
		ODINwarning("failed to save valid data");
	    }
	    count++;
	    got = 0;
	    need = 0;
	}
    }
  
    /* do we have an open file? */
    if (valid) {
	fclose(valid);
	ODINinfo("saved %4d buffers in file '%s'", count, blockname);
	printf("%s\n", blockname);
    }

    fclose(tm);

    /* 
       Output remaining stack to a new heap file, which is given 
       the same name as the file we have just written, 
       but with .heap appended. 
    */
    if (got) {
	strcat(blockname, ".heap");
	heap = fopen(blockname, "w");
	if (heap == NULL) ODINerror("can't open data file '%s'", blockname);

	if (fwrite(stack, blocksize, got, heap) < got) {
	    ODINwarning("failed to save remaining data");
	}
	ODINinfo("saved remaining %d blocks to file '%s'", got, blockname);
	printf("%s\n", blockname);

	fclose(heap);
    }
      
    exit (0);
}
