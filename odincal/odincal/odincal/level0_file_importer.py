"""The database model to register a file in the database"""
from datetime import datetime, timedelta
from os.path import basename, splitext

from sqlalchemy import func, Column, String, Date, DateTime
from odincal.database import OdincalDB


STW_ODIN_REBOOT = 3 * 2 ** 32


class Level0File(OdincalDB.Base):
    """A class to register level0-files"""
    __tablename__ = 'level0_files'

    file = Column(String, primary_key=True)
    measurement_date = Column(Date)
    created = Column(DateTime, default=func.now())

    def __init__(self, filename):
        self.file = basename(filename)
        self.name, self.extension = splitext(self.file)
        self.check_file_type()
        self.measurement_date = self.timestamp_from_filename()
        self.created = datetime.utcnow()

    def timestamp_from_filename(self):
        """ return datetime from filename """
        stw = int(self.name + '0', base=16)
        if stw >= STW_ODIN_REBOOT:
            a = 4.99205631 * 1e4
            b = 7.23413455 * 1e-7
            c = -4.25743315 * 1e-21
            d = 0.
        else:
            a = 5.19601792 * 1e4
            b = 7.23308989 * 1e-7
            c = -6.81289563 * 1e-22
            d = 2.47714080 * 1e-32
        mjd = (a + b * stw + c * stw ** 2 + d * stw ** 3)
        return datetime.date(datetime(1858, 11, 17) + timedelta(days=mjd))

    def check_file_type(self):
        """ import one file """
        if self.extension not in ['.ac1', '.ac2', '.shk', '.att', '.fba']:
            raise ValueError
