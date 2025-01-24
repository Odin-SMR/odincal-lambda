from datetime import date

import pytest

from ..handler.check_era5_handler import NoERA5DataError,assert_era5_exists


def test_handler_ok():
    dates = [date(2024,9,18),date(2024,9,14)]
    assert_era5_exists(dates)

def test_handler_fails():
    dates = [date(1976,9,18),date(2024,9,14)]
    with pytest.raises(NoERA5DataError):
        assert_era5_exists(dates)