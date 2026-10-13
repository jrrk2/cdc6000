#!/usr/bin/env python3
"""Derive an FPC (-Miso) host build of the 1972 CDC 6000 Pascal compiler from
the faithful transcription ocr/pascal.pas.  Nothing here changes pascal.pas;
every dialect adaptation is done to the derived pascal_host.pas.

Host model of the CDC machine:
  * CHAR      -> enumeration D00..D63 holding CDC display code (63-char set)
  * ALFA      -> int64 holding ten packed 6-bit display-code characters
  * words     -> int64 holding the VALUE of a 60-bit ones-complement integer
  * heap CLASSes -> arrays + ALLOC/RESET bump allocator (pointer order kept)
"""
import re, sys, os

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', 'ocr', 'pascal.pas')
OUT = os.path.join(HERE, 'pascal_host.pas')

# ---- CDC display code (63 character set) -------------------------------
DISPLAY = {}
for i, c in enumerate('ABCDEFGHIJKLMNOPQRSTUVWXYZ'):
    DISPLAY[c] = i + 1
for i, c in enumerate('0123456789'):
    DISPLAY[c] = 27 + i
for c, n in {'+': 37, '-': 38, '*': 39, '/': 40, '(': 41, ')': 42, '$': 43,
             '=': 44, ' ': 45, ',': 46, '.': 47, '≡': 48, '[': 49,
             ']': 50, ':': 51, '≠': 52, '→': 53, '∨': 54,
             '∧': 55, '↑': 56, '↓': 57, '<': 58, '>': 59,
             '≤': 60, '≥': 61, '¬': 62, ';': 63}.items():
    DISPLAY[c] = n
# transcription conventions used in pascal.pas
DISPLAY["'"] = 48          # the quote character is code 60B (printed as the triple bar)
DISPLAY['^'] = 56          # up arrow
DISPLAY['↗'] = 53     # '↗' was used for the comment-open arrow


def dcode(ch):
    if ch not in DISPLAY:
        raise SystemExit('no display code for %r' % ch)
    return DISPLAY[ch]


src = open(SRC, encoding='utf-8').read()

# ---- 1. split into comments / strings / code ---------------------------
TOK = re.compile(r"\(\*.*?\*\)|'(?:[^']|'')*'", re.S)
parts = []
pos = 0
for m in TOK.finditer(src):
    if m.start() > pos:
        parts.append(['code', src[pos:m.start()]])
    t = m.group(0)
    parts.append(['comment' if t.startswith('(*') else 'str', t])
    pos = m.end()
parts.append(['code', src[pos:]])


def join_masked():
    """code with strings/comments replaced by placeholders"""
    out = []
    store = []
    for k, t in parts:
        if k == 'code':
            out.append(t)
        else:
            store.append((k, t))
            out.append('\x00%d\x00' % (len(store) - 1))
    return ''.join(out), store


code, store = join_masked()


def strval(ph):
    k, t = store[int(ph)]
    assert k == 'str'
    return t[1:-1].replace("''", "'")


# ---- 2. VALUE section -> INITVALUES ------------------------------------
vs = code.index('   VALUE\n')
ve = code.index('\x00', code.index('TWO STANDARD PROCEDURES') - 40) if False else None
# the VALUE section ends at the comment that starts "(*TWO STANDARD PROCEDURES"
for idx, (k, t) in enumerate(store):
    if k == 'comment' and 'TWO STANDARD PROCEDURES' in t:
        ve = code.index('\x00%d\x00' % idx)
        break
value_text = code[vs + len('   VALUE\n'):ve]
code = code[:vs] + code[ve:]

VALTYPES = {  # name: (low index, element kind)
    'TWOTO17': (None, 'int'), 'BLANK': (None, 'alfa'),
    'SPLITSTAT': (0, 'int'), 'ERRCL': (0, 'enum'), 'TERRCL': (0, 'enum'),
    'WD': (0, 'alfa'), 'WNO': (0, 'int'), 'WCL': (0, 'int'), 'WL': (0, 'int'),
    'SYMNO': ('D37', 'int'), 'SYMCL': ('D37', 'int'),
    'JB1': (0, 'char'), 'JB2': (0, 'char'), 'JX1': (0, 'char'),
    'JX2': (0, 'char'), 'REL': (0, 'char'), 'FS': (0, 'char'),
    'R1': (0, 'char'), 'SH': (12, 'char'), 'INITNAM': (-1, 'alfa'),
}


def octal_or_dec(tok):
    if re.fullmatch(r'[0-7]+B', tok):
        return str(octval(tok[:-1]))
    return tok


def split_top(s, sep=','):
    out, depth, cur = [], 0, ''
    for ch in s:
        if ch in '([':
            depth += 1
        elif ch in ')]':
            depth -= 1
        if ch == sep and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return out


def octval(digits):
    n = int(digits, 8)
    if len(digits) == 20 and n >= 1 << 59:       # 60-bit ones-complement pattern
        n -= (1 << 60) - 1
    return n


def alfaconst(s):
    s = (s + ' ' * 10)[:10]
    v = 0
    for ch in s:
        v = v * 64 + dcode(ch)
    if v >= 1 << 59:
        v -= (1 << 60) - 1
    return str(v)


def conv_item(tok, kind):
    tok = tok.strip()
    m = re.fullmatch(r'\x00(\d+)\x00', tok)
    if m:
        s = strval(m.group(1))
        if kind == 'alfa':
            return alfaconst(s)
        assert len(s) == 1, s
        return 'D%02d' % dcode(s)
    return octal_or_dec(tok)


init_lines = []
for stmt in split_top(value_text, ';'):
    stmt = stmt.strip()
    if not stmt:
        continue
    name, rhs = [x.strip() for x in stmt.split('=', 1)]
    low, kind = VALTYPES[name]
    # strip comment placeholders
    if rhs.startswith('('):
        rhs = rhs[1:rhs.rindex(')')]
        items = []
        for it in split_top(rhs):
            it = it.strip()
            m = re.fullmatch(r'(\d+)\*(.*)', it, re.S)
            if m:
                items += [conv_item(m.group(2), kind)] * int(m.group(1))
            else:
                items.append(conv_item(it, kind))
        for k, v in enumerate(items):
            if isinstance(low, str):
                idx = 'D%02d' % (int(low[1:]) + k)
            else:
                idx = str(low + k)
            init_lines.append('   %s[%s] := %s;' % (name, idx, v))
    else:
        init_lines.append('   %s := %s;' % (name, conv_item(rhs, kind)))

# ---- 3. octal literals -------------------------------------------------
code = re.sub(r'(?<![A-Za-z0-9_\x00])([0-7]+)B(?![A-Za-z0-9_])',
              lambda m: str(octval(m.group(1))), code)

# ---- 4. simple dialect rewrites ----------------------------------------
code = code.replace('(*$', '(* $')
code = code.replace('] OR [', '] + [')
code = re.sub(r'\bGOTO\s+EXIT\s+(\d+)', r'GOTOX \1', code)
code = re.sub(r'\bPACKED\b\s*', '', code)
code = re.sub(r'\bPOWERSET\s+CHAR', 'SET OF TCHAR', code)
code = re.sub(r'\bPOWERSET\s+(\d+\.\.\d+)', 'SET OF BYTE', code)
code = re.sub(r'(\bSHRTINT|\bADDRESS|\bBITRANGE|\bRG3)\s*=\s*[-+0-9B .]+;', r'\1 = INT64;', code)
code = re.sub(r'\[OUT\]', '', code)
code = re.sub(r'\bCHAR\b', 'TCHAR', code)
code = re.sub(r'\bINTEGER\b', 'INT64', code)
code = re.sub(r'\bINPUT\b', 'CINPUT', code)
code = re.sub(r'\bOUTPUT\b', 'COUTPUT', code)
code = re.sub(r'\bPUT\b', 'CPUT', code)
code = re.sub(r'\bGET\b', 'CGET', code)
code = re.sub(r'\bRESET\b', 'CRESET', code)
code = re.sub(r'\bPACK\b', 'CPACK', code)
code = re.sub(r'\bUNPACK\b', 'CUNPACK', code)
code = re.sub(r'\bINT\b\s*\(', 'CINT(', code)
code = re.sub(r'\bCHR\s*\(', 'CCHR(', code)
code = re.sub(r'\bODD\b', 'CODD', code)
code = re.sub(r'\bEXIT\b', 'CEXIT', code)

# ---- sticky VAR/CONST parameter specifiers ------------------------------
def fix_header(m):
    head, params = m.group(1), m.group(2)
    groups = [g for g in split_top(params, ';')]
    var = False
    out = []
    for g in groups:
        g = g.strip()
        mm = re.match(r'(VAR|CONST)\b\s*(.*)', g, re.S)
        if mm:
            var = mm.group(1) == 'VAR'
            g = mm.group(2)
        out.append(('VAR ' if var else '') + g)
    return head + '(' + '; '.join(out) + ')'

code = re.sub(r'(\b(?:PROCEDURE|FUNCTION)\s+\w+\s*)\(([^()]*)\)', fix_header, code)

# empty variant parts
code = re.sub(r'(\w+)\s*:\s*;', r'\1: ( );', code)
# local CONST: commas -> semicolons
def fixconst(m):
    return re.sub(r',(\s*(?:\x00\d+\x00\s*)*[A-Z0-9]+\s*=)', r';\1', m.group(0), flags=re.S)
code = re.sub(r'\bCONST\b(?!\s*\w+\s*:)(.*?)(?=\b(?:VAR|LABEL|PROCEDURE|FUNCTION|BEGIN|TYPE)\b)',
              fixconst, code, flags=re.S)
code = re.sub(r'\bCONST\b\s*\x00', 'CONST \x00', code)

# ---- 5. WRITE statements -----------------------------------------------
CHARARGS = {'C'}
ALFAARGS = re.compile(r'\b(BLANK|ALF|HNAME|NAME)\b')


def write_stmt(args_txt, nl=False):
    out = []
    for a in split_top(args_txt):
        a = a.strip()
        w = '0'
        octal = False
        # top-level colon
        depth = 0
        cpos = None
        for i, ch in enumerate(a):
            if ch in '([':
                depth += 1
            elif ch in ')]':
                depth -= 1
            elif ch == ':' and depth == 0:
                cpos = i
                break
        expr = a
        if cpos is not None:
            expr, w = a[:cpos].strip(), a[cpos + 1:].strip()
            if re.search(r'\bOCT\s*$', w):
                octal = True
                w = re.sub(r'\bOCT\s*$', '', w).strip()
        m = re.fullmatch(r'\x00(\d+)\x00', expr)
        if m:
            s = strval(m.group(1))
            if len(s) == 1:
                out.append('WRC(D%02d,%s)' % (dcode(s), w))
            else:
                out.append('WRPAD(%s-%d)' % (w, len(s)))
                for ch in s:
                    out.append('WRC(D%02d,0)' % dcode(ch))
        elif octal:
            out.append('WRO(%s,%s)' % (expr, w))
        elif expr == 'EOL':
            out.append('WRC(D00,%s)' % w)
        elif expr in CHARARGS:
            out.append('WRC(%s,%s)' % (expr, w))
        elif ALFAARGS.search(expr):
            out.append('WRA(%s,%s)' % (expr, w))
        else:
            out.append('WRI(%s,%s)' % (expr, w))
    return 'BEGIN ' + '; '.join(out) + ' END'


def sub_writes(txt):
    res = ''
    i = 0
    pat = re.compile(r'\bWRITE\s*\(')
    while True:
        m = pat.search(txt, i)
        if not m:
            return res + txt[i:]
        # make sure it is not a procedure declaration "PROCEDURE WRITEIR"
        j = m.end()
        depth = 1
        while depth:
            if txt[j] == '(':
                depth += 1
            elif txt[j] == ')':
                depth -= 1
            j += 1
        args = txt[m.end():j - 1]
        res += txt[i:m.start()] + write_stmt(args)
        i = j


code = sub_writes(code)

# ---- 6. remaining single/multi character literals -----------------------
def lit(m):
    k, t = store[int(m.group(1))]
    if k == 'comment':
        return t
    s = t[1:-1].replace("''", "'")
    if len(s) == 1:
        return 'D%02d' % dcode(s)
    return alfaconst(s)


# comment placeholders are restored later; only translate strings now
def lit_str(m):
    k, t = store[int(m.group(1))]
    if k == 'comment':
        return m.group(0)
    return lit(m)


code = re.sub(r'\x00(\d+)\x00', lit_str, code)
# restore comments
code = re.sub(r'\x00(\d+)\x00', lambda m: store[int(m.group(1))][1], code)

code = code.replace('A : ARRAY [1..ALFALENG] OF TCHAR', 'A : CWORD')

# ---- 7. class types -> ordinary record types + arrays ------------------
m = re.search(r'CONTEXTTABLE\s*:\s*CLASS\s+450\s+OF\s*\n\s*RECORD', code)
a = m.start()
e = code.index('\n   END;', a) + len('\n   END;')
blk = '   CONTEXTTABLE =\n   RECORD' + code[m.end():e] + '\n'
code = code[:a] + code[e:]
v = code.index('\nVAR')
code = code[:v] + '\n' + blk + code[v:]

# LCSLABS (class of 30, local to CASESTAT) -> hoisted pool
m = re.search(r'TYPE PTR = \^LCSLABS ;\s*\nVAR LCSLABS : CLASS\s+30\s+OF\s+RECORD(.*?)END ;\s*\n', code, re.S)
lcs_rec = m.group(1)
code = code[:m.start()] + 'VAR\n' + code[m.end():]
code = code.replace('\nVAR', '\n   PTR = ^LCSLABS;\n   LCSLABS = RECORD' + lcs_rec + 'END;\n\nVAR', 1)

# ---- 8. file declarations ----------------------------------------------
m = re.search(r'PASCLGO\s*:\s*FILE OF INT64;', code)
code = code[:m.start()] + 'PASCLGO : PINT64;' + code[m.end():]
m = re.search(r'PMDFILE\s*:\s*FILE\s+1\s+OF\s*\n\s*RECORD(.*?)\n\s*END;', code, re.S)
pmd_rec = m.group(1)
code = code[:m.start()] + 'PMDFILE : PPMDREC;' + code[m.end():]
code = code.replace('\nVAR', '\n   PMDREC =\n   RECORD' + pmd_rec + '\n   END;\n   PPMDREC = ^PMDREC;\n\nVAR', 1)

# ---- 9. label declarations ---------------------------------------------
tok = re.compile(r'\(\*.*?\*\)|[A-Za-z_][A-Za-z0-9_]*|\d+|:=|<>|<=|>=|\.\.|.', re.S)
toks = [(m.group(0), m.start(), m.end()) for m in tok.finditer(code)]
procs, pstack, bdepth, state, hp = [], [], [], [], []
for k, (t, a_, b_) in enumerate(toks):
    if t.startswith('(*'):
        continue
    T = t.upper()
    if T in ('PROCEDURE', 'FUNCTION') and not (k > 0 and toks[k - 1][0].upper() == 'FORWARD'):
        p = dict(start=k, hdr_end=None, labels=set(), declared=set())
        procs.append(p)
        pstack.append(p); bdepth.append(0); state.append('hdr'); hp.append(0)
        continue
    if not pstack:
        continue
    st = state[-1]
    if st == 'hdr' and T == '(':
        hp[-1] += 1
    if st == 'hdr' and T == ')':
        hp[-1] -= 1
    if st == 'hdr' and T == ';' and hp[-1] == 0:
        pstack[-1]['hdr_end'] = b_
        nxt = next((x[0].upper() for x in toks[k + 1:k + 20] if not x[0].startswith('(*')), '')
        if nxt == 'FORWARD':
            pstack.pop(); bdepth.pop(); state.pop(); hp.pop()
        else:
            state[-1] = 'decl'
        continue
    if st in ('hdr', 'decl'):
        if T == 'LABEL':
            j = k + 1
            while toks[j][0] != ';':
                if toks[j][0].isdigit():
                    pstack[-1]['declared'].add(toks[j][0])
                j += 1
        if T == 'BEGIN':
            state[-1] = 'body'; bdepth[-1] = 1
        continue
    if T == 'GOTO' and pstack:
        j = k + 1
        while toks[j][0].startswith('(*') or toks[j][0].isspace():
            j += 1
        if toks[j][0].isdigit():
            pstack[-1]['labels'].add(toks[j][0])
    if T in ('BEGIN', 'CASE', 'RECORD'):
        bdepth[-1] += 1
    elif T == 'END':
        bdepth[-1] -= 1
        if bdepth[-1] == 0:
            pstack.pop(); bdepth.pop(); state.pop(); hp.pop()
ins = []
for p in procs:
    missing = p['labels'] - p['declared']
    if missing and p['hdr_end']:
        ins.append((p['hdr_end'], '\nLABEL ' + ','.join(sorted(missing, key=int)) + ';'))
for pos_, txt in sorted(ins, reverse=True):
    code = code[:pos_] + txt + code[pos_:]

code = code.replace('GOTOX ', 'GOTO ')

# ---- 10. CASESTAT class pool bookkeeping --------------------------------
code = code.replace('BEGIN (*CASESTAT*)\n', 'BEGIN (*CASESTAT*)\n  LCSSAVE := LCSTOP; LCSLSAVE := LCSLIM; LCSLIM := LCSTOP + 30;\n', 1)
code = code.replace('  ERR := ERR OR LERR ;\nEND (*CASESTAT*) ;',
                    '  ERR := ERR OR LERR ;\n  LCSTOP := LCSSAVE; LCSLIM := LCSLSAVE;\nEND (*CASESTAT*) ;', 1)
code = code.replace('LCA, LCP, LCA1, LCP1, LCA2, LCP2, LMIN, LMAX : SHRTINT ;',
                    'LCA, LCP, LCA1, LCP1, LCA2, LCP2, LMIN, LMAX : SHRTINT ;\n    LCSSAVE, LCSLSAVE : INT64 ;', 1)

# ---- 11. runtime + init + main ----------------------------------------
rt = open(os.path.join(HERE, 'cdcrt.inc'), encoding='utf-8').read()
rt = rt.replace('PARMLIST.ICO := 64', 'PARMLIST.ICO := %d' % int(os.environ.get('ICO', '2048')))
types_prelude = 'TYPE\n   TCHAR = (' + ','.join('D%02d' % i for i in range(64)) + ');\n   ALFA = INT64;\n   PINT64 = ^INT64;\n   PTCHAR = ^TCHAR;\n\n'
header = ('{$mode iso}\n{$R-}{$Q-}\nprogram pascal6000(input,output);\n\n' + types_prelude + 'const\n   EOL = D00;\n   ALFALENG = 10;\n\n')

# first CONST section of the source follows the prelude; make our const part of it
code = header + code

# runtime goes just before the first PROCEDURE
first_proc = code.index('\nPROCEDURE PRTERR')
init = 'PROCEDURE INITVALUES;\nBEGIN\n' + '\n'.join(init_lines) + '\nEND;\n\n'
code = code[:first_proc] + '\n' + rt + '\n' + init + code[first_proc:]

# main program: initialise runtime and values first
main_marker = 'BEGIN\n\n   (* INITIALIZE CONTEXT-TABLE *)'
assert main_marker in code
code = code.replace(main_marker, 'BEGIN\n   INITRT; INITVALUES;\n\n   (* INITIALIZE CONTEXT-TABLE *)', 1)
code = code.rstrip()
assert code.endswith('END .')
code = code[:-len('END .')] + '   FINIRT\nEND.\n'

open(OUT, 'w', encoding='utf-8').write(code)
print('wrote', OUT, len(code.split('\n')), 'lines')
