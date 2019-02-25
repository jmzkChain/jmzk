#!/bin/sh

set -e

# Perform all actions as $POSTGRES_USER
export PGUSER="$POSTGRES_USER"

# Load pg_pathman into $POSTGRES_DB
for DB in "$POSTGRES_DB"; do
    echo "Loading pg_pathman extensions into $DB"
    "${psql[@]}" <<-'EOSQL'
        CREATE EXTENSION IF NOT EXISTS pg_pathman;
EOSQL
done
