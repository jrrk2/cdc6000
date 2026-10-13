#!/bin/bash
# Simulate cdc_eth_core (the board without its PHY) running the program in mem.hex against the server model.
#   run.sh <export dir> [-u uartinput] [-n maxcycles]   mem.hex must be in the current directory (../mkimage.sh builds it)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$HERE/../../..
OBJ=${OBJ:-$PWD/sim_obj}
ETH=$REPO/fpga/eth-rtl
verilator --cc --exe --build -j 8 -Wno-fatal -Wno-lint -Wno-style --top-module cdc_eth_core -CFLAGS -O2 \
  --Wno-widthtrunc --Wno-widthexpand --public-flat-rw -Mdir "$OBJ" \
  $REPO/rtl/cdc_pkg.sv $REPO/rtl/cdc6000_core.sv $HERE/../cdc_eth_core.v $ETH/simpleuart.v $ETH/eth_stream_dma.sv \
  $HERE/cdc_ram_sim.v $HERE/tb.cpp > "$OBJ.log" 2>&1 || { tail -30 "$OBJ.log"; exit 1; }
"$OBJ/Vcdc_eth_core" -r "$1" "${@:2}"
