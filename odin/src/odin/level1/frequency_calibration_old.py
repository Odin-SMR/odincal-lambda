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
        vgeo = self.attitude["vgeo"].loc[self.specs.index].to_numpy()[:, None, None]
        cols = "LO frequency " + self.ac.loc[self.specs.index]["frontend"]

        lo = self.housekeeping.loc[self.specs.index].to_numpy()[
            np.arange(len(self.housekeeping.loc[self.specs.index])),
            self.housekeeping.columns.get_indexer(cols.index),
        ][:, None, None]

        


        ssb_fq = np.repeat(
            np.stack((self.ac.ssb_fq[self.specs.index]).to_numpy())
               + np.array([-35,30,-10,10])[None, :],
            2,
            axis=1,
        )
        print(ssb_fq[0:2, :])
        # print(self.ac.ssb_att.head)
        rf = (ssb_fq[:, :, None]) * 1e6 + lo
    
        delta_f = 100e6 / 112
        if_sign = np.array([+1, -1, +1, -1, -1, +1, -1, +1])[None, :, None]
        grid = (np.arange(112) * delta_f * (if_sign) + rf) * (1.0 - vgeo / 2.99792458e8)
        df = pd.DataFrame(index=self.specs.index)
        df["frequency_grid"] = list(grid)
        df["lo"] = lo.squeeze()
        return df

    def tuning(self: HasFQCalData):

        if self.ac.frontend == '119':
            if_freq = 3900.0e6
            self.lofreq = 114.8498600000000e+9
            self.fcalibrate(self.lofreq, if_freq)
            return

        rxs = {'495': 1, '549': 2, '555': 3, '572': 4}
        if self.frontend not in rxs.keys():
            return
        (if_freq, sbpath) = get_sideband(
            self.frontend, self.lofreq, self.ssb
        )
        self.sbpath = sbpath / 1.0e6
        (adc_split, adc_upper) = (0x0200, 0x0400)
        if self.intmode & adc_split:
            if self.backend == 'AC1':
                if self.intmode & adc_upper:
                    if_freq = if_freq * 3.6 / 3.9
                else:
                    if_freq = if_freq * 4.2 / 3.9
            elif self.backend == 'AC2':
                if self.intmode & adc_upper:
                    if_freq = if_freq * 4.2 / 3.9
                else:
                    if_freq = if_freq * 3.6 / 3.9

        if self.current < 0.19 and self.frontend != '572':
            self.lofreq = 0.0

        self.fcalibrate(self.lofreq, if_freq)
