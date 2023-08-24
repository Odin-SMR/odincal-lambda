"""Connections to the database"""
from pg import DB  # pylint: disable=import-error
from sqlalchemy.ext.declarative import declarative_base
from odincal.config import config


class OdincalDB(object):  # pylint: disable=too-few-public-methods
    """Sqlalchemy settings"""
    Base = declarative_base()
    connect_string = config.get('database', 'connect_string')


class ConfiguredDatabase(DB):  # pylint: disable=too-few-public-methods
    """the odincal database"""

    def __init__(self):
        DB.__init__(
            self,
            dbname=config.get('database', 'dbname'),
            user=config.get('database', 'user'),
            host=config.get('database', 'host'),
            passwd=config.get('database', 'passwd'),
        )
