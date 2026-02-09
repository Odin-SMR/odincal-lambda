import numpy as np

# IAU 1976 spheroid per Odin doc (NOT WGS84)
A = 6378140.0  # m
f = 1.0 / 298.257
e2 = f * (2.0 - f)

OMEGA_EARTH = 7.2921150e-5  # rad/s
C = 299792458.0


def remove_aberration(mb_hat, v, sign=1.0):
    """
    mb_hat: (n,3) unit LOS direction (legacy calls this 'mb')
    v:      (n,3) spacecraft velocity in same frame as mb_hat (m/s)
    Returns e_hat: aberration-corrected unit vector
    sign:
      +1 uses (mb - v/c)/(1 - dv)  (matches your legacy snippet)
      -1 would apply the opposite correction (if you ever need it)
    """
    beta = (sign * v) / C
    dv = np.einsum("ij,ij->i", mb_hat, beta)  # dot(mb, v/c)
    e = (mb_hat - beta) / (1.0 - dv)[:, None]
    e /= np.linalg.norm(e, axis=1, keepdims=True)
    return e


def geodetic_to_ecef_iau76(lat_deg, lon_deg, h_m):
    lat = np.deg2rad(lat_deg)
    lon = np.deg2rad(lon_deg)

    sin_lat = np.sin(lat)
    cos_lat = np.cos(lat)
    sin_lon = np.sin(lon)
    cos_lon = np.cos(lon)

    N = A / np.sqrt(1.0 - e2 * sin_lat * sin_lat)

    x = (N + h_m) * cos_lat * cos_lon
    y = (N + h_m) * cos_lat * sin_lon
    z = (N * (1.0 - e2) + h_m) * sin_lat
    return np.column_stack([x, y, z])


def compute_vgeo_from_products(sc_state_itrs, smr_tan_llh):
    """
    sc_state_itrs: (n,6) [x,y,z,vx,vy,vz] in ITRS/ECEF (m, m/s)
    smr_tan_llh:   (n,3) [lat_deg, lon_deg, alt_km] for Radiometer LOS
    """
    r_sc = sc_state_itrs[:, 0:3]
    v_sc = sc_state_itrs[:, 3:6]

    lat = smr_tan_llh[:, 0]
    lon = smr_tan_llh[:, 1]
    h_m = smr_tan_llh[:, 2] * 1000.0

    r_tan = geodetic_to_ecef_iau76(lat, lon, h_m)

    # LOS unit vector from spacecraft to tangent point (both ITRS)
    los = r_tan - r_sc
    los_hat = los / np.linalg.norm(los, axis=1, keepdims=True)

    # Tangent point velocity due to Earth rotation
    omega = np.array([0.0, 0.0, OMEGA_EARTH])
    v_tan_rot = np.cross(np.broadcast_to(omega, r_tan.shape), r_tan)
    v_sc_rot = np.cross(np.broadcast_to(omega, r_sc.shape), r_sc)

    # LOS-projected relative velocity
    vgeo_tan = np.einsum("ij,ij->i", (v_sc - v_tan_rot), los_hat)
    vgeo_none = np.einsum("ij,ij->i", v_sc, los_hat)
    vgeo_sc = np.einsum("ij,ij->i", (v_sc - v_sc_rot), los_hat)
    return vgeo_tan, vgeo_none, vgeo_sc


import numpy as np


def vgeo_from_products_with_astropy(sc_state_eci, smr_tan_llh, utc):
    """
    sc_state_eci: (n,6) inertial (ECI-ish / "true equator/equinox of date"): x,y,z,vx,vy,vz [m, m/s] :contentReference[oaicite:0]{index=0}
    smr_tan_llh:  (n,3) lat_deg, lon_deg, alt_km (Earth-fixed; Odin tangent uses IAU-1976 spheroid) :contentReference[oaicite:1]{index=1}
    utc:          array-like of datetime64 or ISO strings, length n

    Returns
    -------
    vgeo_tan  : -dot(v_sc - ω×r_tan, los_hat)
    vgeo_none : -dot(v_sc, los_hat)
    vgeo_sc   : -dot(v_sc - ω×r_sc, los_hat)
    """
    # --- convert spacecraft inertial -> ITRS using astropy (TETE trick) ---
    import astropy.units as u
    from astropy.coordinates import (
        ITRS,
        TETE,
        CartesianDifferential,
        CartesianRepresentation,
    )
    from astropy.time import Time

    sc_state_eci = np.asarray(sc_state_eci, dtype=np.float64)
    smr_tan_llh = np.asarray(smr_tan_llh, dtype=np.float64)

    # Be explicit about UTC scale (STW.ATT timestamps are labeled UT1, but inputs are usually UTC) :contentReference[oaicite:2]{index=2}
    t = Time(utc, scale="utc")

    r = sc_state_eci[:, 0:3] * u.m
    v = sc_state_eci[:, 3:6] * (u.m / u.s)

    rep = CartesianRepresentation(r.T)
    dif = CartesianDifferential(v.T)

    tete = TETE(rep.with_differentials(dif), obstime=t)
    itrs = tete.transform_to(ITRS(obstime=t))

    r_sc = np.vstack(
        [itrs.x.to_value(u.m), itrs.y.to_value(u.m), itrs.z.to_value(u.m)]
    ).T
    v_sc = np.vstack(
        [
            itrs.v_x.to_value(u.m / u.s),
            itrs.v_y.to_value(u.m / u.s),
            itrs.v_z.to_value(u.m / u.s),
        ]
    ).T

    # --- tangent point in ITRS/ECEF (use IAU-1976 ellipsoid constants in geodetic_to_ecef_iau76) ---
    lat = smr_tan_llh[:, 0]
    lon = smr_tan_llh[:, 1]
    h_m = smr_tan_llh[:, 2] * 1000.0
    r_tan = geodetic_to_ecef_iau76(lat, lon, h_m)

    # LOS unit vector spacecraft->tangent in ITRS
    los = r_tan - r_sc
    mb_hat = los / np.linalg.norm(los, axis=1, keepdims=True)

    # Earth rotation velocities at tangent and spacecraft (in ITRS)
    omega = np.array([0.0, 0.0, OMEGA_EARTH], dtype=np.float64)
    v_tan_rot = np.cross(np.broadcast_to(omega, r_tan.shape), r_tan)
    v_sc_rot = np.cross(np.broadcast_to(omega, r_sc.shape), r_sc)

    e_hat = remove_aberration(mb_hat, v_sc)
    # NOTE: Depending on astropy version / interpretation, v_sc in ITRS may already be a time-derivative
    # in the rotating frame. These three outputs let you empirically match legacy.
    vgeo_tan = -np.einsum("ij,ij->i", (v_sc + v_tan_rot), e_hat)
    vgeo_none = -np.einsum("ij,ij->i", v_sc, e_hat)
    vgeo_sc = -np.einsum("ij,ij->i", (v_sc + v_sc_rot), e_hat)

    return vgeo_tan, vgeo_none, vgeo_sc
