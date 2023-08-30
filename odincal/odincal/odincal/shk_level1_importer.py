from pg import ProgrammingError, DB
from odincal.database import ConfiguredDatabase


def Lookup(table, stw0):
    i = 0
    while i < len(table[0]) - 1 and table[0][i + 1] < stw0:
        i = i + 1
    return table[1][i]


def shk_level1_importer(stwa, stwb, backend, pg_string=None):
    if pg_string is None:
        con = ConfiguredDatabase()
    else:
        con = DB(pg_string)
    temp = [stwa, stwb, backend]
    query = con.query('''select stw,backend,frontend from ac_level0
                       left join shk_level1 using (stw,backend)
                       where imageloadb is NULL
                       and ac_level0.stw>={0} and ac_level0.stw<={1}
                       and backend='{2}' '''.format(*temp))
    sigresult = query.dictresult()
    print len(sigresult)
    for sig in sigresult:
        # For the split modes and the currently accepted
        # frontend configurations, AC1 will hold REC_495 data in its lower half
        # and REC_549 in its upper half. AC2 will hold REC_572 data in its
        # lower half and REC_555 in its upper half.
        if sig['frontend'] == 'SPL':
            if sig['backend'] == 'AC1':
                frontends = ['495', '549']
            if sig['backend'] == 'AC2':
                frontends = ['572', '555']
        else:
            frontends = [sig['frontend']]
        for ind, frontend in enumerate(frontends):
            shkdict = {
                'stw': sig['stw'],
                'backend': sig['backend'],
                'frontendsplit': frontend,
                'lo': [],
                'ssb': [],
                'mixc': [],
                'imageloada': [],
                'imageloadb': [],
                'hotloada': [],
                'hotloadb': [],
                'mixera': [],
                'mixerb': [],
                'lnaa': [],
                'lnab': [],
                'mixer119a': [],
                'mixer119b': [],
                'warmifa': [],
                'warmifb': [],
            }
            shklist = [
                'LO',
                'SSB',
                'mixC',
                'imageloadB',
                'imageloadA',
                'imageloadB',
                'hotloadA',
                'hotloadB',
                'mixerA',
                'mixerB',
                'lnaA',
                'lnaB',
                '119mixerA',
                '119mixerB',
                'warmifA',
                'warmifB']
            if frontend == '119':
                shklist = ['imageloadA', 'imageloadB', 'hotloadA', 'hotloadB',
                           'mixerA', 'mixerB', 'lnaA', 'lnaB',
                           '119mixerA', '119mixerB', 'warmifA', 'warmifB']
            insert = 1

            for shk in shklist:
                if shk == 'LO' or shk == 'SSB' or shk == 'mixC':
                    sel = [shk + frontend, sig['stw']]
                else:
                    sel = [shk, sig['stw']]
                query = con.query('''select stw,shk_value from shk_level0
                               where shk_type='{0}' and
                               stw>{1}-2080 and stw<{1}+2080
                               order by stw'''.format(*sel[:]))
                result = query.getresult()
                if len(result) < 2 and (shk == 'mixerA' or shk == 'mixerB' or
                                        shk == 'hotloadA' or shk == 'hotloadB' or  # noqa
                                        shk == 'lnaA' or shk == 'lnaB' or
                                        shk == 'imageloadA' or shk == 'imageloadB' or  # noqa
                                        shk == '119mixerA' or shk == '119mixerB' or  # noqa
                                        shk == 'warmifA' or shk == 'warmifB'):
                    shkdict[shk.lower()] = 0
                    continue
                if len(result) < 2:
                    insert = 0
                    break
                stw = []
                data = []
                for row in result:
                    stw.append(row[0])
                    data.append(row[1])
                table = (stw, data)
                data0 = Lookup(table, sig['stw'])
                if shk == '119mixerA':
                    shkdict['mixer119a'] = float(data0)
                elif shk == '119mixerB':
                    shkdict['mixer119b'] = float(data0)
                else:
                    shkdict[shk.lower()] = float(data0)
            if insert == 1:
                try:
                    con.insert('shk_level1', shkdict)
                except ProgrammingError:
                    pass
            # query=con.query('''select LO from shk_level1''')
            # res=query.getresult()
    con.close()
