#!/usr/bin/python
#
#       $ python aero_1b_logA.py <orbit> <AC1|AC2|FBA>
#
#       Purposes:       Inserts/updates sql table 'level1' after process
#                       to level1 completion.
#                       Also, it inserts/updates sql table 'status'
#                       with status word and error messages depending on outcome.
#
#       Uses:           tables 'level1','status' (local @ ymer.oso.chalmers.se)
#                       tables 'level1','status' (remote @ jet.rss.chalmers.se)
#
#
#       Glenn Persson, original version: 2006-11-01
#
#       Modifications:
#
#       2007-02-20      Changed freqmode handling. Reading into string
#                       rather than single integer value as freqmode can
#                       take more than a single value.
#                       sql column types were changed from smallint to varchar
#       2007-02-23      Inserts/updates table row for each processed orbit,
#                       rather than doing it only for successful orbits.
#                       Also inserts/updates new status table.
#       2008-01-16      If freqmode == '\N', it will be set to '0'
#                       Pathname prefix =  '/OdinData/AERO/6.0/' to logfile
#       2008-02-12      Clear errmsg in status table when status=1
#       2008-02-25      Adding 1 second sleep before checking status and
#                       scheduling statusmess for updating error message(s)
#       2008-03-18      Always updating attversion and start/stop_utc even
#                       if processing fails, i.e. logfile doesn't exist.
#       2008-03-19      If .LOG doesn't exist retrieve 'processed' keyword
#                       from .TXT file
#       2008-07-09      Prepared for calversion 7. Inserts new row
#                       rather than updates, when calversions differ
#       2008-08-13      Added handling of FBA orbits
#       2008-08-18      Adapted for cal. version 7.0
#       2008-12-17      Set freq.mode=0 when not defined
#       2009-03-18      Avoid program crash if no attfile is found
#
#       Remaining issue to be solved
#               What to do when > 1 ATT file is found for an orbit???
#
#
#

import MySQLdb
import os
import pg
import re
import stat
import string
import sys
import time

from orbit import OrbitProcessor
from odintime import TimeConverter
from datetime import datetime

from jet import *

if len(sys.argv) < 3:
    print "usage: %s orbit <AC1|AC2|FBA>" % (sys.argv[0])
    sys.exit()

if len(sys.argv) > 2:
    orbit = int(sys.argv[1])
    backend = sys.argv[2]

if (backend != 'AC1') and (backend != 'AC2') and (backend != 'FBA'):
    print sys.argv[0] + " ERROR! -> not valid backend!"
    sys.exit()

table1 = "soda"
table2 = "level1"
table3 = "status"
calfileACn = "aeroorbit.py"
calfileFBA = "fba.py"

# ------------------------------------------------------------------------------
#       construct full pathname to LOGfile for actual orbit/backend
#       extract calversion. Use 'calerror' to indicate success or failure
# ------------------------------------------------------------------------------

hex_string = string.upper(hex(orbit))[2:]
if (len(hex_string) < 4):
    hex_orbit = "0" + hex_string
else:
    hex_orbit = hex_string
if backend == 'AC1':
    file = "/OdinData/AERO/7.0/AC1/" + \
        hex_orbit[0:2] + "/OB1B" + hex_orbit + ".LOG"
    cal = open(calfileACn, "r")
elif backend == 'AC2':
    file = "/OdinData/AERO/7.0/AC2/" + \
        hex_orbit[0:2] + "/OC1B" + hex_orbit + ".LOG"
    cal = open(calfileACn, "r")
else:
    file = "/OdinData/AERO/7.0/FBA/" + \
        hex_orbit[0:2] + "/OD1B" + hex_orbit + ".LOG"
    cal = open(calfileFBA, "r")

caldata = cal.readlines()
calerror = 1
for line in caldata:
    if re.search('VERSION =', line):
        words = string.split(line)
        calver = words[2]
        calerror = 0
cal.close()
if calerror == 1:
    calversion = "None"
else:
    calversion = string.atof(calver)

# ------------------------------------------------------------------------------
#       find out .ATT file(s) for this orbit
#       find out 'attitude reconstruction version number' in soda table
#       (WHAT TO DO WHEN > 1 ATT FILE IS FOUND?!)
# ------------------------------------------------------------------------------

op = OrbitProcessor(orbit, None, None)
files = op.findlevel0('ATT')
attfile = ""
no_of_attfiles = 0
for attfiles in files:
    attfile = attfiles
    print "attfile:    ", attfile
    no_of_attfiles = no_of_attfiles + 1
if (no_of_attfiles > 1):
    print "====================> Oops! more than 1 attfile found..."

if len(attfile) == 0:
    print "-----------------------------------------------------------------"
    print "My God! No attfile found for this orbit. Cannot do any real work!"
    print "-----------------------------------------------------------------"
else:
    db = pg.connect('smr', 'ymer', user='smr')
    query = "SELECT version FROM " + table1 + " WHERE file = '" + attfile + "';"
    # print query
    q = db.query(query)
    attversion = 0.0
    for row in q.getresult():
        attversion = row[0]
    db.close()

# ------------------------------------------------------------------------------
#       use class 'TimeConverter' to calculate start_utc and stop_utc
#       from orbit number
# ------------------------------------------------------------------------------

tc = TimeConverter()

JD = tc.orbit2JD(orbit)
(year, mon, day, hour, min, secs) = tc.cld(JD)
# start_utc = '%4d' % year +'-%02d' % mon +'-%02d' % day +' %02d' % hour +':%02d' % min +':%02.0f' % secs
start_utc = datetime(year, mon, day, hour, min, int(secs))

JD = tc.orbit2JD(orbit + 1)
(year, mon, day, hour, min, secs) = tc.cld(JD)
# stop_utc = '%4d' % year +'-%02d' % mon +'-%02d' % day +' %02d' % hour +':%02d' % min +':%02.0f' % secs
stop_utc = datetime(year, mon, day, hour, min, int(secs))

# ------------------------------------------------------------------------------
#       check existence of LOGfile. Set logexist word depending on outcome
# ------------------------------------------------------------------------------

try:
    log = open(file, "r")
    logexist = 1
except IOError:
    print "--------------------------------------------------------------"
    print "!!!!!!!!!!!!!!--> LOG file was not produced! <--!!!!!!!!!!!!!!"
    print "--------------------------------------------------------------"
    logexist = 0

# ------------------------------------------------------------------------------
#       if LOG file exists, extract all parameters needed
# ------------------------------------------------------------------------------

if logexist == 1:
    buf = os.stat(file)
    created = time.strftime("%Y-%m-%d %H:%M:%S",
                            time.localtime(buf[stat.ST_CTIME]))
    logdata = log.readlines()
    log.close()
    flist = []
    for line in logdata:
        words = string.split(line)
        flist.append(words[12])
        line_spectra = words[14]
    line_scan = len(logdata)
    flist.sort()
    fmodelist = []
    for i in range(len(flist)):
        f = flist.pop()
        if f == '\\N':
            continue
        if f not in fmodelist:
            fmodelist.append(f)
    freqmode = ",".join(fmodelist)
    if freqmode == '\\N':
        freqmode = "0"
    if freqmode == '':
        freqmode = "0"

# ------------------------------------------------------------------------------
#       otherwise, blank some parameters and retrieve processed
#       keyword from .TXT file instead
# ------------------------------------------------------------------------------

else:
    freqmode = "0"
    line_scan = 0
    line_spectra = "0"
    if backend == 'AC1':
        file_old = "/OdinData/AERO/AC1/" + \
            hex_orbit[0:2] + "/OB1B" + hex_orbit + ".TXT"
        file_new = "/OdinData/AERO/7.0/AC1/" + \
            hex_orbit[0:2] + "/OB1B" + hex_orbit + ".TXT"
    elif backend == 'AC2':
        file_old = "/OdinData/AERO/AC2/" + \
            hex_orbit[0:2] + "/OC1B" + hex_orbit + ".TXT"
        file_new = "/OdinData/AERO/7.0/AC2/" + \
            hex_orbit[0:2] + "/OC1B" + hex_orbit + ".TXT"
    else:
        file_old = "/OdinData/AERO/FBA/" + \
            hex_orbit[0:2] + "/OD1B" + hex_orbit + ".TXT"
        file_new = "/OdinData/AERO/7.0/FBA/" + \
            hex_orbit[0:2] + "/OD1B" + hex_orbit + ".TXT"

    if os.path.exists(file_new):
        buf = os.stat(file_new)
        created = time.strftime("%Y-%m-%d %H:%M:%S",
                                time.localtime(buf[stat.ST_CTIME]))
    else:
        if os.path.exists(file_old):
            buf = os.stat(file_old)
            created = time.strftime(
                "%Y-%m-%d %H:%M:%S", time.localtime(buf[stat.ST_CTIME]))
        else:
            created = "2000-01-01 00:00:00"

# ------------------------------------------------------------------------------
#       debug print
# ------------------------------------------------------------------------------

# print "---------------------------------------"
# print "orbit:      ",orbit
# print "backend:    ",backend
# print "logexist:   ",logexist
# print "freqmode:   ",freqmode
# print "nscans:     ",line_scan
# print "nspectra:   ",line_spectra
# print "attversion: ",attversion
# print "calversion: ",calversion
# print "processed:  ",created
# print "start_utc:  ",start_utc
# print "stop_utc:   ",stop_utc
# print "---------------------------------------"

# ------------------------------------------------------------------------------
#       save to local 'level1' table.
#       if row with same orbit/backend/calversion already exist
#       update row, otherwise, insert new row
# ------------------------------------------------------------------------------

db = pg.connect('smr', 'ymer', user='smr')
spectra = int(line_spectra)
cal = string.atof(calversion)

query_select = "SELECT id FROM " + table2 + " WHERE orbit = '" + \
    sys.argv[1] + "' AND backend = '" + sys.argv[2]
query_select = query_select + "' AND calversion = '" + str(calversion) + "'"
print "LOCAL==> " + query_select
q = db.query(query_select)
rows = q.getresult()

if not rows:
    # print sys.argv[0]+" inserting orbit="+sys.argv[1]+" and backend="+sys.argv[2]+" into local table "+table2
    query = "INSERT INTO " + table2
    query = query + \
        " (orbit,backend,freqmode,nscans,nspectra,calversion,attversion,processed,start_utc,stop_utc) VALUES "
    query = query + " ('%d','%s','%s','%d','%d','%d','%s','%s','%s','%s'); " % (orbit, backend,
                                                                                freqmode, line_scan, spectra, calversion, attversion, created, start_utc, stop_utc)
else:
    id = rows[0]
    # print sys.argv[0]+" updating local table "+table2 +" with orbit="+sys.argv[1]+" and backend="+sys.argv[2]
    query = "UPDATE " + table2
    query = query + " SET %s = \'%s\'" % ("freqmode", freqmode)
    query = query + ", %s = '%s'" % ("nscans", line_scan)
    query = query + ", %s = '%s'" % ("nspectra", spectra)
    query = query + ", %s = '%s'" % ("calversion", cal)
    query = query + ", %s = '%s'" % ("attversion", attversion)
    query = query + ", %s = '%s'" % ("processed", created)
    query = query + ", %s = '%s'" % ("start_utc", start_utc)
    query = query + ", %s = '%s'" % ("stop_utc", stop_utc)
    query = query + " WHERE id = %d" % (id)

print "LOCAL==> " + query
q = db.query(query)

if not rows:
    # read newly created id to be used with the "status" table later
    query_select = "SELECT id FROM " + table2 + " WHERE orbit = '" + \
        sys.argv[1] + "' AND backend = '" + sys.argv[2]
    query_select = query_select + \
        "' AND calversion = '" + str(calversion) + "'"
    print "LOCAL==> " + query_select
    q = db.query(query_select)
    rows = q.getresult()
    id = rows[0]

# ------------------------------------------------------------------------------
#       save to local 'status' table. Retrieve 'id' from 'level1'
#       if 'id' entry doesn't exist: insert row. If 'id' entry exist: update row.
#       if logfile does not exist, call statusmess to update errormessage(s),
#       if logfile does exist, clear errormessage(s)
# ------------------------------------------------------------------------------

cid = str(id[0])
cstat = str(logexist)

query_select = "SELECT id FROM " + table3 + " WHERE id = " + cid
print "LOCAL==> " + query_select
q = db.query(query_select)
rows = q.getresult()

if not rows:
    # print sys.argv[0]+" inserting id="+cid+" and status="+cstat+" into local table "+table3
    query = "INSERT INTO " + table3
    query = query + " (id,status) VALUES ('%d','%d'); " % (id[0], logexist)
else:
    # print sys.argv[0]+" updating local table "+table3 +" with id="+cid+" and status="+cstat
    query = "UPDATE " + table3
    query = query + " SET %s = '%d'" % ("status", logexist)
    query = query + " WHERE id = %d" % (id[0])

print "LOCAL==> " + query
q = db.query(query)

time.sleep(1)

if (logexist == 0):
    print "LOCAL==> updating status table message(s)"
    cmd = "python statusmess.py " + backend + " " + str(orbit) + " local"
    print cmd
    os.system(cmd)
else:
    print "LOCAL==> clearing status table message(s)"
    query = "UPDATE " + table3 + " SET %s = '%s'" % ("errmsg", " ")
    query = query + " WHERE id = %d" % (id[0])
    print "LOCAL==> " + query
    q = db.query(query)

db.close()

# ------------------------------------------------------------------------------
#       save to remote 'level1' table.
#       check to see if entry exist.
#       if not insert new row, else update row
# ------------------------------------------------------------------------------

try:
    conn = MySQLdb.connect(
        host=remote_host,
        user=remote_user,
        passwd=remote_pw,
        db=remote_db)
except MySQLdb.Error as e:
    print "Error %d: %s" % (e.args[0], e.args[1])
    sys.exit(1)

cursor = conn.cursor()
query_select = "SELECT id FROM " + table2 + " WHERE orbit = '" + \
    sys.argv[1] + "' AND backend = '" + sys.argv[2]
query_select = query_select + "' AND calversion = '" + str(calversion) + "'"
print "REMOTE==> " + query_select
cursor.execute(query_select)
rows = cursor.fetchone()
if not rows:
    # print sys.argv[0]+" inserting orbit="+sys.argv[1]+" and backend="+sys.argv[2]+" into remote table "+table2
    query = "INSERT INTO " + table2
    query = query + \
        " (orbit,backend,freqmode,nscans,nspectra,calversion,attversion,processed,start_utc,stop_utc) VALUES "
    query = query + " ('%d','%s','%s','%d','%d','%d','%s','%s','%s','%s'); " % (orbit,
                                                                                backend, freqmode, line_scan, spectra, cal, attversion, created, start_utc, stop_utc)
    buf = "inserting"
else:
    id = rows[0]
    # print sys.argv[0]+" updating remote table "+table2 +" with orbit="+sys.argv[1]+" and backend="+sys.argv[2]
    query = "UPDATE " + table2
    query = query + " SET %s = '%s'" % ("freqmode", freqmode)
    query = query + ", %s = '%s'" % ("nscans", line_scan)
    query = query + ", %s = '%s'" % ("nspectra", spectra)
    query = query + ", %s = '%s'" % ("calversion", cal)
    query = query + ", %s = '%s'" % ("attversion", attversion)
    query = query + ", %s = '%s'" % ("processed", created)
    query = query + ", %s = '%s'" % ("start_utc", start_utc)
    query = query + ", %s = '%s'" % ("stop_utc", stop_utc)
    query = query + " WHERE id = %d" % (id)
    buf = "updating"
print "REMOTE==> " + query
cursor.execute(query)

if (buf == 'inserting'):
    # read newly created id to be used with the "status" table later
    query_select = "SELECT id FROM " + table2 + " WHERE orbit = '" + \
        sys.argv[1] + "' AND backend = '" + sys.argv[2]
    query_select = query_select + \
        "' AND calversion = '" + str(calversion) + "'"
    print "REMOTE==> " + query_select
    cursor.execute(query_select)
    rows = cursor.fetchone()
    id = rows[0]

# ------------------------------------------------------------------------------
#       save to remote 'status' table. Retrieve 'id' from 'level1'
#       check to see if entry exist.
#       if entry doesn't exist: insert row. If entry exist: update row.
#       If log file does not exist, update 'status' table with 'error' message
# ------------------------------------------------------------------------------

cid = str(id)
cstat = str(logexist)

query_select = "SELECT id FROM " + table3 + " WHERE id = " + cid
print "REMOTE==> " + query_select
cursor.execute(query_select)
rows = cursor.fetchone()

if not rows:
    # print sys.argv[0]+" inserting id="+cid+" and status="+cstat+" into remote table "+table3
    query = "INSERT INTO " + table3
    query = query + " (id,status) VALUES ('%d','%d'); " % (id, logexist)
else:
    # print sys.argv[0]+" updating remote table "+table3 +" with id="+cid+" and status="+cstat
    query = "UPDATE " + table3
    query = query + " SET %s = '%d'" % ("status", logexist)
    query = query + " WHERE id = %d" % (id)

print "REMOTE==> " + query
cursor.execute(query)

time.sleep(1)

if (logexist == 0):
    print "REMOTE==> updating status table message(s)"
    cmd = "python statusmess.py " + backend + " " + str(orbit) + " remote"
    os.system(cmd)
else:
    print "REMOTE==> clearing status table message(s)"
    query = "UPDATE " + table3 + " SET %s = '%s'" % ("errmsg", " ")
    query = query + " WHERE id = %d" % (id)
    print "REMOTE==> " + query
    cursor.execute(query)

cursor.close()
