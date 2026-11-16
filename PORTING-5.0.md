# Porting NetHack 3.6.x / 5.0 fixes into JadeHack

Source of truth: NetHack 5.0.0 tag `NetHack-5.0.0_Released`, files
`doc/fixes3-6-0.txt` ... `doc/fixes3-6-7.txt` and `doc/fixes5-0-0.txt`.
The 5.0 notes only cover changes since 3.6.7; a 3.4.3-based tree also needs
the 3.6.x notes.

This file tracks the **crash / memory-safety batch** (217 entries matched by
keyword). Status codes: VERIFIED = code read in this tree; LIKELY = description
matches code that exists here, not yet read; N/A = code absent or disabled here.

## Verified present in this tree: PORTED (macOS build OK; escapes() ASan-tested)

| Fix (source file) | Where here | Notes |
|---|---|---|
| pline() buffer overflow on long messages (3.6.0, 3.6.5) | `src/pline.c` `Vsprintf(pbuf, ...)` into `char pbuf[BUFSZ]`; 3 unbounded `Vsprintf` calls | Use `vsnprintf` where available, else bound the format. Solaris 10 has `vsnprintf`. |
| Option escape handling overrun (3.6.0) | `src/options.c` `escapes()` | Writes into `tp` with no bound; `\\` at end of string reads past NUL (`cp[1]`). |
| `docall` prompt overflow with long type names (3.6.1) | `src/do_name.c` `Sprintf(qbuf, "Call %s:", an(xname(...)))` into `QBUFSZ` | Truncate with `strncpy`/`Snprintf` style bound. |

## Batch 2: PORTED (macOS build OK)

| Fix | Where here |
|---|---|
| Prayer: `Luck` dropping below -1 mid-prayer gave `rn1(0,...)` divide by zero (3.6.0) | `src/pray.c` `pleased()` now uses `max(Luck, -1)` |
| Bullwhip at map edge targeted outside the map (3.6.0) | `src/apply.c` `use_whip()` checks `isok(rx, ry)` |
| Eating a generic egg indexed `mons[-1]` (5.0, pr#547) | `src/eat.c` `fpostfx()` requires `corpsenm >= LOW_PM` |
| Applying a wielded cream pie left a dangling `uwep` (3.6.0) | `src/apply.c` `use_cream_pie()` calls `setnotworn()` |

Checked and found N/A here: levitation boots over a sink (3.6 sink change),
`gold_detect` NULL check (no NULL check exists to misorder), `#untrap`
divide by zero (5.0 itself says it can't happen).

## Batch 3: PORTED (macOS build OK)

| Fix | Where here |
|---|---|
| `O` command: "Set <option> to what?" answer overflowed `buf` (3.6.3) | `src/options.c` `doset()` uses `Snprintf` |
| `windowcolors` option value copied unbounded into `BUFSZ` (3.6.x config overflow family) | `src/options.c` `wc_set_window_colors()` bounded `strncpy` |
| Autopickup exception pattern plus added quotes could overrun `apebuf` (3.6.x) | `src/options.c` `apebuf` has room for the quotes |

Checked and found N/A or already handled: acid potion into water (this tree
already tracks `obj_destroyed`), `topten` long-name loop (SporkHack has its own
workaround), `windowtype` copy (uses bounded `nmcpy`), config file line reading
(`fgets` is bounded).

Open: vault guard `fakecorr overflow`. The 5.0 and 3.4.3 path code differs in
a few places (pool check, engraving cleanup) but I could not identify which
change fixes the overflow, so nothing was ported.

## Batch 4: PORTED (macOS build OK, game starts; not exercised in play)

| Fix | Where here |
|---|---|
| Burnt-out wielded/quivered candle or oil potion left a dangling `uwep`/`uquiver` (3.6.0) | `src/timeout.c` `burn_object()` calls `setnotworn()` before freeing |
| Monster killed mid-volley kept throwing (3.6.0) | `src/mthrowu.c` `thrwmu()` stops the loop on `DEADMONSTER` |
| Reviving a shopkeeper corpse read `ESHK()` of a dummy monster with no eshk data (3.6.1) | `src/shk.c` `replshk()` skips the old monster's data when it isn't a shopkeeper |

Not ported: rolling boulder placement (a gameplay change, not a crash), helm of
opposite alignment `uarmh` (3.6 polymorph code), aklys `thrownobj` (unclear
whether this tree's throw code has the stale pointer).

## Batch 5: the two `mons[-1]` fixes from 3.6.5 (PORTED; build OK, game starts)

| Fix | Where here |
|---|---|
| Gating in a demon when `ndemon()`/`dprince()`/`dlord()` returned NON_PM indexed `mons[-1]` | `src/minion.c` `msummon()`: four `cnt` computations check `dtype != NON_PM` |
| "Tin cures stoning" | This tree has no monster tin-eating, so the tin case is N/A. The same bug class does exist for generic eggs (corpsenm NON_PM), fixed in `src/dog.c` (`dogfood()`), `src/mon.c` (`mstoning()`) and `src/muse.c` (egg stoning check), alongside the earlier `eat.c` fix. |

## Batch 6/7: shop fixes (ported; build OK, game starts, not exercised in a shop)

| Fix | Where here |
|---|---|
| Stolen/broken/thrown-out stacks billed for one item (3.6.0) | `src/shk.c` `stolen_value()` multiplies by `quan` |
| Price of a billed or split object changed because its new `o_id` lost/gained the `o_id % 4` surcharge (3.6.0, 3.6.2) | `src/mkobj.c` new `nextoid()` keeps the surcharge the same; used by `splitobj()` and `bill_dummy_object()` |
| Billing dummy was flagged as worn; hero's own object kept a stale `no_charge` (3.6.0) | `src/mkobj.c` `bill_dummy_object()` clears `owornmask`; `no_charge` only for floor/contained objects |
| Hero reviving a shop-owned corpse (undead turning) was never charged (3.6.0) | `src/zap.c` `revive(obj, by_hero)` bills via `stolen_value()` after splitting one off |
| Applying an unpaid stack of oil made hero buy the whole stack (3.6.0) | `src/apply.c` `light_cocktail()` splits first, then bills |
| Snuffing a wielded/quivered oil potion re-merged it and could leave a stale pointer (5.0) | `src/apply.c` `light_cocktail()` skips re-merge when worn |

Checked, not ported:
- Horn of plenty shop ownership: a rules change, not an exploit fix.
- Tin stack charging (3.6.1): same logic already here; the bug was in 3.6's new tin code.
- Teleported-object robbery (5.0): this tree deliberately bills the hero; 5.0 chose robbery.
- Thrown wand shatter, `#tip` gold, shop-owned boulders: features or code this tree lacks.
- Gold into a hero-owned container (3.6.1): 5.0 `in_container()`/`sellobj()` differ
  structurally; no single fix identified.
- Statue reanimation charge (3.6.0 [90]): not looked at in code.

## Batch 8: more shop fixes (ported; build OK, game starts, not exercised in a shop)

| Fix | Where here |
|---|---|
| Dropping an unpaid item while engulfed left it unpaid and still on the bill after the engulfer died (3.6.2) | `src/do.c` `dropy()`: treated as theft via `stolen_value()` + `subfrombill()` before the engulfer takes it |
| Throwing an unpaid item while levitating, recoiling out of the shop, left it unpaid on the floor and on the bill (3.6.2) | `src/dothrow.c` `check_shop_obj()` also accepts the previous shop (`u.ushops0`) |

Only non-container items are handled for the engulfer case; containers with
unpaid contents still need `stolen_container()`-style handling.

Examined and not ported (no clear, separable fix, or behavior not an exploit):
- Sold-twice cases [122]/[123]: 5.0's `sellobj()`/`dropped_container()` differ
  structurally and the notes don't say which change fixes it.
- Gold in a hero-owned floor container giving credit [21], stolen-by-monster
  floor items (5.0 `mpickobj()`), breaking container contents [82], uncharged
  thefts [84], nested container gold [103], bag of holding explosion billing
  [73], statue reanimation [90b]: need a shop test harness to verify; the notes
  are too vague to port safely from prose alone.
- Autopickup of unpaid items [24]: quality-of-life, not an exploit.
- Shop wall repair [17]/[18]/[292]/[293], Passes_walls shared wall [229],
  shop-owned boulders [369]/[412], angry shk thrown items [521]: 3.6/3.7-era
  code paths or too edge-case to verify here.

## N/A because SporkHack changed or removed the code

| Fix | Reason |
|---|---|
| All `cutworm` fixes (3.6.0, 3.6.2, 3.6.3) | `cutworm()` in `src/worm.c` starts with an unconditional `return;`, so long worms can't be cut. |
| `morguemon` NON_PM dereference (3.6.0) | No `morguemon` function with the 3.6 `ndemon` logic in this tree. |

## LIKELY applicable (read code before porting)

Prayer and Luck divide by zero; bullwhip at map edge; applying a wielded cream
pie; thrown potion hitting iron bars; potion of acid dropped into water (freed
memory); "object lost" panics from burning potions of oil, levitation or water
walking boots, polymorph and wielded potions; levitation boots on a sink; shop
and shopkeeper fixes (corpse revival, stolen object outside never-entered
shop, split bills); vault guard "fakecorr overflow"; aklys `thrownobj` stale
pointer; helm of opposite alignment `uarmh` null; multishot volley freed
monster; inventory-slot overflow cases; Plane of Water bmask; `#untrap` divide
by zero; egg `fpostfx` index; farlook `xdir`/`ydir` bounds; `gold_detect` null
check; rolling boulder trap segfault; long-name `topten` loop; config-file long
line and `O` option overflows; `data.base` lookup edge cases.

## N/A: code absent in 3.4.3 or not applicable to a Unix tty/curses build

sortloot, perm_invent, status highlights, sysconf and PANICTRACE, Lua and themed
rooms, `sanity_check`, EXTRA_SANITY_CHECKS, Qt, X11, win32, Amiga and DOS ports,
3.6+ features (zombies, `#terrain`, `#therecmdmenu`, repeat-after-prefix, split
stack `w`). Curses entries need a separate look because `win/curses` exists here
but is a different port than 5.0's.

## Next steps

1. Port the three VERIFIED fixes, rebuild on macOS, smoke test.
2. Read code for each LIKELY item, move it to VERIFIED or N/A, port in batches.
3. (Done: the three VERIFIED fixes below.)
4. Only then triage the non-crash bug fixes in the same notes files.

## Non-crash bug fix triage (in progress)

Scope: the "General Fixes and Modified Features" sections of
`fixes3-6-0.txt` .. `fixes3-6-7.txt` and `fixes5-0-0.txt` (2,235 entries).
Sections for platform ports, new features, code cleanup, and "fixes to
post-X problems exposed via git" are skipped: they concern code this tree
does not have.

Automatic filtering (heuristic, not code-verified):
- 172 entries mention ports/tools this tree lacks (Qt, X11, win32, sysconf,
  Lua, sortloot, perm_invent, etc.).
- 393 mention terms that exist in 5.0 but nowhere in this source tree.
- 1,521 entries are plain prose about behavior with no identifiers to check,
  so each needs a human read. Only 39 name identifiers that do exist here.

Prose read: all 1,521 unanchored entries. 516 flagged as plausible real bugs,
one file per source, in `doc/port-candidates-*.txt`:

| Source | Prose entries | Flagged |
|---|---|---|
| 3.6.0 | 557 | 185 |
| 3.6.1 | 217 | 73 |
| 3.6.2 | 105 | 56 |
| 3.6.3 | 61 | 25 |
| 3.6.4 - 3.6.5 | 9 | 4 |
| 5.0.0 | 570 | 173 |

The unflagged entries are design changes (two-weapon combat, Elbereth, magic
cancellation, many 5.0 balance changes), grammar-only fixes, and features. The
39 entries that name identifiers present in this tree and the 393 with
5.0-only terms were not read individually; revisit if the flagged list runs dry.

Flags come from reading descriptions only. Many 3.6/5.0 entries fix bugs that
were introduced in 3.6 itself, so expect a good share to be N/A once the code
is checked. Highest value first: the two `mons[-1]` accesses in
`doc/port-candidates-3.6.5.txt` (gating a non-valid demon, tin cures stoning),
then shop exploits, then the rest.

## Batch 9: non-shop 3.6.0 candidates, first pass (ported; build OK, tested where noted)

| Fix | Where here | Tested |
|---|---|---|
| Silver ammo wielded/thrown without a launcher did no silver damage ([31]) | `src/uhitm.c` | build only |
| Plural of "Nazgul" ([34]) | `src/objnam.c` | build only |
| Nymphs/monkeys could take rings from under gloves or a cursed weapon; nymphs could steal boulders ([105][106][367]) | `src/steal.c` | build only |
| Wielded artifact light (Sunsword) kept shining after its monster died ([145]) | `src/steal.c` `mdrop_obj()` | build only |
| Lose-level/regain-level cycles boosted HP/Pw ([153]); `u.uhpinc[]`/`u.ueninc[]` record each level's gain | `include/you.h`, `src/exper.c` | in game (`#levelchange`: 23 HP back to exactly 16) |
| Elven cloak, cloak of displacement and ring of stealth discovered wrongly or never ([438][439][440]); "float imperceptibly" while levitating ([232]) | `src/do_wear.c` | in game (elven cloak message) |
| Low-Int spell damage could go negative and heal ([432]) | `src/zap.c`, `src/spell.c` | build only |

Struct `you` changed size: saves, bones, and the data files (`dungeon`, `nhdat`)
must be rebuilt together or the game stops with "Dungeon description not valid".

Checked, not needed: slimeproof already covers noncorporeal monsters ([111]).
Skipped on inspection: score wraparound ([48], needs >2 billion points), empty
wand engraving turn ([301], unclear), lock pick ESC turn ([404], already
returns without using a move).

Not yet worked through: about 150 other entries in `doc/port-candidates-3.6.0.txt`.

## Batch 10: more non-shop 3.6.0 candidates (ported; build OK, game starts, not exercised in play)

| Fix | Where here |
|---|---|
| Wielded/quivered cloak of MR or gray dragon scales gave magic resistance to a polymorphed hero; a wielded potion of blindness gave blindness resistance ([277][278]) | `src/mondata.c` `resists_magm()`, `resists_blnd()` only count worn armor/accessories (and a wielded weapon) |
| Eating rings could raise to-hit, damage and protection without limit ([126]) | `src/eat.c` new `bounded_increase()` |
| Gelatinous cubes could engulf Rider corpses ([275]) | `src/mon.c` `meatobj()` |
| Tainted Medusa corpse gave food poisoning instead of petrification ([103]) | `src/eat.c` `eatcorpse()` treats Medusa as stoning flesh |
| A monster digging a hole cleared the hero's in-floor trap even when the hole was elsewhere ([217]) | `src/dig.c` `digactualhole()` checks `at_u` |
| A djinni from a bottle that "disturbed" you could be peaceful ([68]) | `src/potion.c` `djinni_from_bottle()` |

Skipped: recursive `spoteffects()` guard ([239]) needs the function restructured
with a shared exit label.

Still not worked through: roughly 130 other entries in `doc/port-candidates-3.6.0.txt`.

## Batch 11: more non-shop 3.6.0 candidates (ported; build OK, game starts, not exercised in play)

| Fix | Where here |
|---|---|
| Names from loaded bones (monsters, corpse/statue names, engravings) could carry control codes ([381][508]) | `src/bones.c` new `sanitize_name()` |
| Archeologist's slingable touchstone conferred sling skill ([406]) | `src/weapon.c` `skill_init()` skips ammo in the starting-inventory pass |
| Wearing or removing an amulet of restful sleep clobbered permanent sleepiness ([407]) | `src/do_wear.c` |
| Monsters in suits or cloaks put on shirts; priests and minions wore helms of opposite alignment ([409][427]) | `src/worn.c` `m_dowear_type()` |
| #sit while swallowed gave the wrong message ([484]) | `src/sit.c` |
| Reverse genocide said "some <plural>" even for one monster ([485]) | `src/read.c` |
| A stunned hero could cast spells ([554]) | `src/spell.c` `docast()` |

Checked, N/A: `data.base` "bat"/"combat"/"jack boot" clashes ([509-511]); this tree's
`data.base` has no standalone "bat" entry.

Still not worked through: roughly 110 other entries in `doc/port-candidates-3.6.0.txt`.

## Batch 12: more non-shop 3.6.0 candidates (ported; build OK, game starts, not exercised in play)

| Fix | Where here |
|---|---|
| Wielding a polearm and then pressing ESC at the target prompt took no time ([426]) | `src/apply.c` `use_pole()`, `use_grapple()` |
| A hero polymorphed into a handless form could throw ([298]) | `src/dothrow.c` |
| Eating an artifact the hero can't touch ([300]) | `src/eat.c` `doeat()` |
| A T-shirt worn under a suit could be read ([430]) | `src/read.c` |
| Alchemy explosion didn't wake nearby monsters ([401]) | `src/potion.c` |
| Stone-resistant monsters couldn't wield cockatrice corpses barehanded ([481]) | `src/weapon.c` |
| #untrap didn't require reaching the floor ([370]) | `src/trap.c` `dountrap()` |
| Cockatrice nest/Medusa statues were all giant ants when the record file is empty ([470]) | `src/mkobj.c` `mk_tt_object()` |

Skipped on inspection: bag of holding weight off by one ([456]); this tree's
`DELTA_CWT` macro and `weight()` use matching formulas that 5.0 reworked
together, so changing one without the other would be worse. Amnesia ([368])
already looks correct here.

Still not worked through: roughly 100 other entries in `doc/port-candidates-3.6.0.txt`.

## Batch 13: more non-shop 3.6.0 candidates (ported; build OK)

| Fix | Where here | Tested |
|---|---|---|
| Zapping opening magic at yourself while punished only made the chain quiver ([190]) | `src/zap.c` `zapyourself()` calls `unpunish()` | in game |
| Hero could swap places diagonally with a tame grid bug ([382]) | `src/hack.c` `domove()` | build only |
| Blind levitating hero was told "stairs" without having seen them ([55]) | `src/do.c` `dodown()` | build only |
| Corpses/statues placed by special-level files kept the weight of the default species ([59]) | `src/sp_lev.c` `create_object()` | build only |

Still not worked through: roughly 95 other entries in `doc/port-candidates-3.6.0.txt`.
Many of the rest are rare edge cases (bones leaks, conduct bookkeeping, vault
guard corridors, polymorph messaging) that need a hand-built scenario to check.

## Batch 14: fixes from later versions (3.6.1, 5.0) (ported; build OK, game starts, not exercised in play)

The remaining 3.6.0 entries are mostly rare edge cases and are being left alone.

| Fix | Where here |
|---|---|
| "The the Oracle corpse tastes terrible" for unique monsters ([3.6.1 136]) | `src/eat.c` |
| Orcish heroes could start with a ring of poison resistance ([3.6.1 125]) | `src/u_init.c` |
| Rogue's lock pick started as "+9" though enchantment is meaningless ([3.6.1 165]) | `src/u_init.c` |
| Monsters hit by a scroll-of-earth boulder weren't woken/angered ([3.6.1 181]) | `src/read.c` |
| Random monster teleports never chose map column 1 ([5.0 420]) | `src/teleport.c` `rloc()` |
| Brass lantern said "lamp has run out of power" ([5.0 143]) | `src/apply.c` |
| Out-of-bounds read of `levl[][]` when a ray bounced off the map edge ([5.0 319]) | `src/zap.c` `dobuzz()` |

Checked, not needed: `squadprob[]` size ([5.0 62]; `NSTYPES` is 4 here), monster
multishot launcher typo ([3.6.3 29]; already uses the monster's weapon), monsters
not wielding digging tools ([3.6.2 19]; same logic as 5.0), musical note grammar
([5.0 117]; no notes in this tree).

## Batch 15: 5.0 interface features, group A (ported; build OK)

| Feature (5.0 entry) | Where here | Tested |
|---|---|---|
| `#vanquished` and `#genocided` commands ([120][123]) | `src/end.c` `show_vanquished()`/`show_genocided()`, `src/cmd.c` | in game (empty-list messages) |
| `?` at the "genocide what?" prompts lists what is already genocided ([139]) | `src/read.c` | build only |
| Monster size in probing and stethoscope output ([133]) | `src/pline.c` `mstatusline()` | in game ("...AC 10, small.") |
| Farlook shows "asleep", "can't move" or "meditating" ([100][101]) | `src/pager.c` `lookat()` | build only |
| `swimconfirm` (default on) and `trapconfirm` (default off) boolean options ask before a deliberate step into known water/lava or a known trap ([96-98]) | `include/flag.h`, `src/options.c`, `src/hack.c` `hazard_declined()` | build only |

`swimconfirm`/`trapconfirm` follow this tree's existing `prayconfirm` style rather
than 5.0's `paranoid_confirmation:` syntax. They only prompt on single steps,
not while running or travelling. `struct flag` grew, so data files must be rebuilt
(`make dlb`) as with earlier struct changes.

Not ported from group A: grave headstone text on farlook ([147]; needs a "read"
flag on engravings), unpaid items on the floor in `I u` ([124]), toggle feedback
and `name:value` boolean syntax ([71][45][177]), `sortdiscoveries` ([60]).

## Batch 16: 5.0 gameplay features, group B (ported; build OK, game starts)

| Feature (5.0 entry) | Where here | Tested |
|---|---|---|
| Blessed scroll of teleportation grants one-shot teleport control ([52]) | `src/read.c` | build only (wizard mode always has control) |
| Full healing, and blessed extra healing, cure wounded legs ([91][92]) | `src/potion.c` | build only |
| Drinking a burning potion of oil cures sliming ([80]) | `src/potion.c` | build only |
| Vomiting on an altar angers the god ([82]) | `src/eat.c` `vomit()` | build only |
| High bare-handed/martial arts skill sometimes hits twice ([138]): skilled 20%, expert 40%, master 60%, grand master 80% | `src/uhitm.c` `double_punch()`, `hitum()` | build only |
| Rangers get +1 multishot with any arrow while wielding the Longbow of Diana ([119]) | `src/dothrow.c` | build only |
| A ring of stealth leaves no tracks ([172]) | `src/allmain.c` | build only |
| Pyrolisk eggs explode when broken ([157]) | `src/dothrow.c` `breakobj()` | build only |
| Dropped potions often break on hot ground in Gehennom and the Plane of Fire ([150]) | `src/do.c` `flooreffects()` | build only |

Balance notes: [138] is a large buff for Monks and Samurai; ring-of-stealth
no-tracks ([172]) makes pets follow less reliably.

Not ported from group B: food poisoning countdown messages ([90]), tourists
reading conical hats ([55]), giants wearing mummy wrappings ([118]), throwing
invocation items at the quest leader ([140]), applying gold to flip a coin
([141]), dipping to clean slippery fingers / test potions in a sink ([144][145]),
iron footwear vs. floor traps ([171]), purple worm poly intrinsics ([116]),
spinach tins in the Monk quest ([117]), and monsters seeing/remembering hero
resistances ([76][77], a balance decision).

## Batch 17: group C (partly done; build OK, uncommitted)

| Feature | Where here | Tested |
|---|---|---|
| Themed rooms: 13 fills (ice, boulders, spider nest, trap room, buried treasure, massacre, statuary, lamp, three altars, ghost, storeroom, teleport hub, garden); about 1 in 16 eligible ordinary rooms (`THEMED_ROOM_CHANCE`), 80% skip normal contents | `src/mklev.c` `mkthemedroom()` | in game at 100%: DL2-14 generated with no panics; ice/boulder/massacre rooms seen |
| Item-actions menu: `i` then pick an item for a "Do what with it?" menu (wield, wear, eat, read, zap, throw, drop, call...) | `src/invent.c` `itemactions()`, `getobj()` preselect | in game (wield, take off menus) |
| Shield of drain resistance (new; small shield probability 5 to 3 to keep the class at 1000) | `src/objects.c` | built; data files rebuilt |

Overlap found: Spork already has gold dragons and gold dragon armor (monsters,
light, scales), and its "lightning shield" is 5.0's shield of shock resistance,
so neither was added. Not done: remaining 5.0 options (name:value booleans,
toggle feedback, #toggle, sortdiscoveries), gold-armor hallucination cure.
Objects changed, so rebuild data (`make dlb`) with the binary.

## Batch 18: remaining 5.0 options and gold armor (ported; build OK)

| Feature | Where here | Tested |
|---|---|---|
| Boolean options accept `name:true/yes/on/1` and `name:false/no/off/0` ([45]) | `src/options.c` | in game (config file; `showexp:maybe` is rejected) |
| Message when a boolean option is changed in the game ([71]) | `src/options.c` | in game |
| `#toggle` command: flip a boolean option by (unique prefix of its) name ([177]) | `src/options.c` `dotoggle()`, `src/cmd.c` | in game |
| Gold dragon scales / scale mail confer hallucination resistance ([89]) | `src/objects.c` | build only |

Not ported: `sortdiscoveries` ([60]), rebinding keys and `#saveoptions`.

## Batch 19: new items and monster from 5.0 (ported; build OK)

| New content | Where here | Tested |
|---|---|---|
| Amulet of flying ("cubical"): grants flight; "You are now in flight." / "You land." (and `spoteffects()` after landing over water or lava) | `src/objects.c`, `src/do_wear.c` | in game |
| Amulet of guarding ("perforated"): AC +2 and magic cancellation +2 (max 3) | `src/objects.c`, `src/do_wear.c` `find_ac()`, `src/mhitu.c` `magic_negation()` | in game (AC 6 to 4) |
| Helm of caution ("engraved helmet", inside the shuffled helmet range): warning while worn | `src/objects.c` | wished for; effect not exercised |
| Wand of stasis (uses the spare "spiked" wand slot): freezes every monster on the level for 10-30 turns, no message | `src/objects.c`, `src/zap.c`, `src/mon.c` `movemon()`, `level.flags.stasis_until` in `include/rm.h` | in game (monster stayed put) |
| Genetic engineer (Q, level 12): claw polymorphs the hero unless magic resistant | `src/monst.c`, `include/monattk.h` `AD_POLY`, `src/mhitu.c` | monster created; hit effect not seen |

Probabilities were rebalanced so the amulet (1000), armor (1000) and wand
classes still total 1000. `level.flags` grew (`stasis_until`), so rebuild
`dungeon`/`nhdat` with the binary and rebuild `makedefs` after editing
`objects.c`/`monst.c`.

Not ported: displacer beast (its point is the 5.0 monster-displacement
mechanic, which this tree lacks).

## Batch 20: JadeHack originals, set 1 (build OK, tested in game)

Not from NetHack; designed for JadeHack.

| New content | Details |
|---|---|
| Falchion | d8/d8, steel, curved sword skill, generation prob 15 (as scimitar) |
| Kriegsmesser | two-handed, 2d6 small and large, steel, curved sword skill, prob 22 (as two-handed sword); extra rnd(6) in `dmgval()` |
| Saber | d8/d8, steel, curved sword skill, prob 6; plain "saber" now names this one ("silver saber" is unchanged, "sabre" is an alias) |
| Messer | d6/d6, steel, curved sword skill, prob 10 (as short sword) |
| Wand of stinking fog ("zirconium", took the spare "jeweled" slot) | zapped: a line of single-square stinking clouds (6-11 squares, stops at walls and doors; at yourself, a cloud on your square). Engraved: "The bugs on the floor flee!". Broken: same as a scroll of stinking cloud of equal beatitude centered on you |
| Six new scroll labels: ARGLE BARGLE, HAPAX LEGOMENON, PARPENJAWEEDIE, ZALGO FHTAGN, LOREM IPSUM (spare, unused) and LUKEN BOKE (scroll of ice) | `src/objects.c` |
| Scroll of ice (LUKEN BOKE, prob 20): pick a target within sight; frost blast of 4d6 (blessed 6d6, cursed 2d6) with the usual cold effects on potions and other items, then the 3x3 area freezes: floor becomes ice, pools freeze, fountains and sinks freeze solid and are destroyed, lava cools to plain floor. Confused: no blast, floor around you freezes | `src/read.c` |

Supporting changes: weapon, wand and scroll class probabilities rebalanced to
stay at 1000; `melt_ice()` now melts floor ice back to plain floor (not moat)
and only settles boulders into real pools; this also fixes themed ice rooms.
Rebuild makedefs and the data files after changing `objects.c`.

Addendum: breaking a wand of wind while hallucinating prints "Ha ha, you broke
wind!" and releases the same stinking cloud as a broken wand of stinking fog
(`src/apply.c`); tested in game.

## Batch 22: original JadeHack additions, part 2

| Change | Notes |
|---|---|
| Grapefruit | stats of an orange (80 nutrition, weight 2), prob 10 |
| Jerky | stats of a meat stick but 50 nutrition, prob 20; random generation (so Tourists' random starting food can include it); handled as meat in `dog.c`/`eat.c` |
| Potion appearances | gunky, gelatinous, stinky, coppery, banana-yellow, sickly green, mauve, lime green (spare nameless slots in `objects.c`) |
| Ring appearances | zirconium (metal), zircon (stone, hard), osmium (metal, hard), plastic (plastic) as spare nameless slots; `o_init.c` now spreads uniform ring probabilities over named rings only |
| Random runeswords, athames, tsurugis | prob 1 each (taken from orcish/elven arrows); artifact generation may therefore yield Stormbringer or Magicbane; Muramasa keeps SPFX_NOGEN so is never random |

Food probs rebalanced (tripe -10, egg -10, tin -10) to keep the class at 1000.
Tested in wizard build: wished items, 14 random rings (no nameless ones).

## Batch 23: new artifacts and monsters

Artifacts (`include/artilist.h`, `src/artifact.c`, `src/dothrow.c`; all `SPFX_RESTR`):

| Artifact | Base | Align | Effects |
|---|---|---|---|
| Luck Blade | short sword | neutral | luckstone of equal BUC while carried; +d5 to hit, +4 damage; 1 in (20 - Luck) chance to strike deep for +3d8 |
| Quicksilver | silver short sword | neutral | wielded: new `Super_fast` property (speed 24, `SUPER_FAST` in `prop.h`, `ESuper_fast`; counts as Fast/Very_fast; shows as "super fast" in ^X); +d5 to hit, +4 damage |
| Ravenbrand | scimitar | neutral | +d5 to hit, +d10 damage; blindness resistance (`SPDF_BLIND` now actually grants `EBlind_resistance`, so Sunsword has it too); independent 1/5 slow, 1/3 extra d10, 1/5 blind with the requested messages |
| Spineseeker | stiletto | chaotic, Rogue first gift | +d5 to hit, +4 damage, double damage to fleeing foes |
| Gungnir | spear | neutral | flat +5 to hit, +10 damage; returns to your hand when thrown (any role) |

Flat bonuses are special-cased by artifact in `spec_abon()`/`spec_dbon()`.

Monsters (`src/monst.c`):
lemming, lemming zombie, wire jelly (corpse gives 5% shock resistance by the
existing corpse-weight rule), zuvembie, star vampire (always generated
invisible), fire vampire (SLASH'EM stats as listed on the NetHack wiki; the
unused spellcasting attack is omitted), Kuei-jin (never casts summon nasties),
devil bunny, stinkbite (BOOM! + stinking cloud on death), merozalghu (MZ_HUMAN
so it can wear body armor; added to the summon-nasties list; plural
merozalghi), odorous orb (stink messages in `mattacku`, BLAM! + gas-spore
explosion + stinking cloud on death). Starting weapons are in `makemon.c`.
"Rare" and "very rare" both map to generation frequency 1.
Colors: lemming and lemming zombie are green, stinkbite bright green, antaloon
green, and wire jelly bright yellow (`CLR_YELLOW`).
Rebuild makedefs and the data files after these changes (struct you grew).

## Batch 24: Bard, rapier, silver knives/bolts, skill merge, assorted

| Change | Notes |
|---|---|
| Chicken (c) | very weak, common (freq 3), bite 1d2; new `MS_GLARE` sound ("glares menacingly at you" when chatted to); corpse nutrition 250 and "tastes okay" |
| Sunsword | extra d6 damage against anything not fire resistant (`artifact_hit`) |
| Grimtooth | replaced with the NetHack 5.0 version: warns of elves, +d2 to hit vs elves, +d6 damage vs everything, permanently poisoned in melee, poison resistance while wielded, invoke (`FLING_POISON`) throws blinding or acid venom |
| Skunky | new spare potion appearance |
| Rapier | long sword skill, d10 / d4 large, +1 to hit, prob 6 (long sword 40 -> 34) |
| Silver knife, silver crossbow bolt | prob 4 / 8 (taken from knife and crossbow bolt); silver hurts silver-haters via material; monsters may throw them |
| Spear skill | trident and javelin now use the spear skill; `P_JAVELIN`/`P_TRIDENT` removed and later skills renumbered; each role keeps the best of its three old caps |
| Silver mace | already existed (base of Demonbane); no change |
| Bard | new role (see below) |

Bard (`src/role.c`, `u_init.c`, `monst.c`, `artilist.h`, `dat/Bard.des`, `dat/quest.txt`):
human/gnome/orc, neutral or chaotic, SlashTHEM stats, ranks and gods
(Minerva/Apollo/Linus); sleep resistance at XL5, stealth at XL10.  Starts with a
+1 silver dagger, short sword or messer (1/3 each), +1 leather cloak, apples,
oranges, booze, a harp or flute (orcs: bugle or tooled horn) and a bell or
whistle; knows all instruments, the five bardic spellbooks and 5-15 random
magical items.  Skills: short sword and curved sword up to Skilled (as asked).
Quest (original design): Amphion at the College of Parnassus, lyrist guardians,
sirens, nemesis Aglaope on the Isle of Anthemoessa, artifact The Lyre of Orpheus
(wooden harp: magic resistance carried, invoke = scroll of taming).
Not implemented: techniques, the musicalize spell and songs, harp rumors, and
the Shimmerstaff first gift.  The character menu in `charinit.c` gained a
Bard row (accelerator `B`).

## Batch 25: bracers, shields, songs, more monsters

| Change | Notes |
|---|---|
| Parrying dagger / main gauche | dagger d4/d3 and knife d6/d4; +1 AC plus enchantment while wielded in either hand; two of them, or one plus bracers, do not stack (the better counts); `find_ac()` is refreshed on wield/twoweapon changes |
| Dwarvish bearded axe | EvilHack stats (d8/d10, wt 70, "broad bearded axe"); apply it to hook a monster's weapon away, or chop as usual; dwarves have a 1/20 chance to carry one |
| Silver mace | prob 3 (was never generated) |
| Bracers | plain bracers (AC1 leather), runed bracers (AC2 wood, never random), dwarvish bracers (AC1 steel, safe to +7; dwarves carry them 1/40); shield-slot items that never bash, allow two-handed weapons and two-weapon fighting |
| Crystal shield | dwarvish roundshield stats, weight 80, new material `CRYSTAL`, no spellcasting penalty |
| Gloves of expertise | "snakeskin gloves", AC1; weapon skill counts as one rank higher (Expert becomes Master: +9 hit, +3 damage); a restricted weapon skill counts as Basic (it stays restricted and cannot be advanced); martial arts unaffected |
| Skills | morning star merged into mace; new Shield skill (Basic +1 AC, Skilled +3 / bracers +2, Expert +5 / bracers +3; auto shield bash 1/8, 1/6, 1/4 with +d4 and 1/10 stun at Expert; silver shields do silver damage, blessed shields hit undead and demons harder; trains when attacks miss you or you bash). Valkyries and Knights start at Basic and cap at Expert, Barbarians/Rangers/Samurai Skilled, Archeologists/Cavemen/Priests/Rogues/Tourists/Bards Basic |
| Merozalghu | acid breath 4d12 (never at melee range, usable via #monster when polymorphed); `NATTK` is now 7; breath damage now scales with the damd field (dragons unchanged) |
| Cryolisk / electrolisk | cold gaze that often freezes potions; shock gaze that detonates wands and rings unless you are shock resistant |
| Green dragon | poison breath leaves single-square stinking clouds along its path |
| Spellbook of stench | "sticky", level 4, matter, casts the wand of stinking fog effect |
| Bard songs | six songs (lullaby, cacophony, drowsiness at level 1; despair, friendship, inspire courage at level 3) are always known, need an instrument in inventory, use Charisma, and belong to the new Musical spell skill (Bards reach Expert); Bards also get Basic healing/matter, Skilled divination, Expert enchantment; friendship is temporary taming, inspire courage boosts pets' damage |
| Luck Blade | first sacrifice gift for Bards (takes the Bard's alignment) |
| Bard quest | redone after the DNetHack quest: the Conservatorium, Pindar, rhymers, the Island of Anthemoessa, the Old Ruins, Aglaope and the Lyre of Orpheus |

## Batch 26: object properties, Mephistopheles, the Mall, more monsters

| Change | Notes |
|---|---|
| Shoggoth | SLASH'EM stats (P, level 18, claw 4d8, corrosive touch, 4d8 acid engulf, passive corrosion; acid, regenerates, eats metal) |
| Nightgaunt / byakhee | z; nightgaunt (L12, bite 1d10, claw 1d8, tickle 1d4, wing 1d4) and byakhee (L22, bite 2d8, claw 2d6 x2, poison sting 1d10); both fly, frequency 1 |
| Vlad's Tower | Vlad now carries each of sleep / wind / stinking fog 30% of the time and has a random weapon in his room; two 50% Kuei-jin in the tower, always with katanas there; monsters can now zap wands of stinking fog |
| Object properties | after EvilHack: `oprops`/`oprops_known` on weapons and armor (fire, frost, shock, venom, drain, oilskin, ESP, searching, warning, excellence, fumbling, hunger) plus the new weapon-only Thundering (50% stun for 1d4 turns, monsters get `mstuntime`). Weapons add 4-8 elemental damage (venom d6), armor grants the resistance when worn; properties show in names once noticed or identified, stack only with matching properties, and wizard-mode wishes accept "thundering", "flaming", "freezing", "shocking", "venomous". Spawn: weapons at 1 in 175 scaled by depth/2, armor 1 in 150 |
| Mephistopheles | unaligned devil lord (level 115, AC -15, speed 15), weapon, claw, claw, mage, cleric and fire spell attacks; fire, cold, shock, poison and disintegration resistant; flies, sees invisible; covetous; +7 ranseur; lives asleep in the Sanctum beside the temple until you hold the Amulet; rival of the Wizard of Yendor (they fight each other); gates in demons of any alignment (unaligned) |
| Unicorn horn | no change needed: this tree's unicorn horn no longer cured attribute loss (that code was already disabled) |
| The Mall | branch with two levels (the Mall and the Rat Sewers); entrance stairs down on a level from 7 to 14. The Mall is a lit concourse with twelve shops and a fountain; the Rat Sewers is a dark cavern full of every kind of r plus lemming zombies, with 3 scrolls, 2 armors, 2 weapons and a wand guaranteed. `botl.c` knows the branch name; `mall.des` is in the makefiles |

## Batch 27: more skill merges

Broadsword merged into long sword, club into mace, shuriken into dart (each role keeps the best of its old caps; skills renumbered again, `is_multigen`/`is_poisonable` now start at `-P_DART`; the runesword now uses the long sword skill). Rangers: short sword cap Skilled, long sword Basic.

## Batch 28: crysteel falchion and Pernicimordax

Crysteel falchion ("crystal falchion"): a falchion of gemstone, +1 to hit and +1 damage, probability 1 (taken from darts). Pernicimordax: neutral artifact crysteel falchion, +d5 to hit, +3d6 cold damage to anything not cold resistant, doubled damage against merozalghi, and #invoke grants 100%% cold resistance for 100+3d100 more turns (`u.ucoldres`, ticked in `allmain.c`).

## Batch 29: crystal plate mail, Dramborleg, destroy armor

Crystal plate mail now weighs 150 and is gemstone. Dramborleg (after EvilHack): lawful self-willed dwarvish bearded axe, +d8 to hit and +d8 damage against everything, warns of demons, kills balrogs outright, magic resistance and +1 magic cancellation while wielded. The monster Destroy Armor spell, against a hero with magic resistance, now only strips erodeproofing from an erodeproof target and does nothing more.

## Batch 30: another big monster batch

New monsters: toxic mimic, glue golem (a third of the time leaves a tainted horse corpse), impede (very slow; blocks sight, absorbs rays and thrown things stop at it; hates silver and blessed items), megapede, gug (bite can paralyze), giant shoggoth (Gehennom only; shoggoth corpses are now poisonous and acidic and confer poison and acid resistance), giant solifuge (grabs like an owlbear), antaloon, snow ant, iron ant, chain devil (added to summon nasties), marraenoloth (Gehennom only), ultroloth, piscoloth, Koshchei the Deathless (1 in 10 arch-liches in the Castle or Gehennom, only generation route), nupperibo, dretch, Kerberos (EvilHack's Cerberus, placed in the Valley), yochlol, drow warrior/mage/cleric and driders (random generation). Merozalghi now come in small groups. Notes: the yochlol is a plain demon here (EvilHack has none; I used dNetHack's stats without the shapeshifting), and drider's web spit is omitted.
Items: orcish scimitar (very rare randomly; orcs carry it instead of scimitars; curved sword skill) and Mouser's Scalpel (rapier, neutral, +5 to hit, +1 damage, 1 in 5 chance of an extra blow that may chain, each worth d8+1+enchantment).
Spell: superior healing (level 5 healing spell, heals 6d12; same casting bonus as other healing spells). (Bards keep the Basic shield cap but start Unskilled.)

## Batch 31: crystal helmet

Helm of brilliance now has a fixed appearance, "crystal helmet", and is made of glass (as in NetHack 5.0), so it no longer hampers spellcasting; the special case for it in `spell.c` is gone. It no longer takes part in the helmet appearance shuffle.

## Batch 32

Bards may now also be elves (neutral or chaotic), alongside humans, gnomes and orcs. Elven Bards start with an elven short sword in place of a plain one.

## Batch 33: Bard reworked around the Musicalize spell skill

The six always-known song spells are gone, along with the Musical spell category. Songs are now performed by applying a tonal instrument (flute, horn, harp, bugle or drum, magic ones included) once your Musicalize spell skill is Basic or better: "Perform a song?" lists the songs whose spell you know (Lullaby needs sleep, Cacophony confuse monster, Drowsiness slow monster, Despair and Inspire Courage cause fear, Friendship charm monster). Each costs twice its level in Pw (1 or 3), takes two or three turns and can be interrupted. Success is 2 x (Dex, or Str for drums) x skill rank + level, +50% with the song's preferred instrument or an artifact, doubled below 30% HP, halved for harps and drums with a shield, plus level when blind. Effects last 4-6 turns (a little longer with high Charisma), pets are mildly affected at Unskilled/Basic and unaffected at Skilled/Expert, and Inspire Courage affects only pets. Bards cap at Expert (Monks at Skilled, Priests and Tourists at Basic) and start at Basic knowing only the sleep spell; applying an instrument without singing trains an Unskilled hero up to Basic. Bard enchantment spells still use Charisma.

## Batch 34: glowing potion

New potion appearance "glowing" (`objects.c`, a filler description shuffled in with the others, so it lands on a random potion type each game). A potion with that appearance is a very poor light source while carried in open inventory (not inside a container): it lights a radius-2 circle around the hero, the same radius as a lit bag of poo, but it never burns out and needs no lighting. It keeps glowing after the potion type is identified. Implemented in `do_light_sources()` (`light.c`) via `is_glowing_potion()` (`obj.h`); `addinv()` and `freeinv_core()` force a vision recalc when one is picked up or leaves inventory. Adding a potion description shifts object numbers: rebuild the special levels and data files, and old saves and bones will not load.
