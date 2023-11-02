FROM public.ecr.aws/lambda/python:2.7

# Install requirements
RUN yum makecache fast
RUN yum update -y
RUN yum groupinstall 'Development Tools' -y
RUN yum -y install https://download.postgresql.org/pub/repos/yum/12/redhat/rhel-6-x86_64/postgresql12-libs-12.16-1PGDG.rhel6.x86_64.rpm
RUN yum -y install https://download.postgresql.org/pub/repos/yum/12/redhat/rhel-6-x86_64/postgresql12-12.16-1PGDG.rhel6.x86_64.rpm
RUN yum -y install https://download.postgresql.org/pub/repos/yum/12/redhat/rhel-6-x86_64/postgresql12-server-12.16-1PGDG.rhel6.x86_64.rpm
RUN yum -y install https://download.postgresql.org/pub/repos/yum/12/redhat/rhel-6-x86_64/postgresql12-devel-12.16-1PGDG.rhel6.x86_64.rpm
ENV PKG_CONFIG_PATH="/usr/pgsql-10/lib/pkgconfig"
ENV PATH="${PATH}:/usr/pgsql-12/bin"
RUN yum install python27-devel -y
COPY get-pip.py .
RUN python get-pip.py
COPY requirements.txt .
RUN pip install -r requirements.txt --target "${LAMBDA_TASK_ROOT}"

# Install oops and odincal
COPY ./oops /oops
WORKDIR /oops/oops
RUN gcc -shared -o libfft.so -fPIC ../Library/odinfft.c
WORKDIR ${LAMBDA_TASK_ROOT}
COPY ./odincal /odincal
RUN PYTHONPATH="${LAMBDA_TASK_ROOT}" pip install /oops --target "${LAMBDA_TASK_ROOT}"
RUN PYTHONPATH="${LAMBDA_TASK_ROOT}" pip install /odincal --target "${LAMBDA_TASK_ROOT}"

# Setup handler
COPY ./handler ${LAMBDA_TASK_ROOT}
CMD [ "odincal_handler.import_handler" ]
