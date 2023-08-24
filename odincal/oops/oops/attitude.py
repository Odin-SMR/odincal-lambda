#!/usr/bin/python

import sys
import os
import string
import odin


class AttitudeParser:

    def __init__(self, files, stw0=0, stw1=0x800000000):
        self.files = files
        self.stw0 = stw0
        self.stw1 = stw1
        m = 0
        self.table = {}
        for file in files:
            n = 0
            odin.Info("processing file %s" % (file))
            self.input = open(file, 'r')
            # start to read header info in file
            line = 0
            # first extract soda version
            line0 = self.input.readline()
            line1 = line0.rsplit()
            self.soda = int(float(line1[len(line1) - 1]))
            while (line != 'EOF\n'):
                line = self.input.readline()
            for k in range(5):
                line = self.input.readline()
            t = self.getLine()
            while t:
                (year, mon, day, hour, min, secs,
                 stw, orbit, qt, qa, qe, gps, acs) = t
                if stw >= stw0 and stw <= stw1:
                    key = "%08X" % (stw)
                    if key in self.table:
                        # print "duplicate"
                        qe0 = self.table[key][10]
                        qe1 = t[10]
                        if qe0 != qe1:
                            err0 = qe0[0] * qe0[0] + qe0[1] * \
                                qe0[1] + qe0[2] * qe0[2]
                            err1 = qe1[0] * qe1[0] + qe1[1] * \
                                qe1[1] + qe1[2] * qe1[2]
                            # print "errors:", err0, err1
                            if err1 < err0:
                                self.table[key] = t
                    else:
                        self.table[key] = t
                    n = n + 1
                t = self.getLine()
            odin.Info("total of %5d lines in file %s" %
                      (n, os.path.basename(file)))
            m = m + n
            self.input.close()

        if len(files) > 1:
            odin.Info("total of %5d lines" % (m))

    def rewind(self):
        self.input.seek(0, 0)

    def getLine(self):

        line = self.input.readline()
        if not line:
            return None
        cols = string.split(line)
        if len(cols) < 23:
            return None
        date = cols[0]
        hour = int(cols[1])
        min = int(cols[2])
        sec = float(cols[3])
        stw = long(cols[4])
        orbit = float(cols[5])
        year = int(date[0:4])
        mon = int(date[4:6])
        day = int(date[6:8])
        qt = (float(cols[6]), float(cols[7]), float(cols[8]), float(cols[9]))
        qa = (
            float(
                cols[10]), float(
                cols[11]), float(
                cols[12]), float(
                    cols[13]))
        qe = (float(cols[14]), float(cols[15]), float(cols[16]))
        gps = (float(cols[17]), float(cols[18]), float(cols[19]),
               float(cols[20]), float(cols[21]), float(cols[22]))
        # new attitude format, cols[34] == 5 indicates astronomy fine pointing
        if len(cols) == 37 and int(cols[34]) == 5:
            acs = float(cols[36])
        else:
            acs = 0.0
        self.t1 = (
            year,
            mon,
            day,
            hour,
            min,
            sec,
            stw,
            orbit,
            qt,
            qa,
            qe,
            gps,
            acs)
        return self.t1

    def Lookup(self, stw):
        if stw < self.first:
            # print "not in file"
            return None
        stw0 = self.t0[6]
        stw1 = self.t1[6]
        if stw < stw0:
            print "rewind  %08X %08X %08X" % (stw, stw0, stw1)
            self.rewind()
            stw0 = self.t0[6]
            stw1 = self.t1[6]
        while stw >= stw1:
            stw0 = stw1
            t = self.getLine()
            if t:
                stw1 = t[6]
            else:
                # print "end of file at  %08X" % (stw1)
                stw1 = stw0 + 17
                break
        if stw >= stw0 and stw < stw1:
            # print "found     %08X %08X %08X" % (stw0, stw, stw1)
            return self.t0
        else:
            # print "not found %08X %08X %08X" % (stw0, stw, stw1)
            return None


if __name__ == "__main__":
    import time
    odin.LogAs("python")
    for file in sys.argv[1:]:
        starttime = time.time()
        ap = AttitudeParser((file,))
        elapsed = time.time() - starttime
        print "%s took %10d seconds" % (file, elapsed)
        t = ap.table
        keys = sorted(t.keys())

        (year, mon, day, hour, min, secs, stw,
         orbit, qt, qa, qe, gps, acs) = t[keys[0]]
        msg = " %08X %10d %10.3f %4d-%02d-%02d %02d:%02d:%06.3f" % \
              (stw, stw, orbit, year, mon, day, hour, min, secs)
        print msg

        (year, mon, day, hour, min, secs, stw,
         orbit, qt, qa, qe, gps, acs) = t[keys[-1]]
        msg = " %08X %10d %10.3f %4d-%02d-%02d %02d:%02d:%06.3f" % \
              (stw, stw, orbit, year, mon, day, hour, min, secs)
        print msg
