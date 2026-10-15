#!/bin/sh
# mkimage_shell.sh dir : the memory image of the board in shell mode -- the run time (boots into the shell), the Pascal compiler
# (started by `compile`) and the NFS driver with the shell -- as mem.hex, cdc_ram_bram.v and cdc_ram_bram.init.tcl in dir;
# and the ramdisk (DISK_KB, default 1024 KB) with pascal.pas and pascimg as cdc_disk_bram.v and disk.hex.
# The export on the server needs a file pascimg (written to dir too: the compiler image the run time reloads for a second compile).
set -e
cd "$(dirname "$0")/../.."
mkdir -p "$1"
cd nfs
python3 mkdrv.py build
./hostpc.sh build/nfsdrv.pas 2047 build
H=../host
python3 $H/mkrt.py $H/runtime_shell.asm --nfs build/parmlist.out --shell $H/run/parmlist.out
python3 $H/cdcasm.py $H/runtime_shell.asm $H/runtime_shell.img
python3 $H/pasclgo2img.py build/pasclgo.out build/parmlist.out build/drv.img --noboot
python3 $H/pasclgo2img.py $H/run/pasclgo.out $H/run/parmlist.out build/compiler_shell.img --compiler --noboot
cd ..
cp nfs/build/compiler_shell.img "$1/pascimg"
python3 - "$1" <<'PY'
import sys
d = sys.argv[1]
m = {}
for f in ('host/runtime_shell.img', 'nfs/build/compiler_shell.img', 'nfs/build/drv.img'):
    for l in open(f):
        a, w = l.split(); m[int(a, 8)] = int(w, 8)
with open(d + '/mem.hex', 'w') as o:
    nxt = 0
    for a in sorted(m):
        if a != nxt: o.write('@%x\n' % a)
        o.write('%015x\n' % m[a]); nxt = a + 1
PY
python3 fpga/vc707-eth/genmem.py "$1/cdc_ram_bram.v" host/runtime_shell.img nfs/build/compiler_shell.img nfs/build/drv.img
# the ramdisk (DISK_KB kilobytes, default 1024; 0: none): the compiler's source in the 1972 notation and its image
python3 host/tocdc.py ocr/pascal.pas > "$1/pascal.pas"
python3 fpga/vc707-eth/gendisk.py "$1/cdc_disk_bram.v" "$1/disk.hex" "${DISK_KB:-1024}" "pascal.pas=$1/pascal.pas" "pascimg=$1/pascimg"
wc -l "$1/mem.hex"
