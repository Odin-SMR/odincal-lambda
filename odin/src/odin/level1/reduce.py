from __future__ import annotations

from typing import Protocol

import numpy as np
import pandas as pd

LAGSPERCHIP = 96


class HasACData(Protocol):
    ac: pd.DataFrame
    housekeeping: pd.DataFrame


class ReducerMixin:
    def reduce(self: HasACData) -> pd.DataFrame:
        """Compute spectra for all AC rows in a vectorised fashion."""

        if self.ac.empty:
            # Preserve behaviour for an empty input frame.
            return pd.DataFrame(columns=["spectra"]).set_index(pd.Index([], name="stw"))

        # "cc" has shape (nbands, nlag) per row and "mon" has shape
        # (nbands, 2).  We stack them into dense arrays of shape
        # (nrows, nbands, ...).
        cc_all = np.stack(self.ac["cc"].to_numpy()) * (
            224e6 / self.housekeeping["adjusted_clock"].to_numpy()[:, None, None]
        )
        mon_all = np.stack(self.ac["mon"].to_numpy()) * (
            224e6 / self.housekeeping["adjusted_clock"].to_numpy()[:, None, None]
        )

        spectra = reduce_ac(
            cc_all,
            mon_all[:, :, 0],  # monitor_pos
            mon_all[:, :, 1],  # monitor_neg
        )
        # mask = (spectra == 0.0).all(axis=2,keepdims=True)  # shape (n, 8)
        # print(mask.shape, mask.dtype, spectra.shape, spectra.dtype)
        # mask = np.repeat(mask, 112, axis=2)
        # spectra[mask] = np.nan

        df = pd.DataFrame({"stw": self.ac.index})
        # Store one 2D spectrum per row, keeping the public interface
        # unchanged (a column of ndarray objects).
        df["spectra"] = list(spectra)
        return df.set_index("stw")


def reduce_ac(
    cc_data: np.ndarray,
    acd_mon_pos: np.ndarray,
    acd_mon_neg: np.ndarray,
) -> np.ndarray:
    """Vectorised AC reduction.

    Parameters
    ----------
    cc_data
        Correlation data, shape ``(nrows, nbands, nlag)``.
    acd_mon_pos, acd_mon_neg
        Monitor values, shape ``(nrows, nbands)``.

    Returns
    -------
    np.ndarray
        Spectra with shape ``(nrows, nbands, nred)`` where
        ``nred = 112 * maxchips`` and ``maxchips = nlag // LAGSPERCHIP``.
    """

    cc = np.asarray(cc_data, dtype=np.float64)
    mon_pos = np.asarray(acd_mon_pos, dtype=np.float64)
    mon_neg = np.asarray(acd_mon_neg, dtype=np.float64)

    nrows, nbands, nlag = cc.shape
    maxchips = nlag // LAGSPERCHIP
    nred = maxchips * 112

    # Pad from 96 lags per band to 112 lags as in the original
    # implementation.
    datamod = np.zeros((nrows, nbands, nred), dtype=np.float64)
    datamod[:, :, :nlag] = cc

    # Flatten rows and bands into a single "band index" to simplify
    # vectorised operations.
    nb = nrows * nbands
    data_flat = datamod.reshape(nb, nred)
    mon_pos_flat = mon_pos.reshape(nb)
    mon_neg_flat = mon_neg.reshape(nb)

    # --- zero-lag / monitor checks ---------------------------------

    zlag = data_flat[:, 0]
    mask_zlag = (zlag > 0.0) & (zlag <= 1.0)

    power = np.zeros(nb, dtype=np.float64)
    power[mask_zlag] = zero_lag(zlag[mask_zlag], 1.0)

    c_pos = threshold(mon_pos_flat)
    c_neg = threshold(mon_neg_flat)

    mask_mon = mask_zlag & (c_pos != 0.0) & (c_neg != 0.0)

    cmean = np.zeros(nb, dtype=np.float64)
    dc = np.zeros(nb, dtype=np.float64)
    idx_mon = np.nonzero(mask_mon)[0]
    if idx_mon.size > 0:
        cmean[idx_mon] = (c_pos[idx_mon] + c_neg[idx_mon]) / 2.0
        dc[idx_mon] = np.abs((c_pos[idx_mon] - c_neg[idx_mon]) / (2.0 * cmean[idx_mon]))

    mask_dc = mask_mon & (dc <= 0.1)

    # --- Quantisation correction (Kulkarni & Heiles) ---------------

    data_qc = np.zeros_like(data_flat)
    mask_q_input = mask_dc
    idx_q = np.nonzero(mask_q_input)[0]
    good_q = np.zeros(nb, dtype=bool)
    if idx_q.size > 0:
        band_good, out_q = q_correct(cmean[idx_q], data_flat[idx_q])
        # Map results back into the full-band index space.
        data_qc[idx_q] = out_q
        good_q[idx_q[band_good]] = True

    valid = good_q

    # --- Hanning window and FFT ------------------------------------

    spectra_flat = np.zeros_like(data_flat)
    idx_valid = np.nonzero(valid)[0]
    if idx_valid.size > 0:
        lags = data_qc[idx_valid]

        # Hanning window.
        n = nred
        i = np.arange(n, dtype=np.float64)
        w = 0.5 + 0.5 * np.cos(np.pi * i / n)
        lags *= w

        # FFT
        x = lags
        x2 = np.concatenate(
            [x, np.zeros((x.shape[0], 1)), x[:, :0:-1]],
            axis=1,
        )
        X = np.fft.rfft(x2, axis=1)
        bins = X.real[:, :nred]

        bins *= power[idx_valid][:, None]
        spectra_flat[idx_valid] = bins

    return spectra_flat.reshape(nrows, nbands, nred)


def inv_erfc(z):
    p = [1.591863138, -2.442326820, 0.37153461]
    q = [1.467751692, -3.013136362, 1.00000000]
    x = 1.0 - z
    y = x * x - 0.5625
    y = x * (p[0] + (p[1] + p[2] * y) * y) / (q[0] + (q[1] + q[2] * y) * y)
    return y


def threshold(monitor: np.ndarray) -> np.ndarray:
    """Vectorised version of :func:`threshold`.

    Values outside the valid interval ``[0, 1]`` yield 0, matching the
    scalar behaviour.
    """

    monitor = np.asarray(monitor, dtype=np.float64)
    thr = np.zeros_like(monitor)
    valid = (monitor >= 0.0) & (monitor <= 1.0)
    thr[valid] = np.sqrt(2.0) * inv_erfc(2.0 * monitor[valid])
    return thr


def zero_lag(zlag: np.ndarray, v: float) -> np.ndarray:
    """Vectorised analogue of :func:`zeroLag` for array inputs."""

    zlag = np.asarray(zlag, dtype=np.float64)
    out = np.zeros_like(zlag)
    valid = (zlag < 1.0) & (zlag > 0.0)
    if not np.any(valid):
        return out
    x = v / inv_erfc(zlag[valid])
    out[valid] = x * x / 2.0
    return out


def q_correct(c: np.ndarray, f: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """Vectorised quantisation correction.

    Parameters
    ----------
    c
        Mean monitor values per band, shape ``(nbands,)``.
    f
        Input lags per band, shape ``(nbands, nred)``.

    Returns
    -------
    tuple[np.ndarray, np.ndarray]
        ``(ok, corrected)`` where ``ok`` has shape ``(nbands,)`` and is
        ``True`` for bands that pass the Kulkarni & Heiles constraint
        (no lag with ``|fa| > 0.86``), and ``corrected`` contains the
        corrected lags (with ``f[:, 0]`` set to 1.0).
    """

    c = np.asarray(c, dtype=np.float64)
    f = np.asarray(f, dtype=np.float64)

    nbands, nred = f.shape

    A = (np.pi / 2.0) * np.exp(c * c)
    B = -A * A * A * ((c * c - 1.0) ** 2.0 / 6.0)

    out = np.zeros_like(f)
    out[:, 0] = 1.0

    fa = f[:, 1:]
    bad = np.abs(fa) > 0.86
    ok = ~np.any(bad, axis=1)

    if np.any(ok):
        idx = np.nonzero(ok)[0]
        A_ok = A[idx][:, None]
        B_ok = B[idx][:, None]
        fa_ok = fa[idx]
        out_ok = (A_ok + B_ok * fa_ok * fa_ok) * fa_ok
        out[idx, 1:] = out_ok

    return ok, out
