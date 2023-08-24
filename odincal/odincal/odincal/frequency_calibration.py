import numpy


class Spectra(object):
    """A class derived to perform frequency calibration of odin spectra"""

    def __init__(self, con, data, ref):
        self.ref = ref
        self.start = data['start']
        self.data = numpy.ndarray(
            shape=(112 * 8,),
            dtype='float64',
            buffer=data['spectra']
        )
        self.stw = data['stw']
        self.ssb_fq = data['ssb_fq']
        self.backend = data['backend']
        self.frontend = data['frontend']
        self.vgeo = data['vgeo']
        self.mode = data['mode']
        self.tcal = data['hotloada']
        if self.tcal == 0:
            self.tcal = data['hotloadb']
        self.freqres = 1e6
        if data['sig_type'] == 'SIG':
            self.qerror = data['qerror']
            self.qachieved = data['qachieved']
        self.inttime = data['inttime']
        self.intmode = 511
        self.skyfreq = 0
        self.lofreq = data['lo']
        self.ssb = data['ssb']
        self.temp_pll = data['imageloadb']
        if self.temp_pll == 0:
            self.temp_pll = data['imageloada']
        self.current = data['mixc']
        self.type = data['sig_type']
        self.source = []
        self.topic = []
        self.restfreq = []
        self.latitude = data['latitude']
        self.longitude = data['longitude']
        self.altitude = data['altitude']
        self.tsys = 0
        self.efftime = 0
        self.sbpath = 0
        self.corr_coef = numpy.ndarray(
            shape=(8, 96),
            dtype='float64',
            buffer=data['cc']
        )
        self.zerolag = numpy.array(self.corr_coef[0:8, 0])
        self.skybeamhit = data['skybeamhit']
        self.ssb_att = data['ssb_att']
        self.gain = []
        self.split = 0
        self.freqmode = 0
        self.maxsup = 0

    def tuning(self):

        if self.frontend == '119':
            if_freq = 3900.0e6
            self.lofreq = 114.8498600000000e+9
            self.fcalibrate(self.lofreq, if_freq)
            return

        rxs = {'495': 1, '549': 2, '555': 3, '572': 4}
        if self.frontend not in rxs.keys():
            return
        (if_freq, sbpath) = get_sideband(
            self.frontend, self.lofreq, self.ssb
        )
        self.sbpath = sbpath / 1.0e6
        (adc_split, adc_upper) = (0x0200, 0x0400)
        if self.intmode & adc_split:
            if self.backend == 'AC1':
                if self.intmode & adc_upper:
                    if_freq = if_freq * 3.6 / 3.9
                else:
                    if_freq = if_freq * 4.2 / 3.9
            elif self.backend == 'AC2':
                if self.intmode & adc_upper:
                    if_freq = if_freq * 4.2 / 3.9
                else:
                    if_freq = if_freq * 3.6 / 3.9

        if self.current < 0.19 and self.frontend != '572':
            self.lofreq = 0.0

        self.fcalibrate(self.lofreq, if_freq)

    def fcalibrate(self, lo_freq, if_freq):
        """Perform frequency calibration."""
        if lo_freq == 0.0:
            return
        if self.frontend == '495' or self.frontend == '549':
            drift = 1.0 + (29.23 - 0.138 * self.temp_pll) * 1.0e-6
        else:
            drift = 1.0 + (24.69 - 0.109 * self.temp_pll) * 1.0e-6
        lo_freq = lo_freq * drift
        self.lofreq = lo_freq
        self.skyfreq = lo_freq + if_freq
        self.maxsup = lo_freq - if_freq
        # apply Doppler correction
        self.restfreq = self.skyfreq / (1.0 - self.vgeo / 2.99792456e8)

    def get_freqmode(self):
        """get freqmode for spectrum"""

        lo_freq = self.lofreq * (1.0 + self.vgeo / 2.99792456e8)
        modes = {
            'STRAT': "Stratospheric",
            'ODD_H': "Odd hydrogen",
            'ODD_N': "Odd nitrogen",
            'WATER': "Water isotope",
            'SUMMER': "Summer mesosphere",
            'DYNAM': "Transport"
        }

        config = None
        if self.backend == 'AC1':
            config = self.get_ac1_config()
        elif self.backend == 'AC2':
            config = self.get_ac2_config()

        if config:
            d_freq = [0.0] * len(config)
            for i, _ in enumerate(d_freq):
                d_freq[i] = abs(lo_freq - config[i][0])
            index = d_freq.index(min(d_freq))
            self.freqmode = config[index][1]
            self.topic = config[index][2]
            self.split = config[index][3]
        else:
            pass

        if self.freqmode:
            self.source = "%s FM=%d" % (modes[self.topic], self.freqmode)
        else:
            self.source = "unknown"
            self.topic = "N/A"
            self.split = 0

    def get_ac1_config(self):
        """get definition of config/freqmode for ac1"""

        config = None

        if self.frontend == '495':
            config = [
                [492.750e9, 23, "DYNAM", 0],
                [495.402e9, 29, "DYNAM", 0],
                [499.698e9, 25, "DYNAM", 0]
            ]

        elif self.frontend == '549':
            config = [
                [548.502e9, 2, "STRAT", 0],
                [553.050e9, 19, "WATER", 0],
                [547.752e9, 21, "WATER", 1],
                [553.302e9, 23, "DYNAM", 0],
                [553.302e9, 29, "DYNAM", 0]
            ]

        elif self.frontend == '555':
            config = [[553.298e9, 13, "SUMMER", 0]]

        elif self.frontend == '572':
            config = [[572.762e9, 24, "DYNAM", 0]]

        elif self.frontend == '119':
            config = [[114.852e9, 40, "DYNAM", 0]]

        return config

    def get_ac2_config(self):
        """get definition of config/freqmode for ac1"""

        config = None

        if self.frontend == '495':
            config = [
                [497.880e9, 1, "STRAT", 1],
                [492.750e9, 8, "WATER", 1],
                [494.250e9, 17, "WATER", 0],
                [499.698e9, 25, "DYNAM", 0],
                [491.160e9, 41, "DYNAM", 0]
            ]

        elif self.frontend == '572':
            config = [
                [572.964e9, 22, "DYNAM", 1],
                [572.762e9, 14, "SUMMER", 0]
            ]
        return config


def get_sideband(rx_i, lo_freq, ssb):
    ssb_params = {
        '495': (61600.36, 104188.89, 0.0002977862, 313.0),
        '549': (57901.86, 109682.58, 0.0003117128, 313.0),
        '555': (60475.43, 116543.50, 0.0003021341, 308.0),
        '572': (58120.92, 115256.73, 0.0003128605, 314.0)
    }
    path = 0.0
    c1_ssb = ssb_params[rx_i][0]
    c2_ssb = ssb_params[rx_i][1]
    sf_ssb = ssb_params[rx_i][2]
    sbpath = (
        -ssb_params[rx_i][3] + 2.0 * ssb_params[rx_i][3] * ssb / 4095.0
    ) * 2.0
    for i in range(-2, 3):
        s3900 = (
            299.79 / (ssb + c1_ssb) * (c2_ssb + i / sf_ssb) -
            lo_freq / 1.0e9
        )
        if abs(abs(s3900) - 3.9) < abs(abs(path) - 3.9):
            path = s3900
    if path < 0.0:
        if_freq = -3900.0e6
    else:
        if_freq = 3900.0e6
    return (if_freq, sbpath)


if __name__ == "__main__":
    pass
