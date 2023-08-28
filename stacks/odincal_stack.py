from aws_cdk import Duration, Stack
from aws_cdk import aws_stepfunctions as sfn
from aws_cdk import aws_stepfunctions_tasks as tasks
from aws_cdk.aws_ec2 import Vpc, SubnetSelection, SubnetType
from aws_cdk.aws_iam import Effect, PolicyStatement
from aws_cdk.aws_lambda import (
    Architecture,
    DockerImageCode,
    DockerImageFunction,
)
from aws_cdk.aws_s3 import Bucket
from aws_cdk.aws_sqs import DeadLetterQueue, Queue
from constructs import Construct


class OdincalStack(Stack):
    def __init__(
        self,
        scope: Construct,
        id: str,
        ssm_root: str,
        psql_bucket_name: str,
        queue_retention_period: Duration = Duration.days(14),
        message_timeout: Duration = Duration.hours(12),
        message_attempts: int = 4,
        lambda_timeout: Duration = Duration.seconds(900),
        **kwargs
    ) -> None:
        super().__init__(scope, id, **kwargs)

        # Set up VPC
        vpc = Vpc.from_lookup(
            self,
            "OdinSMRCalibrateLevel1lVPC",
            is_default=False,
            vpc_name="OdinVPC",
        )
        vpc_subnets = SubnetSelection(
            subnet_type=SubnetType.PRIVATE_WITH_EGRESS
        )

        # Set up notification queue for Level 2
        queue_name = "OdinSMROdincalNotificationQueue"
        notification_queue = Queue(
            self,
            queue_name,
            queue_name=queue_name,
            retention_period=queue_retention_period,
            visibility_timeout=message_timeout,
            dead_letter_queue=DeadLetterQueue(
                max_receive_count=message_attempts,
                queue=Queue(
                    self,
                    "Failed" + queue_name,
                    queue_name="Failed" + queue_name,
                    retention_period=queue_retention_period,
                ),
            ),
        )

        # Set up Lambda functions
        calibrate_level1_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalLambda",
            code=DockerImageCode.from_image_asset(
                "./odincal",
            ),
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            memory_size=1024,
            environment={
                "ODIN_PG_HOST_SSM_NAME": f"{ssm_root}/host",
                "ODIN_PG_USER_SSM_NAME": f"{ssm_root}/user",
                "ODIN_PG_PASS_SSM_NAME": f"{ssm_root}/password",
                "ODIN_PG_DB_SSM_NAME": f"{ssm_root}/db",
                "ODIN_PSQL_BUCKET_NAME": psql_bucket_name,
                "ODIN_L1_NOTIFICATIONS": notification_queue.queue_name,
            },
        )

        calibrate_level1_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[f"arn:aws:ssm:*:*:parameter{ssm_root}/*"]
        ))

        notification_queue.grant_send_messages(calibrate_level1_lambda)

        # Set up tasks
        calibrate_level1_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCalibrateLevel1Task",
            lambda_function=calibrate_level1_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "acFile": sfn.JsonPath.string_at("$.name"),
                    "backend": sfn.JsonPath.string_at("$.type"),
                },
            ),
            result_path="$.CalibrateLevel1",
        )
        calibrate_level1_task.add_retry(
            errors=["BadAttitude"],
            max_attempts=3,
            backoff_rate=2,
            interval=Duration.days(5),
        )

        # Set up workflow
        calibrate_level1_fail_state = sfn.Fail(
            self,
            "OdinSMRCalibrateLevel1Fail",
            comment="Somthing went wrong when importing Level 0 file",
        )
        calibrate_level1_success_state = sfn.Succeed(
            self,
            "OdinSMRCalibrateLevel1Success",
        )
        check_calibrate_status_state = sfn.Choice(
            self,
            "OdinSMRCheckCalibrateLevel1Status",
        )
        check_calibrate_status_state.when(
            sfn.Condition.boolean_equals(
                "$.CalibrateLevel1.Payload.success",
                True,
            ),
            calibrate_level1_success_state,
        )
        check_calibrate_status_state.otherwise(calibrate_level1_fail_state)
        calibrate_level1_task.next(check_calibrate_status_state)

        sfn.StateMachine(
            self,
            "OdinSMROdincalStateMachine",
            definition=calibrate_level1_task,
            state_machine_name="OdinSMROdincalStateMachine",
        )

        # Set up additional permissions
        psql_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalPsqlBucket",
            psql_bucket_name,
        )

        psql_bucket.grant_read(calibrate_level1_lambda)
