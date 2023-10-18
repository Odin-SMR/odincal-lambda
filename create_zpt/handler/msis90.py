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
import numpy as np
import pyarrow.parquet as pq  # type: ignore
from nrlmsise00 import msise_model  # type: ignore
from s3fs import S3FileSystem  # type: ignore

from .atmos import BOLTZMAN


class Msis90:
    """class for access to MSIS90e model"""

    def __init__(
        self,
        solardatafile="s3://odin-solar/spacedata_observed.parquet",
    ):
        self.solardatafile = solardatafile

    def extractPTZprofilevarsolar(self, datetime, lat, lng, altitudes):
        s3 = S3FileSystem()
        solardatafile = "s3://odin-solar/spacedata_observed.parquet"
        table = pq.read_table(
            solardatafile,
            columns=["DATE", "AP_AVG", "OBS", "OBS_CENTER81"],
            filesystem=s3,
        )
        df = table.to_pandas()
        apavg, f107, f107a = df.loc[datetime.date().isoformat()]
        P = np.zeros(altitudes.shape)
        T = np.zeros(altitudes.shape)
        Z = altitudes
        for i, alt in enumerate(altitudes):
            dens, temp = msise_model(
                datetime, alt, lat, lng, f107a, f107, apavg, flags=[1] * 24
            )
            t = np.array(temp)
            d = np.array(dens)
            T[i] = t[1]
            P[i] = (d.sum() - d[5]) * BOLTZMAN * t[1] / 100.0

        return P, T, Z

    def extractPTZprofilefixedsolar(self, datetime, lat, lng, altitudes):
        f107 = 100
        f107a = 100
        apavg = 4
        P = np.zeros(altitudes.shape)
        T = np.zeros(altitudes.shape)
        Z = altitudes
        for i, alt in enumerate(altitudes):
            dens, temp = msise_model(
                datetime,
                alt,
                lat,
                lng,
                f107a,
                f107,
                apavg,
            )
            t = np.array(temp)
            d = np.array(dens)
            T[i] = t[1]
            P[i] = (d.sum() - d[5]) * BOLTZMAN * t[1] / 100.0
        return P, T, Z
