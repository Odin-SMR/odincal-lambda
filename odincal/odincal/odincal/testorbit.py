from pg import DB
import numpy


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname='odin')


class Spectra:
    def __init__(self, con, data):
        self.data = numpy.ndarray(shape=(112 * 8,),
                                  dtype='float64', buffer=data['spectra'])
        self.stw = data['stw']
        self.LO = data['ssb_fq']
        self.backend = data['backend']
        self.frontend = data['frontend']
        self.vgeo = data['vgeo']
        if data['sig_type'] != 'sig':
            self.vgeo = 0
        self.tcal = data['hotloada']
        self.freqres = 1e6
        self.inttime = data['inttime']
        self.intmode = 511
        self.skyfreq = []
        self.lofreq = data['lo']
        self.ssb = data['ssb']
        self.Tpll = data['imageloadb']
        self.current = data['mixc']
        self.type = data['sig_type']
        self.source = []
        self.topic = []
        self.restfreq = []
        self.latitude = data['latitude']
        self.longitude = data['longitude']
        self.altitude = data['altitude']


def find_orbits(con):
    # find out which orbits we have and start/end stws
    query = con.query('''select floor(orbit)
                       from attitude_level0
                       group by floor(orbit)
                       ''')
    result = query.getresult()
    orbits = numpy.array(result)
    orbits.shape = (orbits.shape[0],)
    orbit_info = []
    for orbit in orbits:
        query = con.query('''select min(stw),max(stw)
                           from attitude_level0 where
                           floor(orbit)={0}'''.format(orbit))
        result = query.dictresult()
        orbit_info.append((orbit, result[0]['min'], result[0]['max']))
    return orbit_info


def find_scan_in_orbit(con, orbit):
    # find out which scans that starts in the orbit
    orbitscandict = {
        'orbit': [],
        'orbit_start': [],
        'orbit_end': [],
        'scan_start': [],
        'scan_end': [],
        'scans': [],
    }
    query = con.query('''select stw,start
                   from ac_level0
                   natural left join getscans()
                   where start>={1} and start<={2}
                   and backend='AC2'
                   order by stw'''.format(*orbit))
    # AC2 matches with fba data
    result = query.dictresult()

    if len(result) == 0:
        return []
    starts = []
    stws = []
    for row in result:
        starts.append(row['start'])
        stws.append(row['stw'])

    starts = numpy.array(starts)
    starts_unique = numpy.unique(starts)
    stws = numpy.array(stws)

    # find out ac2 stws (min and max) for each scan
    scanstart = []
    scanend = []
    scans = []
    for start in starts_unique:
        # stws that belongs to a scan
        if start != -1 and start is not None:
            stwscan = stws[:, numpy.nonzero(starts == start)]
            scanstart.append(stwscan.min())
            scanend.append(stwscan.max())
            scans.append(stwscan)
            # find out if this scan is processed
            # proctest=[stwscan.min(),stwscan.max()]
            # query=con.query('''select stw from ac_level1b
            #       where stw>={0} and stw<={1}
            #       and backend='AC2'
            #       order by stw'''.format(*proctest))
            # result=query.getresult()

    if len(scanstart) > 0:
        orbitscandict['orbit'] = orbit[0]
        orbitscandict['orbit_start'] = orbit[1]
        orbitscandict['orbit_end'] = orbit[2]
        orbitscandict['scan_start'] = scanstart
        orbitscandict['scan_end'] = scanend
        orbitscandict['scans'] = scans
        return orbitscandict
    else:
        return []


def data_from_orbit(con, orbit):
    # first select all necessary data for each orbit
    stwlim = [min(orbit['scan_start']), max(orbit['scan_end'])]
    query = con.query('''select stw,backend,frontend,sig_type,spectra,inttime,
                       latitude,longitude,altitude,lo,ssb,
                       mixc,imageloadb,hotloada,ssb_fq,mech_type,vgeo
                       from ac_level0
                       natural join ac_level1a
                       natural join shk_level1
                       natural join fba_level0
                       natural left join attitude_level1
                       where stw>={0} and stw<={1}
                       and backend='AC2'
                       order by stw'''.format(*stwlim))

    result = query.dictresult()

    if len(result) == 0:
        return

    # check if already processed
    stwac2 = []
    for data in result:
        if data['backend'] == 'AC2':
            stwac2.append(data['stw'])
    stwac2 = numpy.array(stwac2)
    processlim = [orbit['orbit'], 'AC2']
    query = con.query('''select stws,stws_shape,processed from process where
                       orbit={0} and backend='{1}' '''.format(*processlim))
    proresult = query.dictresult()
    if len(proresult) == 0:
        # not yet processed , insert into process
        pass
    else:
        stws = numpy.ndarray(shape=eval(proresult[0]['stws_shape']),
                             dtype='int64', buffer=proresult[0]['stws'])
        if len(stwac2) == len(stws):
            if numpy.all(stwac2 == stws):
                # no new data
                if proresult[0]['processed'] == 1:
                    # already processed
                    return
                elif proresult[0]['processed'] == 0:
                    # not processed
                    pass
            else:
                # new data,process again even if processed
                pass
        elif len(stwac2) > len(stws):
            # new data, update process, and process
            pass

    temp = {
        'orbit': processlim[0],
        'backend': 'AC2',
        'processed': 0,
        'stws': stwac2.tostring(),
        'stws_shape': stwac2.shape,
    }
    con.insert('process', temp)

    frontend = []
    for row in result:
        frontend.append(row['frontend'])
    # group data by frontend
    frontend = numpy.array(frontend)
    frontends = numpy.unique(frontend)
    if len(frontends) == 1:
        fe = {frontends[0]: [],
              }
    elif len(frontends) == 2:
        fe = {frontends[0]: [],
              frontends[1]: [],
              }
    elif len(frontends) == 3:
        fe = {frontends[0]: [],
              frontends[1]: [],
              frontends[2]: [],
              }
    elif len(frontends) == 4:
        fe = {frontends[0]: [],
              frontends[1]: [],
              frontends[2]: [],
              frontends[3]: [],
              }
    for row in result:
        if row['sig_type'] == 'REF' and row['mech_type'] == 'CAL':
            row['sig_type'] = 'CAL'
        # if row['mech_type']=='SK2':
        #   row['sig_type']='SK2'

        spec = Spectra(con, row)
        fe[spec.frontend].append(spec)
    return fe


def testorbit():
    con = db()
    # get orbits
    orbits = find_orbits(con)
    print 'we have attitude data from ' + str(len(orbits)) + ' orbits '
    for orbit in orbits:
        # find out which scans that starts in orbits
        scans = find_scan_in_orbit(con, orbit)
        if scans:
            print 'orbit ' + str(orbit[0]) + ' has ' + \
                str(len(scans['scans'])) + ' scans'
        else:
            print 'orbit ' + str(orbit[0]) + ' has ' + str(0) + ' scans'
            continue
        # if scans have been processed continue
        if len(scans):
            # get data from scans in orbit and group them by frontend
            fe = data_from_orbit(con, scans)
            # check if the scans already have been processed
            # fe is a dictionary with keys frontends
            for key in fe.keys():
                temp = [orbit[1], orbit[2], key]
                query = con.query('''select start
                   from ac_level0
                   natural left join getscans()
                   where start>={0} and start<={1}
                   and backend='AC2' and frontend='{2}' group by start
                   '''.format(*temp))
                res = query.getresult()
                print 'frontend ' + key + ' has ' + \
                    str(len(fe[key])) + ' / ' + str(len(res)) + \
                    ' spectra / scans in orbit ' + str(orbit[0])

                return fe
            break


if 0:
    con = db()
    query = con.query('''select stw,backend,frontend,sig_type,spectra,mech_type
                       from ac_level0
                       natural left join ac_level1a
                       natural join fba_level0
                       where backend='AC2'
                       and stw>=5530211315 and stw<=5530211507+200
                       order by stw ''')

    result = query.dictresult()
    for data in result:
        x = numpy.ndarray(shape=(112 * 8,),
                          dtype='float64', buffer=data['spectra'])
        print data['sig_type']
        print data['mech_type']

    process_dict = {'ob1xxx': [['ac1', [1, 4, 8, 12], [[1, 2, 3], [
        4, 5, 6, 7], [8, 9, 10, 11], [12, 13, 14, 15]]]], 'ob1xxy': [], }


# select stw,mech_type,sig_type,backend,frontend,mode,ssb_fq from fba_level0
# natural join ac_level0 where backend='AC2'
# and stw>= 5530211315
# order by stw;
# 5530211315
