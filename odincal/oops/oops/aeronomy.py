#!/usr/bin/python
#
#       aeronomy.py <orbit-start> <orbit-stop>
#
#       purpose: lookup aeronomy orbits in orbittbl and create
#                batch-file for processing from level0 to level1.
#
#
#       original version by Michael Olberg
#
#       modified:       by:     why:
#       2008-12-08      glenn   added loop counter and
#                               retrieve pdc ticket every 200:th loop
#       2009-03-17      glenn   removed code for pdcticket

#
#
import sys
import string
import odinpath
import pg
import os


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

    i = 0
    for orbit in orbits:
        i = i + 1
        print "# orbit %d (%d)" % (orbit, i)
        print "rm -f /tmp/hdffiles"
        for backend in ("AC1", "AC2"):
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
        print "python aero_1b_logB.py %d AC1" % (orbit)
        print "python aero_1b_logB.py %d AC2" % (orbit)
        print
