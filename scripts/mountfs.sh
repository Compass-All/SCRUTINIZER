#!/bin/bash
SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 

FSPATH=$PROJ_DIR/$ROOTFS
MOUNTPT="/mnt"

sudo  mount  -t ext4  -o  loop   -w    $FSPATH   $MOUNTPT
