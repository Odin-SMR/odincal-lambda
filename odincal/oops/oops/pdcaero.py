#!/usr/bin/python
#
#       pdcaero.py
#
#       modified on 2008-07-01 by Glenn Persson
#       added '7.0/' as prefix to source path on PDC
#
import os
import sys
import string

files = sys.stdin.readlines()
olddir = ""
if len(files) > 0:
    output = open("/tmp/hdfscript", "w")

    output.write('#!/usr/bin/expect --\n')
    output.write('set timeout -1\n')
    output.write('spawn /usr/local/Krb5-PDC/ftp esrange.pdc.kth.se\n')
    output.write('expect "Name*:"\n')
    output.write('send "glennp\\r"\n')
    output.write('expect "ftp>"\n')
    output.write('send "cd /hsm/projects/esrange/odin/level1b\\r"\n')
    output.write('expect "ftp>"\n')
    output.write('send "cd aero/submm\\r"\n')
    output.write('expect "ftp>"\n')
    output.write('send "bin\\r"\n')
    output.write('expect "ftp>"\n')

    for file in files:
        local = file[:-1]
        path = os.path.split(local)
        file = path[-1]
        if file[1] == 'A':
            backend = "AOS"
        elif file[1] == 'B':
            backend = "AC1"
        elif file[1] == 'C':
            backend = "AC2"
        elif file[1] == 'D':
            backend = "FBA"
        remotedir = string.join((backend, file[4:6]), '/')
        if olddir != remotedir:
            cmd = "mkdir %s" % (remotedir)
            cmd = 'send "' + cmd + '\\r"'
            output.write(cmd + '\n')
            output.write('expect "ftp>"\n')
            cmd = "chmod 755 %s" % (remotedir)
            cmd = 'send "' + cmd + '\\r"'
            output.write(cmd + '\n')
            output.write('expect "ftp>"\n')
            olddir = remotedir

        remote = '7.0/' + string.join((backend, file[4:6], file), '/')

        cmd = "put %s %s" % (local, remote)
        cmd = 'send "' + cmd + '\\r"'
        output.write(cmd + '\n')
        output.write('expect "ftp>"\n')

        cmd = "chmod 644 %s" % (remote)
        cmd = 'send "' + cmd + '\\r"'
        output.write(cmd + '\n')
        output.write('expect "ftp>"\n')

    output.write('send "quit\\r"\n')
    output.write('expect "Goodbye"\n')
    output.write('close\n')
    output.write('wait\n')
    output.close()
