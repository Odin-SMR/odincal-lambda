#!/usr/bin/python
#
#       $ python aeronomyfile.py <file>
#
#       version of aeronomy.py that reads <file> using format
#               1234567890   <- column
#               32560   AC1
#               34560   AC2
#       and prints a batch-file in the same way as original
#       aeronomy.py script
#
#       Glenn Persson, 2007-12-03
#
#
import odinpath
import os
import pg
import re
import string
import sys

if len(sys.argv) < 2:
    inputfile = input("Enter input file: ")
else:
    inputfile = sys.argv[1]


def HDFname(backend, orbit):
    if backend == 'AOS':
        bcode = 'A'
    elif backend == 'AC1':
        bcode = 'B'
    elif backend == 'AC2':
        bcode = 'C'
    elif backend == 'FBA':
        bcode = 'D'
    ocode = "%04X" % (orbit)
    hdffile = "O" + bcode + "1B" + ocode
    return hdffile


def SQLquery(orbit):
    # return status=0 if orbit is NOT an aeronomy orbit
    # return status=1 if orbit is an aeronomy orbit
    status = 0
    db = pg.connect('smr', 'localhost', user='smr')
    sql = "SELECT orbit FROM orbittbl WHERE discipline='AERO'"
    sql = sql + " AND orbit = %d" % (orbit)
    # print sql
    q = db.query(sql)
    for row in q.getresult():
        status = 1
    db.close()
    return status


file = open(inputfile, "r")
lines = file.readlines()
for line in lines:
    line = string.strip(line)
    orbit = int(line[0:5])
    if re.search('AC1', line):
        backend = 'AC1'
    elif re.search('AC2', line):
        backend = 'AC2'
    # print "orbit:",orbit
    # print "backend:",backend
    exist = SQLquery(orbit)
    if (exist == 1):
        print "# orbit %d" % (orbit)
        print "rm -f /tmp/hdffiles"
        dir = odinpath.aeropath(orbit, backend, single=0)
        file = HDFname(backend, orbit)
        hdfname = os.path.join(dir, file + ".HDF.gz")
        logname = os.path.join(dir, file + ".LOG")
        print "python aeroorbit.py %d %s" % (orbit, backend)
        print "python aero_1b_logA.py %d %s" % (orbit, backend)
        print "if [ -r /tmp/hdffiles ]"
        print "then"
        print "  /usr/bin/sort /tmp/hdffiles | python pdcaero.py"
        print "  chmod +x /tmp/hdfscript"
        print "  /tmp/hdfscript"
        print "fi"
        print "python aero_1b_logB.py %d %s" % (orbit, backend)
        print
    else:
        print "# orbit %d IS NOT AN AERONOMY ORBIT !!!!!!!!!!!!!!!!" % (orbit)
