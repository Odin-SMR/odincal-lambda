from pandas import Timestamp
import xarray
from create_zpt.handler.create_zpt_handler import (
    get_scan_data,
    handler,
    merge_era5,
    read_era5,
)
from create_zpt.handler.newdonalettyERANC import Donaletty


def test_scans(scans):
    ds_scans = get_scan_data(scans["ScansInfo"])
    assert isinstance(ds_scans, xarray.Dataset)
    assert list(ds_scans.dims.mapping.keys()) == ["ScanID"]


def test_era5(scans):
    ds_scans = get_scan_data(scans["ScansInfo"])
    ds_era5 = read_era5(ds_scans)
    assert isinstance(ds_era5, xarray.Dataset)
    assert list(ds_scans.dims.mapping.keys()) == ["ScanID"]


def test_merge_era5(scans):
    ds_scans = get_scan_data(scans["ScansInfo"])
    ds_era5 = read_era5(ds_scans)
    ds = merge_era5(ds_scans, ds_era5)
    print(ds)
    assert isinstance(ds_era5, xarray.Dataset)
    assert list(ds.dims.mapping.keys()) == ["ScanID", "level"]


def test_profiles(scans):
    ds_scans = get_scan_data(scans["ScansInfo"])
    ds_era5 = read_era5(ds_scans)
    ds = merge_era5(ds_scans, ds_era5)
    donaletty = Donaletty()
    profiles = donaletty.makeprofile(ds)
    print(profiles)
    assert isinstance(profiles, xarray.Dataset)
    assert list(profiles.dims.mapping.keys()) == ["ScanID", "z"]
