import os
import stat
from pathlib import Path
from typing import Any

import boto3
from psycopg2 import connect

BotoClient = Any
S3Client = BotoClient

PSQL_BUCKET = "odin-psql"


def odin_connection(credentials):
    """Connects to the database, returns a connection"""
    connection_string = (
        "host={0} ".format(credentials.host) +
        "dbname={0} ".format(credentials.db) +
        "user={0} ".format(credentials.user) +
        "password={0} ".format(credentials.password) +
        "sslmode=verify-ca"
    )
    connection = connect(connection_string)
    return connection


def download_file(
    s3_client: S3Client,
    bucket_name: str,
    path_name: str,
    file_name: str,
) -> Path:
    file_path = Path(path_name) / file_name
    file_path.parent.mkdir(parents=True, exist_ok=True)
    s3_client.download_file(
        bucket_name,
        file_name,
        str(file_path),
    )
    return file_path


def setup_postgres(psql_dir: str, psql_bucket: str = PSQL_BUCKET) -> None:
    s3_client = boto3.client('s3')

    # Setup SSL for Postgres
    pg_cert_path = download_file(
        s3_client,
        psql_bucket,
        psql_dir,
        "postgresql.crt",
    )
    root_cert_path = download_file(
        s3_client,
        psql_bucket,
        psql_dir,
        "root.crt",
    )
    pg_key_path = download_file(
        s3_client,
        psql_bucket,
        psql_dir,
        "postgresql.key",
    )

    os.chmod(pg_key_path, stat.S_IWUSR | stat.S_IRUSR)
    os.environ["PGSSLCERT"] = str(pg_cert_path)
    os.environ["PGSSLROOTCERT"] = str(root_cert_path)
    os.environ["PGSSLKEY"] = str(pg_key_path)
