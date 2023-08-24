import odin
import numpy
from pg import DB


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname='odin')


def djl(year, mon, day, hour, min, secs):
    dn = 367 * year - 7 * (year + (mon + 9) / 12) / 4 \
        - 3 * ((year + (mon - 9) / 7) / 100 + 1) / 4 + 275 * mon / 9 \
        + day + 1721029
    jd = float(dn) - 0.5 + (hour + (min + secs / 60.0) / 60.0) / 24.0
    return jd


con = db()
query = con.query(
    '''select year,mon,day,hour,min,secs,stw,orbit,qt,qa,qe,gps,acs
	           from attitude_level0''')
result = query.dictresult()

query = con.query('''select stw,inttime from ac_level0 where sig_type='SIG'
order by stw''')
sigresult = query.dictresult()


for sig in sigresult:
    query = con.query('''select year,mon,day,hour,min,secs,stw,
                           orbit,qt,qa,qe,gps,acs
	                   from attitude_level0 where
                           stw>{0}-200000 and stw<{0}+200000 order by stw'''.format(sig['stw'] + 2**32))
    result = query.dictresult()
    if len(result) > 0:
        stw = float(sig['stw']) + 2**32 - sig['inttime'] * 16.0 / 2.0
        cols = 2 + 2 * 4 + 3 + 6 + 1
        rows = len(result)
        lookup = numpy.zeros(shape=(rows, cols))
        attstw = numpy.zeros(shape=(rows,))
        for index, row in enumerate(result):
            JD = djl(row['year'], row['mon'], row['day'],
                     row['hour'], row['min'], row['secs'])
            attstw[index] = float(row['stw'])
            lookup[index, 0: 2] = numpy.array((JD, row['orbit']))
            qt = eval(row['qt'].replace('{', '(').replace('}', ')'))
            lookup[index, 2: 6] = numpy.array(qt)
            qa = eval(row['qa'].replace('{', '(').replace('}', ')'))
            lookup[index, 6:10] = numpy.array(qa)
            qe = eval(row['qe'].replace('{', '(').replace('}', ')'))
            lookup[index, 10:13] = numpy.array(qe)
            gps = eval(row['gps'].replace('{', '(').replace('}', ')'))
            lookup[index, 13:19] = numpy.array(gps)
            lookup[index, 19] = row['acs']
        ind = (attstw > stw).nonzero()[0]
        if len(ind) == 0:
            pass
            # continue
        if ind[0] == 0:
            pass
            # continue
        # ind=ind[0]
        i = 3
        dt = attstw[i] - attstw[i - 1]
        dt0 = attstw[i] - stw
        dt1 = stw - attstw[i - 1]
        att = (dt0 * lookup[i - 1, :] + dt1 * lookup[i, :]) / dt
        t = (att[0], long(stw), att[1],
             tuple(att[2:6]), tuple(att[6:10]),
             tuple(att[10:13]), tuple(att[13:19]), att[19])
        s = odin.Spectrum()
        s.stw = long(stw)
        s.Attitude(t)
        print s.longitude, s.latitude, s.altitude
        print s.gpspos, s.gpsvel, s.sunpos, s.sunzd
        print s.ra2000, s.dec2000, s.vgeo, s.skybeamhit
        print dir(s)
# (year,mon,day,hour,min,sec,stw,orbit,qt,qa,qe,gps,acs)
