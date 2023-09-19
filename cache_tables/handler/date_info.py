import datetime as dt
from tempfile import TemporaryDirectory
from typing import Any

from .odin_connection import odin_connection, setup_postgres
from .ssm_parameters import get_parameters
from .time_util import datetime2mjd, mjd2stw, stw2datetime


def gen_data(db_connection, query_string):
    query = db_connection.query(query_string)
    result = query.dictresult()
    info_list = []
    for row in result:
        info_dict = {}
        info_dict['Backend'] = row['backend']
        info_dict['FreqMode'] = row['freqmode']
        info_dict['NumScan'] = row['count']
        info_list.append(info_dict)
    return info_list


def gen_query(stw1, stw2):
    query_str = (
        "select freqmode, backend, count(distinct(stw)) "
        "from ac_cal_level1b "
        "where stw >= {0} and stw < {1} "
        "group by backend,freqmode "
        "order by backend,freqmode "
    ).format(stw1, stw2)
    return query_str


def get_date_info(db_connection, date: dt.date | dt.datetime):
    start_date = dt.datetime(date.year, date.month, date.day)
    end_date = start_date + dt.timedelta(days=1)
    mjd_start = int(datetime2mjd(start_date))
    mjd_end = int(datetime2mjd(end_date))
    stw_start = mjd2stw(mjd_start)
    stw_end = mjd2stw(mjd_end)
    query_str = gen_query(stw_start, stw_end)
    return gen_data(db_connection, query_str)


def add_to_database(
    cursor,
    day: dt.date,
    freqmode: int,
    numscans: int,
    backend: str,
):
    """Add an entry to the database, update if necessary"""
    cursor.execute(
        "insert into measurements_cache values(%s,%s,%s,%s)"
        "on conflict(date, freqmode)"
        "do update set nscans=greatest(EXCLUDED.nscans, nscans)",
        (day, freqmode, numscans, backend),
    )


def update_measurements(
    db_connection,
    start_date: dt.date,
    end_date: dt.date | None = None,
):
    """Populate database with 'cached' stats for measurements each day.

    Walks backwards from end_date to start_date, but typically run for one day.
    """
    step = dt.timedelta(days=-1)
    if end_date is None:
        current_date = start_date
    else:
        current_date = end_date
    earliest_date = start_date

    db_cursor = db_connection.cursor()
    freqmode_info: dict[str, list] = dict()
    while current_date >= earliest_date:
        date_info = get_date_info(db_connection, current_date)
        freqmode_info[current_date.isoformat()] = []
        for freqmode in date_info['Info']:
            add_to_database(
                db_cursor,
                current_date,
                freqmode['FreqMode'],
                freqmode['NumScan'],
                freqmode['Backend'],
            )
            freqmode_info[current_date.isoformat()].append(
                {
                    "FreqMode": freqmode["FreqMode"],
                    "Backend": freqmode["Backend"],
                    "NumScan": freqmode["NumScan"],
                }
            )
        db_connection.commit()
        current_date = current_date + step
    db_cursor.close()
    return freqmode_info


def handler(event: dict[str, list[int]], context: Any):
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
        db_connection = odin_connection(pg_credentials)

        dates = [stw2datetime(scan_id) for scan_id in event["Scans"]]
        date_info = update_measurements(db_connection, min(dates), max(dates))

        db_connection.close()

    return {
        "StatusCode": 200,
        "DateInfo": date_info,
    }
