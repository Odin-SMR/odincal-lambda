/* 
   Utility routines for use with the index word present in all
   level 0 files (replacing first dummy word 0x5c5c) 
*/

/*
  unsigned short int iword:

  bit 15:  0 -> invalid index, 1 -> valid index
  bit 14:  0 -> aero, 1 -> astro
  bit 12-13: spare
  bit 8-11: ACDC mode
  bit 0-7:  index 0-255
*/

#include "odinscan.h"

static char rcsid[] = "$Id";

int SIvalid(unsigned short int iword)
{
  return (iword & (1<<15));
}

int SIdiscipline(unsigned short int iword)
{
  if (!SIvalid(iword))   return 0;
  if (iword & (1<<14)) return ASTRO;
  else                 return AERO;
}

int SIacdc(unsigned short int iword)
{
  if (!SIvalid(iword)) return -1;
  else return ((iword & 0x0f00)>>8);
}

int SIindex(unsigned short int iword)
{
  if (!SIvalid(iword)) return -1;
  else return (iword & 0x00ff);
}

