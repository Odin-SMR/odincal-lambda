from typing import Protocol

import numpy as np
import pandas as pd

from odin.level1.geometry import ecef_to_geodetic_xyz, geodetic_to_ecef_llh
from odin.level1.odin_geometry import (
    compute_vgeo_from_products,
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


def datetime64ms_to_jd(s: pd.Series) -> pd.Series:
    return (s - JD_EPOCH) / pd.Timedelta(days=1) + 2_400_000.5


class HasACData(Protocol):
    ac: pd.DataFrame
    att: pd.DataFrame


class AttMatchMixin:
    def att_match(self: HasACData) -> pd.DataFrame:
        """Match attitude data to AC data based on STW."""
        # do some interpolation / nearest-neighbor matching
        stw_ac = self.ac.index.to_numpy() - 14
        stw_att = self.att.index.to_numpy()
        print(stw_ac.dtype, stw_att.dtype)

        sc_itrs = np.stack(self.att["gps"].to_numpy())
        tangent_point_att = np.stack(self.att["smr_pos"].to_numpy())

        tp_ecef_att = geodetic_to_ecef_llh(tangent_point_att)
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
        tp_ecef_ac = np.column_stack(
            [
                np.interp(
                    stw_ac,
                    stw_att,
                    tp_ecef_att[:, i],
                )
                for i in range(tp_ecef_att.shape[1])
            ]
        )
        utc_ac = np.interp(stw_ac, stw_att, utc_att).astype("datetime64[ms]")
        tp_geodetic_ac = ecef_to_geodetic_xyz(tp_ecef_ac)

        df = pd.DataFrame()
        df["stw"] = stw_ac + 14
        df["datetime"] = utc_ac = pd.to_datetime(utc_ac, utc=True)
        df["jd"] = datetime64ms_to_jd(df["datetime"])

        df["smr_height"] = tp_ecef_ac[:, 2]
        df["sc"] = list(sc_itrs_ac)
        df["smr_pos"] = list(tp_geodetic_ac)
        # df["vgeo"],df["vgeo_rel_air"] = vgeo_from_att_variants(
        #     df["datetime"],  # add small offset to avoid interpolation issues at boundaries
        #     sc_itrs_ac[:, 0:3],
        #     sc_itrs_ac[:, 3:6],
        #     tp_geodetic_ac,
        # )
        df["vgeo_tan"],df["vgeo_none"], df["vgeo_sc"] = vgeo_from_products_with_astropy(
            sc_itrs_ac.astype(float), tp_geodetic_ac.astype(float), utc_ac
        )
        # df["vgeo_tan"],df["vgeo_none"], df["vgeo_sc"] = compute_vgeo_from_products(
        #     sc_itrs_ac, tp_geodetic_ac
        # )

        print(df.loc[7077176154:7077178969].filter(regex="vgeo"))
        return df.set_index("stw")
