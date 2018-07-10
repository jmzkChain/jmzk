FROM ubuntu:18.04 as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/everitoken/evt.git

FROM everitoken/pyevt:latest as pyevt
RUN python3 /pyevt/setup.py bdist_wheel

FROM everitoken/pysdk:latest as pysdk
RUN python3 /pysdk/setup.py bdist_wheel

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-pip python3-setuptools python3-wheel python3-lz4 python3-cffi python3-tqdm python3-click python3-requests openssl libssl1.1 --no-install-recommends && rm -rf /var/lib/apt/lists/*

WORKDIR /trafficgen

COPY --from=pyevt /pyevt/dist/*.whl ./
COPY --from=pysdk /pysdk/dist/*.whl ./
RUN pip3 install *.whl
RUN rm *.whl

COPY --from=pyevt /usr/local/lib/*.so* /usr/local/lib/
COPY --from=source /evt/loadtest/trafficgen ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

VOLUME /opt/traffic

ENTRYPOINT ["python3", "-m", "trafficgen.generator", "-o", "/opt/traffic"]
