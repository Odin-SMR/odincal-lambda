from oops.level0 import ACfile, FBAfile, SHKfile, HKdata
from oops import attitude

import numpy
from pg import ProgrammingError
from sys import argv
from os.path import splitext, basename, split
from datetime import datetime
import psycopg2
from StringIO import StringIO
from odincal.config import config
import logging


def getSHK(hk):
    """use Ohlbergs code to read in shk data from file
        and creates a dictionary for easy insertation
        into a postgresdatabase.
        """
    # "first find out dicipline i.e if data is for aero-observations"
    # discipline=[]
    # stw=[]
    # words=hk.getBlock()
    # while words:
    #     discipline_def=hk.getIndex()
    #     stw.extend([discipline_def[0]])
    #    discipline.extend([discipline_def[2]])
    #     words=hk.getBlock()
    # hk.rewind()
    (STWa, LO495, LO549, STWb, LO555, LO572) = hk.getLOfreqs()
    (STW, SSB495, SSB549, SSB555, SSB572) = hk.getSSBtunings()
    shktypes = {
        "LO495": (STWa, LO495),
        "LO549": (STWa, LO549),
        "LO555": (STWb, LO555),
        "LO572": (STWb, LO572),
        "SSB495": (STW, SSB495),
        "SSB549": (STW, SSB549),
        "SSB555": (STW, SSB555),
        "SSB572": (STW, SSB572),
        "mixC495": [],
        "mixC549": [],
        "mixC555": [],
        "mixC572": [],
        "imageloadA": [],
        "imageloadB": [],
        "hotloadA": [],
        "hotloadB": [],
        "mixerA": [],
        "mixerB": [],
        "lnaA": [],
        "lnaB": [],
        "119mixerA": [],
        "119mixerB": [],
        "warmifA": [],
        "warmifB": [],
    }
    shktypeslist = {
        "mixer current 495": "mixC495",
        "mixer current 549": "mixC549",
        "mixer current 555": "mixC555",
        "mixer current 572": "mixC572",
        "image load B-side": "imageloadB",
        "image load A-side": "imageloadA",
        "hot load A-side": "hotloadA",
        "hot load B-side": "hotloadB",
        "mixer A-side": "mixerA",
        "mixer B-side": "mixerB",
        "LNA A-side": "lnaA",
        "LNA B-side": "lnaB",
        "119GHz mixer A-side": "119mixerA",
        "119GHz mixer B-side": "119mixerB",
        "warm IF A-side": "warmifA",
        "warm IF B-side": "warmifB",
    }
    for shktype in shktypeslist:
        table = HKdata[shktype]
        data = hk.getHKword(table[0], sub=table[1])
        data1 = map(table[2], data[1])
        if shktype == "hot load A-side" or shktype == "hot load B-side":
            data1 = numpy.array(data1) + 273.15

        shktypes[shktypeslist[shktype]] = (data[0], data1)
    return shktypes


def db_prep(datadict, db):
    dbdict = dict(datadict)
    dbdict['acd_mon'] = datadict['acd_mon']
    dbdict['cc'] = datadict['cc']
    return dbdict


def getACdis(ac):
    """AC factory.
    reads a fileobject and extract the discipline of AC-data. Uses Ohlbergs
    routines to read the files (ACfile)
    """
    head = ac.getSpectrumHead()
    while head is not None:
        discipline = ac.getIndex()
        if discipline is not None:
            discipline = discipline[2]
            if discipline is None:
                discipline = 'Problem'
        else:
            discipline = 'Problem'
        return discipline
    raise EOFError


def getAC(ac):
    """AC factory.
    reads a fileobject and creates a dictionary for easy insertation
    into a postgresdatabase. Uses Ohlbergs routines to read the files (ACfile)
    """
    backend = {
        0x7380: 'AC1',
        0x73b0: 'AC2',
    }
    CLOCKFREQ = 224.0e6
    head = ac.getSpectrumHead()
    while head is not None:
        data = []
        stw = ac.stw
        back = backend[ac.user]
        for j in range(12):
            data.append(ac.getBlock())
            if data[j] == []:
                raise EOFError
        cc = numpy.array(data, dtype='int16')
        cc.shape = (8, 96)
        cc64 = numpy.array(cc, dtype='int64')
        lags = numpy.array(head[50:58], dtype='uint16')
        lags64 = numpy.array(lags, dtype='int64')
        # combine lags and data to ensure validity of first value in
        # cc-channels
        zlags = numpy.left_shift(lags64, 4) + \
            numpy.bitwise_and(cc64[:, 0], 0xf)
        zlags.shape = (8, 1)
        mode = ac.Mode(head)
        seq, chips, band_start = get_seq(mode)

        for ind in range(8):
            if any(ind == band_start):
                cc64[ind, 0] = zlags[ind, 0]
                if cc64[ind, 2] > 0:
                    # find potential underflow in third element of cc
                    cc64[ind, 2] -= 65536
        # cc64[:,0]=zlags[:,0]
        # find potential underflow in third element of cc
        # mask = cc64[:,2]>0
        # cc64[mask,2]-=65536
        # scale
        if ac.IntTime(head) == 0:
            IntTime = 9999
        else:
            IntTime = ac.IntTime(head)
        cc64 = cc64 * 2048.0 * (1 / IntTime) / (CLOCKFREQ / 2.0)
        mon = numpy.array(head[16:32], dtype='uint16')
        mon.shape = (8, 2)
        # find potential overflows/underflows in monitor values
        mon64 = numpy.bitwise_and(zlags, 0xf0000) + mon
        overflow_mask = numpy.abs(mon64 - zlags) > 0x8000
        mon64[overflow_mask & (mon64 > zlags)] -= 0x10000
        mon64[overflow_mask & (mon64 < zlags)] += 0x10000
        # scale
        mon64 = mon64 * 1024.0 * (1 / IntTime) / (CLOCKFREQ / 2.0)
        prescaler = head[49]
        datadict = {
            'stw': stw,
            'backend': back,
            'frontend': ac.Frontend(head),
            'sig_type': ac.Type(head),
            'ssb_att': "{{{0},{1},{2},{3}}}".format(*ac.Attenuation(head)),
            'ssb_fq': "{{{0},{1},{2},{3}}}".format(*ac.SSBfrequency(head)),
            'prescaler': prescaler,
            'inttime': IntTime,
            'mode': mode,
            'acd_mon': mon64,
            'cc': cc64,
        }
        return datadict
    raise EOFError


def getFBA(fba):
    """AC factory.
    reads a fileobject and creates a dictionary for easy insertation
    into a postgresdatabase. Uses Ohlbergs routines to read the files (ACfile)
    """
    word = fba.getSpectrumHead()
    while word is not None:
        stw = fba.stw
        mech = fba.Type(word)
        datadict = {
            'stw': stw,
            'mech_type': mech,
        }
        return datadict
    raise EOFError


def getATT(file):
    """use Ohlbergs code to read in attitude data from file
    and creates a dictionary for easy insertation
    into a postgresdatabase.
    """
    ap = attitude.AttitudeParser([file])
    # sorting attitudes
    tbl = ap.table
    keys = tbl.keys()
    if not keys:
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
                err0 = qe0[0] * qe0[0] + qe0[1] * qe0[1] + qe0[2] * qe0[2]
                err1 = qe1[0] * qe1[0] + qe1[1] * qe1[1] + qe1[2] * qe1[2]
                # print "errors:", err0, err1
                if err1 < err0:
                    del tbl[key0]
                    key0 = key1
                else:
                    del tbl[key1]
        else:
            key0 = key1

    datalist = []
    for key in keys:
        try:
            (year, mon, day, hour, minute,
             secs, stw, orbit, qt, qa, qe, gps, acs) = ap.table[key]
            datadict = {
                'year': year,
                'mon': mon,
                'day': day,
                'hour': hour,
                'min': minute,
                'secs': secs,
                'stw': stw,
                'orbit': orbit,
                'qt': "{{{0},{1},{2},{3}}}".format(*qt),
                'qa': "{{{0},{1},{2},{3}}}".format(*qa),
                'qe': "{{{0},{1},{2}}}".format(*qe),
                'gps': "{{{0},{1},{2},{3},{4},{5}}}".format(*gps),
                'acs': acs,
                'soda': int(ap.soda),
            }
            datalist.append(datadict)
        except BaseException:
            pass
    return datalist


def get_seq(mode):
    '''get the ac chip configuration from the mode parameter'''
    seq = numpy.zeros(16)
    seq.dtype = int
    ssb = [1, -1, 1, -1, -1, 1, -1, 1]
    mode = (mode << 1) | 1
    for i in range(8):
        if (mode & 1):
            m = i
        seq[2 * m] = seq[2 * m] + 1
        mode >>= 1

    for i in range(8):
        if (seq[2 * i]):
            if (ssb[i] < 0):
                seq[2 * i + 1] = -1
            else:
                seq[2 * i + 1] = 1
        else:
            seq[2 * i + 1] = 0
    chips = []
    band_start = []
    band = 0
    '''chips is a list of vectors'''
    '''for example chips=[[0], [1, 2, 3, 4], [5, 6, 7]] gives
       that we observe three bands: the first is
       from a single chip, for second band chip 1,2,3,4 are cascaded,
       for third band chip 5,6,7 are cascaded'''
    for ind, se in enumerate(seq):
        if ind == band:
            band_start.append(ind / 2)
            chips.append(range(ind / 2, ind / 2 + se))
            band = ind + 2 * se
    band_start = numpy.array(band_start)

    return seq, chips, band_start


def stw_correction(datafile):
    hex_part_of_filename = splitext(basename(datafile))[0]
    file_stw = int(hex_part_of_filename, 16) << 4
    return file_stw & 0xF00000000


def import_file(datafile):
    extension = splitext(datafile)[1]
    fgr = StringIO()
    logger = logging.getLogger('level0.process')
    logger.info('importing file {0}'.format(datafile))
    if extension == '.ac1' or extension == '.ac2':
        f = ACfile(datafile)
        f2 = ACfile(datafile)
        while True:
            try:
                datadict = getAC(f)
                discipline = getACdis(f2)
                datadict['stw'] += stw_correction(datafile)
                if (datadict['inttime'] != 9999 and discipline == 'AERO' and
                    datadict['frontend'] is not None and
                        datadict['sig_type'] != 'problem'):
                    # create an import file to dump in data into
                    fgr.write(
                        str(datadict['stw']) + '\t' +
                        str(datadict['backend']) + '\t' +
                        str(datadict['frontend']) + '\t' +
                        str(datadict['sig_type']) + '\t' +
                        str(datadict['ssb_att']) + '\t' +
                        str(datadict['ssb_fq']) + '\t' +
                        str(datadict['prescaler']) + '\t' +
                        str(datadict['inttime']) + '\t' +
                        str(datadict['mode']) + '\t' +
                        '\\\\x' + datadict['acd_mon'].tostring().encode('hex') + '\t' +  # noqa
                        '\\\\x' + datadict['cc'].tostring().encode('hex') + '\t' +  # noqa
                        str(split(datafile)[1]) + '\t' +
                        str(datetime.now()) + '\n')
            except EOFError:
                break
            except ProgrammingError:
                continue
        conn = psycopg2.connect(config.get('database', 'pgstring'))
        cur = conn.cursor()
        fgr.seek(0)
        cur.execute("create temporary table foo ( like ac_level0 );")
        cur.copy_from(file=fgr, table='foo')
        cur.execute(
            "select stw,count(*) from foo group by stw having count(*)>1")
        cur2 = conn.cursor()
        for r in cur:
            print r
            cur2.execute('''delete from foo where stw={0} and created=any(array(select created from foo where stw={0} limit {1}))'''.format(*[r[0], r[1] - 1]))  # noqa
        cur2.close()
        fgr.close()
        cur.execute(
            "delete from  ac_level0 ac using foo f where f.stw=ac.stw and ac.backend=f.backend")  # noqa
        cur.execute("insert into ac_level0 (select * from foo)")
        conn.commit()
        conn.close()

    elif extension == '.fba':
        f = FBAfile(datafile)
        while True:
            try:
                datadict = getFBA(f)
                datadict['stw'] += stw_correction(datafile)
                # create an import file to dump in data into db
                fgr.write(
                    str(datadict['stw']) + '\t' +
                    str(datadict['mech_type']) + '\t' +
                    str(split(datafile)[1]) + '\t' +
                    str(datetime.now()) + '\n')
            except EOFError:
                break
            except ProgrammingError:
                continue

        conn = psycopg2.connect(config.get('database', 'pgstring'))
        cur = conn.cursor()
        fgr.seek(0)
        cur.execute("create temporary table foo ( like fba_level0 );")
        cur.copy_from(file=fgr, table='foo')
        cur.execute(
            "select stw,count(*) from foo group by stw having count(*)>1")
        cur2 = conn.cursor()
        for r in cur:
            print r
            cur2.execute('''delete from foo where stw={0} and
                        created=any(array(select created from foo where stw={0} limit {1}))'''.format(*[r[0], r[1] - 1]))  # noqa
        cur2.close()
        fgr.close()
        cur.execute(
            "delete from  fba_level0 fba using foo f where f.stw=fba.stw")
        cur.execute("insert into fba_level0 (select * from foo)")
        conn.commit()
        conn.close()

    elif extension == '.att':
        datalist = getATT(datafile)
        for datadict in datalist:
            fgr.write(str(datadict['stw']) + '\t' +
                      str(datadict['soda']) + '\t' +
                      str(datadict['year']) + '\t' +
                      str(datadict['mon']) + '\t' +
                      str(datadict['day']) + '\t' +
                      str(datadict['hour']) + '\t' +
                      str(datadict['min']) + '\t' +
                      str(datadict['secs']) + '\t' +
                      str(datadict['orbit']) + '\t' +
                      str(datadict['qt']) + '\t' +
                      str(datadict['qa']) + '\t' +
                      str(datadict['qe']) + '\t' +
                      str(datadict['gps']) + '\t' +
                      str(datadict['acs']) + '\t' +
                      str(split(datafile)[1]) + '\t' +
                      str(datetime.now()) + '\n')

        conn = psycopg2.connect(config.get('database', 'pgstring'))
        cur = conn.cursor()
        fgr.seek(0)
        cur.execute("create temporary table foo ( like attitude_level0 );")
        cur.copy_from(file=fgr, table='foo')
        fgr.close()
        cur.execute(
            "delete from  attitude_level0 att using foo f where f.stw=att.stw")
        cur.execute("insert into attitude_level0 (select * from foo)")
        conn.commit()
        conn.close()

    elif extension == '.shk':
        hk = SHKfile(datafile)
        datadict = getSHK(hk)
        for data in datadict:
            for index, stw in enumerate(datadict[data][0]):
                stw += stw_correction(datafile)
                fgr.write(str(stw) + '\t' +
                          str(data) + '\t' +
                          str(float(datadict[data][1][index])) + '\t' +
                          str(split(datafile)[1]) + '\t' +
                          str(datetime.now()) + '\n')

        conn = psycopg2.connect(config.get('database', 'pgstring'))
        cur = conn.cursor()
        fgr.seek(0)
        cur.execute("create temporary table foo ( like shk_level0 );")
        cur.copy_from(file=fgr, table='foo')
        cur.execute(
            "select stw,shk_type,count(*) from foo group by stw,shk_type having count(*)>1")  # noqa
        cur2 = conn.cursor()
        for r in cur:
            print r
            cur2.execute('''delete from foo where stw={0} and shk_type='{1}' and
                        created=any(array(select created from foo where stw={0} and shk_type='{1}' limit {2}))'''.format(*[r[0], r[1], r[2] - 1]))  # noqa
        cur2.close()
        fgr.close()
        cur.execute("delete from  shk_level0 shk using foo f where f.stw=shk.stw")  # noqa
        cur.execute("insert into shk_level0 (select * from foo)")
        conn.commit()
        conn.close()
    else:
        pass


def level0data2db():
    if len(argv) > 1:
        files = argv[1::]
        for file in files:
            print file
            import_file(file)
