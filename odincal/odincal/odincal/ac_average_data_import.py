import numpy as N
from pg import DB
from odincal.config import config


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname=config.get('database', 'dbname'),
                    user=config.get('database', 'user'),
                    host=config.get('database', 'host'),
                    passwd=config.get('database', 'passwd'),
                    )


def get_odin_data(con, backend, frontend, version, intmode,
                  sourcemode, freqmode, ssb_fq,
                  Tcal_low, Tcal_high, altitude_range):
    data = {
        'backend': [],
        'frontend': [],
        'version': [],
        'intmode': [],
        'sourcemode': [],
        'freqmode': [],
        'ssb_fq': [],
        'spectra': [],
        'channels': [],
        'skyfreq': [],
        'lofreq': [],
        'altitude': [],
        'hotloada': [],
    }
    temp = [backend, frontend, version, intmode,
            sourcemode, freqmode, ssb_fq,
            Tcal_low, Tcal_high, altitude_range[0], altitude_range[1]]
    print temp

    query = con.query('''select ac_level1b.spectra as spectra,channels,
                       skyfreq,lofreq,altitude,ssb_fq,hotloada,
                       ac_level1b.backend,ac_level1b.frontend,
                       version,intmode,sourcemode,
                       freqmode
                       from ac_level1b
                       join attitude_level1  using (backend,stw)
                       join ac_level0  using (backend,stw)
                       join shk_level1  using (backend,stw)
                       where ac_level1b.backend='{0}' and
                       attitude_level1.backend='{0}' and
                       ac_level0.backend='{0}' and
                       shk_level1.backend='{0}' and
                       ac_level1b.frontend='{1}'
                       and version={2} and intmode={3} and
                       sourcemode='{4}' and freqmode={5} and ssb_fq='{6}'
                       and hotloada>={7} and hotloada<={8}
                       and altitude>={9} and altitude<={10}
                       and sig_type='SIG'
                       limit 20000'''.format(*temp))
    result = query.dictresult()

    print len(result)
    for row in result:
        for item in data.keys():
            if item == 'spectra':
                spec = N.ndarray(shape=(row['channels'],), dtype='float64',
                                 buffer=row['spectra'])
                data[item].append(spec)
            elif item == 'ssb_fq':
                ssb_fq = N.array(row['ssb_fq']) * 1e6
                data['ssb_fq'].append(ssb_fq)
            else:
                data[item].append(row[item])

    for item in data.keys():
        data[item] = N.array(data[item])

    return data


def main():
    '''import average data from measuerements at high tangent points
       into the ac_level1b_average table'''
    con = db()
    version = 8
    temp = [version]
    # get keys for all processed modes
    if 0:
        query = con.query(''' select backend,ac_cal_level1b.frontend,
                  version,intmode,
                  sourcemode,freqmode,ssb_fq
                  from ac_cal_level1b
                  join ac_level0 using(backend,stw)
                  where version={0}
                  group by backend,ac_cal_level1b.frontend,version,
                  intmode,sourcemode,freqmode,ssb_fq
                  order by backend,ac_cal_level1b.frontend,version,
                  intmode,sourcemode,freqmode,ssb_fq'''.format(*temp))
    if 1:
        query = con.query(''' select backend,frontend,
                  version,intmode,
                  sourcemode,freqmode,ssb_fq
                  from ac_level1b_average
                  where version={0}
                  group by backend,frontend,version,
                  intmode,sourcemode,freqmode,ssb_fq
                  order by backend,frontend,version,
                  intmode,sourcemode,freqmode,ssb_fq'''.format(*temp))

        result = query.dictresult()
        print len(result)

    # loop over all modes
    tcalvec = range(277, 291)
    altitude_range = [80e3, 120e3]  # use data in this tangent altitude range
    for row in result:

        for tind in range(len(tcalvec) - 1):

            data = get_odin_data(con, row['backend'], row['frontend'],
                                 row['version'], row['intmode'],
                                 row['sourcemode'], row['freqmode'],
                                 row['ssb_fq'],
                                 tcalvec[tind], tcalvec[tind + 1],
                                 altitude_range)

            if len(data['backend']) == 0:
                continue
            medianTb = N.median(data['spectra'], 0)
            meanTb = N.mean(data['spectra'], 0)

            temp = {
                'backend': data['backend'][0],
                'frontend': data['frontend'][0],
                'version': data['version'][0],
                'intmode': data['intmode'][0],
                'sourcemode': data['sourcemode'][0],
                'freqmode': data['freqmode'][0],
                'ssb_fq': "{{{0},{1},{2},{3}}}".format(*
                                                       [int(data['ssb_fq'][0][0] / 1e6), int(data['ssb_fq'][0][1] / 1e6),
                                                        int(data['ssb_fq'][0][2] / 1e6), int(data['ssb_fq'][0][3] / 1e6)]),
                'hotload_range': "{{{0},{1}}}".format(*
                                                      [float(tcalvec[tind]), float(tcalvec[tind + 1])]),
                'altitude_range': "{{{0},{1}}}".format(*altitude_range),
                'median_spectra': medianTb.tostring(),
                'mean_spectra': meanTb.tostring(),
                'channels': len(medianTb),
                'skyfreq': N.mean(data['skyfreq'], 0),
                'lofreq': N.mean(data['lofreq'], 0),
            }

            tempkeys = [temp['backend'], temp['frontend'], temp['version'],
                        temp['intmode'], temp['sourcemode'], temp['freqmode'],
                        temp['ssb_fq'], temp['hotload_range'],
                        temp['altitude_range']]
            con.query('''delete from ac_level1b_average
                     where backend='{0}' and frontend='{1}' and
                     version={2} and intmode={3} and sourcemode='{4}'
                     and freqmode={5} and ssb_fq='{6}' and
                     hotload_range='{7}' and altitude_range='{8}'
                     '''.format(*tempkeys))

            con.insert('ac_level1b_average', temp)

    con.close()
