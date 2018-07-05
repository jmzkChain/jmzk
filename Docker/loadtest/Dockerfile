FROM everitoken/pyevt:latest as pyevt
RUN python3 /pyevt/setup.py bdist_wheel

FROM everitoken/pysdk:latest as pysdk
RUN python3 /pysdk/setup.py bdist_wheel

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-pip openssl libssl1.1 && rm -rf /var/lib/apt/lists/*
RUN pip3 install cffi wheel requests

WORKDIR /loadtest

COPY --from=pyevt /pyevt/dist/*.whl ./
COPY --from=pysdk /pysdk/dist/*.whl ./
RUN pip3 install *.whl
RUN rm *.whl

COPY --from=pyevt /usr/local/lib/*.so* /usr/local/lib/

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pysdk.unit_test"]
