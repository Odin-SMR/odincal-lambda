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

        vgeo = self.attitude.loc[idx, "vgeo_tan"].to_numpy()[:, None, None]  # (n,1,1)

        hk = self.housekeeping.loc[idx]
        frontend = self.ac.loc[idx, "frontend"]

        lo_cols = ("LO frequency " + frontend).to_numpy()
        colindex = hk.columns.get_indexer(lo_cols)

        rows = np.arange(len(hk))
        #lo = hk.to_numpy()[rows, colindex][:, None, None]  # (n,1,1)
        lo = apply_lofreqcorr_by_frontend(
            self.ac.loc[idx, "frontend"].to_numpy(),
            hk.to_numpy()[rows, colindex],
            hk["image load B-side"].to_numpy(),
            self.attitude.loc[idx, "mjd"].to_numpy()
        )[:, None, None]

        ssb_fq = np.repeat(np.stack(hk.ssb_fq.to_numpy()), 2, axis=1)  # (n,8)
        skyfreq = lo + ssb_fq[:, :, None] * 1e6  # (n,8,1)

        c = 2.99792458e8
        restfreq = skyfreq * (
            1.0 + vgeo / c
        )  # maybe flip sign depending on vgeo convention

        delta_f = (
            hk["adjusted_clock"].to_numpy()[:, None, None] / 2.0 / 112.0
        )  # (n,1,1)
        # delta_f = (
        #     100e6 / 112
        # )  # hardcoded for now, but should be derived from clock and decimation
        print("deltaf", delta_f)


        if_sign = np.array([+1, -1, +1, -1, -1, +1, -1, +1])[None, :, None]  # (1,8,1)
        k = np.arange(112)[None, None, :]  # (1,1,112)

        grid = restfreq + if_sign * k * delta_f  # (n,8,112)

        df = pd.DataFrame(index=idx)
        df["frequency_grid"] = list(grid)
        df["lo"] = lo.squeeze()
        return df


def apply_lofreqcorr_by_frontend(
    frontend: np.ndarray,
    lo: np.ndarray,
    temp: np.ndarray,
    mjd: np.ndarray,
) -> np.ndarray:
    """
    Donal-style LO frequency correction, keyed directly on frontend.

    Parameters
    ----------
    frontend : np.ndarray
        Frontend identifier per scan. Expected values: 495, 549, 555, 572
        (either ints/floats; will be rounded to int).
    lo : np.ndarray
        LO frequency per scan (Hz). Shape (n,).
    temp : np.ndarray
        Warm-load / image-load temperature proxy per scan (degC). Shape (n,).
    mjd : np.ndarray
        Modified Julian Date per scan (days). Shape (n,).

    Returns
    -------
    lo_corr : np.ndarray
        Corrected LO frequency (Hz). Shape (n,).
    """
    frontend = np.asarray(frontend)
    lo = np.asarray(lo, dtype=np.float64)
    temp = np.asarray(temp, dtype=np.float64)
    mjd = np.asarray(mjd, dtype=np.float64)

    if lo.shape != temp.shape or lo.shape != mjd.shape:
        raise ValueError(
            f"lo/temp/mjd must have the same shape; got lo={lo.shape}, temp={temp.shape}, mjd={mjd.shape}"
        )
    if frontend.shape != lo.shape:
        raise ValueError(
            f"frontend must have the same shape as lo; got frontend={frontend.shape}, lo={lo.shape}"
        )

    # Parameters from the snippet you found:
    # (original labels: "1", "2", "19", "13"; we map them directly to frontends)
    p_495 = np.array(
        [-9.77071337e-08, -3.04935334e-10, 1.00004369], dtype=np.float64
    )  # was "1"
    p_555 = np.array(
        [-7.20429255e-08, -9.88146910e-10, 1.00007687], dtype=np.float64
    )  # was "13"
    p_549 = 0.5 * (
        np.array(
            [-2.85146234e-08, -6.44075856e-10, 1.00005892], dtype=np.float64
        )  # "2"
        + np.array(
            [-4.93032042e-08, -6.11110969e-10, 1.00005802], dtype=np.float64
        )  # "19"
    )

    # Choose what to do for 572. If you later find a dedicated triple for 572,
    # swap it in here. For now, reuse the 549 average.
    p_572 = p_549

    # Build (n,3) parameter matrix P = [a,b,c] per scan
    fe = frontend

    P = np.empty((lo.size, 3), dtype=np.float64)
    P[:] = p_549  # default/fallback

    P[fe == "495"] = p_495
    P[fe == "549"] = p_549
    P[fe == "555"] = p_555
    P[fe == "572"] = p_572

    a = P[:, 0]
    b = P[:, 1]
    c = P[:, 2]

    kfactor = a * (temp) + b * mjd + c
    print("kfactor stats:", np.nanmean(kfactor))
    lo_corr = lo * kfactor
    print('lo_corr shape', lo_corr.shape)
    return lo_corr


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
