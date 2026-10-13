#!/usr/bin/env python3
"""Tiny CDC 6000 disassembler for the code the Pascal compiler emits.
usage: cdcdis.py file-of-20-octal-digit-words [first-address-octal]"""
import sys
NAMES = {0:'PS',1:'RJ',2:'JP',4:'EQ',5:'NE',6:'GE',7:'LT',
 0o10:'BX{i} X{j}',0o11:'BX{i} X{j}*X{k}',0o12:'BX{i} X{j}+X{k}',0o13:'BX{i} X{j}-X{k}',
 0o14:'BX{i} -X{k}',0o15:'BX{i} -X{k}*X{j}',0o16:'BX{i} -X{k}+X{j}',0o17:'BX{i} -X{k}-X{j}',
 0o20:'LX{i} {jk}',0o21:'AX{i} {jk}',0o22:'LX{i} B{j},X{k}',0o23:'AX{i} B{j},X{k}',
 0o24:'NX{i} B{j},X{k}',0o25:'ZX{i} B{j},X{k}',0o26:'UX{i} B{j},X{k}',0o27:'PX{i} B{j},X{k}',
 0o30:'FX{i} X{j}+X{k}',0o31:'FX{i} X{j}-X{k}',0o32:'DX{i} X{j}+X{k}',0o33:'DX{i} X{j}-X{k}',
 0o34:'RX{i} X{j}+X{k}',0o35:'RX{i} X{j}-X{k}',0o36:'IX{i} X{j}+X{k}',0o37:'IX{i} X{j}-X{k}',
 0o40:'FX{i} X{j}*X{k}',0o41:'RX{i} X{j}*X{k}',0o42:'DX{i} X{j}*X{k}',0o43:'MX{i} {jk}',
 0o44:'FX{i} X{j}/X{k}',0o45:'RX{i} X{j}/X{k}',0o46:'NO',0o47:'CX{i} X{k}',
 0o50:'SA{i} A{j}+K',0o51:'SA{i} B{j}+K',0o52:'SA{i} X{j}+K',0o53:'SA{i} X{j}+B{k}',
 0o54:'SA{i} A{j}+B{k}',0o55:'SA{i} A{j}-B{k}',0o56:'SA{i} B{j}+B{k}',0o57:'SA{i} B{j}-B{k}',
 0o60:'SB{i} A{j}+K',0o61:'SB{i} B{j}+K',0o62:'SB{i} X{j}+K',0o63:'SB{i} X{j}+B{k}',
 0o64:'SB{i} A{j}+B{k}',0o65:'SB{i} A{j}-B{k}',0o66:'SB{i} B{j}+B{k}',0o67:'SB{i} B{j}-B{k}',
 0o70:'SX{i} A{j}+K',0o71:'SX{i} B{j}+K',0o72:'SX{i} X{j}+K',0o73:'SX{i} X{j}+B{k}',
 0o74:'SX{i} A{j}+B{k}',0o75:'SX{i} A{j}-B{k}',0o76:'SX{i} B{j}+B{k}',0o77:'SX{i} B{j}-B{k}'}
COND = ['ZR','NZ','PL','NG','IR','OR','DF','ID']
LONG = {0o01,0o02,0o03,0o04,0o05,0o06,0o07,0o50,0o51,0o52,0o60,0o61,0o62,0o70,0o71,0o72}
def dis(w, addr):
    out=[]; p=[(w>>45)&0x7fff,(w>>30)&0x7fff,(w>>15)&0x7fff,w&0x7fff]
    n=0
    while n<4:
        x=p[n]; f=(x>>9)&0o77; i=(x>>6)&7; j=(x>>3)&7; k=x&7
        if f in LONG:
            if n==3: out.append((n,'<30-bit split>')); n+=1; continue
            K=(k<<15)|p[n+1]
            if f==3: s='%s X%d,%o'%(COND[i],j,K)
            elif f==1: s='RJ %o'%K
            elif f==2: s='JP B%d+%o'%(i,K)
            elif f in (4,5,6,7): s='%s B%d,B%d,%o'%(NAMES[f],i,j,K)
            else: s=NAMES[f].format(i=i,j=j,k=k,jk=j*8+k).replace('K','%o'%K)
            out.append((n,s)); n+=2
        else:
            s=NAMES.get(f,'?%o'%f).format(i=i,j=j,k=k,jk=j*8+k)
            out.append((n,s)); n+=1
    return out
if __name__=='__main__':
    a=int(sys.argv[2],8) if len(sys.argv)>2 else 0
    for line in open(sys.argv[1]):
        line=line.strip()
        if not line: continue
        w=int(line,8)
        print('%06o %s  %s'%(a,line,' ; '.join(s for n,s in dis(w,a))))
        a+=1
