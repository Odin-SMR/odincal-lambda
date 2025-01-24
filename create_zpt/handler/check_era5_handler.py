"""Check if data is available in the ERA5 DB for creating ZPT for the file
"""

import datetime as dt
from typing import Any

import boto3
from botocore.exceptions import ClientError
from dask import delayed
from xarray import concat, open_zarr

from .era5_dataset import ERA5_BUCKET, ERA5_PATTERN
from .log_configuration import logconfig
from .time_util import mjd2datetime

# logconfig()


class NoERA5DataError(Exception):
    pass


@delayed
def read_dataset(file: str):
    ds = open_zarr(
        file,
        consolidated=True,
    )
    # there is a breakpoint 2024-09-18
    # - Slightly different data format
    # - Zarr3
    if "expver" in ds.coords:
        ds = ds.drop_vars("expver")
    if "number" in ds.coords:
        ds = ds.drop_vars("number")
    return ds


def assert_era5_exists(
    dates: list[dt.date],
) -> None:
    files = [
        f"s3://{ERA5_BUCKET}/{ERA5_PATTERN.format(year=date.year, month=date.month, date=date.isoformat())}"
        for date in dates
    ]
    tasks = [read_dataset(f) for f in files]
    ds_combined = delayed(concat)(tasks, dim="time")
    try:
        # Fetch only the coordinates
        ds_combined.compute()
    except FileNotFoundError as err:
        raise NoERA5DataError(f"No ERA5 data found for {dates} ({err})")


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    dates: set[dt.date] = set()
    for fm in event["ScansInfo"]:
        dates = dates.union(
            {mjd2datetime(scan["MJDStart"]).date() for scan in fm["ScansInfo"]}
        ).union({mjd2datetime(scan["MJDEnd"]).date() for scan in fm["ScansInfo"]})

    assert_era5_exists(list(dates))

    return {"StatusCode": 200}
