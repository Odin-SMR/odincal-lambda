import numpy as np
from .acfile import Attenuation, SSBfrequency, Frontend, Type, IntTime, Mode


def get_seq(
    mode,
) -> tuple[
    np.ndarray[tuple[int], np.dtype[np.integer]],
    list[range],
    np.ndarray[tuple[int], np.dtype[np.integer]],
]:
    """get the ac chip configuration from the mode parameter"""
    seq = np.zeros(16, dtype=int)
    ssb = [1, -1, 1, -1, -1, 1, -1, 1]
    mode = (mode << 1) | 1
    for i in range(8):
        if mode & 1:
            m = i
        seq[2 * m] = seq[2 * m] + 1
        mode >>= 1

    for i in range(8):
        if seq[2 * i]:
            if ssb[i] < 0:
                seq[2 * i + 1] = -1
            else:
                seq[2 * i + 1] = 1
        else:
            seq[2 * i + 1] = 0
    chips = []
    band_start = []
    band = 0
    """chips is a list of vectors"""
    """for example chips=[[0], [1, 2, 3, 4], [5, 6, 7]] gives
       that we observe three bands: the first is
       from a single chip, for second band chip 1,2,3,4 are cascaded,
       for third band chip 5,6,7 are cascaded"""
    for ind, se in enumerate(seq):
        if ind == band:
            band_start.append(ind // 2)
            chips.append(range(ind // 2, ind // 2 + se))
            band = ind + 2 * se
    band_start = np.array(band_start)

    return seq, chips, band_start


def getAC(
    head: np.ndarray[tuple[int], np.dtype[np.uint16]],
    cc: np.ndarray[tuple[int, int], np.dtype[np.uint16]],
    backend: str
) -> dict:
    """AC factory.
    reads a fileobject and creates a dictionary for easy insertation
    into a postgresdatabase. Uses Ohlbergs routines to read the files (ACfile)
    """

    CLOCKFREQ = 224.0e6
    cc64 = np.array(cc, dtype=np.int64)
    lags64 = np.array(head[50:58], dtype=np.int64)
    # combine lags and data to ensure validity of first value in
    # cc-channels
    zlags = np.left_shift(lags64, 4) + np.bitwise_and(cc64[:, 0], 0xF)
    zlags.shape = (8, 1)
    mode = Mode(head)
    seq, chips, band_start = get_seq(mode)

    for ind in range(8):
        if np.any(ind == band_start):
            cc64[ind, 0] = zlags[ind, 0]
            if cc64[ind, 2] > 0:
                # find potential underflow in third element of cc
                cc64[ind, 2] -= 65536
    # cc64[:,0]=zlags[:,0]
    # find potential underflow in third element of cc
    # mask = cc64[:,2]>0
    # cc64[mask,2]-=65536
    # scale
    intTime = IntTime(head)
    if intTime == 0:
        intTime = 9999
    cc64 = cc64 * 2048.0 * (1 / intTime) / (CLOCKFREQ / 2.0)
    mon = np.array(head[16:32], dtype="uint16")
    mon.shape = (8, 2)
    # find potential overflows/underflows in monitor values
    mon64 = np.bitwise_and(zlags, 0xF0000) + mon
    overflow_mask = np.abs(mon64 - zlags) > 0x8000
    mon64[overflow_mask & (mon64 > zlags)] -= 0x10000
    mon64[overflow_mask & (mon64 < zlags)] += 0x10000
    # scale
    mon64 = mon64 * 1024.0 * (1 / intTime) / (CLOCKFREQ / 2.0)
    prescaler = head[49]
    datadict = {
        "frontend": Frontend(head),
        "sig_type": Type(head, backend),
        "ssb_att": "{{{0},{1},{2},{3}}}".format(*Attenuation(head)),
        "ssb_fq": "{{{0},{1},{2},{3}}}".format(*SSBfrequency(head)),
        "prescaler": prescaler,
        "inttime": intTime,
        "mode": mode,
        "acd_mon": mon64,
        "cc": cc64,
    }
    return datadict
