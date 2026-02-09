import numpy as np

# IAU 1976 spheroid per Odin doc (NOT WGS84)
A = 6378140.0
F = 1.0 / 298.257
E2 = F * (2.0 - F)

def geodetic_to_ecef_llh(llh):
    llh = np.asarray(llh, dtype=np.float64)
    lat = np.deg2rad(llh[:, 0])
    lon = np.deg2rad(llh[:, 1])
    h = llh[:, 2]

    sin_lat = np.sin(lat)
    cos_lat = np.cos(lat)
    sin_lon = np.sin(lon)
    cos_lon = np.cos(lon)

    N = A / np.sqrt(1.0 - E2 * sin_lat**2)

    x = (N + h) * cos_lat * cos_lon
    y = (N + h) * cos_lat * sin_lon
    z = (N * (1.0 - E2) + h) * sin_lat
    return np.column_stack((x, y, z))

def ecef_to_geodetic_xyz(xyz, max_iter=10, tol=1e-12, wrap_lon=False):
    xyz = np.asarray(xyz, dtype=np.float64)
    x, y, z = xyz[:, 0], xyz[:, 1], xyz[:, 2]

    lon = np.arctan2(y, x)  # [-pi, pi]
    p = np.sqrt(x * x + y * y)

    eps_p = 1e-20
    p_safe = np.where(p < eps_p, eps_p, p)

    lat = np.arctan2(z, p_safe * (1.0 - E2))

    for _ in range(max_iter):
        sin_lat = np.sin(lat)
        N = A / np.sqrt(1.0 - E2 * sin_lat * sin_lat)
        h = p_safe / np.cos(lat) - N
        lat_new = np.arctan2(z, p_safe * (1.0 - E2 * (N / (N + h))))
        if np.max(np.abs(lat_new - lat)) < tol:
            lat = lat_new
            break
        lat = lat_new

    sin_lat = np.sin(lat)
    N = A / np.sqrt(1.0 - E2 * sin_lat * sin_lat)
    h = p_safe / np.cos(lat) - N

    lat_deg = np.rad2deg(lat)
    lon_deg = np.rad2deg(lon)  # keep [-180, 180] for interpolation safety

    if wrap_lon:
        lon_deg = (lon_deg + 360.0) % 360.0

    return np.column_stack((lat_deg, lon_deg, h))
