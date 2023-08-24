#!/usr/bin/python
#
#       copy of aeronomy.py adapted to use with filterbank
#       creates batchfile that calls fba.py
#
#       Glenn Persson, Aug. 2008
#
#       Use:
#       $ python fbaeronomy.py <orbit1> <orbit2>  > batchfile
#       $ bash batchfile
#
#       where orbit1 = first orbit to process
#             orbit2 =  last orbit to process
#
import sys
import string
import odinpath
import pg
import os


def HDFname(orbit):
    bcode = 'D'
    ocode = "%04X" % (orbit)
    hdffile = "O" + bcode + "1B" + ocode
    return hdffile


if len(sys.argv) < 3:
    print "usage: %s first last" % (sys.argv[0])
else:
    db = pg.connect('smr', 'localhost', user='smr')
    query = "SELECT orbit FROM orbittbl"
    query = query + "\n\tWHERE discipline = 'AERO'"
    query = query + "\n\tAND orbit >= %d" % (string.atoi(sys.argv[1]))
    query = query + "\n\tAND orbit <= %d" % (string.atoi(sys.argv[2]))
    query = query + "\n\tORDER BY orbit"
    # print query

    orbits = []
    q = db.query(query)
    for row in q.getresult():
        orbits.append(row[0])

    backend = "FBA"
    for orbit in orbits:
        print "# orbit %d" % (orbit)
        print "rm -f /tmp/hdffiles"
        dir = odinpath.aeropath(orbit, backend, single=0)
        file = HDFname(orbit)
        hdfname = os.path.join(dir, file + ".HDF.gz")
        logname = os.path.join(dir, file + ".LOG")
        print "python fba.py %d" % (orbit)
        print "python aero_1b_logA.py %d FBA" % (orbit)
        print "if [ -r /tmp/hdffiles ]"
        print "then"
        print "  /usr/bin/sort /tmp/hdffiles | python pdcaero.py"
        print "  chmod +x /tmp/hdfscript"
        print "  /tmp/hdfscript"
        print "fi"
        print "python aero_1b_logB.py %d FBA" % (orbit)
        print
