import numpy
import copy
from odin import odin
from pg import DB
from sys import stderr, stdout, stdin, argv, exit
import os
import string


class db(DB):
    def __init__(self):
        passwd = os.getenv('ODINDB_PASSWD')
        DB.__init__(
            self,
            dbname='odin',
            user='odinop',
            host='malachite.rss.chalmers.se',
            passwd=passwd)


class Calibration_spectrum():
    def __init__(self):
        self.backend = []
        self.frontend = []
        self.version = []
        self.intmode = []
        self.sourcemode = []
        self.freqmode = []
        self.ssb_fq = []
        self.altitude_range = []
        self.hotload_range = []
        self.spectrum = []


class Orbit_data_exporter():
    def __init__(self, backend, orbit, con):
        self.backend = backend
        self.orbit = orbit
        self.con = con

    def get_db_data(self):
        '''export orbit data from database tables'''

        # find min and max stws from orbit
        query = self.con.query('''
                  select min(foo.stw),max(foo.stw) from
                  (select stw from attitude_level1 where
                  orbit>={0} and orbit<{0}+1 order by stw) as foo
                             '''.format(self.orbit))
        result = query.dictresult()
        print result
        if result[0]['max'] is None:
            print 'no attitude data from orbit ' + str(orbit)
            return 0

        # find out which scans that starts in the orbit
        if self.backend == 'AC1':
            stwoff = 1
        else:
            stwoff = 0
        temp = [result[0]['min'], result[0]['max'], self.backend, stwoff]
        if self.backend == 'AC1':
            query = self.con.query('''
                  select min(ac_level0.stw),max(ac_level0.stw)
                  from ac_level0
                  natural join getscansac1({0},{1}+16*60*45)
                  where start>={0} and start<={1} and backend='{2}'
                                 '''.format(*temp))

        if self.backend == 'AC2':
            query = self.con.query('''
                  select min(ac_level0.stw),max(ac_level0.stw)
                  from ac_level0
                  natural join getscansac2({0},{1}+16*60*45)
                  where start>={0} and start<={1} and backend='{2}'
                                 '''.format(*temp))

        result = query.dictresult()
        print result
        if result[0]['max'] is None:
            print 'no data from ' + backend + ' in orbit ' + str(orbit)
            return 0

        # extract all target spectrum data for the orbit
        temp = [result[0]['min'], result[0]['max'], self.backend]
        query = self.con.query('''
              select calstw,stw,backend,orbit,mjd,lst,intmode,spectra,
              alevel,version,channels,skyfreq,lofreq,restfreq,maxsuppression,
              tsys,sourcemode,freqmode,efftime,sbpath,latitude,longitude,
              altitude,skybeamhit,ra2000,dec2000,vsource,qtarget,qachieved,
              qerror,gpspos,gpsvel,sunpos,moonpos,sunzd,vgeo,vlsr,ssb_fq,
              inttime,ac_level1b.frontend,hotloada,lo,sig_type,ac_level1b.soda
              from ac_level1b
              join attitude_level1  using (backend,stw)
              join ac_level0  using (backend,stw)
              join shk_level1  using (backend,stw)
              where stw>={0} and stw<={1} and backend='{2}' and version=8
              and sig_type='SIG'
              order by stw asc,intmode asc'''.format(*temp))
        result = query.dictresult()
        print len(result)

        # extract all calibration spectrum data for the orbit
        query2 = self.con.query('''
               select stw,backend,orbit,mjd,lst,intmode,spectra,alevel,version,
               channels,spectype,skyfreq,lofreq,restfreq,maxsuppression,
               sourcemode,freqmode,sbpath,latitude,longitude,altitude,tspill,
               skybeamhit,ra2000,dec2000,vsource,qtarget,qachieved,qerror,
               gpspos,gpsvel,sunpos,moonpos,sunzd,vgeo,vlsr,ssb_fq,inttime,
               ac_cal_level1b.frontend,hotloada,lo,sig_type,ac_cal_level1b.soda
               from ac_cal_level1b
               join attitude_level1  using (backend,stw)
               join ac_level0  using (backend,stw)
               join shk_level1  using (backend,stw)
               where stw>={0} and stw<={1} and backend='{2}' and version=8
               order by stw asc,intmode asc,spectype asc'''.format(*temp))
        result2 = query2.dictresult()
        print len(result2)

        if result == [] or result2 == []:
            print 'could not extract all necessary data for processing ' + \
                backend + ' in orbit ' + orbit
            return 0

        # combine target and calibration data
        # list of both target and calibration spectrum data
        self.specdata = []
        # list of calstw that tells which scan a spectra belongs to
        self.scaninfo = []

        for ind, row2 in enumerate(result2):
            # fist add calibration spectrum
            self.specdata.append(row2)
            self.scaninfo.append(row2['stw'])
            if ind < len(result2) - 1:
                if result2[ind]['stw'] == result2[ind + 1]['stw']:
                    continue
            for row in result:
                if row['calstw'] == row2['stw']:
                    self.scaninfo.append(row['calstw'])
                    self.specdata.append(row)

        return 1

    def decode_data(self):

        self.spectra = []
        # write data to the odinscan structure for each spectra
        for ind, res in enumerate(self.specdata):
            a = odin.Spectrum()
            a.version = 262  # 1
            a.level = res['alevel'] + res['version']  # 2
            a.quality = 0  # 3
            a.stw = res['stw'] % 2**32  # 4
            a.mjd = res['mjd']  # 5
            a.orbit = res['orbit']  # 6
            a.lst = res['lst']  # 7
            a.source = res['sourcemode'].replace(
                'STRAT', 'stratospheric').replace(
                'ODD_H', 'Odd hydrogen').replace(
                'ODD_N', 'Odd nitrogen').replace(
                'WATER', 'Water isotope').replace(
                'SUMMER', 'Summer mesosphere').replace(
                'DYNAM', 'Transpoort') + ' FM=' + str(res['freqmode'])  # 8

            a.discipline = 'AERO'  # 9
            a.topic = res['sourcemode']  # 10
            a.spectrum = ind + 1  # 11
            a.obsmode = 'SSW'  # 12
            try:
                if res['spectype'] == 'CAL' or res['spectype'] == 'SSB':  # 13
                    a.type = res['spectype']
            except BaseException:
                a.type = 'SPE'
            a.frontend = res['frontend']  # 14
            a.backend = res['backend']  # 15
            a.skybeamhit = res['skybeamhit']  # 16
            a.ra2000 = res['ra2000']  # 17
            a.dec2000 = res['dec2000']  # 18
            a.vsource = res['vsource']  # 19
            a.longitude = res['longitude']  # 20
            a.latitude = res['latitude']  # 21
            a.altitude = res['altitude']  # 22
            a.qtarget = res['qtarget']  # 23
            a.qachieved = res['qachieved']  # 24
            a.qerror = res['qerror']  # 25
            a.gpspos = res['gpspos']  # 26
            a.gpsvel = res['gpsvel']  # 27
            a.sunpos = res['sunpos']  # 28
            a.moonpos = res['moonpos']  # 29
            a.sunzd = res['sunzd']  # 30
            a.vgeo = res['vgeo']  # 31
            a.vlsr = res['vlsr']  # 32
            a.tcal = res['hotloada']  # 33
            try:
                a.tsys = res['tsys']  # 34
            except BaseException:
                a.tsys = 0
            a.sbpath = res['sbpath']  # 35
            a.lofreq = res['lofreq']  # 36
            a.skyfreq = res['skyfreq']  # 37
            a.restfreq = res['restfreq']  # 38
            a.maxsup = res['maxsuppression']  # 39
            a.soda = res['soda']  # 40
            a.freqres = 1000000.0  # 41
            ssb_fq = res['ssb_fq']
            for iq in range(len(ssb_fq)):
                ssb_fq[iq] = ssb_fq[iq] * 1e6
            a.freqcal = ssb_fq  # 42
            a.intmode = res['intmode']  # 43
            a.inttime = res['inttime']  # 44
            try:
                a.efftime = res['efftime']  # 45
            except BaseException:
                a.efftime = 0
            a.channels = res['channels']  # 46

            a.data = numpy.ndarray(shape=(res['channels'],), dtype='float64',
                                   buffer=con.res['spectra'])

            self.spectra.append(a)


def planck(T, f):
    h = 6.626176e-34     # Planck constant (Js)
    k = 1.380662e-23     # Boltzmann constant (J/K)
    T0 = h * f / k
    if (T > 0.0):
        Tb = T0 / (numpy.exp(T0 / T) - 1.0)
    else:
        Tb = 0.0
    return Tb


def freq(lofreq, skyfreq, LO):
    n = 896
    f = numpy.zeros(shape=(n,))
    seq = [1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, 1, 1]
    m = 0
    for adc in range(8):
        if seq[2 * adc]:
            k = seq[2 * adc] * 112
            df = 1.0e6 / seq[2 * adc]
            if seq[2 * adc + 1] < 0:
                df = -df
            for j in range(k):
                f[m + j] = LO[adc / 2] + j * df
            m += k
    fdata = numpy.zeros(shape=(n,))
    if skyfreq >= lofreq:
        for i in range(n):
            v = f[i]
            v = lofreq + v
            v /= 1.0e9
            fdata[i] = v
    else:
        for i in range(n):
            v = f[i]
            v = lofreq - v
            v /= 1.0e9
            fdata[i] = v
    return fdata


class Calibration_step2():
    def __init__(self, con):
        self.con = con
        self.spectra = [Calibration_spectrum()]

    def get_db_data(self, backend, frontend, version, intmode,
                    sourcemode, freqmode, ssb_fq, altitude_range, hotload):

        hotload_lower = int(numpy.floor(hotload))
        hotload_upper = int(numpy.ceil(hotload))
        hotload_range = '''{{{0},{1}}}'''.format(
            *[hotload_lower, hotload_upper])
        temp = [backend, frontend, version, intmode, sourcemode, freqmode,
                ssb_fq, altitude_range, hotload_range]

        # find out if we already have required data
        for ind, spec in enumerate(self.spectra):
            if (spec.backend == backend and
                spec.frontend == frontend and
                spec.version == version and
                spec.intmode == intmode and
                spec.sourcemode == sourcemode and
                spec.freqmode == freqmode and
                spec.ssb_fq == ssb_fq and
                spec.altitude_range == altitude_range and
                    spec.hotload_range == hotload_range):
                self.spec = spec
                return

        # now we did not have the required data, so load it
        query = self.con.query('''
              select hotload_range,median_fit,channels
              from ac_cal_level1c where backend='{0}' and
              frontend='{1}' and version={2} and intmode={3}
              and sourcemode='{4}' and freqmode={5} and ssb_fq='{6}' and
              altitude_range='{7}' and hotload_range='{8}'
                             '''.format(*temp))
        result = query.dictresult()
        if result == []:
            medianfit = 0.0
        else:
            medianfit = numpy.ndarray(shape=(result[0]['channels'],),
                                      dtype='float64',
                                      buffer=result[0]['median_fit'])
        self.spec = Calibration_spectrum()
        self.spec.backend = backend
        self.spec.frontend = frontend
        self.spec.version = version
        self.spec.intmode = intmode
        self.spec.sourcemode = sourcemode
        self.spec.freqmode = freqmode
        self.spec.ssb_fq = ssb_fq
        self.spec.altitude_range = altitude_range
        self.spec.hotload_range = hotload_range
        self.spec.spectrum = medianfit
        self.spectra.append(self.spec)

    def calibration_step2(self, spec, tspill):
        # compensate for ripple on sky beam signal
        t_load = planck(spec.tcal, spec.skyfreq)
        t_sky = planck(2.7, spec.skyfreq)
        eta = 1 - tspill / 300.0  # main beam efficeiency
        w = 1 / eta * (1 - (spec.data) / (t_load))
        spec.data = spec.data - w * self.spec.spectrum
        return spec


def create_hdf_file(spectra, outfile):
    # store data to file
    #   -first store a binary file for each spectra, and produce
    #   a file with a list of all files
    #   -then produce a level1b hdf-file

    basepath = '/home/molflow/odincal'

    # store a binary file for each spectra
    filelist = os.path.join(basepath, 'odincal/odincal/filelist.txt')
    output = open(filelist, 'w+')
    for spec in spectra:
        spec.Save(str(spec.spectrum))
        output.write(str(spec.spectrum) + '\n')
        # print spec.stw
    output.close()

    # produce a level1b hdf-file
    ld_path = os.path.join(basepath, 'parts/hdf4/lib')
    LD = '''LD_LIBRARY_PATH='{0}' '''.format([ld_path])
    program = os.path.join(basepath, 'parts/oops/bin/whdf')
    cmd = string.join([LD, program, '-file', outfile, '-list '])
    os.system(string.join([cmd, filelist]))

    # delete files created in step 1 above
    cmd = string.join(['rm ', filelist])
    os.system(cmd)
    for spec in spectra:
        cmd = string.join(['rm ', str(spec.spectrum)])
        os.system(cmd)

    spec.Save(str(spec.spectrum))


def create_log_file(spectrum, specdata, scaninfo, outfile, hdfoutfile):

    # create a log file with information of each scan
    calstw = numpy.array(scaninfo)
    calstw_unique = numpy.unique(calstw)

    scan = 1
    scaninfo2file = []
    for stw in calstw_unique:
        ind = numpy.nonzero(calstw == stw)[0]
        n0 = numpy.min(ind)
        n1 = numpy.max(ind)
        quality = 0
        line = "%3d\t%4d\t%4d\t%5lu\t" % (scan, n0 + 1, n1 + 1, quality)
        sunZD = (spectrum[n0].sunzd + spectrum[n1].sunzd) / 2.0
        Mjd = (spectrum[n0].mjd + spectrum[n1].mjd) / 2.0
        if spectrum[n0].latitude == 0.0 and spectrum[n0].longitude == 0.0:
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
        else:
            line = line + "%7.2f\t%7.2f\t" % (spectrum[n0].latitude,
                                              spectrum[n0].longitude)
            line = line + "%7.2f\t%7.2f\t" % (spectrum[n1].latitude,
                                              spectrum[n1].longitude)
            line = line + "%7.0f\t%7.0f\t" % (spectrum[n0].altitude,
                                              spectrum[n1].altitude)
        if sunZD == 0.0:
            line = line + "\\N\t"
        else:
            line = line + "%6.2f\t" % (sunZD)
        line = line + "%10.4f\t" % (Mjd)
        if specdata[n0]['freqmode']:
            line = line + "%3d\t" % (specdata[n0]['freqmode'])
        else:
            line = line + "\\N\t"
        line = line + "%s\t%4d" % (hdfoutfile, len(spectrum))
        line = line + '\n'
        scaninfo2file.append(line)
        scan = scan + 1
    f = open(outfile, 'w')
    f.writelines(scaninfo2file)
    f.close()


def HDFname(backend, orbit):
    if backend == 'AOS':
        bcode = 'A'
    elif backend == 'AC1':
        bcode = 'B'
    elif backend == 'AC2':
        bcode = 'C'
    elif backend == 'FBA':
        bcode = 'D'
    ocode = "%04X" % (orbit)
    hdffile = "O" + bcode + "1B" + ocode
    return hdffile


def usage():
    print "usage: %s orbit (AC1|AC2) (0|1) " % (argv[0])
    print "1 performs calibration step 2"


if __name__ == '__main__':
    # example usage: example usage: bin/odinpy level1b_exporter_step2_hdf5.py
    # 46885 AC1 1

    if len(argv) < 4:
        usage()
        exit(0)

    orbit = int(argv[1])
    backend = argv[2]
    calibration = int(argv[3])

    con = db()

    # export calibration data
    o = Orbit_data_exporter(backend, orbit, con)
    ok = o.get_db_data()
    if ok == 0:
        print 'data for orbit {0} not found'.format(orbit)
        exit(0)
    o.decode_data()

    if calibration == 1:
        # perform calibration step2 for target spectrum
        c = Calibration_step2(con)
        for spec, s in zip(o.spectra, o.specdata):
            if spec.type == 'SPE':
                altitude_range = '{80000,120000}'
                # load calibration (high-altitude) spectrum
                c.get_db_data(s['backend'], s['frontend'], s['version'],
                              s['intmode'], s['sourcemode'], s['freqmode'],
                              s['ssb_fq'], altitude_range, s['hotloada'])
                spec = c.calibration_step2(spec, tspill)
            else:
                tspill = s['tspill']

    # create files (hdf-file and log-file)
    hdfname = HDFname(backend, orbit)
    hdfoutfile = hdfname + '.HDF'
    logoutfile = hdfname + '.LOG'
    create_hdf_file(o.spectra, hdfoutfile)
    create_log_file(o.spectra, o.specdata, o.scaninfo, logoutfile, hdfoutfile)

    con.close()
    print '''file {0} created'''.format(hdfoutfile)
