static char rcsid[] = "$Id$";

/*
  Get two unsigned shorts from level 0 file 
  and return as unsigned long in a machine independent manner.
*/
unsigned long LongWord(unsigned short int word[])
{
  long result;

  result = ((long)word[1]<<16) + word[0];
  return result;
}

