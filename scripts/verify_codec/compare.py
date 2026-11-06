import sys; sys.path.insert(0, ".")
import reference as sim
from itertools import product
fn=sys.argv[1]; NMAX=int(sys.argv[2])
cgraphs={}; cdec={}
cur=None
for line in open(fn):
    p=line.split()
    if p[0]=='G':
        idn,N=int(p[1]),int(p[2]); E=frozenset(tuple(map(int,x.split(','))) for x in p[3:])
        cur=(idn,E); cgraphs.setdefault(idn,set()).add(E); cdec[cur]={}
    else:
        key=p[1]; out=p[2] if len(p)>2 else ''
        cdec[cur][key]=out
mism_graph=0; mism_dec=0; checked=0; worst=0; fwdbad=0
for n in range(1,NMAX+1):
    for bits in product([0,1],repeat=n-1):
        B=[1]+list(bits); idn=int(''.join(map(str,B)),2)
        mine={}
        for g in sim.encode_all(B):
            E=frozenset(set(g.path)|set(g.fwd)|{(v,w) for v,w in g.back.items()}); mine[E]=g
        if set(mine)!=cgraphs.get(idn,set()): mism_graph+=1; continue
        for E,g in mine.items():
            d=cdec[(idn,E)]
            py=''.join(map(str,sim.improved_decode(g,n,True)[0]))
            if d['-']!=py: mism_dec+=1
            for k,e in sim.enc_edges(g):
                key=f"{e[0]},{e[1] if k=='F' else e[1]}"
                if k=='F': key=f"{e[0]},{e[1]}"
                else: key=f"{e[0]},{e[1]}"
                h=sim.remove(g,k,e); py=''.join(map(str,sim.improved_decode(h,n,True)[0]))
                checked+=1
                if d[key]!=py: mism_dec+=1
                c=[int(x) for x in d[key]]; dd=sim.delta(B,c); worst=max(worst,dd)
                if k=='F' and dd: fwdbad+=1
print(f"identifiers with different graph sets C vs Python: {mism_graph}")
print(f"decoder outputs compared: {checked} (+ undistorted), mismatches: {mism_dec}")
print(f"C decoder: worst Delta under single removal = {worst}, forward removals not exact = {fwdbad}")
