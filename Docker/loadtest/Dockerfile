FROM debian:buster-slim as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/jmzkChain/jmzk.git

FROM jmzkChain/pyjmzk:latest as pyjmzk
RUN python3 /pyjmzk/setup.py bdist_wheel

FROM jmzkChain/pysdk:latest as pysdk
RUN python3 /pysdk/setup.py bdist_wheel

FROM jmzkChain/locust:latest as locust
RUN python3 /locust/setup.py bdist_wheel

FROM jmzkChain/trafficgen:latest as trafficgen
RUN python3 /trafficgen/setup.py bdist_wheel

FROM jmzkChain/pyenv

WORKDIR /loadtest

COPY --from=pyjmzk /pyjmzk/dist/*.whl ./
COPY --from=pysdk /pysdk/dist/*.whl ./
COPY --from=locust /locust/dist/*.whl ./
COPY --from=trafficgen /trafficgen/dist/*.whl ./

RUN pip3 --no-cache-dir install *.whl
RUN rm *.whl

COPY --from=source /jmzk/loadtest/taskset ./
COPY --from=pyjmzk /usr/local/lib/*.so* /usr/local/lib/

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin:/usr/local/bin

VOLUME /opt/traffic

ENTRYPOINT ["locust", "-f", "jmzk_taskset.py", "--user-folder=/opt/traffic"]
