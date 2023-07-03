#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

static char rcsid[] = "$Id$";
static char old[256];
static char new[256];

static char PROGRAM_NAME[] = "odin  ";

void logname(char *pn)
{
  strncpy(PROGRAM_NAME, pn, 6);
}

#define MAXSTAMPLEN 31

char *timestamp()
{
  static char stamp[MAXSTAMPLEN+1];
  size_t len;
  time_t now;
  struct tm *utc;

  now = time(NULL);
  utc = gmtime(&now);
  memset((void *)stamp, '\0', MAXSTAMPLEN+1);
  len = strftime(stamp, MAXSTAMPLEN, "%y%m%d %H%M", utc);

  return (stamp);
}

void ODINerror(char *fmt, ...)
{
  va_list args;
  int n;

  va_start(args, fmt);
  if (stderr != NULL) {
    sprintf(new, "%s (%-6.6s) FAIL: ", timestamp(), PROGRAM_NAME);
    n = strlen(new);
    vsprintf(new+n, fmt, args);
    if (strcmp(new, old)) {
      fprintf(stderr, "%s\n", new);
      fflush(stderr);
      strcpy(old,new);
    }
  }

  exit(1);
}

void ODINwarning(char *fmt, ...)
{
  va_list args;
  int n;

  va_start(args, fmt);
  if (stderr != NULL) {
    sprintf(new, "%s (%-6.6s) WARN: ", timestamp(), PROGRAM_NAME);
    n = strlen(new);
    vsprintf(new+n, fmt, args);
    if (strcmp(new, old)) {
      fprintf(stderr, "%s\n", new);
      fflush(stderr);
      strcpy(old,new);
    }
  }

  return;
}

void ODINinfo(char *fmt, ...)
{
  va_list args;
  int n;

  va_start(args, fmt);
  if (stderr != NULL) {
    sprintf(new, "%s (%-6.6s) INFO: ", timestamp(), PROGRAM_NAME);
    n = strlen(new);
    vsprintf(new+n, fmt, args);
    if (strcmp(new, old)) {
      fprintf(stderr, "%s\n", new);
      fflush(stderr);
      strcpy(old,new);
    }
  }

  return;
}

void logfile(char *filename)
{
  FILE *fp;
  int fd;

  fp = fopen(filename, "w");
  if (fp == NULL) {
    ODINwarning("can't redirect messages to file");
    return;
  }
  fd = fileno(fp);
  ODINinfo("messages redirected to file '%s' (%d)", filename, fd);
  if (fd < 3) {
    ODINwarning("can't redirect messages to file");
    return;
  }
  close(2); /* close stderr */
  dup(fd);
  close(fd);
}
