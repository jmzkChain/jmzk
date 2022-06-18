FROM jmzkChain/builder:latest as builder
ARG branch=master

RUN git clone -b $branch https://github.com/jmzkChain/jmzk.git --recursive \
    && cd jmzk && echo "$branch:$(git rev-parse HEAD)" > /etc/jmzk-version \
    && cmake -H. -B"/tmp/build" -G"Ninja" -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local  -DSecp256k1_ROOT_DIR=/usr/local -DENABLE_BIND_LIBRARIES=ON
RUN ninja -C /tmp/build -j $(nproc) libjmzk && ninja -C /tmp/build install

FROM jmzkChain/pyenv:latest

WORKDIR /pyjmzk
COPY --from=builder /usr/local/lib/*.so* /usr/local/lib/
COPY --from=builder /jmzk/bind/pyjmzk ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pyjmzk.unit_test"]

