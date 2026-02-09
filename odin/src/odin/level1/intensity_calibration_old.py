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
    def intensity_calibration(self: "HasIntensityCalData") -> pd.DataFrame:
        """
        v2-style calibration adapted to *your* prepare()-layout:
          - ref is a time-smoothed median ref (already in self.specs)
          - hot is a global median spectrum replicated to all STWs (already in self.specs)
          - computes Tsys(f) from ref/hot and Planck J(T,f)
          - applies v2 calibrate_partial form
          - estimates tspill from high-alt spectra and applies v2 post-correction
          - returns calibrated spectra + tspill/eta_corr (+ optional eff/efftime if inttime exists)
        """

        # --- counts arrays (n,8,112) ---
        sig = np.stack(self.specs["spectra_sig"].to_numpy()).astype(np.float64)
        ref = np.stack(self.specs["spectra_ref"].to_numpy()).astype(np.float64)
        hot = np.stack(self.specs["hot"].to_numpy()).astype(np.float64)

        assert ref.shape == sig.shape == hot.shape

        # high altitude subset
        hi_mask = self.specs["t_height"].to_numpy() > 120.0
        sig_hi = sig[hi_mask]


        # --- hot temperature (K), broadcastable ---
        # (your current convention; adjust if needed)
        hot_temp = (
            self.housekeeping.loc[self.specs.index]["image load B-side"].to_numpy()[
                :, None, None
            ]
            + 273.15
            - 1.1
        ).astype(np.float64)

        # --- fq grid (n,8,112) aligned to specs index ---
        fq_grid = np.stack(
            self.frequency_grid["frequency_grid"].loc[self.specs.index].to_numpy()
        ).astype(np.float64)

        # --- Planck brightness temperatures ---
        Tbg = J(2.7, fq_grid)  # (n,8,112)
        Thot = J(hot_temp, fq_grid)  # (n,8,112)

        # --- v2 "calibrate_partial" constants ---
        # In legacy v2: eta=1, eta_ms=1, Tamb=290 -> spill = Tbg
        eta = 1.0
        eta_ms = 1.0
        Tamb = 290.0
        spill = eta_ms * Tbg - (eta_ms - eta) * Tamb  # == Tbg for eta=eta_ms=1

        # ---------------------------------------------------------------------
        # Step A: Build a Tsys spectrum from (HOT, REF)
        #
        # v2 get_tsys() is basically a Y-factor form.
        # With your layout (hot constant, ref time-varying):
        #
        #   Tsys(f) = ref/(hot-ref) * (Thot - Tbg)     (per channel)
        # ---------------------------------------------------------------------
        span = hot - ref
        valid = (ref > 0.0) & (span > 0.0) & np.isfinite(span) & np.isfinite(ref)

        Tsys = np.zeros_like(sig)
        Tsys[valid] = (ref[valid] / span[valid]) * (Thot[valid] - Tbg[valid])

        # ---------------------------------------------------------------------
        # Step B: Apply v2 calibrate_partial:
        #   T = ((sig-ref)/ref)*Tsys + spill
        # ---------------------------------------------------------------------
        cal = np.zeros_like(sig)
        ok = (ref > 0.0) & np.isfinite(ref)
        cal[ok] = (((sig[ok] - ref[ok]) / ref[ok]) * Tsys[ok] + spill[ok]) / eta

        # ---------------------------------------------------------------------
        # Step C: v2 tspill estimate from high-alt calibrated spectra
        #   tspill = median( cal_hi[nonzero] )
        # ---------------------------------------------------------------------
        tspill_default = 9.0
        tspill = tspill_default
        if sig_hi.shape[0] > 0:
            cal_hi = cal[hi_mask]
            nz = cal_hi != 0.0
            if np.any(nz):
                tspill = float(np.nanmedian(cal_hi[nz]))

        # ---------------------------------------------------------------------
        # Step D: v2 post-correction using tspill:
        #   eta_corr = 1 - tspill/300
        #   T <- (T - tspill)/eta_corr   (for nonzero points)
        # ---------------------------------------------------------------------
        eta_corr = 1.0 - tspill / 300.0
        if not np.isfinite(eta_corr) or eta_corr <= 0:
            eta_corr = 1.0

        cal_corr = cal.copy()
        nz_all = cal_corr != 0.0
        cal_corr[nz_all] = (cal_corr[nz_all] - tspill) / eta_corr

        # ---------------------------------------------------------------------
        # Optional Step E: v2-ish efficiency / efftime (radiometer-style)
        # This is only meaningful if you have per-spectrum inttime and your
        # high-alt subset is "mostly line-free".
        # ---------------------------------------------------------------------
        eff = 1.0
        efftime = None

        if "inttime" in self.specs.columns and np.count_nonzero(hi_mask) >= 3:
            inttime = self.specs["inttime"].to_numpy(dtype=np.float64)
            inttime_hi = inttime[hi_mask]

            # variance across channels (per spectrum, per band)
            # shape: (n_hi,8)
            var_hi = np.nanvar(cal_corr[hi_mask], axis=-1, ddof=1)

            # representative tsys per band (mean of Tsys across channels)
            tsys_band = np.nanmean(np.where(Tsys > 0, Tsys, np.nan), axis=-1)  # (n,8)
            tsys_band_hi = tsys_band[hi_mask]

            # channel spacing in Hz (per spectrum, per band)
            df_hz = np.nanmedian(np.abs(np.diff(fq_grid, axis=-1)), axis=-1)  # (n,8)
            df_hz_hi = df_hz[hi_mask]

            teff = np.zeros_like(var_hi)
            good = (var_hi > 0) & (df_hz_hi > 0) & np.isfinite(tsys_band_hi)
            teff[good] = (tsys_band_hi[good] ** 2 / var_hi[good]) / df_hz_hi[good]

            ratio = np.zeros_like(teff)
            ok_it = (inttime_hi[:, None] > 0) & np.isfinite(inttime_hi[:, None])
            ratio[good & ok_it] = teff[good & ok_it] / inttime_hi[:, None][good & ok_it]

            mean_ratio = np.nanmean(np.where(ratio > 0, ratio, np.nan), axis=0)  # (8,)
            if np.any(np.isfinite(mean_ratio)):
                eff = float(np.nanmax(mean_ratio))
            else:
                eff = 1.0

            efftime = inttime * eff * (eta_corr**2)

        # --- output ---
        df = pd.DataFrame(index=self.specs.index)
        df["calibrated"] = list(cal_corr)
        df["tspill"] = tspill
        df["eta_corr"] = eta_corr
        df["eff"] = eff
        if efftime is not None:
            df["efftime"] = efftime

        # Debug helpers (optional; comment out if too big)
        # df["Tsys"] = list(Tsys)
        return df


def J(T, f_hz):
    # T: scalar or array
    # f_hz: scalar or array broadcastable to T
    T0 = h * f_hz / k
    T = np.asarray(T, dtype=float)
    return np.where(T > 0, T0 / (np.expm1(T0 / T)), 0.0)
