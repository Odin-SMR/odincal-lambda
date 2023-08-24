import os
import os.path
from sys import argv


def download_level0():
    for typ in ['ac1', 'ac2', 'shk', 'fba', 'att']:
        if not os.path.exists(typ + '/' + argv[0]):
            os.system('mkdir -p ./{0}/{1}'.format(typ, argv[1]))
        os.system(
            "scp  -o 'GSSAPIKeyExchange yes' donal@esrange.pdc.kth.se:/data/odin/level0/{0}/{1}/* {0}/{1}".format(typ, argv[1]))
