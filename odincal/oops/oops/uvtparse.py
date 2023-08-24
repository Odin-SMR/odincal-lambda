#!/usr/bin/python

import os
import sys
import string
import re
from types import *

ParseError = "ParseError"


class UVTParser:
    def __init__(self, file):
        self.fd = open(file, 'r')
        line = self.fd.readline()
        self.plan = None
        i0 = string.find(line, "[")
        if i0 != -1:
            i1 = string.find(line, "]")
            if i1 != -1:
                self.plan = line[i0 + 1:i1]
        line = self.fd.readline()
        if line[0:4] == "[***":
            self.discipline = string.upper(line[4:8])
            line = self.fd.readline()
            self.comment = string.rstrip(line)
            line = self.fd.readline()
        else:
            self.discipline = None
        self.euler = {}
        self.ompbs = []
        self.modes = []
        self.starings = []
        self.slewings = []
        self.limbpoints = []
        self.limbscans = []
        self.calibrations = []
        self.JD = -1.0

    def djl(self, year, mon, day):
        dn = 367 * year - 7 * (year + (mon + 9) / 12) / 4 \
            - 3 * ((year + (mon - 9) / 7) / 100 + 1) / 4 + 275 * mon / 9 \
            + day - 678972
        jd = float(dn)
        # jd = float(dn)-0.5
        # jd = jd-2400000.5
        return jd

    def parseNewompb(self, line):
        cols = string.split(line)
        if len(cols) < 2:
            return
        ompb = int(cols[1])
        if ompb == 0:
            return
        i0 = string.find(line, '[')
        i1 = string.find(line, ']')
        object = line[i0 + 1:i1 - 1]
        if string.find(object, "Aero") > 0:
            return
        if len(object) >= 31:
            topic = object[0:6]
            source = object[7:23]
            vlsr = float(object[24:31])
            mode = object[32:35]
            if mode[0] == "M":
                map = (int(object[35:41]),
                       int(object[41:47]),
                       float(object[47:]))
            else:
                map = (0, 0, 0.0)
            self.ompbs.append((self.JD, ompb, topic, source, vlsr, mode, map))

    def parsePayload(self, line):
        cols = string.split(line)
        if cols[3] == "SETMODE":
            if len(cols) > 5:
                setup = cols[4]
                vdop = float(cols[5][4:])
                self.modes.append((self.JD, setup, vdop))
        elif cols[3] == "SETINTEGRATE":
            pass
        elif cols[3] == "CALIBRATE":
            parts = string.split(cols[4], "_")
            if len(parts) > 1:
                if parts[1] == "UPLEG":
                    self.calibrations.append((self.JD, "UP"))
                elif parts[1] == "DOWNLEG":
                    self.calibrations.append((self.JD, "DOWN"))
            else:
                pass
        elif cols[3] == "SKYBEAM":
            pass
        elif cols[3] == "HANDOVER":
            pass
        elif cols[3] == "CONFIGURE":
            pass
        else:
            raise ParseError(line)

    def parseStaring(self, line):
        self.start = self.JD
        cols = string.split(line)
        if cols[3] == "INORBIT":
            pass
        elif cols[3] == "SUNEARTH":
            pass
        elif cols[3] == "3AXIS":
            hms = float(cols[4]) / 10000.0
            (h, r) = divmod(hms, 1.0)
            (m, s) = divmod(r * 100.0, 1.0)
            ra = (h + m / 60.0 + s / 36.0) * 15.0
            dec = float(cols[5])
            los = float(cols[6])
            self.starings.append((self.JD, ra, dec, los))
        else:
            raise ParseError(line)

    def parseSlewing(self, line):
        self.start = self.JD
        cols = string.split(line)
        if cols[3] == "3AXIS":
            hms = float(cols[4]) / 10000.0
            (h, r) = divmod(hms, 1.0)
            (m, s) = divmod(r * 100.0, 1.0)
            ra = (h + m / 60.0 + s / 36.0) * 15.0
            dec = float(cols[5])
            los = float(cols[6])
            self.slewings.append((self.JD, ra, dec, los))
        elif cols[3] == "2AXIS":
            hms = float(cols[4]) / 10000.0
            (h, r) = divmod(hms, 1.0)
            (m, s) = divmod(r * 100.0, 1.0)
            ra = (h + m / 60.0 + s / 36.0) * 15.0
            dec = float(cols[5])
            los = float(cols[6])
            self.slewings.append((self.JD, ra, dec, los))
        else:
            raise ParseError(line)

    def parseLimbPoint(self, line):
        self.start = self.JD
        cols = string.split(line)
        plane = float(cols[3])
        alt = float(cols[4])
        self.limbpoints.append((self.JD, plane, alt))

    def parseLimbScan(self, line):
        self.start = self.JD
        cols = string.split(line)
        alt1 = float(cols[3])
        alt2 = float(cols[4])
        alt3 = float(cols[5])
        alt4 = float(cols[6])
        phidot = float(cols[7])
        self.limbscans.append((self.JD, alt1, alt2, alt3, alt4, phidot))

    def processLine(self):
        line = self.fd.readline()
        if not line:
            return None
        if re.compile(r"^\s+$").match(line):
            return line
        line = string.lstrip(line)
        if line[0:7] != "NEWOMPB":
            line = string.upper(line)
        result = line
        cols = string.split(line)

        if line[0:5] == "EULER":
            instrument = cols[4]
            line = self.fd.readline()
            line = string.lstrip(line)
            cols = string.split(line)
            self.euler[instrument] = (float(cols[2]),
                                      float(cols[5]),
                                      float(cols[8]))
            return result

        if line[1:10] == "UT TO STW":
            line = self.fd.readline()
            line = self.fd.readline()
            line = string.lstrip(line)
            cols = string.split(line)
            self.utc2stw = cols

            # print "UT/STW:",self.utc2stw
            return result

        if line[0:7] == "NEWOMPB":
            if self.discipline == "ASTR" and line[7] == ':':
                self.parseNewompb(line)
        else:
            if re.compile(r"\d\d\d\d\d\d\d\d").match(cols[0]):
                year = int(cols[0][0:4])
                mon = int(cols[0][4:6])
                day = int(cols[0][6:8])
                secs = float(cols[1])
                self.JD = self.djl(year, mon, day) + secs / 86400.0
                result = self.JD
                if len(cols) <= 2:
                    return None
                if cols[2] == "PAYLOAD":
                    self.parsePayload(line)
                elif cols[2] == "STARING":
                    self.parseStaring(line)
                elif cols[2] == "STARING_CAL":
                    self.parseStaring(line)
                elif cols[2] == "CAL_SLEW":
                    self.parseSlewing(line)
                elif cols[2] == "LIMB_POINTING":
                    self.parseLimbPoint(line)
                elif cols[2] == "LIMB_SCANNING_CONT":
                    self.parseLimbScan(line)
                else:
                    pass
            else:
                raise ParseError(line)
        return result


if __name__ == "__main__":
    for file in sys.argv[1:]:
        sp = UVTParser(file)

        while True:
            try:
                result = sp.processLine()
                if not result:
                    break
            except ParseError as line:
                print "syntax error:", line
                break

        print "plan:"
        print sp.plan
        print "discipline:"
        print sp.discipline
        print "comment:"
        print sp.comment
        print "instrument:"
        for instrument in sp.euler.keys():
            print instrument, sp.euler[instrument]

        print "ompb:"
        for ompb in sp.ompbs:
            print ompb
        print "mode:"
        for mode in sp.modes:
            print mode
        if sp.discipline == 'ASTR':
            print "staring:"
            for staring in sp.starings:
                print staring
            print "slewing:"
            for slewing in sp.slewings:
                print slewing
            print "calibration:"
            for cal in sp.calibrations:
                print cal
        elif sp.discipline == 'AERO':
            print "limbpoint:"
            for point in sp.limbpoints:
                print point
            print "limbscan:"
            for scan in sp.limbscans:
                print scan
