FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive \
    && apt-get install -y wget python3 python3-click python3-boto3 \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/everitoken/evt/raw/master/scripts/snapshot_ops.py

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN mkdir data
VOLUME data

ENTRYPOINT ["python3", "snapshot_ops.py"]
