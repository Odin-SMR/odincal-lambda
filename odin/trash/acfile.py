import numpy as np
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

def Frontend(words):
    frontend = ["549", "495", "572", "555", "SPL", "119"]
    input = words[36] >> 8 & 0x000F
    if input in range(1, 7):
        rx = frontend[input - 1]
    else:
        # print "(%08X) invalid input channel %d" % (self.stw, input)
        rx = None
    return rx

def Type(words, type:str):
    rx = Frontend(words)
    chop = words[8]
    type = "NAN"
    if chop == 0xAAAA:
        if rx == "495" or rx == "549":
            type = "REF"
        elif rx == "555" or rx == "572" or rx == "119":
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

def Chop(words, type:str):
    """This routine returns the phase of FBA data via
    the chopper wheel infromation contained in AC2"""
    if type != "AC2":
        raise TypeError
    chop = words[8]
    if chop == 0xAAAA:
        type = "SIG"
    else:
        type = "REF"
    return type

def CmdTime(words):
    tcmd = float(words[35] & 0xFF) / 16.0
    return tcmd

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

def Mode(words: np.ndarray[tuple[int], np.dtype[np.uint16]]) -> np.ndarray[tuple[int], np.dtype[np.uint16]]:
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

def ZeroLags(words):
    bands = Mode(words)
    zlag = [0.0] * bands
    scale = 2048.0 / (224.0e6 / 2.0)
    inttime = IntTime(words)
    if inttime > 0.0:
        for i in range(bands):
            # (block, offset) = divmod(i*96,64)
            # block = block+1
            zlag[i] = scale * float(words[50 + i] << 4) / inttime
    return zlag
