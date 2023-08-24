#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "options.h"

extern char PROGRAM_NAME[];
extern struct _options options[];
extern char required[];
extern char description[];
 
static char rcsid[] = "$Id";

/*----------------------------------------------------------------------*\
*  Syntax()
*
*  Print short help message.
\*----------------------------------------------------------------------*/

void Syntax(char *bad)
{
    struct _options *opt;
    int col, len;

    if (*bad != '\0')
	fprintf(stderr, "%s:  bad command line option \"%s\"\r\n\n",
		PROGRAM_NAME, bad);

    fprintf(stderr, "\nusage:  %s", PROGRAM_NAME);
    col = 8 + strlen(PROGRAM_NAME);
    for (opt = options; opt->opt; opt++) {
	len = 3 + strlen(opt->opt);	/* space [ string ] */
	if (col + len > 79) {
	    fprintf(stderr, "\r\n   ");	/* 3 spaces */
	    col = 3;
	}
	fprintf(stderr, " [%s]", opt->opt);
	col += len;
    }

    if (strlen(required) > 0) {
	len = 3 + strlen(required);
	if (col + len > 79) {
	    fprintf(stderr, "\r\n   ");
	    col = 3;
	}
	fprintf(stderr, " %s", required);
	col += len;
    }

    fprintf(stderr, "\r\n\nType %s -help for a full description.\r\n\n",
	    PROGRAM_NAME);
    exit(EX_ARGSBAD);
}

/*----------------------------------------------------------------------*\
*  Help()
*
*  Print long help message.
\*----------------------------------------------------------------------*/

void Help(void)
{
    struct _options *opt;

    fprintf(stderr, "name:\n\t%s\ndescription:\n\t%s\n", 
	    PROGRAM_NAME, description);
    fprintf(stderr, "\nusage:\n\t%s [-options ...] %s\n\n",
	    PROGRAM_NAME, required);
    fprintf(stderr, "where options include:\n");
    for (opt = options; opt->opt; opt++) {
	fprintf(stderr, "\t%-20s %s\n", opt->opt, opt->desc);
    }

    putc('\n', stderr);
}

/*----------------------------------------------------------------------*\
*  GetOpts()
*
*  Process options list
\*----------------------------------------------------------------------*/

void GetOpts(int argc, char **argv)
{
    /* Parse the command line */
    for (argc--, argv++; argc > 0; argc--, argv++) {
	if ((argv[0][0] == '-') && !isdigit(argv[0][1])) {
	    ParseOpts(&argc, &argv);
	}
    }
}

/*----------------------------------------------------------------------*\
*  GetOption()
*
*  Retrieves an option argument.
\*----------------------------------------------------------------------*/

void GetOption(char **poptarg, int *pargc, char ***pargv)
{
    if (*pargc > 0) {
	(*pargc)--;
	(*pargv)++;
	*poptarg = (*pargv)[0];
    } else {
	Syntax("");
    }
}




