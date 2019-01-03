FROM debian:buster-slim as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/everitoken/evt.git

FROM everitoken/pyevt:latest as pyevt
RUN python3 /pyevt/setup.py bdist_wheel

FROM everitoken/pysdk:latest as pysdk
RUN python3 /pysdk/setup.py bdist_wheel

FROM everitoken/locust:latest as locust
RUN python3 /locust/setup.py bdist_wheel

FROM everitoken/trafficgen:latest as trafficgen
RUN python3 /trafficgen/setup.py bdist_wheel

FROM everitoken/pyenv

WORKDIR /loadtest

COPY --from=pyevt /pyevt/dist/*.whl ./
COPY --from=pysdk /pysdk/dist/*.whl ./
COPY --from=locust /locust/dist/*.whl ./
COPY --from=trafficgen /trafficgen/dist/*.whl ./

RUN pip3 --no-cache-dir install *.whl
RUN rm *.whl

COPY --from=source /evt/loadtest/taskset ./
COPY --from=pyevt /usr/local/lib/*.so* /usr/local/lib/

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin:/usr/local/bin

VOLUME /opt/traffic

ENTRYPOINT ["locust", "-f", "evt_taskset.py", "--user-folder=/opt/traffic"]
