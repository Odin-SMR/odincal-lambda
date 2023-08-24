from datetime import date 
import pytest

from odincal.level0_file_importer import Level0File


@pytest.mark.parametrize("filename,expect", (
    ("/path/to/file/00da5210.att", date(2001, 8, 4)),
    ("/path/to/file/06eaba07.att", date(2004, 10, 25)),
    ("/path/to/file/07505920.att", date(2005, 1, 10)),
    ("/path/to/file/0b105c8a.att", date(2007, 1, 8)),
    ("/path/to/file/1001220d.att", date(2009, 8, 24)),
    ("/path/to/file/10235ce2.att", date(2009, 9, 19)),
    ("/path/to/file/15026f16.att", date(2012, 4, 22)),
    ("/path/to/file/1a5e4dff.att", date(2015, 2, 26)),
    ("/path/to/file/1a5eeb29.att", date(2015, 2, 27)),
    ("/path/to/file/20002dd0.att", date(2018, 2, 24)),
    ("/path/to/file/25726391.att", date(2021, 1, 16)),
    ("/path/to/file/30085ce9.att", date(2021, 2, 2)),
    ("/path/to/file/3074223e.att", date(2021, 4, 25)),
))
def test_timestamp(filename, expect):
    """Check STW to date approximation"""
    test_file = Level0File(filename)
    assert test_file.measurement_date == expect


def test_file_imported():
    """Check that files are imported"""
    filename = '1a5ed04b.fba'
    test_file = Level0File(filename)
    assert test_file.extension == '.fba'


def test_file_not_imported():
    """Check that 'other' files are not imported"""
    filename = 'other'
    try:
        Level0File(filename)
    except ValueError:
        assert True
    else:
        assert False
