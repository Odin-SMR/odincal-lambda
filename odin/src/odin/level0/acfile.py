from pathlib import Path

import numpy as np
import pandas as pd

from .filetypes import ac_dt
from .odinfile import Level0File

if_mux: dict[int, str] = {1: "549", 2: "495", 3: "572", 4: "555", 5: "SPL", 6: "119"}
backends: dict[int, str] = {0x8: "AC1", 0xB: "AC2"}
CLOCKFREQ = 224.0e6


class ACfile(Level0File):
    dtype = ac_dt
    sequence_mask = 0x000F

    def __init__(self, file: Path):
        super().__init__(file)
        self._decode()

    @property
    def dataframe(self) -> pd.DataFrame:
        return self._to_dataframe()

    def _decode(self) -> None:
        chopper_pos = self.words[:, 0, 8]
        prescaler = self.words[:, 0, 49]
        int_time = (self.words[:, 0, 12].astype(np.int32) << (14 - prescaler)) / 10.0e6
        mode = (self.words[:, 0, 35] >> 8) & 0x00FF
        mode_with_bit = mode.astype(np.uint8) | np.uint8(0x80)
        # unpack bits, MSB first → band 0..7
        is_start_band = np.unpackbits(mode_with_bit[:, None], axis=1, bitorder="big")

        # unpack bits, MSB first → band 0..7

        # bands = np.bitwise_count(((self.words[:, 0, 35] >> 8) & 0x00FF) | 0x0080)
        lags = self.words[:, 0, 50:58].view(np.int16).astype(np.int32) << 4
        # print("new lags", lags[0, :])

        cc = self.words[:, 1:, :].reshape((-1, 8, 96)).view(np.int16).astype(np.int32)
        # print("new cc", cc[0, :, 0:6])
        zlags = lags | (cc[:, :, 0] & 0xF)
        # print("new zlags", zlags[0, :])
        mon = self.words[:, 0, 16:32].astype(np.int32).reshape(-1, 8, 2)
        # print("is_start", is_start[0, :])
        cc[:, :, 0] = np.where(is_start_band, zlags, cc[:, :, 0])
        # print("new cc", cc[0, :, 0:6])

        # overflow correction
        k = np.rint((2 * cc[:, :, 0] - mon.sum(axis=2)) / 2**16).astype(np.int32)
        if np.any(k != 0):
            print("Applying overflow correction")
            mon_corr = (k[:, :, None] * 2**16) // 2
            mon = mon + mon_corr

        # print("new mon", mon[0, :])
        scaled_cc = cc * 2048.0 * (1 / int_time[:, None, None]) / (CLOCKFREQ / 2.0)
        scaled_mon = mon * 1024.0 * (1 / int_time[:, None, None]) / (CLOCKFREQ / 2.0)
        self.mon = scaled_mon
        self.cc = scaled_cc
        self.inttime = int_time
        self.acdc1_mask = (self.words[:, 0, 32] >> 8) & 0x00FF
        self.acdc2_mask = self.words[:, 0, 32] & 0x00FF
        self.mecha_mask = (self.words[:, 0, 33] >> 8) & 0x00FF
        self.mechb_mask = self.words[:, 0, 33] & 0x00FF
        self.r119_mask = (self.words[:, 0, 34] >> 8) & 0x00FF
        self.bands = np.bitwise_count(((self.words[:, 0, 35] >> 8) & 0x00FF) | 0x0080)
        self.mode = (self.words[:, 0, 35] >> 8) & 0x00FF
        self.if_mux = (self.words[:, 0, 36] >> 8) & 0xF
        self.ssb_att = list(self.words[:, 0, 37:41])
        self.ssb_fq = list(self.words[:, 0, (44, 43, 42, 41)])
        self.user = self.words[:, 0, 45] & 0x000F
        self.s = (self.words[:, 0, 46] >> 8) & 0x00FF
        self.r = self.words[:, 0, 46] & 0x00FF

        self.is_aside = (
            (self.if_mux == 1)
            | (self.if_mux == 2)
            | ((self.if_mux == 5) & (self.user == 0x8))
        )
        swap = chopper_pos != 0xAAAA
        self.is_reference = self.is_aside ^ swap

    def _to_dataframe(self) -> pd.DataFrame:
        df = pd.DataFrame({"stw": self.stw})

        df["mon"] = list(self.mon)
        df["cc"] = list(self.cc)
        df["bands"] = self.bands
        df["mode"] = self.mode
        df["user"] = self.user

        df["inttime"] = self.inttime
        df["ssb_fq"] = list(self.ssb_fq)
        df["s"] = self.s
        df["r"] = self.r
        df["ssb_att"] = list(self.ssb_att)
        df["backend"] = np.fromiter(iter=(backends[k] for k in self.user), dtype="U3")
        df["frontend"] = np.fromiter((if_mux[k] for k in self.if_mux), dtype="U3")

        df["sig_type"] = np.where(self.is_reference, "REF", "SIG")
        df["side"] = np.where(self.is_aside, "A", "B")
        # df["target_index1"] = self.acdc1_mask
        # df["target_index2"] = self.acdc2_mask

        return df.set_index("stw")

    # def _sig_type(self):
    #     backend = (self.backend >> 4) & 0xF
    #     chopper_pos = self.words[:, 0, 8]
    #     is_ac1 = backend == 0x8
    #     if_mux = (self.words[:, 0, 36] >> 8) & 0xF
    #     is_aside = (if_mux == 1) | (if_mux == 2) | ((if_mux == 5) & is_ac1)
    #     swap = chopper_pos != 0xAAAA
    #     is_ref = is_aside ^ swap
    #     return np.where(is_ref, "REF", "SIG")
