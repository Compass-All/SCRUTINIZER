#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"

pushd $SCRIPT_DIR/../dev_workspace/OpenCSD/decoder/build/linux
make
popd
