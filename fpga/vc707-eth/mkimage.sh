#!/bin/sh
# mkimage.sh dir : mem.hex for the board in dir -- runtime (NFS mode), the Pascal compiler as the program, and the NFS driver.
# The export on the server then needs a file `input` (the source, e.g. host/pascal.cdc); the compiler writes `output`,
# `pasclgo` and `pmdfile` there.
set -e
cd "$(dirname "$0")/../.."
mkdir -p "$1"
nfs/selfhost_nfs.sh emu > /dev/null            # builds runtime_nfs.img, nfs/build/{drv,compiler}.img
python3 fpga/vc707/mkmem.py host/runtime_nfs.img nfs/build/compiler.img "$1/mem0.hex"
python3 - "$1" <<'PY'
import sys
d = sys.argv[1]
m = {}
for f in ('host/runtime_nfs.img', 'nfs/build/compiler.img', 'nfs/build/drv.img'):
    for l in open(f):
        a, w = l.split(); m[int(a, 8)] = int(w, 8)
with open(d + '/mem.hex', 'w') as o:
    nxt = 0
    for a in sorted(m):
        if a != nxt: o.write('@%x\n' % a)
        o.write('%015x\n' % m[a]); nxt = a + 1
PY
rm -f "$1/mem0.hex"
python3 fpga/vc707-eth/genmem.py "$1/cdc_ram_bram.v" host/runtime_nfs.img nfs/build/compiler.img nfs/build/drv.img
wc -l "$1/mem.hex"
