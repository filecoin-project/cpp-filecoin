#!/usr/bin/env bash

set -e

# if first arg looks like a flag, assume we want to run fuhon-miner
if [ "${1:0:1}" = '-' ]; then
  set -- fuhon-miner "$@"
fi

exec "$@"
