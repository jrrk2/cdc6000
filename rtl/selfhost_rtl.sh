#!/bin/sh
# The compiler compiles itself on the Verilated CDC 6000 core, twice, and the results are compared with the
# reference run of host/selfhost.sh: stage1 image -> stage2 output -> stage2 image -> stage3 output.
set -e
cd "$(dirname "$0")"
H=../host
make -s
[ -f $H/stage2/pasclgo.out ] || $H/selfhost.sh
mkdir -p r2 r3
CORE=${CORE:-./obj_dir/Vcdc6000_core}      # CORE=./obj_dir_sb/Vcdc6000_sb for the scoreboarded core (make sb)
$CORE $H/runtime.img $H/stage2/in.img -i $H/pascal.cdc -o r2/listing.txt -d r2 -m r2/mem.img
python3 $H/memparm.py r2/mem.img 2048 r2/parmlist.out
python3 $H/pasclgo2img.py r2/pasclgo.out r2/parmlist.out r3/in.img --compiler
$CORE $H/runtime.img r3/in.img -i $H/pascal.cdc -o r3/listing.txt -d r3 -m r3/mem.img
python3 $H/memparm.py r3/mem.img 2048 r3/parmlist.out
cmp r2/pasclgo.out $H/run/pasclgo.out && cmp r2/parmlist.out $H/run/parmlist.out && cmp r2/listing.txt $H/stage2/listing.txt
cmp r2/pasclgo.out r3/pasclgo.out && cmp r2/listing.txt r3/listing.txt && cmp r2/parmlist.out r3/parmlist.out
echo "RTL SELF-HOSTING FIXED POINT: host stage1 == RTL stage2 == RTL stage3"
