from oops import odin
import numpy
import psycopg2
from StringIO import StringIO
from psycopg2 import InternalError, IntegrityError
from odincal.config import config
from odincal.database import ConfiguredDatabase
from datetime import datetime
from pg import DB
import logging

logger = logging.getLogger('att_level1_importer')


def djl(year, mon, day, hour, min, secs):
    dn = 367 * year - 7 * (year + (mon + 9) / 12) / 4 \
        - 3 * ((year + (mon - 9) / 7) / 100 + 1) / 4 + 275 * mon / 9 \
        + day + 1721029
    jd = float(dn) - 0.5 + (hour + (min + secs / 60.0) / 60.0) / 24.0
    return jd


def att_level1_importer(stwa, stwb, soda, backend, pg_string=None):
    # soda=argv[1]
    temp = [stwa, stwb, soda, backend]
    if pg_string is None:
        con = ConfiguredDatabase()
    else:
        con = DB(pg_string)
    query = con.query('''select ac_level0.stw,ac_level0.backend,
                       inttime from ac_level0
                       left join attitude_level1 using (stw)
                       where (soda is NULL or soda!={2})
                       and ac_level0.stw>={0} and ac_level0.stw<={1}
                       and ac_level0.backend='{3}' '''.format(*temp))

    sigresult = query.dictresult()
    logger.info(
        "Got %i spectrum matching soda: %i and stw: [%i,%i]",
        len(sigresult), soda, stwa, stwb
    )
    fgr = StringIO()
    for sig in sigresult:
        keys = [sig['stw'], soda]
        query = con.query('''select year,mon,day,hour,min,secs,stw,
                           orbit,qt,qa,qe,gps,acs
                           from attitude_level0 where
                           stw>{0}-200 and stw<{0}+200
                           and soda={1}
                           order by stw'''.format(*keys))
        result = query.dictresult()
        logger.debug("Got %i att data rows matching %i", len(result), sig['stw'])
        if len(result) > 0:
            # interpolate attitude data to desired stw
            # before doing the actual processing
            stw = float(sig['stw']) - sig['inttime'] * 16.0 / 2.0
            # first create a lookup matrix
            cols = 2 + 2 * 4 + 3 + 6 + 1
            rows = len(result)
            lookup = numpy.zeros(shape=(rows, cols))
            attstw = numpy.zeros(shape=(rows,))
            for index, row in enumerate(result):
                attstw[index] = float(row['stw'])
                # extract database data
                JD = djl(row['year'], row['mon'], row['day'],
                         row['hour'], row['min'], row['secs'])
                # fill up the lookup table with data
                lookup[index, 0: 2] = numpy.array((JD, row['orbit']))
                lookup[index, 2: 6] = numpy.array(row['qt'])
                lookup[index, 6:10] = numpy.array(row['qa'])
                lookup[index, 10:13] = numpy.array(row['qe'])
                lookup[index, 13:19] = numpy.array(row['gps'])
                lookup[index, 19] = row['acs']
            # search for the index with the lowest stw
            # above the desired stw in the lookup table
            ind = (attstw > stw).nonzero()[0]
            if len(ind) == 0:
                continue
                # not enough match
            if ind[0] == 0:
                continue
                # not enough match
            i = ind[0]
            # now interpolate
            dt = attstw[i] - attstw[i - 1]
            dt0 = attstw[i] - stw
            dt1 = stw - attstw[i - 1]
            att = (dt0 * lookup[i - 1, :] + dt1 * lookup[i, :]) / dt
            t = (att[0], long(stw), att[1],
                 tuple(att[2:6]), tuple(att[6:10]),
                 tuple(att[10:13]), tuple(att[13:19]), att[19])
            # now process data using Ohlbergs code (s.Attitude(t))
            logging.debug('Using s.Attitude(%s)', t)
            s = odin.Spectrum()
            s.stw = long(stw)
            s.Attitude(t)
            fgr.write(str(sig['stw']) + '\t' +
                      str(sig['backend']) + '\t' +
                      str(soda) + '\t' +
                      str(s.mjd) + '\t' +
                      str(s.lst) + '\t' +
                      str(s.orbit) + '\t' +
                      str(s.latitude) + '\t' +
                      str(s.longitude) + '\t' +
                      str(s.altitude) + '\t' +
                      str(s.skybeamhit) + '\t' +
                      str(s.ra2000) + '\t' +
                      str(s.dec2000) + '\t' +
                      str(s.vsource) + '\t' +
                      str("{{{},{},{},{}}}".format(*s.qtarget)) + '\t' +
                      str("{{{},{},{},{}}}".format(*s.qachieved)) + '\t' +
                      str("{{{},{},{}}}".format(*s.qerror)) + '\t' +
                      str("{{{},{},{}}}".format(*s.gpspos)) + '\t' +
                      str("{{{},{},{}}}".format(*s.gpsvel)) + '\t' +
                      str("{{{},{},{}}}".format(*s.sunpos)) + '\t' +
                      str("{{{},{},{}}}".format(*s.moonpos)) + '\t' +
                      str(s.sunzd) + '\t' +
                      str(s.vgeo) + '\t' +
                      str(s.vlsr) + '\t' +
                      str(s.level) + '\t' +
                      str(datetime.now()) + '\n')
    if pg_string is None:
        conn = psycopg2.connect(config.get('database', 'pgstring'))
    else:
        conn = psycopg2.connect(pg_string)
    cur = conn.cursor()
    fgr.seek(0)
    logger.debug('Insert resulting data in temp table')
    cur.execute("create temporary table foo ( like attitude_level1 );")
    cur.copy_from(file=fgr, table='foo')
    try:
        cur.execute(
            "delete from attitude_level1 ac using foo f where f.stw=ac.stw and ac.backend=f.backend")  # noqa
        cur.execute("insert into attitude_level1 (select * from foo)")
    except (IntegrityError, InternalError) as e:
        logger.info("Got error inserting data into temp table: %s", e)
        try:
            conn.rollback()
            cur.execute("create temporary table foo ( like attitude_level1 );")
            cur.copy_from(file=fgr, table='foo')
            cur.execute(
                "delete from attitude_level1 ac using foo f where f.stw=ac.stw and ac.backend=f.backend")  # noqa
            cur.execute("insert into attitude_level1 (select * from foo)")

        except (IntegrityError, InternalError) as e1:
            logger.warning("Got another error inserting data: %s", e1)
            conn.rollback()
            conn.close()
            con.close()
            return 1

    fgr.close()
    conn.commit()
    conn.close()
    con.close()
    return 0
