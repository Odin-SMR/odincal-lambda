import boto3
from logging.config import dictConfig
from yaml import safe_load

LOG_CONFIG_PARAMETER = "/odincal/logconf"


def logconfig():
    client = boto3.client("ssm")
    log_config = client.get_parameter(Name=LOG_CONFIG_PARAMETER)["Parameter"]["Value"]
    dictConfig(safe_load(log_config))
