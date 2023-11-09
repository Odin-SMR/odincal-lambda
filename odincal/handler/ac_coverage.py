import pandas as pd
from odindb import squeeze_query
from pg import DB


class NoLevel0Data(Exception):
    pass


class MissingAttitude(Exception):
    pass


class MissingSHK(Exception):
    pass


class MissingACNeighbours(Exception):
    pass


def assert_acfile_has_data_coverage(
    ac_file,
    backend,
    pg_string,
):
    bin_query = squeeze_query(
        """
        WITH ac AS (
            SELECT max(stw) AS max,
                min(stw) AS min
            FROM ac_level0
            WHERE file = $4
        ),
        buffer AS (
            SELECT min - 17 * 60 * 60 * $1 AS min,
                max + 17 * 60 * 60 * $1 AS max
            from ac
        ),
        bin_size AS (
            SELECT 17 * 60 * $2 as bin
        ),
        ac_file_count AS (
            SELECT count(DISTINCT file) AS ac_file_cnt
            FROM ac_level0,
                buffer
            WHERE backend = $3
                AND stw BETWEEN buffer.min AND buffer.max
        ),
        att_count AS (
            SELECT floor(stw /bin) * bin as stw_bin,
                count(*) AS att_cnt
            FROM attitude_level0,
                buffer,
                bin_size
            WHERE stw BETWEEN buffer.min AND buffer.max
            GROUP BY 1
            ORDER BY 1
        ),
        shk_count AS (
            SELECT floor(stw /bin) * bin as stw_bin,
                count(*) AS shk_cnt
            FROM shk_level0,
                buffer,
                bin_size
            WHERE stw BETWEEN buffer.min AND buffer.max
            GROUP BY 1
            ORDER BY 1
        ),
        binned as (
            select shk_count.stw_bin,
                shk_cnt,
                att_cnt
            from shk_count
                LEFT JOIN att_count using (stw_bin)
        )
        select *
        from binned,
            ac_file_count
    """
    )
    con = DB(pg_string)
    buffer_hours = 2
    bin_minutes = 15
    query = con.query(bin_query, (buffer_hours, bin_minutes, backend, ac_file))
    result = query.dictresult()
    if len(result) == 0:
        msg = "No data available for file {2} in the period {0:x}({0}) to {1:x}({1})".format(
            int(start), int(end), ac_file
        )
        raise NoLevel0Data(msg)
    df = pd.DataFrame.from_dict(result)
    con.close()
    start = df.stw_bin.min()
    end = df.stw_bin.max()
    if df.iloc[0].ac_file_cnt < 3:
        msg = "File {2} has missing AC-file neighbours in the period {0:x}({0}) to {1:x}({1})".format(
            int(start), int(end), ac_file
        )
        raise MissingACNeighbours(msg)

    if not df[df.att_cnt == 0].empty or not df[df.att_cnt.isnull()].empty:
        msg = "File {2} has missing attitude data in the period {0:x}({0}) to {1:x}({1})".format(
            int(start), int(end), ac_file
        )
        raise MissingAttitude(msg)

    if not df[df.shk_cnt == 0].empty:
        msg = "File {2} has missing SHK data in the period {0:x}({0}) to {1:x}({1})".format(
            start, end, ac_file
        )
        raise MissingSHK(msg)
