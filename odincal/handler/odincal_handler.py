import json
import os
import stat
from tempfile import mkdtemp

import boto3
from pg import DB

from odincal.calibration_preprocess import PrepareData
from odincal.level1b_window_importer2 import level1b_importer


ATT_BUFFER = 16 * 60 * 60 * 24 * 5  # Five day buffer
ODINCAL_VERSION = 8


class InvalidMessage(Exception):
    pass


class BadAttitude(Exception):
    pass


class NotifyFailed(Exception):
    pass


def download_file(
    s3_client,
    bucket_name,
    path_name,
    file_name,
):
    file_path = os.path.join(path_name, file_name)
    try:
        os.makedirs(path_name)
    except OSError:
        pass
    s3_client.download_file(
        bucket_name,
        file_name,
        file_path,
    )
    return file_path


def get_env_or_raise(variable_name):
    var = os.environ.get(variable_name)
    if var is None:
        raise EnvironmentError(
            "{0} is a required environment variable".format(
                variable_name,
            )
        )
    return var


def assert_has_attitude_coverage(
    ac_file, backend, version, con, buffer=ATT_BUFFER,
):
    prepare = PrepareData(ac_file, backend, version, con)
    ac_stw_start, ac_stw_end = prepare.get_stw_from_acfile()

    query = con.query(
        "select max(stw) as latest_att_stw from attitude_level0;"
    )

    result = query.dictresult()
    if result[0]["latest_att_stw"] - ac_stw_end < buffer:
        msg = "attitude data with STW {0} not recent enough for {1} with STW {2} to {3} (buffer required: {4})".format(  # noqa
            result[0]["latest_att_stw"],
            ac_file,
            ac_stw_start,
            ac_stw_end,
            buffer,
        )
        raise BadAttitude(msg)


def notify_queue(
    sqs_client,
    notification_queue,
    scans,
):
    response = sqs_client.send_message(
        QueueUrl=notification_queue,
        MessageBody=json.dumps({
            "scans": scans,
        }),
    )
    if response["ResponseMetadata"]["HTTPStatusCode"] != 200:
        msg = "Notification failed for scans {0} with status {1}".format(
            scans,
            response["ResponseMetadata"],
        )
        raise NotifyFailed(msg)


def handler(event, context):
    pg_host_ssm_name = get_env_or_raise("ODIN_PG_HOST_SSM_NAME")
    pg_user_ssm_name = get_env_or_raise("ODIN_PG_USER_SSM_NAME")
    pg_pass_ssm_name = get_env_or_raise("ODIN_PG_PASS_SSM_NAME")
    pg_db_ssm_name = get_env_or_raise("ODIN_PG_DB_SSM_NAME")
    psql_bucket = get_env_or_raise("ODIN_PSQL_BUCKET_NAME")
    notification_queue = get_env_or_raise("ODIN_L1_NOTIFICATIONS")
    version = ODINCAL_VERSION
    ac_file = os.path.split(event["acFile"])[-1]
    backend = event["backend"].upper()

    psql_dir = mkdtemp()
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
    os.environ["PGSSLCERT"] = pg_cert_path
    os.environ["PGSSLROOTCERT"] = root_cert_path
    os.environ["PGSSLKEY"] = pg_key_path

    ssm_client = boto3.client("ssm")
    db_host = ssm_client.get_parameter(
        Name=pg_host_ssm_name,
        WithDecryption=True,
    )["Parameter"]["Value"]
    db_user = ssm_client.get_parameter(
        Name=pg_user_ssm_name,
        WithDecryption=True,
    )["Parameter"]["Value"]
    db_pass = ssm_client.get_parameter(
        Name=pg_pass_ssm_name,
        WithDecryption=True,
    )["Parameter"]["Value"]
    db_name = ssm_client.get_parameter(
        Name=pg_db_ssm_name,
        WithDecryption=True,
    )["Parameter"]["Value"]
    
    pg_string = "host={0} user={1} password={2} dbname={3} sslmode=verify-ca".format(  # noqa: E501
        db_host,
        db_user,
        db_pass,
        db_name,
    )
    con = DB(pg_string)
    assert_has_attitude_coverage(ac_file, backend, version, con)
    scans = level1b_importer(ac_file, backend, version, con, pg_string)

    sqs_client = boto3.client("sqs")
    notify_queue(
        sqs_client,
        notification_queue,
        scans,
    )
    return {
        "success": True,
        "scans": len(scans),
        "backend": backend,
        "file": ac_file,
    }
