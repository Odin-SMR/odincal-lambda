#!/usr/bin/python

import sys

from orbit import OrbitProcessor

for arg in sys.argv[1:]:
    orbit = int(arg)
    op = OrbitProcessor(orbit, None, None)
    for type in ('SHK', 'AC1', 'AC2', 'AOS', 'FBA', 'ATT', 'UVT'):
        files = op.findlevel0(type)
        # print type
        for file in files:
            print "\t", file
