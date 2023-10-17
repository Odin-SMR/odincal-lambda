import aws_cdk.aws_logs as logs
from constructs import Construct
from aws_cdk.aws_logs import RetentionDays


class OdinCalLogs(logs.LogGroup):
    def __init__(
        self,
        scope: Construct,
    ) -> None:
        super().__init__(
            scope,
            "OdinCalLogs",
            log_group_name="/Odin/OdinCal",
            retention=RetentionDays.ONE_MONTH,
        )
