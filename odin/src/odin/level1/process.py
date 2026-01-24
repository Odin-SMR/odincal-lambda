from odin.level1.reduce import ReducerMixin
import pandas as pd


class Level1(ReducerMixin):
    def __init__(
        self, ac: pd.DataFrame
    ):  # , att: pd.DataFrame, shk: pd.DataFrame, fba: pd.DataFrame):
        self.ac = ac
        # self.att = att
        # self.shk = shk
        # self.fba = fba
        self._spectra = self.reduce()

    @property
    def spectra(self) -> pd.DataFrame:
        return self._spectra
