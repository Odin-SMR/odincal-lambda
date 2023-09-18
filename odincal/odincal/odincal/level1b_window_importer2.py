from sys import argv
import datetime
import copy
import logging
import numpy
from oops import odin  # pylint: disable=import-error
from odincal.calibration_preprocess import PrepareData, filter_data, NoScansError
from odincal.database import ConfiguredDatabase
from odincal.frequency_calibration import Spectra
from odincal.intensity_calibration import calibrate


TDIFF = 45 * 60 * 16


def frequency_calibrate(listof_uncal_spec, con):
    '''use the object Spectra to perform a frequency calibration'''
    listofspec = []
    for row in listof_uncal_spec:
        spec = Spectra(con, row, 0)
        # if spec.lofreq < 1:
        #     continue

        if spec.frontend == 'SPL':
            # split data into two spectra
            spectrum = odin.Spectrum()
            spectrum.backend = spec.backend
            spectrum.channels = len(spec.data)
            spectrum.data = spec.data
            spectrum.lofreq = spec.lofreq
            spectrum.intmode = spec.intmode
            spectrum.frontend = spec.frontend
            (spec1, spec2) = spectrum.Split()
            if spec1.frontend == row['frontendsplit']:
                spec.data = spec1.data
                spec.intmode = spec1.intmode
                spec.frontend = spec1.frontend
            if spec2.frontend == row['frontendsplit']:
                spec.data = spec2.data
                spec.intmode = spec2.intmode
                spec.frontend = spec2.frontend

        spec.tuning()
        if spec.lofreq < 1:
            continue
        spec.get_freqmode()
        listofspec.append(spec)

    return listofspec


def intensity_calibrate(listofspec, con, calstw, version, soda):
    '''perform intensity calibration and import the result to
       database tables. The listofspec is assumed to contain data
       from a window +-45 min around the scan to be calibrated.
    '''

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
    modes = numpy.concatenate(
        (numpy.concatenate(([0], modes)), [len(fsky)]))

    for mode in range(len(modes) - 1):
        start = int(modes[mode] + 1)
        stop = int(modes[mode + 1])
        # check that this mode contain data from the
        # scan to be calibrated
        if any(scanstw[start:stop] == calstw):
            pass
        else:
            continue

        # intensity calibrate
        (calibrated, version, tspill) = calibrate(
            listofspec[start:stop], calstw, version)

        if calibrated is None:
            continue
        insert_spec_in_db(con, calibrated, calstw, soda, version, tspill)


def insert_spec_in_db(con, calibrated, calstw, soda, version, tspill):
    '''store data into database tables
        ac_level1b (spectra) or ac_cal_level1b (tsys and ssb)
    '''
    for spec in calibrated:
        specs = []
        if spec.split:
            # split data into two spectra
            spectrum = odin.Spectrum()
            spectrum.backend = spec.backend
            spectrum.channels = len(spec.data)
            spectrum.data = spec.data
            spectrum.lofreq = spec.lofreq
            spectrum.intmode = spec.intmode
            (spec1, spec2) = spectrum.Split()
            specs.append(spec1)
            specs.append(spec2)
        else:
            specs.append(spec)

        for spec_i in specs:
            if spec.type == 'SPE' and spec.start == calstw:
                temp = {
                    'stw': spec.stw,
                    'backend': spec.backend,
                    'frontend': spec.frontend,
                    'version': int(version),
                    'intmode': spec_i.intmode,
                    'soda': soda,
                    'spectra': spec_i.data.tostring(),
                    'channels': len(spec_i.data),
                    'skyfreq': spec.skyfreq,
                    'lofreq': spec.lofreq,
                    'restfreq': spec.restfreq,
                    'maxsuppression': spec.maxsup,
                    'tsys': spec.tsys,
                    'sourcemode': spec.topic,
                    'freqmode': spec.freqmode,
                    'efftime': spec.efftime,
                    'sbpath': spec.sbpath,
                    'calstw': calstw
                }

                tempkeys = [
                    temp['stw'],
                    temp['backend'],
                    temp['frontend'],
                    temp['version'],
                    temp['intmode'],
                    temp['soda'],
                    temp['sourcemode'],
                    temp['freqmode']
                ]
                con.query(
                    '''delete from ac_level1b
                       where stw={0} and backend='{1}' and frontend='{2}'
                       and version={3} and intmode={4} and soda={5} and
                       sourcemode='{6}' and freqmode={7}
                    '''.format(*tempkeys))

                con.insert('ac_level1b', temp)

            if spec.type == 'CAL' or spec.type == 'SSB':

                temp = {
                    'stw': spec.stw,
                    'backend': spec.backend,
                    'frontend': spec.frontend,
                    'version': int(version),
                    'spectype': spec.type,
                    'intmode': spec_i.intmode,
                    'soda': soda,
                    'spectra': spec_i.data.tostring(),
                    'channels': len(spec_i.data),
                    'skyfreq': spec.skyfreq,
                    'lofreq': spec.lofreq,
                    'restfreq': spec.restfreq,
                    'maxsuppression': spec.maxsup,
                    'sourcemode': spec.topic,
                    'freqmode': spec.freqmode,
                    'sbpath': spec.sbpath,
                    'tspill': tspill,
                }

                tempkeys = [
                    temp['stw'],
                    temp['backend'],
                    temp['frontend'],
                    temp['version'],
                    temp['spectype'],
                    temp['intmode'],
                    temp['soda'],
                    temp['sourcemode'],
                    temp['freqmode']
                ]

                con.query(
                    '''delete from ac_cal_level1b
                        where stw={0} and backend='{1}' and frontend='{2}'
                        and version={3} and spectype='{4}' and intmode={5}
                        and soda={6} and sourcemode='{7}'
                        and freqmode={8}
                     '''.format(*tempkeys))

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


def preprocess_data(acfile, backend, version, con, stw1, stw2, tdiff, logger, pg_string=None):  # noqa
    """prepara data for calibration, i.e import data
       to database tables"""

    prepare_data = PrepareData(acfile, backend, version, con, pg_string)

    # find out which sodaversion we have for this data
    sodaversion = prepare_data.get_soda_version(stw1, stw2)
    if sodaversion == -1:
        info = {'info': 'no attitude data',
                'total_scans': 0,
                'success_scans': 0,
                'version': version}
        report_result(con, acfile, info)
        logger.warning(
            'no imported level0 attitude data found for processing file {0}'.format(acfile))  # noqa
        return 1

    # perform level0 data processing
    error = prepare_data.att_level1_process(
        stw1 - tdiff, stw2 + tdiff, sodaversion)
    if error == 1:
        report_result(con, acfile, {'info': 'pg problem'})
        return 1

    error = prepare_data.shk_level1_process(
        stw1 - tdiff, stw2 + tdiff)
    if error == 1:
        report_result(con, acfile, {'info': 'pg problem'})
        return 1

    error = prepare_data.ac_level1a_process(
        stw1 - tdiff, stw2 + tdiff)
    if error == 1:
        report_result(con, acfile, {'info': 'pg problem'})
        return 1

    return 0


class Level1BImportError(Exception):
    pass


class Level1BPrepareDataError(Level1BImportError):
    pass


class Level1BPreprocessDataError(Level1BImportError):
    pass


def preprocess_level1b(acfile, backend, version, con, pg_string=None):
    '''perform preprocessing'''

    logging.basicConfig()
    logger = logging.getLogger('level1b pre-process')
    logger.info('pre-processing file {0}'.format(acfile))

    prepare_data = PrepareData(acfile, backend, version, con, pg_string)

    # find out max and min stw from acfile to calibrate
    try:
        stw1, stw2 = prepare_data.get_stw_from_acfile()
    except NoScansError as err:
        info = {
            'info': 'no ac data',
            'total_scans': 0,
            'success_scans': 0,
            'version': version,
        }
        report_result(con, acfile, info)
        msg = 'no imported level0 ac data found for processing file {0} ({1})'.format(acfile, err)  # noqa
        raise Level1BPrepareDataError(msg)

    error = preprocess_data(
        acfile, backend, version, con, stw1, stw2, TDIFF, logger, pg_string)
    if error:
        msg = 'could not preprocess data for processing file {0}'.format(acfile)  # noqa
        raise Level1BPreprocessDataError(msg)

    return stw1, stw2


def job_info_level1b(
    stw1,
    stw2,
    acfile,
    backend,
    version,
    con,
    pg_string=None,
):
    logging.basicConfig()
    logger = logging.getLogger('level1b get job batch info')
    logger.info('getting job batch info for file {0}'.format(acfile))

    prepare_data = PrepareData(acfile, backend, version, con, pg_string)

    # find out which scan that starts in the file to calibrate
    try:
        scanstarts = prepare_data.get_scan_starts(stw1, stw2)
    except NoScansError as err:
        info = {
            'info': 'no scans found in file',
            'total_scans': 0,
            'success_scans': 0,
            'version': version,
        }
        report_result(con, acfile, info)
        msg = 'no scans found for processing file {0} ({1})'.format(acfile, err)  # noqa
        raise Level1BPrepareDataError(msg)

    # get data to be used in calibration
    sodaversion = prepare_data.get_soda_version(stw1, stw2)

    return scanstarts, sodaversion


def import_level1b(
    scanstarts,
    sodaversion,
    acfile,
    backend,
    version,
    con,
    pg_string,
):
    '''intensity and frequency calibration'''

    logging.basicConfig()
    logger = logging.getLogger('level1b process')
    logger.info('processing file {0}'.format(acfile))

    firstscan = scanstarts[0]['start']
    lastscan = scanstarts[len(scanstarts) - 1]['start']
    prepare_data = PrepareData(acfile, backend, version, con, pg_string)
    try:
        result = prepare_data.get_data_for_calibration(
            firstscan, lastscan, TDIFF, sodaversion,
        )
    except NoScansError as err:
        info = {
            'info': 'necessary data not available',
            'total_scans': 0,
            'success_scans': 0,
            'version': version,
        }
        report_result(con, acfile, info)
        msg = 'necessary data not available for processing file {0} ({1})'.format(acfile, err)  # noqa
        raise Level1BPrepareDataError(msg)

    # now loop over the scans
    success_scans = 0
    scan_ids = []
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
            scandata = filter_data(
                resultfresh,
                row['start'],
                row['ssb_att'],
                scanfrontend,
                TDIFF,
            )
            # now we are ready to start to calibrate
            # spectra for a scan
            if scandata == []:
                continue
            listofspec = frequency_calibrate(scandata, con)
            if listofspec == []:
                continue
            intensity_calibrate(
                listofspec,
                con,
                row['start'],
                version,
                sodaversion,
            )
        scan_ids.append(row["start"])
        success_scans = success_scans + 1

    info = {
        'info': '',
        'total_scans': len(scanstarts),
        'success_scans': success_scans,
        'version': version,
    }
    report_result(con, acfile, info)
    con.close()
    return scan_ids


def level1b_importer(acfile, backend, version, con, pg_string=None):
    '''perform preprocessing, and intensity and frequency calibration'''

    stw1, stw2 = preprocess_level1b(
        acfile,
        backend,
        version,
        con,
        pg_string,
    )
    scanstarts, sodaversion = job_info_level1b(
        stw1,
        stw2,
        acfile,
        backend,
        version,
        con,
        pg_string,
    )
    return import_level1b(
        scanstarts,
        sodaversion,
        acfile,
        backend,
        version,
        con,
        pg_string,
    )


def main():
    # acfile='100b8721.ac1'
    # backend='AC1'
    # level0_process=1
    # version=10
    # ../../bin/odinpy level1b_window_importer2.py 100b8721.ac1 AC1 1 10
    acfile = argv[1]
    backend = argv[2]
    # level0_process = int(argv[3])
    version = int(argv[4])
    con = ConfiguredDatabase()
    level1b_importer(acfile, backend, version, con)
    con.close()


if __name__ == "__main__":
    main()
