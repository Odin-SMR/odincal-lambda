#  msis90.py
#
#  Copyright 2015 Donal Murtagh <donal@chalmers.se>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#
#
from datetime import datetime
from typing import TypedDict

import numpy as np
import pyarrow.dataset as ds  # type: ignore
from nrlmsise00 import msise_model  # type: ignore
from s3fs import S3FileSystem  # type: ignore

from .atmos import BOLTZMAN

SOLAR_DATA_FILE = "s3://odin-solar/spacedata_observed.parquet"


class MsisPTZ(TypedDict):
    P: np.ndarray
    T: np.ndarray
    Z: np.ndarray


def extractPTZprofilevarsolar(
    date_time: datetime,
    lat: float,
    lng: float,
    altitudes: np.ndarray,
) -> MsisPTZ:
    s3 = S3FileSystem()
    solardatafile = SOLAR_DATA_FILE
    dataset = ds.dataset(
        solardatafile,
        format="parquet",
        filesystem=s3,
    )
    table = dataset.to_table(
        columns=["DATE", "AP_AVG", "OBS", "OBS_CENTER81"],
        filter=ds.field("DATE") == date_time.date(),
    )
    df = table.to_pandas()
    apavg, f107, f107a = df.iloc[-1]
    P = np.zeros(altitudes.shape)
    T = np.zeros(altitudes.shape)
    Z = altitudes
    for i, alt in enumerate(altitudes):
        dens, temp = msise_model(
            date_time, alt, lat, lng, f107a, f107, apavg, flags=[1] * 24
        )
        t = np.array(temp)
        d = np.array(dens)
        T[i] = t[1]
        P[i] = (d.sum() - d[5]) * BOLTZMAN * t[1] / 100.0

    return MsisPTZ(P=P, T=T, Z=Z)
