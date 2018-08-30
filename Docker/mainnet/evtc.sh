#!/bin/bash

# Usage:
# Go into cmd loop: sudo ./evtc.sh
# Run single cmd:  sudo ./evtc.sh <evtc paramers>

PREFIX="evtc"
if [ -z $1 ] ; then
  while :
  do
    read -e -p "evtc " cmd
    history -s "$cmd"
    $PREFIX $cmd
  done
else
  $PREFIX $@
fi
