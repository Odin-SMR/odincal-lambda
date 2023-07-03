import os
from subprocess import call
from pg import DB


class db(DB):
    def __init__(self):
        passwd = os.getenv('ODINDB_PASSWD')
        DB.__init__(
            self,
            dbname='odin',
            user='odinop',
            host='malachite',
            passwd=passwd)
        # DB.__init__(self,dbname='odin_test')


def level1b_importer():
    con = db()
    processquery = con.query('''select file,backend from ac_level0
                       left join in_process using (file)
                       left join processed using (file)
                       where (in_process.file is Null and
                              processed.file is Null)
                       and backend='AC2'
                       group by file,backend
                       ''')
    process = processquery.dictresult()

    for processrow in process:
        processtemp = {'file': processrow['file']}
        con.insert('in_process', processtemp)

        tempfile = [processrow['file']]
        binfile = (
            '/home/bengt/work/odincal_2013/odincal/bin/level1b_window_importer'
            )
        call([binfile, processrow['file'], processrow['backend'], '1'])

    con.close()


level1b_importer()
