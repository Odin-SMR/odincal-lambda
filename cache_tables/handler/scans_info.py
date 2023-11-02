import datetime as dt
from tempfile import TemporaryDirectory
from typing import Any

import requests
from requests.exceptions import HTTPError, RequestException

from .odin_connection import odin_connection, setup_postgres
from .ssm_parameters import get_parameters
from .log_configuration import logconfig

logconfig()

API_BASE = "https://odin-smr.org"
MAX_RETRIES = 3
SLEEP_TIME = 60  # seconds


def add_to_database(
    cursor,
    day: dt.date,
    freqmode: int,
    backend: str,
    altend: float,
    altstart: float,
    datetime_i: dt.datetime,
    latend: float,
    latstart: float,
    lonend: float,
    lonstart: float,
    mjdend: float,
    mjdstart: float,
    numspec: int,
    scanid: int,
    sunzd: float,
    quality: int,
) -> None:
    """Add an entry to the database, update if necessary"""

    cursor.execute(
        'insert into scans_cache '
        'values(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)'
        'on conflict(date, freqmode, scanid) '
        'do update set '
        'altend=EXCLUDED.altend, altstart=EXCLUDED.altstart, '
        'latend=EXCLUDED.latend, latstart=EXCLUDED.latstart, '
        'lonend=EXCLUDED.lonend, lonstart=EXCLUDED.lonstart, '
        'mjdend=EXCLUDED.mjdend, mjdstart=EXCLUDED.mjdstart, '
        'numspec=EXCLUDED.numspec, '
        'sunzd=EXCLUDED.sunzd, '
        'datetime=EXCLUDED.datetime, '
        'created=EXCLUDED.created, '
        'quality=EXCLUDED.quality ',
        (
            day,
            freqmode, backend, scanid,
            altend, altstart,
            latend, latstart,
            lonend, lonstart,
            mjdend, mjdstart,
            numspec,
            sunzd,
            datetime_i,
            dt.datetime.now(),
            quality,
        ),
    )


class RetriesExhaustedError(Exception):
    pass


def get_odin_data(
    endpoint: str,
    api_base: str = API_BASE,
) -> dict:
    url = f"{api_base}/{endpoint}"
    try:
        response = requests.get(url, timeout=365)
        response.raise_for_status()
    except (HTTPError, RequestException) as msg:
        raise RetriesExhaustedError(
            f"Retries exhausted for {url} ({msg})"
        )
    return response.json()


def update_scans(
    pg_credentials,
    date_info: dict[str, Any],
) -> list[dict[str, Any]]:
    """Populate database with 'cached' scans for each day.
    """

    all_scans = []
    freqmode = date_info["FreqMode"]
    if freqmode == 0:
        return []
    date_str = date_info["DateString"]

    scans_api = f"rest_api/v5/freqmode_raw/{date_str}/{freqmode}/"
    scan_data = get_odin_data(scans_api)

    db_connection = odin_connection(pg_credentials)
    db_cursor = db_connection.cursor()

    for scan in scan_data["Data"]:
        if scan["ScanID"] in date_info["Scans"]:
            add_to_database(
                db_cursor,
                dt.date.fromisoformat(date_str),
                date_info['FreqMode'],
                date_info['Backend'],
                scan["AltEnd"],
                scan["AltStart"],
                scan["DateTime"],
                scan["LatEnd"],
                scan["LatStart"],
                scan["LonEnd"],
                scan["LonStart"],
                scan["MJDEnd"],
                scan["MJDStart"],
                scan["NumSpec"],
                scan["ScanID"],
                scan["SunZD"],
                scan["Quality"],
            )
            all_scans.append(scan)

    db_connection.commit()
    db_cursor.close()
    db_connection.close()

    return all_scans


def handler(event: dict[str, Any], context: Any) -> dict[str, Any]:
    pg_credentials = get_parameters(
        [
            "/odin/psql/user",
            "/odin/psql/db",
            "/odin/psql/host",
            "/odin/psql/password",
        ]
    )

    with TemporaryDirectory(
        "psql",
        "/tmp/",
    ) as psql_dir:
        setup_postgres(psql_dir)

        scans = update_scans(pg_credentials, event)

    if len(scans) == 0:
        return {
            "StatusCode": 204,
            "ScansInfo": [],
        }

    scans = [
        {
            "AltStart": scan["AltStart"],
            "AltEnd": scan["AltEnd"],
            "LatStart": scan["LatStart"],
            "LatEnd": scan["LatEnd"],
            "LonStart": scan["LonStart"],
            "LonEnd": scan["LonEnd"],
            "MJDStart": scan["MJDStart"],
            "MJDEnd": scan["MJDEnd"],
            "ScanID": scan["ScanID"],
            "FreqMode": event["FreqMode"],
            "Backend": event["Backend"],
        }
        for scan in scans
    ]

    return {
        "StatusCode": 200,
        "ScansInfo": scans,
    }
