#!/usr/bin/env python3
"""mkdrv.py outdir : join nfslib.pas, shell.pas and nfsmain.pas into outdir/nfsdrv.pas.

The 1972 compiler has strings of ten characters only, so the shell's text is kept in an array TXT: every SAY('...') in shell.pas
becomes SAY(offset), and TXT (a VAR line and a VALUE list) is put in where nfslib.pas has the markers (*TXT*) and (*TXTV*).
A \\n in a text is a carriage return and a line feed."""
import os, re, sys

here = os.path.dirname(os.path.abspath(__file__))
out = sys.argv[1]
lib = open(here + '/nfslib.pas').read()
shell = open(here + '/shell.pas').read()
main = open(here + '/nfsmain.pas').read()
shell = shell.replace('(*EDIT*)', open(here + '/edit.pas').read())

txt = [0]                      # offset 0 is an empty message
offs = {}
def msg(m):
    s = m.group(1).replace("''", "'")
    if s not in offs:
        offs[s] = len(txt)
        for p in s.replace('\\n', '\r\n'):
            txt.append(ord(p))
        txt.append(0)
    return 'SAY(%d)' % offs[s]
shell = re.sub(r"SAY\('((?:[^']|'')*)'\)", msg, shell)
vals = ', '.join(str(v) for v in txt)
lines, cur = [], '   TXT = ('
for v in txt:
    t = str(v) + ', '
    if len(cur) + len(t) > 118:
        lines.append(cur.rstrip()); cur = '      '
    cur += t
lines.append(cur.rstrip().rstrip(',') + ');')
lib = lib.replace('(*TXT*)', 'TXT : ARRAY [0..%d] OF INTEGER;' % (len(txt) - 1))
lib = lib.replace('(*TXTV*)', '\n'.join(lines))
os.makedirs(out, exist_ok=True)
open(out + '/nfsdrv.pas', 'w').write(lib + shell + '\n' + main)
print('%s/nfsdrv.pas: %d text bytes' % (out, len(txt)))
