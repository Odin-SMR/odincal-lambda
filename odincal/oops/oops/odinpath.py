import sys
import os
import re


def aeropath(orbit, backend, single=1):
    if single:
        sub = "%d" % (orbit)
    else:
        sub = "%02X" % (orbit / 256)

    dir = os.path.join("/OdinData/AERO/7.0", backend)
    if not os.path.isdir(dir):
        # print "creating spectra in", dir
        os.mkdir(dir, 755)
    dir = os.path.join(dir, sub)
    if not os.path.isdir(dir):
        # print "creating spectra in", dir
        os.mkdir(dir, 755)
    # print "storing spectra in ", dir
    return dir
