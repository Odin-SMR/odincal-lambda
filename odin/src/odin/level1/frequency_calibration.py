from typing import Protocol

import numpy as np
import pandas as pd


class HasFQCalData(Protocol):
    specs: pd.DataFrame
    ac: pd.DataFrame
    housekeeping: pd.DataFrame
    attitude: pd.DataFrame


class FrequencyCalibrationMixin:
    def frequency_calibration(self: HasFQCalData) -> pd.DataFrame:
        idx = self.specs.index

        vgeo = self.attitude.loc[idx, "vgeo"].to_numpy()[:, None, None]  # (n,1,1)

        hk = self.housekeeping.loc[idx]
        frontend = self.ac.loc[idx, "frontend"]

        lo_cols = ("LO frequency " + frontend).to_numpy()
        colindex = hk.columns.get_indexer(lo_cols)

        rows = np.arange(len(hk))
        lo = hk.to_numpy()[rows, colindex][:, None, None]  # (n,1,1)

        ssb_fq = np.repeat(np.stack(hk.ssb_fq.to_numpy()), 2, axis=1)  # (n,8)
        skyfreq = lo + ssb_fq[:, :, None] * 1e6  # (n,8,1)

        c = 2.99792458e8
        restfreq = skyfreq * (
            1.0 + vgeo / c
        )  # maybe flip sign depending on vgeo convention

        delta_f = (
            hk["adjusted_clock"].to_numpy()[:, None, None] / 2.0 / 112.0
        )  # (n,1,1)
        # delta_f = 1e6  # hardcoded for now, but should be derived from clock and decimation

        if_sign = np.array([+1, -1, +1, -1, -1, +1, -1, +1])[None, :, None]  # (1,8,1)
        k = np.arange(112)[None, None, :]  # (1,1,112)

        grid = restfreq + if_sign * k * delta_f  # (n,8,112)

        df = pd.DataFrame(index=idx)
        df["frequency_grid"] = list(grid)
        df["lo"] = lo.squeeze()
        return df


# _RX_CONST = {
#     "495": (61600.36, 104188.89, 0.0002977862),
#     "549": (57901.86, 109682.58, 0.0003117128),
#     "555": (60475.43, 116543.50, 0.0003021341),
#     "572": (58120.92, 115256.73, 0.0003128605),
# }
# def get_sideband(rx_i, lo_freq, ssb):
#     ssb_params = {
#         "495": (
#             61600.36,
#             104188.89,
#             0.0002977862,
#         ),
#         "549": (
#             57901.86,
#             109682.58,
#             0.0003117128,
#         ),
#         "555": (
#             60475.43,
#             116543.50,
#             0.0003021341,
#         ),
#         "572": (
#             58120.92,
#             115256.73,
#             0.0003128605,
#         ),
#     }
#     path = 0.0
#     c1_ssb = ssb_params[rx_i][0]
#     c2_ssb = ssb_params[rx_i][1]
#     sf_ssb = ssb_params[rx_i][2]
#     for i in range(-2, 3):
#         s3900 = 299.79 / (ssb + c1_ssb) * (c2_ssb + i / sf_ssb) - lo_freq / 1.0e9
#         if abs(abs(s3900) - 3.9) < abs(abs(path) - 3.9):
#             path = s3900
#     if path < 0.0:
#         if_freq = -3900.0e6
#     else:
#         if_freq = 3900.0e6
#     return if_freq
