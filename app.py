#!/usr/bin/env python3
from aws_cdk import App, Environment

from stacks.odincal_stack import OdincalStack

env_EU = Environment(account="991049544436", region="eu-north-1")

app = App()

OdincalStack(
    app,
    "OdinSMROdincalStack",
    level0_queue_arn="todo:insert:arn:for:OdimSMRLevel0ACFilesQueue:here",
    ssm_root="/odin/psql",
    psql_bucket_name="odin-psql",
    env=env_EU,
)

app.synth()
