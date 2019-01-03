FROM everitoken/builder:latest as builder
ARG branch=master

RUN git clone -b $branch https://github.com/everitoken/evt.git --recursive \
    && cd evt && echo "$branch:$(git rev-parse HEAD)" > /etc/evt-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=/usr/local  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_BIND_LIBRARIES=ON
RUN ninja -C /tmp/build -j $(nproc) libevt && ninja -C /tmp/build install


FROM debian:buster-slim

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-setuptools python3-wheel python3-cffi openssl libssl1.1 --no-install-recommends && rm -rf /var/lib/apt/lists/*

WORKDIR /pyevt
COPY --from=builder /usr/local/lib/*.so* /usr/local/lib/
COPY --from=builder /evt/bind/pyevt ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pyevt.unit_test"]

