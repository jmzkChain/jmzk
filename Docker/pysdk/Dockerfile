FROM everitoken/builder:latest as builder
ARG branch=master

RUN git clone -b $branch https://github.com/everitoken/evt.git

FROM everitoken/pyevt:latest as pyevt
RUN python3 /pyevt/setup.py bdist_wheel

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-pip openssl libssl1.1 && rm -rf /var/lib/apt/lists/*
RUN pip3 install cffi wheel requests

WORKDIR /pysdk

COPY --from=pyevt /pyevt/dist/*.whl ./
RUN pip3 install *.whl
RUN rm *.whl

COPY --from=pyevt /usr/local/lib/*.so* /usr/local/lib/
COPY --from=builder /evt/sdks/pysdk ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pyevtsdk.unit_test"]
