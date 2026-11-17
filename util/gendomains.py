#!/usr/bin/env python3
# Generates dat/vecna.des and dat/caves.des: Vecna's Domain (three levels),
# the Sunless Sea, the Spider Caves and the Giant Caverns.
#
#   python3 util/gendomains.py dat
import random, sys

W, H = 76, 18
rng = random.Random(20261118)


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
        for x in range(x1, x2 + 1):
            self.set(x, y1, '-')
            self.set(x, y2, '-')
        for y in range(y1, y2 + 1):
            self.set(x1, y, '|')
            self.set(x2, y, '|')
        self.rect(x1 + 1, y1 + 1, x2 - 1, y2 - 1, floor)

    def cells(self, chars, x1=0, y1=0, x2=W - 1, y2=H - 1):
        return [(x, y) for y in range(y1, y2 + 1) for x in range(x1, x2 + 1)
                if self.g[y][x] in chars]

    def text(self):
        return '\n'.join(''.join(r) for r in self.g)

    def reach(self, start, ok):
        seen = {start}
        stack = [start]
        while stack:
            x, y = stack.pop()
            for dx in (-1, 0, 1):
                for dy in (-1, 0, 1):
                    p = (x + dx, y + dy)
                    if p not in seen and self.get(*p) in ok:
                        seen.add(p)
                        stack.append(p)
        return seen


def pc(x, y):
    return "(%02d,%02d)" % (x, y)


class Picker:
    """hand out distinct cells from a pool"""
    def __init__(self, used=None):
        self.used = set(used or [])

    def take(self, cells, n=1):
        pool = [c for c in cells if c not in self.used]
        rng.shuffle(pool)
        out = pool[:n]
        for c in out:
            self.used.add(c)
        return out

    def one(self, cells):
        return self.take(cells, 1)[0]


def header(name, flags, extra=''):
    return 'LEVEL: "%s"\nFLAGS: %s\nINIT_MAP:solidfill,\' \'\n%sGEOMETRY:center,center' % (name, flags, extra)


def mon(ch, name, cell, extra=''):
    return "MONSTER:('%s',\"%s\"),%s%s" % (ch, name, pc(*cell), extra)


# ======================================================== Vecna's Domain
vecna = []

# ---- level 1: the withered forest
def forest():
    while True:
        g = Grid('.')
        g.rect(0, 0, W - 1, 0, ' ')
        g.rect(0, H - 1, W - 1, H - 1, ' ')
        g.rect(0, 0, 0, H - 1, ' ')
        g.rect(W - 1, 0, W - 1, H - 1, ' ')
        yard = [(3, 2, 11, 5), (3, 12, 11, 15), (14, 7, 22, 10)]
        yards = yard + [(W - 1 - x2, y1, W - 1 - x1, y2) for (x1, y1, x2, y2) in yard]
        keep = set()
        for (x1, y1, x2, y2) in yards:
            for y in range(y1 - 1, y2 + 2):
                for x in range(x1 - 1, x2 + 2):
                    keep.add((x, y))
        for y in range(1, H - 1):
            for x in range(1, W - 1):
                if (x, y) in keep:
                    continue
                if abs(x - 37.5) < 4 and abs(y - 8.5) < 3:
                    continue
                if rng.random() < 0.16:
                    g.set(x, y, 'T')
        stairs = [(2, 8), (73, 8), (37, 1), (37, 16)]
        for (x, y) in stairs:
            g.set(x, y, '.')
        reach = g.reach((37, 8), '.')
        if all(s in reach for s in stairs) and \
                all((x, y) in reach for (x1, y1, x2, y2) in yards for (x, y) in [(x1, y1)]):
            return g, yards, stairs

g, yards, stairs = forest()
pk = Picker(stairs + [(37, 8), (38, 8), (37, 9), (38, 9)])
vecna.append(header("vecna1", "noteleport,hardfloor,mazelevel", '') + '\nMAP\n%s\nENDMAP' % g.text())
vecna.append('MESSAGE: "You have entered a desolate landscape, completely devoid of life."')
vecna.append('REGION:(00,00,75,17),unlit,"ordinary"')
vecna.append('NON_DIGGABLE:(00,00,75,17)')
vecna.append('BRANCH:(36,08,39,09),(0,0,0,0)')
vecna.append('$place = { %s }\nSHUFFLE:$place' % ','.join(pc(*s) for s in stairs))
vecna.append('STAIR:$place[0],down')
for i in (1, 2, 3):
    vecna.append('MONSTER:(\'m\',"giant mimic"),$place[%d],m_feature "staircase down"' % i)
floor = g.cells('.')
for (x1, y1, x2, y2) in yards:
    for c in pk.take([(x, y) for (x, y) in floor if x1 <= x <= x2 and y1 <= y <= y2], 7):
        vecna.append('GRAVE:%s,random' % pc(*c))
for nm in ("lich", "lich", "demilich", "demilich"):
    vecna.append(mon('L', nm, pk.one(floor)))
for ch, n in (('V', 6), ('Z', 8), ('M', 8)):
    for c in pk.take(floor, n):
        vecna.append("MONSTER:'%s',%s" % (ch, pc(*c)))
for c in pk.take(floor, 4):
    vecna.append('MONSTER:(\' \',"ghost"),%s' % pc(*c))
vecna.append('LOOP [10] {\n  TRAP:random,random\n}')

# ---- level 2: the lava stronghold
g = Grid('L')
# the west landing and the two cells
g.rect(1, 6, 9, 12, '.')
g.box(2, 2, 6, 5)          # north cell
g.set(4, 5, 'F')
g.box(2, 13, 6, 16)         # south cell
g.set(4, 13, 'F')
# the stronghold
g.box(10, 1, 50, 16)
g.set(10, 9, '+')           # the gate
# internal walls: the north and south sections
g.box(31, 2, 41, 7)
g.set(36, 7, '+')
g.box(31, 10, 41, 15)
g.set(36, 10, '+')
# the drawbridge and the causeway
g.set(50, 9, '|')           # the portcullis wall (drawbridge raised)
g.set(51, 9, 'L')           # the span
g.rect(52, 8, 53, 10, '.')
g.rect(54, 8, 57, 9, '.')
g.box(54, 4, 57, 7)         # the cell that guards the bridge
g.set(55, 7, 'F')
# Kas's chamber
g.box(58, 3, 73, 15)
g.set(58, 9, '+')
g.rect(59, 4, 72, 14, '.')
g2 = g
sec_n = [(x, y) for (x, y) in g2.cells('.', 32, 3, 40, 6)]
sec_s = [(x, y) for (x, y) in g2.cells('.', 32, 11, 40, 14)]
court = g2.cells('.', 11, 2, 30, 15)
kas = g2.cells('.', 59, 4, 72, 14)
lavaside = [(x, y) for (x, y) in g2.cells('L', 51, 5, 56, 14) if (x, y) != (51, 9)]
pk = Picker([(3, 9), (72, 9)])
vecna.append(header("vecna2", "noteleport,hardfloor,mazelevel") + '\nMAP\n%s\nENDMAP' % g2.text())
vecna.append('REGION:(00,00,75,17),unlit,"ordinary"')
vecna.append('REGION:(59,04,72,14),lit,"ordinary"')
vecna.append('NON_DIGGABLE:(00,00,75,17)')
vecna.append('STAIR:(03,09),up')
vecna.append('STAIR:(72,09),down')
vecna.append('DRAWBRIDGE:(51,09),west,closed')
for (cx, cy, arrows) in ((4, 3, 50), (4, 15, 50), (55, 5, 100)):
    vecna.append("MONSTER:('&',\"marilith\"),%s {\n  OBJECT:(')',\"orcish bow\")\n  OBJECT:(')',\"orcish arrow\"),quantity:%d\n}" % (pc(cx, cy), arrows))
for c in pk.take(court, 6):
    vecna.append("MONSTER:'V',%s" % pc(*c))
for c in pk.take(court, 4):
    vecna.append("MONSTER:'Z',%s" % pc(*c))
for c in pk.take(court, 7):
    vecna.append("MONSTER:'M',%s" % pc(*c))
for c in pk.take(court, 4):
    vecna.append('MONSTER:(\' \',"ghost"),%s' % pc(*c))
for c in pk.take(court, 4):
    vecna.append(mon('Z', 'revenant', c))
for c in pk.take(court, 2):
    vecna.append(mon('e', 'beholder', c))
for sec in (sec_n, sec_s):
    for c in pk.take(sec, 2):
        vecna.append(mon('L', 'master lich', c))
    for c in pk.take(sec, 2):
        vecna.append("MONSTER:'Z',%s" % pc(*c))
    for c in pk.take(sec, 2):
        vecna.append("MONSTER:'M',%s" % pc(*c))
for c in pk.take(lavaside, 4):
    vecna.append(mon(':', 'salamander', c))
vecna.append("MONSTER:('V',\"Kas\"),(72,09)")
for nm, ch, n in (("weredemon", '&', 1), ("balrog", '&', 1), ("demilich", 'L', 2),
                  ("vampire lord", 'V', 5), ("vampire mage", 'V', 1)):
    for c in pk.take(kas, n):
        vecna.append(mon(ch, nm, c))
vecna.append("OBJECT:('(',\"iron safe\"),(71,08)")
vecna.append("OBJECT:('(',\"chest\"),(71,10)")

# ---- level 3: Vecna's lair
g = Grid(' ')
g.rect(54, 1, 74, 16, '.')       # the east bank
g.rect(0, 0, 0, 0, ' ')
g.rect(53, 3, 53, 16, 'L')       # the lava river
g.rect(53, 1, 53, 2, '.')
g.box(66, 2, 72, 6)              # the alhoon's room
g.set(69, 6, '+')
# the fortress
g.box(4, 1, 52, 16)
g.set(52, 2, 'S')                # the secret way in, by the top of the river
g.set(53, 9, 'L')            # the span
g.set(52, 9, '|')
# the entrance hall past the drawbridge
g.box(44, 6, 52, 12)
g.set(44, 9, '+')
# the storeroom in the middle
g.box(28, 6, 37, 12)
g.set(37, 9, '+')
# the throne room
g.box(6, 4, 24, 13)
g.set(24, 8, '+')
g3 = g
vecna.append(header("vecna3", "noteleport,hardfloor,mazelevel") + '\nMAP\n%s\nENDMAP' % g3.text())
vecna.append('REGION:(00,00,75,17),unlit,"ordinary"')
vecna.append('REGION:(07,05,23,12),lit,"throne",unfilled')
vecna.append('NON_DIGGABLE:(00,00,75,17)')
vecna.append('STAIR:(73,15),up')
vecna.append('DRAWBRIDGE:(53,09),west,closed')
hall = g3.cells('.', 45, 7, 51, 11)
store = g3.cells('.', 29, 7, 36, 11)
throne_room = g3.cells('.', 7, 5, 23, 12)
fort = [c for c in g3.cells('.', 5, 2, 51, 15)
        if c not in hall and c not in store and c not in throne_room]
bank = g3.cells('.', 54, 1, 74, 16)
pk = Picker([(73, 15), (10, 8), (8, 8), (7, 8)])
vecna.append("OBJECT:('\\\\',\"throne\"),(08,08)")
vecna.append("MONSTER:('L',\"Vecna\"),(08,08)")
for c in pk.take([(x, y) for (x, y) in throne_room if x in (10, 11)], 4):
    vecna.append(mon('L', 'lich', c))
vecna.append(mon('L', 'arch-lich', pk.one([(x, y) for (x, y) in throne_room if x in (12, 13)])))
vecna.append("CONTAINER:('(',\"chest\"),(07,07) {\n  OBJECT:'=',blessed\n  OBJECT:'=',cursed\n  OBJECT:'=',uncursed\n  OBJECT:'\"',blessed\n  OBJECT:'\"',uncursed\n  OBJECT:'!',blessed\n  OBJECT:'!',cursed\n  OBJECT:'?',blessed\n  OBJECT:'?',uncursed\n  OBJECT:'?',uncursed\n  OBJECT:'+',uncursed\n  OBJECT:'+',uncursed\n  OBJECT:'+',uncursed\n  OBJECT:'+',cursed\n  OBJECT:'+',blessed\n  OBJECT:'*',uncursed\n  OBJECT:'*',uncursed\n  OBJECT:'*',uncursed\n  OBJECT:'*',uncursed\n  OBJECT[20%]:('(',\"magic marker\")\n}")
vecna.append("GOLD:20000,(07,07)")
vecna.append("OBJECT:('(',\"iron safe\"),(32,09)")
vecna.append("OBJECT:('(',\"chest\"),(30,08)")
vecna.append("OBJECT:('(',\"chest\"),(30,10)")
vecna.append(mon('L', 'demilich', (32, 9)))
vecna.append(mon('U', 'shambling horror', pk.one(hall)))
vecna.append(mon('H', 'elder minotaur', pk.one(hall)))
for c in pk.take(hall, 2):
    vecna.append(mon('Z', 'skeleton', c))
vecna.append("MONSTER:('h',\"alhoon\"),(69,03)")
vecna.append("MONSTER[35%]:('h',\"alhoon\"),random")
vecna.append(mon('L', 'master lich', (53, 1)))
for c in pk.take(g3.cells('L', 53, 3, 53, 16), 4):
    vecna.append(mon(':', 'salamander', c))
for ch, n in (('Z', 4), ('M', 4), ('V', 2), ('&', 2)):
    for c in pk.take(fort, n):
        vecna.append("MONSTER:'%s',%s" % (ch, pc(*c)))
for c in pk.take(fort, 2):
    vecna.append('MONSTER:(\' \',"ghost"),%s' % pc(*c))
for c in pk.take(fort + throne_room, 12):
    vecna.append(mon('Z', 'revenant', c))
for c in pk.take(fort, 4):
    vecna.append(mon('L', 'lich', c))
for c in pk.take(fort, 2):
    vecna.append(mon('L', 'demilich', c))
vecna.append(mon('e', 'beholder', pk.one(fort)))
for c in pk.take(fort, 4):
    vecna.append(mon('d', 'hound of Tindalos', c))
vecna.append('LOOP [8] {\n  OBJECT:random,random\n}')
vecna.append('LOOP [12] {\n  TRAP:random,random\n}')
vecna.append("OBJECT:(')',\"axe\"),random")
for nm in ("archeologist", "barbarian", "bard", "caveman", "healer", "knight",
           "pirate", "priest", "ranger", "rogue", "samurai", "tourist",
           "valkyrie", "wizard"):
    vecna.append("OBJECT:('%%',\"corpse\"),random,montype:\"%s\"" % nm)

# ============================================================ the caves
caves = []

# ---- the Sunless Sea
g = Grid('}')
for y in range(1, H - 1):
    for x in range(1, 5):
        g.set(x, y, '.')                     # the western shore
g.rect(0, 0, 0, H - 1, '}')
isl = []
for y in range(5, 13):
    for x in range(32, 46):
        if ((x - 38.5) / 7.0) ** 2 + ((y - 8.5) / 4.0) ** 2 <= 1.0:
            g.set(x, y, '.')
            isl.append((x, y))
# the beach round the island
for y in range(3, 15):
    for x in range(28, 50):
        if ((x - 38.5) / 11.0) ** 2 + ((y - 8.5) / 6.2) ** 2 <= 1.0 and g.get(x, y) == '}':
            g.set(x, y, '.')
beach = [c for c in g.cells('.', 28, 3, 49, 14) if c not in isl]
# a small island for the engraving
for (x, y) in [(60, 4), (61, 4), (60, 5), (61, 5), (59, 5)]:
    g.set(x, y, '.')
sea = g.text()
water = g.cells('}', 5, 1, W - 2, H - 2)
pk = Picker()
caves.append(header("sunless", "mazelevel,noteleport") + '\nMAP\n%s\nENDMAP' % sea)
caves.append('REGION:(00,00,75,17),lit,"ordinary"')
caves.append('STAIR:(02,09),up')
caves.append("OBJECT:('(',\"chest\"),(38,08)")
for _ in range(2):
    caves.append("OBJECT:'/',%s" % pc(*pk.one(isl)))
for _ in range(2):
    caves.append("OBJECT:'\"',%s" % pc(*pk.one(isl)))
for _ in range(2):
    caves.append("OBJECT:'=',%s" % pc(*pk.one(isl)))
for _ in range(3):
    caves.append("OBJECT:'*',%s" % pc(*pk.one(isl)))
for c in pk.take(beach, 4):
    caves.append("OBJECT:random,%s" % pc(*c))
for c in pk.take(beach, 5):
    caves.append(mon('s', 'giant crab', c))
for c in pk.take(water, 8):
    caves.append(mon(';', 'shark', c))
for c in pk.take(water, 7):
    caves.append("MONSTER:';',%s" % pc(*c))
caves.append('ENGRAVING:(60,04),burn,"I slept, and the lamp washed into the sea."')
caves.append('OBJECT:random,(60,04)')
lamp = pk.one([c for c in water if c[0] > 8])
caves.append("OBJECT:('(',\"magic lamp\"),%s" % pc(*lamp))

# a sunken city's worth of deep ones: the deepest one rules the island
caves.append(mon('H', 'deepest one', (38, 7)))
for c in pk.take(isl + beach, 12):
    caves.append(mon('h', 'deep one', c))
for c in pk.take(isl + beach, 4):
    caves.append(mon('h', 'deeper one', c))

# ---- the Spider Caves
caves.append('''
LEVEL: "spiders"
FLAGS:mazelevel
INIT_MAP: mines, '.' , ' ', true, true, unlit, true
NOMAP
STAIR: random, up''')
caves.append("OBJECT:('!',\"full healing\"),random\nOBJECT:('!',\"full healing\"),random")
caves.append("OBJECT:('/',\"make invisible\"),random\nOBJECT:('/',\"fire\"),random")
caves.append("OBJECT:('\"',\"amulet versus poison\"),random")
caves.append("OBJECT:('[',\"speed boots\"),random")
caves.append("OBJECT:('=',\"conflict\"),random")
for amt in (150, 220, 310, 400, 520):
    caves.append("GOLD:%d,random" % amt)
caves.append("LOOP [48] {\n  OBJECT:('%',\"egg\"),random,montype:\"giant spider\"\n}")
caves.append("LOOP [48] {\n  TRAP:\"web\",random\n}")
caves.append("LOOP [22] {\n  MONSTER:('s',\"giant spider\"),random\n}")
caves.append("LOOP [10] {\n  MONSTER:'s',random\n}")
caves.append("MONSTER:('s',\"Shelob\"),random\nMONSTER:('s',\"Girtab\"),random")
caves.append("LOOP [2] {\n  MONSTER:('s',\"werespider\"),random\n}")
caves.append("MONSTER:('@',\"werespider\"),random")

# ---- the Giant Caverns
g = Grid(' ')
# the west entrance cavern with the stairs
g.box(1, 5, 9, 12)
g.set(9, 8, '+')
# the great corridor
g.rect(10, 8, 20, 8, '.')
# the guard rooms north and south of the corridor
g.box(12, 1, 20, 5)
g.set(16, 5, '+')
g.set(16, 7, '.')
g.rect(16, 6, 16, 7, '.')
g.box(12, 11, 20, 16)
g.set(16, 11, '+')
g.rect(16, 9, 16, 10, '.')
# the central hall
g.box(21, 3, 42, 14)
g.set(21, 8, '+')
# the throne hall
g.box(44, 2, 72, 15)
g.set(43, 8, '.')
g.set(42, 8, '+')
g.set(44, 8, '+')
# the treasury behind a secret door
g.box(60, 5, 70, 11)
g.set(60, 8, 'S')
gg = g
hall = gg.cells('.', 22, 4, 41, 13)
guard_n = gg.cells('.', 13, 2, 19, 4)
guard_s = gg.cells('.', 13, 12, 19, 15)
throne = gg.cells('.', 45, 3, 58, 14)
vault = gg.cells('.', 61, 6, 69, 10)
corr = gg.cells('.', 10, 8, 20, 8)
entry = gg.cells('.', 2, 6, 8, 11)
pk = Picker([(3, 8), (70, 8), (66, 8)])
caves.append(header("giants", "mazelevel") + '\nMAP\n%s\nENDMAP' % gg.text())
caves.append('REGION:(00,00,75,17),lit,"ordinary"')
caves.append('REGION:(45,03,58,14),lit,"throne",unfilled')
caves.append('STAIR:(03,08),up')
caves.append("OBJECT:('\\\\',\"throne\"),(52,08)")
caves.append("MONSTER:('H',\"Hrungnir\"),(52,08)")
caves.append("CONTAINER:('(',\"chest\"),(66,08) {\n  OBJECT:'\"'\n  OBJECT:'\"'\n  OBJECT:'*'\n  OBJECT:'*'\n  OBJECT:'*'\n  OBJECT:'*'\n  OBJECT:'*'\n  OBJECT:random\n  OBJECT:random\n  OBJECT:random\n}")
caves.append("GOLD:%d,(65,08)" % rng.randint(1500, 3500))
giants = ([("hill giant", 3), ("fire giant", 3), ("frost giant", 3),
           ("storm giant", 3), ("stone giant", 3), ("jotunn", 5)])
spots = throne + hall + guard_n + guard_s
for nm, n in giants:
    for c in pk.take(spots, n):
        caves.append(mon('H', nm, c))
for ch, n in (('H', 7), ('O', 8), ('T', 4)):
    for c in pk.take(spots + corr, n):
        caves.append("MONSTER:'%s',%s" % (ch, pc(*c)))
caves.append('LOOP [10] {\n  OBJECT:random,random\n}')
caves.append('LOOP [4] {\n  TRAP:random,random\n}')
caves.append('NON_DIGGABLE:(00,00,75,17)')

out = sys.argv[1]
open(out + '/vecna.des', 'w').write(
    "#\tvecna.des\n#\tVecna's Domain, after EvilHack: the withered forest, the lava\n"
    "#\tstronghold of Kas and Vecna's lair.  Generated by util/gendomains.py.\n#\n"
    + '\n'.join(vecna) + '\n')
open(out + '/caves.des', 'w').write(
    "#\tcaves.des\n#\tThe Sunless Sea, the Spider Caves and the Giant Caverns, after\n"
    "#\tSLASH'EM.  Generated by util/gendomains.py.\n#\n"
    + '\n'.join(caves) + '\n')
print("ok")
