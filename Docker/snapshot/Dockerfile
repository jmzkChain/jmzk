FROM jmzkChain/pyenv:latest

RUN wget https://github.com/jmzkChain/jmzk/raw/master/scripts/snapshot_ops.py

ENV LC_ALL=C.UTF-8
ENV LANG=C.UTF-8

RUN mkdir data
VOLUME data

ENTRYPOINT ["python3", "snapshot_ops.py"]
