""" imports or calibrates files """

from datetime import date, datetime, timedelta
from os.path import join, basename
from time import sleep
from logging import getLogger
from pg import InternalError  # pylint: disable=import-error
from odincal.config import config
from odincal.database import ConfiguredDatabase
from odincal.level0 import import_file
from odincal.level1b_window_importer2 import level1b_importer


MAX_LEVEL0 = 200
MAX_LEVEL1 = 10


def get_full_path(filename):
    """ Calculate filenames"""
    dirname = '/odindata/odin/level0/'
    if filename.endswith('ac1'):
        filetype = 'ac1'
    elif filename.endswith('ac2'):
        filetype = 'ac2'
    elif filename.endswith('shk'):
        filetype = 'shk'
    elif filename.endswith('fba'):
        filetype = 'fba'
    elif filename.endswith('att'):
        if (int('0x' + filename.replace('.att', ''), base=16) <
                int('0x' + '0ce8666f.att'.replace('.att', ''), base=16)):
            filetype = 'att_17'
        else:
            filetype = 'att'
    else:
        pass
    partdir = filename[0:3]
    full_filename = join(dirname, filetype, partdir, filename)

    return full_filename


def clean_database(con):
    """ remove jobs from tables level0_files_in_process
     and in_process that has been there for a few days"""
    today = datetime.today()
    remove_date = today + timedelta(days=-4)
    query = con.query('''
           select file from level0_files_in_process
           where created<'{0}'
           '''.format(*[remove_date]))
    result = query.dictresult()
    for row in result:
        con.query('''
          delete from level0_files_in_process where file='{0}'
          '''.format(*[row['file']]))

    query = con.query('''
           select file from in_process
           where created<'{0}'
           '''.format(*[remove_date]))
    result = query.dictresult()
    for row in result:
        con.query('''
          delete from in_process where file='{0}'
          '''.format(*[row['file']]))


def main():  # noqa pylint: disable=too-many-locals, too-many-branches, too-many-statements
    """ The main func """
    max_retries = 3
    for wait_factor in range(max_retries):
        try:
            con = ConfiguredDatabase()
            break
        except InternalError as msg:
            print msg
            sleep((wait_factor + 1) * 5)
        if wait_factor == max_retries - 1:
            exit(1)

    clean_database(con)

    # prepare to launch new jobs

    # make a list of which periods we consider
    configuration = config.get('odincal', 'config')
    period_starts = eval(config.get(configuration, 'period_start'))  # noqa pylint: disable=eval-used
    period_ends = eval(config.get(configuration, 'period_end'))  # noqa pylint: disable=eval-used

    version = int(config.get(configuration, 'version'))

    periods = []
    for period_start, period_end in zip(period_starts, period_ends):
        if period_end == 'today':
            end = datetime.today()
            info = 'today'
        else:
            end = datetime.strptime(period_end, '%Y-%m-%d')
            info = 'old period'
        start = datetime.strptime(period_start, '%Y-%m-%d')
        start_1 = start - timedelta(days=1)
        end_1 = end + timedelta(days=1)
        period = {
            'start': start.date(),
            'end': end.date(),
            'start-1': start_1.date(),
            'end+1': end_1.date(),
            'info': info
        }
        periods.append(period)
    # loop over the periods
    # perform level0 import for the period first
    # if all level0 import is done for a period
    # we can start to calibrate
    for period in periods:
        print period
        # for level0_period we consider to import level0 data
        level0_period = [
            str(period['start-1']), str(period['end+1']),
            MAX_LEVEL0
        ]
        # for ac_period we consider to calibrate data
        ac_period = [
            str(period['start']),
            str(period['end']),
            MAX_LEVEL1,
            version
        ]
        # check if we have any level0_files at all for this period
        query = con.query('''
           select file,measurement_date from level0_files
           where measurement_date>='{0}' and measurement_date<='{1}' limit 1
           '''.format(*level0_period))
        result = query.dictresult()
        if len(result) == 0:
            # we do not have any level0_files for this period
            # continue with next period
            continue

        # check if we have any level0 files
        # where data is not imported for this period
        query = con.query('''
           select level0_files.file,measurement_date from level0_files
           left join level0_files_imported using(file)
           left join level0_files_in_process using(file)
           where measurement_date>='{0}' and measurement_date<='{1}'
           and level0_files_imported.created is Null and
           level0_files_in_process.created is Null
           order by measurement_date asc limit {2}
           '''.format(*level0_period))

        result = query.dictresult()

        if len(result) > 0:
            # do level0 import
            for row in result:
                import_level0_group(row, con)
            con.close()
            exit(0)
        else:
            # check that all level0 files are imported
            # we can possibly have level0 files in process
            query = con.query('''
           select file,measurement_date from level0_files
           left join level0_files_imported using(file)
           where measurement_date>='{0}' and measurement_date<='{1}'
           and level0_files_imported.created is Null
           order by file limit 1
           '''.format(*level0_period))
            result = query.dictresult()

            if len(result) > 0:
                # there are level0 files in process,
                # we should not start to calibrate data within
                # this period yet
                # continue to next period
                continue

            # check if this is the near real time period
            if period['info'] == 'today':
                # require some extra check
                # check which is the last date from where we have all
                # different files imported
                query = con.query('''
             select right(file,3) as ext,max(measurement_date)
             from level0_files_imported
             join level0_files using (file)
             where measurement_date>='{0}' and
             measurement_date<='{1}' group by ext'''.format(*level0_period))
                result = query.dictresult()
                time0 = date(2100, 1, 1)
                att_data = 0
                shk_data = 0
                fba_data = 0
                for row in result:
                    if row['ext'] == 'ac1' or row['ext'] == 'ac2':
                        continue
                    time1 = row['max']
                    if time1 < time0:
                        time0 = time1
                    if row['ext'] == 'att':
                        att_data = 1
                    if row['ext'] == 'shk':
                        shk_data = 1
                    if row['ext'] == 'fba':
                        fba_data = 1
                if att_data == 1 and shk_data == 1 and fba_data == 1:
                    # the latest date we have attitude,shk,and fba data
                    # is t1, now subtract 2 days for safety reason
                    time0 = time0 - timedelta(days=2)
                    ac_period[1] = str(time0)
                else:
                    # continue to next period
                    continue
            # check which ac_files that are not calibrated
            query = con.query('''
           select file from level0_files
           join level0_files_imported using(file)
           left join processed using(file)
           left join in_process using(file)
           where measurement_date>='{0}' and measurement_date<='{1}'
           and (right(file,3)='ac1' or right(file,3)='ac2')
           and (processed.total_scans is Null or
           processed.version!={3}) and (
           in_process.created is Null or
           in_process.version!={3})
           order by measurement_date desc limit {2}
           '''.format(*ac_period))
            result = query.dictresult()

            if len(result) > 0:
                # do level1b_calibration import
                calibrate_level0(result, con, version)
                con.close()
                exit(0)
    con.close()


def calibrate_level0(grp, open_con, version):
    """ run calibration """
    job_list = []
    for row in grp:
        temp = {
            'file': row['file'],
            'created': datetime.today(),
            'version': version,
        }
        open_con.insert('in_process', temp)
        job_list.append(temp)
    for job in job_list:
        filename = job['file']
        suffix = filename[-3:].upper()
        con = ConfiguredDatabase()
        level1b_importer(
            filename, suffix, version, con)
        # con.delete('in_process', filename)
        con.close()

def import_level0_group(row, open_con):
    """ run import """
    filelist = []
    logger = getLogger(__name__)
    if row is not None:
        fullname = get_full_path(row['file'])
        temp = {
            'file': row['file'],
            'created': datetime.today()
        }
        logger.info(row['file'], row['measurement_date'])
        open_con.insert('level0_files_in_process', temp)
        filelist.append(fullname)

    for filename in filelist:
        import_file(filename)
        open_con.delete('level0_files_in_process', file=basename(filename))
        temp = {'file': basename(filename), 'created': datetime.today()}
        open_con.insert('level0_files_imported', temp)


if __name__ == '__main__':
    main()
