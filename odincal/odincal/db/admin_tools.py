import os
import pkg_resources
import StringIO


def create_datamodel():
    model = pkg_resources.resource_filename(__name__, 'create.sql')
    os.system('psql -f {}'.format(model))
