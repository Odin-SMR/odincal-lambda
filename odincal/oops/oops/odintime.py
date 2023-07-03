#!/usr/bin/python

import sys
import math
import time
import string


def fitOrbit(stw):
    c = [-9.631225714726023e-27,
         4.743802284530715e-17,
         1.071801519797240e-05
         - 1.818550814109342e+00]
    o = c[3] + (c[2] + (c[1] + c[0] * stw) * stw) * stw
    return o


def fitMJD(stw):
    c = [1.978082016291609e-31,
         -1.368706688851376e-21,
         7.233098097480957e-07,
         5.196017912410997e+04]
    mjd = c[3] + (c[2] + (c[1] + c[0] * stw) * stw) * stw
    return mjd


class TimeConverter:
    def __init__(self):
        # file = "/archive/time/eqx.mat"
        file = "/home/bengt/test/eqx.mat"
        input = open(file, 'r')
        lines = input.readlines()
        i = 0
        self.orbits = []
        for line in lines:
            cols = string.split(line)
            orbit = int(float(cols[0]))
            JD = float(cols[3])
            if i == orbit:
                self.orbits.append(JD)
                i = i + 1
            else:
                print "index mismatch"
            input.close()

        # file = "/archive/time/STW_PLN.DAT"
        file = "/home/bengt/Downloads/stw_pln.dat"
        input = open(file, 'r')
        lines = input.readlines()
        self.STW = []
        for line in lines:
            cols = string.split(line)
            if len(cols) == 14 and cols[1] == "0000":
                expr = "0x" + cols[7] + "L"
                stw = long(expr, 16)
                if stw < 0:
                    stw = stw + 0x100000000
                # stw = cols[7]
                year = int(float(cols[8]) / 10000.0)
                mon = int(float(cols[8]) / 100.0) % 100
                day = int(float(cols[8])) % 100
                utc = string.split(cols[10], ':')
                hour = int(utc[0])
                min = int(utc[1])
                secs = float(utc[2])
                JD = self.djl(year, mon, day, hour, min, secs)
                rate = float(cols[11])
                self.STW.append([stw, JD, rate])
        input.close()

    def djl(self, year, mon, day, hour, min, secs):
        dn = 367 * year - 7 * (year + (mon + 9) / 12) / 4 \
            - 3 * ((year + (mon - 9) / 7) / 100 + 1) / 4 + 275 * mon / 9 \
            + day + 1721029
        jd = float(dn) - 0.5 + (hour + (min + secs / 60.0) / 60.0) / 24.0
        return jd

    def cld(self, JD):
        (z, f) = divmod(JD + 0.5, 1.0)
        if z < 2299161:
            a = z
        else:
            alpha = int((z - 1867216.25) / 36524.25)
            a = z + 1 + alpha - alpha / 4

        b = a + 1524
        c = int((b - 122.1) / 365.25)
        d = int(365.25 * c)
        e = int((b - d) / 30.6001)

        day = int(b - d - int(30.6001 * e))

        if e < 14:
            month = e - 1
        else:
            month = e - 13

        if month > 2:
            year = c - 4716
        else:
            year = c - 4715

        (z, f) = divmod(24.0 * f, 1.0)
        hour = int(z)
        (z, f) = divmod(60.0 * f, 1.0)
        min = int(z)
        secs = 60.0 * f
        return (year, month, day, hour, min, secs)

    def orbit2JD(self, orbit):
        if orbit < 0.0:
            return 0.0
        i = int(orbit)
        if len(self.orbits) > i + 1:
            r = orbit - i
            JD0 = self.orbits[i]
            JD1 = self.orbits[i + 1]
            JD = JD0 + r * (JD1 - JD0)
        else:
            JD0 = self.orbits[-2]
            JD1 = self.orbits[-1]
            r = orbit - len(self.orbits) + 1.0
            JD = JD1 + r * (JD1 - JD0)
        return JD

    def JD2orbit(self, JD):
        if JD < 2451960.8604086721:
            return 0.0
        for i in range(1, len(self.orbits)):
            if self.orbits[i] > JD:
                r = (JD - self.orbits[i - 1]) / \
                    (self.orbits[i] - self.orbits[i - 1])
                orbit = float(i - 1) + r
                break
        else:
            r = (JD - self.orbits[-2]) / (self.orbits[-1] - self.orbits[-2])
            orbit = float(len(self.orbits)) + r - 2.0
        return orbit

    def stw2JD(self, stw):
        n0 = 0
        n1 = len(self.STW) - 1
        if stw <= self.STW[n0][0]:
            stw0, JD0, rate = self.STW[n0]
        elif stw >= self.STW[n1][0]:
            stw0, JD0, rate = self.STW[n1]
        else:
            while (n1 - n0) > 1:
                n2 = (n1 + n0) / 2
                stw0 = self.STW[n2][0]
                if stw <= stw0:
                    n1 = n2
                else:
                    n0 = n2
            stw0, JD0, rate = self.STW[n0]
        JD = JD0 + (stw - stw0) * rate / 86400.0
        return JD

    def JD2stw(self, JD):
        n0 = 0
        n1 = len(self.STW) - 1
        if JD <= self.STW[n0][1]:
            stw0, JD0, rate = self.STW[n0]
        elif JD >= self.STW[n1][1]:
            stw0, JD0, rate = self.STW[n1]
        else:
            while (n1 - n0) > 1:
                n2 = (n1 + n0) / 2
                JD0 = self.STW[n2][1]
                if JD <= JD0:
                    n1 = n2
                else:
                    n0 = n2
            stw0, JD0, rate = self.STW[n0]
        stw = long(stw0 + math.floor((JD - JD0) * 86400.0 / rate + 0.5))
        return stw

    def timestamp(self, stw):
        JD = self.stw2JD(stw)
        orbit = self.JD2orbit(JD)
        (year, mon, day, hour, min, secs) = self.cld(JD)
        msg = " %08X %10d %10.3f %15.7f %4d-%02d-%02d %02d:%02d:%06.3f" % \
              (stw, stw, orbit, JD, year, mon, day, hour, min, secs)
        return msg


def usage():
    print "usage: %s            " % (sys.argv[0])
    print "       %s -stw number" % (sys.argv[0])
    print "       %s -orbit number" % (sys.argv[0])
    print "       %s -mjd number" % (sys.argv[0])
    print "       %s -jd number" % (sys.argv[0])
    print "       %s -date yyyy-mm-dd hh:mm:ss" % (sys.argv[0])
    sys.exit()


if __name__ == "__main__":
    tc = TimeConverter()
    if len(sys.argv) == 1:
        (year, mon, day, hour, min, secs, wday, yday, isdst) = time.gmtime()
        JD = tc.djl(year, mon, day, hour, min, secs)
        stw = tc.JD2stw(JD)
        orbit = tc.JD2orbit(JD)
#        for orbit in range(400,len(tc.orbits)):
#            JD = tc.orbit2JD(orbit)
#            (year, mon, day, hour, min, secs) = tc.cld(JD)
#            # orbit = tc.JD2orbit(JD)
#            # print "back  %8.2f" % orbit

#            stw = tc.JD2stw(JD)
#            print "%6.0f %15.7f %4d-%02d-%02d %02d:%02d:%06.3f %08X %10d" % \
#                  (orbit, JD, year, mon, day, hour, min, secs, stw, stw)
    else:
        if len(sys.argv) < 3:
            usage()
        if sys.argv[1] == '-stw':
            stw = sys.argv[2]
            if string.find(stw, '0x') == 0:
                stw = long(stw, 16)
            else:
                stw = long(stw)
            JD = tc.stw2JD(stw)
            orbit = tc.JD2orbit(JD)
            (year, mon, day, hour, min, secs) = tc.cld(JD)
        elif sys.argv[1] == '-orbit':
            orbit = float(sys.argv[2])
            JD = tc.orbit2JD(orbit)
            stw = tc.JD2stw(JD)
            (year, mon, day, hour, min, secs) = tc.cld(JD)
        elif sys.argv[1] == '-mjd':
            mjd = float(sys.argv[2])
            JD = mjd + 2400000.5
            stw = tc.JD2stw(JD)
            orbit = tc.JD2orbit(JD)
            (year, mon, day, hour, min, secs) = tc.cld(JD)
        elif sys.argv[1] == '-jd':
            JD = float(sys.argv[2])
            stw = tc.JD2stw(JD)
            orbit = tc.JD2orbit(JD)
            (year, mon, day, hour, min, secs) = tc.cld(JD)
        elif sys.argv[1] == '-date':
            date = sys.argv[2]
            if len(sys.argv) == 3:
                utc = "00:00:00"
            else:
                utc = sys.argv[3]
            (year, mon, day) = string.split(date, '-')
            year = int(year)
            mon = int(mon)
            day = int(day)
            (hour, min, sec) = string.split(utc, ':')
            hour = int(hour)
            min = int(min)
            secs = int(sec)
            JD = tc.djl(year, mon, day, hour, min, secs)
            stw = tc.JD2stw(JD)
            orbit = tc.JD2orbit(JD)
        else:
            usage()

    print " orbit  |   Julian Date   |    date    |     utc      |",
    print "STW(hex) | STW(dec)"
    print string.join(map((lambda x: '-' * x), [8, 17, 12, 14, 10, 11]), '+')
    print "%7.1f | %15.7f |" % (orbit, JD),
    print "%4d-%02d-%02d |" % (year, mon, day),
    print "%02d:%02d:%06.3f |" % (hour, min, secs),
    print "%08X |%10d" % (stw, stw)
