import numpy as np

from odin.level1.geometry import (
    ecef_to_geodetic_xyz,
    geodetic_to_ecef_llh,
)


def test_geodetic_ecef_roundtrip():
    """
    Round-trip test:
    geodetic -> ECEF -> geodetic

    Uses realistic SMR tangent-point values.
    """

    # lat [deg], lon [deg], height [m]
    llh = np.array([
        [-40.486, 328.373, 32_281.0],   # typical tangent point
        [  0.000,   0.000,      0.0],   # equator, sea level
        [ 45.000, 120.000, 10_000.0],   # mid-latitude
        [-89.999,  10.000, 30_000.0],   # near south pole
    ])

    xyz = geodetic_to_ecef_llh(llh)
    llh_rt = ecef_to_geodetic_xyz(xyz)

    # latitude & longitude: ~1e-8 deg ≈ millimeter-level
    np.testing.assert_allclose(
        llh_rt[:, 0], llh[:, 0], atol=1e-8
    )
    np.testing.assert_allclose(
        llh_rt[:, 1], llh[:, 1], atol=1e-8
    )

    # height: allow centimeter-level numerical noise
    np.testing.assert_allclose(
        llh_rt[:, 2], llh[:, 2], atol=1e-2
    )


def test_tangent_altitude_preserved():
    llh_km = np.array(
        [
            [-40.5, 328.4, 32.0],  # km
            [-10.0, 45.0, 85.0],
        ]
    )

    llh_m = llh_km.copy()
    llh_m[:, 2] *= 1000.0

    xyz = geodetic_to_ecef_llh(llh_m)
    llh_rt = ecef_to_geodetic_xyz(xyz)

    assert np.allclose(llh_rt[:, 2] / 1000.0, llh_km[:, 2], atol=1e-5)
