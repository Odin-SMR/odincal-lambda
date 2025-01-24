from os import environ
import boto3
import pytest


@pytest.fixture(autouse=True, scope="session")
def aws_environment():
    profile_name = "odin-cdk"
    session = boto3.Session(profile_name=profile_name)
    credentials = session.get_credentials()
    environ["AWS_SECRET_ACCESS_KEY"] = credentials.secret_key
    environ["AWS_ACCESS_KEY_ID"] = credentials.access_key
