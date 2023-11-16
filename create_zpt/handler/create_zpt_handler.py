"""Create and store ZPT file for input
"""
import datetime as dt
import os
from typing import Any

from pandas import DataFrame, Timestamp  # type: ignore
from xarray import DataArray
import pyarrow as pa  # type: ignore
import pyarrow.parquet as pq  # type: ignore

from .check_zpt_handler import ZPT_BUCKET, ZPT_PATTERN
from .era5_dataset import get_dataset as get_era5
from .newdonalettyERANC import Donaletty
from .time_util import mjd2datetime
from .geoloc_tools import get_scan_geo_loc
from .geos import gmh
from .scan_data_descriptions import parameter_desc
from .log_configuration import logconfig


AWS_REGION = "eu-north-1"


def get_scan_data(scans_info: list[dict[str, Any]]) -> DataArray:
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
    df["DateMid"] = df["MJDMid"].apply(
        lambda x: mjd2datetime(x).replace(tzinfo=None)
    )

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

    dates: list[dt.date] = list(set(
        Timestamp(d).date() for d in scans.DateMid.values
    ))
    dates.sort()
    era5_data = get_era5(dates)
    era5_data["longitude"] = era5_data.longitude - 180

    scans["era5_level"] = era5_data.level
    scans["era5_z"] = (
        ["ScanID", "level"],
        era5_data.z.sel(
            latitude=scans["LatMid"],
            longitude=scans["LonMid"],
            time=scans["DateMid"],
            method="nearest",
        ).data,
    )
    scans.era5_z.attrs = era5_data.z.attrs
    scans["era5_t"] = (
        ["ScanID", "level"],
        era5_data.t.sel(
            latitude=scans["LatMid"],
            longitude=scans["LonMid"],
            time=scans["DateMid"],
            method="nearest",
        ).data,
    )
    scans.era5_t.attrs = era5_data.t.attrs
    scans["era5_gmh"] = gmh(scans.LatMid, scans.era5_z)
    scans.era5_gmh.attrs = {
        "long_name": "geometric height",
        "units": "km",
    }
    # pressure in mb:
    scans["theta"] = scans["era5_t"] * (1e3 / scans["level"]) ** 0.286
    scans.theta.attrs = {
        "long_name": "Potential temperature",
        "units": "K",
    }

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
