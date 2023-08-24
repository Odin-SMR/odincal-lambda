#ifndef ODINTIME_H
#define ODINTIME_H

unsigned long stw2utc(unsigned long STW, int reset);
unsigned long utc2stw(unsigned long utc, int reset);
double stw2mjd(unsigned long stw, int reset);
unsigned long mjd2stw(double mjd, int reset);
double utc2mjd(unsigned long utc);
unsigned long mjd2utc(double mjd);
double mjd2orbit(double mjd);
double orbit2mjd(double orbit);
double utc2orbit(unsigned long utc);
unsigned long orbit2utc(double orbit);
double mjd(int year, int month, int day, int hour, int min, double secs);
char *asciitime(unsigned long utc);

#endif
