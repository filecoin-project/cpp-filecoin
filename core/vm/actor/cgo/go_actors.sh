#!/bin/sh
set -e

if ! [ -e go_actors.sha ] || ! shasum -s -c go_actors.sha; then
  go build -buildmode=c-archive go_actors.go cgo.go
  shasum go_actors.go go.mod go.sum c_actors.h go_actors.sh > go_actors.sha
fi
