from numpy import ndarray, array, nonzero, array
import numpy as N
from pg import DB
from odincal.config import config


class db(DB):
    def __init__(self):
        DB.__init__(self, dbname=config.get('database', 'dbname'),
                    user=config.get('database', 'user'),
                    host=config.get('database', 'host'),
                    passwd=config.get('database', 'passwd'),
                    )


def freq(lofreq, skyfreq, LO):
    n = 896
    f = N.zeros(shape=(n,))
    seq = [1, 1, 1, -1, 1, 1, 1, -1, 1, -1, 1, 1, 1, -1, 1, 1]
    m = 0
    for adc in range(8):
        if seq[2 * adc]:
            k = seq[2 * adc] * 112
            df = 1.0e6 / seq[2 * adc]
            if seq[2 * adc + 1] < 0:
                df = -df
            for j in range(k):
                f[m + j] = LO[adc / 2] + j * df
            m += k
    fdata = N.zeros(shape=(n,))
    if skyfreq >= lofreq:
        for i in range(n):
            v = f[i]
            v = lofreq + v
            v /= 1.0e9
            fdata[i] = v
    else:
        for i in range(n):
            v = f[i]
            v = lofreq - v
            v /= 1.0e9
            fdata[i] = v
    return fdata


def decode_data(con, row):
    data = {
        'backend': [],
        'frontend': [],
        'version': [],
        'intmode': [],
        'sourcemode': [],
        'freqmode': [],
        'ssb_fq': [],
        'mean_spectra': [],
        'median_spectra': [],
        'channels': [],
        'skyfreq': [],
        'lofreq': [],
        'altitude_range': [],
        'hotload_range': [],
    }

    for item in data.keys():
        if item == 'mean_spectra' or item == 'median_spectra':
            data[item] = N.ndarray(shape=(row['channels'],), dtype='float64',
                                   buffer=row[item])
        elif item == 'ssb_fq' or item == 'altitude_range' or item == 'hotload_range':
            data[item] = N.array(row[item])
        else:
            data[item] = row[item]
    return data


class Fit_spectrum():
    def __init__(self, data, f):
        self.ssb_fq = data['ssb_fq']
        self.medianTb = data['median_spectra']
        self.intmode = data['intmode']
        self.backend = data['backend']
        self.data = data
        self.f = f

    def filter_spectrum(self, freq, spectrum):
        '''low pass filter'''
        # find indexes of the spectrum medianTb
        # that are "contaminated by atmospheric lines"
        # these indexes will not be used for the low pass fit

        # if medianTb spectrum change by more than 0.3 K between
        # five channels we flag this channel to be contaminated
        index = N.nonzero(N.abs(spectrum[5:-5] -
                                spectrum[10::]) > .3)[0] + 5
        # flag also the 15 closest channels on each side
        non_ok_index = []
        for ind in index:
            zr = 15
            non_ok_index.extend(
                N.array(range(N.max([0, ind - zr]), N.min([ind + zr, 112 * 2]))))

        # find indexes of "dead" channels
        non_ok = N.nonzero((spectrum == 0))[0]
        non_ok.shape = (non_ok.shape[0],)

        # combine indexes that are contamintated by lines or dead
        non_ok_index.extend(N.array(non_ok))
        non_ok_index = N.unique(N.array(non_ok_index))
        return non_ok_index

    def fit_spectrum(self):

        self.get_ssb_module_indexes()
        n = 112 * 2  # number of channels per module

        # first fit data from each ssb module independently
        A = []
        Err = []
        for ind, module in enumerate(self.module_indexes):

            # find indexes for the current module
            spectrum_indexes = range(ind * n, (ind + 1) * n)
            self.freq_indexes = range(module * n, (module + 1) * n)
            module_data = N.array(self.medianTb[spectrum_indexes])
            module_freq = N.array(self.f[self.freq_indexes])

            # sort data on frequency
            sort_freq = N.argsort(module_freq)

            # now apply a low pass filter on frequency sorted data
            non_ok_index = self.filter_spectrum(
                module_freq[sort_freq], module_data[sort_freq])

            # take care of that the filter was applied on sorted data
            if len(non_ok_index) > 0:
                non_ok_index = sort_freq[non_ok_index]
            ok_index = N.setdiff1d(range(n), non_ok_index)

            # now fit data (part 1)
            # fit the function y=a+b*sin(c*freq+d)
            # for a range of fixed c parameters
            a, err = self.fit_module_data(
                module_freq[ok_index], module_data[ok_index], module)
            A.append(a)
            Err.append(err)
        # now find out which c value that gave the least error
        # when considering data from all modules
        Err = N.array(Err)
        min_err_ind = N.argmin(N.sum(N.array(Err), 0))

        # now get the spectrum fit
        median_fit = []
        for ind, module in enumerate(self.module_indexes):
            spectrum_indexes = range(ind * n, (ind + 1) * n)
            freq_indexes = range(module * n, (module + 1) * n)
            a = A[ind][min_err_ind]
            Tbfit = a[0] + a[1] * N.sin(2 * N.pi / a[2] * (
                self.f[freq_indexes] - N.median(self.f[freq_indexes])) + a[3])
            find = N.nonzero(self.medianTb[spectrum_indexes] == 0)[0]
            Tbfit[find] = 0
            median_fit.extend(Tbfit)

        self.median_fit = N.array(median_fit)
        Tdata = {'median': self.medianTb,
                 'highaltfit': self.median_fit}
        print str(self.data['backend'] + ' ' + self.data['frontend'] + ' ' +
                  self.data['sourcemode'] + ' ' +
                  str(self.data['freqmode']) + ' ' +
                  str(self.data['intmode']) +
                  str(self.data['hotload_range']))
        return Tdata

    def fit_module_data(self, freq, spectrum, module):
        '''fit the function y=a+b*sin(c*freq+d), for a range
          of fixed c parameter, to spectrum'''

        problem = 0
        if len(spectrum) < 56:
            problem = 1
        if self.backend == 'AC1' and module == 0:
            problem = 1
        a = []  # an empty list of fit parameters
        err = []  # an empty list with errors for each fit

        # loop over a range of fixed 'c' parameters
        for ind in N.arange(0.01, 1, 0.001):
            if problem == 1:
                a.append([0, 0, 1, 0])
                err.append(9e4)
                continue
            ff = freq - N.median(self.f[self.freq_indexes])

            k1 = N.ones(len(freq))
            k2 = N.sin(2 * N.pi / ind * ff)
            k3 = N.cos(2 * N.pi / ind * ff)
            k = N.array([k1, k2, k3])
            k = k.transpose()
            m = N.linalg.lstsq(k, spectrum)[0]

            a0 = m[0]
            a1 = N.sqrt(m[1]**2 + m[2]**2)
            a2 = float(ind)
            a3 = N.arctan2(m[2], m[1])
            fa = a0 + a1 * N.sin(2 * N.pi / a2 * ff + a3)

            a.append([a0, a1, a2, a3])
            err.append(N.sum((fa - spectrum)**2))

        return a, err

    def get_ssb_module_indexes(self):
        n = 2 * 112
        if self.intmode == 511:
            module_indexes = range(0, 4)
            self.freq = self.f
        elif self.intmode == 1023:
            module_indexes = N.argsort(self.ssb_fq)[2:4]
            if module_indexes[0] > module_indexes[1]:
                module_indexes = [module_indexes[1], module_indexes[0]]
            freq_indexes = range(module_indexes[0] * n,
                                 (module_indexes[0] + 1) * n)
            freq_indexes.extend(range(module_indexes[1] * n,
                                      (module_indexes[1] + 1) * n))
            self.freq = self.f[freq_indexes]
        elif self.intmode == 2047:
            module_indexes = N.argsort(self.ssb_fq)[0:2]
            if module_indexes[0] > module_indexes[1]:
                module_indexes = [module_indexes[1], module_indexes[0]]
            freq_indexes = range(module_indexes[0] * n,
                                 (module_indexes[0] + 1) * n)
            freq_indexes.extend(range(module_indexes[1] * n,
                                      (module_indexes[1] + 1) * n))
            self.freq = self.f[freq_indexes]
        self.module_indexes = module_indexes


def main():
    '''fit data found in the ac_level1b_average table
       and import the fits to the ac_cal_level1c table'''

    con = db()
    version = 8
    temp = [version, '{80000,120000}']
    # find out for which modes we have data in the ac_level1b_average table
    query = con.query(''' select backend,frontend,
                  version,intmode,
                  sourcemode,freqmode,ssb_fq,hotload_range,
                  altitude_range,median_spectra,mean_spectra,
                  skyfreq,lofreq,channels
                  from ac_level1b_average
                  where version={0} and altitude_range='{1}'
                  order by backend,sourcemode,freqmode,
                  frontend,ssb_fq,hotload_range
                  '''.format(*temp))
    result = query.dictresult()

    # fit data for each mode and import to table ac_cal_level1c
    for row in result:
        data = decode_data(con, row)
        f = freq(data['lofreq'], data['skyfreq'],
                 data['ssb_fq'] * 1e6)

        f = Fit_spectrum(data, f)
        Tdata = f.fit_spectrum()

        if 1:
            temp = {
                'backend': row['backend'],
                'frontend': row['frontend'],
                'version': row['version'],
                'intmode': row['intmode'],
                'sourcemode': row['sourcemode'],
                'freqmode': row['freqmode'],
                'ssb_fq': row['ssb_fq'],
                'hotload_range': row['hotload_range'],
                'altitude_range': row['altitude_range'],
                'median_spectra': Tdata['median'].tostring(),
                'median_fit': Tdata['highaltfit'].tostring(),
                'channels': len(Tdata['median']),
            }

            tempkeys = [temp['backend'], temp['frontend'], temp['version'],
                        temp['intmode'], temp['sourcemode'], temp['freqmode'],
                        temp['ssb_fq'], temp['hotload_range'],
                        temp['altitude_range']]
            con.query('''delete from ac_cal_level1c
                     where backend='{0}' and frontend='{1}'
                     and version={2} and intmode={3}
                     and sourcemode='{4}' and freqmode={5}
                     and ssb_fq='{6}' and
                     hotload_range='{7}' and
                     altitude_range='{8}' '''.format(*tempkeys))
            con.insert('ac_cal_level1c', temp)
        else:
            pass

    con.close()
