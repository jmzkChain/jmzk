#!/bin/sh
cd /opt/jmzk/bin

if [ -f '/opt/jmzk/etc/jmzkd/config.ini' ]; then
    echo
  else
    mkdir /opt/jmzk/etc
    mkdir /opt/jmzk/etc/jmzkd
    cp /config.ini /opt/jmzk/etc/jmzkd
fi

while :; do
    case $1 in
        --config-dir=?*)
            CONFIG_DIR=${1#*=}
            ;;
        *)
            break
    esac
    shift
done

if [ ! "$CONFIG_DIR" ]; then
    CONFIG_DIR="--config-dir=/opt/jmzk/etc/jmzkd"
else
    CONFIG_DIR=""
fi

DATA_DIR="--data-dir=/opt/jmzk/data"
SNAPSHOTS_DIR="--snapshots-dir=/opt/jmzk/snapshots"

exec /opt/jmzk/bin/jmzkd $CONFIG_DIR $DATA_DIR $SNAPSHOTS_DIR $@
