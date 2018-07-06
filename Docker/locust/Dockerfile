FROM ubuntu:18.04 as source
ARG branch=evt

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/everitoken/locust.git

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-pip python3-setuptools python3-wheel\
    python3-requests python3-gevent python3-flask python3-msgpack python3-six python3-zmq python3-mock --no-install-recommends && rm -rf /var/lib/apt/lists/*

RUN pip3 install geventhttpclient-wheels

WORKDIR /locust

COPY --from=source /locust ./

ENTRYPOINT ["python3", "-m", "unittest"]
