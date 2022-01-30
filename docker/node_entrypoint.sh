#!/usr/bin/env bash

set -e

# if first arg looks like a flag, assume we want to run fuhon-node
if [ "${1:0:1}" = '-' ]; then
  set -- fuhon-node "$@"
fi

exec "$@"
