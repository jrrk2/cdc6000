#!/usr/bin/env python3
"""memparm.py memdump.img ico parmlist.out : read PARMLIST (11 words at ico+1) of a finished compiler run from a memory dump."""
import sys
m = {}
for l in open(sys.argv[1]):
    a, w = l.split(); m[int(a, 8)] = int(w, 8)
ico = int(sys.argv[2]); b = ico + 1
g = lambda k: m.get(b + k, 0)
with open(sys.argv[3], 'w') as f:
    f.write('ICO %d\nLPJMPTAB %d\nLOADPT %d\nLIMCODE %d\nENTRYPT %d\nERRFLAG %d\n' % (g(0), g(8), g(6), g(7), g(10), 1 if g(1) else 0))
