FROM bitnami/minideb-extras:stretch-r288
LABEL maintainer "Bitnami <containers@bitnami.com>"

ENV BITNAMI_PKG_CHMOD="-R g+rwX" \
    HOME="/"

# Install required system packages and dependencies
RUN install_packages libbsd0 libc6 libedit2 libgcc1 libicu57 liblzma5 libncurses5 libnss-wrapper libssl1.1 libstdc++6 libtinfo5 libxml2 libxslt1.1 zlib1g
RUN bitnami-pkg unpack postgresql-11.2.0-0 --checksum 9e1a4e02db6b41240d735830b263a1d7190aa55bcf82e664069dc28b4d8ae035
RUN echo 'en_GB.UTF-8 UTF-8' >> /etc/locale.gen && locale-gen
RUN echo 'en_US.UTF-8 UTF-8' >> /etc/locale.gen && locale-gen

RUN mkdir /docker-entrypoint-initdb.d /opt/bitnami/postgresql/conf/
RUN chmod -R g+rwX /opt/bitnami/postgresql/conf/

COPY rootfs /
COPY initdb.d/* /docker-entrypoint-initdb.d/

ENV BITNAMI_APP_NAME="postgresql" \
    BITNAMI_IMAGE_VERSION="11.2.0-debian-9-r4" \
    LANG="en_US.UTF-8" \
    LANGUAGE="en_US:en" \
    NAMI_PREFIX="/.nami" \
    PATH="/opt/bitnami/postgresql/bin:$PATH" \
    POSTGRESQL_CLUSTER_APP_NAME="walreceiver" \
    POSTGRESQL_DATABASE="" \
    POSTGRESQL_DATA_DIR="/bitnami/postgresql" \
    POSTGRESQL_INITDB_ARGS="" \
    POSTGRESQL_INITDB_WALDIR="" \
    POSTGRESQL_MASTER_HOST="" \
    POSTGRESQL_MASTER_PORT_NUMBER="5432" \
    POSTGRESQL_NUM_SYNCHRONOUS_REPLICAS="0" \
    POSTGRESQL_PASSWORD="" \
    POSTGRESQL_PORT_NUMBER="5432" \
    POSTGRESQL_REPLICATION_MODE="master" \
    POSTGRESQL_REPLICATION_PASSWORD="" \
    POSTGRESQL_REPLICATION_USER="" \
    POSTGRESQL_SYNCHRONOUS_COMMIT_MODE="on" \
    POSTGRESQL_USERNAME="postgres"

RUN echo "deb http://ftp.cn.debian.org/debian sid main" >> /etc/apt/sources.list
# Compile PG_PATHMAN
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

EXPOSE 5432

USER 1001
ENTRYPOINT [ "/app-entrypoint.sh" ]
CMD [ "/run.sh" ]
