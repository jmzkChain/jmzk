#!/bin/sh
cd /opt/evt/bin

if [ -f '/opt/evt/etc/evtd/config.ini' ]; then
    echo
  else
    mkdir /opt/evt/etc/evtd
    cp /config.ini /opt/evt/etc/evtd
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
    CONFIG_DIR="--config-dir=/opt/evt/etc/evtd"
else
    CONFIG_DIR=""
fi

DATA_DIR="--data-dir=/opt/evt/data"
SNAPSHOTS_DIR="--snapshots-dir=/opt/evt/snapshots"

exec /opt/evt/bin/evtd $CONFIG_DIR $DATA_DIR $SNAPSHOTS_DIR $@
