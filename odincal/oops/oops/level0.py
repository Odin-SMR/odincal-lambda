# pylint: skip-file
import sys
import struct

SSB_PARAMS = {
    '495': (61600.36, 104188.89, 0.0002977862, 313.0),
    '549': (57901.86, 109682.58, 0.0003117128, 313.0),
    '555': (60475.43, 116543.50, 0.0003021341, 308.0),
    '572': (58120.92, 115256.73, 0.0003128605, 314.0)
}


class Level0File:
    """ A Python class to handle Odin level 0 files """
    TypeError = "wrong file type"

    def __init__(self, file):
        self.name = file
        # Windows version needs 'rb' !!!
        self.input = open(file, 'r')
        _, stw, user = self.getHead()
        self.first = stw
        self.user = user
        sizeunit = 15 * struct.calcsize('H')
        if user == 0x732c:
            self.type = 'SHK'
            self.tail = 4
            self.blocksize = 5 * sizeunit
        elif user == 0x73ec:
            self.type = 'FBA'
            self.tail = 4
            self.blocksize = 1 * sizeunit
        elif (user & 0xfff0) == 0x7360:
            self.type = 'AOS'
            self.tail = 4
            self.blocksize = 8 * sizeunit
        elif (user & 0xfff0) == 0x7380:
            self.type = 'AC1'
            self.tail = 7
            self.blocksize = 5 * sizeunit
        elif (user & 0xfff0) == 0x73B0:
            self.type = 'AC2'
            self.tail = 7
            self.blocksize = 5 * sizeunit
        else:
            raise TypeError
        self.input.seek(-self.blocksize, 2)
        sync, stw, user = self.getHead()
        self.last = stw
        self.input.seek(0, 0)

    def getHead(self):
        data = self.input.read(struct.calcsize('H' * 4))
        head = struct.unpack('H' * 4, data)
        sync = head[0]
        stw = head[2] * 65536 + head[1]
        user = head[3]
        return sync, stw, user

    def getIndex(self):
        data = self.input.read(self.blocksize)
        if len(data) == self.blocksize:
            n = self.blocksize / 2
            words = struct.unpack('H' * n, data)
            self.sync = words[0]
            self.stw = words[2] * 65536 + words[1]
            self.user = words[3]
            index = words[-self.tail]
            valid = (index & 0x8000) != 0
            if valid:
                if index & 0x4000:
                    dis = 'ASTR'
                else:
                    dis = 'AERO'
            else:
                dis = None
            acdcmode = (index & 0x0f00) >> 8
            science = index & 0x00ff
            return (self.stw, valid, dis, acdcmode, science)
        else:
            return None

    def getBlock(self):
        data = self.input.read(self.blocksize)
        if len(data) == self.blocksize:
            n = self.blocksize / 2
            words = struct.unpack('H' * n, data)
            self.sync = words[0]
            self.stw = words[2] * 65536 + words[1]
            self.user = words[3]
            words = words[4:-self.tail]
        else:
            words = []
        return words

    def rewind(self):
        self.input.seek(0, 0)

    def __del__(self):
        # print "closing file", self.name
        self.input.close


HKdata = {
    'AOS laser temperature': [1, 0, lambda x: 0.01141 * x + 7.3],
    'AOS laser current': [1, 1, lambda x: 0.0388 * x],
    'AOS structure': [1, 2, lambda x: 0.01167 * x + 0.764],
    'AOS continuum': [1, 3, lambda x: (5.7718e-6 * x + 6.929e-3) * x - 2.1],
    'AOS processor': [1, 4, lambda x: 0.01637 * x - 6.54],
    'varactor 495': [17, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0],
    'varactor 549': [18, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0],
    'varactor 572': [25, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0],
    'varactor 555': [26, 0, lambda x: 4.8 * 5.0 * x / 4095.0 - 12.0],
    'gunn 495': [17, 1, lambda x: 80.0 * 5.0 * x / 4095.0],
    'gunn 549': [18, 1, lambda x: 80.0 * 5.0 * x / 4095.0],
    'gunn 572': [25, 1, lambda x: 80.0 * 5.0 * x / 4095.0],
    'gunn 555': [26, 1, lambda x: 80.0 * 5.0 * x / 4095.0],
    'harmonic mixer 495': [17, 2, lambda x: 1.2195 * 5.0 * x / 4095.0],
    'harmonic mixer 549': [18, 2, lambda x: 1.2195 * 5.0 * x / 4095.0],
    'harmonic mixer 572': [25, 2, lambda x: 1.2195 * 5.0 * x / 4095.0],
    'harmonic mixer 555': [26, 2, lambda x: 1.2195 * 5.0 * x / 4095.0],
    'doubler 495': [17, 3, lambda x: -1.6129 * 5.0 * x / 4095.0],
    'doubler 549': [18, 3, lambda x: -1.6129 * 5.0 * x / 4095.0],
    'doubler 572': [25, 3, lambda x: -1.6129 * 5.0 * x / 4095.0],
    'doubler 555': [26, 3, lambda x: -1.6129 * 5.0 * x / 4095.0],
    'tripler 495': [17, 4, lambda x: -1.2195 * 5.0 * x / 4095.0],
    'tripler 549': [18, 4, lambda x: -1.2195 * 5.0 * x / 4095.0],
    'tripler 572': [25, 4, lambda x: -1.2195 * 5.0 * x / 4095.0],
    'tripler 555': [26, 4, lambda x: -1.2195 * 5.0 * x / 4095.0],
    'mixer current 495': [19, 0, lambda x: 5.0 * x / 4095.0 / 1.22],
    'mixer current 549': [19, 1, lambda x: 5.0 * x / 4095.0 / 1.22],
    'mixer current 572': [27, 0, lambda x: 5.0 * x / 4095.0 / 1.22],
    'mixer current 555': [27, 1, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 1 bias 495': [19, 2, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 1 bias 549': [19, 4, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 1 bias 572': [27, 2, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 1 bias 555': [27, 4, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 2 bias 495': [19, 3, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 2 bias 549': [19, 5, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 2 bias 572': [27, 3, lambda x: 5.0 * x / 4095.0 / 1.22],
    'HEMT 2 bias 555': [27, 5, lambda x: 5.0 * x / 4095.0 / 1.22],
    'warm IF A-side': [20, 0, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'warm IF B-side': [28, 0, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'hot load A-side': [20, 1, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'hot load B-side': [28, 1, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'image load A-side': [20, 2, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'image load B-side': [28, 2, lambda x: 20.0 * (5.0 * x / 4095.0 - 1.16)],
    'mixer A-side': [20, 3, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    'mixer B-side': [28, 3, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    'LNA A-side': [20, 4, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    'LNA B-side': [28, 4, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    '119GHz mixer A-side': [20, 5, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    '119GHz mixer B-side': [28, 5, lambda x: 70.0 * (5.0 * x / 4095.0 - 3.86) + 273.15],
    'HRO frequency 495': [21, 0, lambda x: x + 4000.0],
    'HRO frequency 549': [21, 2, lambda x: x + 4000.0],
    'HRO frequency 572': [29, 0, lambda x: x + 4000.0],
    'HRO frequency 555': [29, 2, lambda x: x + 4000.0],
    'PRO frequency 495': [21, 1, lambda x: x / 32.0 + 100.0],
    'PRO frequency 549': [21, 3, lambda x: x / 32.0 + 100.0],
    'PRO frequency 572': [29, 1, lambda x: x / 32.0 + 100.0],
    'PRO frequency 555': [29, 3, lambda x: x / 32.0 + 100.0],
    'LO mechanism A 495': [37, 0, lambda x: x],
    'LO mechanism A 549': [38, 0, lambda x: x],
    'LO mechanism A 572': [37, 2, lambda x: x],
    'LO mechanism A 555': [38, 2, lambda x: x],
    'LO mechanism B 495': [43, 0, lambda x: x],
    'LO mechanism B 549': [44, 0, lambda x: x],
    'LO mechanism B 572': [43, 2, lambda x: x],
    'LO mechanism B 555': [44, 2, lambda x: x],
    'SSB mechanism A 495': [35, 0, lambda x: x],
    'SSB mechanism A 549': [36, 0, lambda x: x],
    'SSB mechanism A 572': [35, 2, lambda x: x],
    'SSB mechanism A 555': [36, 2, lambda x: x],
    'SSB mechanism B 495': [41, 0, lambda x: x],
    'SSB mechanism B 549': [42, 0, lambda x: x],
    'SSB mechanism B 572': [41, 2, lambda x: x],
    'SSB mechanism B 555': [42, 2, lambda x: x],
    '119GHz voltage': [46, 4, lambda x: -56.0 + x * 112.0 / 4095.0],
    '119GHz current': [46, 12, lambda x: -1091.0 + x * 2178.0 / 4095.0],
    'ACDC1 sync': [47, -1, lambda x: (int(x) >> 8) & 0x000f],
    'ACDC2 sync': [48, -1, lambda x: (x >> 3) & 0x000f],
    '119GHz DRO': [13, 1, lambda x: 944.035 - (0.8374 - (2.567e-4 - 2.74e-8 * x) * x) * x],
    'ACS availability': [49, 13, lambda x: x]
}


class SHKfile(Level0File):
    """ A derived  class to handle Odin level 0 SHK files """

    def __init__(self, file):
        # print "init SHK..."
        Level0File.__init__(self, file)
        if self.type != 'SHK':
            raise TypeError

    def getHKword(self, which, sub=-1):
        self.rewind()
        stw = []
        data = []
        words = self.getBlock()
        while words:
            word = words[which]
            found = (word != 0xffff)
            if sub > -1:
                found = ((word & 0x000f) == sub)
                word = word >> 4
            if found:
                stw.append(self.stw)
                data.append(word)
            words = self.getBlock()
        return stw, data

    def getLOfreqs(self):
        def sub(word):
            return word & 0x000f

        def freq(hro, pro, m):
            return ((4000.0 + hro) * m + pro / 32.0 + 100.0) * 6.0e6
        self.rewind()
        stw = []
        aside = []
        bside = []
        words = self.getBlock()
        while words:
            stw.append(self.stw)
            aside.append(int(words[21]))
            bside.append(int(words[29]))
            words = self.getBlock()

        i = 0
        STWa = []
        STWb = []
        LO495 = []
        LO549 = []
        LO555 = []
        LO572 = []
        while i < len(stw) - 3:
            if sub(aside[i]) == 0:
                STWa.append(stw[i])
                if sub(aside[i]) == 0 and sub(aside[i + 1]) == 1:
                    hro = aside[i] >> 4
                    pro = aside[i + 1] >> 4
                    LO495.append(freq(hro, pro, 17.0))
                else:
                    LO495.append(0.0)

                if sub(aside[i + 2]) == 2 and sub(aside[i + 3]) == 3:
                    hro = aside[i + 2] >> 4
                    pro = aside[i + 3] >> 4
                    LO549.append(freq(hro, pro, 19.0))
                else:
                    LO549.append(0.0)

            if sub(bside[i]) == 0:
                STWb.append(stw[i])
                if sub(bside[i]) == 0 and sub(bside[i + 1]) == 1:
                    hro = bside[i] >> 4
                    pro = bside[i + 1] >> 4
                    LO572.append(freq(hro, pro, 20.0))
                else:
                    LO572.append(0.0)

                # print "%04x %04x %04x %04x" % \
                #       (bside[i], bside[i+1], bside[i+2], bside[i+3])
                if sub(bside[i + 2]) == 2 and sub(bside[i + 3]) == 3:
                    hro = bside[i + 2] >> 4
                    pro = bside[i + 3] >> 4
                    LO555.append(freq(hro, pro, 19.0))
                    # print LO555[-1]
                else:
                    LO555.append(0.0)
                    #                i = i+4
                    #            else:
            i = i + 1

        return (STWa, LO495, LO549, STWb, LO555, LO572)

    def getSSBtunings(self):
        def sub(word):
            return word & 0x000f
        self.rewind()
        stw = []
        aside = []
        bside = []
        which = []
        words = self.getBlock()
        while words:
            stw.append(self.stw)
            if words[35] != 0xffff and words[36] != 0xffff:
                aside.append(words[35])
                bside.append(words[36])
                which.append('A')
            else:
                aside.append(words[41])
                bside.append(words[42])
                which.append('B')
            words = self.getBlock()

        i = 0
        STW = []
        SSB495 = []
        SSB549 = []
        SSB555 = []
        SSB572 = []
        while i < len(stw) - 2:
            if sub(aside[i]) == 0 and sub(bside[i]) == 0 \
               and sub(aside[i + 2]) == 2 and sub(bside[i + 2]) == 2:
                STW.append(stw[i])
                if which[i] == 'A':
                    SSB495.append(aside[i] >> 4)
                    SSB572.append(aside[i + 2] >> 4)
                    SSB549.append(bside[i] >> 4)
                    SSB555.append(bside[i + 2] >> 4)
                else:
                    SSB495.append(aside[i + 2] >> 4)
                    SSB572.append(aside[i] >> 4)
                    SSB549.append(bside[i + 2] >> 4)
                    SSB555.append(bside[i] >> 4)
                i = i + 2
            else:
                i = i + 1

        return (STW, SSB495, SSB549, SSB555, SSB572)


class FBAfile(Level0File):
    """ A derived  class to handle Odin level 0 FBA files """

    def __init__(self, file):
        Level0File.__init__(self, file)
        if self.type != 'FBA':
            raise TypeError
        self.block0 = 0x73ec

    def getSpectrumHead(self):
        words = self.getBlock()
        while words:
            if self.sync == 0x2bd3 and self.user == self.block0:
                return words
            words = self.getBlock()
        return None

    def Type(self, words):
        phase = ['REF', 'SK1', 'CAL', 'SK2']
        mirror = words[5]
        if mirror == 0xffff:
            mirror = words[6]
            if mirror == 0xffff:
                mirror = 0
        mirror = (mirror >> 13) & 3
        type = phase[mirror]
        return type


class ACfile(Level0File):
    """ A derived  class to handle Odin level 0 AC1 and AC2 files """

    def __init__(self, file):
        Level0File.__init__(self, file)
        if self.type != 'AC1' and self.type != 'AC2':
            raise TypeError
        if self.type == 'AC1':
            self.block0 = 0x7380
        elif self.type == 'AC2':
            self.block0 = 0x73b0

    def getSpectrumHead(self):
        words = self.getBlock()
        while words:
            if self.sync == 0x2bd3 and self.user == self.block0:
                return words
            words = self.getBlock()
        return None

    def Attenuation(self, words):
        att = [0] * 4
        for i in range(4):
            att[i] = words[37 + i]
            # if att[i] <= 95:
            #     print "(%08X) SSB[%d] attenuation at maximum" % (self.stw,i)
            # elif att[i] >= 145:
            #     print "(%08X) SSB[%d] attenuation at minimum" % (self.stw,i)
        return att

    def SSBfrequency(self, words):
        ssb = [0] * 4
        for i in range(4):
            ssb[i] = words[44 - i]
            # if ssb[i] < 3000 or ssb[i] > 5000:
            #     print "(%08X) SSB[%d] frequency out of range" % (self.stw,i)
        return ssb

    def Frontend(self, words):
        frontend = ['549', '495', '572', '555', 'SPL', '119']
        input = words[36] >> 8 & 0x000f
        if input in range(1, 7):
            rx = frontend[input - 1]
        else:
            # print "(%08X) invalid input channel %d" % (self.stw, input)
            rx = None
        return rx

    def Type(self, words):
        rx = self.Frontend(words)
        chop = words[8]
        type = 'NAN'
        if chop == 0xaaaa:
            if rx == '495' or rx == '549':
                type = 'REF'
            elif rx == '555' or rx == '572' or rx == '119':
                type = 'SIG'
            elif rx == 'SPL':
                if self.type == 'AC1':
                    type = 'REF'
                else:
                    type = 'SIG'
        else:
            if rx == '495' or rx == '549':
                type = 'SIG'
            elif rx == '555' or rx == '572' or rx == '119':
                type = 'REF'
            elif rx == 'SPL':
                if self.type == 'AC1':
                    type = 'SIG'
                else:
                    type = 'REF'
        return type

    def Chop(self, words):
        """ This routine returns the phase of FBA data via
        the chopper wheel infromation contained in AC2"""
        if self.type != 'AC2':
            raise TypeError
        chop = words[8]
        if chop == 0xaaaa:
            type = 'SIG'
        else:
            type = 'REF'
        return type

    def CmdTime(self, words):
        tcmd = float(words[35] & 0xff) / 16.0
        return tcmd

    def IntTime(self, words):
        prescaler = int(words[49])
        if prescaler >= 2 and prescaler <= 6:
            samples = long(0x0000ffff & words[12])
            samples = samples << (14 - prescaler)
        else:
            # prescaler out of range
            samples = 0
        inttime = float(samples) / 10.0e6
        return inttime

    def Mode(self, words):
        mode = words[35] >> 8 & 0x00ff
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

    def ZeroLags(self, words):
        bands = self.Mode(words)
        zlag = [0.0] * bands
        scale = 2048.0 / (224.0e6 / 2.0)
        inttime = self.IntTime(words)
        if inttime > 0.0:
            for i in range(bands):
                # (block, offset) = divmod(i*96,64)
                # block = block+1
                zlag[i] = scale * float(words[50 + i] << 4) / inttime
        return zlag


class AOSfile(Level0File):
    """ A derived  class to handle Odin level 0 AOS files """

    def __init__(self, file):
        Level0File.__init__(self, file)
        if self.type != 'AOS':
            raise TypeError
        self.block0 = 0x7360

    def getSpectrumHead(self):
        words = self.getBlock()
        while words:
            if self.sync == 0x2bd3 and self.user == self.block0:
                if words[1] != 322:
                    return words
            words = self.getBlock()
        return None

    def Frontend(self, words):
        input = words[30 + 2]
        if input == 1:
            rx = '555'
        elif input == 2:
            rx = '572'
        elif input == 4:
            rx = '495'
        elif input == 8:
            rx = '549'
        elif input == 16:
            rx = '119'
        else:
            rx = 'OFF'
        return rx

    def Type(self, words):
        if words[19]:
            aligned = words[8] & 0x0080
        elif words[20]:
            aligned = words[9] & 0x0080
        else:
            aligned = words[8] & 0x0080

        rx = self.Frontend(words)
        if aligned:
            if rx == '495' or rx == '549':
                type = 'SIG'
            else:
                type = 'REF'
        else:
            if rx == '495' or rx == '549':
                type = 'REF'
            else:
                type = 'SIG'

        if type == 'REF':
            calmirror = words[11] & 0x000f
            if calmirror == 1:
                type = 'SK1'
            elif calmirror == 2:
                type = 'CAL'
            elif calmirror == 3:
                type = 'SK2'
        if words[30 + 2] == 0:
            type = 'DRK'
        if words[30 + 4] == 1:
            type = 'CMB'
        return type

    def IntTime(self, words):
        samples = words[35]
        inttime = float(samples) * (1760.0 / 3.0e5)
        return inttime

    def Mode(self, words):
        mode = words[1]
        return mode


def listHKdata():
    print "available HK data:"
    keys = sorted(HKdata.keys())
    for key in keys:
        print "\t", key


def Lookup(table, stw0):
    i = 0
    while i < len(table[0]) - 1 and table[0][i + 1] < stw0:
        i = i + 1
    return table[1][i]


def removeDuplicates(stw, data):
    i = 1
    if len(stw) != len(data):
        raise RuntimeError
    while i < len(stw) - 1:
        if (data[i - 1] == data[i]) and (data[i] == data[i + 1]):
            del stw[i]
            del data[i]
        else:
            i = i + 1
    return stw, data


def getSideBand(rx, LO, ssb):
    d = 0.0
    C1 = SSB_PARAMS[rx][0]
    C2 = SSB_PARAMS[rx][1]
    sf = SSB_PARAMS[rx][2]
    sbpath = (-SSB_PARAMS[rx][3] + 2.0 *
              SSB_PARAMS[rx][3] * ssb / 4095.0) * 2.0
    for i in range(-2, 3):
        s3900 = 299.79 / (ssb + C1) * (C2 + i / sf) - LO / 1.0e9
        if abs(abs(s3900) - 3.9) < abs(abs(d) - 3.9):
            d = s3900
    if d < 0.0:
        IFfreq = -3900.0e6
    else:
        IFfreq = 3900.0e6
    return (IFfreq, sbpath)


def main():
    """main function"""
    from oops.odintime import TimeConverter

    if len(sys.argv) > 1:
        which = sys.argv[1]
        # print len(sys.argv)
        if which in HKdata:
            table = HKdata[which]
            # print table
            for i in range(2, len(sys.argv)):
                sys.stderr.write("next file '%s'\n" % (sys.argv[i]))
                shk = SHKfile(sys.argv[i])
                # print shk.first, shk.last
                stw, HK = shk.getHKword(table[0], sub=table[1])
                if table[2]:
                    f = table[2]
                    HK = map(f, HK)
                if HK:
                    # for i in range(0, len(stw)):
                    #    print "%10d %10.3f" % (stw[i], HK[i])
                    removeDuplicates(stw, HK)
                    # print "removed duplicates:"
                    tc = TimeConverter()
                    for i in range(0, len(stw)):
                        JD = tc.stw2JD(stw[i])
                        o = tc.JD2orbit(JD)
                        print "%10d %12.3f %10.3f" % (stw[i], o, HK[i])
                    # print "%10d %10.3f" % (stw[i], HK[i])
                # tk=stwplot.show(stw, HK, title=which)
                # tk.mainloop()
                del shk
    else:
        listHKdata()


if __name__ == "__main__":
    main()
