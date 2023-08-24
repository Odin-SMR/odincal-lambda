#!/usr/bin/python

import sys
import os
import string

import odin
from process import SourceProcessor


def usage():
    print "usage: %s source topic mode orbit (AC1|AC2|AOS)" % (sys.argv[0])


if __name__ == "__main__":
    if len(sys.argv) > 5:
        source = sys.argv[1]
        topic = sys.argv[2]
        mode = sys.argv[3]
        orbit = string.atoi(sys.argv[4])
        backend = sys.argv[5]
    else:
        usage()
        sys.exit(0)

    print "topic:       ", topic
    print "source:      ", source
    print "obsmode:     ", mode
    print "backend:     ", backend
    orbits = []
    orbits.append(orbit)
    # on     = run[1]
    print "orbits:      ", orbits
    # print "coordinates: ", on
    # if mode == 'TPW':
    #    off = run[2]
    #    print "off pos.:    ", off

    sp = SourceProcessor(source, topic, orbits, backend)
    dir = sp.setupDir()
    odin.LogAs("astro", os.path.join(dir, backend + ".log"))
    sp.findSource()
#    sys.exit(0)

    odin.Info("produce level 1a data")
    sp.getLevel1a(mode)

    if mode == 'TPW':
        # odin.Info("sort total power data into phases")
        # offpos = offset(on,off)
        # odin.Info("off position at %5.1f%5.1f" % (offpos[0],offpos[1]))
        # sp.sortTPW(offpos)

        if backend != 'AOS':
            odin.Info("find calibration data")
            sp.findCAL()

    odin.Info("check for bad REF and CAL spectra")
    sp.clean()

    odin.Info("calibrate")
    sp.calibrate(mode)
    # sp.FITS("1B")
