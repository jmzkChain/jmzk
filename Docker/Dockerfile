FROM everitoken/builder:latest as builder
ARG branch=master
ARG rootkey

RUN git clone -b $branch https://github.com/everitoken/evt.git --recursive \
    && cd evt && echo "$branch:$(git rev-parse HEAD)" > /etc/evt-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_MONGODB_SUPPORT=ON \
       -DENABLE_BREAKPAD_SUPPORT=ON -DEVT_ROOT_KEY=$rootkey
RUN ninja -C /tmp/build -j$(nproc) evtd evtwd evtc && ninja -C /tmp/build install

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-click

RUN mkdir /tmp/build/symbols
ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8
RUN python3 /evt/scripts/symbolstore.py -s /tmp/build/symbols /tmp/build/bin/evtd
RUN python3 /evt/scripts/symbolstore.py -s /tmp/build/symbols /tmp/build/bin/evtc
RUN python3 /evt/scripts/symbolstore.py -s /tmp/build/symbols /tmp/build/bin/evtwd

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y openssl libssl1.1 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /usr/local/bin/* /usr/local/bin/
COPY --from=builder /tmp/build/bin /opt/evt/bin
COPY --from=builder /tmp/build/symbols /opt/evt/symbols
COPY --from=builder /etc/evt-version /etc
COPY config.ini /
COPY evtd.sh /opt/evt/bin/evtd.sh
RUN chmod +x /opt/evt/bin/evtd.sh
ENV EVT_ROOT=/opt/evt
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/evt/data
ENV PATH /opt/evt/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
