from ConfigParser import SafeConfigParser
from pkg_resources import resource_stream
from os.path import expanduser

config = SafeConfigParser()
defaults = resource_stream(__name__, 'defaults.cfg')
config.readfp(defaults)
config.read([
    expanduser('~/.odincal.cfg'),
    expanduser('~/.odincal.secret')
])
