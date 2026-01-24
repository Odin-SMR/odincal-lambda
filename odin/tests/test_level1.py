from odin.level0.acfile import ACfile
from odin.level1.process import Level1
from pathlib import Path
import numpy as np


def test_level1_process():
    ac = ACfile(Path("odin/data/1a5d72de.ac1")).dataframe().iloc[:2]
    l1 = Level1(ac)
    sp = np.stack(l1.spectra["spectra"].to_numpy())
    assert sp.shape == (len(ac), 8, 112)
    assert sp[:,0,:].sum() == 0
    assert sp[:,1,50:70].max() == sp.max()
