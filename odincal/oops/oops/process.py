# standard Python modules
import sys
import os
import string
import re
import math
import time

# third party Python modules
import pg
from Numeric import *

# Odin modules
import odin
import odinpath
# from astroorbit import AstroOrbit, precess
from orbit import OrbitProcessor


def bySTW(one, two):
    stw0, stw1 = one.stw, two.stw
    return cmp(stw0, stw1)


def offset(on, off):
    ra0 = on[0] * math.pi / 180.0
    de0 = on[1] * math.pi / 180.0
    print "on:  %10.4f %10.4f" % (ra0 * 180.0 / math.pi, de0 * 180.0 / math.pi)
    ra1 = off[0] * math.pi / 180.0
    de1 = off[1] * math.pi / 180.0
    print "off: %10.4f %10.4f" % (ra1 * 180.0 / math.pi, de1 * 180.0 / math.pi)
    delta = [ra1 - ra0, de1 - de0]
    delta[0] = delta[0] * math.cos(de0)
    return [delta[0] * 180.0 / math.pi, delta[1] * 180.0 / math.pi]


class SpectrumProcessor:
    """ Class for general spectrum processing. """

    def __init__(self, files=None):
        self.spectra = {}
        if files:
            for file in files:
                if os.path.isfile(file):
                    s = odin.Spectrum(file)
                    self.spectra[file] = s

    def medfilter(self, a, m):
        """ Apply median filer of wifth m to an array of values. """
        width = 2 * m + 1
        n = len(a)
        if n <= width:
            odin.Warn("not enough values for median filter")
            return a
        med = zeros((n, width), Float)
        for i in range(n):
            i1 = i - m
            if i1 < 0:
                i1 = 0
            elif i1 > n - width:
                i1 = n - width
            i2 = i1 + width
            if i2 > n:
                i2 = n

            # one row of med are 'width' adjacent values
            med[i] = array(a[i1:i2])

        # sort and pick the middle column (median values)
        med = sort(med)
        med = med[:, m]
        return med

    def median(self, a):
        """ Return the median value for a list of mean intensities. """
        b = sorted(a[:])
        n = len(b) / 2
        return b[n]

    def sortspec(self, spectra):
        """ Sort spectra into phases."""
        sig = []
        ref = []
        cal = []
        for s in spectra:
            if s.type == 'SIG':
                sig.append(s)
            elif s.type == 'SK1' or s.type == 'SK2' or s.type == 'REF':
                ref.append(s)
            elif s.type == 'CAL':
                cal.append(s)

        return (sig, ref, cal)

    def cleanref(self, ref):
        (rstw, rmat) = self.matrix(ref)
        ns = rmat.shape[0]
        if ns < 2:
            return []
        if ref[0].backend == 'AOS':
            mr = add.reduce(rmat, 1) / rmat.shape[1]
        else:
            rms = zeros(8, Float)
            for band in range(8):
                n = 112
                i0 = band * n
                i1 = i0 + n
                mr = add.reduce(rmat[:, i0:i1], 1) / float(n)
                mr = mr - add.reduce(mr) / float(ns)
                rms[band] = sqrt(add.reduce(mr * mr) / float(ns - 1))
            i = argsort(where(rms == 0.0, max(rms), rms))
            band = i[0]
            i0 = band * n
            i1 = i0 + n
            mr = add.reduce(rmat[:, i0:i1], 1) / float(n)

        n = len(mr)
        med = self.medfilter(mr, 2)

        for i in range(n - 1, -1, -1):
            if med[i] == 0.0 or abs((mr[i] - med[i]) / med[i]) > 0.001:
                # print "remove spectrum", i
                # del mr[i]
                del ref[i]
        del med
        del mr

        return ref

    def cleancal(self, cal, ref):
        (rstw, rmat) = self.matrix(ref)
        ns = rmat.shape[0]

        R = cal[0].Copy()
        if R.frontend == '119':
            Tmin = 400.0
            Tmax = 1300.0
        else:
            Tmin = 2500.0
            Tmax = 6000.0

        # k = 0
        for k in range(len(cal)):
            stw = float(cal[k].stw)
            i = searchsorted(rstw, stw)
            if i == 0:
                R.data = ref[0].data
            elif i == ns:
                R.data = ref[ns - 1].data
            else:
                dt0 = rstw[i] - stw
                dt1 = stw - rstw[i - 1]
                dt = rstw[i] - rstw[i - 1]
                R.data = (dt0 * ref[i - 1].data + dt1 * ref[i].data) / dt
            cal[k].Gain(R)

        (cstw, cmat) = self.matrix(cal)
        # mc = add.reduce(cmat,1)/cmat.shape[1]
        nc = cmat.shape[0]
        if nc < 5:
            for i in range(nc - 1, -1, -1):
                del cal[i]
            return cal

        if cal[0].backend == 'AOS':
            mc = add.reduce(cmat, 1) / cmat.shape[1]
        else:
            rms = zeros(8, Float)
            n = 112
            for band in range(8):
                i0 = band * n
                i1 = i0 + n
                mc = add.reduce(cmat[:, i0:i1], 1) / float(n)
                mc = mc - add.reduce(mc) / float(nc)
                rms[band] = sqrt(add.reduce(mc * mc) / float(nc - 1))
            i = argsort(where(rms == 0.0, max(rms), rms))
            i0 = i[0] * n
            i1 = i0 + n
            mc = add.reduce(cmat[:, i0:i1], 1) / float(n)

        # to start with, disgard any Tsys spectra which are clearly
        # too high or too low
        mc0 = take(mc, nonzero(less(mc, Tmax)))
        mc0 = take(mc0, nonzero(greater(mc0, Tmin)))
        n1 = len(mc0)
        odin.Info("%d (%d) cal measurements acceptable" % (n1, nc))

        # print "Tsys values", n1
        # print mc0
        if n1 == 0:
            tsys = 0.0
            cal = []
        else:
            tsys = sort(mc0)[n1 / 2]
            print "mean Tsys", tsys
            for i in range(nc - 1, -1, -1):
                if abs((mc[i] - tsys) / tsys) > 0.02:
                    # print "%10.3f %10.3f %10.3f" % \
                    # (tsys,mc[i],(mc[i]-tsys)/tsys)
                    del cal[i]

        del mc
        n2 = len(cal)
        odin.Info("kept %d (%d) cal measurements, median Tsys: %8.2fK" %
                  (n2, nc, tsys))

        return cal

    def matrix(self, spectra):
        """Return a matrix where each row is the data vector of one spectrum.
        Once the matrix is available, add.reduce(matrix,0)/matrix.shape[0]
        will return a mean spectrum, and add.reduce(matrix,1)/matrix.shape[1]
        will return a vector containing the mean intensity for each spectrum.
        """

        if len(spectra) == 0:
            return None

        rows = len(spectra)
        cols = spectra[0].channels
        stw = zeros(rows, Float)
        mat = zeros((rows, cols), Float)

        stw0 = 0
        for i in range(rows):
            s = spectra[i]
            if stw0 > s.stw:
                odin.Warn("spectrum out of order")
            mat[i, :] = s.data
            stw[i] = s.stw
            stw0 = s.stw

        return (stw, mat)

    def getDefault(self, ref):
        odin.Warn("using default calibration spectrum")
        rows = 2
        C = ref[0].Copy()
        C.data = 0.0 * C.data + 1.0
        cstw = zeros(rows, Float)
        cstw[0] = ref[0].stw
        cstw[1] = ref[-1].stw
        if C.backend == 'AOS':
            tsys = zeros(rows, Float)
        else:
            tsys = zeros((rows, 8), Float)
        if C.frontend == '119':
            tsys = tsys + 600
        else:
            tsys = tsys + 3500

        return (C, tsys, cstw)

    def getTsys(self, cal):
        (cstw, cmat) = self.matrix(cal)
        (rows, cols) = cmat.shape

        if self.backend == 'AOS':
            tsys = add.reduce(cmat, 1) / cols
            c = add.reduce(cmat) / rows
            mn = add.reduce(c) / cols
            if mn != 0.0:
                c = c / mn
        else:
            tsys = zeros((rows, 8), Float)
            for band in range(8):
                n = 112
                i0 = band * n
                i1 = i0 + n
                tsys[:, band] = add.reduce(cmat[:, i0:i1], 1) / n
            c = add.reduce(cmat) / rows
            for band in range(8):
                n = 112
                i0 = band * n
                i1 = i0 + n
                mn = add.reduce(c[i0:i1]) / n
                if mn != 0.0:
                    c[i0:i1] = c[i0:i1] / mn

        C = cal[0].Copy()
        # C.stw = self.stw0
        # C.orbit = self.orbit
        C.data = c

        return (C, tsys, cstw)

    def addpos(self, spectra):
        """ Average consecutive REF and CAL spectra. """
        if not spectra:
            return []
        oldtype = 'N/A'
        n = len(spectra)
        i = 0
        ave = []
        while i < n:
            s = spectra[i]
            if s.type == 'SIG':
                ave.append(s)
                i = i + 1
            else:
                total = 0.0
                t = s.inttime
                total = total + t
                B = s.Copy()
                B.data = s.data * t
                i = i + 1
                while i < n and spectra[i].type == B.type:
                    s = spectra[i]
                    t = s.inttime
                    total = total + t
                    B.data = B.data + s.data * t
                    i = i + 1
                B.data = B.data / total
                B.inttime = total
                ave.append(B)
        return ave

    def interpolate(self, mstw, m, stw):
        rows = m.shape[0]
        if rows != len(mstw):
            raise IndexError
        i = searchsorted(mstw, stw)
        if i == 0:
            return m[0]
        elif i == rows:
            return m[rows - 1]
        else:
            dt0 = float(mstw[i] - stw)
            dt1 = float(stw - mstw[i - 1])
            dt = float(mstw[i] - mstw[i - 1])
            return (dt0 * m[i - 1] + dt1 * m[i]) / dt

    def SSWcalibrate(self, sig, ref, cal):
        """Do intensity calibration for beam switching."""

        VERSION = 0x0001
        odin.Info("calibrating using version %04x" % (VERSION))

        if len(cal) > 0:
            (C, tsys, cstw) = self.getTsys(cal)
        else:
            (C, tsys, cstw) = self.getDefault(ref)
            cal = [C, C]

        ncal = len(cstw)

        (rstw, rmat) = self.matrix(ref)
        n = len(ref)
        R = ref[0].Copy()

        calibrated = []
        for s in sig:
            stw = float(s.stw)
            i = searchsorted(cstw, stw)
            if i == ncal:
                i = ncal - 1
            gain = cal[i].Copy()
            if self.backend == 'AOS':
                gain.data = C.data * tsys[i]
            else:
                for band in range(8):
                    n = 112
                    i0 = band * n
                    i1 = i0 + n
                    gain.data[i0:i1] = C.data[i0:i1] * tsys[i][band]

            R.data = self.interpolate(rstw, rmat, stw)
            T = self.interpolate(cstw, tsys, stw)

            # if self.backend == "AOS":
            #     gain.data = C.data*T
            #   # m = gain.Moment()
            #   # print "mean Tsys %10.3f   rms %10.3f" % (m[0], m[1])
            # else:
            #     for band in range(8):
            #         n = 112
            #         i0 = band*n
            #         i1 = i0+n
            #         gain.data[i0:i1] = C.data[i0:i1]*T[band]

            s.Calibrate(R, gain, 0.0)
            m = gain.Moment()
            # print "mean Tsys %10.3f   rms %10.3f" % (m[0], m[1])
            s.tsys = m[0]

            # set calibration version number
            s.level = s.level | VERSION

            calibrated.append(s)

        return calibrated

    def TPWcalibrate(self, sig, ref, cal):
        """Do intensity calibration for position switching."""

        VERSION = 0x0002
        odin.Info("calibrating using version %04x" % (VERSION))

        if len(cal) > 0:
            (C, tsys, cstw) = self.getTsys(cal)
        else:
            (C, tsys, cstw) = self.getDefault(ref)
            cal = [C, C]

        ncal = len(cstw)

        (rstw, rmat) = self.matrix(ref)
        [rows, cols] = rmat.shape
        rms = zeros(rows, Float)
        # the following array will count how often particular REF spectra
        # have been chosen by the calibration algorithm
        picks = zeros(rows, Int)

        n = len(ref)
        R = ref[0].Copy()

        calibrated = []
        dict = self.spectra
        spectra = dict.values()
        spectra.sort(bySTW)
        (t0, t1) = self.getSwitchTimes(spectra)
        print "median int.time: %10.3f, switch period: %10.3f" % (t0, t1)
        if t1 == 0.0:
            odin.Warn("failed to deduce position switching period")
        for s in sig:
            stw = float(s.stw)
            i = searchsorted(cstw, stw)
            if i == ncal:
                i = ncal - 1
            gain = cal[i].Copy()
            if self.backend == 'AOS':
                gain.data = C.data * tsys[i]
            else:
                for band in range(8):
                    n = 112
                    i0 = band * n
                    i1 = i0 + n
                    gain.data[i0:i1] = C.data[i0:i1] * tsys[i][band]

            for i in range(rows):
                dt = s.stw - rstw[i]
                # print "%10d %10d %10d" % (rstw[i], s.stw, dt)
                if abs(dt) < int(16 * t1 * 1.5):
                    # print "use"
                    # get a copy of the REF channels
                    rdata = rmat[i].copy()
                    # get the indices of channels where REF had zero value
                    zi = repeat(arange(cols), equal(rdata, 0))
                    if self.backend == 'AC1':
                        # always include 0...223 and 672...895 in index set
                        zi = concatenate([zi, arange(0, 2 * 112)])
                        zi = concatenate([zi, arange(6 * 112 + 1, 8 * 112)])
                    # set all channels with index in zi to 1.0 in order
                    # to avoid division by zero further down
                    put(rdata, zi, 1.0)
                    x = (s.data - rdata) / rdata
                    # print x
                    # in result, set all channels with index in zi to 0.0
                    put(x, zi, 0.0)
                    sx2 = add.reduce(x**2)
                    sx = add.reduce(x)
                    n = float(cols)
                    # print "rms[%4d] = %10.3f %12.5e %12.5e" % (i,n, sx, sx2)
                    rms[i] = sqrt((sx2 - sx**2 / n) / (n - 1.0))
                    # print "rms[%4d] = %12.5e" % (i,rms[i])
                else:
                    rms[i] = 0.0

            # find the indeces of non-zero results
            ind = nonzero(rms)
            # use those rms values to pick the best one
            use = take(rms, ind)
            # for i in ind:
            #     print "rms: %3d %10.6f" % (i, rms[i]),
            #     dt = s.stw-rstw[i]
            #     print "%10d %10d %10d" % (rstw[i], s.stw, dt)
            # for i in range(len(use)):
            #     print "use: %3d %10.6f" % (i, use[i])

            # get an array of sorted indeces
            if len(use) > 0:
                i = argsort(use)[0]
                # print "lowest rms:"
                # print "%3d %10.6f" % (i, use[i])

                # get an index into the original array
                i = ind[i]
                # print "original:"
                # print "%3d %10.6f" % (i, rms[i])

                picks[i] = picks[i] + 1
                # print "using ref spectrum[%4d]: %12.5e" % (i, rms[i])
                R.data = rmat[i]

                s.Calibrate(R, gain, 1.0)
                m = gain.Moment()
                # print "mean Tsys %10.3f   rms %10.3f" % (m[0], m[1])
                s.tsys = m[0]

                # set calibration version number
                s.level = s.level | VERSION

                calibrated.append(s)

        maxpick = max(picks)
        if maxpick > 10:
            odin.Warn("maximum reference pick: %d" % (maxpick))
        return calibrated

    def earthfilter(self, spectra):
        """ Filter out all spectra which have contributions from the
        Earth's atmosphere. """

        i = 0
        while i < len(spectra):
            s = spectra[i]
            if s.type == 'SK1' and s.skybeamhit & 0x0001:
                del spectra[i]
            elif s.type == 'SK2' and s.skybeamhit & 0x0010:
                del spectra[i]
            elif s.skybeamhit & 0x0100:
                del spectra[i]
            else:
                x = s.qerror[0]
                y = s.qerror[1]
                z = s.qerror[2]
                err = math.sqrt(x * x + y * y + z * z)
                if err > 120.0:
                    del spectra[i]
                else:
                    i = i + 1
        return spectra

    def getSwitchTimes(self, spectra):
        """ Determine the median integration time and the switch
        period between SIG and REF. The latter is deduced from looking
        at the autocorrelation of the spectrum types."""

        (t0, t1) = (0.0, 0.0)
        t = zeros(len(spectra), Float)
        p = zeros(len(spectra), Int)
        for i in range(len(spectra)):
            s = spectra[i]
            if s.type == 'SIG':
                p[i] = 1
            if s.type == 'CAL':
                p[i] = -1
            t[i] = s.inttime

        t0 = sort(t)[len(t) / 2]
        if sometrue(equal(p, 1)):
            a = add.reduce(p * p) / float(len(p))
            b = a
            i = 0
            while b <= a:
                a = b
                i = i + 1
                if i == len(p):
                    break
                b = add.reduce(p[i:] * p[:-i]) / float(len(p) - i)
                # print "%8.3f %2d" % (b, i)
            t1 = i * t0
        return (t0, t1)


class SourceProcessor:
    """ class for processing a number of orbits for given source. """

    def __init__(self, source, topic, orbits, backend, dir=None):
        self.source = source
        self.topic = topic
        self.orbits = orbits
        self.backend = backend
        if dir:
            self.dir = dir
        else:
            self.dir = source

    def setupDir(self):
        root = odinpath.astroroot(self.dir)
        self.root = root
        return root

    def orbitDir(self, orbit, create=0):
        dir = odinpath.astropath(self.root, self.backend, orbit)
        if create:
            caldir = odinpath.calibrated(dir)

        os.chdir(dir)
        return dir

    def findSource(self):
        db = pg.connect('odin', 'ymer', user='smr')
        source = re.compile(r"[\W]").sub(r".", self.source)
        query = "SELECT source,topic,lon,lat,vlsr FROM master"
        query = query + "\n\tWHERE source ~ '%s'" % (source)
        query = query + "\n\tAND topic = '%s'" % (self.topic)
        query = query + "\n\tORDER BY source"
        print query
        q = db.query(query)
        sources = []
        print "found:"
        for row in q.getresult():
            # if len(sys.argv) < 5:
            sources.append(row)
            source = row[0]
            topic = row[1]
            RA = float(row[2])
            DE = float(row[3])
            vsource = float(row[4])
            print "%s %s %12.6f %12.6f %7.1f" % \
                  (source, topic, RA, DE, vsource)

        if not sources:
            print "no such source '%s'" % (source)
            topic = "COMMIS"
            RA = 0.0
            DE = 0.0
            vsource = 0.0
        else:
            source = sources[0][0]
            topic = sources[0][1]
            RA = float(sources[0][2])
            DE = float(sources[0][3])
            vsource = float(sources[0][4])
        print "using:"
        print "%s %s %12.6f %12.6f %7.1f" % \
              (source, topic, RA, DE, vsource)

        self.RA = RA
        self.DE = DE
        self.V = vsource

    def getLevel1a(self, mode):
        for orbit in self.orbits:
            starttime = time.time()
            dir = self.orbitDir(orbit, 1)

            op = OrbitProcessor(orbit, self.backend, 'ASTR')
            odin.Info("start processing of orbit %d" % (orbit))

            if not op.findlevel0('ATT'):
                odin.Warn("no ATT files found for orbit %d" % (orbit))
                continue
            else:
                op.process(mode)
                if not hasattr(op, 'spectra') or not op.spectra:
                    odin.Warn("no spectra found for orbit %d" % (orbit))
                    continue

            op.attitude()
            spectra = op.spectra

            total = 0
            for rx in spectra.keys():
                for s in spectra[rx]:
                    if s.inttime < 1.0:
                        continue
                    dra = s.ra2000 - self.RA
                    dde = s.dec2000 - self.DE
                    dra = dra * math.cos(self.DE * math.pi / 180.0)
                    if self.RA == 0.0 and self.DE == 0.0:
                        dra = 0.0
                        dde = 0.0
                    # print "%12.4f %12.4f" % (dra*60.0, dde*60.0)
                    if abs(dra) < 2.0 and abs(dde) < 2.0:
                        s.source = self.source
                        s.topic = self.topic
                        s.vsource = self.V * 1000.0
                        dv = s.vlsr + s.vsource
                        s.restfreq = s.skyfreq / (1.0 - dv / 2.99792456e8)
                        # print "%8.2f %8.2f %12.6f" % \
                        # (s.vsource/1000.0, s.vlsr/1000.0, s.restfreq/1.0e9)
                        s.xoff = dra
                        s.yoff = dde
                        s.tilt = 0.0
                        total = total + 1
                        s.Save()
#                    else:
#                        print "spectrum (%5.1f,%5.1f) not at (%5.1f,%5.1f)" % \
#                              (s.ra2000, s.dec2000, self.RA, self.DE)
#                        print "offset exceeds 2 deg: not storing"

            odin.Info("found %d spectra for '%s'" % (total, self.source))
            stoptime = time.time()
            elapsed = stoptime - starttime
            odin.Info("processing orbit %d for %s took %.3f seconds" %
                      (orbit, self.backend, elapsed))

        del op

    def spectrumFiles(self, dir):
        files = sorted(os.listdir(dir))
        filere = re.compile(r"[AF]..\.[0-9A-F]{8}\.[0-9A-Z]{3}")
        specnames = []
        for file in files:
            if filere.match(file):
                specnames.append(file)
        specnames.sort()
        return specnames

    def sortSSW(self):
        for orbit in self.orbits:
            print "sort positions for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            s = odin.Spectrum()
            files = self.spectrumFiles(dir)
            for file in files:
                fullname = os.path.join(dir, file)
                s.Load(fullname)
                # print fullname, s.obsmode, s.type
                if s.obsmode != 'SSW':
                    print "wrong mode, delete", file
                    os.remove(file)
#                else:
#                    d = math.sqrt(s.xoff*s.xoff + s.yoff*s.yoff)
#                    if d > 2.0:
#                        print "distance from ON  %12.4f" % (d*60.0)
#                        print "delete", file
#                        os.remove(file)

    def sortTPW(self, offset):
        for orbit in self.orbits:
            print "sort positions for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            s = odin.Spectrum()
            files = self.spectrumFiles(dir)
            for file in files:
                fullname = os.path.join(dir, file)
                s.Load(fullname)
                # print fullname, s.obsmode, s.type
                if s.obsmode != 'TPW':
                    print "wrong mode, delete", file
                    os.remove(file)
#                else:
#                    d0 = math.sqrt(s.xoff*s.xoff + s.yoff*s.yoff)
                    # print "distance from source %12.4f %12.4f %12.4f" \
                    # % (s.xoff,s.yoff,d0*60.0)
#                    if d0 < 1.0/60.0:
#                        if s.type == 'REF':
#                             os.remove(file)
#                             s.type = 'SIG'
#                             s.Save()
#                        pass
#                    elif d0 < 2.0:
#                        if s.type == 'SIG':
#                            os.remove(file)
#                            s.type = 'REF'
#                            s.Save()
#                        pass
#                    else:
#                        print "distance from ON  %12.4f" % (d0*60.0)
#                        print "delete", file
#                        os.remove(file)

    def findCAL(self):
        for orbit in self.orbits:
            print "find calibrations for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            files = self.spectrumFiles(dir)
            if not files:
                continue

            n = len(files)
            k = 0
            mean = 0.0
            for i in range(n):
                fullname = os.path.join(dir, files[i])
                s = odin.Spectrum(fullname)
                # print "%6.2f %04X" % (s.inttime, s.skybeamhit)
                if s.inttime >= 3.0:
                    if not s.skybeamhit & 0x0100:
                        m = s.Moment()
                        mean = mean + m[0]
                        k = k + 1
                        # print "new mean: %6.3f %10.3f %4d %6.3f" % \
                        #       (m[0], mean, k, mean/k)

            if k == 0:
                continue

            mean = mean / k
            print "mean intensity for", k, "ONs and OFFs:", mean

            for i in range(n):
                fullname = os.path.join(dir, files[i])
                s = odin.Spectrum(fullname)
                if s.inttime < 3.0:
                    if not s.skybeamhit & 0x0100:
                        m = s.Moment()
                        if m[0] > 1.05 * mean:
                            if s.type != 'CAL':
                                print "found CAL spectrum", fullname
                                os.remove(fullname)
                                s.type = 'CAL'
                                s.Save()

    def clean(self):
        for orbit in self.orbits:
            print "clean orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            files = self.spectrumFiles(dir)
            pp = SpectrumProcessor(files)
            dict = pp.spectra
            spectra = dict.values()
            print "length before earth filter:", len(spectra)
            spectra = pp.earthfilter(spectra)
            print "length after  earth filter:", len(spectra)
            for s in spectra:
                dra = s.xoff
                dde = s.yoff
                if dra * dra + dde * dde > 4.0:
                    spectra.remove(s)
            print "length after offset filter:", len(spectra)

            spectra.sort(bySTW)
            (sig, ref, cal) = pp.sortspec(spectra)
            print "lengths before cleanref: ", len(sig), len(ref), len(cal)
            if len(ref) > 0:
                ref = pp.cleanref(ref)
            print "lengths after cleanref:  ", len(sig), len(ref), len(cal)
            if len(cal) > 0 and len(ref) > 0:
                cal = pp.cleancal(cal, ref)
            print "lengths after cleancal:  ", len(sig), len(ref), len(cal)
            caldir = odinpath.calibrated(dir)
            os.chdir(caldir)
            for file in dict.keys():
                s = dict[file]
                if s.type == 'REF' or s.type == 'SK1' or s.type == 'SK2':
                    if s not in ref:
                        fullname = os.path.join(dir, file)
                        # print "remove", fullname
                        os.remove(fullname)
                elif s.type == 'CAL':
                    if s not in cal:
                        fullname = os.path.join(dir, file)
                        # print "remove", fullname
                        os.remove(fullname)
                    else:
                        # fullname = os.path.join(dir,file)
                        # s.FreqSort()
                        # if s.backend == 'AOS':
                        #     s.Redres(0.62e6)
                        s.Save()
                        # print "keep", fullname
                        # sys.exit(1)

    def addpos(self):
        for orbit in self.orbits:
            print "add CAL/REFs for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            files = self.spectrumFiles(dir)
            pp = SpectrumProcessor(files)
            dict = pp.spectra
            spectra = dict.values()
            spectra.sort(bySTW)
            spectra = pp.addpos(spectra)
            for file in dict.keys():
                s = dict[file]
                if s not in spectra:
                    fullname = os.path.join(dir, file)
                    os.remove(fullname)
                    print "remove", fullname

    def calibrate(self, mode):
        cal = []
        for orbit in self.orbits:
            print "find calibrations for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            caldir = odinpath.calibrated(dir)
            files = self.spectrumFiles(caldir)
            for file in files:
                fullname = os.path.join(caldir, file)
                c = odin.Spectrum(fullname)
                if c.type == 'CAL':
                    cal.append(c)

        ncal = len(cal)
        if ncal == 0:
            odin.Warn("no calibration spectra found")
            # return None
        odin.Info("total of %d calibration spectra found" % (ncal))

        for orbit in self.orbits:
            print "calibrate orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue
            caldir = os.path.join(dir, 'calibrated')

            files = self.spectrumFiles(dir)
            pp = SpectrumProcessor(files)
            dict = pp.spectra
            spectra = dict.values()
            spectra = pp.earthfilter(spectra)
            spectra.sort(bySTW)
            # spectra = pp.addpos(spectra)
            pp.backend = self.backend

            (sig, ref, c) = pp.sortspec(spectra)
            if len(sig) > 0 and len(ref) > 0:
                if mode == 'SSW':
                    calibrated = pp.SSWcalibrate(sig, ref, cal)
                else:
                    calibrated = pp.TPWcalibrate(sig, ref, cal)

                print "storing spectra in", caldir
                os.chdir(caldir)
                for c in calibrated:
                    c.FreqSort()
                    if c.backend == 'AOS':
                        c.Redres(0.62e6)
                    c.Save()
            else:
                if len(sig) == 0:
                    odin.Warn("no SIG spectra in orbit %d" % (orbit))
                if len(ref) == 0:
                    odin.Warn("no REF spectra in orbit %d" % (orbit))
                calibrated = []
            odin.Info("total of %d calibrated spectra for orbit %d" %
                      (len(calibrated), orbit))

    def FITS(self, level):
        program = '/home/smr/C/Odin/Programs/wfits'
        for orbit in self.orbits:
            print "write FITS file for orbit", orbit
            dir = self.orbitDir(orbit)
            if not dir:
                continue

            if level == '1B':
                dir = os.path.join(dir, 'calibrated')
                os.chdir(dir)

            files = self.spectrumFiles(dir)
            if files:
                if self.backend == 'AOS':
                    bcode = 'A'
                elif self.backend == 'AC1':
                    bcode = 'B'
                elif self.backend == 'AC2':
                    bcode = 'C'

                ocode = "%04X" % (orbit)
                filename = "O" + bcode + level + ocode + ".FIT"

                cmd = string.join([program, '-file', filename, '-list -'])
                print cmd
                output = os.popen(cmd, 'w')
                for file in files:
                    output.write(file + '\n')
                output.close()

                cmd = string.join(["/bin/gzip", "-f", filename])
                print cmd
                os.system(cmd)

                filelog = open("/tmp/fitsfiles", "a")
                gzipname = os.path.join(dir, filename) + ".gz"
                filelog.write(gzipname + '\n')
                filelog.close()
