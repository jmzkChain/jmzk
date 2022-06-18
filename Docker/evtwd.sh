#!/bin/sh
cd /opt/jmzk/bin

if [ -f '/opt/jmzk/etc/jmzkwd/config.ini' ]; then
    echo
  else
    mkdir /opt/jmzk/etc
    mkdir /opt/jmzk/etc/jmzkwd
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
    CONFIG_DIR="--config-dir=/opt/jmzk/etc/jmzkwd"
else
    CONFIG_DIR=""
fi

WALLET_DIR="--wallet-dir=/opt/jmzk/data/wallet"
DATA_DIR="--data-dir=/opt/jmzk/data/wallet"

exec /opt/jmzk/bin/jmzkwd $CONFIG_DIR $DATA_DIR $WALLET_DIR $@
