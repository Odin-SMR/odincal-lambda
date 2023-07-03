import os
from odincal.level0_file_importer import import_file

basedir = '/misc/pearl/odin/level0/'
level0 = ['ac1', 'ac2', 'att', 'fba', 'shk']
catalogues = ['1c9', '1ca', '1cb', '1cc', '1cd', '1ce', '1cf', '1d0', '1d1']


for level0type in level0:
    for calalogue in catalogues:
        datadir = os.path.join(basedir, level0type, calalogue)
        if os.path.isdir(datadir):
            pass
        else:
            continue
        datafiles = os.listdir(datadir)
        for datafile in datafiles:
            print datafile
            import_file(datafile)
