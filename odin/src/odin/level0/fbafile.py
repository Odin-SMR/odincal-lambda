from functools import cached_property
from pathlib import Path

import numpy as np
import pandas as pd

from odin.db_ops import upsert_df

from .filetypes import fba_dt
from .odinfile import Level0File


class FBAfile(Level0File):
    dtype = fba_dt
    sequence_mask = 0
    PHASE = np.array(["REF", "SK1", "CAL", "SK2"], dtype="<U3")
    FFFF = np.uint16(0xFFFF)

    def __init__(self, file: Path):
        super().__init__(file)

    def _type_from_words(
        self, words: np.ndarray[tuple[int, int, int], np.dtype[np.uint16]]
    ) -> np.ndarray[tuple[int], np.dtype[np.str_]]:
        w5 = words[..., 5]
        w6 = words[..., 6]

        mirror = np.where(w5 == self.FFFF, w6, w5)
        mirror = np.where(mirror == self.FFFF, np.uint16(0), mirror)

        idx = (mirror >> np.uint16(13)) & np.uint16(3)  # 0..3
        return self.PHASE[idx.astype(np.intp)]

    @cached_property
    def mech(self) -> np.ndarray[tuple[int], np.dtype[np.str_]]:
        mechtype = self._type_from_words(self.words)
        return mechtype

    @property
    def dataframe(self) -> pd.DataFrame:
        df = pd.DataFrame()
        df["stw"] = self.stw
        df["mech_type"] = self.mech
        return df.set_index("stw")

    def to_parquet(
        self,
    ) -> None:
        df = self.dataframe
        df["prefix"] = np.char.mod("%03X", df.index.to_numpy() >> 24)
        df["created"] = self._created
        for name, df_group in df.groupby("prefix"):
            fname = f"s3://odin-level0/fba/{name}/{self._file.stem}.parquet"
            df_group.drop(columns=["prefix"]).to_parquet(fname)

    def to_db(self) -> None:
        df = self.dataframe
        df["created"] = self._created
        df["file"] = self._file.name
        upsert_df(
            "host=postgres dbname=odin user=odin password=odin",
            df.reset_index(),
            "fba_level0",
            ["stw"],
        )
