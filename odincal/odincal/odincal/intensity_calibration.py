import copy
import numpy


def calibrate(spectra, calstw, version):
    """Do intensity calibration for all receivers"""

    (sigs, refs, cals) = sortspec(spectra)

    if not sigs or not refs or not cals:
        return None, version, None

    cals = cleancal(cals, refs)
    if len(cals) == 0:
        return None, version, None

    print 'skyfreq: {0} GHz'.format(refs[0].skyfreq / 1e9)
    medtsys = get_medtsys(cals)
    (rstw, rmat, _, _) = matrix(refs)
    calibrated = []
    tsys = copy.copy(sigs[0])
    tsys.type = 'CAL'
    tsys.data = medtsys.data
    if calstw != 0:
        tsys.stw = calstw
    calibrated.append(tsys)
    ssb = copy.copy(tsys)
    ssb.type = 'SSB'
    ssb.data = ssbcurve(ssb)
    calibrated.append(ssb)

    for sig_i in sigs:

        sig = copy.copy(sig_i)
        stw = float(sig.stw)
        ref = refs[0]
        ref.data = interpolate(rstw, rmat, stw)

        calibrate_partial(sig, ref.data, tsys.data)

        # calculate band average gain
        n_channels = 112
        bands = 8
        sig.gain = numpy.zeros(8)
        for band in range(bands):
            i0 = band * n_channels
            i1 = i0 + n_channels
            sig.gain[band] = numpy.mean(
                ref.data[i0:i1] / tsys.data[i0:i1], 0)

        # calculate average tsys
        tsys_data = numpy.take(
            tsys.data, numpy.nonzero(tsys.data)[0])
        if tsys_data.shape[0] == 0:
            return None, version, None
        sig.tsys = numpy.add.reduce(
            tsys_data) / tsys_data.shape[0]

        calibrated.append(sig)

    (tspill, eff) = get_efficiencies(calibrated)
    eta = 1.0 - tspill / 300.0
    for spec in calibrated:
        if spec.type == 'SPE' and spec.ref != 1:
            spec.data = numpy.choose(
                numpy.equal(spec.data, 0.0),
                ((spec.data - tspill) / eta, 0.0)
            )
            spec.efftime = spec.inttime * eff * eta**2
            if spec.efftime > numpy.finfo('float32').max:
                spec.efftime = float('inf')

    return calibrated, version, tspill


def get_efficiencies(calibrated_specs):
    '''get contribution from the baffle and efficient integration time
       from high altitude spectra'''

    eff = []
    tspill = []
    maxalt = numpy.max([spec.altitude for spec in calibrated_specs])
    for sig in calibrated_specs:
        if (sig.type == 'SPE' and
                sig.altitude > maxalt - 10.0e3 and sig.ref != 1):
            tspill.append(numpy.median(sig.data[sig.data.nonzero()]))
            if sig.backend == "AOS":
                data = sig.data
                mean = numpy.add.reduce(data) / data.shape[0]
                msq = numpy.add.reduce(
                    (data - mean)**2) / (data.shape[0] - 1)
                teff = (sig.tsys * sig.tsys / msq) / sig.freqres
                eff.append(teff / sig.inttime)
            else:
                n_channels = 112
                bands = 8
                sigma = numpy.zeros(shape=(bands,))
                for band in range(bands):
                    i0 = band * n_channels
                    i1 = i0 + n_channels
                    data = sig.data[i0:i1]
                    mean = numpy.add.reduce(data) / float(n_channels)
                    msq = numpy.add.reduce(
                        (data - mean)**2) / float(n_channels - 1)
                    sigma[band] = msq
                msq = min(numpy.take(sigma, numpy.nonzero(sigma))[0])
                if sig.backend == 'AC1':
                    # do not use data from ssb module 1
                    msq = min(
                        numpy.take(sigma[2:], numpy.nonzero(sigma[2:]))[0])
                teff = numpy.zeros(shape=(8,))
                teff[sigma != 0] = (
                    sig.tsys**2 / sigma[sigma != 0]) / sig.freqres
                if sig.backend == 'AC1':
                    teff[0] = 0
                    teff[1] = 0
                eff.append(teff / sig.inttime)

    # Tspill is the median contribution from the baffle
    if tspill:
        tspill = numpy.median(tspill)
    else:
        tspill = 9.0

    # eff is the mean integration efficiency, in the sense that
    # s.inttime*eff is an effective integration time related to
    # the noise level of a spectrum via the radiometer formula
    eff = numpy.array(eff)
    if eff.size > 0:
        eff = numpy.max(numpy.mean(eff, 0))
    else:
        eff = 1.0
    print 'Tspill: ' + str(tspill)
    print 'eff: ' + str(eff)
    return (tspill, eff)


def cleancal(cal, ref):

    (rstw, rmat, _, _) = matrix(ref)
    spec = copy.copy(cal[0])
    (tmin, tmax) = acceptable_tsys(spec.frontend)
    for k, _ in enumerate(cal):
        stw = float(cal[k].stw)
        # find reference data by table lookup
        spec.data = interpolate(rstw, rmat, stw)
        # calculate a Tsys spectrum from given C and R
        cal[k].data = get_tsys(cal[k], spec)
    # combine CAL spectra into matrix
    (_, cmat, _, _) = matrix(cal)
    nc = cmat.shape[0]
    if cal[0].backend == 'AOS':
        mc = numpy.add.reduce(cmat, 1) / cmat.shape[1]
    else:
        n = 112
        bands = len(cal[0].data) / n
        rms = numpy.zeros(shape=(bands,))
        idx = []
        for band in range(bands):
            i0 = band * n
            i1 = i0 + n
            mc = numpy.mean(cmat[:, i0:i1], 1)
            if numpy.sum((mc != 0.0).astype(int)) < nc or nc < 2:
                rms[band] = 1.0e10
            else:
                diff = mc - numpy.mean(mc)
                rms[band] = numpy.sqrt(numpy.sum(diff ** 2) / float(nc - 1))
            if rms[band] != 0.0 and rms[band] != 1.0e10:
                idx.append(numpy.arange(i0, i1))
            print "rms of band {0} = {1}".format(band, rms[band])
        valid_channels = numpy.array(idx).flatten()
        if valid_channels.size == 0:
            return []
        mc = numpy.mean(cmat[:, valid_channels], 1)
    return [cali for cali, mci in zip(cal, mc) if mci >= tmin and mci <= tmax]


def ssbcurve(spec):
    dB = {'495': -26.7, '549': -27.7, '555': -32.3, '572': -28.7}
    if spec.frontend in ['495', '549', '555', '572']:
        maxdB = dB[spec.frontend]
    else:
        return numpy.ones(shape=len(spec.data,))

    fIF = 3900e6
    if spec.skyfreq > spec.lofreq:
        fim = spec.lofreq - fIF
    else:
        fim = spec.lofreq + fIF
    c = 2.9979e8
    d0 = c / fIF / 4.0
    l = c / fim
    n = numpy.floor(d0 / l)
    dx = (n + 0.5) * l - d0

    x0 = d0 + dx
    if spec.skyfreq > spec.lofreq:
        f = get_frequency(spec) - 2.0 * fIF / 1.0e9
    else:
        f = get_frequency(spec) + 2.0 * fIF / 1.0e9
        fim = spec.lofreq + fIF
    l = c / f / 1.0e9
    p = 2.0 * numpy.pi * x0 / l
    Y = 0.5 * (1.0 + numpy.cos(p))
    S = 10.0**(maxdB / 10.0)
    Z = S + (1.0 - S) * Y
    Z = 1.0 - Z
    return Z


def matrix(spectra):
    if not spectra:
        return None

    rows = len(spectra)
    cols = len(spectra[0].data)

    stw = numpy.zeros(shape=(rows,))
    start = numpy.zeros(shape=(rows,))
    inttime = numpy.zeros(shape=(rows,))
    mat = numpy.zeros(shape=(rows, cols))

    for index in range(rows):
        spec = spectra[index]
        if spec.data.shape[0] == cols:
            mat[index, :] = spec.data
        else:
            pass
        stw[index] = spec.stw
        inttime[index] = spec.inttime
        start[index] = spec.start

    return (stw, mat, inttime, start)


def get_frequency(spec):
    df = 1.0e6
    n = 896
    f = numpy.zeros(shape=(n,))
    for j in range(n / 8):
        f[j + 0 * n / 8] = spec.ssb_fq[0] * 1e6 - (n / 8 - 1 - j) * df
        f[j + 1 * n / 8] = spec.ssb_fq[0] * 1e6 + j * df
        f[j + 2 * n / 8] = spec.ssb_fq[1] * 1e6 - (n / 8 - 1 - j) * df
        f[j + 3 * n / 8] = spec.ssb_fq[1] * 1e6 + j * df
        f[j + 4 * n / 8] = spec.ssb_fq[2] * 1e6 - (n / 8 - 1 - j) * df
        f[j + 5 * n / 8] = spec.ssb_fq[2] * 1e6 + j * df
        f[j + 4 * n / 8] = spec.ssb_fq[2] * 1e6 + j * df
        f[j + 5 * n / 8] = spec.ssb_fq[2] * 1e6 - (n / 8 - 1 - j) * df
        f[j + 6 * n / 8] = spec.ssb_fq[3] * 1e6 - (n / 8 - 1 - j) * df
        f[j + 7 * n / 8] = spec.ssb_fq[3] * 1e6 + j * df

    seq = [1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, 1, 1]
    m = 0
    for adc in range(8):
        if seq[2 * adc]:
            k = seq[2 * adc] * 112
            df = 1.0e6 / seq[2 * adc]
            if seq[2 * adc + 1] < 0:
                df = -df
            for j in range(k):
                f[m + j] = spec.ssb_fq[adc / 2] * 1e6 + j * df
            m += k
    fdata = numpy.zeros(shape=(n,))
    if spec.skyfreq >= spec.lofreq:
        for i in range(n):
            v = f[i]
            v = spec.lofreq + v
            v /= 1.0e9
            fdata[i] = v
    else:
        for i in range(n):
            v = f[i]
            v = spec.lofreq - v
            v /= 1.0e9
            fdata[i] = v
    return fdata


def get_medtsys(cals):
    cal = cals[0]
    (tmin, tmax) = acceptable_tsys(cal.frontend)
    (_, cmat, _, _) = matrix(cals)
    if cal.backend == 'AOS':
        cal.data = numpy.add.reduce(cmat) / float(cmat.shape[0])
    else:
        n_channels = 112
        bands = len(cal.data) / n_channels
        cal.data = numpy.zeros(shape=(len(cal.data,)))
        for band in range(bands):
            data = numpy.zeros(shape=(n_channels,))
            i0 = band * n_channels
            k = 0
            for i in range(cmat.shape[0]):
                d = cmat[i, i0:i0 + n_channels]
                if len(numpy.nonzero(d)[0]) == n_channels:
                    tsys = numpy.add.reduce(d) / n_channels
                    if tsys > tmin and tsys < tmax:
                        data = data + d
                        k = k + 1
            if k > 0:
                data = data / float(k)
            cal.data[i0:i0 + n_channels] = data
    return cal


def get_tsys(cal, ref):
    data = numpy.zeros(shape=(len(cal.data),))
    epsr = 1.0
    eta_mt = 1.0
    eta_ms = 1.0
    tspill = 290.0
    t_bg = planck(2.7, cal.skyfreq)
    t_hot = planck(cal.tcal, cal.skyfreq)
    if t_hot == 0.0:
        t_hot = 275.0
    dt = (
        epsr * eta_mt * t_hot - eta_ms * t_bg +
        (eta_ms - eta_ms) * tspill
    )
    for index in range(0, len(cal.data)):
        if ref.data[index] > 0.0:
            if cal.data[index] > ref.data[index]:
                data[index] = (
                    ref.data[index] /
                    (cal.data[index] - ref.data[index])
                )
                data[index] *= dt
            else:
                data[index] = 0.0
        else:
            data[index] = 0.0
    return data


def interpolate(mstw, m, stw):
    rows = m.shape[0]
    if rows != len(mstw):
        raise IndexError
    index = numpy.searchsorted(mstw, stw)

    if index == 0:
        return m[0]
    if index == rows:
        return m[rows - 1]

    dt0 = float(mstw[index] - stw)
    dt1 = float(stw - mstw[index - 1])
    dt = float(mstw[index] - mstw[index - 1])
    return (dt0 * m[index - 1] + dt1 * m[index]) / dt


def acceptable_tsys(rx):
    if rx == '119':
        tmin = 400.0
        tmax = 1500.0
    else:
        tmin = 2000.0
        tmax = 7000.0
    return (tmin, tmax)


def calibrate_partial(sig, ref, cal):
    # eta_ml = [0.976, 0.968, 0.978, 0.975, 0.954]
    eta = 1.0
    eta_ms = 1.0
    t_amb = 290.0
    t_bg = planck(2.7, sig.skyfreq)
    spill = eta_ms * t_bg - (eta_ms - eta) * t_amb
    ok_ind = numpy.nonzero(ref > 0.0)[0]
    data = numpy.zeros(shape=len(sig.data),)
    data[ok_ind] = (
        (sig.data[ok_ind] - ref[ok_ind]) / ref[ok_ind] *
        cal[ok_ind] + spill
    ) / eta
    sig.data = data
    sig.type = 'SPE'


def sortspec(spectra):
    sig = []
    ref = []
    cal = []
    for spec in spectra:
        if spec.type == 'SIG':
            sig.append(spec)
        elif (spec.type == 'SK1' or
              spec.type == 'SK2' or
              spec.type == 'REF'):
            ref.append(spec)
        elif spec.type == 'CAL':
            cal.append(spec)
    return (sig, ref, cal)


def planck(temperature, frequency):
    h = 6.626176e-34  # Planck constant (Js)
    k = 1.380662e-23  # Boltzmann constant (J/K)
    t0 = h * frequency / k
    if temperature > 0.0:
        tb = t0 / (numpy.exp(t0 / temperature) - 1.0)
    else:
        tb = 0.0
    return tb


if __name__ == "__main__":
    pass
