#!/bin/sh
# The compiler compiles itself on the Verilated CDC 6000 core, twice, and the results are compared with the
# reference run of host/selfhost.sh: stage1 image -> stage2 output -> stage2 image -> stage3 output.
set -e
cd "$(dirname "$0")/../rtl"
H=../host
true
[ -f $H/stage2/pasclgo.out ] || $H/selfhost.sh
mkdir -p r2sb r3sb
CORE=${CORE:-./obj_dir/Vcdc6000_core}      # CORE=./obj_dir_sb/Vcdc6000_sb for the scoreboarded core (make sb)
$CORE $H/runtime.img $H/stage2/in.img -i $H/pascal.cdc -o r2sb/listing.txt -d r2sb -m r2sb/mem.img
python3 $H/memparm.py r2sb/mem.img 2048 r2sb/parmlist.out
python3 $H/pasclgo2img.py r2sb/pasclgo.out r2sb/parmlist.out r3sb/in.img --compiler
$CORE $H/runtime.img r3sb/in.img -i $H/pascal.cdc -o r3sb/listing.txt -d r3sb -m r3sb/mem.img
python3 $H/memparm.py r3sb/mem.img 2048 r3sb/parmlist.out
cmp r2sb/pasclgo.out $H/run/pasclgo.out && cmp r2sb/parmlist.out $H/run/parmlist.out && cmp r2sb/listing.txt $H/stage2/listing.txt
cmp r2sb/pasclgo.out r3sb/pasclgo.out && cmp r2sb/listing.txt r3sb/listing.txt && cmp r2sb/parmlist.out r3sb/parmlist.out
echo "RTL SELF-HOSTING FIXED POINT: host stage1 == RTL stage2 == RTL stage3"
