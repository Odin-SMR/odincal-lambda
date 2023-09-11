#! /usr/bin/env python3.8
"""
Part of odin-api, tools to make it happen
"""
from os import environ
from time import sleep
from datetime import date, timedelta, datetime
from argparse import ArgumentParser
from psycopg2 import connect
from requests import get
from requests.exceptions import HTTPError
from dateutil import parser as date_parser


def odin_connection():
    """Connects to the database, returns a connection"""
    connection_string = (
        "host={0} ".format(environ.get("PGHOST")) +
        "dbname={0} ".format(environ.get("PGDBNAME")) +
        "user={0} ".format(environ.get("PGUSER")) +
        "password={0}".format(environ.get("PGPASS"))
    )
    connection = connect(connection_string)
    return connection


def delete_day_from_database(cursor, day):
    """Delete all entries for a single day from database"""
    cursor.execute(
        """
        delete from scans_cache
        where date=%s
        """,
        (day,))


def add_to_database(cursor, day, freqmode, backend, altend, altstart,
                    datetime_i, latend, latstart, lonend, lonstart, mjdend,
                    mjdstart, numspec, scanid, sunzd, quality):
    """Add an entry to the database.

    To avoid conflicts, make sure to delete the old ones first, e.g. by calling
    delete_day_from_database()!"""
    cursor.execute(
        'insert into scans_cache values(%s,%s,%s,%s,%s,%s,%s,%s,'
        '%s,%s,%s,%s,%s,%s,%s,%s,%s)',
        (day, freqmode, backend, scanid, altend, altstart, latend, latstart,
         lonend, lonstart, mjdend, mjdstart, numspec, sunzd, datetime_i,
         datetime.now(), quality))


def setup_arguments():
    """setup command line arguments"""
    parser = ArgumentParser(description="Repopulate the cached data table")
    parser.add_argument("-s", "--start", dest="start_date", action="store",
                        default=(date.today()-timedelta(days=42)).isoformat(),
                        help="start of period to look for new data "
                        "(default: one month back)")
    parser.add_argument("-e", "--end", dest="end_date", action="store",
                        default=date.today().isoformat(),
                        help="end of period to look for new data "
                        "(default: today)")
    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true",
                        help="use verbose output")
    return parser


def main(start_date=date.today()-timedelta(days=42), end_date=date.today(),
         verbose=False):
    """Script to populate database with 'cached' info.

    Walks backwards from end_date to start_date."""
    max_retries = 3
    sleep_time = 60
    step = timedelta(days=-1)
    current_date = end_date
    earliest_date = start_date
    db_connection = odin_connection()
    db_cursor = db_connection.cursor()
    if 'ODIN_API_PRODUCTION' not in environ:
        url_base = 'http://localhost:5000'
    else:
        url_base = 'https://odin.rss.chalmers.se'
    while current_date >= earliest_date:
        if verbose:
            print("Working on {0}".format(current_date))
        url_day = (
            '{0}/rest_api/v5/freqmode_info/{1}/'.format(
                url_base,
                current_date.isoformat())
            )
        response = get(url_day, timeout=60)
        retries = max_retries
        while retries > 0:
            try:
                response.raise_for_status()
                break
            except HTTPError as msg:
                print("{0} {1} {2}".format(current_date, msg, url_day))
                retries -= 1
                print("Retries left {0}".format(retries))
                sleep(sleep_time * 2 ** (max_retries - retries - 1))
        if retries == 0:
            print("* FAILED: {} {}".format(current_date, url_day))
            continue

        delete_day_from_database(db_cursor, current_date.isoformat())
        db_connection.commit()
        json_data_day = response.json()
        for freqmode in json_data_day['Data']:
            if freqmode['FreqMode'] == 0:
                if verbose:
                    print("Skipping FreqMode 0 on {}".format(current_date))
                continue
            if verbose:
                print("Working on {0}".format(freqmode['FreqMode']))
            url_scan = (
                '{0}/rest_api/v5/freqmode_raw/{1}/{2}/'.format(
                    url_base,
                    current_date.isoformat(),
                    freqmode['FreqMode'])
                )
            retries = max_retries
            while retries > 0:
                response = get(url_scan, timeout=420)
                try:
                    response.raise_for_status()
                    break
                except HTTPError as msg:
                    print("{0} {1} {2}".format(current_date, msg, url_scan))
                    retries -= 1
                    print("Retries left {0}".format(retries))
                    sleep(sleep_time * 2 ** (max_retries - retries - 1))
            if retries == 0:
                print("* FAILED: {} {}".format(current_date, url_day))
                continue

            json_data_scan = response.json()
            for scan in json_data_scan['Data']:
                add_to_database(
                    db_cursor,
                    json_data_day['Date'],
                    freqmode['FreqMode'],
                    freqmode['Backend'],
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
            db_connection.commit()
            if verbose:
                print("freqmode {0} OK".format(freqmode["FreqMode"]))
        if verbose:
            print("{0} OK".format(current_date))
        current_date += step

    db_cursor.close()
    db_connection.close()


def cli():
    """run the main application"""
    parser = setup_arguments()
    args = parser.parse_args()

    try:
        start_date = date_parser.parse(args.start_date).date()
    except TypeError:
        print("Could not understand start date {0}".format(args.start_date))
        exit(1)

    try:
        end_date = date_parser.parse(args.end_date).date()
    except TypeError:
        print("Could not understand end date {0}".format(args.end_date))
        exit(1)

    try:
        assert end_date > start_date
    except AssertionError:
        print("End date must be after start date!")
        print("Got: start {0}, end {1}".format(args.start_date, args.end_date))
        exit(1)

    exit(main(start_date, end_date, args.verbose))


if __name__ == '__main__':
    cli()
