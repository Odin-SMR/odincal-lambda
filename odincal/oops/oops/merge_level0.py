#!/usr/bin/python
#
#    merge_level0.py
#
#    merge lines in planned_raw / level0_raw tables with respect to setup
#    The purpose is to merge setups W3A/W3B into 1 line and W5A/W5B into 1 line
#    and insert rows into tables planned / level0.
#    Tables planned / level0 are emptied to start with.
#
#    Glenn Persson, 2008-02-13
#
#
import sys
import MySQLdb
import pg

from jet import *

table_local_in = "planned_raw"
table_local_ut = "planned"
table_local_seq = "planned_id_seq"
table_remote_in = "level0_raw"
table_remote_ut = "level0"

first = "yes"
old_start_orbit = ""
old_stop_orbit = ""
old_start_utc = ""
old_stop_utc = ""
old_vdop = -7
old_mode = ""
old_setup = ""
ix = 0

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

# empty destination tables

db = pg.connect("smr", "ymer", user="smr")

sql = "DELETE FROM " + table_local_ut
print sql
q = db.query(sql)
sql = "ALTER SEQUENCE " + table_local_seq + " RESTART WITH 1"
print sql
q = db.query(sql)

query = "DELETE FROM " + table_remote_ut
print query
cursor.execute(query)
query = "ALTER TABLE " + table_remote_ut + " AUTO_INCREMENT=1"
print query
cursor.execute(query)

#  read local source table and start the loop

sql = "SELECT * FROM " + table_local_in + " ORDER BY start_orbit"
print sql
q = db.query(sql)
rows = q.getresult()
for row in q.getresult():
    ix = ix + 1
    #id = row[0]
    start_orbit = row[1]
    stop_orbit = row[2]
    start_utc = row[3]
    stop_utc = row[4]
    vdop = row[5]
    mode = row[6]
    setup = row[7]
    if ((setup == "W3A") or (setup == "W4A")):
        setup = "W3A,W4A"
    if ((setup == "W5A") or (setup == "W5B")):
        setup = "W5A,W5B"
    if ((old_setup != setup) or ((setup != "W3A,W4A") and (setup != "W5A,W5B"))):
        if (first == "yes"):
            first = "no"
            old_start_orbit = start_orbit
            old_stop_orbit = stop_orbit
            old_start_utc = start_utc
            old_stop_utc = stop_utc
            old_vdop = vdop
            old_mode = mode
            old_setup = setup
            #print "ix: ",ix
            continue
        # new setup => create entry
        ##print old_start_orbit,old_stop_orbit,old_start_utc,old_stop_utc,old_vdop,old_mode,old_setup

        #  insert row into local table
        query = "INSERT INTO " + table_local_ut + \
            " (start_orbit,stop_orbit,start_utc,stop_utc,vdop,mode,setup) "
        query = query + " VALUES (%10.2f,%10.2f,'%s','%s',%.1f,'%s','%s'); " % (
            old_start_orbit, old_stop_orbit, old_start_utc, old_stop_utc, float(old_vdop), old_mode, old_setup)
        print query
        q = db.query(query)

        #  insert row into remote table
        query = "INSERT INTO " + table_remote_ut + \
            " (start_orbit,stop_orbit,start_utc,stop_utc,vdop,mode,setup) "
        query = query + " VALUES (%10.2f,%10.2f,'%s','%s',%.1f,'%s','%s'); " % (
            old_start_orbit, old_stop_orbit, old_start_utc, old_stop_utc, float(old_vdop), old_mode, old_setup)
        print query
        cursor.execute(query)

        #  set old-values = new values
        old_start_orbit = start_orbit
        old_start_utc = start_utc
        old_stop_orbit = stop_orbit
        old_stop_utc = stop_utc
        old_vdop = vdop
        old_mode = mode
        old_setup = setup
        #print "ix: ",ix
    else:
        old_stop_orbit = stop_orbit
        old_stop_utc = stop_utc
        old_vdop = vdop
        old_mode = mode
        #print "ix: ",ix


#  insert final row into local table and close database
query = "INSERT INTO " + table_local_ut + \
    " (start_orbit,stop_orbit,start_utc,stop_utc,vdop,mode,setup) "
query = query + " VALUES (%10.2f,%10.2f,'%s','%s',%.1f,'%s','%s'); " % (old_start_orbit,
                                                                        old_stop_orbit, old_start_utc, old_stop_utc, float(old_vdop), old_mode, old_setup)
print query
q = db.query(query)
db.close()

#  insert final row into remote table and close database
query = "INSERT INTO " + table_remote_ut + \
    " (start_orbit,stop_orbit,start_utc,stop_utc,vdop,mode,setup) "
query = query + " VALUES (%10.2f,%10.2f,'%s','%s',%.1f,'%s','%s'); " % (old_start_orbit,
                                                                        old_stop_orbit, old_start_utc, old_stop_utc, float(old_vdop), old_mode, old_setup)
print query
cursor.execute(query)
cursor.close()
