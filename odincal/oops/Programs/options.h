/*
* Exit codes 
*/

/* $Id$ */

#define	EX_SUCCESS	0		/* success! */
#define	EX_ARGSBAD	1		/* invalid args */
#define	EX_BADFILE	2		/* invalid filename */

/*----------------------------------------------------------------------*\
*  Options data structure definition.
\*----------------------------------------------------------------------*/

struct _options {
    char *opt;
    char *desc;
};

void	GetOpts		( int , char ** );
void	Syntax		( char * );
void	Help		( void );
void	ParseOpts	( int *, char *** );
void	GetOption	( char **, int *, char *** );

