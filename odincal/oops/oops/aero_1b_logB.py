#!/usr/bin/python
#
#       $ python aero_1b_logB.py <orbit> <AC1|AC2|FBA>
#
#       Purpose:    update local and remote sql tables with parameters
#                   'uploaded', 'filename', 'logname' read from /tmp/hdffiles
#                   If this file doesn't exist, the script exits.
#                   If no HDF.gz file is found, these parameters
#                   will take the following values:
#                   uploaded = "2000-01-01 00:00:00"
#                   filename = " ", logname = " "
#
#                   The script is scheduled from aeronomy processing batchfile
#                   after the aeronomy orbits have been uploaded to PDC
#
#       Uses:           table 'level1' (local @ ymer.oso.chalmers.se)
#                             'level1' (remote @ jet.rss.chalmers.se)
#
#
#       Glenn Persson, 2006-11-01
#
#       Modifications:
#
#       2008-01-10              bug fix
#       2008-01-16              prefix '6.0/' to filename and logname
#       2008-07-09              prefix 6.0 changed to 7.0
#       2008-08-13              handling of FBA data
#       2008-09-26              fixed bug in query statement

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

from jet import *

file_name = '/tmp/hdffiles'
table = "level1"
calversion = 7.0

if len(sys.argv) < 3:
    print "usage: %s orbit <AC1|AC2|FBA>" % (sys.argv[0])
    sys.exit()

if len(sys.argv) > 2:
    orbit = int(sys.argv[1])
    backend = sys.argv[2]

if (backend != 'AC1') and (backend != 'AC2') and (backend != 'FBA'):
    print sys.argv[0] + " ERROR! -> not valid backend!"
    sys.exit()

# ----------------------------------------------------------------------------
#       check existence of LOGfile. If it doesn't exist, exit
# ----------------------------------------------------------------------------

try:
    log = open(file_name, "r")
except IOError:
    print sys.argv[0] + " Oops! Open error on " + \
        file_name + " ===> No logging."
    sys.exit()

# ----------------------------------------------------------------------------
#       get filestamp of upload script
# ----------------------------------------------------------------------------

file_stats = os.stat(file_name)
uploaded = time.strftime("%Y-%m-%d %H:%M:%S",
                         time.localtime(file_stats[stat.ST_CTIME]))

# ----------------------------------------------------------------------------
#       get filename and logname
# ----------------------------------------------------------------------------

n = 0
log = open(file_name, "r")
logdata = log.readlines()
for line in logdata:
    line = string.strip(line)
    if backend == 'AC1':
        if re.search('/AERO/7.0/AC1', line):
            if re.search('HDF.gz', line):
                buf = line
                n = string.find(buf, '7.0/AC1')
                filename = buf[n:]
            elif re.search('LOG', line):
                buf = line
                n = string.find(buf, '7.0/AC1')
                logname = buf[n:]
    elif backend == 'AC2':
        if re.search('/AERO/7.0/AC2', line):
            if re.search('HDF.gz', line):
                buf = line
                n = string.find(buf, '7.0/AC2')
                filename = buf[n:]
            elif re.search('LOG', line):
                buf = line
                n = string.find(buf, '7.0/AC2')
                logname = buf[n:]
    else:
        if re.search('/AERO/7.0/FBA', line):
            if re.search('HDF.gz', line):
                buf = line
                n = string.find(buf, '7.0/FBA')
                filename = buf[n:]
            elif re.search('LOG', line):
                buf = line
                n = string.find(buf, '7.0/FBA')
                logname = buf[n:]

log.close()

# ----------------------------------------------------------------------------
#       debug print
# ----------------------------------------------------------------------------
# print "n:        ",n
# print "orbit:       ",orbit
# print "backend:      ",backend
# print "uploaded:     ",uploaded
# if (n > 0):
#       print "filename:     ",filename
#       print "logname:      ",logname

# ----------------------------------------------------------------------------
#       check that entry exist in local SQL table
#       if not, exit
#       otherwise, update entry
# ----------------------------------------------------------------------------

db = pg.connect('smr', 'ymer', user='smr')
query_select = "SELECT id FROM " + table + " WHERE orbit=('%d') " % orbit
if backend == 'AC1':
    query_select = query_select + " AND backend='AC1'"
elif backend == 'AC2':
    query_select = query_select + " AND backend='AC2'"
else:
    query_select = query_select + " AND backend='FBA'"
query_select = query_select + " AND calversion = '" + str(calversion) + "'"
print query_select
q = db.query(query_select)
for row in q.getresult():
    id = row[0]
if q <= 0:
    print "-------> orbit does not exist in local table!"

query = "UPDATE " + table + " SET "
if (n == 0):
    query = query + " uploaded=('2000-01-01 00:00:00'),"
    query = query + " filename=(' '),"
    query = query + " logname=(' ')"
else:
    query = query + " uploaded=('%s'), " % uploaded
    query = query + " filename=('%s'), " % filename
    query = query + " logname=('%s') " % logname
query = query + " WHERE id=('%s') " % id
print sys.argv[0] + " updating local table " + table + \
    " for orbit " + sys.argv[1] + " and backend " + sys.argv[2]
print query
q = db.query(query)
db.close()

# ----------------------------------------------------------------------------
#       check that entry exist in remote SQL table
#       if not, exit
#       otherwise, update entry
# ----------------------------------------------------------------------------

try:
    conn = MySQLdb.connect(
        host=remote_host,
        user=remote_user,
        passwd=remote_pw,
        db=remote_db)
except MySQLdb.Error as e:
    print "Error %d: %s" % (e.args[0], e.args[1])
    sys.exit()

cursor = conn.cursor()
# print query_select
cursor.execute(query_select)
rows = cursor.fetchone()
if not rows:
    print "-------> orbit does not exist in remote table!"
else:
    id = rows[0]

query = "UPDATE " + table + " SET "
if (n == 0):
    query = query + " uploaded=('2000-01-01 00:00:00'),"
    query = query + " filename=(' '),"
    query = query + " logname=(' ')"
else:
    query = query + " uploaded=('%s'), " % uploaded
    query = query + " filename=('%s'), " % filename
    query = query + " logname=('%s') " % logname
query = query + " WHERE id=('%s') " % id
print sys.argv[0] + " updating remote table " + table + \
    " for orbit " + sys.argv[1] + " and backend " + sys.argv[2]
print query
cursor.execute(query)
cursor.close()
