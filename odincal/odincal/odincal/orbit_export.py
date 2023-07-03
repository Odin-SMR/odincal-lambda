import numpy
from oops import odin
from pg import DB
from sys import  argv, exit
import h5py


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname='odin')


if __name__ == "__main__":
    orbit = argv[1]
    backend = argv[2]
    outfile = argv[3]

    con = db()
    # find min and max stws from orbit
    query = con.query('''select min(stw),max(stw)
                       from attitude_level0 where
                       floor(orbit)={0}'''.format(orbit))
    result = query.dictresult()
    if result[0]['max'] is None:
        print 'no attitude data from orbit ' + str(orbit)
        exit(0)

    # find out which scans that starts in the orbit
    if backend == 'AC1':
        stwoff = 1
    else:
        stwoff = 0
    temp = [result[0]['min'], result[0]['max'], backend, stwoff]
    query = con.query('''select min(ac_level0.stw),max(ac_level0.stw)
                   from ac_level0
                   join getscans() on (getscans.stw+{3}=ac_level0.stw)
                   where start>={0} and start<={1}
                   and backend='{2}'
                   '''.format(*temp))

    result = query.dictresult()
    if result[0]['max'] is None:
        print 'no data from ' + backend + ' in orbit ' + str(orbit)
        exit(0)

    # extract all necessary data for the orbit
    temp = [result[0]['min'], result[0]['max'], backend]
    query = con.query('''select stw,backend,orbit,mjd,lst,intmode,spectra,
                       alevel,version,
                       channels,
                       skyfreq,lofreq,restfreq,maxsuppression,
                       tsys,sourcemode,freqmode,efftime,sbpath,
                       calstw,latitude,longitude,altitude,skybeamhit,
                       ra2000,dec2000,vsource,qtarget,qachieved,qerror,
                       gpspos,gpsvel,sunpos,moonpos,sunzd,vgeo,vlsr,
                       ssb_fq,inttime,frontend,
                       hotloada,lo
                       from ac_level1b
                       natural join attitude_level1
                       natural join ac_level0
                       natural join shk_level1
                       where stw>={0} and stw<={1} and backend='{2}'
                       order by stw'''.format(*temp))
    result = query.dictresult()
    if result == []:
        print 'could not extract all necessary data for processing ' + \
            backend + ' in orbit ' + orbit
        exit(0)
    g = h5py.File(outfile, 'w')
    stw = []
    backend = []
    orbit = []
    mjd = []
    alevel = []
    lst = []
    spectra = []
    intmode = []
    channels = []
    skyfreq = []
    lofreq = []
    restfreq = []
    maxsuppression = []
    tsys = []
    sourcemode = []
    freqmode = []
    efftime = []
    sbpath = []
    latitude = []
    longitude = []
    altitude = []
    skybeamhit = []
    ra2000 = []
    dec2000 = []
    vsource = []
    qtarget = []
    qachieved = []
    qerror = []
    gpspos = []
    gpsvel = []
    sunpos = []
    moonpos = []
    sunzd = []
    vgeo = []
    vlsr = []
    ssb_fq = []
    inttime = []
    frontend = []
    hotloada = []
    lo = []
    for res in result:
        stw.append(res['stw'])
        backend.append(res['backend'])
        mjd.append(res['mjd'])
        orbit.append(res['orbit'])
        lst.append(res['lst'])
        spec = numpy.ndarray(shape=(res['channels'],), dtype='float64',
                             buffer=res['spectra'])
        spectra.append(spec)
        intmode.append(res['intmode'])
        channels.append(res['channels'])
        skyfreq.append(res['skyfreq'])
        lofreq.append(res['lofreq'])
        restfreq.append(res['restfreq'])
        maxsuppression.append(res['maxsuppression'])
        tsys.append(res['tsys'])
        sourcemode.append(res['sourcemode'] + ' FM=' + str(res['freqmode']))
        alevel.append(res['alevel'] + res['version'])
        freqmode.append(res['freqmode'])
        efftime.append(res['efftime'])
        sbpath.append(res['sbpath'])
        latitude.append(res['latitude'])
        longitude.append(res['longitude'])
        altitude.append(res['altitude'])
        skybeamhit.append(res['skybeamhit'])
        ra2000.append(res['ra2000'])
        dec2000.append(res['dec2000'])
        vsource.append(res['vsource'])
        qtarget.append(res['qtarget'])
        qachieved.append(res['qachieved'])
        qerror.append(res['qerror'])
        gpspos.append(res['gpspos'])
        gpsvel.append(res['gpsvel'])
        sunpos.append(res['sunpos'])
        moonpos.append(res['moonpos'])
        sunzd.append(res['sunzd'])
        vgeo.append(res['vgeo'])
        vlsr.append(res['vlsr'])
        ssb_fq.append(res['ssb_fq'])
        inttime.append(res['inttime'])
        frontend.append(res['frontend'])
        hotloada.append(res['hotloada'])
        lo.append(res['lo'])
    g['SMR/STW'] = stw
    g['SMR/Backend'] = backend
    g['SMR/Level'] = alevel
    g['SMR/Orbit'] = orbit
    g['SMR/MJD'] = mjd
    g['SMR/LST'] = lst
    g['spectra'] = spectra
    g['SMR/IntMode'] = intmode
    g['SMR/Channels'] = channels
    g['SMR/SkyFreq'] = skyfreq
    g['SMR/LOFreq'] = lofreq
    g['SMR/RestFreq'] = restfreq
    g['SMR/MaxSuppression'] = maxsuppression
    g['SMR/Tsys'] = tsys
    g['SMR/Source'] = sourcemode
    # g['SMR/freqmode']=freqmode
    g['SMR/EffTime'] = efftime
    g['SMR/SBpath'] = sbpath
    g['SMR/Latitude'] = latitude
    g['SMR/Longitude'] = longitude
    g['SMR/Altitude'] = altitude
    g['SMR/SkyBeamHit'] = skybeamhit
    g['SMR/RA2000'] = ra2000
    g['SMR/Dec2000'] = dec2000
    g['SMR/VSource'] = vsource
    g['SMR/Qtarget'] = qtarget
    g['SMR/Qachieved'] = qachieved
    g['SMR/Qerror'] = qerror
    g['SMR/GPSpos'] = gpspos
    g['SMR/GPSvel'] = gpsvel
    g['SMR/SunPos'] = sunpos
    g['SMR/MoonPos'] = moonpos
    g['SMR/SunZD'] = sunzd
    g['SMR/Vgeo'] = vgeo
    g['SMR/Vlsr'] = vlsr
    g['SMR/FreqCal'] = ssb_fq
    g['SMR/IntTime'] = inttime
    g['SMR/Frontend'] = frontend
    g['SMR/Tcal'] = hotloada
    # g['SMR/LOFreq']=lo
    g.close()
    con.close()
