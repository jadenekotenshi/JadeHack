#!/usr/bin/env python3
# Generates dat/Pirate.des for JadeHack's Pirate quest.
import random, sys

W, H = 76, 18
random.seed(20261117)


class Grid:
    def __init__(self, fill):
        self.g = [[fill] * W for _ in range(H)]

    def set(self, x, y, c):
        if 0 <= x < W and 0 <= y < H:
            self.g[y][x] = c

    def get(self, x, y):
        if 0 <= x < W and 0 <= y < H:
            return self.g[y][x]
        return ' '

    def rect(self, x1, y1, x2, y2, c):
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                self.set(x, y, c)

    def box(self, x1, y1, x2, y2, floor='.'):
        """walled building"""
        for x in range(x1, x2 + 1):
            self.set(x, y1, '-')
            self.set(x, y2, '-')
        for y in range(y1, y2 + 1):
            self.set(x1, y, '|')
            self.set(x2, y, '|')
        self.rect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, floor)

    def cells(self, chars, x1=0, y1=0, x2=W - 1, y2=H - 1):
        out = []
        for y in range(y1, y2 + 1):
            for x in range(x1, x2 + 1):
                if self.g[y][x] in chars:
                    out.append((x, y))
        return out

    def text(self):
        return '\n'.join(''.join(r) for r in self.g)


def pc(x, y):
    return "(%02d,%02d)" % (x, y)


def pick(cells, n, used):
    out = []
    pool = [c for c in cells if c not in used]
    random.shuffle(pool)
    for c in pool[:n]:
        used.add(c)
        out.append(c)
    return out


MONGEN = 'MON_GENERATION:86%, (96,"skeletal pirate"), (24,\'r\'), (24,"soldier"), (6,\'E\')'
out = []
out.append('''#	Pirate.des
#	Pirate quest levels for JadeHack, after the Pirate quest of Chris's
#	patch (also in dNetHack): Tortuga, a flooded filler, the shoreline
#	of Shipwreck Island, a cavern, and the caldera where Blackbeard's
#	Ghost sits on the Treasury of Proteus.  Pirates swim, so water is
#	no obstacle to them.  Scimitars in this file turn into scimitars,
#	falchions, messers, sabers or silver sabers (see create_object()).
#''')

# ---------------------------------------------------------------- Tortuga
g = Grid('}')
# the island
g.rect(2, 1, 73, 16, '.')
for (x, y) in [(2, 1), (3, 1), (2, 2), (73, 1), (72, 1), (73, 2), (2, 16), (3, 16),
               (2, 15), (73, 16), (72, 16), (73, 15)]:
    g.set(x, y, '}')
# stone between the town and the fort
g.rect(35, 1, 49, 16, ' ')
# the cave in the middle
cave = set()
for y in range(2, 16):
    hw = 5.5 - abs(y - 8.5) * 0.45
    cx = 42
    for x in range(35, 50):
        d = abs(x - cx) + random.random() * 1.2
        if d <= hw + 0.6:
            cave.add((x, y))
for (x, y) in cave:
    g.set(x, y, '.')
# passages west (street) and east (gate)
g.rect(35, 7, 38, 8, '.')
g.rect(46, 7, 49, 8, '.')
# the town: homes in the north row
homes = [(4, 2, 10, 5), (12, 2, 18, 5), (20, 2, 26, 5)]
for (x1, y1, x2, y2) in homes:
    g.box(x1, y1, x2, y2)
    g.set((x1 + x2) // 2, y2, '+')
# the brothel
g.box(28, 2, 33, 5)
g.set(30, 5, '+')
# shops in the south row
shops = [("potion shop", 3, 9, 9, 14), ("weapon shop", 11, 9, 17, 14),
         ("weapon shop", 19, 9, 25, 14), ("tool shop", 27, 9, 33, 14)]
for (nm, x1, y1, x2, y2) in shops:
    g.box(x1, y1, x2, y2)
    g.set((x1 + x2) // 2, y1, '+')
# the fort
g.box(50, 2, 72, 15)
g.set(50, 8, '+')
g.rect(51, 3, 71, 14, '.')
# the keep with the stairs
g.box(57, 5, 65, 11)
g.set(57, 8, '+')
g.set(65, 8, '+')
strt = g

out.append('''LEVEL: "Pir-strt"
FLAGS: noteleport,hardfloor,mazelevel
INIT_MAP:solidfill,' '
%s
GEOMETRY:center,center
MAP
%s
ENDMAP''' % (MONGEN, strt.text()))
out.append('REGION:(00,00,75,17),lit,"ordinary"')
out.append('REGION:(36,02,48,15),unlit,"ordinary"')
for (nm, x1, y1, x2, y2) in shops:
    out.append('REGION:(%02d,%02d,%02d,%02d),lit,"%s"' % (x1 + 1, y1 + 1, x2 - 1, y2 - 1, nm))
out.append('REGION:(29,03,32,04),lit,"ordinary"')
out.append('STAIR:(61,08),down')
out.append('BRANCH:(05,03,25,04),(0,0,0,0)')
out.append('NON_DIGGABLE:(00,00,75,17)')
for (nm, x1, y1, x2, y2) in shops:
    out.append('DOOR:closed,(%02d,%02d)' % ((x1 + x2) // 2, y1))
out.append('DOOR:closed,(30,05)')
out.append('DOOR:closed,(50,08)')
out.append('DOOR:closed,(57,08)')
out.append('DOOR:closed,(65,08)')
used = set()
# the mayor and his guard in the caves
out.append('MONSTER:(\'@\',"Mayor Cummerbund"),(42,08)')
out.append('MONSTER:(\'@\',"pirate brother"),(40,09)')
out.append('MONSTER:(\'@\',"pirate brother"),(44,07)')
used |= {(42, 8), (40, 9), (44, 7)}
street = [c for c in strt.cells('.', 3, 6, 34, 8)]
town_open = street + strt.cells('.', 3, 1, 34, 8)
for (x, y) in pick(street, 6, used):
    out.append('MONSTER:(\'@\',"pirate brother"),%s' % pc(x, y))
for (x, y) in pick(street, 12, used):
    out.append('MONSTER:(\'Z\',"skeletal pirate"),%s' % pc(x, y))
for (x, y) in pick(street, 3, used):
    out.append('MONSTER:(\' \',"ghost"),%s' % pc(x, y))
out.append('MONSTER:(\'&\',"succubus"),(30,03)')
out.append('MONSTER:(\'&\',"incubus"),(31,04)')
# the army
keep = strt.cells('.', 58, 6, 64, 10)
yard = strt.cells('.', 51, 3, 71, 14)
yard = [c for c in yard if c not in keep and c not in used]
used.add((61, 8))
out.append('MONSTER:(\'@\',"sergeant"),%s' % pc(*pick([c for c in keep if c != (61, 8)], 1, used)[0]))
for (x, y) in pick([c for c in keep if c != (61, 8)], 3, used):
    out.append('MONSTER:(\'@\',"soldier"),%s' % pc(x, y))
for (x, y) in pick(yard, 6, used):
    out.append('MONSTER:(\'@\',"soldier"),%s' % pc(x, y))
out.append('LOOP [3 + 1d3] {\n  OBJECT:random,random\n}')
out.append('LOOP [2] {\n  TRAP:random,random\n}')
out.append('WALLIFY')

# ------------------------------------------------------- the flooded filler
def islands(n, seed_cells=None):
    gg = Grid('}')
    for _ in range(n):
        cx = random.randint(5, W - 6)
        cy = random.randint(3, H - 4)
        rx = random.randint(2, 6)
        ry = random.randint(1, 3)
        for y in range(cy - ry, cy + ry + 1):
            for x in range(cx - rx, cx + rx + 1):
                if ((x - cx) / (rx + 0.5)) ** 2 + ((y - cy) / (ry + 0.5)) ** 2 <= 1.0:
                    if 1 <= x < W - 1 and 1 <= y < H - 1:
                        gg.set(x, y, '.')
    return gg


fa = islands(16)
# make sure the up and down stairs sit on land, well apart
land = fa.cells('.')
left = [c for c in land if c[0] < 20]
right = [c for c in land if c[0] > 56]
while not left:
    fa.rect(3, 8, 8, 10, '.')
    land = fa.cells('.')
    left = [c for c in land if c[0] < 20]
while not right:
    fa.rect(66, 8, 71, 10, '.')
    land = fa.cells('.')
    right = [c for c in land if c[0] > 56]
up = random.choice(left)
dn = random.choice(right)
out.append('''
LEVEL: "Pir-fila"
FLAGS:mazelevel,noflip
INIT_MAP:solidfill,' '
%s
GEOMETRY:center,center
MAP
%s
ENDMAP
REGION:(00,00,75,17),lit,"ordinary"
STAIR:%s,up
STAIR:%s,down''' % (MONGEN, fa.text(), pc(*up), pc(*dn)))
for n, nm, ch in [(3, "skeletal pirate", 'Z'), (3, "soldier", '@'), (4, "parrot", 'B'),
                  (2, "monkey", 'Y')]:
    out.append('LOOP [%d] {\n  MONSTER:(\'%s\',"%s"),random\n}' % (n, ch, nm))
out.append('MONSTER:(\' \',"ghost"),random')
out.append('LOOP [4 + 1d4] {\n  OBJECT:random,random\n}')
out.append('LOOP [3] {\n  TRAP:random,random\n}')

# --------------------------------------------- the shoreline of the island
sh = Grid('}')
# west beach with the up stairs
for y in range(3, 15):
    for x in range(1, 14):
        d = ((x - 5) / 8.0) ** 2 + ((y - 9) / 6.5) ** 2
        if d <= 1.0:
            sh.set(x, y, '.')
# land around the wreck
for y in range(2, 16):
    for x in range(14, 52):
        d = ((x - 36) / 17.0) ** 2 + ((y - 8.5) / 7.2) ** 2
        if d <= 1.0:
            sh.set(x, y, '.')
sh.rect(12, 8, 22, 9, '.')      # land bridge from the beach
# the wrecked ship
sh.box(25, 5, 47, 12)
sh.set(25, 8, '+')              # the way aboard
sh.set(36, 5, '+'); sh.set(36, 12, '+')
for y in range(6, 12):          # bulkhead
    sh.set(36, y, '|')
sh.set(36, 8, '+')
sh.set(47, 8, '|')              # secret door in the stern
# the cavern on the far side
cav = set()
for y in range(2, 16):
    for x in range(52, 74):
        d = ((x - 62.5) / 11.0) ** 2 + ((y - 8.5) / 7.0) ** 2
        if d + random.random() * 0.35 <= 1.0:
            cav.add((x, y))
sh.rect(51, 1, 74, 16, '}')
for (x, y) in cav:
    sh.set(x, y, '.')
# solid rock round the cavern floor, and the passage from the stern
for (x, y) in list(cav):
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            if sh.get(x + dx, y + dy) == '}':
                sh.set(x + dx, y + dy, ' ')
sh.rect(48, 8, 52, 8, '.')
sh.rect(48, 7, 52, 7, ' ')
sh.rect(48, 9, 52, 9, ' ')
sh.set(48, 8, '.')
# the water east of the stern passage must be rock too
for x in range(48, 53):
    for y in (6, 10):
        if sh.get(x, y) == '}':
            sh.set(x, y, ' ')
for x in range(48, 53):
    for y in (7, 9):
        if sh.get(x, y) in '}':
            sh.set(x, y, ' ')

beach = sh.cells('.', 1, 3, 13, 14)
cavern = [c for c in sh.cells('.', 52, 2, 74, 16)]
used = set()
upc = (3, 9)
sh.set(*upc, c='.') if False else None
hull = sh.cells('.', 26, 6, 46, 11)
used |= {upc}
out.append('''
LEVEL: "Pir-loca"
FLAGS:mazelevel,noteleport
INIT_MAP:solidfill,' '
%s
GEOMETRY:center,center
MAP
%s
ENDMAP
REGION:(00,00,75,17),lit,"ordinary"
REGION:(52,01,74,16),unlit,"ordinary"
STAIR:%s,up''' % (MONGEN, sh.text(), pc(*upc)))
dnc = random.choice([c for c in cavern if c[0] > 58])
out.append('STAIR:%s,down' % pc(*dnc))
used.add(dnc)
out.append('DOOR:secret,(47,08)')
out.append('DOOR:closed,(25,08)')
out.append('DOOR:closed,(36,08)')
out.append('NON_DIGGABLE:(26,06,46,11)')
water = [c for c in sh.cells('}', 0, 0, 51, 17) if 2 <= c[1] <= 15]
for (x, y) in pick(water, 6, used):
    out.append('MONSTER:(\';\',"shark"),%s' % pc(x, y))
for (x, y) in pick([c for c in beach if c != upc], 8, used):
    out.append('MONSTER:(\'Z\',"skeletal pirate"),%s' % pc(x, y))
out.append('MONSTER:(\' \',"ghost"),%s' % pc(*pick(beach, 1, used)[0]))
for (x, y) in pick([c for c in beach], 5, used):
    out.append('OBJECT:random,%s' % pc(x, y))
# the crew of the Captain
cabin = [c for c in hull if c[0] != 36]
for (x, y) in pick(cabin, 9, used):
    out.append('MONSTER:(\'@\',"soldier"),%s' % pc(x, y))
out.append('MONSTER:(\'@\',"lieutenant"),%s' % pc(*pick([c for c in cabin if c[0] > 37], 1, used)[0]))
out.append('MONSTER:(\'@\',"captain"),%s,"Captain Ketch",hostile' % pc(*pick([c for c in cabin if c[0] > 37], 1, used)[0]))
for (x, y) in pick([c for c in cavern if c != dnc], 11, used):
    out.append('MONSTER:(\'Z\',"skeletal pirate"),%s' % pc(x, y))
for (x, y) in pick(cavern, 3, used):
    out.append('MONSTER:(\' \',"ghost"),%s' % pc(x, y))
for (x, y) in pick(cavern, 1, used):
    out.append('MONSTER:(\'&\',"damned pirate"),%s' % pc(x, y))
out.append('LOOP [9] {\n  OBJECT:(\')\',"scimitar"),random\n}')
out.append('LOOP [15] {\n  OBJECT:random,random\n}')
out.append('LOOP [4] {\n  TRAP:random,random\n}')
out.append('WALLIFY')

# ------------------------------------------------- the lower filler cavern
out.append('''
LEVEL: "Pir-filb"
FLAGS:mazelevel,noflip
INIT_MAP: mines, '.' , ' ', true, true, unlit, true
%s
NOMAP
STAIR: random, up
STAIR: random, down
LOOP [7] {
  OBJECT: random, random
}
LOOP [7] {
  TRAP: random, random
}
LOOP [11] {
  MONSTER: ('Z', "skeletal pirate"), random
}
LOOP [3] {
  MONSTER: (' ', "ghost"), random
}
MONSTER: ('&', "damned pirate"), random
MONSTER: ('@', "sergeant"), random, hostile''' % MONGEN)

# ------------------------------------------------------- the caldera (goal)
def make_caldera():
    while True:
        gg = Grid(' ')
        for y in range(1, H - 1):
            for x in range(1, W - 1):
                if random.random() < 0.46:
                    gg.set(x, y, '.')
        for _ in range(4):
            new = Grid(' ')
            for y in range(1, H - 1):
                for x in range(1, W - 1):
                    n = sum(1 for dx in (-1, 0, 1) for dy in (-1, 0, 1)
                            if (dx or dy) and gg.get(x + dx, y + dy) == '.')
                    if gg.get(x, y) == '.':
                        new.set(x, y, '.' if n >= 4 else ' ')
                    else:
                        new.set(x, y, '.' if n >= 5 else ' ')
            gg = new
        # the dais
        cx, cy = 38, 9
        for y in range(H):
            for x in range(W):
                if ((x - cx) / 7.0) ** 2 + ((y - cy) / 4.0) ** 2 <= 1.0:
                    gg.set(x, y, '.')
        # keep the component of the dais
        seen = {(cx, cy)}
        stack = [(cx, cy)]
        while stack:
            x, y = stack.pop()
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    p = (x + dx, y + dy)
                    if p not in seen and gg.get(*p) == '.':
                        seen.add(p)
                        stack.append(p)
        if len(seen) < 450:
            continue
        ng = Grid(' ')
        for (x, y) in seen:
            ng.set(x, y, '.')
        # keep a margin of rock round the edge of the map
        for x in range(W):
            ng.set(x, 0, ' ')
            ng.set(x, H - 1, ' ')
        for y in range(H):
            ng.set(0, y, ' ')
            ng.set(W - 1, y, ' ')
        # the way in has to be on the left
        westcells = [c for c in seen if c[0] <= 14]
        if not westcells:
            continue
        return ng, westcells


gc, west = make_caldera()
upg = min(west)
cen = (38, 9)
floor = gc.cells('.')
used = {upg, cen}
out.append('''
LEVEL: "Pir-goal"
FLAGS:mazelevel,noteleport
INIT_MAP:solidfill,' '
%s
GEOMETRY:center,center
MAP
%s
ENDMAP
REGION:(00,00,75,17),unlit,"ordinary"
STAIR:%s,up
NON_DIGGABLE:(00,00,75,17)''' % (MONGEN, gc.text(), pc(*upg)))
# Blackbeard sits on the loot
out.append('OBJECT:(\'(\',"chest"),%s,blessed,0,NAME:"The Treasury of Proteus"' % pc(*cen))
out.append('MONSTER:(\'@\',"Blackbeard\'s Ghost"),%s' % pc(*cen))
dais = [c for c in floor if ((c[0] - 38) / 7.0) ** 2 + ((c[1] - 9) / 4.0) ** 2 <= 0.8 and c != cen]
for (x, y) in pick(dais, 6, used):
    out.append('GOLD:%d,%s' % (random.randint(200, 900), pc(x, y)))
for (x, y) in pick(dais, 4, used):
    out.append("OBJECT:'*',%s" % pc(x, y))
for (x, y) in pick(dais, 5, used):
    out.append('OBJECT:(\'(\',"chest"),%s' % pc(x, y))
out.append('LOOP [5] {\n  OBJECT:(\')\',"scimitar"),random\n}')
out.append("LOOP [6] {\n  OBJECT:')',random\n}")
out.append("LOOP [6] {\n  OBJECT:'(',random\n}")
out.append('LOOP [4] {\n  TRAP:random,random\n}')
out.append('LOOP [15] {\n  MONSTER:(\'Z\',"human zombie"),random\n}')
out.append("LOOP [2] {\n  MONSTER:'Z',random\n}")
out.append('LOOP [8] {\n  MONSTER:(\'W\',"wraith"),random\n}')
out.append("MONSTER:'W',random")
out.append('LOOP [3] {\n  MONSTER:(\'&\',"damned pirate"),random\n}')
out.append('LOOP [9] {\n  MONSTER:(\'Z\',"skeletal pirate"),random\n}')

open(sys.argv[1], 'w').write('\n'.join(out) + '\n')
print("ok")
