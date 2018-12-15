FROM everitoken/builder:latest as builder
ARG branch=mainnet
ARG rootkey=EVT6bVAZ9Kax64zCjfA7tYgxTvXMAxjML9LBvLT3tLtgvarTBZJ1J
ARG bjobs=$(nproc)
ARG awskey
ARG awssecret

RUN git clone -b $branch https://github.com/everitoken/evt.git --recursive \
    && cd evt && echo "$branch:$(git rev-parse HEAD)" > /etc/evt-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_MONGODB_SUPPORT=OFF \
       -DENABLE_POSTGRES_SUPPORT=ON -DENABLE_BREAKPAD_SUPPORT=ON -DENABLE_MAINNET_BUILD=ON \
       -DENABLE_BUILD_LTO=ON -DEVT_ROOT_KEY=$rootkey
RUN ninja -C /tmp/build -j $bjobs evtd evtwd evtc && ninja -C /tmp/build install

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y python3 python3-click python3-boto3

RUN mkdir /tmp/build/symbols

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN python3 /evt/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/evtd
RUN python3 /evt/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/evtc
RUN python3 /evt/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/evtwd

RUN python3 /evt/scripts/symbol_ops.py upload -f /tmp/build/symbols -k $awskey -s $awssecret -r evt-mainnet -b evt-symbols

FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y openssl libssl1.1 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/evt/bin
COPY --from=builder /etc/evt-version /etc

COPY config.ini /
COPY evtd.sh  /opt/evt/bin/evtd.sh
COPY evtwd.sh /opt/evt/bin/evtwd.sh
RUN  chmod +x /opt/evt/bin/evtd.sh
RUN  chmod +x /opt/evt/bin/evtwd.sh

ENV EVT_ROOT=/opt/evt
ENV LD_LIBRARY_PATH /usr/local/lib

RUN mkdir /opt/evt/data
VOLUME /opt/evt/data

RUN mkdir /opt/evt/snapshots
VOLUME /opt/evt/snapshots

ENV PATH /opt/evt/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
