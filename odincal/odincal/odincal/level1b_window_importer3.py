import numpy
import copy
import datetime
import logging
import pg
from pg import DB, ProgrammingError
from sys import stderr, stdout, stdin, argv, exit
from odin import odin
from odincal.logclient import set_odin_logging
from odincal.calibration_preprocess import Prepare_data
from odincal.frequency_calibration import Spectra
from odincal.intensity_calibration import Level1b_cal, planck
from odincal.config import config


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname=config.get('database', 'dbname'),
                    user=config.get('database', 'user'),
                    host=config.get('database', 'host'),
                    passwd=config.get('database', 'passwd'),
                    )


def frequency_calibrate(listof_uncal_spec, con):
    '''use the object Spectra to perform a frequency calibration'''
    listofspec = []
    for row in listof_uncal_spec:
        spec = Spectra(con, row, 0)
        if spec.lofreq < 1:
            continue

        if spec.frontend == 'SPL':
            # split data into two spectra
            aa = odin.Spectrum()
            aa.backend = spec.backend
            aa.channels = len(spec.data)
            aa.data = spec.data
            aa.lofreq = spec.lofreq
            aa.intmode = spec.intmode
            aa.frontend = spec.frontend
            (s1, s2) = aa.Split()
            if s1.frontend == row['frontendsplit']:
                spec.data = s1.data
                spec.intmode = s1.intmode
                spec.frontend = s1.frontend
            if s2.frontend == row['frontendsplit']:
                spec.data = s2.data
                spec.intmode = s2.intmode
                spec.frontend = s2.frontend

        spec.tuning()
        if spec.lofreq < 1:
            continue
        spec.freqMode()
        listofspec.append(spec)

    return listofspec


def intensity_calibrate(listofspec, con, calstw, version, soda):
    '''Use the object Level1b_cal to perform intensity calibration
       of a given scan and import the result to database tables.
       The listofspec is assumed to contain data from a window
       +-45 min around the scan to be calibrated.
    '''

    if 1:
        fsky = []
        scanstw = []
        for spec in listofspec:
            fsky.append(spec.skyfreq)
            scanstw.append(spec.start)
        fsky = numpy.array(fsky)
        scanstw = numpy.array(scanstw)
        # create vector with all indices of fsky where frequency
        # changes by more than one MHz
        modes = numpy.nonzero(abs(fsky[1:] - fsky[:-1]) > 1.0e6)
        modes = numpy.array(modes[0])
        # prepend integer '0' and append integer 'len(fsky)'
        modes = numpy.concatenate((numpy.concatenate(([0], modes)),
                                   [len(fsky)]))
        for m in range(len(modes) - 1):
            start = int(modes[m] + 1)
            stop = int(modes[m + 1])
            # check that this mode contain data from the
            # scan to be calibrated
            if any(scanstw[start:stop] == calstw):
                pass
            else:
                continue

            # intensity calibrate
            ac = Level1b_cal(listofspec[start:stop], calstw, con)
            (calibrated, VERSION, Tspill) = ac.calibrate(version)
            if calibrated is None:
                continue

            # store data into database tables
            # ac_level1b (spectra) or ac_cal_level1b (tsys and ssb)
            print datetime.datetime.now()
            for s in calibrated:
                specs = []
                if s.split:
                    # split data into two spectra
                    aa = odin.Spectrum()
                    aa.backend = s.backend
                    aa.channels = len(s.data)
                    aa.data = s.data
                    aa.lofreq = s.lofreq
                    aa.intmode = s.intmode
                    (s1, s2) = aa.Split()
                    specs.append(s1)
                    specs.append(s2)
                else:
                    specs.append(s)

                # return
                # database insert
                for spec in specs:
                    # print s.gain
                    # print 'insert cal'
                    # spec.data=spec.data[0:112]
                    if s.type == 'SPE' and s.start == calstw:
                        temp = {
                            'stw': s.stw,
                            'backend': s.backend,
                            'frontend': s.frontend,
                            'version': int(VERSION),
                            'intmode': spec.intmode,
                            'soda': soda,
                            'spectra': spec.data.tostring(),
                            'channels': len(spec.data),
                            'skyfreq': s.skyfreq,
                            'lofreq': s.lofreq,
                            'restfreq': s.restfreq,
                            'maxsuppression': s.maxsup,
                            'tsys': s.tsys,
                            'sourcemode': s.topic,
                            'freqmode': s.freqmode,
                            'efftime': s.efftime,
                            'sbpath': s.sbpath,
                            'calstw': calstw
                        }

                        tempkeys = [
                            temp['stw'], temp['backend'],
                            temp['frontend'], temp['version'],
                            temp['intmode'], temp['soda'],
                            temp['sourcemode'], temp['freqmode']]
                        con.query('''delete from ac_level1b
                     where stw={0} and backend='{1}' and frontend='{2}'
                     and version={3} and intmode={4} and soda={5} and
                     sourcemode='{6}' and freqmode={7}'''.format(
                            *tempkeys))
                        con.insert('ac_level1b', temp)
                    # print 'insert done'
                for spec in specs:
                    if s.type == 'CAL' or s.type == 'SSB':
                        temp = {
                            'stw': s.stw,
                            'backend': s.backend,
                            'frontend': s.frontend,
                            'version': int(VERSION),
                            'spectype': s.type,
                            'intmode': spec.intmode,
                            'soda': soda,
                            'spectra': spec.data.tostring(),
                            'channels': len(spec.data),
                            'skyfreq': s.skyfreq,
                            'lofreq': s.lofreq,
                            'restfreq': s.restfreq,
                            'maxsuppression': s.maxsup,
                            'sourcemode': s.topic,
                            'freqmode': s.freqmode,
                            'sbpath': s.sbpath,
                            'tspill': Tspill,
                        }

                        tempkeys = [
                            temp['stw'], temp['backend'],
                            temp['frontend'], temp['version'],
                            temp['spectype'], temp['intmode'],
                            temp['soda'], temp['sourcemode'],
                            temp['freqmode']]
                        con.query('''delete from ac_cal_level1b
                     where stw={0} and backend='{1}' and frontend='{2}'
                     and version={3} and spectype='{4}' and intmode={5}
                     and soda={6} and sourcemode='{7}'
                     and freqmode={8}'''.format(*tempkeys))
                        con.insert('ac_cal_level1b', temp)


def report_result(con, acfile, info):
    '''report result to the database processing related tables'''
    temp = [acfile, info['version']]
    con.query('''delete from in_process
                 where file='{0}' and version={1} '''.format(*temp))
    if info['info'] == 'pg problem':
        return

    processtemp = {'file': acfile,
                   'info': info['info'],
                   'total_scans': info['total_scans'],
                   'success_scans': info['success_scans'],
                   'version': info['version']}
    con.query('''delete from processed
                     where file='{0}' and version={1} '''.format(*temp))
    con.insert('processed', processtemp)


def level1b_importer():
    '''perform an intensity and frequency calibration'''
    # acfile='100b8721.ac1'
    # backend='AC1'
    # level0_process=1
    # version=10
    # ../../bin/odinpy level1b_window_importer2.py 100b8721.ac1 AC1 1 10
    acfile = argv[1]
    backend = argv[2]
    level0_process = int(argv[3])
    version = int(argv[4])

    set_odin_logging()
    logger = logging.getLogger('level1b process')
    logger.info('processing file {0}'.format(acfile))

    con = db()

    p = Prepare_data(acfile, backend, version, con)

    # find out max and min stw from acfile to calibrate
    stw1, stw2 = p.get_stw_from_acfile()
    if stw1 == -1:
        info = {'info': 'no ac data',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        logger.warning(
            'no imported level0 ac data found for processing file {0}'.format(acfile))
        return

    # find out which sodaversion we have for this data
    sodaversion = p.get_soda_version(stw1, stw2)
    if sodaversion == -1:
        info = {'info': 'no attitude data',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        logger.warning(
            'no imported level0 attitude data found for processing file {0}'.format(acfile))
        return

    tdiff = 45 * 60 * 16
    # perform level0 data processing
    if level0_process == 1:
        error = p.att_level1_process(stw1 - tdiff, stw2 + tdiff, sodaversion)
        if error == 1:
            report_result(con, acfile, {'info': 'pg problem'})
            return
        error = p.shk_level1_process(stw1 - tdiff, stw2 + tdiff)
        if error == 1:
            report_result(con, acfile, {'info': 'pg problem'})
            return
        error = p.ac_level1a_process(stw1 - tdiff, stw2 + tdiff)
        if error == 1:
            report_result(con, acfile, {'info': 'pg problem'})
            return

    if level0_process == 2:
        error = p.att_level1_process(stw1 - tdiff, stw2 + tdiff, sodaversion)
        if error == 1:
            report_result(con, acfile, {'info': 'pg problem'})
            return
        error = p.shk_level1_process(stw1 - tdiff, stw2 + tdiff)
        if error == 1:
            report_result(con, acfile, {'info': 'pg problem'})
            return
        info = {'info': 'preprocess',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        return

    # find out which scan that starts in the file to calibrate
    scanstarts = p.get_scan_starts(stw1, stw2)
    if scanstarts == []:
        info = {'info': 'no scans found in file',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        return

    # get data to be used in calibration
    tdiff = 45 * 60 * 16
    firstscan = scanstarts[0]['start']
    lastscan = scanstarts[len(scanstarts) - 1]['start']
    result = p.get_data_for_calibration(
        firstscan, lastscan, tdiff, sodaversion)
    if result == []:
        info = {'info': 'necessary data not available',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        return

    # now loop over the scans
    success_scans = 0
    for row in scanstarts:
        # find out which frontend(s) we have in the scan
        scanfrontends = []
        for datarow in result:
            if datarow['start'] == row['start']:
                scanfrontends.append(datarow['frontendsplit'])
        scanfrontends = numpy.array(scanfrontends)
        scanfrontends = numpy.unique(scanfrontends)

        resultfresh = copy.deepcopy(result)
        for scanfrontend in scanfrontends:
            # filter data i.e remove undesired references
            scandata = p.filter_data(resultfresh, row['start'],
                                     row['ssb_att'], scanfrontend,
                                     tdiff)
            # now we are ready to start to calibrate
            # spectra for a scan
            if scandata == []:
                continue
            print datetime.datetime.now()
            listofspec = frequency_calibrate(scandata, con)
            if listofspec == []:
                continue
            intensity_calibrate(listofspec, con, row['start'],
                                version, sodaversion)
        success_scans = success_scans + 1

    info = {'info': '',
            'total_scans': len(scanstarts),
            'success_scans': success_scans,
            'version': version}
    report_result(con, acfile, info)
    con.close()


level1b_importer()
