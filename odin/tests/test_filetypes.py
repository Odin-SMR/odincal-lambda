from odin.level0.filetypes import ac1_dt, ac2_dt, shk_dt, os_dt, aos_dt, fba_dt

def test_block_lengths():
    # According to icd 4.2.4
    assert ac1_dt.itemsize == 75*2
    assert ac2_dt.itemsize == 75*2
    assert shk_dt.itemsize == 75*2
    assert os_dt.itemsize == 165*2
    assert aos_dt.itemsize == 120*2
    assert fba_dt.itemsize == 15*2
