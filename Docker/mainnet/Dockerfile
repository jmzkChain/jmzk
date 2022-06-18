FROM jmzkChain/builder:latest as builder
ARG branch=mainnet
ARG rootkey=jmzk6bVAZ9Kax64zCjfA7tYgxTvXMAxjML9LBvLT3tLtgvarTBZJ1J
ARG bjobs=$(nproc)
ARG awskey
ARG awssecret

RUN git clone -b $branch https://github.com/jmzkChain/jmzk.git --recursive \
    && cd jmzk && echo "$branch:$(git rev-parse HEAD)" > /etc/jmzk-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/tmp/build \
    -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ \
    -DCMAKE_AR=/usr/bin/gcc-ar -DCMAKE_RANLIB=/usr/bin/gcc-ranlib \
    -DSecp256k1_ROOT_DIR=/usr/local \
    -DENABLE_POSTGRES_SUPPORT=ON -DENABLE_BREAKPAD_SUPPORT=ON -DENABLE_MAINNET_BUILD=ON \
    -DENABLE_BUILD_LTO=OFF -Djmzk_ROOT_KEY=$rootkey
RUN ninja -C /tmp/build -j $bjobs jmzkd jmzkwd jmzkc && ninja -C /tmp/build install

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y python3 python3-click python3-boto3

RUN mkdir /tmp/build/symbols

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN python3 /jmzk/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/jmzkd
RUN python3 /jmzk/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/jmzkc
RUN python3 /jmzk/scripts/symbol_ops.py export -s /tmp/build/symbols /tmp/build/bin/jmzkwd

RUN python3 /jmzk/scripts/symbol_ops.py upload -f /tmp/build/symbols -k $awskey -s $awssecret -r jmzk-mainnet -b jmzk-symbols

FROM debian:buster-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y openssl libssl1.1 libllvm7 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/jmzk/bin
COPY --from=builder /etc/jmzk-version /etc
COPY --from=builder /jmzk/LICENSE.txt /opt/jmzk/

COPY config.ini /
COPY jmzkd.sh  /opt/jmzk/bin/jmzkd.sh
COPY jmzkwd.sh /opt/jmzk/bin/jmzkwd.sh
RUN  chmod +x /opt/jmzk/bin/jmzkd.sh
RUN  chmod +x /opt/jmzk/bin/jmzkwd.sh

ENV jmzk_ROOT=/opt/jmzk
ENV LD_LIBRARY_PATH /usr/local/lib

RUN mkdir /opt/jmzk/data
VOLUME /opt/jmzk/data

RUN mkdir /opt/jmzk/snapshots
VOLUME /opt/jmzk/snapshots

ENV PATH /opt/jmzk/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
