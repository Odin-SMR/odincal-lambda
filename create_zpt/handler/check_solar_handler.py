"""Check if data is available in the Solar DB for creating ZPT for the file
"""
import datetime as dt
from typing import Any

from pandas import Timestamp  # type: ignore
from pyarrow import dataset as ds  # type: ignore
from pyarrow.fs import FileSystem, S3FileSystem  # type: ignore

from .time_util import mjd2datetime


SOLAR_BUCKET = "odin-solar"
SOLAR_DB = "spacedata_observed.parquet"
S3_REGION = "eu-north-1"


class NoSolarDataError(Exception):
    pass


def assert_solar_exists(
    date: dt.date,
    filesystem: FileSystem,
    bucket: str = SOLAR_BUCKET,
    key: str = SOLAR_DB,
) -> None:
    data = ds.dataset(
        f"{bucket}/{key}",
        filesystem,
    ).to_table(
        filter=ds.field("DATE") == Timestamp(date)
    ).to_pandas()

    if data.size == 0:
        raise NoSolarDataError(f"No solar data found for {date}")


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    dates: set[dt.date] = set()
    for fm in event["ScansInfo"]:
        dates = dates.union(
            {mjd2datetime(scan["MJDStart"]).date() for scan in fm}
        ).union(
            {mjd2datetime(scan["MJDEnd"]).date() for scan in fm}
        )

    filesystem = S3FileSystem(region=S3_REGION)
    for date in dates:
        assert_solar_exists(date, filesystem)

    return {"StatusCode": 200}
