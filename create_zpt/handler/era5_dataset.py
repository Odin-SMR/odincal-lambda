import datetime as dt

import boto3
import s3fs  # type: ignore
import xarray

ERA5_BUCKET = "odin-era5"
ERA5_PATTERN = "{year}/{month:02d}/ea_pl_{date}.zarr"

s3 = s3fs.S3FileSystem()

s3_client = boto3.client("s3")


def get_dataset(range: list[dt.date]):
    stores = [
        s3fs.S3Map(
            root="s3://{bucket}/{key}".format(
                bucket=ERA5_BUCKET,
                key=ERA5_PATTERN.format(
                    year=d.year,
                    month=d.month,
                    date=d.isoformat()
                )
            ),
            check=False,
            s3=s3,
        )
        for d in range
    ]
    datasets = [xarray.open_zarr(store) for store in stores]
    combined = xarray.concat(datasets, dim="time")
    combined.sortby("time")
    return combined
