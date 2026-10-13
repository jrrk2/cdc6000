#!/bin/sh
# sbvariants.sh [n] : build the scoreboarded core with several unit latencies, unit counts and stack sizes, and run n random programs on each.
cd "$(dirname "$0")/.."
n=${1:-150}
build() { # name, parameters
  d=${TMPDIR:-/tmp}/sbv_$1
  if [ ! -x $d/Vcdc6000_sb ] || [ rtl/cdc6000_sb.sv -nt $d/Vcdc6000_sb ]; then
    rm -rf $d; mkdir -p $d
    (cd rtl && verilator -CFLAGS -O2 -CFLAGS -DCORE_SB --exe --Wno-fatal --Wno-widthtrunc --Wno-widthexpand --Wno-multidriven --Wno-BLKANDNBLK \
       -Wno-UNOPTFLAT -Wno-lint --public-flat-rw -Mdir $d --cc cdc_pkg.sv cdc6000_sb.sv --top-module cdc6000_sb $2 main_cdc.cpp > $d/build.log 2>&1 \
       && make -C $d -f Vcdc6000_sb.mk > $d/make.log 2>&1) || { echo "build $1 failed"; tail -5 $d/build.log; return 1; }
  fi
  printf "%-8s %s: " "$1" "$2"
  RTL=$d/Vcdc6000_sb tests/randcmp.sh 1 $n 120 sb | tail -3
}
build slow  "-GLAT_BOOL=1 -GLAT_SHIFT=2 -GLAT_INC=2 -GNINC=2 -GMEMLAT=2"
build one   "-GNINC=1 -GLAT_INC=1 -GMEMLAT=1 -GNSK=4"
build three "-GNINC=3 -GLAT_INC=3 -GLAT_BOOL=2 -GNSK=16 -GMEMLAT=2"
build deep  "-GLAT_BOOL=3 -GLAT_SHIFT=4 -GLAT_INC=4 -GNINC=2 -GMEMLAT=2 -GNSK=2"
