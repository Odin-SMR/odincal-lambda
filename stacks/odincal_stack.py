from aws_cdk import Duration, Stack
from aws_cdk import aws_stepfunctions as sfn
from aws_cdk import aws_stepfunctions_tasks as tasks
from aws_cdk.aws_ec2 import Vpc, SubnetSelection, SubnetType
from aws_cdk.aws_iam import Effect, PolicyStatement
from aws_cdk.aws_lambda import (
    Architecture,
    Code,
    DockerImageCode,
    DockerImageFunction,
    Function,
    InlineCode,
    Runtime,
)
from aws_cdk.aws_s3 import Bucket
from constructs import Construct


class OdincalStack(Stack):
    def __init__(
        self,
        scope: Construct,
        id: str,
        ssm_root: str,
        psql_bucket_name: str,
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
            memory_size=2048,
            environment={
                "ODIN_PG_HOST_SSM_NAME": f"{ssm_root}/host",
                "ODIN_PG_USER_SSM_NAME": f"{ssm_root}/user",
                "ODIN_PG_PASS_SSM_NAME": f"{ssm_root}/password",
                "ODIN_PG_DB_SSM_NAME": f"{ssm_root}/db",
                "ODIN_PSQL_BUCKET_NAME": psql_bucket_name,
            },
        )

        calibrate_level1_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[f"arn:aws:ssm:*:*:parameter{ssm_root}/*"]
        ))

        date_info_lambda = Function(
            self,
            "OdinSMROdincalDateInfoLambda",
            code=Code.from_asset(
                "./cache_tables",
                bundling={
                    "image": Runtime.PYTHON_3_10.bundling_image,
                    "command": [
                        "bash",
                        "-c",
                        "pip install -r requirements.txt -t /asset-output && cp -au . /asset-output",
                    ],
                },
            ),
            handler="handler.date_info.handler",
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            runtime=Runtime.PYTHON_3_10,
        )

        scans_info_lambda = Function(
            self,
            "OdinSMROdincalScansInfoLambda",
            code=Code.from_asset(
                "./cache_tables",
                bundling={
                    "image": Runtime.PYTHON_3_10.bundling_image,
                    "command": [
                        "bash",
                        "-c",
                        "pip install -r requirements.txt -t /asset-output && cp -au . /asset-output",
                    ],
                },
            ),
            handler="handler.scans_info.handler",
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            runtime=Runtime.PYTHON_3_10,
        )

        activate_level2_lambda = Function(
            self,
            "OdinSMRLevel0Lambda",
            code=InlineCode.from_asset("./activate_l2"),
            handler="handler.activate_l2_handler.activate_l2_handler",
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            runtime=Runtime.PYTHON_3_10,
        )

        activate_level2_lambda.add_to_role_policy(
            PolicyStatement(
                actions=[
                    "states:ListStateMachines",
                    "states:StartExecution",
                ],
                resources=["*"],
            ),
        )

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
        date_info_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalDateInfoTask",
            lambda_function=date_info_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "Scans": sfn.JsonPath.string_at("$.CalibrateLevel1.Scans"),
                },
            ),
            result_path="$.DateInfo",
        )
        scans_info_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalScansInfoTask",
            lambda_function=scans_info_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "DateInfo": sfn.JsonPath.string_at("$.DateInfo.DateInfo"),
                },
            ),
            result_path="$.ScansInfo",
        )
        activate_level2_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalActivateLevel2Task",
            lambda_function=scans_info_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "Scans": sfn.JsonPath.string_at("$.ScansInfo.ScansInfo"),
                    "File": sfn.JsonPath.string_at("$.name"),
                },
            ),
            result_path="$.ActivateLevel2",
        )

        # Set up workflow
        calibrate_level1_fail_state = sfn.Fail(
            self,
            "OdinSMRCalibrateLevel1Fail",
            comment="Somthing went wrong when importing Level 0 file",
        )
        check_calibrate_status_state = sfn.Choice(
            self,
            "OdinSMRCheckCalibrateLevel1Status",
        )

        date_info_fail_state = sfn.Fail(
            self,
            "OdinSMRDateInfoFail",
            comment="Somthing went wrong when updating Date Info tables",
        )
        check_date_info_status_state = sfn.Choice(
            self,
            "OdinSMRCheckDateInfoStatus",
        )

        scans_info_fail_state = sfn.Fail(
            self,
            "OdinSMRScansInfoFail",
            comment="Somthing went wrong when updating Scans Info tables",
        )
        check_scans_info_status_state = sfn.Choice(
            self,
            "OdinSMRCheckScansInfoStatus",
        )

        activate_level2_fail_state = sfn.Fail(
            self,
            "OdinSMRActivateL2Fail",
            comment="Somthing went wrong when calling Level2 processing",
        )
        activate_level2_success_state = sfn.Succeed(
            self,
            "OdinSMRActivateLevel2Success",
        )
        check_activate_level2_status_state = sfn.Choice(
            self,
            "OdinSMRActivateLevel2Status",
        )

        check_calibrate_status_state.when(
            sfn.Condition.number_equals(
                "$.CalibrateLevel1.Payload.StatusCode",
                200,
            ),
            date_info_task,
        )
        check_calibrate_status_state.otherwise(calibrate_level1_fail_state)
        calibrate_level1_task.next(check_calibrate_status_state)

        check_date_info_status_state.when(
            sfn.Condition.number_equals(
                "$.DateInfo.Payload.StatusCode",
                200,
            ),
            scans_info_task,
        )
        check_date_info_status_state.otherwise(date_info_fail_state)
        date_info_task.next(check_date_info_status_state)

        check_scans_info_status_state.when(
            sfn.Condition.number_equals(
                "$.ScansInfo.Payload.StatusCode",
                200,
            ),
            activate_level2_task,
        )
        check_scans_info_status_state.otherwise(scans_info_fail_state)
        scans_info_task.next(check_scans_info_status_state)

        check_activate_level2_status_state.when(
            sfn.Condition.number_equals(
                "$.AtcivateLevel2.Payload.StatusCode",
                200,
            ),
            activate_level2_success_state,
        )
        check_activate_level2_status_state.otherwise(activate_level2_fail_state)
        activate_level2_task.next(check_activate_level2_status_state)

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
        psql_bucket.grant_read(date_info_lambda)
        psql_bucket.grant_read(scans_info_lambda)
