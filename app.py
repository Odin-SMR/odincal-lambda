#!/usr/bin/env python3
from aws_cdk import App, Environment

from stacks.odincal_stack import OdincalStack

env_EU = Environment(account="991049544436", region="eu-north-1")

app = App()

OdincalStack(
    app,
    "OdinSMROdincalStack",
    level0_queue_arn="OdimSMRLevel0ACFilesQueue",
    pg_host_ssm_name="/odin/psql/host",
    pg_user_ssm_name="/odin/psql/user",
    pg_pass_ssm_name="/odin/psql/password",
    pg_db_ssm_name="/odin/psql/db",
    psql_bucket_name="odin-psql",
    env=env_EU,
)

app.synth()
