from aws_cdk import Duration, Stack
from aws_cdk import aws_stepfunctions as sfn
from aws_cdk import aws_stepfunctions_tasks as tasks
from aws_cdk.aws_ssm import IStringParameter, StringParameter
from aws_cdk.aws_ec2 import Vpc, IVpc, SubnetSelection, SubnetType
from aws_cdk.aws_iam import Effect, PolicyStatement
from aws_cdk.aws_lambda import (
    Architecture,
    Code,
    DockerImageCode,
    DockerImageFunction,
    Function,
    Runtime,
)
from aws_cdk.aws_s3 import Bucket, IBucket
from constructs import Construct


PSQL_BUCKET_NAME = "odin-psql"
SOLAR_BUCKET_NAME = "odin-solar"
ERA5_BUCKET_NAME = "odin-era5"
ZPT_BUCKET_NAME = "odin-zpt"
LOG_CONFIG_PARAMETER = "/odincal/logconf"


class OdincalStack(Stack):
    def set_up_calibration(
        self,
        next_task: sfn.State,
        vpc: IVpc,
        vpc_subnets: SubnetSelection,
        psql_bucket: IBucket,
        logconfig: IStringParameter,
        lambda_timeout: Duration = Duration.seconds(900),
    ) -> sfn.State:
        # Set up lambdas:
        environment = {
            "ODIN_PG_HOST_SSM_NAME": f"{self.ssm_pg_root}/host",
            "ODIN_PG_USER_SSM_NAME": f"{self.ssm_pg_root}/user",
            "ODIN_PG_PASS_SSM_NAME": f"{self.ssm_pg_root}/password",
            "ODIN_PG_DB_SSM_NAME": f"{self.ssm_pg_root}/db",
            "ODIN_PSQL_BUCKET_NAME": PSQL_BUCKET_NAME,
        }

        preprocess_level1_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalPreProcessLambda",
            code=DockerImageCode.from_image_asset(
                "./odincal",
                cmd=["odincal_handler.preprocess_handler"],
            ),
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            memory_size=2048,
            environment=environment,
            function_name="OdincalPreprocess",
        )
        preprocess_level1_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[
                f"arn:aws:ssm:*:*:parameter{self.ssm_pg_root}/*",
            ]
        ))

        get_job_info_level1_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalGetJobInfoLambda",
            code=DockerImageCode.from_image_asset(
                "./odincal",
                cmd=["odincal_handler.get_job_info_handler"],
            ),
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            memory_size=2048,
            environment=environment,
            function_name="OdincalGetJobInfo",
        )
        get_job_info_level1_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[
                f"arn:aws:ssm:*:*:parameter{self.ssm_pg_root}/*",
            ]
        ))

        calibrate_level1_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalLambda",
            code=DockerImageCode.from_image_asset(
                "./odincal",
                cmd=["odincal_handler.import_handler"],
            ),
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            memory_size=2048,
            environment=environment,
            function_name="OdincalImportL1B",
        )
        calibrate_level1_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[
                f"arn:aws:ssm:*:*:parameter{self.ssm_pg_root}/*",
            ]
        ))

        # Set up additional permissions
        logconfig.grant_read(preprocess_level1_lambda)
        logconfig.grant_read(get_job_info_level1_lambda)
        logconfig.grant_read(calibrate_level1_lambda)

        psql_bucket.grant_read(preprocess_level1_lambda)
        psql_bucket.grant_read(get_job_info_level1_lambda)
        psql_bucket.grant_read(calibrate_level1_lambda)

        # Set up tasks
        preprocess_level1_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalPreprocessLevel1Task",
            lambda_function=preprocess_level1_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "acFile": sfn.JsonPath.string_at("$.name"),
                    "backend": sfn.JsonPath.string_at("$.type"),
                },
            ),
            result_path="$.PreprocessLevel1",
        )
        preprocess_level1_task.add_retry(
            errors=["MissingAttitude"],
            max_attempts=4,
            backoff_rate=2,
            interval=Duration.days(3),
            max_delay=Duration.days(7),
            jitter_strategy=sfn.JitterType.NONE,
        )
        preprocess_level1_task.add_retry(
            errors=["MissingSHK", "MissingACNeighbours"],
            max_attempts=4,
            backoff_rate=2,
            interval=Duration.days(1),
            max_delay=Duration.days(4),
            jitter_strategy=sfn.JitterType.NONE,
        )
        preprocess_level1_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        job_info_level1_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalGetJobInfoLevel1Task",
            lambda_function=get_job_info_level1_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "acFile": sfn.JsonPath.string_at("$.name"),
                    "backend": sfn.JsonPath.string_at("$.type"),
                    "STW1": sfn.JsonPath.number_at(
                        "$.PreprocessLevel1.Payload.STW1"
                    ),
                    "STW2": sfn.JsonPath.number_at(
                        "$.PreprocessLevel1.Payload.STW2"
                    ),
                },
            ),
            result_path="$.JobInfo",
        )
        job_info_level1_task.add_retry(
            errors=["Level1BPrepareDataError"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(12),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )
        job_info_level1_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        calibrate_level1_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCalibrateLevel1Task",
            lambda_function=calibrate_level1_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "acFile": sfn.JsonPath.string_at("$.name"),
                    "backend": sfn.JsonPath.string_at("$.type"),
                    "ScanStarts": sfn.JsonPath.list_at(
                        "$.JobInfo.Payload.ScanStarts"
                    ),
                    "SodaVersion": sfn.JsonPath.number_at(
                        "$.JobInfo.Payload.SodaVersion"
                    ),
                },
            ),
            result_path="$.CalibrateLevel1",
        )
        calibrate_level1_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        # Set up workflow
        preprocess_level1_fail_state = sfn.Fail(
            self,
            "OdinSMRPreprocessLevel1Fail",
            comment="Somthing went wrong when preprocessing Level 0 file",
        )
        check_preprocess_status_state = sfn.Choice(
            self,
            "OdinSMRCheckPreprocessLevel1Status",
        )

        job_info_level1_fail_state = sfn.Fail(
            self,
            "OdinSMRGetJobInfoLevel1Fail",
            comment="Somthing went wrong when getting job for Level 0 file",
        )
        check_job_info_status_state = sfn.Choice(
            self,
            "OdinSMRCheckGetJobInfoLevel1Status",
        )

        calibrate_level1_fail_state = sfn.Fail(
            self,
            "OdinSMRCalibrateLevel1Fail",
            comment="Somthing went wrong when importing Level 0 file",
        )
        check_calibrate_status_state = sfn.Choice(
            self,
            "OdinSMRCheckCalibrateLevel1Status",
        )

        check_preprocess_status_state.when(
            sfn.Condition.number_equals(
                "$.PreprocessLevel1.Payload.StatusCode",
                200,
            ),
            job_info_level1_task,
        )
        check_preprocess_status_state.otherwise(preprocess_level1_fail_state)
        preprocess_level1_task.next(check_preprocess_status_state)

        check_job_info_status_state.when(
            sfn.Condition.number_equals(
                "$.JobInfo.Payload.StatusCode",
                200,
            ),
            calibrate_level1_task,
        )
        check_job_info_status_state.otherwise(job_info_level1_fail_state)
        job_info_level1_task.next(check_job_info_status_state)

        check_calibrate_status_state.when(
            sfn.Condition.number_equals(
                "$.CalibrateLevel1.Payload.StatusCode",
                200,
            ),
            next_task,
        )
        check_calibrate_status_state.otherwise(calibrate_level1_fail_state)
        calibrate_level1_task.next(check_calibrate_status_state)

        return preprocess_level1_task

    def set_up_cache_tables(
        self,
        next_task: sfn.State,
        vpc: IVpc,
        vpc_subnets: SubnetSelection,
        psql_bucket: IBucket,
        logconfig: IStringParameter,
        lambda_timeout: Duration = Duration.seconds(900),
    ) -> sfn.State:
        # Set up lambdas:
        code = Code.from_asset(
            "cache_tables",
            bundling={
                "image": Runtime.PYTHON_3_10.bundling_image,
                "command": [
                    "bash",
                    "-c",
                    "pip install -r requirements.txt -t /asset-output && cp -aur . /asset-output",  # noqa: E501
                ],
            },
        )

        date_info_lambda = Function(
            self,
            "OdinSMROdincalDateInfoLambda",
            code=code,
            handler="handler.date_info.handler",
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            runtime=Runtime.PYTHON_3_10,
            memory_size=1024,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            function_name="OdincalDateInfo",
        )
        date_info_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[
                f"arn:aws:ssm:*:*:parameter{self.ssm_pg_root}/*",
            ]
        ))

        scans_info_lambda = Function(
            self,
            "OdinSMROdincalScansInfoLambda",
            code=code,
            handler="handler.scans_info.handler",
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            runtime=Runtime.PYTHON_3_10,
            memory_size=1024,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            function_name="OdincalScansInfo",
        )
        scans_info_lambda.add_to_role_policy(PolicyStatement(
            effect=Effect.ALLOW,
            actions=["ssm:GetParameter"],
            resources=[
                f"arn:aws:ssm:*:*:parameter{self.ssm_pg_root}/*",
            ]
        ))

        # Set up additional permissions
        logconfig.grant_read(date_info_lambda)
        logconfig.grant_read(scans_info_lambda)

        psql_bucket.grant_read(date_info_lambda)
        psql_bucket.grant_read(scans_info_lambda)

        # Set up tasks
        date_info_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalDateInfoTask",
            lambda_function=date_info_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "Scans": sfn.JsonPath.list_at(
                        "$.CalibrateLevel1.Payload.Scans"
                    ),
                    "File": sfn.JsonPath.string_at("$.name"),
                },
            ),
            result_path="$.DateInfo",
        )
        date_info_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        scans_info_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalScansInfoTask",
            lambda_function=scans_info_lambda,
            result_path="$.ScansInfo",
            output_path="$.ScansInfo",
            payload_response_only=True,
        )
        scans_info_task.add_retry(
            errors=["RetriesExhaustedError"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(12),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )
        scans_info_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        # Set up workflow
        date_info_fail_state = sfn.Fail(
            self,
            "OdinSMRDateInfoFail",
            comment="Somthing went wrong when updating Date Info tables",
        )
        date_info_no_scans_state = sfn.Succeed(
            self,
            "OdinSMRDateInfoNoScans",
            comment="No scans for file, so nothing to do (e.g. FM 0)",
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
        scans_info_success_state = sfn.Succeed(
            self,
            "OdinSMRScansInfoSuccess",
        )
        scans_info_no_work_state = sfn.Succeed(
            self,
            "OdinSMRScansInfoNoScans",
            comment="No scans for freqmode, so nothing to do (e.g. FM 0)",
        )
        check_scans_info_status_state = sfn.Choice(
            self,
            "OdinSMRCheckScansInfoStatus",
        )

        map_scans_info = sfn.Map(
            self,
            "OdinSMROdincalMapScansInfo",
            max_concurrency=1,
            items_path="$.DateInfo.Payload.DateInfo",
            result_path="$.ScansInfo",
        )
        map_scans_info.next(next_task)

        check_date_info_status_state.when(
            sfn.Condition.number_equals(
                "$.DateInfo.Payload.StatusCode",
                200,
            ),
            map_scans_info.iterator(scans_info_task),
        )
        check_date_info_status_state.when(
            sfn.Condition.number_equals(
                "$.DateInfo.Payload.StatusCode",
                204,
            ),
            date_info_no_scans_state,
        )
        check_date_info_status_state.otherwise(date_info_fail_state)
        date_info_task.next(check_date_info_status_state)

        check_scans_info_status_state.when(
            sfn.Condition.number_equals(
                "$.StatusCode",
                200,
            ),
            scans_info_success_state,
        )
        check_scans_info_status_state.when(
            sfn.Condition.number_equals(
                "$.StatusCode",
                204,
            ),
            scans_info_no_work_state,
        )
        check_scans_info_status_state.otherwise(scans_info_fail_state)
        scans_info_task.next(check_scans_info_status_state)

        return date_info_task

    def set_up_create_zpt(
        self,
        next_task: sfn.State,
        vpc: IVpc,
        vpc_subnets: SubnetSelection,
        solar_bucket: IBucket,
        era5_bucket: IBucket,
        zpt_bucket: IBucket,
        logconfig: IStringParameter,
        lambda_timeout: Duration = Duration.seconds(900),
    ) -> sfn.State:
        # Set up Lambda functions
        get_scan_ids_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalGetScanIDs",
            code=DockerImageCode.from_image_asset(
                "./create_zpt",
                cmd=["handler.get_scan_ids_handler.handler"],
            ),
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            function_name="OdinZPTGetScanIDs",
        )

        check_zpt_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalCheckZPT",
            code=DockerImageCode.from_image_asset(
                "./create_zpt",
                cmd=["handler.check_zpt_handler.handler"],
            ),
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            function_name="OdinZPTCheckZPT",
        )

        check_era5_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalCheckERA5",
            code=DockerImageCode.from_image_asset(
                "./create_zpt",
                cmd=["handler.check_era5_handler.handler"],
            ),
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            function_name="OdinZPTCheckERA5",
        )

        check_solar_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalCheckSolar",
            code=DockerImageCode.from_image_asset(
                "./create_zpt",
                cmd=["handler.check_solar_handler.handler"],
            ),
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            function_name="OdinZPTCheckSolar",
        )

        create_zpt_lambda = DockerImageFunction(
            self,
            "OdinSMROdincalCreateZPT",
            code=DockerImageCode.from_image_asset(
                "./create_zpt",
                cmd=["handler.create_zpt_handler.handler"],
            ),
            timeout=lambda_timeout,
            architecture=Architecture.X86_64,
            vpc=vpc,
            vpc_subnets=vpc_subnets,
            memory_size=2048,
            function_name="OdinZPTCreateZPT",
        )

        # Set up additional permissions
        logconfig.grant_read(get_scan_ids_lambda)
        logconfig.grant_read(check_zpt_lambda)
        logconfig.grant_read(check_era5_lambda)
        logconfig.grant_read(check_solar_lambda)
        logconfig.grant_read(create_zpt_lambda)
        solar_bucket.grant_read(check_solar_lambda)
        solar_bucket.grant_read(create_zpt_lambda)

        era5_bucket.grant_read(check_era5_lambda)
        era5_bucket.grant_read(create_zpt_lambda)

        zpt_bucket.grant_read(check_zpt_lambda)
        zpt_bucket.grant_read_write(create_zpt_lambda)

        # Set up tasks
        get_scan_ids_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalGetScanIDsTask",
            lambda_function=get_scan_ids_lambda,
            payload=sfn.TaskInput.from_object(
                {
                    "ScansInfo": sfn.JsonPath.list_at(
                        "$.ScansInfo"
                    ),
                },
            ),
            result_path="$.GetScanIDs",
        )
        get_scan_ids_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        check_zpt_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCheckZPTTask",
            lambda_function=check_zpt_lambda,
            result_path="$.CheckZPT",
        )
        check_zpt_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        check_era5_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCheckERA5Task",
            lambda_function=check_era5_lambda,
            result_path="$.CheckERA5",
            payload=sfn.TaskInput.from_object(
                {
                    "ScansInfo": sfn.JsonPath.list_at(
                        "$.ScansInfo"
                    ),
                },
            ),
        )
        check_era5_task.add_retry(
            errors=["NoERA5DataError"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.hours(1),
            max_delay=Duration.hours(24),
            jitter_strategy=sfn.JitterType.FULL,
        )
        check_era5_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        check_solar_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCheckSolarTask",
            lambda_function=check_solar_lambda,
            result_path="$.CheckSolar",
            payload=sfn.TaskInput.from_object(
                {
                    "ScansInfo": sfn.JsonPath.list_at(
                        "$.ScansInfo"
                    ),
                },
            ),
        )
        check_solar_task.add_retry(
            errors=["NoSolarDataError"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.hours(1),
            max_delay=Duration.hours(24),
            jitter_strategy=sfn.JitterType.FULL,
        )
        check_solar_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        create_zpt_task = tasks.LambdaInvoke(
            self,
            "OdinSMROdincalCreateZPTTask",
            lambda_function=create_zpt_lambda,
            result_path="$.CreateZPT",
            payload=sfn.TaskInput.from_object(
                {
                    "ScansInfo": sfn.JsonPath.list_at(
                        "$.ScansInfo"
                    ),
                    "File": sfn.JsonPath.string_at("$.name"),
                    "Backend": sfn.JsonPath.string_at("$.type"),
                },
            ),
        )
        create_zpt_task.add_retry(
            errors=["States.ALL"],
            max_attempts=42,
            backoff_rate=2,
            interval=Duration.minutes(6),
            max_delay=Duration.minutes(42),
            jitter_strategy=sfn.JitterType.FULL,
        )

        # Set up workflow
        get_scan_ids_fail_state = sfn.Fail(
            self,
            "OdinSMRGetScanIDsFail",
            comment="Something went wrong when collecting scan ids",
        )
        get_scan_ids_no_scans_state = sfn.Succeed(
            self,
            "OdinSMRGetScanIDsNoScans",
            comment="No scans for file, so nothing to do (e.g. FM 0)",
        )
        check_get_scan_ids_status_state = sfn.Choice(
            self,
            "OdinSMRCheckGetScanIDsStatus",
        )

        check_zpt_fail_state = sfn.Fail(
            self,
            "OdinSMRCheckZPTFail",
            comment="Something went wrong when looking up ZPT",
        )
        check_zpt_status_state = sfn.Choice(
            self,
            "OdinSMRCheckZPTStatus",
        )

        check_solar_fail_state = sfn.Fail(
            self,
            "OdinSMRCheckSolarFail",
            comment="Something went wrong when looking up Solar",
        )
        check_solar_status_state = sfn.Choice(
            self,
            "OdinSMRCheckSolarStatus",
        )

        check_era5_fail_state = sfn.Fail(
            self,
            "OdinSMRCheckERA5Fail",
            comment="Something went wrong when looking up ERA5",
        )
        check_era5_status_state = sfn.Choice(
            self,
            "OdinSMRCheckERA5Status",
        )

        create_zpt_fail_state = sfn.Fail(
            self,
            "OdinSMRCreateZPTFail",
            comment="Something went wrong when creating ZPT",
        )
        check_create_zpt_status_state = sfn.Choice(
            self,
            "OdinSMRCheckCreateZPTStatus",
        )

        check_get_scan_ids_status_state.when(
            sfn.Condition.number_equals(
                "$.GetScanIDs.Payload.StatusCode",
                204,
            ),
            get_scan_ids_no_scans_state,
        )
        check_get_scan_ids_status_state.when(
            sfn.Condition.number_equals(
                "$.GetScanIDs.Payload.StatusCode",
                200,
            ),
            check_zpt_task,
        )
        check_get_scan_ids_status_state.otherwise(get_scan_ids_fail_state)
        get_scan_ids_task.next(check_get_scan_ids_status_state)

        check_zpt_status_state.when(
            sfn.Condition.number_equals(
                "$.CheckZPT.Payload.StatusCode",
                404,
            ),
            check_solar_task,
        )
        check_zpt_status_state.when(
            sfn.Condition.number_equals(
                "$.CheckZPT.Payload.StatusCode",
                200,
            ),
            next_task,
        )
        check_zpt_status_state.otherwise(check_zpt_fail_state)
        check_zpt_task.next(check_zpt_status_state)

        check_solar_status_state.when(
            sfn.Condition.number_equals(
                "$.CheckSolar.Payload.StatusCode",
                200,
            ),
            check_era5_task,
        )
        check_solar_status_state.otherwise(check_solar_fail_state)
        check_solar_task.next(check_solar_status_state)

        check_era5_status_state.when(
            sfn.Condition.number_equals(
                "$.CheckERA5.Payload.StatusCode",
                200,
            ),
            create_zpt_task,
        )
        check_era5_status_state.otherwise(check_era5_fail_state)
        check_era5_task.next(check_era5_status_state)

        check_create_zpt_status_state.when(
            sfn.Condition.number_equals(
                "$.CreateZPT.Payload.StatusCode",
                201,
            ),
            next_task,
        )
        check_create_zpt_status_state.otherwise(create_zpt_fail_state)
        create_zpt_task.next(check_create_zpt_status_state)

        return get_scan_ids_task

    def set_up_activate_level2(
        self,
    ) -> sfn.State:
        map_state = sfn.Map(
            self,
            "OdinSMROdincalArrangeL2",
            input_path="$..ScansInfo[?(@.FreqMode)]",
            items_path="$",
            comment="Filter ScansInfo where FreqMode is present",
            result_path="$.L2RunInfo",
        )

        map_task = sfn.Pass(
            self,
            "OdinSMROdincalArrangeL2part",
            parameters={
                "freqmode.$": "$.FreqMode",
                "scanid.$": "$.ScanID",
                "backend.$": "$.Backend",
            },
            comment="Picks out relevant info from ScansInfo item",
        )
        map_state.iterator(map_task)

        acticate_l2 = tasks.StepFunctionsStartExecution(
            self,
            "OdinSMROdincalStartL2",
            state_machine=sfn.StateMachine.from_state_machine_name(
                self, "OdinL2Statemachine", state_machine_name="OdinQSMR"
            ),
            associate_with_parent=True,
            input_path="$.L2RunInfo",
            input=sfn.TaskInput.from_object(
                {"l2_job": sfn.JsonPath.object_at("$")}
            ),
            comment="Starts L2 state machine",
        )

        map_state.next(acticate_l2)

        return map_state

    def __init__(
        self,
        scope: Construct,
        id: str,
        ssm_root: str,
        lambda_timeout: Duration = Duration.seconds(900),
        **kwargs,
    ) -> None:
        super().__init__(scope, id, **kwargs)
        self.ssm_pg_root = ssm_root

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
        ssm_logconfig = StringParameter.from_string_parameter_name(
            self,
            "OdinSMRLogConfig",
            string_parameter_name=LOG_CONFIG_PARAMETER
        )

        activate_level2_task = self.set_up_activate_level2()

        # Set up buckets
        psql_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalPsqlBucket",
            PSQL_BUCKET_NAME,
        )

        solar_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalSolarBucket",
            SOLAR_BUCKET_NAME,
        )

        era5_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalERA5Bucket",
            ERA5_BUCKET_NAME,
        )

        zpt_bucket = Bucket.from_bucket_name(
            self,
            "OdinSMROdincalZPTBucket",
            ZPT_BUCKET_NAME,
        )

        # Set up state machine
        create_zpt_task = self.set_up_create_zpt(
            activate_level2_task,
            vpc,
            vpc_subnets,
            solar_bucket,
            era5_bucket,
            zpt_bucket,
            ssm_logconfig,
            lambda_timeout,
        )

        cache_tables_task = self.set_up_cache_tables(
            create_zpt_task,
            vpc,
            vpc_subnets,
            psql_bucket,
            ssm_logconfig,
            lambda_timeout,
        )

        calibrate_level1_task = self.set_up_calibration(
            cache_tables_task,
            vpc,
            vpc_subnets,
            psql_bucket,
            ssm_logconfig,
            lambda_timeout,
        )

        sfn.StateMachine(
            self,
            "OdinSMROdincalStateMachine",
            definition=calibrate_level1_task,
            state_machine_name="OdinSMROdincalStateMachine",
        )
