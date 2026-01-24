from __future__ import annotations
import pandas as pd
import numpy as np
from typing import Protocol
from math import pi, exp, sqrt


class HasACData(Protocol):
    ac: pd.DataFrame


class ReducerMixin:
    def reduce(self: HasACData) -> pd.DataFrame:
        l1a = Level1a()
        df = pd.DataFrame()
        df["stw"] = self.ac.index
        spectra = []
        for row_cc, row_mon in self.ac[["cc", "mon"]].itertuples(index=False):
            l1a.reduceAC(
                row_cc,
                row_mon[:, 0],
                row_mon[:, 1],
            )
            sp = np.stack(l1a.got)
            spectra.append(sp)
        df["spectra"] = list(np.stack(spectra))
        return df.set_index("stw")


class Level1a:
    """A class to process level 0 files into level 1a."""

    def __init__(self, old: bool = False):
        self.LAGSPERCHIP = 96
        self.CLOCKFREQ = 224.0e6
        self.SAMPLEFREQ = 10.0e6
        self.old = old

    def reduceAC(self, cc_data, acd_mon_pos, acd_mon_neg):
        self.got = []
        for i in range(len(cc_data)):
            self.maxchips = len(cc_data[i]) // 96
            self.nred = self.maxchips * 112
            datamod = np.zeros((1, self.nred))
            datamod[0, 0 : self.maxchips * self.LAGSPERCHIP] = cc_data[i]
            goti = self.reduce1Band(datamod[0, :], acd_mon_pos[i], acd_mon_neg[i])
            if goti[0] == 1:
                self.got.append(goti[1])
            else:
                self.got.append(np.zeros(shape=(self.nred,)))

    def reduce1Band(self, data, monitor_pos, monitor_neg):
        zlag = data[0]
        if zlag <= 0:
            return 0, 0
        if zlag > 1:
            return 0, 0
        power = zeroLag(zlag, 1.0)
        c_pos = threshold(monitor_pos)
        c_neg = threshold(monitor_neg)
        if c_pos == 0 or c_neg == 0:
            print("zero monitor value")
            return 0, 0
        else:
            cmean = (c_pos + c_neg) / 2.0
            dc = abs((c_pos - c_neg) / 2.0 / cmean)
            if dc > 0.1:
                print("too high monitor difference")
                return 0, 0
        data = qCorrect(cmean, data, self.nred)
        if data[0] == 0:
            print("quantisation correction failed")
            return 0, 0

        hanning_inplace(data[1])
        # data0 = np.array(data[1], dtype=np.float64, copy=True)
        data0 = odinfft_numpy_no_hann(data[1])

        data0 = data0 * power
        return 1, np.stack(data0)


def odinfft_numpy_no_hann(lags: np.ndarray) -> np.ndarray:
    """
    NumPy version of odinfft() with the Hanning step removed.

    Input:
      lags: shape (n,), float64-ish, where n = 112 * maxchips and n % 7 == 0

    Output:
      out: shape (n,), float64
           spectrum-like values packed into 7 blocks of size n/7,
           mimicking the final rearrangement in odinfft().
    """
    x = np.asarray(lags, dtype=np.float64)
    n = x.size
    if n % 7 != 0:
        raise ValueError(f"n must be a multiple of 7, got n={n}")

    # 1) Even/symmetric expansion around x[0] (matches the comment in the C code)
    #    [x0, x1, ..., x(n-1), 0, x(n-1), ..., x1]  length = 2n
    x2 = np.concatenate([x, [0.0], x[:0:-1]])

    # 2) Real FFT of the even sequence -> should be (numerically) real for perfect even symmetry
    #    rfft gives bins 0..n (inclusive), so length n+1
    X = np.fft.rfft(x2)

    # For an even real sequence, imag part should be ~0; keep real part.
    # Drop the Nyquist bin so we have exactly n outputs (like odinfft overwrites n samples).
    bins = X.real[:n]  # shape (n,)

    return bins


def hanning_inplace(data):
    n = data.shape[0]
    i = np.arange(n)
    w = 0.5 + 0.5 * np.cos(np.pi * i / n)
    data *= w


# def blended_window(n: int, alpha: float) -> np.ndarray:
#     w = (n)
#     return 1.0 - alpha + alpha*w


def inv_erfc(z):
    p = [1.591863138, -2.442326820, 0.37153461]
    q = [1.467751692, -3.013136362, 1.00000000]
    x = 1.0 - z
    y = x * x - 0.5625
    y = x * (p[0] + (p[1] + p[2] * y) * y) / (q[0] + (q[1] + q[2] * y) * y)
    return y


def threshold(monitor):
    if monitor < 0.0 or monitor > 1.0:
        return 0.0
    thr = sqrt(2.0) * inv_erfc(2.0 * monitor)
    return thr


def qCorrect(c, f, n):
    # Perform quantisation correction using Kulkarni & Heiles approximation.
    # (taken from Kulkarni, S.R., Heiles, C., 1980, AJ, 85, 1413.
    A = (pi / 2.0) * exp(c * c)
    B = -A * A * A * (pow((c * c - 1), 2.0) / 6.0)
    f[0] = 1.0
    for i in range(1, n):
        fa = f[i]
        if abs(fa) > 0.86:
            # level too high in QCorrect
            return 0, 0
        f[i] = (A + B * fa * fa) * fa
    return 1, f


def zeroLag(zlag, v):
    if zlag >= 1.0 or zlag <= 0.0:
        return 0.0
    x = v / inv_erfc(zlag)
    return x * x / 2.0
