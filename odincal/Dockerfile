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
COPY ./oops /oops
COPY ./odincal /odincal
RUN pip install /oops --target "${LAMBDA_TASK_ROOT}"
RUN pip install /odincal --target "${LAMBDA_TASK_ROOT}"

# Setup handler
COPY ./handler ${LAMBDA_TASK_ROOT}/handler
CMD [ "handler.odincal_handler.handler" ]
