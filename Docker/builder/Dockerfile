FROM ubuntu:18.04

LABEL author="Harry <harrywong@live.com>" maintainer="Harry <harrywong@live.com>" version="1.1.0" \
  description="This is a base image for building everitoken/evt"

RUN echo 'APT::Install-Recommends 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && echo 'APT::Install-Suggests 0;' >> /etc/apt/apt.conf.d/01norecommends \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y sudo wget curl net-tools ccache ninja-build ca-certificates unzip gnupg git build-essential libtool autoconf automake m4 openssl libssl1.1 libssl-dev libgmp-dev zlib1g-dev

RUN wget --no-check-certificate https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.sh \
    && bash cmake-3.12.3-Linux-x86_64.sh --prefix=/usr/local --exclude-subdir --skip-license \
    && rm cmake-3.12.3-Linux-x86_64.sh

RUN wget https://dl.bintray.com/boostorg/release/1.68.0/source/boost_1_68_0.tar.bz2 -O - | tar -xj \
    && cd boost_1_68_0 \
    && ./bootstrap.sh --prefix=/usr/local \
    && ./b2 -d0 -j$(nproc) --with-thread --with-date_time --with-system --with-filesystem --with-program_options \
       --with-signals --with-serialization --with-chrono --with-test --with-context --with-locale --with-coroutine --with-iostreams toolset=gcc link=shared install \
    && cd .. && rm -rf boost_1_68_0

RUN git clone --depth 1 git://github.com/cryptonomex/secp256k1-zkp \
    && cd secp256k1-zkp \
    && ./autogen.sh \
    && ./configure --prefix=/usr/local \
    && make -j$(nproc) install \
    && cd .. && rm -rf secp256k1-zkp

RUN git clone --depth 1 -b r1.13 git://github.com/mongodb/mongo-c-driver \
    && cd mongo-c-driver \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DENABLE_SSL=OPENSSL -DENABLE_STATIC=OFF -DENABLE_BSON=ON -DENABLE_TESTS=OFF \
    && cmake --build build --target install \
    && cd .. && rm -rf mongo-c-driver

RUN git clone --depth 1 -b releases/stable git://github.com/mongodb/mongo-cxx-driver \
    && cd mongo-cxx-driver \
    && cmake -H. -Bbuild -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release  -DCMAKE_INSTALL_PREFIX=/usr/local\
    && cmake --build build --target install \
    && cd .. && rm -rf mongo-cxx-driver

RUN git clone --depth 1 --single-branch --branch master https://github.com/lz4/lz4.git \
    && cd lz4 \
    && make -j${nproc} install \
    && cd .. && rm -rf lz4

RUN git clone --depth 1 --single-branch --branch master https://github.com/facebook/zstd.git \
    && cd zstd \
    && make -j${nproc} install \
    && cd .. && rm -rf zstd

RUN git clone --depth 1 --single-branch --branch master https://github.com/jemalloc/jemalloc.git \
    && cd jemalloc \
    && ./autogen.sh \
    && ./configure --prefix=/usr/local \
    && make -j${nproc} install_bin install_include install_lib \
    && cd .. && rm -rf jemalloc

RUN git clone -b master https://github.com/google/benchmark.git \
    && cd benchmark \
    && cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_ENABLE_LTO=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local\
    && cmake --build build --target install \
    && cd .. && rm -rf benchmark

RUN git clone -b master https://github.com/everitoken/breakpad.git \
    && cd breakpad \
    && ./configure --prefix=/usr/local\
    && make -j$(nproc) install \
    && cd .. && rm -rf breakpad

RUN wgt https://ftp.postgresql.org/pub/source/v11.1/postgresql-11.1.tar.bz2 -O - | tar -xj \
    && cd postgresql-11.1 \
    && ./configure --prefix=/usr/local \
    && make -C src/include install \
    && make -C src/interfaces install \
    && cd .. && rm -rf postgresql-11.1

RUN cp /usr/bin/ccache /usr/local/bin/
RUN ln -s ccache /usr/local/bin/gcc
RUN ln -s ccache /usr/local/bin/g++
RUN ln -s ccache /usr/local/bin/cc
RUN ln -s ccache /usr/local/bin/c++
