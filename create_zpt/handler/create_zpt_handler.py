"""Create and store ZPT file for input
"""

import datetime as dt
import os
from typing import Any

from pandas import DataFrame, Timestamp  # type: ignore
from xarray import Dataset
import pyarrow as pa  # type: ignore
import pyarrow.parquet as pq


from .check_zpt_handler import ZPT_BUCKET, ZPT_PATTERN
from .era5_dataset import get_dataset as get_era5
from .newdonalettyERANC import Donaletty
from .time_util import mjd2datetime
from .geoloc_tools import get_scan_geo_loc
from .geos import gmh
from .scan_data_descriptions import parameter_desc
from .log_configuration import logconfig


AWS_REGION = "eu-north-1"


def get_scan_data(scans_info: list[dict[str, Any]]) -> Dataset:
    data = []
    for d in scans_info:
        data.extend(d["ScansInfo"])

    df = DataFrame.from_records(data).drop_duplicates(subset=["ScanID"])
    df["MJDMid"] = (df["MJDStart"] + df["MJDEnd"]) * 0.5
    df["LatMid"], df["LonMid"] = get_scan_geo_loc(
        df["LatStart"],
        df["LonStart"],
        df["LatEnd"],
        df["LonEnd"],
    )
    df["DateMid"] = df["MJDMid"].apply(lambda x: mjd2datetime(x).replace(tzinfo=None))

    scans = df.set_index("ScanID").to_xarray()
    scans.ScanID.attrs = parameter_desc["scanid"]
    scans.DateMid.attrs = parameter_desc["mid_date"]
    scans.Backend.attrs = parameter_desc["backend"]
    scans.LatStart.attrs = parameter_desc["latitude"]
    scans.LonStart.attrs = parameter_desc["longitude"]
    scans.MJDStart.attrs = parameter_desc["mjd"]
    scans.LatMid.attrs = parameter_desc["mid_latitude"]
    scans.LonMid.attrs = parameter_desc["mid_longitude"]
    scans.MJDMid.attrs = parameter_desc["mjd_mid"]
    scans.LatEnd.attrs = parameter_desc["end_latitude"]
    scans.LonEnd.attrs = parameter_desc["end_longitude"]
    scans.MJDEnd.attrs = parameter_desc["end_mjd"]

    return scans


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    logconfig()
    scans = get_scan_data(event["ScansInfo"])

    era5_data = read_era5(scans)

    scans = merge_era5(scans, era5_data)

    donaletty = Donaletty()
    profiles = donaletty.makeprofile(scans)

    table = pa.Table.from_pandas(profiles.to_dataframe().reset_index())

    filename = os.path.split(event["File"])[-1]
    prefix = filename[0:3]
    backend = event["Backend"]
    zpt_file = ZPT_PATTERN.format(
        backend=backend,
        prefix=prefix,
        filename=filename,
    )

    s3 = pa.fs.S3FileSystem(region=AWS_REGION)
    pq.write_table(
        table,
        f"{ZPT_BUCKET}/{zpt_file}",
        filesystem=s3,
        version="2.6",
    )

    return {
        "StatusCode": 201,
    }


def read_era5(scans: Dataset) -> Dataset:
    dates: list[dt.date] = list(set(Timestamp(d).date() for d in scans.DateMid.values))
    dates.sort()
    era5_data = get_era5(dates)
    era5_data["longitude"] = era5_data.longitude - 180
    return era5_data


def merge_era5(ds: Dataset, era5: Dataset) -> Dataset:
    ds["era5_level"] = era5.level
    ds["era5_z"] = (
        ["ScanID", "level"],
        era5.z.sel(
            latitude=ds["LatMid"],
            longitude=ds["LonMid"],
            time=ds["DateMid"],
            method="nearest",
        ).data,
    )
    ds.era5_z.attrs = era5.z.attrs
    ds["era5_t"] = (
        ["ScanID", "level"],
        era5.t.sel(
            latitude=ds["LatMid"],
            longitude=ds["LonMid"],
            time=ds["DateMid"],
            method="nearest",
        ).data,
    )
    ds.era5_t.attrs = era5.t.attrs
    ds["era5_gmh"] = gmh(ds.LatMid, ds.era5_z)
    ds.era5_gmh.attrs = {
        "long_name": "geometric height",
        "units": "km",
    }
    # pressure in mb:
    ds["theta"] = ds["era5_t"] * (1e3 / ds["level"]) ** 0.286
    ds.theta.attrs = {
        "long_name": "Potential temperature",
        "units": "K",
    }
    return ds
