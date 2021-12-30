#!/usr/bin/env bash

set -e

exec fuhon-node --config fuhon-node.cfg --genesis genesis.car --repo fuhon-node-repo
