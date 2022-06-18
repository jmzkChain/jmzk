FROM debian:buster-slim as source
ARG branch=master

RUN apt-get update && DEBIAN_FRONTEND=noninteractive && apt-get install -y git
RUN git clone -b $branch https://github.com/jmzkChain/jmzk.git

FROM jmzkChain/pyjmzk:latest as pyjmzk
RUN python3 /pyjmzk/setup.py bdist_wheel


FROM jmzkChain/pyenv

WORKDIR /pysdk

COPY --from=pyjmzk /pyjmzk/dist/*.whl ./
RUN pip3 --no-cache-dir install *.whl
RUN rm *.whl

COPY --from=pyjmzk /usr/local/lib/*.so* /usr/local/lib/
COPY --from=source /jmzk/sdks/pysdk ./

ENV LD_LIBRARY_PATH /usr/local/lib
ENV PATH /usr/sbin:/usr/bin:/sbin:/bin

ENTRYPOINT ["python3", "-m", "pyjmzksdk.unit_test", "http://118.31.58.10:8888"]
