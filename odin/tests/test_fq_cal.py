from odin.level1.frequency_calibration import get_sideband


def test_sideband():
    lo_freq = 114.84986e9
    ssb = 313.0e9

    if_freq, sbpath = get_sideband("495", lo_freq, ssb)
    print(if_freq, sbpath)
    raise AssertionError
