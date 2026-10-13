# derive a p2c-friendly file from pascal.pas (faithful transcription stays untouched)
import re,sys
s=open('pascal.pas').read()
s=re.sub(r"(?<![A-Za-z0-9_'])([0-7]+)B(?![A-Za-z0-9_])",lambda m:str(int(m.group(1),8)),s)
s=s.replace("(*$","(* $")
s=re.sub(r"(\w+): ;",r"\1: ( );",s)                    # empty variants
s=re.sub(r"POWERSET\s+(\d+\.\.\d+)",r"SET OF \1",s)
s=re.sub(r"\[OUT\]","",s)
s=re.sub(r"FILE\s+1\s+OF","FILE OF",s)
s=re.sub(r"CLASS\s+\d+\s+OF","",s)
open('pascal_p2c.pas','w').write(s)

i=s.index("CONST"); j=s.index("\nTYPE")
blk=s[i:j]
blk=re.sub(r",(\s*(?:\(\*.*?\*\)\s*)*[A-Z0-9]+ =)",r";\1",blk,flags=re.S)
s=s[:i]+blk+s[j:]
open('pascal_p2c.pas','w').write(s)

s=open("pascal_p2c.pas").read()
s=re.sub(r"(\w+) +: *;",r"\1 : ( );",s)
s=s.replace("POWERSET CHAR","SET OF CHAR")
a=s.index("   VALUE\n"); b=s.index("   (*TWO STANDARD PROCEDURES")
s=s[:a]+"   (* VALUE section (CDC extension) omitted for p2c *)\n\n"+s[b:]
open("pascal_p2c.pas","w").write(s)
s=open("pascal_p2c.pas").read()
s=s.replace("] OR [","] + [")
s=s.replace("'↗'","'^'").replace("'↓'","'~'").replace("'¬'","'!'")
open("pascal_p2c.pas","w").write(s)

# ---- second stage: dialect fixes that need a little parsing ----
s=open("pascal_p2c.pas").read()
s=re.sub(r"([;(]\s*)CONST (\w+\s*:)",r"\1\2",s)
s=re.sub(r"\bEOL\b","CHR(0)",s)
tok=re.compile(r"\(\*.*?\*\)|'(?:[^']|'')*'|[A-Za-z_][A-Za-z0-9_]*|\d+|:=|<>|<=|>=|\.\.|.",re.S)
toks=[(m.group(0),m.start(),m.end()) for m in tok.finditer(s)]
# find procedures: header start index, body begin, body end (matching END)
procs=[]  # dict(start_tok, hdr_end_char, begin_tok, end_tok, parent)
stack=[]
depth_stack=[]
i=0; n=len(toks)
def kw(t): return t.upper() if t[0].isalpha() else t
cur=None
# first pass: build proc tree via nesting of BEGIN/END within bodies
pstack=[]   # active procs
bdepth=[]   # block depth per proc body
hp=[]
state=[]    # per proc: 'hdr','decl','body'
for k,(t,a,b) in enumerate(toks):
    if t.startswith("(*"): continue
    T=t.upper()
    if T in("PROCEDURE","FUNCTION") and not (k>0 and toks[k-1][0].upper()=="FORWARD"):
        p=dict(start=k,hdr_end=None,labels=set(),defs=set(),parent=pstack[-1] if pstack else None)
        procs.append(p); pstack.append(p); bdepth.append(0); state.append('hdr'); hp.append(0); continue
    if not pstack: continue
    st=state[-1]
    if st=='hdr' and T=='(': hp[-1]+=1
    if st=='hdr' and T==')': hp[-1]-=1
    if st=='hdr' and T==';' and hp[-1]==0:
        pstack[-1]['hdr_end']=b
        # forward decl?
        nxt=next((x[0].upper() for x in toks[k+1:k+20] if not x[0].startswith("(*")),"")
        if nxt=="FORWARD": pstack.pop(); bdepth.pop(); state.pop(); hp.pop()
        else: state[-1]='decl'
        continue
    if st=='hdr' or st=='decl':
        if T=="BEGIN": state[-1]='body'; bdepth[-1]=1
        elif T in("CASE","RECORD"): pass
        continue
    # body
    if T in("BEGIN","CASE","RECORD"): bdepth[-1]+=1
    elif T=="END":
        bdepth[-1]-=1
        if bdepth[-1]==0: pstack.pop(); bdepth.pop(); state.pop(); hp.pop()
    # label detection in own body
    if T=="GOTO" and pstack:
        p=pstack[-1]
        j=k+1
        while toks[j][0].startswith("(*") or toks[j][0].isspace(): j+=1
        if toks[j][0].isdigit(): p['labels'].add(toks[j][0])
# also labels for main program body (outside procs) ignored.
ins=[]
for p in procs:
    if p['labels'] and p['hdr_end']: ins.append((p['hdr_end'],"\nLABEL "+",".join(sorted(p['labels'],key=int))+";"))
for pos,txt in sorted(ins,reverse=True): s=s[:pos]+txt+s[pos:]
open("pascal_p2c.pas","w").write(s)

# ---- class types -> ordinary record types ----
s=open("pascal_p2c.pas").read()
s=s.replace("MAXLABS = 20;","ALFALENG = 10; MAXLABS = 20;",1)
m=re.search(r"CONTEXTTABLE : *\n *PACKED RECORD",s)
a=m.start(); e=s.index("\n   END;",a)+len("\n   END;")
blk="   CONTEXTTABLE =\n   PACKED RECORD"+s[m.end():e]+"\n"
s=s[:a]+s[e:]
v=s.index("\nVAR")
s=s[:v]+"\n"+blk+s[v:]
m=re.search(r"VAR LCSLABS : *PACKED RECORD",s)
a=m.start(); e=s.index("END ;",a)+len("END ;")
s=s[:a]+"LCSLABS = PACKED RECORD"+s[m.end():e]+"\nVAR"+s[e:]
open("pascal_p2c.pas","w").write(s)

# ---- INT() coercion ----
s=open("pascal_p2c.pas").read()
s=re.sub(r"\bINT\((CH|'0')\)",r"ORD(\1)",s)
s=re.sub(r"\bINT\(",r"INTBITS(",s)
s=s.replace("\nPROCEDURE PRTERR","\nFUNCTION INTBITS(A : ALFA) : INTEGER; EXTERNAL;\nPROCEDURE APPEND(VAR A : INTEGER; B,C : INTEGER); EXTERNAL;\nPROCEDURE INSERT(A,B : INTEGER; VAR C : INTEGER); EXTERNAL;\n\nPROCEDURE PRTERR",1)
s=re.sub(r"GOTO EXIT (\d+)",r"GOTO \1",s)
open("pascal_p2c.pas","w").write(s)

# local CONST sections: commas -> semicolons
s=open("pascal_p2c.pas").read()
def fixconst(m):
    return re.sub(r",(\s*(?:\(\*.*?\*\)\s*)*[A-Z0-9]+ *=)",r";\1",m.group(0),flags=re.S)
s=re.sub(r"\bCONST\b(?! *\w+ *:)(.*?)(?=\b(?:VAR|LABEL|PROCEDURE|FUNCTION|BEGIN|TYPE)\b)",fixconst,s,flags=re.S)
open("pascal_p2c.pas","w").write(s)
