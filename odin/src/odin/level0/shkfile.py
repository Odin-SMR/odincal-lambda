from collections.abc import Callable
from functools import cached_property
from pathlib import Path

import numpy as np
import pandas as pd

from odin.db_ops import upsert_df

from .filetypes import shk_dt
from .odinfile import Level0File

SHK_DB_NAMES: dict[str, str] = {
    "mixer current 495": "mixC495",
    "mixer current 549": "mixC549",
    "mixer current 555": "mixC555",
    "mixer current 572": "mixC572",
    "image load B-side": "imageloadB",
    "image load A-side": "imageloadA",
    "hot load A-side": "hotloadA",
    "hot load B-side": "hotloadB",
    "mixer A-side": "mixerA",
    "mixer B-side": "mixerB",
    "LNA A-side": "lnaA",
    "LNA B-side": "lnaB",
    "119GHz mixer A-side": "119mixerA",
    "119GHz mixer B-side": "119mixerB",
    "warm IF A-side": "warmifA",
    "warm IF B-side": "warmifB",
    "LO frequency 555": "LO555",
    "LO frequency 549": "LO549",
    "LO frequency 572": "LO572",
    "LO frequency 495": "LO495",
    "SSB549": "SSB549",
    "SSB555": "SSB555",
    "SSB572": "SSB572",
    "SSB495": "SSB495",
}

SHK_VALUES: dict[
    str,
    tuple[
        int,
        int,
        Callable[
            [pd.Series],
            pd.Series,
        ],
    ],
] = {
    # Name: (word index, subid, conversion function)
    "AOS laser temperature": (1, 0, lambda x: 0.01141 * x + 7.3),
    "AOS laser current": (1, 1, lambda x: 0.0388 * x),
    "AOS structure": (1, 2, lambda x: 0.01167 * x + 0.764),
    "AOS continuum": (1, 3, lambda x: (5.7718e-6 * x + 6.929e-3) * x - 2.1),
    "AOS processor": (1, 4, lambda x: 0.01637 * x - 6.54),
    "varactor 495": (17, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0),
    "varactor 549": (18, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0),
    "varactor 572": (25, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0),
    "varactor 555": (26, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0),
    "gunn 495": (17, 1, lambda x: 80.0 * 5.0 * x / 4095.0),
    "gunn 549": (18, 1, lambda x: 80.0 * 5.0 * x / 4095.0),
    "gunn 572": (25, 1, lambda x: 80.0 * 5.0 * x / 4095.0),
    "gunn 555": (26, 1, lambda x: 80.0 * 5.0 * x / 4095.0),
    "harmonic mixer 495": (17, 2, lambda x: 1.2195 * 5.0 * x / 4095.0),
    "harmonic mixer 549": (18, 2, lambda x: 1.2195 * 5.0 * x / 4095.0),
    "harmonic mixer 572": (25, 2, lambda x: 1.2195 * 5.0 * x / 4095.0),
    "harmonic mixer 555": (26, 2, lambda x: 1.2195 * 5.0 * x / 4095.0),
    "doubler 495": (17, 3, lambda x: -1.6129 * 5.0 * x / 4095.0),
    "doubler 549": (18, 3, lambda x: -1.6129 * 5.0 * x / 4095.0),
    "doubler 572": (25, 3, lambda x: -1.6129 * 5.0 * x / 4095.0),
    "doubler 555": (26, 3, lambda x: -1.6129 * 5.0 * x / 4095.0),
    "tripler 495": (17, 4, lambda x: -1.2195 * 5.0 * x / 4095.0),
    "tripler 549": (18, 4, lambda x: -1.2195 * 5.0 * x / 4095.0),
    "tripler 572": (25, 4, lambda x: -1.2195 * 5.0 * x / 4095.0),
    "tripler 555": (26, 4, lambda x: -1.2195 * 5.0 * x / 4095.0),
    "mixer current 495": (19, 0, lambda x: 5.0 * x / 4095.0 / 1.22),
    "mixer current 549": (19, 1, lambda x: 5.0 * x / 4095.0 / 1.22),
    "mixer current 572": (27, 0, lambda x: 5.0 * x / 4095.0 / 1.22),
    "mixer current 555": (27, 1, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 1 bias 495": (19, 2, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 1 bias 549": (19, 4, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 1 bias 572": (27, 2, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 1 bias 555": (27, 4, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 2 bias 495": (19, 3, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 2 bias 549": (19, 5, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 2 bias 572": (27, 3, lambda x: 5.0 * x / 4095.0 / 1.22),
    "HEMT 2 bias 555": (27, 5, lambda x: 5.0 * x / 4095.0 / 1.22),
    "warm IF A-side": (20, 0, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "warm IF B-side": (28, 0, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "hot load A-side": (20, 1, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "hot load B-side": (28, 1, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "image load A-side": (20, 2, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "image load B-side": (28, 2, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)),
    "mixer A-side": (20, 3, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "mixer B-side": (28, 3, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "LNA A-side": (20, 4, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "LNA B-side": (28, 4, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "119GHz mixer A-side": (20, 5, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "119GHz mixer B-side": (28, 5, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15),
    "HRO frequency 495": (21, 0, lambda x: x + 4000.0),
    "HRO frequency 549": (21, 2, lambda x: x + 4000.0),
    "HRO frequency 572": (29, 0, lambda x: x + 4000.0),
    "HRO frequency 555": (29, 2, lambda x: x + 4000.0),
    "PRO frequency 495": (21, 1, lambda x: x / 32.0 + 100.0),
    "PRO frequency 549": (21, 3, lambda x: x / 32.0 + 100.0),
    "PRO frequency 572": (29, 1, lambda x: x / 32.0 + 100.0),
    "PRO frequency 555": (29, 3, lambda x: x / 32.0 + 100.0),
    "LO mechanism A 495": (37, 0, lambda x: 310.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism A 549": (38, 0, lambda x: 288.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism A 572": (37, 2, lambda x: 312.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism A 555": (38, 2, lambda x: 310.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism B 572": (43, 0, lambda x: 312.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism B 555": (44, 0, lambda x: 310.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism B 495": (43, 2, lambda x: 310.0 * (-1 + 2 * x / 4095.0)),
    "LO mechanism B 549": (44, 2, lambda x: 288.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 495": (35, 0, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 495 Ref": (35, 1, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 549": (36, 0, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 549 Ref": (36, 1, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 572": (35, 2, lambda x: 302.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 572 Ref": (35, 3, lambda x: 302.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 555": (36, 2, lambda x: 308.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism A 555 Ref": (36, 3, lambda x: 308.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 495": (41, 2, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 495 Ref": (41, 3, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 549": (42, 2, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 549 Ref": (42, 3, lambda x: 313.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 572": (41, 0, lambda x: 302.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 572 Ref": (41, 1, lambda x: 302.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 555": (42, 0, lambda x: 308.0 * (-1 + 2 * x / 4095.0)),
    "SSB mechanism B 555 Ref": (42, 1, lambda x: 308.0 * (-1 + 2 * x / 4095.0)),
    "119GHz voltage": (46, 4, lambda x: -56.0 + x * 112.0 / 4095.0),
    "119GHz current": (46, 12, lambda x: -1091.0 + x * 2178.0 / 4095.0),
    # -1 prevously defined as all subids not aplicable in this implementation
    # TODO: fix these two entries once the proper conversion is known
    # "ACDC1 sync": (47, -1, lambda x: (x.astype("UInt16") >> np.uint16(8)) & 0x000F),
    # "ACDC2 sync": (48, -1, lambda x: (x.astype("UInt16") >> np.uint16(3)) & 0x000F),
    "119GHz DRO": (
        13,
        1,
        lambda x: 944.035 - (0.8374 - (2.567e-4 - 2.74e-8 * x) * x) * x,
    ),
    "ACS availability": (49, 13, lambda x: x),
    "acdc1 program": (55, 2, lambda x: x),
    "acdc2 program": (56, 2, lambda x: x),
    "acdc1 msg": (55, 0, lambda x: x),
    "acdc2 msg": (56, 0, lambda x: x),
}


class SHKfile(Level0File):
    dtype = shk_dt
    sequence_mask = 0

    def __init__(self, file: Path):
        super().__init__(file)

    def get_values(self, word: int, subid: int) -> pd.Series:
        mask = (self.subid[:, :, word] == subid).squeeze()
        values = (self.values[:, :, word]).squeeze()
        series = pd.Series(values, index=self.stw, dtype="UInt16")
        series[~mask] = pd.NA
        return series

    @cached_property
    def subid(self) -> np.ndarray[tuple[int, int, int], np.dtype[np.uint16]]:
        return self.words & np.uint16(0x000F)

    @cached_property
    def values(self) -> np.ndarray[tuple[int, int, int], np.dtype[np.uint16]]:
        return self.words >> np.uint16(4)

    @property
    def dataframe(self) -> pd.DataFrame:
        df = pd.DataFrame(index=self.stw)
        df.index.name = "stw"
        for name, (word, subid, func) in SHK_VALUES.items():
            vals = self.get_values(word=word, subid=subid).astype(np.float32)
            df[name] = func(vals)
        # save sync words without conversion
        df["ACDC1 Trigger"] = self.words[:, :, 47].squeeze() >> 3 & 0xF
        df["ACDC2 Trigger"] = self.words[:, :, 48].squeeze() >> 3 & 0xF
        df["ACDC1 valid"] = self.words[:, :, 47].squeeze() & 0x5
        df["ACDC2 valid"] = self.words[:, :, 48].squeeze()& 0x5

        df["Calibration A"] = (self.words[:, :, 33] >> 13 & 0x3).squeeze()
        df["Calibration B"] = (self.words[:, :, 39] >> 13 & 0x3).squeeze()
        return df

    def to_parquet(self, filename: Path | None = None) -> None:
        if filename is None:
            filename = self._file
        df = self.dataframe
        df["prefix"] = np.char.mod("%03X", df.index.to_numpy() >> 24)
        df["created"] = self._created
        for name, df_group in df.groupby("prefix"):
            fname = f"s3://odin-level0/shk/{name}/{self._file.stem}.parquet"
            df_group.drop(columns=["prefix"]).to_parquet(fname)

    def to_db(self) -> None:
        with pd.option_context("display.max_columns", None):
            df = self.dataframe
            df["SSB549"] = df["SSB mechanism A 549"].fillna(df["SSB mechanism B 549"])
            df["SSB555"] = df["SSB mechanism A 555"].fillna(df["SSB mechanism B 555"])
            df["SSB572"] = df["SSB mechanism A 572"].fillna(df["SSB mechanism B 572"])
            df["SSB495"] = df["SSB mechanism A 495"].fillna(df["SSB mechanism B 495"])
            df_fix = df[SHK_DB_NAMES.keys()].reset_index()
            df_cleaned = df_fix.rename(columns=SHK_DB_NAMES).dropna(axis=0, how="all")
            df_narrow = (
                df_cleaned.melt(["stw"], var_name="shk_type", value_name="shk_value")
                .set_index("stw")
                .sort_index()
            )
            df_nonans = df_narrow.dropna().copy()
            df_nonans["file"] = self._file.name
            df_nonans["created"] = self._created
            upsert_df(
                "host=postgres dbname=odin user=odin password=odin",
                df_nonans.reset_index(),
                "shk_level0",
                ["stw", "shk_type"],
            )
