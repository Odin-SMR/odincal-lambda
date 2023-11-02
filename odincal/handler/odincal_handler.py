import json
import os
import stat
import logging
from tempfile import mkdtemp

import boto3
from ac_coverage import assert_acfile_has_data_coverage
from log_configuration import logconfig
from odincal.level1b_window_importer2 import (
    preprocess_level1b,
    job_info_level1b,
    import_level1b,
)

logconfig()

ODINCAL_VERSION = 8


class InvalidMessage(Exception):
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


def notify_queue(
    sqs_client,
    notification_queue,
    scans,
):
    response = sqs_client.send_message(
        QueueUrl=notification_queue,
        MessageBody=json.dumps(
            {
                "scans": scans,
            }
        ),
    )
    if response["ResponseMetadata"]["HTTPStatusCode"] != 200:
        msg = "Notification failed for scans {0} with status {1}".format(
            scans,
            response["ResponseMetadata"],
        )
        raise NotifyFailed(msg)


def setup_postgres():
    pg_host_ssm_name = get_env_or_raise("ODIN_PG_HOST_SSM_NAME")
    pg_user_ssm_name = get_env_or_raise("ODIN_PG_USER_SSM_NAME")
    pg_pass_ssm_name = get_env_or_raise("ODIN_PG_PASS_SSM_NAME")
    pg_db_ssm_name = get_env_or_raise("ODIN_PG_DB_SSM_NAME")
    psql_bucket = get_env_or_raise("ODIN_PSQL_BUCKET_NAME")

    psql_dir = mkdtemp()
    s3_client = boto3.client("s3")

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
    )[
        "Parameter"
    ]["Value"]
    db_user = ssm_client.get_parameter(
        Name=pg_user_ssm_name,
        WithDecryption=True,
    )[
        "Parameter"
    ]["Value"]
    db_pass = ssm_client.get_parameter(
        Name=pg_pass_ssm_name,
        WithDecryption=True,
    )[
        "Parameter"
    ]["Value"]
    db_name = ssm_client.get_parameter(
        Name=pg_db_ssm_name,
        WithDecryption=True,
    )[
        "Parameter"
    ]["Value"]

    return "host={0} user={1} password={2} dbname={3} sslmode=verify-ca".format(  # noqa: E501
        db_host,
        db_user,
        db_pass,
        db_name,
    )


def preprocess_handler(event, context):
    logger = logging.getLogger("level1b.pre-process.handler")

    version = ODINCAL_VERSION
    ac_file = os.path.split(event["acFile"])[-1]
    backend = event["backend"].upper()

    logger.debug("setting up postgres before pre-processing of {0}".format(ac_file))
    pg_string = setup_postgres()

    logger.debug("checking attitude coverage for {0}".format(ac_file))
    assert_acfile_has_data_coverage(ac_file, backend, pg_string)

    logger.debug("starting pre-processing of {0}".format(ac_file))
    stw1, stw2 = preprocess_level1b(
        ac_file,
        backend,
        version,
        pg_string=pg_string,
    )

    return {
        "StatusCode": 200,
        "STW1": stw1,
        "STW2": stw2,
        "Backend": backend,
        "File": ac_file,
    }


def get_job_info_handler(event, context):
    logger = logging.getLogger("level1b.job_info.handler")
    logger.debug("starting job info handler")
    version = ODINCAL_VERSION
    ac_file = os.path.split(event["acFile"])[-1]
    backend = event["backend"].upper()
    stw1 = event["STW1"]
    stw2 = event["STW2"]

    pg_string = setup_postgres()

    scan_starts, soda_version = job_info_level1b(
        stw1,
        stw2,
        ac_file,
        backend,
        version,
        pg_string=pg_string,
    )

    return {
        "StatusCode": 200,
        "ScanStarts": scan_starts,
        "SodaVersion": soda_version,
        "Backend": backend,
        "File": ac_file,
    }


def import_handler(event, context):
    logger = logging.getLogger("level1b.import_l1b.handler")
    logger.debug("stating import handler")

    version = ODINCAL_VERSION
    ac_file = os.path.split(event["acFile"])[-1]
    backend = event["backend"].upper()
    soda_version = event["SodaVersion"]
    scan_starts = event["ScanStarts"]

    pg_string = setup_postgres()

    scans = import_level1b(
        scan_starts,
        soda_version,
        ac_file,
        backend,
        version,
        pg_string=pg_string,
    )

    return {
        "StatusCode": 200,
        "Scans": scans,
        "Backend": backend,
        "File": ac_file,
    }
