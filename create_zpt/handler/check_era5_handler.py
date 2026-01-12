"""Check if data is available in the ERA5 DB for creating ZPT for the file"""

import datetime as dt
from typing import Any

from .era5_dataset import read_zarr_dataset
from .time_util import mjd2datetime

# logconfig()


class NoERA5DataError(Exception):
    pass


def assert_era5_exists(
    dates: list[dt.date],
) -> None:
    try:
        ds = read_zarr_dataset(dates)
    except FileNotFoundError as err:
        raise NoERA5DataError(f"No ERA5 data found for {dates} ({err})")
    else:
        ds.close()


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    dates: set[dt.date] = set()
    for fm in event["ScansInfo"]:
        dates = dates.union(
            {mjd2datetime(scan["MJDStart"]).date() for scan in fm["ScansInfo"]}
        ).union({mjd2datetime(scan["MJDEnd"]).date() for scan in fm["ScansInfo"]})

    assert_era5_exists(list(dates))

    return {"StatusCode": 200}
