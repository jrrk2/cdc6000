#!/usr/bin/env python3
"""Convert pascal.pas (ASCII transliteration) to the 1972 CDC listing notation
that the compiler itself understands: comment brackets, AND/OR/NOT, <> <= >=,
quote = triple-bar.  UTF-8 glyphs are decoded by the host runtime."""
import re, sys
src = open(sys.argv[1], encoding='utf-8').read()
TOK = re.compile(r"\(\*.*?\*\)|'(?:[^']|'')*'", re.S)
out, pos = [], 0
def code(t):
    t = re.sub(r'\bAND\b', '∧', t)
    t = re.sub(r'\bOR\b', '∨', t)
    t = re.sub(r'\bNOT\b', '¬', t)
    t = t.replace('<>', '≠').replace('<=', '≤').replace('>=', '≥')
    return t
for m in TOK.finditer(src):
    out.append(code(src[pos:m.start()]))
    t = m.group(0)
    if t.startswith('(*'):
        body = t[2:-2]
        out.append('→' + body + '↓')
    else:
        out.append('≡' + t[1:-1].replace("''", "≡≡").replace('↗', '→') + '≡')
    pos = m.end()
out.append(code(src[pos:]))
sys.stdout.write(''.join(out))
