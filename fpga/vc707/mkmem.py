#!/usr/bin/env python3
"""mkmem.py runtime.img program.img mem.hex : merge two images into the $readmemh file that initialises the core's memory
(one 15 hex digit word per line, @address lines for the gaps)."""
import sys
m = {}
for f in sys.argv[1:3]:
    for l in open(f):
        a, w = l.split(); m[int(a, 8)] = int(w, 8)
with open(sys.argv[3], 'w') as o:
    nxt = 0
    for a in sorted(m):
        if a != nxt: o.write('@%x\n' % a)
        o.write('%015x\n' % m[a]); nxt = a + 1
