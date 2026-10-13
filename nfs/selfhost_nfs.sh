#!/bin/sh
# The compiler compiles itself with every file reached over NFS: the source is read from, and the listing and code are
# written to, an export directory served by nfs/netmodel.h.  usage: selfhost_nfs.sh [emu|rtl]
set -e
cd "$(dirname "$0")"
H=../host
which=${1:-emu}
python3 mkdrv.py build
./hostpc.sh build/nfsdrv.pas 2047 build
python3 $H/mkrt.py $H/runtime_nfs.asm --nfs build/parmlist.out
python3 $H/cdcasm.py $H/runtime_nfs.asm $H/runtime_nfs.img
python3 $H/pasclgo2img.py build/pasclgo.out build/parmlist.out build/drv.img --noboot
python3 $H/pasclgo2img.py $H/run/pasclgo.out $H/run/parmlist.out build/compiler.img --compiler --b4=0x9c00
rm -rf export_$which && mkdir export_$which && cp $H/pascal.cdc export_$which/input
if [ $which = rtl ]; then
  ../rtl/obj_dir/Vcdc6000_core $H/runtime_nfs.img build/compiler.img build/drv.img -r export_$which
else
  $H/cdcemu $H/runtime_nfs.img build/compiler.img build/drv.img -r export_$which
fi
python3 - "$which" <<'PY'
import sys
d = 'export_' + sys.argv[1]
a = open(d + '/pasclgo').read().split(); b = open('../host/run/pasclgo.out').read().split()
assert a == b, 'pasclgo differs'
assert open(d + '/output').read() == open('../host/stage2/listing.txt').read(), 'listing differs'
print('NFS SELF-HOSTING (%s): code and listing identical to the reference' % sys.argv[1])
PY
