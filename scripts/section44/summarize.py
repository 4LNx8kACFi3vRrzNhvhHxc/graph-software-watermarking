dg = [tuple(map(int, l.split())) for l in open('dgs8_rooted.out')]
names = ["dijkstra_check (repository)", "dijkstra_check_labeled (Algorithm 3)",
         "procedure of [BENTO:2019]", "procedure of [BENTO:2019], one contraction per vertex"]
print(f"Dijkstra graphs <= 8 vertices (with entry): {len(dg)}")
for i, name in enumerate(names):
    print(f"  {name}: rejected {sum(x[i] == 0 for x in dg)}, non-terminating {sum(x[i] == 9 for x in dg)}")
r = [tuple(map(int, l.split())) for l in open('rand60k.out')]
d = [int(l) for l in open('rand60k_isdg.txt')]
a = [int(l) for l in open('rand60k_anyentry.txt')]
print(f"random flow graphs: {len(r)}; Dijkstra graphs with entry 0: {sum(d)}; Dijkstra graphs for some entry: {sum(a)}")
for i, name in enumerate(names):
    acc = sum(x[i] == 1 for x in r)
    no_entry = sum(1 for x, y, z in zip(r, d, a) if x[i] == 1 and not z)
    other_entry = sum(1 for x, y, z in zip(r, d, a) if x[i] == 1 and z and not y)
    mis = sum(1 for x, y in zip(r, d) if y and x[i] != 1)
    nt = sum(x[i] == 9 for x in r)
    print(f"  {name}: accepted {acc}; not Dijkstra for any entry {no_entry}; Dijkstra only for another entry {other_entry}; "
          f"Dijkstra with entry 0 but rejected {mis}; non-terminating {nt}")
