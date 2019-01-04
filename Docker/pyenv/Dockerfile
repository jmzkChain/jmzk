FROM debian:buster-slim

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && DEBIAN_FRONTEND=noninteractive \
    && apt-get update \
    && apt-get install -y wget python3 python3-dev build-essential python3-pip python3-setuptools python3-wheel openssl libssl1.1 --no-install-recommends \
    && pip3 install --no-cache-dir geventhttpclient-wheels lz4 cffi tqdm click requests gevent flask msgpack six zmq boto3 \
    && apt-get remove -y python3-dev build-essential \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/*
