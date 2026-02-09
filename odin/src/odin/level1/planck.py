import numpy as np

h = 6.626176e-34
k = 1.380662e-23


def J(T, f_hz):
    # T: scalar or array
    # f_hz: scalar or array broadcastable to T
    T0 = h * f_hz / k
    T = np.asarray(T, dtype=float)
    return np.where(T > 0, T0 / (np.expm1(T0 / T)), 0.0) 
