from datetime import datetime
from pg import DB
from pandas import DataFrame
import logging

logger = logging.getLogger("odincal.skh_level1_importer")


def find_nearest(df, stw):
    index = df.index.searchsorted(stw)
    try:
        data = df.iloc[index]
    except IndexError:
        return None
    real_stw = data.name
    if (real_stw > stw + 2080) or (real_stw < stw - 2080):
        return None
    return data.shk_value


def shk_level1_importer(stwa, stwb, backend, pg_string):
    con = DB(pg_string)
    query = con.query(
        """\
        select stw,backend,frontend from ac_level0
        where ac_level0.stw>=$1 and ac_level0.stw<=$2
        and backend=$3""",
        (stwa, stwb, backend),
    )

    sigresult = query.dictresult()
    logger.debug("Got %i spectrum to import [%i,%i]", len(sigresult), stwa, stwb)
    query = con.query(
        """\
        select stw,shk_type,shk_value from shk_level0
        where stw between $1 and $2
        order by stw""",
        (stwa - 2080, stwb + 2080),
    )
    shk_rows = query.dictresult()
    logger.debug("Got %i SHK-rows for STW [%i,%i]", len(shk_rows), stwa, stwb)
    df = DataFrame.from_dict(shk_rows).set_index("stw")
    gr = df.groupby("shk_type")
    table_data = []
    logger.debug("starting matching process")
    for sig in sigresult:
        data = gr.apply(find_nearest, sig["stw"])
        # For the split modes and the currently accepted
        # frontend configurations, AC1 will hold REC_495 data in its lower half
        # and REC_549 in its upper half. AC2 will hold REC_572 data in its
        # lower half and REC_555 in its upper half.
        if sig["frontend"] == "SPL":
            if sig["backend"] == "AC1":
                frontends = ["495", "549"]
            if sig["backend"] == "AC2":
                frontends = ["572", "555"]
        else:
            frontends = [sig["frontend"]]
        for frontend in frontends:
            shkdict = {
                "stw": sig["stw"],
                "backend": sig["backend"],
                "frontendsplit": frontend,
                "imageloada": data.get("imageloadA", None),
                "imageloadb": data.get("imageloadB", None),
                "hotloada": data.get("hotloadA", None),
                "hotloadb": data.get("hotloadB", None),
                "mixera": data.get("mixerA", None),
                "mixerb": data.get("mixerB", None),
                "lnaa": data.get("lnaA", None),
                "lnab": data.get("lnaB", None),
                "mixer119a": data.get("119mixerA", None),
                "mixer119b": data.get("119mixerB", None),
                "warmifa": data.get("warmifA", None),
                "warmifb": data.get("warmifB", None),
                "created": datetime.now(),
            }
            if frontend != "119":
                shkdict["lo"] = data["LO" + frontend]
                shkdict["ssb"] = data["SSB" + frontend]
                shkdict["mixc"] = data["mixC" + frontend]
            table_data.append(shkdict)
    logger.debug("upserting %s rows", len(table_data))
    con.start()
    for data in table_data:
        con.upsert("shk_level1", data)
    con.end()
    logger.debug("finished SHK level1 importer")

    con.close()


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    psql_connection_string = "postgresql://odin@127.0.0.1/odin?sslmode=verify-ca"
    shk_level1_importer(14166160221,14166833456, "AC2", psql_connection_string)