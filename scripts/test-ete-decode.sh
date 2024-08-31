#!/bin/bash

SCRIPT_DIR="$(dirname $(readlink -f "$0"))"
source $SCRIPT_DIR/config.sh 


export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:"$THIRD_PARTY_DIR/OpenCSD/decoder/lib/builddir"

$THIRD_PARTY_DIR/OpenCSD/decoder/tests/bin/builddir/trc_pkt_lister -ss_dir $ETE_DECODER -decode -id 0x2 -logfilename $ETE_DECODER/decode.txt
