import numpy


class Ref_fit():
    '''A class that perform weighted quadratic fit'''

    def __init__(self):
        # a parameter that control the weighting exp(-t0/lamb)
        self.lamb = 100 * 16.0

    def interp(self, t, y, t1, tint, start):
        '''input: y=spectra shape=(lenth of times, length of spectrum),
              t=times, t1=target time (scalar)'''
        # find the stw of the scan start
        calstw = numpy.max(start[start <= t1])
        # find the length of the scan
        maxstw = numpy.max(t[start == calstw])
        minstw = numpy.min(t[start == calstw])
        self.lamb = (maxstw - minstw)
        # print t1,minstw,maxstw,calstw,maxstw-minstw
        # use only data within the "window" time of the scan
        self.t0 = t - t1
        index = numpy.abs(self.t0) <= self.lamb
        # print numpy.min(self.t0[index]),numpy.max(self.t0[index])
        self.w = numpy.exp(-numpy.abs(16 * self.t0[index]) / self.lamb)
        self.sigma = 1 / numpy.sqrt(tint[index])
        # print self.sigma.shape
        # print t.shape
        self.ws2 = self.w**2 / self.sigma**2
        n0 = numpy.sum(self.ws2)
        n1 = numpy.sum(self.t0[index] * self.ws2)
        n2 = numpy.sum(self.t0[index]**2 * self.ws2)
        n3 = numpy.sum(self.t0[index]**3 * self.ws2)
        n4 = numpy.sum(self.t0[index]**4 * self.ws2)
        m1 = [n0, n1, n2]
        m2 = [n1, n2, n3]
        m3 = [n2, n3, n4]
        M = numpy.array([m1, m2, m3])
        self.M0 = numpy.linalg.det(M)
        y1 = y[index, :]
        y1 = y1.swapaxes(0, 1)
        a10 = numpy.linalg.linalg.dot(y1, self.ws2)
        a20 = numpy.linalg.linalg.dot(y1, self.t0[index] * self.ws2)
        a30 = numpy.linalg.linalg.dot(y1, self.t0[index]**2 * self.ws2)
        a1 = [a10[0], m1[1], m1[2]]
        a2 = [a20[0], m2[1], m2[2]]
        a3 = [a30[0], m3[1], m3[2]]
        A = numpy.array([a1, a2, a3])
        # A[0,0]=a10,A[1,0]=a20,A[2,0]=a30
        detA = (
            a10 * (A[1, 1] * A[2, 2] - A[1, 2] * A[2, 1]) +
            A[0, 1] * (A[1, 2] * a30 - a20 * A[2, 2]) +
            A[0, 2] * (a20 * A[2, 1] - A[1, 1] * a30))
        self.c0 = detA / self.M0

        return self.c0


def eosmls_fit(t1, t, y):
    sigma = 1.0
    lamb = 3.0
    t0 = numpy.abs(t - t1)
    w = numpy.exp(-t0 / lamb)
    s2 = w**2 / sigma**2

    m1 = [numpy.sum(t / t / s2), numpy.sum(t / s2), numpy.sum(t**2 / s2)]
    m2 = [numpy.sum(t / s2), numpy.sum(t**2 / s2), numpy.sum(t**3 / s2)]
    m3 = [numpy.sum(t**2 / s2), numpy.sum(t**3 / s2), numpy.sum(t**4 / s2)]
    M = numpy.array([m1, m2, m3])

    a1 = [numpy.sum(y * t / t / s2), numpy.sum(t / s2), numpy.sum(t**2 / s2)]
    a2 = [numpy.sum(y * t / s2), numpy.sum(t**2 / s2), numpy.sum(t**3 / s2)]
    a3 = [numpy.sum(y * t**2 / s2), numpy.sum(t**3 / s2), numpy.sum(t**4 / s2)]
    A = numpy.array([a1, a2, a3])

    b1 = [numpy.sum(t / t / s2), numpy.sum(y / s2), numpy.sum(t**2 / s2)]
    b2 = [numpy.sum(t / s2), numpy.sum(y * t / s2), numpy.sum(t**3 / s2)]
    b3 = [numpy.sum(t**2 / s2), numpy.sum(y * t**2 / s2), numpy.sum(t**4 / s2)]
    B = numpy.array([b1, b2, b3])

    c1 = [numpy.sum(t / t / s2), numpy.sum(t / s2), numpy.sum(y / s2)]
    c2 = [numpy.sum(t / s2), numpy.sum(t**2 / s2), numpy.sum(y * t / s2)]
    c3 = [numpy.sum(t**2 / s2), numpy.sum(t**3 / s2), numpy.sum(y * t**2 / s2)]
    C = numpy.array([c1, c2, c3])

    a = numpy.linalg.det(A) / numpy.linalg.det(M)
    b = numpy.linalg.det(B) / numpy.linalg.det(M)
    c = numpy.linalg.det(C) / numpy.linalg.det(M)
    y0 = a + b * t1 + c * t1**2
    print a, b, c, y0
    return y0,


# (rstw,rmat)
if 0:
    t = 10 * numpy.random.random(100)
    s = numpy.ones(100)
    a = 1.1
    b = 0.01
    c = 0.002
    y = a + b * t + c * t**2
    t1 = 7.2

    rstw = t
    rmat = []
    for ind in range(t.shape[0]):
        rmat.append(y[ind] * numpy.ones(896))
    rmat = numpy.array(rmat)

    f = Lodin_fit()
    f.interp(rstw, rmat, t1, s, s)

    print a + b * t1 + c * t1**2
    print f.c0[0]



# use in cleancal and interpolate

# l1080
