from pathlib import Path

import numpy as np
import pytest

from odin.level0.acfile import ACfile
from odin.level0.attfile import AttFile
from odin.level0.fbafile import FBAfile
from odin.level0.shkfile import SHKfile
from odin.level1.process import Level1


@pytest.fixture(scope="module")
def level1():
    ac = ACfile(Path("odin/data/1a5d72de.ac1")).dataframe.loc[7077401291:7077404138]
    att = AttFile(Path("odin/data/1a5d72e0.att")).dataframe
    shk = SHKfile(Path("odin/data/1a5d72e1.shk")).dataframe
    fba = FBAfile(Path("odin/data/1a5d72df.fba")).dataframe
    return Level1(ac, att, shk, fba)


def test_level1_process(level1):
    sp = np.stack(level1.spectra["spectra"].to_numpy())
    assert sp.shape == (len(level1.ac), 8, 112), f"Unexpected spectra shape: {sp.shape}"
    assert sp[:, 0, :].sum() == 0, "This file has only data in bands 2 and 3."
    assert sp[:, 1, 50:70].max() == sp.max(), "Major peak not in expected location."


