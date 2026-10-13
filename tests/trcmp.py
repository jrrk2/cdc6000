#!/usr/bin/env python3
"""trcmp.py prog.img emu.trace rtl.trace : the first place where the issue trace of a core (+trace) differs from cdcemu -t (no-operations left out)."""
import sys
m = {}
for f in (sys.argv[1], 'host/runtime.img'):
    for l in open(f):
        a, w = l.split(); m.setdefault(int(a, 8), int(w, 8))
def norm(l):
    p, s = l.strip().split()[-1].split('/'); return '%o/%d' % (int(p, 8), int(s))
def isno(pw):
    p, s = pw.split('/'); w = m.get(int(p, 8), 0); par = (w >> (45 - 15 * int(s))) & 0x7fff
    return (par >> 9) == 0o46
e = [norm(l) for l in open(sys.argv[2]) if l.startswith('T')]
e = [x for x in e if not isno(x)]
r = [norm(l) for l in open(sys.argv[3]) if l.startswith('T')]
for i, (a, b) in enumerate(zip(e, r)):
    if a != b:
        print('first difference at issue #%d: emu %s, rtl %s' % (i, a, b)); print('emu :', e[max(0, i - 8):i + 3]); print('rtl :', r[max(0, i - 8):i + 3]); break
else:
    print('same up to', min(len(e), len(r)), '(emu %d, rtl %d)' % (len(e), len(r)))
