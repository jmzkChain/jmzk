FROM everitoken/builder:latest as builder
ARG branch=master

RUN git clone -b $branch https://github.com/everitoken/evt.git --recursive \
    && cd evt && echo "$branch:$(git rev-parse HEAD)" > /etc/evt-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
       -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_BIND_LIBRARIES=ON \
    && cmake --build /tmp/build --target install -- -j$(nproc)


FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y python3 openssl libssl1.1 && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/

WORKDIR /pyevt
COPY --from=builder /usr/local/*.so* /usr/local
COPY --from=builder /tmp/build/lib/libevt.so lib/
COPY --from=builder /tmp/build/lib/libfc_lite.so lib/
COPY --from=builder /tmp/build/lib/libevt_chain_lite.so lib/
COPY --from=builder /evt/bind/pyevt/pyevt/* .

ENTRYPOINT ["python", "/pyevt/unit_test.py"]
