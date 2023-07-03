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


def level1b_exporter():
    '''export data orbit data from database tables
       and store data into hdf5 format'''
    # example usage: bin/level1b_exporter 8575 AC1 odintestfile.hdf5

    orbit = argv[1]
    backend = argv[2]
    # outfile=argv[3]
    orbit1 = int(orbit) + 14

    outfile = HDFname(backend, int(orbit))
    hdffile = outfile + '.HDF'
    scaninfofile = outfile + '.LOG'

    con = db()
    # find min and max stws from orbit
    query = con.query('''select min(stw),max(stw)
                       from attitude_level1 where
                       floor(orbit)={0}'''.format(orbit1))
    result = query.dictresult()
    if result[0]['max'] is None:
        print 'no attitude data from orbit ' + str(orbit)
        exit(0)

    # extract all target spectrum data for the orbit
    temp = [result[0]['min'], result[0]['max'], backend]
    query = con.query(
        '''select calstw,stw,backend,orbit,mjd,lst,intmode,spectra,
                       alevel,version,
                       channels,
                       skyfreq,lofreq,restfreq,maxsuppression,
                       tsys,sourcemode,freqmode,efftime,sbpath,
                       latitude,longitude,altitude,skybeamhit,
                       ra2000,dec2000,vsource,qtarget,qachieved,qerror,
                       gpspos,gpsvel,sunpos,moonpos,sunzd,vgeo,vlsr,
                       ssb_fq,inttime,ac_level1b.frontend,
                       hotloada,lo,sig_type,ac_level1b.soda
                       from ac_level1b
                       join attitude_level1  using (backend,stw)
                       join ac_level0  using (backend,stw)
                       join shk_level1  using (backend,stw)
                       where calstw>={0} and calstw<={1} and backend='{2}'
                       and sig_type='SIG'
                       order by stw,intmode'''.format(*temp))
    result = query.dictresult()
    # extract all calibration spectrum data for the orbit
    query2 = con.query('''select stw,backend,orbit,mjd,lst,intmode,spectra,
                       alevel,version,
                       channels,spectype,
                       skyfreq,lofreq,restfreq,maxsuppression,
                       sourcemode,freqmode,sbpath,
                       latitude,longitude,altitude,skybeamhit,
                       ra2000,dec2000,vsource,qtarget,qachieved,qerror,
                       gpspos,gpsvel,sunpos,moonpos,sunzd,vgeo,vlsr,
                       ssb_fq,inttime,ac_cal_level1b.frontend,
                       hotloada,lo,sig_type,ac_cal_level1b.soda
                       from ac_cal_level1b
                       join attitude_level1  using (backend,stw)
                       join ac_level0  using (backend,stw)
                       join shk_level1  using (backend,stw)
                       where stw>={0} and stw<={1} and backend='{2}'
                       order by stw,intmode,spectype'''.format(*temp))
    result2 = query2.dictresult()

    if result == []:
        print 'could not extract all necessary data for processing ' + \
            backend + ' in orbit ' + orbit
        exit(0)

    # combine target and calibration data
    result3 = []
    scaninfo = []
    for ind, row2 in enumerate(result2):
        result3.append(row2)
        scaninfo.append(row2['stw'])
        if ind < len(result2) - 1:
            if result2[ind]['stw'] == result2[ind + 1]['stw']:
                continue
        for row in result:
            if row['calstw'] == row2['stw']:
                scaninfo.append(row['calstw'])
                result3.append(row)

    spectrum = []

    # write data to the odinscan structure for each spectra
    for ind, res in enumerate(result3):
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
            'DYNAM', 'Transport') +\
            ' FM=' + str(res['freqmode'])  # 8
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
                               buffer=res['spectra'])
        # a.data=numpy.ndarray(shape=(res['channels'],),dtype='float32')
        spectrum.append(a)

    # store data to file
    #   -first store a binary file for each spectra, and produce
    #   a file with a list of all files
    #   -then produce a level1b hdf-file

    basepath = '/home/odinop/odincal_tmp/odincal'

    # store a binary file for each spectra
    filelist = os.path.join(basepath, 'odincal/odincal/filelist.txt')
    output = open(filelist, 'w+')
    for spec in spectrum:
        print dir(spec)
        spec1 = spec.Copy()
        spec1.Save(str(spec1.spectrum))
        # spec.Save()
        output.write(str(spec.spectrum) + '\n')
        # print spec.stw
    output.close()

    # produce a level1b hdf-file
    ld_path = os.path.join(basepath, 'parts/hdf4/lib')
    LD = '''LD_LIBRARY_PATH='{0}' '''.format([ld_path])
    program = os.path.join(basepath, 'parts/oops/bin/whdf')
    cmd = string.join([LD, program, '-file', hdffile, '-list '])
    os.system(string.join([cmd, filelist]))

    # delete files created in step 1 above
    cmd = string.join(['rm ', filelist])
    os.system(cmd)
    for spec in spectrum:
        cmd = string.join(['rm ', str(spec.spectrum)])
        os.system(cmd)

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
        if result3[n0]['freqmode']:
            line = line + "%3d\t" % (result3[n0]['freqmode'])
        else:
            line = line + "\\N\t"
        line = line + "%s\t%4d" % (outfile, len(spectrum))
        line = line + '\n'
        scaninfo2file.append(line)
        scan = scan + 1
    f = open(scaninfofile, 'w')
    f.writelines(scaninfo2file)
    f.close()
    con.close()


# ../../bin/level1b_exporter 65971 AC1 test.hdf
