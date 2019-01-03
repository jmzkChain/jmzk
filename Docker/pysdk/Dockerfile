FROM debian:buster-slim as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/everitoken/evt.git

FROM everitoken/pyevt:latest as pyevt
RUN python3 /pyevt/setup.py bdist_wheel

FROM debian:buster-slim

FROM everitoken/pyenv

WORKDIR /pysdk

COPY --from=pyevt /pyevt/dist/*.whl ./
RUN pip3 --no-cache-dir install *.whl
RUN rm *.whl

COPY --from=pyevt /usr/local/lib/*.so* /usr/local/lib/
COPY --from=source /evt/sdks/pysdk ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pyevtsdk.unit_test", "http://118.31.58.10:8888"]
