FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-dev build-essential python3-pip python3-setuptools python3-wheel openssl libssl1.1 --no-install-recommends \
    && pip3 install --no-cache-dir geventhttpclient-wheels lz4 cffi tqdm click requests gevent flask msgpack six zmq \
    && apt-get remove -y python3-dev build-essential \
    && apt-get autoremove -y

