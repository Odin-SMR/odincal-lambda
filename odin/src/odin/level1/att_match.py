from typing import Protocol

import pandas as pd
from oops.odin import Spectrum

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
        df = self.att.reindex(
            self.ac.index,
            method="bfill",
            tolerance=17,
        )
        df["jd"] = datetime64ms_to_jd(df["datetime"])
        data_list = []
        for stw_curr, jd, orbit, qt, qa, qe, gps, acs in df[
            ["jd", "orbit", "qt", "qa", "qe", "gps", "acs"]
        ].itertuples(index=True, name=None):
            s = Spectrum()
            s.stw = stw_curr
            s.Attitude(
                (
                    jd,
                    stw_curr,
                    orbit,
                    qt,
                    qa,
                    qe,
                    gps,
                    acs,
                )
            )
            data = {k: getattr(s, k) for k in FIELDS}

            data_list.append(data)
        df_attitude = pd.DataFrame(data_list).set_index("stw")
        # df_attitude["stw"] = df_out.index.values
        df[df_attitude.columns] = df_attitude
        df_attitude["smr_height"] = df["smr_pos"].str[2]
        return df
