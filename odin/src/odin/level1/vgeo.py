import astropy.units as u
import numpy as np
from astropy.coordinates import (
    GCRS,
    ITRS,
    CartesianDifferential,
    CartesianRepresentation,
    EarthLocation,
    SkyCoord,
)
from astropy.time import Time

OMEGA_E = 7.292115e-5  # rad/s


def tangential_speed(r_sc, v_sc):
    """
    r_sc: (N,3) position [m]
    v_sc: (N,3) velocity [m/s]
    Returns:
      v_tan: (N,) tangential speed magnitude
    """
    r_sc = np.asarray(r_sc, dtype=np.float64)
    v_sc = np.asarray(v_sc, dtype=np.float64)

    r_hat = r_sc / np.linalg.norm(r_sc, axis=1, keepdims=True)
    v_rad = np.sum(v_sc * r_hat, axis=1)  # signed
    v_tan_vec = v_sc - v_rad[:, None] * r_hat
    v_tan = np.linalg.norm(v_tan_vec, axis=1)  # magnitude
    return -v_tan


def vgeo_from_att(
    dt_utc,  # pandas Series/array of tz-aware UTC datetimes, length N
    r_sc_eci_m,  # (N,3) meters, ECI-like (true-of-date inertial)
    v_sc_eci_mps,  # (N,3) m/s in same inertial frame
    tan_llh_km,  # (N,3) [lat_deg, lon_deg, alt_km] WGS84
):
    t = Time(dt_utc.to_numpy())

    # Tangent point in ITRS (Earth-fixed)
    lat = tan_llh_km[:, 0] * u.deg
    lon = tan_llh_km[:, 1] * u.deg
    h = (tan_llh_km[:, 2] * 1000.0) * u.m
    loc = EarthLocation.from_geodetic(lon=lon, lat=lat, height=h)
    r_tan_itrs = loc.get_itrs(obstime=t).cartesian.xyz.to_value(u.m).T  # (N,3)

    # Spacecraft state in inertial -> transform to ITRS
    rep = CartesianRepresentation(
        r_sc_eci_m[:, 0] * u.m, r_sc_eci_m[:, 1] * u.m, r_sc_eci_m[:, 2] * u.m
    )
    dif = CartesianDifferential(
        v_sc_eci_mps[:, 0] * u.m / u.s,
        v_sc_eci_mps[:, 1] * u.m / u.s,
        v_sc_eci_mps[:, 2] * u.m / u.s,
    )
    sc_gcrs = SkyCoord(rep.with_differentials(dif), frame=GCRS(obstime=t))

    sc_itrs = sc_gcrs.transform_to(ITRS(obstime=t))
    r_sc_itrs = sc_itrs.cartesian.xyz.to_value(u.m).T  # (N,3)
    v_sc_itrs = sc_itrs.velocity.d_xyz.to_value(u.m / u.s).T  # (N,3)

    # LOS in ITRS
    dr = r_tan_itrs - r_sc_itrs
    uhat = dr / np.linalg.norm(dr, axis=1, keepdims=True)

    vgeo = np.sum(v_sc_itrs * uhat, axis=1)  # sign depends on your LOS convention
    return -vgeo


def vgeo_from_att_variants(dt_utc, r_sc_eci_m, v_sc_eci_mps, tan_llh_km):
    t = Time(dt_utc.to_numpy())

    # Tangent point in ITRS (ECEF)
    lat = tan_llh_km[:, 0] * u.deg
    lon = tan_llh_km[:, 1] * u.deg
    h = (tan_llh_km[:, 2] * 1000.0) * u.m
    loc = EarthLocation.from_geodetic(lon=lon, lat=lat, height=h)
    tan_itrs = loc.get_itrs(obstime=t)

    r_tan_itrs = tan_itrs.cartesian.xyz.to_value(u.m).T  # (N,3)

    # Spacecraft state in inertial (GCRS) with velocity
    rep = CartesianRepresentation(
        r_sc_eci_m[:, 0] * u.m, r_sc_eci_m[:, 1] * u.m, r_sc_eci_m[:, 2] * u.m
    )
    dif = CartesianDifferential(
        v_sc_eci_mps[:, 0] * u.m / u.s,
        v_sc_eci_mps[:, 1] * u.m / u.s,
        v_sc_eci_mps[:, 2] * u.m / u.s,
    )
    sc_gcrs = SkyCoord(rep.with_differentials(dif), frame=GCRS(obstime=t))

    # 1) ITRS version (what you have)
    sc_itrs = sc_gcrs.transform_to(ITRS(obstime=t))
    r_sc_itrs = sc_itrs.cartesian.xyz.to_value(u.m).T
    v_sc_itrs = sc_itrs.velocity.d_xyz.to_value(u.m / u.s).T

    dr_itrs = r_tan_itrs - r_sc_itrs
    uhat_itrs = dr_itrs / np.linalg.norm(dr_itrs, axis=1, keepdims=True)
    v_itrs = np.sum(v_sc_itrs * uhat_itrs, axis=1)

    # 2) Inertial LOS relative to co-rotating air at tangent
    r_sc_gcrs = sc_gcrs.cartesian.xyz.to_value(u.m).T
    v_sc_gcrs = sc_gcrs.velocity.d_xyz.to_value(u.m / u.s).T

    tan_gcrs = tan_itrs.transform_to(GCRS(obstime=t))
    r_tan_gcrs = tan_gcrs.cartesian.xyz.to_value(u.m).T

    dr_gcrs = r_tan_gcrs - r_sc_gcrs
    uhat_gcrs = dr_gcrs / np.linalg.norm(dr_gcrs, axis=1, keepdims=True)

    # omega x r in ITRS then rotate to GCRS is simplest if we stay in ITRS:
    # air velocity in ITRS is ~0, but in inertial it is omega x r (expressed in ITRS).
    # We can compute it in ITRS and transform that differential via the frame.
    omega = np.array([0.0, 0.0, OMEGA_E])
    v_air_itrs = np.cross(omega[None, :], r_tan_itrs)  # m/s in ITRS components

    # transform that velocity vector to GCRS components using the rotation at time t
    # easiest hack: represent as a differential attached to r_tan_itrs and transform
    rep_t = CartesianRepresentation(
        r_tan_itrs[:, 0] * u.m, r_tan_itrs[:, 1] * u.m, r_tan_itrs[:, 2] * u.m
    )
    dif_air = CartesianDifferential(
        v_air_itrs[:, 0] * u.m / u.s,
        v_air_itrs[:, 1] * u.m / u.s,
        v_air_itrs[:, 2] * u.m / u.s,
    )
    air_itrs = SkyCoord(rep_t.with_differentials(dif_air), frame=ITRS(obstime=t))
    air_gcrs = air_itrs.transform_to(GCRS(obstime=t))
    v_air_gcrs = air_gcrs.velocity.d_xyz.to_value(u.m / u.s).T

    v_rel_air = np.sum((v_sc_gcrs - v_air_gcrs) * uhat_gcrs, axis=1)

    return -v_itrs, -v_rel_air
