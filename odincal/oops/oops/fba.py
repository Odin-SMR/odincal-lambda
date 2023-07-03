#!/usr/bin/python

# standard Python modules
import sys
import os
import string
import re
import time

# third party Python modules
import pg
from Numeric import *

# Odin modules
import odin
import odinpath
from level0 import *
from odintime import TimeConverter
from attitude import AttitudeParser


class FBAProcessor:
    def __init__(self, orbit):
        self.orbit = orbit
        self.backend = 'FBA'
        self.mode = 'AERO'
        self.tc = TimeConverter()
        self.JD0 = self.tc.orbit2JD(orbit)
        self.JD1 = self.tc.orbit2JD(orbit + 1)
        self.stw0 = self.tc.JD2stw(self.JD0)
        self.stw1 = self.tc.JD2stw(self.JD1)
        self.ilast = 0
        print "looking for", self.backend, "spectra in orbit", orbit
        print "starting:", self.starttime()
        print "ending:  ", self.stoptime()

        """Find level 0 files which provide data for the orbit in question."""
        db = pg.connect('smr', 'ymer', user='smr')
        query = "SELECT ext,file FROM level0"
        query = query + "\n\tWHERE start < %d" % (self.stw1)
        query = query + " AND stop > %d" % (self.stw0)
        query = query + "\n\tORDER BY file"
        print query
        q = db.query(query)
        self.files = {}
        for row in q.getresult():
            ext = row[0]
            if ext not in self.files:
                self.files[ext] = []
            self.files[ext].append(row[1])

    def starttime(self):
        (year, mon, day, hour, min, secs) = self.tc.cld(self.JD0)
        start = "%4d-%02d-%02d %02d:%02d:%04.1f %15.6f %08X" %\
                (year, mon, day, hour, min, secs, self.JD0, self.stw0)
        return start

    def stoptime(self):
        (year, mon, day, hour, min, secs) = self.tc.cld(self.JD1)
        stop = "%4d-%02d-%02d %02d:%02d:%04.1f %15.6f %08X" %\
            (year, mon, day, hour, min, secs, self.JD1, self.stw1)
        return stop

    def findlevel0(self, ext):
        """Return level 0 files with given extension"""

        files = []
        if ext in self.files:
            files = self.files[ext]
        return files

    def doFBA(self, file):
        spectra = []
        odin.Info("next FBA file: %s" % (file))
        fba = odin.FBAfile(file)
        while True:
            try:
                s = fba.NextSpectrum()
                if s:
                    if s.stw > self.stw0 and s.stw <= self.stw1:
                        IFfreq = 3900.0e6
                        LOfreq = 114.8498600000000e+9
                        s.lofreq = LOfreq
                        s.skyfreq = LOfreq + IFfreq
                        s.maxsup = LOfreq - IFfreq
                        s.restfreq = s.skyfreq
                        WFREQUENCY = 0x00001000
                        s.quality = s.quality & ~WFREQUENCY
                        spectra.append(s)

            except EOFError:
                break
        odin.Info("processed %d spectra" % (len(spectra)))
        return spectra

    def getTemperature(self, which):
        stw = []
        T = []
        table = HKdata[which]
        for file in self.shk:
            hk = SHKfile(file)
            HK = hk.getHKword(table[0], sub=table[1])
            for i in range(len(HK[0])):
                stw.append(HK[0][i])
                T.append(HK[1][i])
        T = map(table[2], T)
        stw, T = removeDuplicates(stw, T)
        return (stw, T)

    def getLoadTemp(self):
        if not hasattr(self, 'temp'):
            self.temp = {}
        if 'hotLoad' not in self.temp:
            self.temp['hotLoad'] = self.getTemperature("hot load A-side")
            if len(self.temp['hotLoad'][0]) == 0:
                self.temp['hotLoad'] = self.getTemperature("hot load B-side")
        return

    def getChopper(self):
        files = self.findlevel0('AC2')
        if not files:
            odin.Warn("no AC2 files found for orbit %d" % (self.orbit))
            return None
        stw = []
        typ = []
        for file in files:
            odin.Info("next AC2 file: %s" % (file))
            ac2 = ACfile(file)
            while True:
                head = ac2.getSpectrumHead()
                if not head:
                    break
                stw.append(ac2.stw)
                beam = ac2.Chop(head)
                typ.append(beam)
        # chopper = removeDuplicates(stw,typ)
        return (stw, typ)

    def Lookup(self, table, stw0):
        i = self.ilast
        while i < len(table[0]) - 2 and table[0][i + 1] < stw0:
            i = i + 1
        self.ilast = i
        # print stw0,table[0][i],table[1][i],table[0][i+1],table[1][i+1]
        return table[1][i + 1]

    def accum(self, s):
        if s:
            sum = s[-1]
            if len(s) > 1:
                for a in s[0:-1]:
                    sum.inttime = sum.inttime + a.inttime
                    for i in range(3):
                        sum.data[i] = sum.data[i] + a.data[i]
                    for i in range(3):
                        sum.data[i] = sum.data[i] / len(s)
                sum.efftime = sum.inttime
            return sum
        else:
            return None

    def process(self):
        files = self.findlevel0(self.backend)
        all = []
        if not files:
            odin.Warn("no %s files found for orbit %d" %
                      (self.backend, self.orbit))
            return None
        shk = self.findlevel0('SHK')
        if not shk:
            odin.Warn("no SHK files found for orbit %d" % (self.orbit))
            return None
        self.shk = shk
        self.getLoadTemp()
        chopper = self.getChopper()
        oldtype = 'N/A'
        if chopper:
            for filename in files:
                spectra = self.doFBA(filename)
                for s in spectra:
                    s.tcal = Lookup(self.temp['hotLoad'], s.stw) + 273.15
                    chop = self.Lookup(chopper, s.stw)
                    if chop == 'SIG':
                        s.type = 'SIG'
                    all.append(s)
        else:
            odin.Warn("no chopper information")

        self.spectra = []
        if len(all) > 0:
            i0 = 0
            i1 = 0
            while i1 < len(all):
                if all[i0].type == all[i1].type:
                    i1 = i1 + 1
                else:
                    sum = self.accum(all[i0:i1])
                    if sum:
                        self.spectra.append(sum)
                    i0 = i1
            sum = self.accum(all[i0:i1])
            if sum:
                self.spectra.append(sum)

        # print "%10d %s %6.0f %s" % (s.stw,s.type,s.data[0],chop)
        odin.Info("total %d spectra" % (len(self.spectra)))

    def sortspec(self, spectra):
        sig = []
        ref = []
        cal = []
        for s in spectra:
            if s.type == 'SIG':
                sig.append(s)
            elif s.type == 'SK1' or s.type == 'SK2':
                ref.append(s)
            elif s.type == 'CAL':
                cal.append(s)

        return (sig, ref, cal)

    def median(self, a):
        b = a[:]
        n = len(b) / 2
        b.sort()
        return b[n]

    def medfilter(self, a, m):
        n = len(a)
        if n < m:
            return
        c = [0] * n
        c0 = self.median(a[0:2 * m + 1])
        for i in range(m):
            c[i] = c0

        for i in range(m, n - m):
            c[i] = self.median(a[i - m:i + m + 1])

        c0 = self.median(a[n - 2 * m - 1:n])
        for i in range(n - m, n):
            c[i] = c0
        for i in range(n):
            a[i] = c[i]

    def smooth(self, spectra, type):
        # print "smoothing", type
        r = [[], [], []]
        n = 0
        for s in spectra:
            if s.type == type:
                for i in range(3):
                    r[i].append(s.data[i])
                n = n + 1
        if n > 0:
            for i in range(3):
                self.medfilter(r[i], 5)
            n = 0
            for s in spectra:
                if s.type == type:
                    for i in range(3):
                        s.data[i] = r[i][n]
                    n = n + 1

    def Calibrate(self, spectra):
        VERSION = 0x0001
        odin.Info("calibrating using version %04x" % (VERSION))

        Tsys = 0.0
        ref = None
        cal = None
        for s in spectra:
            if s.type == 'SK1':
                ref = s.Copy()
            elif s.type == 'CAL' and ref:
                cal = s.Copy()
                Th = cal.tcal
                Tc = 2.7
                cal.CalcTsys(ref, Th, Tc)
                m = cal.Moment()
                Tsys = m[0]
                cal = s.Copy()
                cal.Gain(ref)
                break

        c = []
        k = 0
        while k < len(spectra) - 1:
            r = spectra[k]
            s = spectra[k + 1]
            # print k,r.type,s.type
            if r.type == 'SK1' and s.type == 'SIG':
                ref = r.Copy()
                if cal:
                    s.Calibrate(ref, cal)
                    s.type = 'SPE'
                    s.tsys = Tsys
                    s.efftime = s.inttime
                    s.level = s.level | VERSION
                    if len(c):
                        c.append(s)
                k = k + 2

            elif r.type == 'CAL' and s.type == 'SIG':
                if ref:
                    tsys = r.Copy()
                    Th = tsys.tcal
                    Tc = 2.7
                    tsys.CalcTsys(ref, Th, Tc)
                    m = tsys.Moment()
                    Tsys = m[0]
                    cal = r.Copy()
                    cal.Gain(ref)
                    tsys.type = 'CAL'
                    if len(c) == 0 or c[-1].type == 'SPE':
                        c.append(tsys)
                        ssb = tsys.Copy()
                        ssb.type = 'SSB'
                        n = len(ssb.data)
                        ssb.data = ones(n, Float)
                        c.append(ssb)
                k = k + 2
            else:
                k = k + 1
        return c

    def attitude(self):
        """Lookup attitude information for all spectra."""

        attfiles = self.findlevel0('ATT')
        if attfiles:
            ap = AttitudeParser(attfiles, self.stw0, self.stw1)

            # sorting attitudes
            tbl = ap.table
            keys = tbl.keys()
            if not keys:
                odin.Warn("no attitude info found for orbit %d" % (self.orbit))
                return

            keys.sort()
            key0 = keys[0]
            for key1 in keys[1:]:
                stw0 = tbl[key0][6]
                stw1 = tbl[key1][6]
                if stw1 - stw0 < 17:
                    print "stw: %d %d %d" % (stw0, stw1, stw1 - stw0)
                    qe0 = tbl[key0][10]
                    qe1 = tbl[key1][10]
                    if qe0 != qe1:
                        err0 = qe0[0] * qe0[0] + qe0[1] * \
                            qe0[1] + qe0[2] * qe0[2]
                        err1 = qe1[0] * qe1[0] + qe1[1] * \
                            qe1[1] + qe1[2] * qe1[2]
                        print "errors:", err0, err1
                        if err1 < err0:
                            del tbl[key0]
                            key0 = key1
                        else:
                            del tbl[key1]
                else:
                    key0 = key1

            rows = len(tbl.keys())
            cols = 2 + 2 * 4 + 3 + 6 + 1

            attstw = zeros(rows, Float)
            lookup = zeros((rows, cols), Float)

            odin.Info("setting up %d x %d lookup table" % lookup.shape)
            keys = sorted(tbl.keys())
            row = 0
            for key in keys:
                (year, mon, day, hour, min, secs, stw,
                 orbit, qt, qa, qe, gps, acs) = tbl[key]
                JD = self.tc.djl(year, mon, day, hour, min, secs)
                attstw[row] = float(stw % 0x100000000)
                lookup[row, 0: 2] = array((JD, orbit))
                lookup[row, 2: 6] = array(qt)
                lookup[row, 6:10] = array(qa)
                lookup[row, 10:13] = array(qe)
                lookup[row, 13:19] = array(gps)
                lookup[row, 19] = acs
                row = row + 1

            mins = (self.JD1 - self.JD0) * 1440.0
            odin.Info(
                "orbit %5d lasted for %5.2f minutes" %
                (self.orbit, mins))
            mins = (attstw[-1] - attstw[0]) / 16.0 / 60.0
            odin.Info("attitude available for %5.2f minutes" % (mins))

            if hasattr(self, 'spectra'):
                for s in self.spectra:
                    stw = float(s.stw) - s.inttime * 16.0 / 2.0
                    i = searchsorted(attstw, stw)
                    if i > 0 and i < row:
                        dt = attstw[i] - attstw[i - 1]
                        dt0 = attstw[i] - stw
                        dt1 = stw - attstw[i - 1]
                        # print dt0, dt1, dt
                        att = (dt0 * lookup[i - 1, :] +
                               dt1 * lookup[i, :]) / dt
                        t = (att[0], s.stw, att[1],
                             tuple(att[2:6]), tuple(att[6:10]),
                             tuple(att[10:13]), tuple(att[13:19]), att[19])
                        # print t
                        s.Attitude(t)
        else:
            odin.Warn("no ATT files found for orbit %d" % (self.orbit))

    def PDCname(self):
        bcode = 'D'
        ocode = "%04X" % (self.orbit)
        self.pdcfile = "O" + bcode + "1B" + ocode
        return self.pdcfile

    def LogScan(self, i, n0, n1, fmode, spectra):
        if not hasattr(self, 'logfile'):
            self.PDCname()
            logfile = self.pdcfile + '.LOG'
            self.logfile = open(logfile, 'w')

        total = len(spectra)
        s0 = spectra[n0]
        s1 = spectra[n1]
        sunZD = (s0.sunzd + s1.sunzd) / 2.0
        mjd = (s0.mjd + s1.mjd) / 2.0
        quality = 0
        line = "%3d\t%4d\t%4d\t%5lu\t" % (i, n0 + 1, n1 + 1, quality)
        if s0.latitude == 0.0 and s0.longitude == 0.0:
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
        else:
            line = line + "%7.2f\t%7.2f\t" % (s0.latitude, s0.longitude)
            line = line + "%7.2f\t%7.2f\t" % (s1.latitude, s1.longitude)
            line = line + "%7.0f\t%7.0f\t" % (s0.altitude, s1.altitude)
        if sunZD == 0.0:
            line = line + "\\N\t"
        else:
            line = line + "%6.2f\t" % (sunZD)
        line = line + \
            "%10.4f\t%3d\t%s\t%4d" % (mjd, fmode, self.pdcfile, total)
        self.logfile.write(line + '\n')


def usage():
    print "usage: %s orbit" % (sys.argv[0])


if __name__ == "__main__":
    if len(sys.argv) > 1:
        starttime = time.time()
        orbit = string.atoi(sys.argv[1])
        backend = "FBA"

        dir = odinpath.aeropath(orbit, backend, single=0)
        os.chdir(dir)

        op = FBAProcessor(orbit)
        filename = op.PDCname() + '.TXT'
        odin.LogAs("fba", filename)

        if not op.findlevel0('ATT'):
            odin.Warn("no ATT files found for orbit %d" % (orbit))
        else:
            op.process()

        if hasattr(op, 'spectra') and len(op.spectra) > 0:
            op.attitude()
            spectra = op.spectra

            (sig, ref, cal) = op.sortspec(spectra)
            if len(cal) == 0 or len(ref) == 0:
                if len(ref) == 0:
                    odin.Warn("no reference spectra found")
                if len(cal) == 0:
                    odin.Warn("no calibration spectra found")
                spectra = None
            else:
                op.smooth(spectra, 'CAL')
                op.smooth(spectra, 'SK1')
                spectra = op.Calibrate(spectra)

            if spectra:
                program = '/home/smr/C/Odin/Programs/whdf'
                filename = op.PDCname() + '.HDF'
                cmd = string.join([program, '-file', filename, '-list -'])
                print cmd
                output = os.popen(cmd, 'w')

                scan = 0
                spectrum = 0
                first = 0
                last = -1
                fm = 20
                for s in spectra:
                    # print spectrum, s.type
                    if s.type == 'CAL':
                        if scan > 0:
                            first = last + 1
                            last = spectrum
                            # print scan,spectrum,first,last,fm
                            op.LogScan(scan, first, last, fm, spectra)
                        scan = scan + 1
                    s.source = "Filterbank FM=20"
                    s.topic = "STRAT"
                    s.restfreq = s.skyfreq / (1.0 + 7.0 / 2.997924e5)
                    s.spectrum = spectrum
                    spectrum = spectrum + 1
                    file = s.Save()
                    output.write(file + '\n')

                output.close()
                cmd = string.join(["/bin/gzip", "-f", filename])
                print cmd
                os.system(cmd)
                if hasattr(op, 'logfile'):
                    op.logfile.close()

                filelog = open("/tmp/hdffiles", "a")
                gzipname = os.path.join(dir, filename) + ".gz"
                logname = string.replace(gzipname, "HDF.gz", "LOG")
                filelog.write(gzipname + '\n')
                filelog.write(logname + '\n')
                filelog.close()

                files = os.listdir(dir)
                filere = re.compile(r"[AF]..\.[0-9A-F]{8}\.[A-Z]{3}")
                for file in files:
                    if filere.match(file):
                        file = os.path.join(dir, file)
                        os.remove(file)

        del op
        stoptime = time.time()
        elapsed = stoptime - starttime
        odin.Info("processing orbit %d for FBA took %.3f seconds" %
                  (orbit, elapsed))

    else:
        usage()
