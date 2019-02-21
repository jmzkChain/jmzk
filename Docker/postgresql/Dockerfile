FROM postgres:11.2

ARG POSTGRES_DEV_VERSION=11
ARG PATHMAN_VERSION=1.5.3

RUN set -x \
	&& apt-get update && apt-get install -y --no-install-recommends ca-certificates wget unzip make postgresql-server-dev-${POSTGRES_DEV_VERSION} gcc libc6-dev libssl-dev libkrb5-dev \
    && wget -O /tmp/pg_pathman.zip https://github.com/postgrespro/pg_pathman/archive/$PATHMAN_VERSION.zip \
	&& unzip /tmp/pg_pathman.zip  -d /tmp \
    && cd /tmp/pg_pathman-${PATHMAN_VERSION} \
    && make USE_PGXS=1 install \
    && rm -rf /var/lib/apt/lists/* && apt-get purge -y --auto-remove ca-certificates wget unzip make postgresql-server-dev-${POSTGRES_DEV_VERSION} gcc libc6-dev libssl-dev libkrb5-dev \
    && rm -rf /tmp/pg_*

ENV POSTGRES_USERNAME="postgres" \
    POSTGRES_PASSWORD="" \
    PGDATA="/bitnami/postgresql"

COPY initdb.d/* /docker-entrypoint-initdb.d/
COPY after-initdb.sh /docker-entrypoint-initdb.d/0-after-initdb.sh
