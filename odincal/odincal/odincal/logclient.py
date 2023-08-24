from ConfigParser import SafeConfigParser
from os.path import expanduser
from pkg_resources import resource_filename
import logging.config
from tempfile import NamedTemporaryFile


def set_odin_logging():
    parser = SafeConfigParser()
    used_files = parser.read([
        resource_filename('odincal', 'logclient.cfg'),
        expanduser('~/.odin/logclient.cfg'),
    ])
    # create a temporary file on disk
    config_file = NamedTemporaryFile()
    parser.write(config_file)
    config_file.file.flush()
    # let the logger read the temporary file
    logging.config.fileConfig(config_file.name)
