FROM everitoken/builder:latest as builder
ARG branch=master

RUN git clone -b $branch https://github.com/everitoken/evt.git --recursive \
    && cd evt && echo "$branch:$(git rev-parse HEAD)" > /etc/evt-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_BIND_LIBRARIES=ON
RUN ninja -C /tmp/build -j $(nproc) libevt && ninja -C /tmp/build install


FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 python3-pip openssl libssl1.1 && rm -rf /var/lib/apt/lists/*
RUN pip3 install cffi wheel

WORKDIR /pyevt
COPY --from=builder /usr/local/lib/*.so* /usr/local/lib/
COPY --from=builder /tmp/build/lib/libevt.so lib/
COPY --from=builder /tmp/build/lib/libfc_lite.so lib/
COPY --from=builder /tmp/build/lib/libevt_chain_lite.so lib/
COPY --from=builder /evt/bind/pyevt/pyevt/* ./

ENV LD_LIBRARY_PATH /pyevt/lib:/usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "/pyevt/unit_test.py"]
