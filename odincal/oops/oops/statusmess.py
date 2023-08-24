#!/usr/bin/python
#
#       statusmess.py  <backend> <orbit> <mode>
#
#       where   backend = AC1, AC2, FBA
#               orbit   = decimal orbit
#               mode    = update, local, remote, verify
#               where   'update' updates both local and remote database tables
#                       'local' updates only local database table
#                       'remote' updates only remote database table
#                       'verify' does not update databases
#
#       purpose: update database status tables with WARN messages retrieved
#       from processed (level0->level1b) TXT files
#
#       Normally automatically scheduled during 0->1b processing, but
#       can also be run separately.
#
#       Glenn Persson, Jan. 2008
#
#       modified:
#       2008-02-05      new path due to 6.0 prefix
#       2008-08-13      added handling of FBA data
#       2008-08-18      bug fix
#       2008-09-24      prepare path for v7.0
#
#
import MySQLdb
import os
import pg
import re
import string
import sys

from jet import *

path_old = '/OdinData/AERO/'
path_new = '/OdinData/AERO/7.0/'
table1 = 'level1'
table2 = 'status'

#  ---------------------------------------------------------
#  read arguments and do syntax check
#  ---------------------------------------------------------
if len(sys.argv) != 4:
    print "Use: $ python " + \
        sys.argv[0] + \
        " <AC1|AC2|FBA> <decimal orbit> <verify|update|local|remote>"
    sys.exit()
backend = sys.argv[1]
orbit = sys.argv[2]
mode = sys.argv[3]
if (((backend != 'AC1') & (backend != 'AC2') & (backend != 'FBA')) | (
        (mode != 'verify') & (mode != 'update') & (mode != 'local') &
        (mode != 'remote'))):
    print "Use: $ python statusmess.py <AC1|AC2|FBA> <decimal orbit> <verify|update|local|remote>"
    sys.exit()

#  ---------------------------------------------------------
#  create file name and verify that file exists.
#  (try both old path and new 7.0 path
#  If file doesn't exist, exit
#  ---------------------------------------------------------
number = int(orbit)
hex_buf = string.upper(hex(number))[2:]
hex_string = hex_buf
while (len(hex_string) < 4):
    hex_string = "0" + hex_string
subdir = hex_string[0:2]
if (backend == 'AC1'):
    prefix = 'OB1B'
elif (backend == 'AC2'):
    prefix = 'OC1B'
else:
    prefix = 'OD1B'

file = path_old + backend + '/' + subdir + '/' + prefix + hex_string + '.TXT'
if not os.path.exists(file):
    file = path_new + backend + '/' + subdir + '/' + prefix + hex_string + '.TXT'
    if not os.path.exists(file):
        print "File does not exist!", file
        sys.exit()

#  ---------------------------------------------------------
#  open file and search for all lines containing 'WARN'
#  save into array 'raw'
#  ---------------------------------------------------------
i = 0
j = 0
logfile = open(file, 'r')
raw = []
data = logfile.readlines()
for line in data:
    i = i + 1
    line = string.strip(line)
    if re.search('WARN', line):
        mess = line[26:]
        raw.append(mess + '\n')
        j = j + 1
logfile.close
if (mode == 'verify'):
    print 'logfile: ', file
    print i, " lines in ", file
    print j, " WARN lines"

#  ---------------------------------------------------------
#  sort rawfile
#  ---------------------------------------------------------
raw.sort()
# print raw

#  ---------------------------------------------------------
#  remove duplicates from sorted array
#  join all messages into new array separated with newline
#  ---------------------------------------------------------
i = 0
j = 0
msg = []
for line in raw:
    line = string.strip(line)
    # print "sorted rawline:",line
    if (i == 0):
        oldline = string.upper(line)
    if (line[:10] != oldline[:10]):
        msg.append(line)
        j = j + 1
        # print "lines differ. i=",i," j=",j
        if (mode == 'verify'):
            print "message->", line
    oldline = line
    i = i + 1

msgstring = string.join(msg, "\n")

#  ---------------------------------------------------------
#  retrieve 'id' for this orbit/backend from local table
#  ---------------------------------------------------------
if ((mode == 'update') | (mode == 'local')):
    db = pg.connect('smr', 'ymer', user='smr')
    query = "SELECT id FROM " + table1 + " WHERE orbit='" + \
        orbit + "' AND backend='" + backend + "'"
    q = db.query(query)
    rows = q.getresult()
    id = rows[0]
    cid = str(id[0])
    # print "local query: ",query
    # print "local id: ",cid
    #  ---------------------------------------------------------
    #  update local database table
    #  ---------------------------------------------------------
    query = "UPDATE " + table2 + " SET errmsg = '" + msgstring + "' WHERE id = " + cid
    # print "local query: ",query
    q = db.query(query)
    db.close()

#  ---------------------------------------------------------
#  retrieve 'id' for this orbit/backend from remote table
#  ---------------------------------------------------------
if ((mode == 'update') | (mode == 'remote')):
    try:
        conn = MySQLdb.connect(host=remote_host, user=remote_user,
                               passwd=remote_pw, db=remote_db)
    except MySQLdb.Error as e:
        print "Error %d: %s" % (e.args[0], e.args[1])
        sys.exit(1)
    cursor = conn.cursor()
    query = "SELECT id FROM " + table1 + " WHERE orbit='" + \
        orbit + "' AND backend='" + backend + "'"
    cursor.execute(query)
    rows = cursor.fetchone()
    id = rows[0]
    # print "remote query: ",query
    # print "remote id: ",id
    #  ---------------------------------------------------------
    #  update remote database table
    #  ---------------------------------------------------------
    query = "UPDATE " + table2 + " SET errmsg = '" + \
        msgstring + "' WHERE id = " + str(id)
    # print "remote query: ",query
    cursor.execute(query)
    cursor.close()
