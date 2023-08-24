#!/usr/bin/python

# import pdb

# standard Python modules
import sys
import os
import string
import math
import time

# third party Python modules
import pg
from numpy import *

# Odin modules
import odin
import odinpath
from level0 import *
from odintime import TimeConverter
from attitude import AttitudeParser


def bySTW(one, two):
    stw0, stw1 = one.stw, two.stw
    return cmp(stw0, stw1)


class OrbitProcessor:
    """A class to produce calibrated level 1B spectra for a given orbit,
    a given backend (AC1, AC2 or AOS) and a given discipline (ASTR or AERO).

    The script assumes that necessary level 0 files are stored in the 'smr'
    database."""

    def __init__(self, orbit, backend, mode):
        self.orbit = orbit
        self.backend = backend
        self.mode = mode
        self.tc = TimeConverter()
        self.JD0 = self.tc.orbit2JD(orbit)
        self.JD1 = self.tc.orbit2JD(orbit + 1)
        self.stw0 = self.tc.JD2stw(self.JD0)
        self.stw1 = self.tc.JD2stw(self.JD1)
        # print self.orbit, self.stw0,self.stw1,self.JD0,self.JD1
        # print a
        self.soda = 0.0
        print "looking for", backend, "spectra in orbit", orbit
        print "starting:", self.starttime()
        print "ending:  ", self.stoptime()
        if 0:
            """Find level 0 files which provide data for the orbit in question."""
            db = pg.connect('smr', 'ymer', user='smr')
            query = "SELECT ext,file FROM level0"
            query = query + "\n\tWHERE start < %d" % (self.stw1)
            query = query + " AND stop > %d" % (self.stw0)
            query = query + "\n\tORDER BY file"
            # print query
            q = db.query(query)
            # print q.getresult()
            self.files = {}
            for row in q.getresult():
                ext = row[0]
                if ext not in self.files:
                    self.files[ext] = []
                self.files[ext].append(row[1])
            db.close()
        else:
            self.files = {}
            self.files['SHK'] = []
            self.files['AC1'] = []
            self.files['ATT'] = []
            self.files['ACS'] = []
            self.files['AC2'] = []
            self.files['FBA'] = []
            self.files['SHK'].append('/home/bengt/Downloads/149a04f5.shk')
            self.files['AC1'].append('/home/bengt/Downloads/149a04f1.ac1')
            self.files['ATT'].append('/home/bengt/Downloads/149a3d2a.att')
            self.files['ATT'].append('/home/bengt/Downloads/149b7a86.att')
            self.files['ATT'].append('/home/bengt/Downloads/149b8f85.att')
            self.files['ACS'].append('/home/bengt/Downloads/149a04f2.acs')
            self.files['AC2'].append('/home/bengt/Downloads/149a04f1.ac2')
            self.files['FBA'].append('/home/bengt/Downloads/149a04f2.fba')

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
        """Find level 0 files with given extension which provide
        data for the orbit in question."""

        files = []
        if ext in self.files:
            files = self.files[ext]
        return files

    def tuning(self, s):
        """Lookup frequencies for given spectrum."""

        if s.frontend == '119':
            IFfreq = 3900.0e6
            LOfreq = 114.8498600000000e+9
            self.FCalibrate(s, LOfreq, IFfreq)
            return

        rxs = {'495': 1, '549': 2, '555': 3, '572': 4}
        if s.frontend not in rxs.keys():
            return

        if not self.shk:
            IFfreq = 3900.0e6
            LOfreq = float(s.frontend) * 1.0e9
            self.FCalibrate(s, LOfreq, IFfreq)
            return

        rx = s.frontend
        index = rxs[rx]
        if not hasattr(self, 'rx'):
            self.rx = {}
        if s.frontend not in self.rx:
            shk = self.shk

            stw0 = []
            stw1 = []
            stw2 = []
            freq = []
            mech = []
            mixC = []
            for file in shk:
                odin.Info("next SHK file: %s" % (file))
                hk = SHKfile(file)
                LO = hk.getLOfreqs()
                if rx == '495' or rx == '549':
                    for i in range(len(LO[0])):
                        stw0.append(LO[0][i])
                        freq.append(LO[index][i])
                if rx == '555' or rx == '572':
                    for i in range(len(LO[3])):
                        stw0.append(LO[3][i])
                        freq.append(LO[index + 1][i])
                SSB = hk.getSSBtunings()
                for i in range(len(SSB[0])):
                    stw1.append(SSB[0][i])
                    mech.append(SSB[index][i])
                mixer = "mixer current %s" % (rx)
                table = HKdata[mixer]
                mixer = hk.getHKword(table[0], sub=table[1])
                for i in range(len(mixer[0])):
                    stw2.append(mixer[0][i])
                    mixC.append(mixer[1][i])

            LOtable = removeDuplicates(stw0, freq)
            SSBtable = removeDuplicates(stw1, mech)
            mixC = map(table[2], mixC)
            mixtable = removeDuplicates(stw2, mixC)

            self.rx[rx] = (LOtable, SSBtable, mixtable)
        else:
            LOtable = self.rx[rx][0]
            SSBtable = self.rx[rx][1]
            mixtable = self.rx[rx][2]

        if len(LOtable[0]):
            LOfreq = Lookup(LOtable, s.stw)
            if LOfreq:
                ssb = Lookup(SSBtable, s.stw)
                (IFfreq, sbpath) = getSideBand(rx, LOfreq, ssb)
                s.sbpath = sbpath / 1.0e6
#                (AC_SPLIT, AC_UPPER) = (0x0010, 0x0020)
                (ADC_SPLIT, ADC_UPPER) = (0x0200, 0x0400)
                if s.intmode & ADC_SPLIT:
                    if s.backend == 'AC1':
                        #                        if s.intmode & AC_UPPER: IFfreq = IFfreq*3.6/3.9
                        if s.intmode & ADC_UPPER:
                            IFfreq = IFfreq * 3.6 / 3.9
                        else:
                            IFfreq = IFfreq * 4.2 / 3.9
                    elif s.backend == 'AC2':
                        #                        if s.intmode & AC_UPPER: IFfreq = IFfreq*4.2/3.9
                        if s.intmode & ADC_UPPER:
                            IFfreq = IFfreq * 4.2 / 3.9
                        else:
                            IFfreq = IFfreq * 3.6 / 3.9
                current = Lookup(mixtable, s.stw)
                if current < 0.25:
                    odin.Warn("low mixer current %5.2f" % (current))
                    if rx != '572':
                        LOfreq = 0.0
            else:
                IFfreq = 0.0
                odin.Warn("LO frequency lookup failed")
            self.FCalibrate(s, LOfreq, IFfreq)

    def getTemperature(self, which):
        """Lookup given temperature in HK data and return a table
        of satellite time words and temperature readings."""

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

    def medfilter(self, a, m):
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
        if len(ref) == 0:
            return
        (rstw, rmat) = self.matrix(ref)
        ns = rmat.shape[0]
        if ref[0].backend == 'AOS':
            mr = add.reduce(rmat, 1) / rmat.shape[1]
        else:
            n = 112
            bands = ref[0].channels / n
            rms = zeros(bands, Float)
            for band in range(bands):
                i0 = band * n
                i1 = i0 + n
                mr = add.reduce(rmat[:, i0:i1], 1) / float(n)
                mr = mr - add.reduce(mr) / float(ns)
                rms[band] = sqrt(add.reduce(mr * mr) / float(ns - 1))
            # print rms
            i = argsort(where(rms == 0.0, max(rms), rms))
            # print "best  band", i[0]
            # print "worst band", i[7]
            i0 = i[0] * n
            i1 = i0 + n
            mr = add.reduce(rmat[:, i0:i1], 1) / float(n)

        n = len(mr)
        med = self.medfilter(mr, 2)

        for i in range(n - 1, -1, -1):
            if med[i] == 0.0 or abs((mr[i] - med[i]) / med[i]) > 0.001:
                del ref[i]
        del med
        del mr

        n2 = len(ref)
        odin.Info("kept %d (%d) ref measurements" % (n2, n))

        return ref

    def acceptableTsys(self, rx):
        if rx == '119':
            Tmin = 400.0
            Tmax = 1500.0
        else:
            Tmin = 2000.0
            Tmax = 7000.0
        return (Tmin, Tmax)

    def cleancal(self, cal, ref):
        if len(ref) == 0 or len(cal) == 0:
            return
        (rstw, rmat) = self.matrix(ref)
        ns = rmat.shape[0]

        R = cal[0].Copy()
        (Tmin, Tmax) = self.acceptableTsys(R.frontend)

        for k in range(len(cal)):
            stw = float(cal[k].stw)
            # find reference data by table lookup
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

            # calculate a Tsys spectrum from given C and R
            cal[k].Gain(R)

        # combine CAL spectra into matrix
        (cstw, cmat) = self.matrix(cal)
        nc = cmat.shape[0]
        if cal[0].backend == 'AOS':
            mc = add.reduce(cmat, 1) / cmat.shape[1]
        else:
            n = 112
            bands = cal[0].channels / n
            rms = zeros(bands, Float)
            for band in range(bands):
                i0 = band * n
                i1 = i0 + n
                mc = add.reduce(cmat[:, i0:i1], 1) / float(n)
                # print "mc =", mc, nc, len(nonzero(mc))
                if len(nonzero(mc)) < nc or nc < 2:
                    rms[band] = 1.0e10
                else:
                    mc = mc - add.reduce(mc) / float(nc)
                    rms[band] = sqrt(add.reduce(mc * mc) / (float(nc - 1)))
                print "rms of band", band, " =", rms[band]
            i = argsort(where(rms == 0.0, max(rms), rms))
            i0 = i[0] * n
            i1 = i0 + n
            # mc = add.reduce(cmat[:,i0:i1],1)/float(n)
            mc = add.reduce(cmat, 1) / cmat.shape[1]

        # to start with, disgard any Tsys spectra which are clearly
        # too high
        # print mc
        mc0 = take(mc, nonzero(less(mc, Tmax)))
        mc0 = take(mc0, nonzero(greater(mc0, Tmin)))
        n1 = len(mc0)
        odin.Info("%d (%d) cal measurements acceptable" % (n1, nc))
        if n1 == 0:
            tsys = 0.0
            cal = []
        else:
            tsys = sort(mc0)[n1 / 2]
            print "mean Tsys", tsys
            for i in range(nc - 1, -1, -1):
                if mc[i] < Tmin or mc[i] > Tmax or abs(
                        (mc[i] - tsys) / tsys) > 0.02:
                    # print "%10.3f %10.3f %10.3f" % \
                    # (tsys,mc[i],(mc[i]-tsys)/tsys)
                    del cal[i]

        del mc
        n2 = len(cal)
        odin.Info("kept %d (%d) cal measurements, median Tsys: %8.2fK" %
                  (n2, nc, tsys))

        return cal

    def matrix(self, spectra):
        if len(spectra) == 0:
            return None

        spectra.sort(bySTW)
        rows = len(spectra)
        cols = spectra[0].channels
        stw = zeros(rows, Float)
        mat = zeros((rows, cols), Float)

        for i in range(rows):
            s = spectra[i]
            if s.data.shape[0] == cols:
                mat[i, :] = s.data
            else:
                odin.Warn("number of channels mismatch")
            stw[i] = s.stw

        return (stw, mat)

    def fitTsys(self, cal):
        """ Fit a third order polynomial to a measured gain (Tsys) curve. """

        tsys = cal.Copy()
        (Tmin, Tmax) = self.acceptableTsys(tsys.frontend)

        tsys.FreqSort()
        # sorted, normalised frequency array
        f = tsys.Frequency() - tsys.skyfreq / 1.0e9
        y = tsys.data

        n = len(y)
        i = where(greater(y, Tmax), zeros(n), ones(n))
        i = where(less(y, Tmin), zeros(n), i)
        i = nonzero(i)
        f = take(f, i)
        y = take(y, i)

        try:
            c = odin.Polyfit(f, y, 3)
        except BaseException:
            m = tsys.Moment()
            c = [m[0], 0.0, 0.0, 0.0]
        # c = odin.Polyfit(f,y,1)
        # print "polynomial coefficients:", c

        f = tsys.Frequency() - tsys.skyfreq / 1.0e9
        y = tsys.data
        z = c[0] + (c[1] + (c[2] + c[3] * f) * f) * f
        # z = c[0] + c[1]*f

        # return to non-sorted spectrum
        tsys = cal.Copy()
        f = tsys.Frequency() - tsys.skyfreq / 1.0e9
        y = tsys.data
        z = c[0] + (c[1] + (c[2] + c[3] * f) * f) * f
        # z = c[0] + c[1]*f

        tsys.data = z
        return tsys

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
                i = i + 1
        return spectra

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

    def medTsys(self, cal):
        C = cal[0].Copy()
        (Tmin, Tmax) = self.acceptableTsys(C.frontend)

        (cstw, cmat) = self.matrix(cal)
        if C.backend == 'AOS':
            C.data = add.reduce(cmat) / float(cmat.shape[0])
        else:
            n = 112
            bands = C.channels / n
            for band in range(bands):
                c = zeros(n, Float)
                i0 = band * n
                i1 = i0 + n
                k = 0
                for i in range(cmat.shape[0]):
                    d = cmat[i, i0:i1]
                    if len(nonzero(d)) == 112:
                        tsys = add.reduce(d) / 112.0
                        if tsys > Tmin and tsys < Tmax:
                            c = c + d
                            k = k + 1
                if k > 0:
                    c = c / float(k)
                C.data[i0:i1] = c

        # sort along each row, i.e channel
#        sorted = transpose(sort(transpose(cmat)))
#        n = int(sorted.shape[0]/2)
#        C.data = sorted[n]
#        if sorted.shape[0] % 1:
#            print "odd"
#            # take the middle row of the sorted spectra
#            # which will be the median for each channel
#            C.data = sorted[n]
#        else:
#            print "even"
#            # for even number of rows we take the average of two rows
#            # C.data = (sorted[n-1]+sorted[n])/2.0
#            C.data = sorted[n-1]

#        print "medTsys", C
        return C

    def medTsys2(self, cal):
        C = cal[0].Copy()
        (Tmin, Tmax) = self.acceptableTsys(C.frontend)
        (cstw, cmat) = self.matrix(cal)

        if C.backend == 'AOS':
            order = 5
            n = cmat.shape[1]
            x = arange(float(n)) / float(n)
            y = zeros(n, Float)
            k = 0
            for i in range(cmat.shape[0]):
                d = cmat[i]
                tsys = add.reduce(d) / float(n)
                if tsys > Tmin and tsys < Tmax:
                    y = y + d
                    k = k + 1
            if k > 0:
                y = y / float(k)
                c = odin.Polyfit(x, y, order)
                for i in range(order + 1):
                    print "%10.4f " % (c[i]),
                print
                y = zeros(n, Float)
                for i in range(order + 1):
                    y = y * x
                    y = y + c[order - i]
                C.data = y
            else:
                C.data = (Tmin + Tmax) / 2.0

        else:
            order = 3
            n = 112
            x = arange(float(n)) / float(n)
            for band in range(8):
                i0 = band * n
                i1 = i0 + n
                y = zeros(n, Float)
                k = 0
                for i in range(cmat.shape[0]):
                    d = cmat[i, i0:i1]
                    tsys = add.reduce(d) / float(n)
                    if tsys > Tmin and tsys < Tmax:
                        y = y + d
                        k = k + 1
                if k > 0:
                    y = y / float(k)
                    c = odin.Polyfit(x, y, order)
                    for i in range(order + 1):
                        print "%10.4f " % (c[i]),
                    print
                    y = zeros(n, Float)
                    for i in range(order + 1):
                        y = y * x
                        y = y + c[order - i]
                    C.data[i0:i1] = y
                else:
                    C.data[i0:i1] = (Tmin + Tmax) / 2.0

        return C

    def getTsys(self, cal):
        (cstw, cmat) = self.matrix(cal)
        (rows, cols) = cmat.shape

        if self.backend == 'AOS':
            # calculate mean Tsys for each spectrum
            tsys = add.reduce(cmat, 1) / cols
            # calculate mean Tsys spectrum
            c = add.reduce(cmat) / rows
            mn = add.reduce(c) / cols
            # calculate normalised Tsys spectrum
            if mn != 0.0:
                c = c / mn
        else:
            n = 112
            bands = cal[0].channels / n
            tsys = zeros((rows, bands), Float)
            for band in range(bands):
                i0 = band * n
                i1 = i0 + n
                tsys[:, band] = add.reduce(cmat[:, i0:i1], 1) / n
            c = add.reduce(cmat) / rows
            for band in range(bands):
                i0 = band * n
                i1 = i0 + n
                mn = add.reduce(c[i0:i1]) / n
                if mn != 0.0:
                    c[i0:i1] = c[i0:i1] / mn

        C = cal[0].Copy()
        C.stw = self.stw0
        C.orbit = self.orbit
        C.data = c

        return (C, tsys, cstw)

    def ICalibrate(self, spectra, rx):
        """Perform intensity calibration."""

        VERSION = 0x0001
        calibrated = []
        if len(spectra) == 0:
            odin.Warn("no spectra found for receiver %s" % (rx))
            return calibrated

        ref = None
        cal = None
        sky = None

        # print "number of spectra before earth filter:", len(spectra)
        # if self.mode == 'ASTR':
        #    spectra = self.earthfilter(spectra)
        # print "number of spectra after  earth filter:", len(spectra)

        if len(spectra) == 0:
            return calibrated

        (sig, ref, cal) = self.sortspec(spectra)
        ref = self.cleanref(ref)
        if len(ref) == 0:
            odin.Warn("no reference spectra found for receiver %s" % (rx))
            return calibrated
        if len(cal) == 0:
            odin.Warn("no calibration spectra found for receiver %s" % (rx))
            return calibrated

        cal = self.cleancal(cal, ref)
        # (C,tsys,cstw) = self.getTsys(cal)
        # ncal = len(cstw)
        # if ncal == 0:
        #    odin.Warn("no calibration spectra found after filter")
        #     return None
        C = self.medTsys(cal)
        Tsys = take(C.data, nonzero(C.data))
        Tsys = add.reduce(Tsys) / float(Tsys.shape[0])

        (rstw, rmat) = self.matrix(ref)

        n = len(ref)
        R = ref[0].Copy()
        # ms = []
        # o = []

        # gain = cal[0].Copy()
        # if self.backend == 'AOS':
        #     gain.data = C.data*add.reduce(tsys)/ncal
        # else:
        #     for band in range(8):
        #         n = 112
        #         i0 = band*n
        #         i1 = i0+n
        #        gain.data[i0:i1] = C.data[i0:i1]*add.reduce(tsys[:,band])/ncal
        #  calibrated.append(gain)

        for s in sig:
            stw = float(s.stw)
            R.data = self.interpolate(rstw, rmat, stw)
            s.Calibrate(R, C)
            s.tsys = Tsys

            # set calibration version number
            s.level = s.level | VERSION

            calibrated.append(s)

        return calibrated

    def FCalibrate(self, s, LOfreq, IFfreq):
        """Perform frequency calibration."""

        if LOfreq == 0.0:
            return

        # print "LO", LOfreq, " IF", IFfreq, s.freqcal
        WFREQUENCY = 0x00001000
        quality = s.quality & ~WFREQUENCY
        rxs = {'495': 1, '549': 2, '555': 3, '572': 4}
        if s.frontend in rxs.keys() and self.shk:
            if not hasattr(self, 'temp'):
                self.temp = {}
            if 'PllBox' not in self.temp:
                self.temp['PllBox'] = self.getTemperature("image load B-side")
            Tplltable = self.temp['PllBox']
            if len(Tplltable[0]):
                Tpll = Lookup(Tplltable, s.stw)
            else:
                Tpll = 21.0
                quality = quality | WFREQUENCY
            if s.frontend == '495' or s.frontend == '549':
                drift = 1.0 + (29.23 - 0.138 * Tpll) * 1.0e-6
            else:
                drift = 1.0 + (24.69 - 0.109 * Tpll) * 1.0e-6
            LOfreq = LOfreq * drift
        else:
            quality = quality | WFREQUENCY

        s.lofreq = LOfreq
        s.skyfreq = LOfreq + IFfreq
        s.maxsup = LOfreq - IFfreq
        s.restfreq = s.skyfreq
        s.quality = quality

    def checked(self, s):
        """Do a quick check on given spectrum."""

        if s.discipline != self.mode:
            odin.Warn("no discipline")
            s.discipline = self.mode
            # return 0
        # aeronomy data are always beam switched
        if s.discipline == 'AERO' and s.obsmode == 'TPW':
            odin.Warn("TPW mode not supported for aeronomy")
            return 0
        # aeronomy data are always 8 band spectra (intmode = 511)
        if s.discipline == 'AERO' and (s.intmode & 0x01ff) != 511:
            odin.Warn("intmode %d not supported for aeronomy" % (s.intmode))
            return 0
        if s.inttime == 0.0:
            odin.Warn("zero integration time")
            return 0
        # for all but CMB and DRK spectra we require valid LO freq
        if s.type != 'CMB' and s.type != 'DRK':
            if s.lofreq == 0.0:
                odin.Warn("no frequency for receiver %s" % (s.frontend))
                return 0
        return 1

    def doAC(self, file):
        """Extract AC1 or AC2 spectra from level 0 files."""

        spectra = []
        odin.Info("next AC  file: %s" % (file))
        ac = odin.ACfile(file)
        stw0 = self.stw0 % 0x100000000
        stw1 = self.stw1 % 0x100000000
        while True:
            try:
                s = ac.NextSpectrum()
                print s
                if s:
                    # if s.stw > self.stw0 and s.stw <= self.stw1:
                    if s.stw > stw0 and s.stw <= stw1:
                        if s.frontend == 'SPL':
                            (s1, s2) = s.Split()
                            self.tuning(s1)
                            if self.checked(s1):
                                spectra.append(s1)
                            self.tuning(s2)
                            if self.checked(s2):
                                spectra.append(s2)
                        else:
                            self.tuning(s)
                            if self.checked(s):
                                spectra.append(s)
                        # break
                        # time.sleep(1)
            except IOError:
                odin.Warn("I/O error occured reading file %s" % (file))
            except EOFError:
                break
        odin.Info("processed %d spectra" % (len(spectra)))
        return spectra

    def doAOS(self, file):
        """Extract AOS spectra from level 0 files."""

        spectra = []
        odin.Info("next AOS file: %s" % (file))
        aos = odin.AOSfile(file)
        stw0 = self.stw0 % 0x100000000
        stw1 = self.stw1 % 0x100000000
        while True:
            try:
                s = aos.NextSpectrum()
                if s:
                    # if s.stw > self.stw0 and s.stw <= self.stw1:
                    if s.stw > stw0 and s.stw <= stw1:
                        self.tuning(s)
                        if self.checked(s):
                            spectra.append(s)
                        # break
                        # time.sleep(1)
            except EOFError:
                break
        odin.Info("processed %d spectra" % (len(spectra)))
        return spectra

    def getLoadTemp(self):
        """Get a table of calibration load temperatures."""
        if not hasattr(self, 'temp'):
            self.temp = {}
        if 'hotLoad' not in self.temp:
            self.temp['hotLoad'] = self.getTemperature("hot load A-side")
            if len(self.temp['hotLoad'][0]) == 0:
                self.temp['hotLoad'] = self.getTemperature("hot load B-side")
        return

    def getTargetIndex(self):
        """Get a table of target indeces from HK file."""

        files = self.findlevel0('SHK')
        STW = []
        tindex = []
        for file in files:
            hk = SHKfile(file)
            acdc = HKdata['ACDC1 sync']
            (stw, HK) = hk.getHKword(acdc[0], sub=acdc[1])
            for i in range(0, len(stw)):
                n = int(HK[i])
                STW.append(stw[i])
                tindex.append((n >> 3) & 0x001f)
        tbl = removeDuplicates(STW, tindex)
        return tbl

    def getCalMirror(self):
        """Get a table of calibration mirror positions."""

        files = self.findlevel0('FBA')
        AUXfile = FBAfile
        if not files:
            odin.Warn("no FBA files found for orbit %d" % (self.orbit))
            files = self.findlevel0('AOS')
            if not files:
                odin.Warn("no AOS files found for orbit %d" % (self.orbit))
                return None
            AUXfile = AOSfile
        stw = []
        typ = []
        for file in files:
            odin.Info("next AUX file: %s" % (file))
            aux = AUXfile(file)
            while True:
                head = aux.getSpectrumHead()
                if not head:
                    break
                beam = aux.Type(head)
                if beam == "SK1" or beam == "SK2" or beam == "CAL":
                    typ.append(beam)
                    stw.append(aux.stw)
        mirror = removeDuplicates(stw, typ)
        return mirror

    def process(self, mode=None):
        """Process level 0 files into level 1a."""

        files = self.findlevel0(self.backend)
        all = []
        target = None
        if not files:
            odin.Warn("no %s files found for orbit %d" %
                      (self.backend, self.orbit))
            return None
        shk = self.findlevel0('SHK')
        if not shk:
            odin.Warn("no SHK files found for orbit %d" % (self.orbit))
            # return None
        self.shk = shk
        if shk:
            self.getLoadTemp()
        mirror = self.getCalMirror()

        for filename in files:
            if self.backend == 'AC1' or self.backend == 'AC2':
                spectra = self.doAC(filename)
            elif self.backend == 'AOS':
                spectra = self.doAOS(filename)
            else:
                spectra = []
            for s in spectra:
                if mode and s.obsmode != mode:
                    odin.Warn("wrong observing mode: %s" % (s.obsmode))
                    continue
                if shk:
                    if s.obsmode == 'TPW':
                        if not target:
                            target = self.getTargetIndex()
                        index = Lookup(target, s.stw)
                        # print "index", index
                        if index == 2 and s.type == 'SIG':
                            s.type = 'REF'
                        elif index == 1 and s.type == 'REF':
                            s.type = 'SIG'
                    else:
                        if self.backend == 'AC1' or self.backend == 'AC2':
                            if s.type == 'REF' and mirror:
                                s.type = Lookup(mirror, s.stw)

                    # print "spectrum type %10d %s" % (s.stw, s.type)
                    all.append(s)
        if self.backend == 'AOS':
            comb = []
            for s in all:
                if s.freqcal[0] == 2100.0e6 and \
                   s.freqcal[1] == 6.2000e5 and \
                   s.freqcal[2] == 2.0000e0 and \
                   s.freqcal[3] == 0.0:
                    odin.Warn("uncorrected spectrum")
                if s.type == 'CMB':
                    print "comb spectrum", s.freqcal
                    comb.append(s.freqcal)
        msg = "total of %d spectra" % (len(all))
        odin.Info(msg)
        print msg

        self.spectra = {}
        for s in all:
            if shk:
                s.tcal = Lookup(self.temp['hotLoad'], s.stw) + 273.15
            else:
                s.tcal = 300.0
            rx = s.frontend
            if rx in ['119', '495', '549', '555', '572', '   ']:
                if rx not in self.spectra:
                    self.spectra[rx] = []
                self.spectra[rx].append(s)

    def calibrate(self):
        """Do intensity calibration for all receivers."""

        if hasattr(self, 'spectra'):
            for rx in self.spectra.keys():
                self.spectra[rx] = self.ICalibrate(self.spectra[rx], rx)
                if self.spectra[rx]:
                    msg = "%d calibrated spectra for receiver %s" % \
                          (len(self.spectra[rx]), rx)
                    print msg
                    odin.Info(msg)
                else:
                    odin.Warn("no calibrated spectra for receiver %s" % (rx))
                    del self.spectra[rx]

    def attitude(self):
        """Lookup attitude information for all spectra."""

        attfiles = self.findlevel0('ATT')
        if attfiles:
            # for file in attfiles:
            #    db = pg.connect('smr','ymer',user='smr')
            #    query = "SELECT version FROM soda"
            #    query = query + " WHERE file = '%s'" % (file)
            #    print query
            #    q = db.query(query)
            #    for row in q.getresult():
            #        soda = float(row[0])
            #        if self.soda != soda and self.soda != 0.0:
            #            odin.Warn("mixture of Soda versions: %f != %f" % \
            #                      (self.soda, soda))
            #        self.soda = soda
            #        odin.Info("file %s, soda version %f" % (file,self.soda))
            #    db.close()
            self.soda = 0.0
            print attfiles
            ap = AttitudeParser(attfiles, self.stw0, self.stw1)
            print ap
            # sorting attitudes
            tbl = ap.table
            print tbl
            keys = tbl.keys()
            if not keys:
                print -1
                odin.Warn("no attitude info found for orbit %d" % (self.orbit))
                return
            keys.sort()
            key0 = keys[0]
            for key1 in keys[1:]:
                stw0 = tbl[key0][6]
                stw1 = tbl[key1][6]
                if stw1 - stw0 < 17:
                    qe0 = tbl[key0][10]
                    qe1 = tbl[key1][10]
                    if qe0 != qe1:
                        err0 = qe0[0] * qe0[0] + qe0[1] * \
                            qe0[1] + qe0[2] * qe0[2]
                        err1 = qe1[0] * qe1[0] + qe1[1] * \
                            qe1[1] + qe1[2] * qe1[2]
                        # print "errors:", err0, err1
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
                attstw[row] = float(stw % 0x100000000L)
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
                for rx in self.spectra.keys():
                    for s in self.spectra[rx]:
                        stw = float(s.stw) - s.inttime * 16.0 / 2.0
                        i = searchsorted(attstw, stw)
                        if i > 0 and i < row:
                            dt = attstw[i] - attstw[i - 1]
                            dt0 = attstw[i] - stw
                            dt1 = stw - attstw[i - 1]
                            att = (dt0 * lookup[i - 1, :] +
                                   dt1 * lookup[i, :]) / dt
                            t = (att[0], long(s.stw), att[1],
                                 tuple(att[2:6]), tuple(att[6:10]),
                                 tuple(att[10:13]), tuple(att[13:19]), att[19])
                            s.Attitude(t)
                            s.soda = self.soda
                            if s.discipline == 'ASTR':
                                s.restfreq = s.skyfreq / \
                                    (1.0 - s.vlsr / 2.99792456e8)
                            else:
                                s.restfreq = s.skyfreq / \
                                    (1.0 - s.vgeo / 2.99792456e8)
        else:
            odin.Warn("no ATT files found for orbit %d" % (self.orbit))

    def save(self, dir=None):
        """Save all spectra in given directory."""

        home = os.getcwd()
        if dir:
            if os.path.isdir(dir):
                os.chdir(dir)
            else:
                odin.Warn("no such directory: " + dir)
                return
        else:
            dir = home

        n = 0
        if hasattr(self, 'spectra'):
            for rx in self.spectra.keys():
                for s in self.spectra[rx]:
                    file = s.Save()
                    # file = os.path.join(dir,file)
                    odin.Info("spectrum saved as '%s'" % (file))

                    n = n + 1
        odin.Info("%d spectra saved in directory '%s'" % (n, dir))
        if not dir == home:
            os.chdir(home)


def usage():
    print "usage: %s orbit (AC1|AC2|AOS) (ASTR|AERO)" % (sys.argv[0])


if __name__ == "__main__":
    if len(sys.argv) > 3:
        orbit = string.atoi(sys.argv[1])
        backend = sys.argv[2]
        discipline = sys.argv[3]
        if discipline not in ['ASTR', 'AERO']:
            usage()
        # pdb.set_trace()
        op = OrbitProcessor(orbit, backend, discipline)
        odin.LogAs("python")
        # op.process('SSW')
        op.process()
        if not hasattr(op, 'spectra') or len(op.spectra) == 0:
            sys.exit()

        op.attitude()
        # op.calibrate()

        if discipline == "ASTR":
            root = odinpath.astroroot()
            dir = odinpath.astropath(root, backend, orbit)
        else:
            dir = odinpath.aeropath(orbit, backend)

        print "string spectra in", dir
        os.chdir(dir)
        if op.spectra:
            for rx in op.spectra.keys():
                for s in op.spectra[rx]:
                    # s.FreqSort()
                    # if s.backend == "AC1":
                    #    s.FixBands()
                    s.Save()

            print "Done!"

    else:
        usage()
