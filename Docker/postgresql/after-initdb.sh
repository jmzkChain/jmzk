#!/usr/bin/env bash

mkdir $PGDATA/conf.d
sed -i "s/#include_dir = 'conf.d'/include_dir = 'conf.d'/" $PGDATA/postgresql.conf
cp /docker-entrypoint-initdb.d/*.conf $PGDATA/conf.d/

echo "Stop PG"
pg_ctl -D "$PGDATA" -m fast -w stop

echo "Start PG"
pg_ctl -D "$PGDATA" \
    -w start

echo "Reload done"
