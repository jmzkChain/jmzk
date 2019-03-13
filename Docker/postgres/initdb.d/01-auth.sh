#!/bin/sh

set -e

{
        echo "# TYPE  DATABASE        USER            ADDRESS                 METHOD"
        echo "local   all             all                                     trust"
        echo "host    all             all             127.0.0.1/32            trust"
        echo "host    all             all             0.0.0.0/0               $AUTH"
} > "$POSTGRES_DATA/pg_hba.conf"
