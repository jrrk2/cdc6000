#!/bin/bash
# Simulate the whole VC707 design (core + UART + FIFOs) in Verilator, with the host playing the UART.
#   run.sh <input text> <outdir>        mem.hex must exist in the current directory (see ../mkmem.py)
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
RTL=$HERE/../../../rtl
OBJ=${OBJ:-$PWD/sim_obj}
verilator --cc --exe --build -j 8 -Wno-fatal -Wno-lint -Wno-style --top-module vc707_cdc_top -CFLAGS -O2 \
  --Wno-widthtrunc --Wno-widthexpand -Mdir "$OBJ" "$HERE/xilinx_stubs.v" $RTL/cdc_pkg.sv $RTL/cdc6000_core.sv \
  $HERE/../simpleuart.v $HERE/../vc707_cdc_top.v $HERE/tb.cpp > "$OBJ.log" 2>&1 || { tail -30 "$OBJ.log"; exit 1; }
mkdir -p "$2"
"$OBJ/Vvc707_cdc_top" -i "$1" -o "$2/listing.txt" -d "$2"
