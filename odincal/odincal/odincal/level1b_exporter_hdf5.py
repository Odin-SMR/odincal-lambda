import numpy
import copy
import os
from pg import DB
from sys import stderr, stdout, stdin, argv, exit
import h5py


class db(DB):
    def __init__(self):
        passwd = os.getenv('ODINDB_PASSWD')
        DB.__init__(
            self,
            dbname='odin',
            user='odinop',
            host='malachite.rss.chalmers.se',
            passwd=passwd)


class Spectrum():
    def __init__(self):
        self.stw = []
        self.backend = []
        self.orbit = []
        self.mjd = []
        self.level = []
        self.lst = []
        self.spectrum = []
        self.intmode = []
        self.channels = []
        self.skyfreq = []
        self.lofreq = []
        self.restfreq = []
        self.maxsuppression = []
        self.tsys = []
        self.sourcemode = []
        self.freqmode = []
        self.efftime = []
        self.sbpath = []
        self.latitude = []
        self.longitude = []
        self.altitude = []
        self.skybeamhit = []
        self.ra2000 = []
        self.dec2000 = []
        self.vsource = []
        self.qtarget = []
        self.qachieved = []
        self.qerror = []
        self.gpspos = []
        self.gpsvel = []
        self.sunpos = []
        self.moonpos = []
        self.sunzd = []
        self.vgeo = []
        self.vlsr = []
        self.ssb_fq = []
        self.inttime = []
        self.frontend = []
        self.hotloada = []
        self.lo = []
        self.sigtype = []
        self.version = []
        self.quality = []
        self.discipline = []
        self.topic = []
        self.spectrum_index = []
        self.obsmode = []
        self.type = []
        self.soda = []
        self.freqres = []
        self.pointer = []
        self.tspill = []


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
                  natural join getscansac2({0},{1}+16*60+45)
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
              inttime,ac_level1b.frontend,hotloada,hotloadb,lo,sig_type,
              ac_level1b.soda
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
               ac_cal_level1b.frontend,hotloada,hotloadb,lo,sig_type,
               ac_cal_level1b.soda
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
        # list of calstw that tells which scan a spectra belongs too
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

        for ind, res in enumerate(self.specdata):
            spec = Spectrum()
            for item in ['stw', 'mjd', 'orbit', 'lst', 'intmode',
                         'channels', 'skyfreq', 'lofreq', 'restfreq',
                         'maxsuppression', 'sbpath', 'latitude', 'longitude',
                         'altitude', 'skybeamhit', 'ra2000', 'dec2000',
                         'vsource', 'sunzd', 'vgeo', 'vlsr', 'inttime',
                         'hotloada', 'lo', 'freqmode', 'soda']:
                setattr(spec, item, res[item])
            if spec.hotloada == 0:
                spec.hotloada = res['hotloadb']
            for item in ['qtarget', 'qachieved', 'qerror', 'gpspos',
                         'gpsvel', 'sunpos', 'moonpos']:
                setattr(spec, item, res[item])

            ssb_fq1 = res['ssb_fq']
            spec.ssb_fq = tuple(numpy.array(ssb_fq1) * 1e6)

            # change backend and frontend to integer
            if res['backend'] == 'AC1':
                spec.backend = 1
            elif res['backend'] == 'AC2':
                spec.backend = 2

            if res['frontend'] == '555':
                spec.frontend = 1
            elif res['frontend'] == '495':
                spec.frontend = 2
            elif res['frontend'] == '572':
                spec.frontend = 3
            elif res['frontend'] == '549':
                spec.frontend = 4
            elif res['frontend'] == '119':
                spec.frontend = 5
            elif res['frontend'] == 'SPL':
                spec.frontend = 6

            data = numpy.ndarray(shape=(res['channels'],), dtype='float64',
                                 buffer=res['spectra'])
            spec.spectrum = data

            # deal with fields that only are stored for calibration
            # or target signals
            try:
                spec.tsys = res['tsys']
                spec.efftime = res['efftime']
            except BaseException:
                spec.tsys = 0.0
                spec.efftime = 0.0
            try:
                spec.tspill = res['tspill']
                tspill_index = ind
                if res['spectype'] == 'CAL':
                    spec.type = 3
                elif res['spectype'] == 'SSB':
                    spec.type = 9
            except BaseException:
                spec.type = 8
                spec.tspill = self.specdata[tspill_index]['tspill']

            spec.sourcemode = res['sourcemode'].replace(
                'STRAT', 'stratospheric').replace(
                'ODD_H', 'Odd hydrogen').replace(
                'ODD_N', 'Odd nitrogen').replace(
                'WATER', 'Water isotope').replace(
                'SUMMER', 'Summer mesosphere').replace(
                'DYNAM', 'Transport') +\
                ' FM=' + str(res['freqmode'])

            spec.level = res['alevel'] + res['version']

            spec.version = 262
            spec.quality = 0
            spec.discipline = 1
            spec.topic = 1
            spec.spectrum_index = ind
            spec.obsmode = 2

            spec.freqres = 1000000.0
            spec.pointer = [ind, res['channels'], 1, res['stw']]
            self.spectra.append(spec)


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

    def calibration_step2(self, spec):
        # compensate for ripple on sky beam signal
        t_load = planck(spec.hotloada, spec.skyfreq)
        t_sky = planck(2.7, spec.skyfreq)
        eta = 1 - spec.tspill / 300.0  # main beam efficeiency
        w = 1 / eta * (1 - (spec.spectrum) / (t_load))
        spec.spectrum = spec.spectrum - w * self.spec.spectrum
        return spec


def create_hdf_file(spectra, outfile):

    g = h5py.File(outfile, 'w')
    dtype1 = [
        ('Version', '>u2'),  # 1
        ('Level', '>u2'),  # 2
        ('Quality', '>u4'),  # 3
        ('STW', '>u8'),  # 4
        ('MJD', '>f8'),  # 5
        ('Orbit', '>f8'),  # 6
        ('LST', '>f4'),  # 7
        ('Source', '|S32'),  # 8
        ('Discipline', '>u2'),  # 9
        ('Topic', '>u2'),  # 10
        ('Spectrum', '>u2'),  # 11
        ('ObsMode', '>u2'),  # 12
        ('Type', '>u2'),  # 13
        ('Frontend', '>u2'),  # 14
        ('Backend', '>u2'),  # 15
        ('SkyBeamHit', '>u2'),  # 16
        ('RA2000', '>f4'),  # 17
        ('Dec2000', '>f4'),  # 18
        ('VSource', '>f4'),  # 19
        ('Longitude', '>f4'),  # 20
        ('Latitude', '>f4'),  # 21
        ('Altitude', '>f4'),  # 22
        ('Qtarget', '>f8', (4,)),  # 23
        ('Qachieved', '>f8', (4,)),  # 24
        ('Qerror', '>f8', (3,)),  # 25
        ('GPSpos', '>f8', (3,)),  # 26
        ('GPSvel', '>f8', (3,)),  # 27
        ('SunPos', '>f8', (3,)),  # 28
        ('MoonPos', '>f8', (3,)),  # 29
        ('SunZD', '>f4'),  # 30
        ('Vgeo', '>f4'),  # 31
        ('Vlsr', '>f4'),  # 32
        ('Tcal', '>f4'),  # 33
        ('Tsys', '>f4'),  # 34
        ('SBpath', '>f4'),  # 35
        ('LOFreq', '>f8'),  # 36
        ('SkyFreq', '>f8'),  # 37
        ('RestFreq', '>f8'),  # 38
        ('MaxSuppression', '>f8'),  # 39
        ('SodaVersion', '>f8'),  # 40
        ('FreqRes', '>f8'),  # 41
        ('FreqCal', '>f8', (4,)),  # 42
        ('IntMode', '>i4'),  # 43
        ('IntTime', '>f4'),  # 44
        ('EffTime', '>f4'),  # 45
        ('Channels', '>i4'),  # 46
        ('pointer', '>u8', (4,))  # 47
    ]
    dtype2 = [
        ('crosscheck', '>u8'),
        ('Array', '>f8', (896,))
    ]
    dtype3 = [
        ('crosscheck', '>u8'),
        ('Array', '>f8', (448,))
    ]

    # dtype2=[('Array','>f8', (len(datadict['spectra']),))]
    # dtype2=[('Array','>f8', (len(spectra[0].spectrum),))]
    dset1 = g.create_dataset("SMR", shape=(len(spectra), 1), dtype=dtype1)
    # dset2 = g.create_dataset("R896",shape=(len(spectra),1),dtype=dtype2)

    ind2 = 0
    ind3 = 0
    for ind, spec in enumerate(spectra):
        data1 = (
            spec.version,  # 1
            spec.level,  # 2
            spec.quality,  # 3
            spec.stw,  # 4
            spec.mjd,  # 5
            spec.orbit,  # 6
            spec.lst,  # 7
            spec.sourcemode,  # 8
            spec.discipline,  # 9
            spec.topic,  # 10
            spec.spectrum_index,  # 11
            spec.obsmode,  # 12
            spec.type,  # 13
            spec.frontend,  # 14
            spec.backend,  # 15
            spec.skybeamhit,  # 16
            spec.ra2000,  # 17
            spec.dec2000,  # 18
            spec.vsource,  # 19
            spec.longitude,  # 20
            spec.latitude,  # 21
            spec.altitude,  # 22
            spec.qtarget,  # 23
            spec.qachieved,  # 24
            spec.qerror,  # 25
            spec.gpspos,  # 26
            spec.gpsvel,  # 27
            spec.sunpos,  # 28
            spec.moonpos,  # 29
            spec.sunzd,  # 30
            spec.vgeo,  # 31
            spec.vlsr,  # 32
            spec.hotloada,  # 33
            spec.tsys,  # 34
            spec.sbpath,  # 35
            spec.lofreq,  # 36
            spec.skyfreq,  # 37
            spec.restfreq,  # 38
            spec.maxsuppression,  # 39
            spec.soda,  # 40
            spec.freqres,  # 41
            spec.ssb_fq,  # 42
            spec.intmode,  # 43
            spec.inttime,  # 44
            spec.efftime,  # 45
            spec.channels,  # 46
            spec.pointer  # 47
        )
        if spec.channels == 896:
            pass
        if spec.channels == 448:
            pass

        data2 = (spec.spectrum_index, tuple(spec.spectrum))
        dset1[ind] = data1
        if spec.channels == 896:
            if ind2 == 0:
                dset2 = g.create_dataset("R896",
                                         shape=(len(spectra), 1), dtype=dtype2)
            dset2[ind2] = data2
            ind2 = ind2 + 1
        if spec.channels == 448:
            if ind3 == 0:
                dset2 = g.create_dataset("R448",
                                         shape=(len(spectra), 1), dtype=dtype3)
            dset3[ind3] = data2
            ind3 = ind3 + 1
    # dset2 = g.create_dataset("SPECTRUM",data=list_of_spec)

    g.close()


def create_log_file(spectra, scaninfo, outfile, hdfoutfile):

    calstw = numpy.array(scaninfo)
    calstw_unique = numpy.unique(calstw)
    scan = 1
    scaninfo2file = []
    print len(calstw)
    for stw in calstw_unique:
        ind = numpy.nonzero(calstw == stw)[0]
        n0 = numpy.min(ind)
        n1 = numpy.max(ind)
        quality = 0
        line = "%3d\t%4d\t%4d\t%5lu\t" % (scan, n0 + 1, n1 + 1, quality)
        sunZD = (spectra[n0].sunzd + spectra[n1].sunzd) / 2.0
        Mjd = (spectra[n0].mjd + spectra[n1].mjd) / 2.0
        if spectra[n0].latitude == 0.0 and spectra[n0].longitude == 0.0:
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
            line = line + "\\N\t\\N\t"
        else:
            line = line + "%7.2f\t%7.2f\t" % (spectra[n0].latitude,
                                              spectra[n0].longitude)
            line = line + "%7.2f\t%7.2f\t" % (spectra[n1].latitude,
                                              spectra[n1].longitude)
            line = line + "%7.0f\t%7.0f\t" % (spectra[n0].altitude,
                                              spectra[n1].altitude)
        if sunZD == 0.0:
            line = line + "\\N\t"
        else:
            line = line + "%6.2f\t" % (sunZD)
        line = line + "%10.4f\t" % (Mjd)
        if spectra[n0].freqmode:
            line = line + "%3d\t" % (spectra[n0].freqmode)
        else:
            line = line + "\\N\t"
        line = line + "%s\t%4d" % (hdfoutfile, len(spectra))
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

    # example usage: python level1b_exporter_step2_hdf5.py 46885 AC1 1
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
            if spec.type == 8:
                altitude_range = '{80000,120000}'
                # load calibration (high-altitude) spectrum
                c.get_db_data(s['backend'], s['frontend'], s['version'],
                              s['intmode'], s['sourcemode'], s['freqmode'],
                              s['ssb_fq'], altitude_range, s['hotloada'])
                spec = c.calibration_step2(spec)

    # create files (hdf-file and log-file)
    hdfname = HDFname(backend, orbit)
    hdfoutfile = hdfname + '.HDF5'
    create_hdf_file(o.spectra, hdfoutfile)
    logoutfile = hdfname + '.LOG'
    create_log_file(o.spectra, o.scaninfo, logoutfile, hdfoutfile)

    con.close()
    print '''file {0} created'''.format(hdfoutfile)
