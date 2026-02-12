from typing import Protocol

import numpy as np
import pandas as pd


class HasACData(Protocol):
    ac: pd.DataFrame
    shk: pd.DataFrame


def FECA_drift(t):
    return 1 + (29.23 - 0.138 * t) * 1e-6


def FECB_drift(t):
    return 1 + (24.69 - 0.109 * t) * 1e-6


class HouseKeepingMixin:
    def shk_match(self: HasACData) -> pd.DataFrame:
        """Match housekeeping data to AC data based on STW."""
        # do some interpolation / nearest-neighbor matching
        df = (
            self.shk.dropna(how="all", axis=0)
            .ffill()
            .bfill()
            .reindex(
                self.ac.index,
                method="ffill",
            )
        )
        temp_source_lock = "image load B-side"  # TODO: fix based on frontend
        temp = df[temp_source_lock]
        temp_source_warm = "warm IF B-side"
        temp_warm = df[temp_source_warm]
        print(temp.mean(), temp.min(), temp.max())
        df["LO frequency 495"] = (
            FECA_drift(temp)
            * (df["HRO frequency 495"] * 17.0 + df["PRO frequency 495"])
        ) * 6.0e6

        df["LO frequency 549"] = (
            FECA_drift(temp)
            * (df["HRO frequency 549"] * 19.0 + df["PRO frequency 549"])
        ) * 6.0e6
        df["LO frequency 572"] = (
            FECB_drift(temp)
            * (df["HRO frequency 572"] * 20.0 + df["PRO frequency 572"])
        ) * 6.0e6

        df["LO frequency 555"] = (
            #FECB_drift(temp)
            1 * (df["HRO frequency 555"] * 19.0 + df["PRO frequency 555"])
        ) * 6.0e6
        df["ssb_fq"] = np.where(
            self.ac["backend"].to_numpy() == "AC1",
            self.ac["ssb_fq"] * (1 + (50.03 - 0.974 * temp_warm.to_numpy()) * 1e-6),
            self.ac["ssb_fq"] * (1 + (50.63 - 0.920 * temp_warm.to_numpy()) * 1e-6),
        )
        df["adjusted_clock"] = np.where(
            self.ac["backend"].to_numpy() == "AC1",
            224.001466e6 - 68.3 * (temp_warm.to_numpy() - 22.5),
            223.059419e6 - 85.9 * (temp_warm.to_numpy() - 21.2),
        )
        return df
