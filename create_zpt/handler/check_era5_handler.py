"""Check if data is available in the ERA5 DB for creating ZPT for the file
"""
import datetime as dt
from typing import Any

import boto3
from botocore.exceptions import ClientError

from .era5_dataset import ERA5_BUCKET, ERA5_PATTERN
from .time_util import mjd2datetime
from .log_configuration import logconfig

logconfig()


class NoERA5DataError(Exception):
    pass


def assert_era5_exists(
    date: dt.date,
    s3_client: Any,
    bucket: str = ERA5_BUCKET,
) -> None:
    try:
        s3_client.Object(
            bucket,
            ERA5_PATTERN.format(
                year=date.year,
                month=date.month,
                date=date.isoformat(),
            ),
        )
    except ClientError as err:
        raise NoERA5DataError(f"No ERA5 data found for {date} ({err})")


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    dates: set[dt.date] = set()
    for fm in event["ScansInfo"]:
        dates = dates.union(
            {mjd2datetime(scan["MJDStart"]).date() for scan in fm["ScansInfo"]}
        ).union({mjd2datetime(scan["MJDEnd"]).date() for scan in fm["ScansInfo"]})

    s3_client = boto3.resource("s3")
    for date in dates:
        assert_era5_exists(date, s3_client)

    return {"StatusCode": 200}
