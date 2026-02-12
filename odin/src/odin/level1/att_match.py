from typing import Protocol

import numpy as np
import pandas as pd

from odin.level1.odin_geometry import (
    vgeo_from_products_with_astropy,
)

JD_EPOCH = pd.Timestamp("1858-11-17T00:00:00Z")  # MJD epoch

FIELDS = [
    "stw",
    "lst",
    "skybeamhit",
    "ra2000",
    "dec2000",
    "vsource",
    "sunpos",
    "moonpos",
    "vgeo",
    "vlsr",
    "sunzd",
]


def datetime64ms_to_mjd(s: pd.Series) -> pd.Series:
    return (s - JD_EPOCH) / pd.Timedelta(days=1) #+ 2_400_000.5


class HasACData(Protocol):
    ac: pd.DataFrame
    att: pd.DataFrame


class AttMatchMixin:
    def att_match(self: HasACData) -> pd.DataFrame:
        """Match attitude data to AC data based on STW."""
        # do some interpolation / nearest-neighbor matching
        stw_ac = self.ac.index.to_numpy()  # - 14
        stw_att = self.att.index.to_numpy()
        print(stw_ac.dtype, stw_att.dtype)

        sc_itrs = np.stack(self.att["gps"].to_numpy())

        utc_att = self.att["datetime"].astype("int")
        print(utc_att.dtype)

        sc_itrs_ac = np.column_stack(
            [
                np.interp(
                    stw_ac,
                    stw_att,
                    sc_itrs[:, i],
                )
                for i in range(sc_itrs.shape[1])
            ]
        )

        tp_llh_att = np.stack(self.att["smr_pos"].to_numpy()).astype(
            float
        )  # [lat_deg, lon_deg, alt_km?]
        # Make sure units are correct:
        # If ACS gives alt in km (STW.ATT does), convert to meters for interpolation if you want meters
        tp_lat = tp_llh_att[:, 0]
        tp_lon = tp_llh_att[:, 1]
        tp_h_km = tp_llh_att[:, 2]

        # unwrap lon in radians to avoid 359->1 discontinuity
        lon_rad = np.deg2rad(tp_lon)
        lon_rad_unwrap = np.unwrap(lon_rad)
        tp_lon_unwrap = np.rad2deg(lon_rad_unwrap)

        lat_ac = np.interp(stw_ac, stw_att, tp_lat)
        lon_ac = np.interp(stw_ac, stw_att, tp_lon_unwrap)
        h_ac_km = np.interp(stw_ac, stw_att, tp_h_km)

        # wrap lon back if you want
        lon_ac = (lon_ac + 360.0) % 360.0

        tp_llh_ac = np.column_stack([lat_ac, lon_ac, h_ac_km])

        utc_ac = np.interp(stw_ac, stw_att, utc_att).astype("datetime64[ms]")

        df = pd.DataFrame()
        df["stw"] = stw_ac  # + 14
        df["datetime"] = utc_ac = pd.to_datetime(utc_ac, utc=True)
        df["mjd"] = datetime64ms_to_mjd(df["datetime"])

        df["smr_height"] = tp_llh_ac[:, 2]
        df["sc"] = list(sc_itrs_ac)
        df["smr_pos"] = list(tp_llh_ac)
        df["vgeo_tan"], df["vgeo_none"], df["vgeo_sc"] = (
            vgeo_from_products_with_astropy(
                sc_itrs_ac.astype(float), tp_llh_ac.astype(float), utc_ac
            )
        )

        return df.set_index("stw")
