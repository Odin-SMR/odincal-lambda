from __future__ import annotations

import csv
import io
from collections.abc import Iterable

import numpy as np
import pandas as pd
import psycopg
from psycopg import sql


def query_df(sql: str, params: dict, conn_info: str) -> pd.DataFrame:
    with psycopg.connect(
        conn_info,
        connect_timeout=5,
    ) as conn:
        # pandas can use a DB-API connection
        return pd.read_sql(sql, conn, params=params)  # type: ignore[arg-type]


def _pg_array_literal(value) -> str | None:
    """
    Convert array-ish Python/NumPy values to a Postgres array literal: "{...}".
    Return None to emit SQL NULL in COPY.
    """
    if value is None:
        return None

    # pandas NA / numpy nan -> NULL
    try:
        if pd.isna(value):
            return None
    except Exception:
        pass

    # Structured numpy scalar: dtype([('q0','<f8'),...])
    if isinstance(value, np.void) and getattr(value.dtype, "names", None):
        vals = [value[name] for name in value.dtype.names]
        return "{" + ",".join(format(float(v), ".10g") for v in vals) + "}"

    # ndarray
    if isinstance(value, np.ndarray):
        arr = np.asarray(value, dtype=np.float64).ravel()
        return "{" + ",".join(format(float(v), ".10g") for v in arr) + "}"

    # list/tuple
    if isinstance(value, (list, tuple)):
        return "{" + ",".join(format(float(v), ".10g") for v in value) + "}"

    # already a string (assume already formatted)
    if isinstance(value, str):
        return value

    raise TypeError(f"Unsupported value for Postgres array column: {type(value)!r}")


def upsert_df(
    conn_str: str,
    df: pd.DataFrame,
    table: str,
    conflict_cols: list[str],
    update_cols: list[str] | None = None,
    schema: str = "public",
    array_cols: Iterable[str] = (),
    drop_stw_range: bool = False,
):
    if df.empty:
        return

    df = df.copy()

    if update_cols is None:
        update_cols = [c for c in df.columns if c not in conflict_cols]

    cols = list(df.columns)
    tmp_table = f"_tmp_upsert_{table}"

    # --- Generic handling of Postgres array columns ---
    array_cols = [c for c in array_cols if c in df.columns]
    for c in array_cols:
        df[c] = df[c].map(_pg_array_literal)

    # CSV buffer for COPY
    buf = io.StringIO()
    df.to_csv(
        buf,
        index=False,
        header=False,
        quoting=csv.QUOTE_MINIMAL,  # important: "{1,2,3}" contains commas->quote in CSV
        na_rep="",  # emty field, COPY CSV treats it as NULL by default
    )
    buf.seek(0)
    with psycopg.connect(conn_str) as conn:
        with conn.cursor() as cur:
            cur.execute(
                sql.SQL("DROP TABLE IF EXISTS {}.{}").format(
                    sql.Identifier(schema), sql.Identifier(tmp_table)
                )
            )
            cur.execute(
                sql.SQL(
                    obj=(
                        "CREATE TEMP TABLE {} (LIKE {}.{} "
                        "INCLUDING DEFAULTS) ON COMMIT DROP"
                    )
                ).format(
                    sql.Identifier(tmp_table),
                    sql.Identifier(schema),
                    sql.Identifier(table),
                )
            )

            copy_sql = sql.SQL(
                obj=("COPY {} ({}) FROM STDIN WITH (FORMAT csv)")
            ).format(
                sql.Identifier(tmp_table),
                sql.SQL(", ").join(map(sql.Identifier, cols)),
            )

            with cur.copy(copy_sql) as copy:
                copy.write(buf.getvalue())

            if drop_stw_range:
                min_stw = df["stw"].min()
                max_stw = df["stw"].max()
                delete_sql = sql.SQL("""
                    DELETE FROM {}.{}
                    WHERE stw BETWEEN %s AND %s
                """).format(
                    sql.Identifier(schema),
                    sql.Identifier(table),
                )
                cur.execute(delete_sql, (min_stw, max_stw))

            upsert_sql = sql.SQL("""
                INSERT INTO {}.{} ({})
                SELECT {} FROM {}
                ON CONFLICT ({})
                DO UPDATE SET {}
            """).format(
                sql.Identifier(schema),
                sql.Identifier(table),
                sql.SQL(", ").join(map(sql.Identifier, cols)),
                sql.SQL(", ").join(map(sql.Identifier, cols)),
                sql.Identifier(tmp_table),
                sql.SQL(", ").join(map(sql.Identifier, conflict_cols)),
                sql.SQL(", ").join(
                    sql.SQL("{} = EXCLUDED.{}").format(
                        sql.Identifier(c), sql.Identifier(c)
                    )
                    for c in update_cols
                ),
            )

            cur.execute(upsert_sql)

        conn.commit()
