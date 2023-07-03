from aws_cdk import Duration, Fn, Stack, Size
from aws_cdk.aws_ec2 import Vpc, SubnetSelection, SubnetType
from aws_cdk.aws_iam import Effect, PolicyStatement
from aws_cdk.aws_lambda import (
    Architecture,
    DockerImageCode,
    DockerImageFunction,
)
from aws_cdk.aws_lambda_event_sources import SqsEventSource
from aws_cdk.aws_s3 import Bucket
from aws_cdk.aws_sqs import DeadLetterQueue, Queue
from constructs import Construct


class OdincalStack(Stack):
    def __init__(
        self,
        scope: Construct,
        id: str,
        level0_queue_arn: str,
        pg_host_ssm_name: str,
        pg_user_ssm_name: str,
        pg_pass_ssm_name: str,
        pg_db_ssm_name: str,
        psql_bucket_name: str,
        queue_retention_period: Duration = Duration.days(14),
        message_timeout: Duration = Duration.hours(24),
        message_attempts: int = 4,
        lambda_timeout: Duration = Duration.seconds(900),
        **kwargs
    ) -> None:
        super().__init__(scope, id, **kwargs)

        vpc = Vpc.from_lookup(
            self,
            "OdinSMROdincalVPC",
            is_default=False,
            vpc_name="OdinVPC",
        )
        vpc_subnets = SubnetSelection(
            subnet_type=SubnetType.PRIVATE_WITH_EGRESS
        )

        queue_name = "OdinSMROdincalNotificationQueue"
        notification_queue = Queue(
            self,
            queue_name,
            queue_name=queue_name,
            retention_period=queue_retention_period,
            visibility_timeout=message_timeout,
            content_based_deduplication=True,
            dead_letter_queue=DeadLetterQueue(
                max_receive_count=message_attempts,
                queue=Queue(
                    self,
                    "Failed" + queue_name,
                    queue_name="Failed" + queue_name,
                    retention_period=queue_retention_period,
                    content_based_deduplication=True,
                ),
            ),
        )

        level1_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalLambda",
            code=DockerImageCode.from_image_asset(
                "./odincal",
            ),
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            ephemeral_storage_size=Size.mebibytes(512),
            memory_size=1024,
            environment={
                "ODIN_PG_HOST_SSM_NAME": pg_host_ssm_name,
                "ODIN_PG_USER_SSM_NAME": pg_user_ssm_name,
                "ODIN_PG_PASS_SSM_NAME": pg_pass_ssm_name,
                "ODIN_PG_DB_SSM_NAME": pg_db_ssm_name,
                "ODIN_PSQL_BUCKET_NAME": psql_bucket_name,
                "ODIN_L1_NOTIFICATIONS": notification_queue.queue_name,
            },
        )

        for ssm_name in (
            pg_host_ssm_name,
            pg_user_ssm_name,
            pg_pass_ssm_name,
            pg_db_ssm_name,
        ):
            level1_lambda.add_to_role_policy(PolicyStatement(
                effect=Effect.ALLOW,
                actions=["ssm:GetParameter"],
                resources=[f"arn:aws:ssm:*:*:parameter{ssm_name}"]
            ))

        psql_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalPsqlBucket",
            psql_bucket_name,
        )

        psql_bucket.grant_read(level1_lambda)

        l0_jobs_queue = Queue.from_queue_arn(
            self,
            "OdinSMRLevel0JobsQueue",
            Fn.import_value(level0_queue_arn),
        )
        level1_lambda.add_event_source(SqsEventSource(
            l0_jobs_queue,
            batch_size=1,
        ))

        notification_queue.grant_send_messages(level1_lambda)
