""" Check if ZPT file already exists for the file being processed.
"""
import os
from typing import Any

import boto3
from botocore.exceptions import ClientError

ZPT_BUCKET = "odin-zpt"
ZPT_PATTERN = "{backend}/{prefix}/{filename}.zarr"


class NoZPTError(Exception):
    pass


def assert_zpt_exists(
    filename: str,
    prefix: str,
    backend: str,
    s3_client: Any,
    bucket: str = ZPT_BUCKET,
) -> None:
    try:
        s3_client.Object(
            bucket,
            ZPT_PATTERN.format(
                backend=backend,
                prefix=prefix,
                filename=filename,
            )
        ).load()
    except ClientError as err:
        raise NoZPTError(f"No ERA5 data found for {filename} ({err})")


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    filename = os.path.split(event["name"])[-1]
    prefix = filename[0:3]
    backend = event["type"]

    s3_client = boto3.resource("s3")
    try:
        assert_zpt_exists(filename, prefix, backend, s3_client)
    except NoZPTError:
        return {"StatusCode": 404}

    return {"StatusCode": 200}
