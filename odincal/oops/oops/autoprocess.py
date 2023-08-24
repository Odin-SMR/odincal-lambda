#!/usr/bin/python
#
#       autoprocess.py
#
#       Purpose: Automatic processing of aeronomy data from level0 to level1b
#                for version 7.0 calibration
#                Should be scheduled by cron at regular intervals
#
#
#       Glenn Persson, original version: 2007-05-31

#       2008-07-01      preparing for version 7.0
#       2008-07-07      moved auto-routines to v6.0
#       2008-12-04      moved auto-routines to ~/autoprocess.
#                               renamed autorun.sh autobatch
#       2008-12-08      new philosophy: If more orbits in orbittbl
#                       compared to parfile, prepare batchfile to run.
#
#
import os
import pg
import string
import sys
import time
from time import localtime, strftime

v7_path = "/home/smr/v7.0/"
genpath = "/home/smr/autoprocess/"
logfile = v7_path + "autoprocess.log"
parfile = v7_path + "autolastorbit"
genfile = genpath + "autoparam.log"

# --------------------------------------------------------------------------
#       log schedule time
# --------------------------------------------------------------------------

logging = open(logfile, 'a+')
while True:
    line = logging.readline()
    if not line:
        break
timebuf = strftime("%a, %d %b %Y %H:%M:%S\n", localtime())
buf = "\nscheduled at " + timebuf
logging.write(buf)

# --------------------------------------------------------------------------
#       check first if there are orbits to process
# --------------------------------------------------------------------------

file = open(parfile, 'r')
for line in file:
    lastorbit = string.strip(line)
file.close()

file = open(genfile, 'r')
params = file.readlines()
for line in params:
    line = string.strip(line)
    if (line[0:3] == 'UVT'):
        uvtfiles = int(line[4:])
    if (line[0:3] == 'SUB'):
        subdir = line[4:]
    if (line[0:3] == 'ORB'):
        orbit = int(line[4:])
file.close()

if int(lastorbit) >= orbit - 1:
    buf = " Highest orbitnumber in db: " + \
        str(orbit) + " Last processed: " + lastorbit
    logging.write(buf)
    logging.write("\n")
    logging.close()
    sys.exit()

# --------------------------------------------------------------------------
#       OK, orbits to process. Retrieve ticket to pdc
# --------------------------------------------------------------------------

cmd = "/home/smr/.pdcticket > /dev/null"
os.system(cmd)

# --------------------------------------------------------------------------
#       prepare and run batch-file
# --------------------------------------------------------------------------

first = int(lastorbit) + 1
last = orbit - 1
new_orbits = last - first + 1

cmd = " python aeronomy.py " + str(first) + " " + str(last) + " > autobatch"
os.system(cmd)
logging.write(cmd)
logging.write("\n")

buf = " processing " + str(new_orbits) + " orbits"
logging.write(buf)
logging.write("\n")

cmd = " bash autobatch"
os.system(cmd)

# --------------------------------------------------------------------------
#       send me mail message
# --------------------------------------------------------------------------

cmd = " mail -s 'autoprocessing v7.0 done' glenn.persson@chalmers.se"
os.system(cmd)

# --------------------------------------------------------------------------
#       notify teamleaders
# --------------------------------------------------------------------------

cmd = "python /home/glenn/bin/notify_aerogroup.py /home/smr/v7.0/autobatch 7"
os.system(cmd)
logging.write(cmd)
logging.write("\n")

# --------------------------------------------------------------------------
#       update last processed orbit
# --------------------------------------------------------------------------

file = open(parfile, 'w')
BUF = str(orbit - 1) + '\n'
file.write(BUF)
file.close()

logging.close()
