from pathlib import Path

import numpy as np
import pandas as pd

from .old_tools import getAC

from .filetypes import ac_dt
from .odinfile import Level0File

if_mux: dict[int, str] = {1:"549", 2:"495", 3:"572", 4:"555", 5:"SPL", 6:"119"}
backends: dict[int, str] = {0x8: "AC1", 0xB: "AC2"}
CLOCKFREQ = 224.0e6


class ACfile(Level0File):
    dtype = ac_dt
    sequence_mask = 0x000F

    def __init__(self, file: Path):
        super().__init__(file)

    def old(self) -> pd.DataFrame:
        ac = getAC(self.stw, self.words)
        df = pd.DataFrame(ac)
        return df.set_index("stw")

    def dataframe(self) -> pd.DataFrame:
        int_time = (
            self.words[:, 0, 12].astype(np.int32) << (14 - self.words[:, 0, 49])
        ) / 10.0e6
        bands = np.bitwise_count(((self.words[:, 0, 35] >> 8) & 0x00FF) | 0x0080)
        lags = self.words[:, 0, 50:58].view(np.int16).astype(np.int32) << 4
        # print("new lags", lags[0, :])

        cc = self.words[:, 1:, :].reshape((-1, 8, 96)).view(np.int16).astype(np.int32)
        # print("new cc", cc[0, :, 0:6])
        zlags = lags | (cc[:, :, 0] & 0xF)
        # print("new zlags", zlags[0, :])
        mon = self.words[:, 0, 16:32].astype(np.int32).reshape(-1, 8, 2)
        is_start = self.band_start()
        # print("is_start", is_start[0, :])
        cc[:, :, 0] = np.where(is_start, zlags, cc[:, :, 0])
        # print("new cc", cc[0, :, 0:6])

        #overflow correction
        k = np.rint((2*cc[:,:,0] -mon.sum(axis=2))/2**16).astype(np.int32)
        if np.any(k !=0):
            print("Applying overflow correction")
            mon_corr = (k[:, :, None] * 2**16)//2
            mon = mon + mon_corr

        # print("new mon", mon[0, :])
        scaled_cc = cc * 2048.0 * (1 / int_time[:, None, None]) / (CLOCKFREQ / 2.0)
        scaled_mon = mon * 1024.0 * (1 / int_time[:, None, None]) / (CLOCKFREQ / 2.0)

        df = pd.DataFrame({"stw": self.stw})

        df["u_mon"] = list(mon)
        df["u_cc"] = list(cc)
        df["mon"] = list(scaled_mon)
        df["cc"] = list(scaled_cc)
        df["bands"] = np.bitwise_count(((self.words[:, 0, 35] >> 8) & 0x00FF) | 0x0080)
        df["mode"] = (self.words[:, 0, 35] >> 8) & 0x00FF
        df["inttime"] = (
            self.words[:, 0, 12].astype(np.int32) << (14 - self.words[:, 0, 49])
        ) / 10.0e6
        df["ssb_fq"] = list(self.words[:, 0, (44, 43, 42, 41)])
        df["ssb_att"] = list(self.words[:, 0, 37:41])
        df["backend"] = np.fromiter(
            iter=(backends[k] for k in (self.backend >> 4) & 0xF), dtype="U3"
        )
        df["frontend"] = np.fromiter(
            (if_mux[k] for k in ((self.words[:, 0, 36] >> 8) & 0xF)), dtype="U3"
        )

        df["sig_type"] = self._sig_type()

        return df.set_index("stw")

    def _sig_type(self):
        backend = (self.backend >> 4) & 0xF
        chopper_pos = self.words[:, 0, 8]
        is_ac1 = backend == 0x8
        if_mux = (self.words[:, 0, 36] >> 8) & 0xF
        is_aside = (if_mux == 1) | (if_mux == 2) | ((if_mux == 5) & is_ac1)
        swap = chopper_pos != 0xAAAA
        is_ref = is_aside ^ swap
        return np.where(is_ref, "REF", "SIG")

    def band_start(self) -> np.ndarray:
        mode = ((self.words[:, 0, 35] >> 8) & 0x00FF).astype(np.uint8)

        # apply implicit MSB
        mask = mode | np.uint8(0x80)

        # unpack bits, MSB first → band 0..7
        is_start = np.unpackbits(mask[:, None], axis=1, bitorder='big')

        return is_start.astype(bool)

