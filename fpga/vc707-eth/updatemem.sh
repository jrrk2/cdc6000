#!/bin/sh
# updatemem.sh imagedir builddir : a software change without Vivado's place and route -- the memory contents of imagedir/mem.hex
# (mkimage.sh or mkimage_shell.sh) put into the bitstream of the full build in builddir (build.tcl: out/vc707_cdc_eth.bit and
# out/cdc_ram.mmi).  Takes about 15 s; writes imagedir/vc707_cdc_eth.bit.  Needs Vivado's settings64.sh sourced (updatemem).
set -e
img=$1; bld=$2
python3 - "$img" <<'PY'
import sys
d = sys.argv[1]
m = {}; cur = 0
for l in open(d + '/mem.hex'):
    l = l.strip()
    if l.startswith('@'): cur = int(l[1:], 16)
    elif l: m[cur] = int(l, 16); cur += 1
with open(d + '/full.mem', 'w') as o:          # one block of 64K words of 15 hex digits (60 bits); the space is bytes, 8 a word
    o.write('@0\n')
    for a in range(65536): o.write('%015x\n' % m.get(a, 0))
PY
updatemem -meminfo "$bld/out/cdc_ram.mmi" -data "$img/full.mem" -proc cdc_ram -bit "$bld/out/vc707_cdc_eth.bit" \
    -out "$img/vc707_cdc_eth.bit" -force > "$img/updatemem.log" 2>&1
grep -q "update_mem completed successfully" "$img/updatemem.log" || { cat "$img/updatemem.log"; exit 1; }
ls -la "$img/vc707_cdc_eth.bit"
