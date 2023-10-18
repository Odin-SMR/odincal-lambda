import numpy as np


def sph2cart(azimuth, elevation, radius):
    rcos_theta = radius * np.cos(elevation)
    x_cart = rcos_theta * np.cos(azimuth)
    y_cart = rcos_theta * np.sin(azimuth)
    z_cart = radius * np.sin(elevation)
    return x_cart, y_cart, z_cart


def cart2sph(x_cart, y_cart, z_cart):
    hypot_xy = np.hypot(x_cart, y_cart)
    radius = np.hypot(hypot_xy, z_cart)
    elevation = np.arctan2(z_cart, hypot_xy)
    azimuth = np.arctan2(y_cart, x_cart)
    return azimuth, elevation, radius


def get_scan_geo_loc(lat_start, lon_start, lat_end, lon_end):
    lat_start = np.radians(lat_start)
    lon_start = np.radians(lon_start)
    lat_end = np.radians(lat_end)
    lon_end = np.radians(lon_end)

    [x_start, y_start, z_start] = sph2cart(lon_start, lat_start, 1)
    [x_end, y_end, z_end] = sph2cart(lon_end, lat_end, 1)
    [lon_mid, lat_mid, _] = cart2sph(
        (x_start + x_end) * 0.5,
        (y_start + y_end) * 0.5,
        (z_start + z_end) * 0.5,
    )

    lon_mid = np.degrees(lon_mid)
    lat_mid = np.degrees(lat_mid)

    return lat_mid, lon_mid
