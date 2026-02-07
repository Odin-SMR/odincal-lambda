from typing import Protocol

import numpy as np
import pandas as pd

h = 6.626176e-34
k = 1.380662e-23


class HasIntensityCalData(Protocol):
    specs: pd.DataFrame
    ac: pd.DataFrame
    housekeeping: pd.DataFrame
    attitude: pd.DataFrame
    frequency_grid: pd.DataFrame


class intensityCalibrationMixin:
    def intensity_calibration(self: HasIntensityCalData) -> pd.DataFrame:
        sig = np.stack(self.specs["spectra_sig"].to_numpy())
        sig_hi = np.stack(
            self.specs["spectra_sig"][self.specs["t_height"] > 120].to_numpy()
        )
        ref_hi = np.stack(
            self.specs["spectra_ref"][self.specs["t_height"] > 120].to_numpy()
        )
        ref = np.stack(self.specs["spectra_ref"].to_numpy())
        hot = np.stack(self.specs["hot"].to_numpy())
        hot_hi = np.stack(self.specs["hot"][self.specs["t_height"] > 120].to_numpy())
        hot_temp = (
            self.housekeeping.loc[self.specs.index]["image load B-side"].to_numpy()[
                :, None, None
            ]
            + 273.15
            - 1.1
        )
        fq_grid = np.stack(
            self.frequency_grid["frequency_grid"]
            .loc[self.specs["spectra_sig"].index]
            .to_numpy()
        ).astype(np.float64)
        print(fq_grid.shape, fq_grid.dtype)

        Tbg = J(2.7, fq_grid)
        Thot = J(hot_temp, fq_grid)
        eta = 1

        hot_ref_span_hi = hot_hi - ref_hi
        span = np.percentile(hot_ref_span_hi, 10, axis=0)
        W = np.nanmedian(span, axis=-1, keepdims=True) / span
        W = W / np.nanmedian(W, axis=-1, keepdims=True)

        diff_eq = (sig - ref) * W
        diff_eq_hi = (sig_hi - ref_hi) * W
        span_eq = (hot - ref) * W

        offset_counts = np.percentile(diff_eq_hi, 50, axis=0)  # or 10 if you like
        diff_eq0 = diff_eq - offset_counts

        cal = (Tbg + (diff_eq0) * (Thot - Tbg) / (span_eq)) / eta

        df = pd.DataFrame(index=self.specs.index)
        cal_T = J_inv(cal, fq_grid)
        df["calibrated"] = list(cal)
        return df


def J(T, f_hz):
    # T: scalar or array
    # f_hz: scalar or array broadcastable to T
    T0 = h * f_hz / k
    T = np.asarray(T, dtype=float)
    return np.where(T > 0, T0 / (np.expm1(T0 / T)), 0.0)


def J_inv(Jb, f_hz):
    T0 = h * f_hz / k
    Jb = np.asarray(Jb, dtype=float)
    # Jb = T0/(exp(T0/T)-1)  =>  exp(T0/T)=1+T0/Jb  =>  T = T0 / ln(1+T0/Jb)
    return np.where(Jb > 0, T0 / np.log1p(T0 / Jb), 0.0)
