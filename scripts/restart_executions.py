from typing import List, NewType, TypedDict
import boto3
import json
from datetime import datetime
from os import environ
import pandas as pd

if not environ.get("AWS_PROFILE"):
    environ["AWS_PROFILE"] = "odin-cdk"

STATE_MACHINE = (
    "arn:aws:states:eu-north-1:991049544436:stateMachine:OdinSMROdincalStateMachine"
)


class State(TypedDict):
    job: str
    status: str
    date: datetime | None


sfn_client = boto3.client("stepfunctions")
History = NewType[List[State]]

def get_history(nmax: int) -> History:
    history: History = []
    paginator = sfn_client.get_paginator("list_executions")
    response_iterator = paginator.paginate(
        stateMachineArn=STATE_MACHINE,
        PaginationConfig={
            "MaxItems": nmax,
            "PageSize": 100,
        },
    )
    for response in response_iterator:
        for exec in response["executions"]:
            info = sfn_client.describe_execution(executionArn=exec["executionArn"])
            input = json.loads(info["input"])
            if "name" in input:
                history.append(
                    State(
                        job=input["name"],
                        status=exec["status"],
                        date=exec.get("stopDate", None),
                    )
                )
    return history

def run_last_failed(hist: History) -> None:
    df = pd.DataFrame.from_dict(hist)
    df.sort_values(by=["job", "date"]).drop_duplicates(subset="job", keep="last")
    jobs = df[df.status == "FAILED"]["job"].to_list()
    for j in jobs:
        type = "ac2"
        if j.startswith("ac1"):
            type = "ac1"
        sfn_input = json.dumps(dict(name=j, type=type))
        sfn_client.start_execution(stateMachineArn=STATE_MACHINE, input=sfn_input)


if __name__ =="__main__":
    hist = get_history(1000)
    run_last_failed(hist)