import numpy as np
import xarray
from pandas import to_datetime  # type: ignore

from .atmos import intatm, IDEALGAS
from .msis90 import Msis90 as M90


MSISZ = np.r_[49.0, 50.0, 51, np.arange(75, 151, 1)]


class Donaletty:
    """
    Create ZPT2 file using the ERA5 data
    Created
        Donal Murtagh July 2011.

    """

    def donaletty(self, scan_data, newz):
        """Inputs :
                scan_data : heights (km), pressure (hPa) and temperature
                            profiles, coordinates and times for scans; should
                            extend to at least 60 km
                newz : heights to interpolate to
        Output : ZPT array [3, 0-120]
        """

        # fix Msis from 70-150 km with solar effects
        msis = M90()
        # to ensure an ok stratopause Donal wants to add a 50 km point
        # from msis
        scan_datetime = to_datetime(scan_data.DateMid.data).to_pydatetime()
        msisT = msis.extractPTZprofilevarsolar(
            scan_datetime,
            scan_data.LatMid,
            scan_data.LonMid,
            MSISZ,
        )[1]
        z = np.r_[scan_data.era5_gmh.data, MSISZ]
        temp = np.r_[scan_data.era5_t, msisT]
        normrho = (
            np.interp([20], scan_data.era5_gmh.data, scan_data.era5_level.data)
            * 28.9644
            / 1000
            / IDEALGAS
            / np.interp([20], scan_data.era5_gmh.data, scan_data.era5_t.data)
        )
        newT, newp, _, _, _, _, _ = intatm(
            z, temp, newz, 20, normrho[0], scan_data.LatMid.item(),
        )
        zpt = xarray.Dataset(
            data_vars=dict(
                p=(["z"], newp),
                t=(["z"], newT),
            ),
            coords=dict(
                z=(["z"], newz),
            ),
        )
        return zpt

    def makeprofile(self, scans: xarray.DataArray):
        ecmz = np.arange(45)
        newz = np.arange(151)
        scan_on_interp_gmh = scans.groupby("ScanID").map(
            lambda ds: ds.squeeze().swap_dims(level="era5_gmh").interp(
                era5_gmh=ecmz, kwargs={"fill_value": 273}
            )
        )
        zpt_donaletty = scan_on_interp_gmh.groupby("ScanID").map(
            self.donaletty, args=(newz,)
        )
        return zpt_donaletty
