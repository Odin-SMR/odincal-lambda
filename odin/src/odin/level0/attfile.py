from datetime import datetime, timezone
from pathlib import Path

import numpy as np
import pandas as pd
from numpy.lib import recfunctions as rfn

from odin.db_ops import upsert_df

from .filetypes import att_dt


class AttFile:
    """Reader for ASCII attitude files.

    The files consist of a header that ends with a line containing
    the string "EOF". All following lines are expected to be
    tab-separated numeric columns, which are read into a NumPy array.
    """

    def __init__(self, file: Path):
        self._file = file
        self._created = datetime.now(timezone.utc)

        with file.open("r") as fh:
            soda_line = fh.readline()
            self._soda_version = soda_line.rsplit()[-1]
            for line in fh:
                if "EOF" in line:
                    break
            for line in fh:
                if "c\n" in line:
                    break
            data = np.loadtxt(
                fh,
                encoding="utf8",
                dtype=att_dt,
                converters={
                    30: lambda x: int(x, 16),
                },
            )

        self._data = data

    @property
    def soda_version(self) -> str:
        return self._soda_version

    @property
    def flat(self):
        return rfn.structured_to_unstructured(self._data)

    @property
    def data(self) -> np.ndarray:
        return self._data

    @property
    def dataframe(self) -> pd.DataFrame:
        df = pd.DataFrame()
        date = self._data["ut"]["date"].astype(str)
        day = np.array([np.datetime64(f"{d[:4]}-{d[4:6]}-{d[6:]}") for d in date])
        ms = (
            self._data["ut"]["hour"].astype(np.float64) * 3600
            + self._data["ut"]["minute"].astype(np.float64) * 60
            + self._data["ut"]["second"].astype(np.float64)
        ) * 1000
        df["datetime"] = pd.to_datetime(
            day.astype("datetime64[ms]") + ms.astype("timedelta64[ms]")
        ).tz_localize("UTC")
        df["stw"] = self._data["stw"]
        df["orbit"] = self._data["orbit"]
        df["qt"] = self._data["qt"].tolist()
        df["qa"] = self._data["qa"].tolist()
        df["qe"] = self._data["qe"].tolist()
        df["gps"] = self._data["gps"].tolist()
        df["os_pos"] = self._data["pos_os"].tolist()
        df["smr_pos"] = self._data["pos_smr"].tolist()
        df["sci"] = self._data["sci"]
        df["mode"] = self._data["mode"]
        df["acs"] = self._data["acs"].tolist()
        return df.set_index("stw")

    def to_parquet(self, filename: Path | None = None) -> None:
        if filename is None:
            filename = self._file
        df = self.dataframe
        df["prefix"] = np.char.mod("%03X", df.index.to_numpy() >> 24)
        df["created"] = self._created
        for name, df_group in df.groupby("prefix"):
            fname = f"s3://odin-level0/att/{self.soda_version}/{name}/{self._file.stem}.parquet"
            df_group.drop(columns=["prefix"]).to_parquet(fname)

    def to_db(self) -> None:
        df = pd.DataFrame()
        df["stw"] = self._data["stw"]
        df["soda"] = int(float(self._soda_version))
        df["year"] = self._data["ut"]["date"] // 10000
        df["mon"] = (self._data["ut"]["date"] % 10000) // 100
        df["day"] = self._data["ut"]["date"] % 100
        df["hour"] = self._data["ut"]["hour"]
        df["min"] = self._data["ut"]["minute"]
        df["secs"] = self._data["ut"]["second"]
        df["orbit"] = self._data["orbit"]
        df["qt"] = self._data["qt"].tolist()
        df["qa"] = self._data["qa"].tolist()
        df["qe"] = self._data["qe"].tolist()
        df["gps"] = self._data["gps"].tolist()
        df["acs"] = self._data["acs"]
        df["file"] = self._file.name
        df["created"] = self._created
        upsert_df(
            "host=postgres dbname=odin user=odin password=odin",
            df,
            "attitude_level0",
            ["stw", "soda"],
            array_cols=["qt", "qa", "qe", "gps"],
        )
