#!/usr/bin/python

import os
import re
import sys
import string
import math
import time

from numpy import *

from oops import odin
from oops import odinpath
from oops.orbit import OrbitProcessor, bySTW


class AeroOrbit(OrbitProcessor):

    def __init__(self, orbit, backend, view='no'):
        if view == 'yes':
            from odinshow import SpectrumViewer
            self.root = Tk()
            self.sv = SpectrumViewer(self.root)
        OrbitProcessor.__init__(self, orbit, backend, "AERO")

    def checkaltitude(self, spectra):
        print "check altitudes"

        n = len(spectra)
        for i in range(n - 1, -1, -1):
            s = spectra[i]
            alt = s.altitude
            x = s.qerror[0]
            y = s.qerror[1]
            z = s.qerror[2]
            err = math.sqrt(x * x + y * y + z * z)
            if err > 20.0:
                del spectra[i]
            elif add.reduce(s.qachieved) == 0.0:
                del spectra[i]
            elif alt < 0.0 or alt > 120000.0:
                odin.Warn("deleting altitude: %10.3f" % (alt / 1000.0))
                del spectra[i]
        n2 = len(spectra)
        odin.Info("kept %d (%d) measurements" % (n2, n))

    def calibrate(self, spectra):
        """Do intensity calibration for all receivers."""

        print "start calibration"

        # from odinshow import SpectrumViewer
        # from Tkinter import *
        # root = Tk()
        # sv = SpectrumViewer(root)

        VERSION = 0x0007
        odin.Info("calibrating using version %04x" % (VERSION))
        (sig, ref, cal) = self.sortspec(spectra)
        self.checkaltitude(sig)
        if len(sig) == 0:
            return None
        scans = self.getScans(sig)
        odin.Info("number of spectra/scans: %d/%d" % (len(sig), len(scans)))
        if len(scans) == 0:
            return None
        ns = len(sig) / len(scans)
        odin.Info("average number of spectra per scan: %d" % (ns))
        if ns < 5:
            return None
        if len(ref) > 0:
            ref = self.cleanref(ref)
        if len(ref) == 0:
            odin.Warn("no reference spectra found")
            return None

        if len(cal) > 0:
            cal = self.cleancal(cal, ref)
        if len(cal) == 0:
            odin.Warn("no calibration spectra found")
            return None

        # (C,tsys,cstw) = self.getTsys(cal, ref)
        # ncal = len(cstw)
        # if ncal == 0:
        #     odin.Warn("no calibration spectra found after filter")
        #     return None
        if hasattr(self, 'root'):
            self.sv.g.yaxis_configure(min=0, max=5000)
            for c in cal:
                self.sv.showSpectrum(c)
                self.root.update()

        C = self.medTsys(cal)
        # print "mean Tsys spectrum"
        if hasattr(self, 'root'):
            self.sv.showSpectrum(C)
            self.root.update()
        # print "hit return to proceed",
        # sys.stdin.readline()

        nref = len(ref)
        (rstw, rmat) = self.matrix(ref)
        # (sstw,smat) = self.matrix(sig)

        calibrated = []
        maxalt = 0.0
        iscan = 0
        sig.sort(bySTW)

        for scan in scans:
            iscan = iscan + 1
            nspec = 0

            odin.Info("new scan from %08X to %08X" % (scan[0], scan[1]))
            # gain = C.Copy()
            for s in sig:
                if s.stw > scan[0]:
                    gain = s.Copy()
                    gain.type = 'CAL'
                    gain.channels = C.channels
                    gain.data = C.data
                    break

            # print "gain spectrum"
            # sv.g.yaxis_configure(min = 0, max = 10000)
            # sv.showSpectrum(gain)
            # root.update()
            # print "hit return to proceed",
            # sys.stdin.readline()

            calibrated.append(gain)
            ssb = gain.Copy()
            ssb.type = 'SSB'
            # n = len(ssb.data)
            ssb.data = self.ssbCurve(ssb)
            calibrated.append(ssb)

            if hasattr(self, 'root'):
                self.sv.g.yaxis_configure(min=-20, max=300)
            for t in sig:
                s = t.Copy()
                if s.stw < scan[0] or s.stw > scan[1]:
                    continue
                stw = float(s.stw)
                R = ref[0]

                R.data = self.interpolate(rstw, rmat, stw)

                s.Calibrate(R, gain, 1.0)
                T = take(gain.data, nonzero(gain.data))
                if T.shape[0] == 0:
                    return None

                s.tsys = add.reduce(T) / T.shape[0]

                d = take(s.data, nonzero(s.data))
                mean = add.reduce(d) / d.shape[0]
                msq = sqrt(add.reduce((d - mean)**2) / (d.shape[0] - 1))
                # print "mean, msq: %10.3f %10.3f" % (mean,msq)
                if mean < -1.0 or mean > 300.0:
                    odin.Warn("spectrum mean out of range: %10.1f" % (mean))
                    continue
#                if msq > 50.0:
#                    odin.Warn("spectrum rms  out of range: %10.1f" % (msq))
#                    continue

                # print "spectrum"
                if hasattr(self, 'root'):
                    self.sv.showSpectrum(s)
                    self.root.update()
                # print "hit return to proceed",
                # sys.stdin.readline()

                nspec = nspec + 1
                if s.altitude > maxalt:
                    maxalt = s.altitude
                calibrated.append(s)
            odin.Info("scan %2d with %2d spectra" % (iscan, nspec))
        odin.Info("total number of scans %2d" % (iscan))

        # for s in calibrated:
        #     d = take(s.data, nonzero(s.data))
        #     mean = add.reduce(d)/d.shape[0]
        #     msq = sqrt(add.reduce((d-mean)**2)/(d.shape[0]-1))
        #     print "mean, msq: %10.3f %10.3f" % (mean,msq)

        ########## determination of baffle contribution ##########
        ### Tspill = zeros(8, Float)
        Tspill = []
        eff = []
        odin.Info("maximum altitude in orbit %6.2f km" % (maxalt / 1000.0))
        for s in calibrated:
            if s.type == 'SPE' and s.altitude > maxalt - 10.0e3:
                n = len(s.data)
                med = sort(s.data)[n / 2]
                Tspill.append(med)

                if self.backend == "AOS":
                    d = s.data
                    mean = add.reduce(d) / d.shape[0]
                    msq = add.reduce((d - mean)**2) / (d.shape[0] - 1)
                    teff = (s.tsys * s.tsys / msq) / s.freqres
                    eff.append(teff / s.inttime)
                else:
                    n = 112
                    bands = 8
                    sigma = zeros(bands, Float)
                    for band in range(bands):
                        i0 = band * n
                        i1 = i0 + n
                        d = s.data[i0:i1]
                        mean = add.reduce(d) / float(n)
                        msq = add.reduce((d - mean)**2) / float(n - 1)
                        sigma[band] = msq
                    # print sigma
                    # msq = sort(sigma)[1]
                    msq = min(take(sigma, nonzero(sigma)))
                    teff = (s.tsys * s.tsys / msq) / s.freqres
                    eff.append(teff / s.inttime)
#                print "%10.3f %6.2f %6.2f %6.2f %6.2f %6.2f" % \
#                      (s.altitude, msq, s.tsys/sqrt(s.freqres*teff), \
#                       teff, s.inttime, teff/s.inttime)

        # Tspill is the median contribution from the baffle
        n = len(Tspill)
        if n:
            Tspill = sort(Tspill)[n / 2]
        else:
            odin.Warn("no spectra at high altitude")
            Tspill = 9.0
        print "mean contribution from baffle:   %7.3f K" % (Tspill)

        # eff is the mean integration efficiency, in the sense that
        # s.inttime*eff is an effective integration time related to
        # the noise level of a spectrum via the radiometer formula
        eff = array(eff)
        n = len(eff)
        if n:
            eff = add.reduce(eff) / n
        else:
            eff = 1.0
        print "mean efficiency:   %7.3f" % (eff)

        eta = 1.0 - Tspill / 300.0
        odin.Info("mean contribution from baffle:   %7.3f K" % (Tspill))
        odin.Info("resulting spill-over efficiency: %7.3f" % (eta))
        odin.Info("effective/real integration time: %7.3f" % (eff))
        for s in calibrated:
            # set version number of calibration routine
            s.level = s.level | VERSION
            if s.type == 'SPE':
                s.data = choose(equal(s.data, 0.0),
                                ((s.data - Tspill) / eta, 0.0))
                s.efftime = s.inttime * eff
        ########## determination of baffle contribution ##########

        return calibrated

    def getScans(self, sig):
        """ Find start and stop times of scans, i.e. up or down nods through
        the atmosphere. The routine is supposed to be called with an array
        of all the SIG spectra of one orbit."""

        sig.sort(bySTW)
        n = len(sig)
        stw = zeros(n, Float)
        h = zeros(n, Float)
        scans = []

        # Construct vectors of stw and altitude
        for i in range(n):
            s = sig[i]
            stw[i] = s.stw
            h[i] = s.altitude

        # check for reasonable range of altitudes
        h0 = min(h)
        h1 = max(h)
        odin.Info(
            "height range: %10.4f %10.4f %10.4f %10.4f" %
            (h0 / 1000.0, h1 / 1000.0, (h1 - h0) / 1000.0, (h1 + h0) / 2.0 / 1000.0))
        if h0 > 20.0e3:
            odin.Warn("minimum altitude achieved: %10.4f km" % (h0 / 1000.0))
        if h1 < 60.0e3:
            odin.Warn("maximum altitude achieved: %10.4f km" % (h1 / 1000.0))

        # find crossings of the mean altitude
        hm = (h0 + h1) / 2.0
        r = h - hm
        # print h0, h1, hm
        t = (stw - stw[0]) / 16.0
        # get the indeces of r, where r changes sign
        i = nonzero(where((r[:-1] * r[1:]) < 0, 1.0, 0.0))

        # if height interval is less than 10 km or only one crossing found,
        # assume one scan for the whole orbit
        if h1 - h0 < 10.0e3 or len(i) == 1:
            scans.append([long(stw[0]), long(stw[-1])])
            return scans

        for l in range(1, len(i), 2):
            i[l] = i[l] + 1

        # the top and bottom of scans occur at the arithmetic means
        # of the times just found
        y = (take(t, i[:-1]) + take(t, i[1:])) / 2.0
        n = float(len(y))
        x = arange(n) + 1.0

        # if more than 10 seconds between stw[0] and first scan:
        if y[0] > 10.0:
            scans.append([long(stw[0]), long(y[0] * 16.0 + stw[0])])

        for i in range(len(y) - 1):
            stw0 = long(y[i] * 16.0 + stw[0])
            stw1 = long(y[i + 1] * 16.0 + stw[0])
            scans.append([stw0, stw1])

        # if more than 10 seconds between last scan and stw[-1]:
        dt = (stw[-1] - stw[0]) / 16.0
        if dt - y[-1] > 10.0:
            scans.append([long(y[-1] * 16.0 + stw[0]), long(stw[-1])])

        return scans

    def ssbCurve(self, s):
        dB = {'495': -26.7, '549': -27.7, '555': -32.3, '572': -28.7}
        if s.frontend in ['495', '549', '555', '572']:
            maxdB = dB[s.frontend]
        else:
            return ones(len(s.channels), Float)

        fIF = 3900e6
        if s.skyfreq > s.lofreq:
            fim = s.lofreq - fIF
        else:
            fim = s.lofreq + fIF
        c = 2.9979e8
        d0 = c / fIF / 4.0
        l = c / fim
        n = floor(d0 / l)
        dx = (n + 0.5) * l - d0

        x0 = d0 + dx
        if s.skyfreq > s.lofreq:
            f = s.Frequency() - 2.0 * fIF / 1.0e9
        else:
            f = s.Frequency() + 2.0 * fIF / 1.0e9
            fim = s.lofreq + fIF
        l = c / f / 1.0e9
        p = 2.0 * pi * x0 / l
        Y = 0.5 * (1.0 + cos(p))
        S = 10.0**(maxdB / 10.0)
        Z = S + (1.0 - S) * Y
        Z = 1.0 - Z
        # for i in range(10):
        #     print "%15.6e %15.6e %10.5f" % (f[i], l[i], Z[i])
        return Z

    def PDCname(self):
        if self.backend == 'AOS':
            bcode = 'A'
        elif self.backend == 'AC1':
            bcode = 'B'
        elif self.backend == 'AC2':
            bcode = 'C'
        ocode = "%04X" % (self.orbit)
        self.pdcfile = "O" + bcode + "1B" + ocode
        return self.pdcfile

    def LogScan(self, i, n0, n1, fmode, total, s0, s1):
        if not hasattr(self, 'logfile'):
            self.PDCname()
            logfile = self.pdcfile + '.LOG'
            self.logfile = open(logfile, 'w')

        quality = 0
        line = "%3d\t%4d\t%4d\t%5lu\t" % (i, n0 + 1, n1 + 1, quality)
        sunZD = (s0.sunzd + s1.sunzd) / 2.0
        mjd = (s0.mjd + s1.mjd) / 2.0
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
        line = line + "%10.4f\t" % (mjd)
        if fmode:
            line = line + "%3d\t" % (fmode)
        else:
            line = line + "\\N\t"
        line = line + "%s\t%4d" % (self.pdcfile, total)
        self.logfile.write(line + '\n')
        line = "scan %2d in freq mode %2d (%4d,%4d)" % (i, fmode, n0, n1)
        odin.Info(line)
        return

    def FreqMode(self, s):
        df = 30.0e6
        self.freqmode = 0
        self.split = 0
        # LO = s.lofreq*(1.0 - 7.0/2.997924e5)
        LO = s.lofreq * (1.0 + s.vgeo / 2.99792456e8)
        modes = {
            'STRAT': "Stratospheric",
            'ODD_H': "Odd hydrogen",
            'ODD_N': "Odd nitrogen",
            'WATER': "Water isotope",
            'SUMMER': "Summer mesosphere",
            'DYNAM': "Transport"
        }

        config = None
        if s.backend == 'AC1':
            if s.frontend == '495':
                config = [[492.750e9, 23, "DYNAM", 0],
                          [495.402e9, 29, "DYNAM", 0],
                          [499.698e9, 25, "DYNAM", 0]]

                # [499.302e9, 3, "ODD_H", 0]
                # [490.302e9, 6, "ODD_N", 0]

            elif s.frontend == '549':
                config = [[548.502e9, 2, "STRAT", 0],
                          [553.050e9, 19, "WATER", 0],
                          [547.752e9, 21, "WATER", 1],
                          [553.302e9, 23, "DYNAM", 0],
                          [553.302e9, 29, "DYNAM", 0]]

                # [553.200e9, 3, "ODD_H", 0]
                # [547.752e9, 6, "ODD_N", 0]
                #     mode 6 has same LO as mode 21, except that
                #     the number of channels will be only half

            elif s.frontend == '555':
                config = [[553.298e9, 13, "SUMMER", 0]]

                # [555.552e9, 9, "WATER", 0],

            elif s.frontend == '572':
                config = [[572.762e9, 24, "DYNAM", 0]]

                # [573.318e9, 9, "WATER", 0]

        elif s.backend == 'AC2':
            if s.frontend == '495':
                config = [[497.880e9, 1, "STRAT", 1],
                          [492.750e9, 8, "WATER", 1],
                          [494.250e9, 17, "WATER", 0],
                          [499.698e9, 25, "DYNAM", 0]]

                # elif s.frontend == '555':
                # config = [[553.300e9,15, "ODD_H", 0]]
                # config = [[553.300e9,16, "ODD_H", 0]]
            elif s.frontend == '572':
                config = [[572.964e9, 22, "DYNAM", 1],
                          [572.762e9, 14, "SUMMER", 0]]

                # [572.964e9, 4, "ODD_H", 1],
                #     mode 22 has same LO as mode 4, except that
                #     the SSB-LOs use slightly different tuning
                # [573.984e9, 7, "ODD_N", 1],
                # [572.028e9,10, "STRAT", 1],
                # [572.100e9,15, "ODD_H", 0],
                # [571.964e9,16, "ODD_H", 0]]

        # elif s.backend == 'AOS':
        #     if s.frontend == '549':
        #         config = [[553.050e9,12, "WATER", 0]]
        #     elif s.frontend == '555':
        #         config = [[557.214e9, 5, "ODD_H", 0],
        #                   [555.540e9,11, "ODD_H", 0]]

        if config:
            df = [0.0] * len(config)
            for i in range(len(df)):
                df[i] = abs(LO - config[i][0])
            i = df.index(min(df))
            # print "configuration", i, config[i]
            self.freqmode = config[i][1]
            self.topic = config[i][2]
            self.split = config[i][3]
            # print "configuration %s:%s:%10.1f" % \
            #       (s.backend, s.frontend, LO/1.0e6),
            # print " %d, %s" % (self.freqmode, self.topic)
        else:
            odin.Warn("unknown configuration %s:%s:%10.1f" %
                      (s.backend, s.frontend, LO / 1.0e6))

        if self.freqmode:
            self.source = "%s FM=%d" % (modes[self.topic], self.freqmode)
        else:
            self.source = "unknown"
            self.topic = "N/A"

        return self.freqmode


def usage():
    print "usage: %s orbit (AC1|AC2|AOS) [-view]" % (sys.argv[0])


if __name__ == "__main__":
    if len(sys.argv) > 2:
        starttime = time.time()
        orbit = string.atof(sys.argv[1])
        backend = sys.argv[2]
        if len(sys.argv) > 3 and sys.argv[3] == "-view":
            view = "yes"
            from Tkinter import *
        else:
            view = "no"

        #raw = odinpath.aeropath(orbit, backend, single=0)
        # dir = odinpath.calibrated(raw)
        #dir = raw
        # os.chdir(dir)

        ao = AeroOrbit(orbit, backend, view)
        filename = ao.PDCname() + '.TXT'
        odin.LogAs("aero", filename)

        odin.Info("processing orbit %d" % (orbit))
        odin.Info("start " + ao.starttime())
        odin.Info("stop  " + ao.stoptime())

        if not ao.findlevel0('ATT'):
            odin.Warn("no ATT files found for orbit %d" % (orbit))
        else:
            ao.process()

        print 110
        b = ao.spectra.keys()
        print b
        # rx=ao.spectra['495']
        rx = ao.spectra['549']
        print len(rx)
        for i in range(400):
            spectra = []
            spectra.append(rx[i])
            spec = array(spectra[0].data)
            print spectra[0].lofreq, spectra[0].skyfreq, spectra[0].maxsup, spectra[0].restfreq
        #print ao.spectra.Spectrum
        print 111
        print bb
        if hasattr(ao, 'spectra'):
            ao.attitude()
            spectra = []
            for rx in ao.spectra.keys():
                for s in ao.spectra[rx]:
                    spectra.append(s)
            if len(spectra) > 0:
                rows = len(spectra)
                #fSKY = zeros(rows, Float)
                fSKY = zeros(shape=(rows,))
                for i in range(rows):
                    fSKY[i] = spectra[i].skyfreq
                # create vector with all indices of fSKY where frequency
                # changes by more than one MHz
                modes = nonzero(abs(fSKY[1:] - fSKY[:-1]) > 1.0e6)
                modes = ndarray(shape=(len(modes[0]),), buffer=modes[0])
                # prepend integer '0' and append integer 'rows'
                modes = concatenate((concatenate(([0], modes)), [rows]))
            # os.chdir(raw)
            # for rx in spectra.keys():
            #     for s in spectra[rx]:
            #         s.Save()
            # os.chdir(dir)

            total = 0
            spectrum = 0
            scans = []
            files = []
            specs = []
            filename = ao.PDCname() + '.mat'
            mat = open(filename, "w")
            for m in range(len(modes) - 1):
                start = int(modes[m])
                stop = int(modes[m + 1])
                print "calibrating spectra[%d:%d]" % (start, stop)
                cals = ao.calibrate(spectra[start:stop])
                print 1
                if not cals:
                    continue

                i = 0
                mdl = zeros(4, Float)
                for s in cals:
                    fm = ao.FreqMode(s)
                    if s.type == 'CAL':
                        scans.append((total, fm))
                    if s.type == 'SPE':
                        i = i + 1
                        t = s.orbit - float(ao.orbit)
                        mdl[0] = add.reduce(s.data[0:224]) / 224.0
                        mdl[1] = add.reduce(s.data[224:448]) / 224.0
                        mdl[2] = add.reduce(s.data[448:672]) / 224.0
                        mdl[3] = add.reduce(s.data[672:896]) / 224.0
                        str = "%8.4f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f\n" % \
                              (t, s.longitude, s.latitude, s.altitude / 1000.0, s.sunzd,
                               s.tsys, mdl[0], mdl[1], mdl[2], mdl[3])
                        mat.write(str)

                    spectrum = spectrum + 1
                    s.source = ao.source
                    s.topic = ao.topic
                    # apply Doppler correction
                    s.restfreq = s.skyfreq / (1.0 - s.vgeo / 2.99792456e8)
                    if ao.split:
                        (s1, s2) = s.Split()
                        total = total + 1
                        s1.spectrum = total
                        file = s1.Save()
                        files.append(file)
                        specs.append(s1)
                        total = total + 1
                        s2.spectrum = total
                        file = s2.Save()
                        files.append(file)
                        specs.append(s2)
                    else:
                        total = total + 1
                        s.spectrum = total
                        file = s.Save()
                        files.append(file)
                        specs.append(s)
                del cals
            mat.close()
            if total > 0:
                program = '/home/smr/C/Programs/whdf'
                filename = ao.PDCname() + '.HDF'
                cmd = string.join([program, '-file', filename, '-list -'])
                print cmd
                output = os.popen(cmd, 'w')
                for file in files:
                    output.write(file + '\n')
                output.close()

                # print scans

                if len(scans) == 1:
                    fm = scans[0][1]
                    ao.LogScan(1, 0, total - 1, fm, total, specs[0], specs[-1])
                else:
                    for i in range(1, len(scans)):
                        first = scans[i - 1][0]
                        last = scans[i][0] - 1
                        if scans[i - 1][1] == scans[i][1]:
                            fm = scans[i - 1][1]
                        else:
                            fm = 0
                        # print "log scan", first+1, last+1, total
                        ao.LogScan(i, first, last, fm, total,
                                   specs[first], specs[last])
                    first = scans[i][0]
                    last = total - 1
                    fm = scans[i][1]
                    if last - first > 10:
                        # print "log scan", first+2, last+1, total
                        ao.LogScan(
                            len(scans),
                            first,
                            last,
                            fm,
                            total,
                            specs[first],
                            specs[last])

                cmd = string.join(["/bin/gzip", "-f", filename])
                print cmd
                os.system(cmd)
                ao.logfile.close()

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
            else:
                odin.Warn("no calibrated spectra for orbit %d" % (orbit))
                print "no calibrated spectra for orbit %d" % (orbit)

        stoptime = time.time()
        elapsed = stoptime - starttime
        odin.Info("processing orbit %d for %s took %.3f seconds" %
                  (orbit, backend, elapsed))
        if hasattr(ao, 'root'):
            print "click the <QUIT> button to exit"
            ao.root.mainloop()
        del ao
        sys.exit()

    else:
        usage()
