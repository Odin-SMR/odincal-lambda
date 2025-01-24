from datetime import date
import pytest

from create_zpt.handler.era5_dataset import get_dataset


def test_read_era5_ok():
    dates = [date(2024, 9, 18), date(2024, 9, 14)]
    ds = get_dataset(dates)
    assert "time" in ds.coords
    assert ds.t.shape[0] == 8, "Should be 2 days * 4 times"
    ds.close()


def test_read_era5_fail():
    dates = [date(1976, 9, 18), date(2024, 9, 14)]
    with pytest.raises(FileNotFoundError):
        ds = get_dataset(dates)
