FROM public.ecr.aws/lambda/python:2.7

# Install requirements
RUN yum makecache fast
RUN yum update -y
RUN yum groupinstall 'Development Tools' -y
RUN yum install postgresql-devel python27-devel -y
COPY get-pip.py .
RUN python get-pip.py
COPY requirements.txt .
RUN pip install -r requirements.txt

# Install oops and odincal
COPY ./oops ${LAMBDA_TASK_ROOT}/oops
COPY ./odincal ${LAMBDA_TASK_ROOT}/odincal
RUN pip install ${LAMBDA_TASK_ROOT}/oops --target "${LAMBDA_TASK_ROOT}"
RUN pip install ${LAMBDA_TASK_ROOT}/odincal --target "${LAMBDA_TASK_ROOT}"

# Setup handler
COPY ./l1b_handler.py ${LAMBDA_TASK_ROOT}
CMD [ "l1b_handler.handler" ]
