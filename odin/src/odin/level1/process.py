from __future__ import annotations

import numpy as np
import pandas as pd
from numpy.lib.stride_tricks import sliding_window_view

from odin.level1.att_match import AttMatchMixin
from odin.level1.fba_match import FilterBufferMixin
from odin.level1.frequency_calibration import FrequencyCalibrationMixin
from odin.level1.intensity_calibration import intensityCalibrationMixin
from odin.level1.reduce import ReducerMixin
from odin.level1.shk_match import HouseKeepingMixin


class Level1(
    ReducerMixin,
    AttMatchMixin,
    HouseKeepingMixin,
    FilterBufferMixin,
    FrequencyCalibrationMixin,
    intensityCalibrationMixin,
):
    def __init__(
        self,
        ac: pd.DataFrame,
        att: pd.DataFrame,
        shk: pd.DataFrame,
        fba: pd.DataFrame,
    ):
        ## raw data
        self.ac = ac
        self.att = att
        self.shk = shk
        self.fba = fba
        ## steps to calibrate the data
        self._attitude = self.att_match()
        self._housekeeping = self.shk_match()
        self._filter_buffer = self.fba_match()
        self._spectra = self.reduce()
        self.prepare()
        self._frequency_grid = self.frequency_calibration()
        self._calibrated = self.intensity_calibration()

    @property
    def calibrated(self) -> pd.DataFrame:
        return self._calibrated

    @property
    def spectra(self) -> pd.DataFrame:
        return self._spectra

    @property
    def attitude(self) -> pd.DataFrame:
        return self._attitude

    @property
    def housekeeping(self) -> pd.DataFrame:
        return self._housekeeping

    @property
    def filter_buffer(self) -> pd.DataFrame:
        return self._filter_buffer

    @property
    def frequency_grid(self) -> pd.DataFrame:
        return self._frequency_grid

    def prepare(self):
        # att8 = np.repeat(np.stack(self.ac.ssb_att.to_numpy()), 2, axis=1).astype(np.float32)
        # print(att8[0:2, :])
        # dB_per_lsb = 10.0 / 255.0
        # ref = np.max(att8, axis=1, keepdims=True)   # if max code = most attenuation
        # att8_dB = (att8 - ref) * dB_per_lsb
        # gain = 10 ** (att8_dB / 10.0)

        signal_mask = self.ac["sig_type"] == "SIG"
        reference_mask = (
            (self.filter_buffer["mech_type"] == "SK1")
            | (self.filter_buffer["mech_type"] == "SK2")
        ) & (self.ac["sig_type"] == "REF")

        hotload_mask = (self.filter_buffer["mech_type"] == "CAL") & (
            self.ac["sig_type"] == "REF"
        )
        # adj_power = gain[:, :, None] * np.stack(self.spectra["spectra"].to_numpy())

        # adj = pd.Series(
        #     list(adj_power), index=self.spectra.index, name="spectra"
        # )

        # sig = adj[signal_mask]
        # ref = adj[reference_mask]
        # hot = adj[hotload_mask]
        sig = self.spectra["spectra"][signal_mask]
        ref = self.spectra["spectra"][reference_mask]
        hot = self.spectra["spectra"][hotload_mask]
        print(sig.shape, ref.shape, hot.shape,signal_mask.shape)
        t_height = np.stack(self.attitude["smr_pos"].to_numpy())[signal_mask][:,2]

        W = 9 # smoothing window size in spectra (must be odd for median)
        # sig_spectrum = np.stack(sig.to_numpy())
        ref_spectrum = np.stack(ref.to_numpy())
        padded_ref = np.pad(
            ref_spectrum, ((W // 2, W // 2), (0, 0), (0, 0)), mode="edge"
        )
        window_ref = sliding_window_view(padded_ref, W, axis=0)
        med_ref = pd.Series(
            list(np.median(window_ref, axis=3)), index=ref.index, name="spectra"
        )
        med_ref.index.name = "stw"
        self.specs = pd.merge_asof(
            sig,
            med_ref,
            on="stw",
            suffixes=("_sig", "_ref"),
            direction="nearest",
            tolerance=64 * 4,
        ).set_index("stw")
        hot_median = np.median(np.stack(hot.to_numpy()), axis=0)
        hot_series = pd.Series(
            data=[hot_median] * len(self.specs),
            index=self.specs.index,
        )
        self.specs["hot"] = hot_series
        self.specs["t_height"] = t_height
