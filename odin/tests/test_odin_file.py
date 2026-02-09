from pathlib import Path

import pandas as pd
import pytest

from odin.level0.acfile import ACfile
from odin.level0.attfile import AttFile
from odin.level0.fbafile import FBAfile
from odin.level0.shkfile import SHKfile


@pytest.mark.parametrize(
    "filename,size,backend",
    [
        ("./odin_cal/data/ac1/1a5/1a5e3ec1.ac1", 7910, 0x7380),
        ("./odin_cal/data/ac2/1a5/1a5e3ec1.ac2", 7908, 0x73B0),
    ],
)
def test_ac_file(filename, size, backend):
    with pd.option_context("display.max_columns", None):
        file = Path(filename)
        ac = ACfile(file)
        #ac.to_parquet()
        # assert False
        assert ac.stw.size == size
        assert ac.backend.size == size
        assert ac.words.shape == (size, 13, 64)
        assert ac._file.name == file.name
        assert (ac.backend == backend).all()


@pytest.mark.parametrize(
    "filename,size,soda_version",
    [
        ("./odin_cal/data/fba/1a5/1a5ed04b.fba", 5633, "21.00"),
    ],
)
def test_fba_file(filename, size, soda_version):
    file = Path(filename)
    fba = FBAfile(file)
    fba.to_parquet()
    fba.to_db()
    assert fba.stw.size == size
    assert fba.backend.size == size
    assert fba.words.shape == (size, 1, 7)
    assert fba._file.name == file.name
    assert (fba.backend == 0x73EC).all()


@pytest.mark.parametrize(
    "filename,size,soda_version",
    [
        ("./odin_cal/data/att/1a5/06edc55b.att", 3060, "21.00"),
        ("./odin_cal/data/att/1a5/1a5e4dff.att", 38578, "20.00"),
        ("./odin_cal/data/att/1a5/394f8a16.att", 54832, "21.00"),
        ("./odin_cal/data/att/1a5/39530677.att", 2224, "21.00"),
    ],
)
def test_att_file(filename, size, soda_version):
    file = Path(filename)
    att = AttFile(file)
    att.to_parquet()
    att.to_db()
    assert att.soda_version == soda_version
    assert att.data.shape == (size,)
    assert att.flat.shape == (size, 37)  # 37 columns
    assert (att.data["qt"]["q2"] == att.flat[:, 8]).all()


@pytest.mark.parametrize(
    "filename,size",
    [
        ("odin_cal/data/shk/1a5/1a5e8df8.shk", 4180),
    ],
)
def test_shk_file(filename, size):
    file = Path(filename)
    shk = SHKfile(file)
    # values = shk.get_values(word=19, subid=0)
    # print(values[:14])
    # df = shk.dataframe
    # clean = df.dropna(axis=0,how='all')
    # narrow = (clean.reset_index().melt(["stw"])).set_index("stw").sort_index()
    # print(narrow.dropna().iloc[:20])

    shk.to_db()

    shk.to_parquet()

    assert shk.data.shape == (size,)
