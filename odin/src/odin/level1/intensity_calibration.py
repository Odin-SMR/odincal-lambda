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


class IntensityCalibrationMixin:
    def intensity_calibration(self: HasIntensityCalData) -> pd.DataFrame:
        S = np.stack(self.specs["spectra_sig"].to_numpy())
        print(self.specs["t_height"])
        S_hi = np.stack(
            self.specs["spectra_sig"][self.specs["t_height"] > 120].to_numpy()
        )
        C_hi = np.stack(
            self.specs["spectra_ref"][self.specs["t_height"] > 120].to_numpy()
        )
        C = np.stack(self.specs["spectra_ref"].to_numpy())
        H = np.stack(self.specs["hot"].to_numpy())
        H_hi = np.stack(self.specs["hot"][self.specs["t_height"] > 120].to_numpy())
        hot_temp = (
            self.housekeeping.loc[self.specs.index]["image load B-side"].to_numpy()[
                :, None, None
            ]
            + 273.15
        )
        fq_grid = np.stack(
            self.frequency_grid["frequency_grid"]
            .loc[self.specs["spectra_sig"].index]
            .to_numpy()
        ).astype(np.float64)

        Tbg = J(2.7, fq_grid)
        Thot = J(hot_temp, fq_grid)
        eta = 1

        diff = S - C
        span = H - C
        span_hi = H_hi - C_hi

        p20 = np.nanpercentile(span_hi, 20, axis=0)
        span_hi_med = np.nanmedian(span_hi, axis=0)
        mask = (span_hi > p20) & (span_hi > 0.7 * span_hi_med)
        pe_hi = np.where(mask, (S_hi - C_hi) / span_hi, np.nan)
        PE = np.nanmedian(pe_hi, axis=0)
        PE = PE - np.nanmedian(PE, axis=-1, keepdims=True)

        diff_debiased = diff - PE[None, :, :] * (span)


        cal_J = Tbg + diff_debiased * (Thot - Tbg) / span
        cal_T = J_inv(cal_J, fq_grid)

        hi = self.specs["t_height"].to_numpy() > 120

        Tamb = J(300.0, fq_grid)

        ratio = (cal_J[hi] - Tbg[hi]) / (Tamb[hi] - Tbg[hi])
        one_minus_eta_band = np.clip(np.nanmedian(ratio, axis=(0, 2)), 0.0, 0.3)
        eta_band = 1.0 - one_minus_eta_band

        eta = eta_band[None, :, None]
        cal_J_corr = (cal_J - (1.0 - eta) * Tamb) / eta
        cal_T_corr = J_inv(cal_J_corr, fq_grid)

        df = pd.DataFrame(index=self.specs.index)
        df["calibrated"] = list(cal_J_corr)
        # df["path_error"] = list(path_error)
        df["span"] = list(span)
        df["calibrated_T"] = list(cal_T_corr)

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
