import numpy
from math import pi, sqrt, exp, cos
from odincal.database import ConfiguredDatabase
import ctypes
from pkg_resources import resource_filename
import psycopg2
from psycopg2 import InternalError, IntegrityError
from StringIO import StringIO
from odincal.config import config
from datetime import datetime


class Level1a:
    """A class to process level 0 files into level 1a."""

    def __init__(self):
        self.LAGSPERCHIP = 96
        self.CLOCKFREQ = 224.0e6
        self.SAMPLEFREQ = 10.0e6

    def reduceAC(self, cc_data, acd_mon_pos, acd_mon_neg):
        self.got = []
        for i in range(len(cc_data)):
            self.maxchips = len(cc_data[i]) / 96
            self.nred = self.maxchips * 112
            datamod = numpy.zeros(shape=(1, self.nred))
            datamod[0, 0:self.maxchips * self.LAGSPERCHIP] = cc_data[i]
            goti = self.reduce1Band(
                datamod[
                    0,
                    :],
                acd_mon_pos[i],
                acd_mon_neg[i])
            if goti[0] == 1:
                self.got.append(goti[1])
            else:
                self.got.append(numpy.zeros(shape=(self.nred,)))

    def reduce1Band(self, data, monitor_pos, monitor_neg):
        zlag = data[0]
        if zlag <= 0:
            return 0, 0
        if zlag > 1:
            return 0, 0
        power = zeroLag(zlag, 1.0)
        c_pos = threshold(monitor_pos)
        c_neg = threshold(monitor_neg)
        if (c_pos == 0 or c_neg == 0):
            return 0, 0
        else:
            cmean = (c_pos + c_neg) / 2.0
            dc = abs((c_pos - c_neg) / 2.0 / cmean)
            if dc > 0.1:
                return 0, 0
        data = qCorrect(cmean, data, self.nred)
        if data[0] == 0:
            return 0, 0
        # perform an fft of data
        librarypath = resource_filename('oops', 'libfft.so')
        libc = ctypes.CDLL(librarypath, mode=3)
        n0 = ctypes.c_int(112 * self.maxchips)
        c_float_p = ctypes.POINTER(ctypes.c_double)
        data0 = numpy.array(data[1])
        data_p = data0.ctypes.data_as(c_float_p)
        libc.odinfft(data_p, n0)
        # Reintroduce power into filter shapes.
        data0 = data0 * power
        return 1, data0


def hanning(data, n):
    w = pi / n
    for i in range(1, n):
        data[i] = data[i] * (0.5 + 0.5 * cos(w * i))
    return data


def inv_erfc(z):
    p = [1.591863138, -2.442326820, 0.37153461]
    q = [1.467751692, -3.013136362, 1.00000000]
    x = 1.0 - z
    y = (x * x - 0.5625)
    y = x * (p[0] + (p[1] + p[2] * y) * y) / (q[0] + (q[1] + q[2] * y) * y)
    return y


def threshold(monitor):
    if (monitor < 0.0 or monitor > 1.0):
        return 0.0
    thr = sqrt(2.0) * inv_erfc(2.0 * monitor)
    return thr


def qCorrect(c, f, n):
    # Perform quantisation correction using Kulkarni & Heiles approximation.
    # (taken from Kulkarni, S.R., Heiles, C., 1980, AJ, 85, 1413.
    A = (pi / 2.0) * exp(c * c)
    B = -A * A * A * (pow((c * c - 1), 2.0) / 6.0)
    f[0] = 1.0
    for i in range(1, n):
        fa = f[i]
        if (abs(fa) > 0.86):
            # level too high in QCorrect
            return 0, 0
        f[i] = (A + B * fa * fa) * fa
    return 1, f


def zeroLag(zlag, v):
    if (zlag >= 1.0 or zlag <= 0.0):
        return 0.0
    x = v / inv_erfc(zlag)
    return x * x / 2.0


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


def ac_level1a_importer(stwa, stwb, backend):
    con = ConfiguredDatabase()
    temp = [stwa, stwb, backend]
    query = con.query('''select ac_level0.stw,ac_level0.backend,
                 acd_mon,cc,mode from
                 ac_level0
                 left join ac_level1a using (backend,stw)
                 where ac_level1a.stw is Null
                 and ac_level0.stw>={0} and ac_level0.stw<={1}
                 and ac_level0.backend='{2}' '''.format(*temp))

    result = query.dictresult()
    ac = Level1a()
    fgr = StringIO()
    print len(result)
    for ind, rowb in enumerate(result):
        acd_mon = numpy.ndarray(shape=(8, 2), dtype='float64',
                                buffer=rowb['acd_mon'])
        cc = numpy.ndarray(shape=(8 * 96,), dtype='float64',
                           buffer=rowb['cc'])
        # xheck the mode of the correlator configuration
        seq, chips, band_start = get_seq(rowb['mode'])
        # make a list with of vectors with cc-data for the band
        # to process, take mode into account
        cclist = []
        for ind, band in enumerate(band_start):
            cclist.append(cc[band * 96:(band + len(chips[ind])) * 96, ])

        ac.reduceAC(cclist, acd_mon[band_start, 0], acd_mon[band_start, 1])

        a = numpy.zeros(shape=(8 * 112,))
        for ind, band in enumerate(band_start):
            a[band * 112:(band + len(chips[ind])) * 112] = ac.got[ind]
        fgr.write(
            str(rowb['stw']) + '\t' +
            str(rowb['backend']) + '\t' +
            '\\\\x' + abs(a).tostring().encode('hex') + '\t' +
            str(datetime.now()) + '\n')

    conn = psycopg2.connect(config.get('database', 'pgstring'))
    cur = conn.cursor()
    fgr.seek(0)
    cur.execute("create temporary table foo ( like ac_level1a );")
    cur.copy_from(file=fgr, table='foo')
    try:
        cur.execute(
            "delete from ac_level1a ac using foo f where f.stw=ac.stw and ac.backend=f.backend")  # noqa
        cur.execute("insert into ac_level1a (select * from foo)")
    except (IntegrityError, InternalError) as e:
        print e
        try:
            conn.rollback()
            cur.execute("create temporary table foo ( like ac_level1a );")
            cur.copy_from(file=fgr, table='foo')
            cur.execute(
                "delete from ac_level1a ac using foo f where f.stw=ac.stw and ac.backend=f.backend")  # noqa
            cur.execute("insert into ac_level1a (select * from foo)")
        except (IntegrityError, InternalError) as e1:
            print e1
            conn.rollback()
            conn.close()
            con.close()
            return 1
    fgr.close()
    conn.commit()
    conn.close()
    con.close()
    return 0
