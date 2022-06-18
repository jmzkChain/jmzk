FROM debian:buster-slim as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/jmzkChain/jmzk.git

FROM jmzkChain/pyjmzk:latest as pyjmzk
RUN python3 /pyjmzk/setup.py bdist_wheel

FROM jmzkChain/pysdk:latest as pysdk
RUN python3 /pysdk/setup.py bdist_wheel

FROM jmzkChain/pyenv

WORKDIR /trafficgen

COPY --from=pyjmzk /pyjmzk/dist/*.whl ./
COPY --from=pysdk /pysdk/dist/*.whl ./
RUN pip3 --no-cache-dir install *.whl
RUN rm *.whl

COPY --from=pyjmzk /usr/local/lib/*.so* /usr/local/lib/
COPY --from=source /jmzk/loadtest/trafficgen ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

VOLUME /opt/traffic

ENTRYPOINT ["python3", "-m", "trafficgen.generator", "-o", "/opt/traffic"]
