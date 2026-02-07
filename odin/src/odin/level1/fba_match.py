from typing import Protocol

import pandas as pd


class HasACData(Protocol):
    ac: pd.DataFrame
    fba: pd.DataFrame


class FilterBufferMixin:
    def fba_match(self: HasACData) -> pd.DataFrame:
        """Match filter buffer data to AC data based on STW."""
        # do some interpolation / nearest-neighbor matching
        df = self.fba.reindex(
            self.ac.index,
            method="ffill",
        )
        return df
