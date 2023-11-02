import boto3
from os import environ
from logging.config import dictConfig
from yaml import safe_load

def logconfig():
    ssm_client = boto3.client("ssm")
    log_ssm_parameter =  environ.get("ODIN_LOGCONFIG", None)
    log_config = "version: 1"
    if log_ssm_parameter:
        log_config = ssm_client.get_parameter(
            Name=log_ssm_parameter
        )["Parameter"]["Value"]
    dictConfig(safe_load(log_config))
