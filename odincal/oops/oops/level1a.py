#!/usr/bin/python

import sys
import string
import os
import math

import odin
from process import SourceProcessor, offset


def usage():
    print "%s <source name>.py [backends ...]" % (sys.argv[0])
    sys.exit(0)


if __name__ == "__main__":
    if len(sys.argv) == 1:
        usage()

    execfile(sys.argv[1])
    if len(sys.argv) == 2:
        backends = ('AC1', 'AC2', 'AOS')
    else:
        backends = (sys.argv[2:])

    print "topic:       ", topic
    print "source:      ", source
    print "obsmode:     ", mode
    for backend in backends:
        for run in runs:
            orbits = run[0]
            on = run[1]
            print "orbits:      ", orbits
            print "coordinates: ", on
            if mode == 'TPW':
                off = run[2]
                print "off pos.:    ", off

            sp = SourceProcessor(source, topic, orbits, backend)
            dir = sp.setupDir()
            odin.LogAs("astro", os.path.join(dir, backend + ".log"))
            sp.findSource()

            odin.Info("produce level 1a data")
            sp.getLevel1a(mode)

            if mode == 'TPW':
                odin.Info("sort total power data into phases")
                offpos = offset(on, off)
                odin.Info(
                    "off position at %5.1f%5.1f" %
                    (offpos[0], offpos[1]))
                sp.sortTPW(offpos)

                if backend != 'AOS':
                    odin.Info("find calibration data")
                    sp.findCAL()

#            sp.FITS("1A")
