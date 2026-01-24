from typing import Any

import numpy as np

# if_mux: dict[int, str] = {1:"549", 2:"495", 3:"572", 4:"555", 5:"SPL", 6:"119"}
# backends: dict[int, str] = {0x8: "AC1", 0xB: "AC2"}
N_CHIPS = 8
PAD = np.uint8(255)
CLOCKFREQ = 224.0e6


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


def Attenuation(words):
    att = [0] * 4
    for i in range(4):
        att[i] = words[37 + i]
        # if att[i] <= 95:
        #     print "(%08X) SSB[%d] attenuation at maximum" % (self.stw,i)
        # elif att[i] >= 145:
        #     print "(%08X) SSB[%d] attenuation at minimum" % (self.stw,i)
    return att


def SSBfrequency(words):
    ssb = [0] * 4
    for i in range(4):
        ssb[i] = words[44 - i]
        # if ssb[i] < 3000 or ssb[i] > 5000:
        #     print "(%08X) SSB[%d] frequency out of range" % (self.stw,i)
    return ssb


def IntTime(words):
    prescaler = int(words[49])
    if prescaler >= 2 and prescaler <= 6:
        samples = int(0x0000FFFF & words[12])
        samples = samples << (14 - prescaler)
    else:
        # prescaler out of range
        samples = 0
    inttime = float(samples) / 10.0e6
    return inttime


def Frontend(words):
    frontend = ["549", "495", "572", "555", "SPL", "119"]
    input = words[36] >> 8 & 0x000F
    if input in range(1, 7):
        rx = frontend[input - 1]
    else:
        # print "(%08X) invalid input channel %d" % (self.stw, input)
        rx = None
    return rx


def Type(words, type):
    rx = Frontend(words)
    chop = words[8]
    type = "NAN"
    if chop == 0xAAAA:
        if rx == "495" or rx == "549":  # aside
            type = "REF"
        elif rx == "555" or rx == "572" or rx == "119":  # bside
            type = "SIG"
        elif rx == "SPL":
            if type == "AC1":
                type = "REF"
            else:
                type = "SIG"
    else:
        if rx == "495" or rx == "549":
            type = "SIG"
        elif rx == "555" or rx == "572" or rx == "119":
            type = "REF"
        elif rx == "SPL":
            if type == "AC1":
                type = "SIG"
            else:
                type = "REF"
    return type


def Mode(words):
    mode = words[35] >> 8 & 0x00FF
    # bands = 0
    # if mode == 0x7f or mode == 0xf7:
    #     bands = 8
    # elif mode == 0x2a or mode == 0xa2:
    #     bands = 4
    # elif mode == 0x08 or mode == 0x8a:
    #     bands = 2
    # elif mode == 0x00:
    #     bands = 1
    return mode


def getAC(
    stw: np.ndarray[tuple[int], np.dtype[np.uint64]],
    words: np.ndarray[tuple[int, int, int], np.dtype[np.uint16]],
) -> list[dict[str, Any]]:
    head = words[0, :, :]
    dd: list[dict[str, Any]] = []
    for i in range(words.shape[0]):
        cur_stw = stw[i]

        head = words[i, 0, :]
        data = words[i, 1:, :]
        cc = np.array(data, dtype='int16')
        cc.shape = (8, 96)
        cc64 = np.array(cc, dtype="int64")
        lags = np.array(head[50:58], dtype="uint16")
        lags64 = np.array(lags, dtype="int64")
        # combine lags and data to ensure validity of first value in
        # cc-channels
        zlags = np.left_shift(lags64, 4) + np.bitwise_and(cc64[:, 0], 0xF)
        if i == 0:
             print("old zlags", zlags)
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
        # if i == 0:
        #     print("lags", lags64)
        #     print("zlags", zlags)
        #     print("cc", cc64[:, 0:6])
        if IntTime(head) == 0:
            intTime = 9999
        else:
            intTime = IntTime(head)
        scaled_cc64 = cc64 * 2048.0 * (1 / intTime) / (CLOCKFREQ / 2.0)
        mon = np.array(head[16:32], dtype="uint16")
        mon.shape = (8, 2)
        # if i == 0:
        #     print("mon", mon)
        # find potential overflows/underflows in monitor values
        mon64 = np.bitwise_and(zlags, 0xF0000) + mon
        overflow_mask = np.abs(mon64 - zlags) > 0x8000
        mon64[overflow_mask & (mon64 > zlags)] -= 0x10000
        mon64[overflow_mask & (mon64 < zlags)] += 0x10000
        # scale
        if i == 0:
            print("old mon", mon64)
            print("cc", cc64[:, 0:6])
        scaled_mon64 = mon64 * 1024.0 * (1 / intTime) / (CLOCKFREQ / 2.0)
        prescaler = head[49]
        datadict = {
            "stw": cur_stw,
            "prescaler": prescaler,
            "inttime": intTime,
            "mode": mode,
            "u_mon": mon64,
            "mon": scaled_mon64,
            "u_cc": cc64,
            "cc": scaled_cc64,
        }
        dd.append(datadict)
    return dd
