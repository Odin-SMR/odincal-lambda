import datetime as dt

import s3fs  # type: ignore
from xarray import Dataset, concat, open_dataset, open_zarr
from dask import delayed
import zarr
import zarr.storage

ERA5_BUCKET = "odin-era5"
ERA5_PATTERN = "{year}/{month:02d}/ea_pl_{date}.zarr"


@delayed
def read_dataset(file: str):
    s3 = s3fs.S3FileSystem(asynchronous=True)

    # Open the Zarr store
    store = zarr.storage.FsspecStore(path=file, fs=s3)
    ds = open_zarr(
        store,
        consolidated=True,
    )
    # there is a breakpoint 2024-09-18
    # - Slightly different data format
    # - files after are in zarr3
    if "expver" in ds.coords:
        ds = ds.drop_vars("expver")
    if "number" in ds.coords:
        ds = ds.drop_vars("number")
    return ds


def read_zarr_dataset(
    dates: list[dt.date],
) -> Dataset:
    files = [
        f"{ERA5_BUCKET}/{ERA5_PATTERN.format(year=date.year, month=date.month, date=date.isoformat())}"
        for date in dates
    ]
    tasks = [read_dataset(f) for f in files]
    ds_combined = delayed(concat)(tasks, dim="time")
    return ds_combined.compute()


def get_dataset(range: list[dt.date]) -> Dataset:
    ds = read_zarr_dataset(range)
    return ds.sortby("time")
