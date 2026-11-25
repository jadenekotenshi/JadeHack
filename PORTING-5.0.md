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
(Minerva/Apollo/Fharlanghn); sleep resistance at XL5, stealth at XL10.  Starts with a
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
| Parrying dagger / main gauche / swordbreaker | dagger d4/d3, knife d6/d4 and short sword d6/d6 (Batch 149); +1 AC plus enchantment while wielded in either hand; two of them, or one plus bracers, do not stack (the better counts); `find_ac()` is refreshed on wield/twoweapon changes |
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
| Nightgaunt / byakhee | z; nightgaunt (L12, bite 1d10, claw 1d8, tickle 1d4, wing 1d4) and byakhee (L22 then, L13 since Batch 110, bite 2d8, claw 2d6 x2, poison sting 1d10); both fly, frequency 1 |
| Vlad's Tower | Vlad now carries each of sleep / wind / stinking fog 30% of the time and has a random weapon in his room; two 50% Kuei-jin in the tower, always with katanas there; monsters can now zap wands of stinking fog |
| Object properties | after EvilHack: `oprops`/`oprops_known` on weapons and armor (fire, frost, shock, venom, drain, oilskin, ESP, searching, warning, excellence, fumbling, hunger) plus the new weapon-only Thundering (50% stun for 1d4 turns, monsters get `mstuntime`). Weapons add 4-8 elemental damage (venom d6), armor grants the resistance when worn; properties show in names once noticed or identified, stack only with matching properties, and wizard-mode wishes accept "thundering", "flaming", "freezing", "shocking", "venomous". Spawn: weapons at 1 in 175 scaled by depth/2, armor 1 in 150 |
| Mephistopheles | unaligned devil lord (level 115, AC -15, speed 15), weapon, claw, claw, mage, cleric and fire spell attacks; fire, cold, shock, poison and disintegration resistant; flies, sees invisible; (covetous until Batch 72); +7 ranseur; lives asleep in the Sanctum beside the temple until you hold the Amulet; rival of the Wizard of Yendor (they fight each other); gates in demons of any alignment (unaligned) |
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

## Batch 35: ghost mouse, ghost rat, demighost

Three undead `r` that pass through walls (`M1_WALLWALK`, unsolid, breathless; resist cold, sleep, poison and stone) and attack with a cold touch. All are `G_NOCORPSE`. The demighost is `G_NOGEN` and appears only in the Rat Sewers (`dat/mall.des`, 6+2d4 mice, 4+2d4 rats, 2+2d4 demighosts); the mouse and rat are also generated randomly in the dungeon (frequency 2 and 1, genocidable).

| Monster | Level | Speed | AC | Attack | Colour |
|---|---|---|---|---|---|
| ghost mouse | 1 | 12 | 6 | touch cold 1d2 | red |
| ghost rat | 2 | 12 | 6 | touch cold 1d4 | red |
| demighost | 3 | 14 | 5 | touch cold 1d6 | orange (bright red) |

The spec gave only the mouse and rat damage and called the demighost "slightly stronger than a ghost rat"; its level, speed, AC and 1d6 are my choice. Rebuild the special levels and data files; old saves will not load.

## Batch 36: attribute points

Cave-dwellers start with 80 total attribute points instead of 75 (`init_attr()` call in `u_init.c`). Bard secondary attributes now favor Strength and Dexterity equally: the attribute weights (Str Int Wis Dex Con Cha) are 20/10/10/20/10/30, with Charisma still the top attribute (was 10/10/15/25/10/30, Dexterity second).

Quicksilver: if you are already fast or very fast when you wield it, you get the message "You speed up even more!" (`set_artifact_intrinsic()`).

Bards can advance Two-weapon combat to Basic (cap only; they start Unskilled).

Rogues start with Two-weapon combat at Basic (`skill_init()` in `weapon.c`).

## Batch 37: gauntlets of protection, gauntlets of haste, boots of shielding

| Item | Appearance | AC | MC | Prob | Notes |
|---|---|---|---|---|---|
| gauntlets of protection | "adamantine gauntlets" (fixed) | 2 | 3 | 1 | metal, wt 20, cost 200 |
| gauntlets of haste | "crystal chitin gauntlets" (shuffled with the other gloves) | 0 (enchantable) | 0 | 1 | grants the same Fast property as speed boots, with the same speed-up and slow-down messages; leather, wt 10, cost 150 |
| boots of shielding | "crystal boots" (fixed) | 2 | 0 | 2 | gemstone, wt 20, cost 100 |

Boots of shielding make you immune to bear traps (the trap snaps shut harmlessly), spear traps, spiked pits (you step across them; ordinary pits still work) and wounded legs (`set_wounded_legs()` returns early). Armor class probabilities were kept at 1000 by trimming low boots (17 to 15), high boots (13 to 12) and leather gloves (10 to 9). The new objects shift object numbers: rebuild the special levels and data files; old saves and bones will not load.

Interpretation notes: "Fast" is the speed-boots property (very fast, 5/3 speed), not the weaker intrinsic; the spiked pit is bypassed entirely rather than just its spikes being blunted.

## Batch 38: artifact changes

- Frost Brand is now a broadsword and Fire Brand a two-handed sword (`artilist.h`). Existing games with the old artifacts will not load anyway: the artifact count changed.
- Snickersnee is renamed **Kiku-Ichimonji** (`ART_KIKU_ICHIMONJI`); its damage bonus to all targets is now 2d6 instead of 1d8 (flat dice in `spec_dbon()`), to-hit bonus unchanged (d5).
- New **Claiomh Solais**: lawful falchion, not self-willed. Lights like Sunsword (`artifact_light()`, radius 2, only snuffed by unwielding), +d5 to hit and +d10 damage to everything, polymorph control while wielded (`SPFX_POLYC`).
- New **Ichigeki**: neutral dagger, +d3 to hit and +d6 damage to everything. Each hit has a 1 in 20 chance to strike a fatal weakness and kill outright ("Ichigeki hits <monster>'s fatal weakness!"); it does not work on shapeless (amorphous), unsolid or incorporeal monsters, puddings, jellies, blobs, the Riders, or while you are swallowed (`artifact_hit()`).
- `makesingular()` no longer strips the "s" from "Solais", so the artifact can be wished for.
- Adding artifacts changes the version check, so the dungeon and data files must be rebuilt (`make dungeon`, `dlb`); old saves and bones will not load.

## Batch 39: Narya and Nenya

Two unaligned artifact rings (`artilist.h`, helpers in `artifact.c`/`spell.c`).

- **Base ring.** Ring appearances are shuffled each game, so the base ring is picked after the shuffle by `fix_lotr_rings()` (called from `hack_artifacts()` and again after `restnames()` in `restgamestate()`). Narya: the ring that currently looks like a "ruby ring", unless that is polymorph, conflict, aggravate monster or hunger, then the "gold ring" with the same test, then a ring of fire resistance. Nenya: "diamond ring", then "platinum ring", then a ring of cold resistance. An appearance that no real ring currently has counts as unavailable. A new filler ring appearance, "platinum", was added so a platinum ring can exist (it may be unused in a given game).
- **Spell.** Wearing Narya teaches fireball if it is not already known; Nenya teaches cone of cold (`lotr_ring_on()`/`lotr_ring_off()` in `spell.c`, tracked in the new `u.uringspells`). The spell is forgotten again when the ring comes off, unless you knew it already. While the ring is worn its spell is always cast successfully (0% fail): `ring_spell_free()` skips the strength, encumbrance, confusion, spell-memory and failure-rate checks. Lack of Pw, stun, hands that are not free and being too hungry to cast (nutrition 10 or less, i.e. about to faint) still stop it. (This tree has no polymorph spellcasting restriction, so none is applied.)
- **Damage.** `lotr_ring_damage()` adds 1d10 to every `hmon_hitmon()` hit (melee, thrown, fired, unarmed) against a foe that is not fire resistant (Narya) or not cold resistant (Nenya).
- **Resistance.** Fire (Narya) or cold (Nenya) resistance through the artifact's defn field, skipped when the base ring is already fire or cold resistance.
- **Charisma bonus.** `lotr_cha_bonus()` adds your Charisma to the base spell success chance for all spells, once per ring worn (so +Cha each, +2xCha with both).
- **Generation.** Rings can become Narya or Nenya at creation with 1/25 odds when the ring type matches the current base ring; they can also be wished for.
- The artifact count, ring list and `struct u` changed: rebuild the data files (dungeon, special levels, `dlb`); old saves and bones will not load.

## Batch 40: Vilya

A third unaligned artifact ring. Base ring: the "sapphire ring", else the "brass ring" (if sapphire is polymorph, conflict, aggravate monster or hunger, or unused), else a ring of protection (`fix_lotr_rings()`).

While worn: shock and disintegration resistance (defn field), warning (spfx), magical breathing (`set_artifact_intrinsic()`, worn only), an AC bonus equal to the D&D 3rd edition Intelligence modifier, (Int - 10) / 2 rounded down and never negative (`vilya_ac_bonus()` in `find_ac()`, refreshed every turn while worn), and one extra HP of regeneration per turn on top of everything else (moveloop in `allmain.c`, also while polymorphed). Wearing it teaches healing if you do not know it (forgotten again when it comes off, bit 4 of `u.uringspells`), and healing, extra healing and superior healing (if you know them) are failure-free with the same restrictions as Narya and Nenya.

## Batch 41: Aegis

New lawful artifact, a dwarvish roundshield named **Aegis** (not self-willed, `SPFX_NOGEN` so it never generates randomly or as a sacrifice gift; wishable). Worn effects: reflection and petrification resistance (`set_artifact_intrinsic()`), shock resistance (defn), magic cancellation 3 on its own (`magic_negation()`), and a 1 in 3 chance for each shield bash to terrify the target (`shield_bash()` in `uhitm.c`; the bash itself still needs Basic shield skill and its usual odds). It is the crowning gift of a lawful Healer (`gcrownu()` in `pray.c`, if it does not already exist; also unrestricts the shield skill). `makesingular()` was taught that "Aegis" is not a plural so it can be wished for.

Aegis never causes a spellcasting penalty, like the crystal shield (`percent_success()` skips both the role shield penalty and the heavy-shield halving for it).

## Batch 42: Kusanagi

New lawful artifact **Kusanagi**, a broadsword (Samurai see it as a "ninja-to" through the existing Japanese item names), `SPFX_NOGEN` and not self-willed. It is the crowning gift of a Samurai (`gcrownu()`, if it does not already exist).

- +d10 damage against everything; double damage against snakes, nagas, dragons and anything with the slithy flag (`artifact_hit()`).
- Fire and poison resistance while wielded.
- Cuts through spider webs when you walk into one (`dotrap()`).
- `#invoke` (new `WIND_BLAST` power) acts like zapping a wand of wind, including in the self direction; it uses the normal artifact invoke cooldown.
Rebuild the data files; old saves and bones will not load.

## Batch 43: artifact dual-wielding

Two-weapon combat used to refuse any artifact in the off hand. `can_twoweapon()` (`wield.c`) now asks `artifact_offhand_ok(primary, secondary)` (`artifact.c`). Only artifact *weapons* are restricted; other artifacts are unaffected. An ordinary weapon in the off hand is always fine.

- Grimtooth: fine in the off hand, except second to Sting or Orcrist.
- Sting and Orcrist: fine in the off hand, except second to Grimtooth.
- Kusanagi: only second to Kiku-Ichimonji or Ichigeki.
- Ichigeki: only second to Kiku-Ichimonji or Kusanagi.
- Kiku-Ichimonji: only second to Ichigeki or Kusanagi.
- Spineseeker, Dragonbane, Demonbane, Werebane and Trollsbane (the Banes other than Sunsword): second to anything.
- Grayswandir (primary) accepts no artifact in the off hand.
- Every other artifact weapon refuses the off hand.

## Batch 44: Yoichi no Yumi, and Ogresmasher off hand

- Ogresmasher now also accepts being held second to anything (`artifact_offhand_ok()`), like the other Banes. Giantslayer is a sling and cannot be held in the off hand anyway.
- New lawful artifact **Yoichi no Yumi**, a yumi (the Samurai's "long bow"). It is not `SPFX_NOGEN`, so it can be generated randomly, but `mk_artifact()` never picks it as a sacrifice or crowning gift. Ya fired from it get +d10 to hit (`thitmonst()` in `dothrow.c`) and +d12 damage (`hmon_hitmon()` in `uhitm.c`); other ammo and melee are unaffected. `#invoke` (new `CREATE_YA` power) makes 2d12 ya with the same blessing and enchantment as the bow, using the normal artifact invoke cooldown.
- So that it can actually turn up, the yumi's random generation probability went from 0 to 2 (taken from the plain bow, 32 to 30); the yumi's other appearance ("long bow") is unchanged.
Rebuild the data files; old saves and bones will not load.

## Batch 45: special arrows, Wallet of Perseus, Yoichi gift

- **Yoichi no Yumi** may now be a lawful sacrifice gift (the exclusion added in Batch 44 is gone). It has no role, so it is never the guaranteed first gift. It is still not a crowning gift.
- **Lucky arrow** ("shimmery green arrow", bright green): d8/d8, rolls to hit twice and keeps the better roll (`thitmonst()`), generation probability 2, stacks of 2d6, cost 40. A Ranger has a 25% chance to start with a stack of them instead of the +2 arrows (`u_init.c`).
- **Burst arrow** ("shimmery blue arrow", cyan): d4/d4, when it hits it also does 1d4 damage to hostile monsters on the adjacent squares (not tame or peaceful ones, and never you; `arrow_burst()` in `dothrow.c`). Probability 1, stacks of 1d10, cost 30.
- **Sleep arrow** ("narrow arrow", magenta): d4/d4, 50% chance to put a monster that survives the hit (and does not resist sleep) to sleep for 1d4 more turns. Probability 1, stacks of 1d10, cost 30.
  The plain arrow's generation probability dropped from 47 to 43 to keep the weapon class at 1000. "Rare" and "very rare" both map to probability 1 as before.
- **The Wallet of Perseus** (as in SLASH'EM): an unaligned artifact bag of holding, base price 10000. Contents weigh 1/3 (uncursed), 1/6 (blessed) or 4x (cursed), instead of 1/2, 1/4 and 2x (`bag_contents_weight()` in `mkobj.c`, shared with the pickup code). It is never generated at random (only weapons, armor and rings can turn into artifacts); it can come as a sacrifice gift after the first gift, from wishing, or from bones. `makesingular()` learned that "Perseus" is not a plural.
Rebuild the data files; old saves and bones will not load.

## Batch 46: role skill and starting-gear changes

All in `u_init.c`; no data rebuild or save break (skill caps are looked up at start, but existing characters keep what they have).

- Barbarian and Ranger: two-weapon combat cap Basic to Skilled.
- Archeologist: spear, cap Expert (was restricted).
- Healer: clerical spells, cap Skilled (was restricted).
- Knight: axe and curved sword raised from Skilled to Expert. Two-handed sword, polearms and spear were already Expert.
- Priest: attack spells, cap Basic (was restricted).
- Dwarvish Knights start with a dwarvish bearded axe (+1, blessed) instead of the long sword, via the race substitution table (`inv_subs`). It is wielded and the skill is Basic from the start like any starting weapon.

## Batch 47: the Pirate

A new role after the Pirate of Chris's patch and dNetHack (firearms and the githyanki pirate left out). Rebuild everything (objects, monsters, artifacts, `dungeon`, `quest.dat` and the `.lev` files all changed); old saves and bones do not load.

**Role** (`role.c`, `u_init.c`, `attrib.c`, `charinit.c`): human, elf or orc, neutral or chaotic, menu accelerator `P` (the role sits after Priest in the table so `-p` still means Priest). Gods Nereus (lawful), Neptune (neutral), Triton (chaotic). Ranks Landlubber, Sailor, Cutthroat, Bosun, Second Mate, First Mate, Captain, Pirate Lord, Dread Pirate. Attributes: minimums 10/7/7/10/10/7 (Str Int Wis Dex Con Cha), 75 points, weights 22/15/10/22/20/11; 11 initial HP, d8 later; special spell cause fear (Intelligence). Intrinsics: swimming at XL1, speed at XL7, stealth at XL11. Pet: parrot or monkey. The saber is called a cutlass for Pirates (also accepted when wishing).
- Starting kit: a rustproof +0 saber (cutlass), 5-8 rustproof +0 knives (elven daggers for elves), an uncursed +1 leather jacket or +0 oilskin cloak (half each), +0 high boots, 2-4 cram rations (lembas for elves), 3-9 of bananas, oranges, jerky or grapefruits (one kind), 3 potions of booze, one random ring (a +1 to +3 enchantment if it is chargeable), a rustproof +0 pick-axe and an oilskin sack. They know the oilskin cloak and sack. No gold.
- Skills (merged skills keep the best old cap): Expert in dagger, knife, long sword, curved sword, mace, flail, spear, crossbow and bare hands; Skilled in axe, pick-axe, dart, whip, escape spells and two-weapon combat; Basic in short sword, unicorn horn and the attack, divination and enchantment spells. They start at Basic in curved sword and knife.
- Crowning: always "the Pirate King", and the gift is Reaver. A wielded scimitar, falchion, messer, saber or silver saber turns into Reaver; otherwise a blessed rustproof +1 Reaver lands at your feet. A dead Pirate's bones rise as a skeletal pirate instead of a ghost.

**Artifacts** (`artilist.h`, `artifact.c`):
- **Reaver**: saber, +d5 to hit and +2d8 damage against everything (about level with Kiku-Ichimonji, whose katana base is better). A Pirate who hits snatches a random unworn, unwielded item from the target one time in five (one in ten for anyone else); cockatrice corpses are skipped unless you wear gloves, and a full pack drops the item at your feet. Only a Pirate can hold it in the off hand. Crowning gift only.
- **The Marauder's Map**: a scroll of magic mapping that is never used up. Reading it works like the clairvoyance spell; `#invoke` detects objects and names every artifact on the level. While a Pirate carries it, objects are four times as likely to be generated as artifacts (`artifact_chance()` in `mkobj.c`). It is the only sacrifice gift a Pirate can get, and Pirates are the only ones who can get it.
- **The Treasury of Proteus** (quest artifact, unaligned in effect, self-willed): a chest weighing 50, always unlocked and empty when made. Carrying it gives magic resistance and Luck. Its contents weigh a third (a sixth if blessed, four times if cursed), like the Wallet of Perseus. It absorbs curses meant for what you carry ("The Treasury of Proteus has just blocked a curse."): always while it is not cursed and 19 times in 20 otherwise, turning a blessed Treasury uncursed and an uncursed one cursed. `#invoke` lets you choose one non-artifact item inside and the sea changes it (a polymorph, like polypiling), with the usual artifact invoke delay. There is no random polymorph timer. King of the hill: while you carry it, one random monster in five is a skeletal pirate (a damned pirate in Gehennom or the endgame); they are hostile and fight soldiers and sergeants.
- Wizard mode: `^V` then `?` lists the quest levels.

**Monsters** (`monst.c`): parrot (B, pet or random), skeletal pirate (Z, level 5, 1d6 weapon, never random), damned pirate (&, level 10, 2d6, fire resistant, hates silver as a demon), the player-monster pirate, pirate brother (quest guardian), Mayor Cummerbund (leader, level 20, 2d8 x2, MR 70) and Blackbeard's Ghost (nemesis, level 15, flies, walks through walls, steals the quest artifact). Skeletal and damned pirates carry cursed, damaged scimitar-class blades and armor (`makemon.c`). `pirate_blade()` picks a scimitar, falchion, messer, saber or silver saber; the quest levels' "scimitars" go through it (`sp_lev.c`).

**Quest** (`dat/Pirate.des`, `dat/quest.txt`): Tortuga (the harbor, noteleport, with potion, two weapon and tool shops, a brothel, the Mayor and his brothers in the central cave, and a fort whose keep holds the down stairs), a flooded filler, the shoreline of Shipwreck Island (a wrecked ship with Captain Ketch and soldiers, a secret door at the stern into a cavern with the down stairs, sharks around), a dark cavern filler, and the caldera where Blackbeard's Ghost sits on the Treasury. Random monsters on the quest levels are 55% skeletal pirates, 14% r, 14% soldiers, 3% E, 14% anything else. `Pirate.des` was generated by `util/genpir.py` (run it with the output path), so the town, the cave and the caldera are fixed layouts. Pirates swim, so the water is only a nuisance (wet scrolls and potions; an oilskin sack helps).

Not done: pirate-speak renames other than the cutlass (rum, sea biscuits, ditty bags), skeletal pirates freezing you when chatted to, pirates targeting a carrier of the Treasury other than you, the githyanki pirate, and sea monsters in Tortuga's harbor.

## Batch 48: Grayswandir nerf

Grayswandir no longer doubles the damage of every hit. It now gives +d5 to hit and +2d8 damage against everything (`spec_dbon()`, `PHYS(5,16)` in `artilist.h`), a mean of 20.5 at +7 against small monsters instead of 23, and 13.5 at +0 instead of 9. The two cross at +4.5; above that the old doubling was stronger. Against small monsters it is now exactly 1.0 per hit above Kiku-Ichimonji at any enchantment. The silver bonus against silver-haters is unchanged. Only the binary changes; saves and bones still load.

## Batch 49: Luck Blade deep strike

The Luck Blade's deep strike now happens with chance 1 in (14 - Luck) per hit (it was 1 in 20 - Luck), so it fires on every hit at the maximum Luck of 13, and it adds 2d8 damage instead of 3d8. Against small monsters at +7 (14.5 before the strike: d6 + 7 + the flat +4) the mean is now 15.1 at Luck 0, 15.3 at Luck 3, 16.0 at Luck 8, 16.8 at Luck 10, 19.0 at Luck 12 and **23.5** at Luck 13; it used to be 15.2, 15.3, 15.6, 15.8, - and 16.4 at Luck 13. Normal-Luck play is therefore unchanged (still the weakest of the named blades), while at maximum Luck it is 3.0 above Grayswandir's +2d8 (20.5), which in turn gets its silver bonus against silver-haters. Only the binary changes; saves and bones still load.

## Batch 50: Pernicimordax

Pernicimordax's cold damage is now 4d4 (mean 10) instead of 3d6 (mean 10.5), against anything that is not cold resistant (`spec_dbon()`, `COLD(5,4)` in `artilist.h`). It is a little more consistent (spread 4 to 16 rather than 3 to 18) and still drops to a plain +1 falchion against cold-resistant monsters. Against small monsters at +7 the mean is 22.5 (was 23.0), or 12.5 against cold-resistant ones. Only the binary changes; saves and bones still load.

## Batch 51: no artifact partners for Frost Brand and Pernicimordax

Like Grayswandir, Frost Brand and Pernicimordax now refuse to be wielded together with any other artifact weapon when two-weaponing (`artifact_offhand_ok()`): they accept no artifact in the off hand, and they are not on anyone's accepted list for the off hand, so they cannot be the off-hand weapon either. An ordinary weapon in the other hand is still fine. (Fire Brand is two-handed, so the question does not arise.) Only the binary changes; saves and bones still load.

## Batch 52: Sunsword and Werebane bonuses against their targets

Sunsword's +d5 to hit and +d4 damage against undead, and Werebane's against lycanthropes, never applied: both artifacts name a pseudo class (`S_UNDEAD`, `S_WERE`) that exists only for the warning code, and `spec_applies()` compared it to the real monster class. It now treats `S_UNDEAD` as "has `M2_UNDEAD`" and `S_WERE` as "has `M2_WERE`" (or a lycanthropic hero). Sunsword's bonus is worth about +2.5 damage and a to-hit bonus against undead: a +7 Sunsword now does about 17.5 to a small undead monster that is not fire resistant (15.0 against non-undead that is not fire resistant); Werebane gains the same sort of bonus against lycanthropes. The warnings are unchanged. Only the binary changes; saves and bones still load.

## Batch 53: more lycanthropes

Seven new shapeshifters, each a base form that changes into a beast form like the werewolf (`monst.c`, `were.c`). Each pair has the same level; the beast's bite gives you that lycanthropy. Rebuild everything (the monster list changed, so `lev_comp`, the `.lev` files and `quest.dat` too); old saves and bones do not load.

| Pair | Level | Human-form weapon | Beast form |
|---|---|---|---|
| werelynx (f) | 7 | 2d4 | speed 15, AC 6, claws 1d6 x2, bite 1d10 (lycanthropy) |
| werepanther (f) | 9 | 2d4 | speed 15, AC 6, claws 1d8 x2, bite 1d10 |
| weretiger (f) | 11 | 2d6 | speed 12, AC 5, claws 2d4 x2, bite 2d6 |
| werebear (Y) | 12 | 2d6 | speed 12, AC 4, claws 1d8 x2, bite 2d6 |
| wereshark (;) | 13 | 2d6 | speed 12, AC 2, bite 4d6 |
| wereraven (B) | 6 | 2d4 | speed 20, flies, bite 1d6 (lycanthropy), claw 1d6 blinding |

- All are chaotic, poison resistant, regenerate, hostile, not polymorphable; the human forms (@) are normal random monsters whose corpses give lycanthropy, the beast forms are never random. They summon help like the others: lynxes, panthers (and a lynx), jaguars and tigers, owlbears and yetis, ravens; the wereshark summons nothing.
- **Wereshark**: only changes into a shark in the water (`new_were()`), in the sea or any pool, and a hero with wereshark lycanthropy only changes while standing in water (`you_were()`), since a shark stranded on land withers away (as other eels do).
- **Weredemon** (&, level 14, Gehennom only; `G_HELL`): the demon form has a weapon 2d8, claw 1d8 and a fire breath 3d6, resists fire and poison, hates silver like any demon. It changes into a "weredemon hound" (a hell hound: bite 3d6, fire breath 3d6, vulnerable to cold), which is shown as "changes into a hell hound". Neither form inflicts lycanthropy (the bite is plain physical), and the demon form does not summon demons (it summons hell hound pups and hell hounds like other weres). Dying in hound form leaves the demon form as the "true form".
- New macros `is_were_base()` and `is_were_beast()` (`mondata.h`) replace the old checks on `S_HUMAN`/`is_human()` in the lycanthrope code (`were.c`, `mhitu.c`, `mhitm.c`, `mon.c`, `potion.c`, `zap.c`), since the weredemon's base form is not human.
- Wizard mode: "human werebear", "human werewolf" and so on now create the human forms (`name_to_mon()`).
- Werebane now has real targets at the top end (its +d5 to hit and +d4 damage applies to all of them).

## Batch 54: weresharks in the Pirate quest

Two weresharks (in shark form) now lurk in the water of the flooded filler level and two more in the shoreline of Shipwreck Island, besides that level's six sharks (`dat/Pirate.des`, regenerated with `util/genpir.py`; the layouts are unchanged). Out of the water they take human form; in the water they change into sharks and back as usual. Needs the data files rebuilt.

## Batch 55: Mephistopheles is a demon prince

Mephistopheles is now an `M2_PRINCE` instead of an `M2_LORD`, so `is_dprince()` is true for him and `is_dlord()` is not. The consequences: when he gates in demons he uses the prince odds (1 in 20 a prince, 1 in 4 a lord, otherwise an ordinary demon, instead of 1 in 50 and 1 in 20), his missile attacks get the prince multishot (+2 rather than +1), his spells call up more demons, and gear he is given is at least +1. He is exempt from the rule that demon princes start peaceful, invisible and open to bribery (`makemon.c`), so he is still hostile, visible and asleep in the Sanctum as before. Only the binary changes; saves and bones still load.

## Batch 56: Mephistopheles slows down

Mephistopheles's speed goes from 15 to 12 (`monst.c`), the same as an unhurried hero, so a hero with intrinsic speed, speed boots or Quicksilver can outrun him and fight hit-and-run. (He was still covetous then; see Batch 72.) Only the binary changes; saves and bones still load. (Batch 26's "speed 15" line is out of date.)

## Batch 57: Quicksilver and the Luck Blade pair up

`artifact_offhand_ok()` now accepts Quicksilver in the off hand with the Luck Blade as the main weapon and the other way round, so the two can be two-weaponed together (each still refuses every other artifact partner). Both are short swords, so the usual two-weapon skill rules apply, and a Pirate, Rogue or anyone else who can two-weapon with a short sword gets speed 24 from Quicksilver and the Luck Blade's luck and deep strike in one go. Only the binary changes; saves and bones still load.

## Batch 58: Vecna's Domain, the Sunless Sea, the Spider Caves, the Giant Caverns, deep ones and hounds of Tindalos

Four new branches (`dat/dungeon.def`) and their levels (`dat/vecna.des`, `dat/caves.des`, generated by `util/gendomains.py`; the town, cave and fort layouts are fixed, only monster and object placement within them is by the script's seed). Rebuild everything (`dungeon`, the `.lev` files, `nhdat`; `Makefile.dat`/`Makefile.top` list the new files); old saves and bones do not load. The depths follow SLASH'EM's dungeon depths relative to Gehennom (the castle is level 25 to 29, so the Valley is 26 to 30):

| Branch | Entry | Levels |
|---|---|---|
| Vecna's Domain | magic portal on the level below the Valley | 3 |
| The Spider Caves | stairs on Gehennom level 2 to 4 | 1 |
| The Sunless Sea | stairs on Gehennom level 3 to 5 | 1 |
| The Giant Caverns | stairs on Gehennom level 6 to 9 | 1 |

**Vecna's Domain** (after EvilHack; no teleporting, no digging, mostly dark):
1. The withered forest: dead trees, six graveyards, the portal in the middle and four possible down stairs, three of which are giant mimics; liches and demiliches, vampires, zombies, mummies, ghosts, traps.
2. The lava stronghold: marilith cells with bars at the west landing, a courtyard of vampires, zombies, mummies, ghosts, revenants and beholders, two master liches in each of the north and south sections, a drawbridge across the lava (salamanders in it) and a marilith guarding the causeway to Kas's chamber. Kas stands on the down stairs with a weredemon, a balrog, two demiliches, five vampire lords and a vampire mage, beside an iron safe and a chest.
3. Vecna's lair: the up stairs in the southeast, a lava river with a drawbridge (and a secret door where the river ends in the north), an alhoon's room, a store room, an entrance hall with a shambling horror, two skeletons and an elder minotaur, and the throne room with Vecna on the throne, four liches and an arch-lich around him and a chest of treasure. Twelve revenants, liches, demiliches, a beholder, four hounds of Tindalos, demons, zombies, a corpse of every player role but the Monk (and the Cavewoman/Priestess variants) and so on in the rest.
- New monsters: alhoon (h, level 26, speed 9, AC -6: weapon, 5d6 cold touch, two int-draining tentacles, mage spells), revenant (Z, level 13, level-draining touch), beholder (e, level 8: gazes that slow, sleep, stone and stun, a 8d8 bite), elder minotaur (H, level 26, speed 14, claws 6d6 x2, hug 6d4, butt 8d4), Vecna (L, level 46 but capped at 50 like other great demons: 8d6 cold touch, 6d6 level-draining touch, mage and clerical spells; always carries a cursed rotted robe, a ring of slow digestion and the Hand or the Eye of Vecna, which drops when he dies) and Kas (V, level 26, speed 16, two weapon attacks 4d4 and a level-draining bite; carries the Sword of Kas, plate mail, helmet, gloves, iron shoes and a ring of slow digestion). The vampire mage, previously disabled, is now a real monster but is never randomly generated.
- New artifacts: the **Sword of Kas** (chaotic two-handed sword, +d10 to hit, double damage against anything that is not chaotic and against liches, triple against Vecna, poisoned edge worth +d6 against anything not poison resistant, Strength 25 and stoning resistance while wielded; any alignment can wield it, a lawful hero loses alignment on each hit), the **Hand of Vecna** (leather gloves: regeneration, cold resistance, drain resistance) and the **Eye of Vecna** (lenses: telepathy, searching, hallucination and blindness resistance), all chaotic and never generated otherwise. The wiki gives no stats for the last two, so these are my own.
- Not done: Vecna keeping liches from being genocided, his death gaze and warp, the sword's bloodthirst and the priest penalty.

**The Sunless Sea** (after SLASH'EM): a flooded level with the up stairs on the west shore, a central island with a chest, two wands, two amulets, two rings and three gems, a beach with four items and five giant crabs (new: s, level 5, swims), eight sharks and seven other sea monsters, an island with a burned engraving and an item, and a magic lamp lying in the water. Sunken-city flavour from dNetHack: a deepest one, four deeper ones and twelve deep ones.
**The Spider Caves** (after SLASH'EM): an unlit cave with 22 giant spiders, 10 other arachnids, Shelob (new: level 20, two poisonous bites and a claw) and Girtab (new: level 22, claws 5d4 x2 and a 3d8 sting), two werespiders and a human werespider (new, see below), 48 webs, 48 spider eggs, five piles of gold, two potions of full healing (instead of invulnerability), a wand of make invisible, an amulet versus poison, speed boots, a ring of conflict and a wand of fire (instead of death). The potion is called "full healing" in this game.
**The Giant Caverns** (after SLASH'EM): a fortress of rooms with the up stairs in the west, a throne hall with **Hrungnir** on the throne (new: a unique stone giant, level 32, two 4d10 weapon attacks and a kick, stone, fire and poison resistant, throws boulders; much stronger than the Largest Giant of SLASH'EM it replaces), 20 giants (hill, fire, frost, storm, stone and five jotunn), seven more random H, eight O, four T, ten items and four traps, and a chest of gems and amulets behind a secret door. The **jotunn** (H, level 14, 3d10 weapon, cold and poison resistant, throws boulders) is a new random giant between the frost and storm giants.
**Werespiders** (s and @, level 9, bite 2d6 lycanthropy plus a poisonous bite; summon cave and giant spiders): the sixth lycanthrope family.

**Deep ones and hounds of Tindalos** (after dNetHack): **deep one** (h, level 7, speed 9, two 1d6 weapon attacks and a 2d4 kick, swims, cold and sleep resistant, corpse conveys sleep resistance, small groups, never in Gehennom), **deeper one** (h, level 15, 3d4 x2 and a 4d6 kick, comes with 3 to 12 deep ones) and **deepest one** (H, level 30, speed 15, 3d8 x2 and a 5d6 kick, comes with 10 to 19 deep ones and 3 to 12 deeper ones). They share a soul: when one dies every other one on the level gains hit points (2, 4 or 8 by what died, and a level for about every eight gained). The **hound of Tindalos** (d, level 14, speed 12, never random): a 2d6 reaching touch that heals it by half the damage, a 4d4 tentacle and mage spells; it appears only where I put it, four of them in Vecna's lair.

**Other changes**: `short_dgn_names` in `botl.c` now falls back to the dungeon's own name, so the status line shows "Vecna's Domain" and so on instead of a wrong name; the new dungeons are defined at the end of `dungeon.def` so existing dungeon numbers do not change; wizard-mode `^V ?` lists all the new levels.

## Batch 59: rare hounds of Tindalos, rutterkin, redcaps and rabites

- **Hound of Tindalos**: now randomly generated, but only in Gehennom, in small packs and very rarely (frequency 1, `G_HELL|G_SGROUP`). They are still placed in Vecna's lair.
- **Rutterkin** (i, after Hack'EM): level 5, speed 12, AC 2, MR 20, chaotic; one 1d4 claw whose poison drains Strength; regenerates, wanders, follows you between levels, hates silver (like every imp-class monster here). Frequency 1.
- **Redcap** (i, after Hack'EM): level 10, speed 15, AC 2, MR 30, lawful (so lawful heroes may find them peaceful); bite 1d2, weapon 2d4, claw 1d4; poison resistant (the corpse conveys it), amphibious, tunnels with a pick-axe, goes berserk when hurt. Frequency 1. There is no scythe in this game, so redcaps carry a voulge (three in four) and a pick-axe (half the time) instead.
- **Rabite** and **silktail** (r, Hack'EM's rabbit and rabid rabbit under other names): the rabite is level 0, speed 15, AC 7, bite 1d3, herbivorous, frequency 2; the silktail is level 3, speed 18, AC 7, bite 2d6 that drains Constitution, poisonous to eat, frequency 1. Neither is generated in Gehennom, and neither breeds. Only the binary and data files change; old saves and bones do not load.

## Batch 60: the gem dragons

Six new dragons after the chromatic ones, each with a baby, scales and scale mail. All adults are level 17 (the chromatic ones are 15), speed 9, AC -2, MR 25, fly, see invisible, lay eggs and hoard jewels; bite 3d8 and two 1d4 claws like the others. They are generated about a third as often as a chromatic dragon, and (being deeper) not before the difficulty reaches their level. Each adult carries a hoard of its own stone (3 to 6 of them) plus 2 to 4 random gems; a baby carries one or two. The hoard drops when it dies. Scales drop 1 time in 3, as for other dragons; enchanting scales turns them into scale mail, and a polymorph while wearing either makes you the dragon.

| Dragon | Colour | Breath | Scales / scale mail | Resists |
|---|---|---|---|---|
| Emerald | bright green | howling gale, 4d6 | magical breathing | poison, swims |
| Topaz | brown | ray of stilled time, 4d6 | free action | sleep |
| Sapphire | bright blue | psychic blast, 4d6 | stoning resistance | stoning |
| Ruby | bright magenta | hellfire, 6d6 | regeneration | fire (corpse conveys it), hurt by cold, infravisible |
| Crystal | cyan | dazzling light, 4d6 | half physical damage | shock |
| Amethyst | magenta | blast of pure force, 6d6 | half spell damage | sleep |

The shimmering dragon is still switched off, so the crystal dragon takes cyan; if shimmering dragons are ever enabled they will need another colour. Topaz is brown because every bright yellow is taken. Ruby and amethyst differ only in brightness.

**The breaths** are not `buzz()` rays: they are straight beams drawn with the existing zap glyphs, and they do not bounce off walls. All new code is `gem_beam()` in `mthrowu.c`, driven by new attack types `AD_WIND`, `AD_TIME`, `AD_PSYC`, `AD_HELL`, `AD_PRSM` and `AD_FORC`. The random-breath Chromatic Dragon is unchanged, and the chromatic dragons still use `buzz()`.
- **Wind** (emerald): stuns you for 1 to 4 turns and hurls you one or two squares back (not in Sokoban, not when levitating, flying, riding, stuck or chained). Half physical damage halves the damage. Cannot be reflected.
- **Slowing** (topaz): you are slowed for 15 to 29 turns; a fast hero loses a third of their movement instead of nothing (new rule in `allmain.c`); free action turns it away. Slows monsters. Reflectable.
- **Psychic** (sapphire): stuns and confuses; magic resistance halves the damage and blocks both; half spell damage halves it again. Mindless monsters are immune. Cannot be reflected.
- **Hellfire** (ruby): fire damage that still does half to the fire resistant (it cannot be resisted completely); burns armor, scrolls, potions and spellbooks like fire. Reflectable.
- **Light** (crystal): half damage, but double to undead, gremlins, vampires and so on, and it blinds for 1d12 per die unless you resist blindness. Reflectable.
- **Force** (amethyst): magic resistance halves it, half spell damage halves it again (so amethyst scale mail protects against amethyst breath). Cannot be reflected.
If you polymorph into a gem dragon you breathe the same beams with #monster.

New in the code: `is_gem_dragon()` in `mondata.h`, the six `AD_` codes in `monattk.h`, the dragon, scale and scale-mail entries (kept in the same order as before, after yellow, so every range check was widened to end at amethyst), the egg mappings, the hoard in `m_initinv`, and the one-in-three reroll in `rndmonst`. Old saves and bones do not load.

## Batch 61: Keolewa hits harder

Keolewa's bonus damage goes from d6 to d10 against every monster (`PHYS(5,10)` in `artilist.h`; the to-hit bonus stays d5). Against small and medium monsters it now averages 3.5 + 5.5 = 9.0 before the Caveman's own bonuses (large: 2.0 + 5.5 = 7.5), level with Grayswandir's 2d8. Only the binary changes.

## Batch 62: Plague

**Plague** (after SLASH'EM): a chaotic orcish bow, randomly generated, never a gift. Arrows fired from it get +d5 to hit (`dothrow.c`) and +d6 damage (`uhitm.c`), and become poisoned as they leave the string if they are not already (`dothrow.c`; only the arrows you fire, not the rest of the stack). Wielding it confers sickness resistance, through a new `SPDF_SICK` flag that `defends(AD_DISE, ...)` reports (`Sick_resistance` already asked it). The usual rules for poisoned weapons apply: Samurai and lawful heroes are penalised for using it, and the poison wears off now and then. A hero who is not chaotic risks being blasted when handling it, like any cross-aligned restricted artifact. Only the binary changes.

## Batch 63: Heartseeker

**Heartseeker**: a lawful elven bow that is not self-willed, randomly generated, never a gift. Arrows fired from it get +5 to hit (`dothrow.c`) and +d8 damage (`uhitm.c`). They also count as **blessed and silver** against whatever they hit, while staying the kind of arrow they are: they do their own damage, and on top of it the blessed bonus (d4 against undead and demons, +2 to hit them) and the silver bonus (d20 against silver-haters, with the "sears" message; shades can be hurt by them). This is `heartseeker_shot()` in `weapon.c`, consulted by `hitval()` and `dmgval()`; it only applies to arrows the hero has just fired from the bow, not to arrows monsters shoot. **Actual silver arrows** fired from it also have an even chance of one extra shot, on top of the normal volley and skill bonuses. I took "when firing silver arrows" to mean real silver arrows, since every arrow it fires already counts as silver for damage; if you meant every arrow, it is a one-line change in `dothrow.c`. Only the binary changes.

## Batch 64: Plague hits harder

Plague's arrows now get +5 to hit (was +d5) and +d8 damage (was +d6), the same as Heartseeker. With the poison (+d6 on 9 hits in 10 against anything that isn't poison resistant) an elven arrow averages 4.0 + 4.5 + 3.15 = 11.65 against small and medium targets, or 8.5 against poison-resistant ones, level with Heartseeker's 8.5 there. Only the binary changes.

## Batch 65: Mirrorbright

**Mirrorbright**: a neutral shield of reflection, randomly generated, never a gift. Being a shield of reflection it reflects; as an artifact it also gives hallucination resistance while worn (the same `SPFX_HALRES` as the Eye of Vecna), does not add the usual spellcasting penalty for wearing a shield (`spell.c`, as for Aegis and crystal shields), and acts as a weak light source: it glows with radius 1 while worn ("begins to glow softly" / "stops glowing"), compared with 2 for Sunsword and 3 for a lamp. The light is `artifact_light()` plus `Shield_on()` and `Shield_off()`. Only the binary changes.

Mirrorbright also has one point better base armor class than a normal shield of reflection (base AC 3 instead of 2, before enchantment), through `ARM_BONUS` in `hack.h`.

## Batch 66: Frostbite

**Frostbite**: a neutral crossbow, randomly generated, never a gift. Wielding it confers cold resistance (`SPDF_COLD`). Bolts fired from it get +5 to hit (`dothrow.c`) and +d8 damage (`uhitm.c`), and a bolt that hits adds 2d6 cold damage to its target (nothing if the target resists cold) and sends 2d6 of frost into every hostile, non-tame monster next to the target that does not resist cold ("The bolt bursts with frost!", "X is chilled!"). Pets and peaceful monsters are spared, as with burst arrows, and you are not hurt since wielding the crossbow makes you cold resistant. `frost_burst()` in `dothrow.c` and the `frostbite_shot()` macro in `obj.h`. An ordinary bolt does d4+1 against small and medium monsters (3.5 on average; d6+1, 4.5, against large ones), so a hit averages 3.5 + 4.5 + 7 = 15 against a small or medium target (16 against a large one), 7 less if it resists cold, and 7 on each hostile monster beside it. Only the binary changes.

## Batch 67: Durandal

**Durandal**: a lawful broadsword. +d5 to hit and +d10 damage against anything that is not lawful (the same `SPFX_DALIGN` test as the other cross-aligned artifacts: neutral, chaotic and unaligned monsters). It is the **first sacrifice gift of a lawful Knight**: Excalibur and the Magic Mirror of Merlin are never gifts, and `mk_artifact()` hands a hero their role's artifact first, so Durandal is the first gift as long as it does not exist yet. Other lawful heroes can be given it too, like any other role artifact, and it can turn up as a random broadsword. In the off hand it is accepted **only beside Excalibur** (`artifact_offhand_ok()`); Excalibur itself still refuses the off hand, so the pair works only as Excalibur first and Durandal second. It is deliberately weaker than Kiku-Ichimonji: against a non-lawful target it averages the broadsword's 2d4 (5.0 against small and medium monsters; d6+1, 4.5, against large ones) plus 5.5, so 10.5 or 10.0, and nothing extra against lawful ones. Only the binary changes.

## Batch 68: Arcanos Guard

**Arcanos Guard**: a neutral crystal plate mail and the **first sacrifice gift of a Healer** (the Staff of Aesculapius is never a gift, so the role-first rule in `mk_artifact()` picks this); it can also be a random find, and other neutral heroes can be given it later like any role artifact. While worn:
- **Energy regeneration** (`SPFX_EREGEN`).
- **Easier spellcasting**: four points off the caster's spellcasting penalty (`spell.c`). In a Healer's spell menu extra healing went from 62% to 50% failure and stone to flesh from 71% to 59%.
- **#invoke**: a burst of arcane force around you (`arcane_burst()` in `artifact.c`). Every hostile monster next to you takes 4d8 force damage (half if it resists magic) and, unless it resists, is stunned and flees for a few turns. Pets and peaceful monsters are left alone, and nothing happens (and the cooldown is not used) if no hostile monster is adjacent. It uses the usual artifact cooldown (a random wait with a median of about 70 turns). In a test four jackals around the hero all died.
Crystal plate mail is not metallic, so it has no armor spellcasting penalty of its own. Only the binary changes.

## Batch 69: gloves and helm switch fixes

Wearing gloves of expertise printed "Unknown type of gloves" because `Gloves_on()` and `Gloves_off()` had no case for them; the same was true of the helm of caution in `Helmet_on()` and `Helmet_off()`. Both are now handled (neither needs any effect of its own: expertise works through the skill code, caution through its warning property). The bow to-hit penalty switch in `thitmonst()` (`dothrow.c`) also did not know the newer gloves: gauntlets of protection now count as metal like gauntlets of power (-2 to hit with bows), and gauntlets of haste and gloves of expertise have no penalty. Wishing for, wearing and removing every new glove and helm was checked in wizard mode.

## Batch 70: audit of the new items

Every new wearable (helms, boots, shields, bracers, amulets, gnomish suit, all dragon scales and scale mail, crystal plate mail), every new weapon and staff, the three new wands, the scroll of ice, the new food and potion, the new containers and the new spells were wished for, used and checked against the panic log in wizard mode. Found and fixed:
- **Scroll of ice** never made its frost blast: it passed explosion type 13 (a sleep ray) instead of 12 (cold) to `explode()`, which logged "explosion base type 13?" and returned (`read.c`).
- **Wand of stinking fog** (and the spell of stench) zapped at yourself hit "object 490 used?" in `zapyourself()`; it now puts a cloud on your own square (`zap.c`).
- **Wishing for the wand of stasis** gave a random wand, because the plural-stripping in `makesingular()` turned "stasis" into "stasi" (`objnam.c`). "Kas" and "Tindalos" are also protected from it now.
- **Wishing for the Sword of Kas, the Hand of Vecna or the Eye of Vecna** by name gave a random sword, glove or lens, because "of Kas"/"of Vecna" was read as "figurine of a Kas". A name that is an artifact's name is no longer split that way (`objnam.c`).
Not exercised: spellbook casting for the high-level new spells (a first-level Wizard fails them every time; each has its case in `spelleffects()`), and the dwarvish bearded axe's apply and the bag of poo beyond opening them.

## Batch 71: merozalghi are vulnerable to cold

The merozalghu now has `M4_VULNERABLE_COLD`: cold damage against it is multiplied by 1.5 through `damage_mon()`, the same as for the other cold-vulnerable monsters (Pernicimordax's d(3,6) cold and double damage against merozalghi already existed and now stack with it). Data files must be rebuilt (`monst.c` changed).

## Batch 72: Mephistopheles is no longer covetous

`M3_COVETOUS` is removed from Mephistopheles (`monst.c`), so he no longer teleports to you, steals the Amulet or the other covetous targets, or teleports away to heal (`tactics()` is never used for him). He still waits in the Sanctum until you take the Amulet, follows you between levels and walks up to you like any other stalker, is still a demon prince who gates in demons and fights the Wizard of Yendor, and still cannot be tamed (he is a demon). Together with his speed of 12 this makes him a monster you must outfight or outrun on foot. Data files must be rebuilt.

## Batch 73: glue golem crash

Destroying a glue golem crashed the game two times in three. `make_corpse()` (`mon.c`) only makes the tainted horse corpse one time in three; the other times it fell out of the switch with no object and went on to use the null pointer (`bypass_obj()`, the invisibility and blindness code, `stackobj()`). It now returns at once when there is nothing to leave (`mon.c`), which also covers any other special case that leaves nothing. Eight glue golems were destroyed in a row with Luck Blade wielded in wizard mode with no crash; the weapon has nothing to do with it.

## Batch 74: corpse stress test

`util/stress_test.py` (first written as `util/corpse_stress.py`) is a regression test for death handling, written after the glue golem crash (Batch 73). `src/allmain.c` has a function, compiled only when `CORPSE_STRESS_TEST` is defined, that creates and kills every ordinary monster type repeatedly in wizard mode (12 times through `mondied()`, with every fifth one a revived monster, and twice through `xkilled()`; uniques, quest leaders and nemeses, guardians, priests, shopkeepers and guards are skipped), logging each monster to `ct.log` before killing it. The script copies an already built tree, defines the macro, builds, runs the game on a pseudo-terminal, dismisses every `--More--`, and reports the monster being killed if the game dies (and anything written to the panic log). Run it as `python3 util/stress_test.py ~/jadehack-play/build --test corpse` after a sync and full build; it takes about three minutes and exits 0 only if everything survived. Checked both ways: it passes on the current tree (about 5,900 kills) and, with the Batch 73 fix removed, reports a segmentation fault at the first glue golem. The normal build is unaffected.

## Batch 75: item stress test

The same script now has a second test, `--test items` (and `--test all`, the default, runs both). `src/allmain.c` gets a second function compiled only when `ITEM_STRESS_TEST` is defined, triggered by a file named `itemtest` in the wizard-mode playground, which writes `it.log`:
- every named kind of object is made 16 times in different states (blessed, cursed, uncursed, enchanted, eroded, erodeproof, greased, identified or not, stacked, with an object property), and each is named several ways (`xname`, `doname`, plural, singular, `distant_name`), weighed, put in a container with other objects when it is one, placed on the floor and stacked, put into your inventory and taken out again, and discarded;
- a corpse, statue, figurine, egg and tin of every monster type, and `mkcorpstat()` for every monster;
- every artifact, four times;
- 1,500 random objects of every class (and of all classes).
The log notes each step before it is done and marks any step after which the panic log grew, so the script can name the object that caused an `impossible()` as well as the one that crashed the game. A full run (both tests) takes about six minutes and made about 26,000 log lines; it passes on the current tree, and a deliberately planted crash in `xname()` for the wand of stasis was reported as "GAME DIED (signal 11) at: object 489 stasis 0". Note that it only creates and handles objects: wearing, zapping, applying and reading them is not covered. The normal build is unaffected.

## Batch 76: item use stress test

The stress test script has a third test, `--test use` (also part of `--test all`), written to catch the kind of bug found by hand in Batches 69 and 70 (an unknown glove type when worn, a wand that logged "object used?" when zapped at yourself). `src/allmain.c` gets a function compiled only when `USE_STRESS_TEST` is defined, triggered by a file named `usetest` in the wizard-mode playground, which writes `ut.log`. It uses every kind of object through the real commands, each in nine states (uncursed, blessed and cursed, each normal, confused, and blind and hallucinating):
- armor, rings and amulets: worn (`dowear`, `doputon`, with the delay afterwards finished), then the hero fights a monster while wearing it (attacking it and being attacked by it, and `^X`-style enlightenment, armor class and eight rounds of timeouts), then taken off;
- weapons: wielded and fought with, applied in four directions, thrown; launchers are fired (`dofire`) with every kind of ammunition that suits them;
- wands: zapped at yourself, east, up and down, engraved with, and broken (`do_break_wand`);
- scrolls, spellbooks, potions, food and tools: read (with the learning delay finished), the spell cast in four directions and at yourself, quaffed and thrown, eaten, applied in four directions (blindfolds, towels and lenses also put on);
- every artifact is invoked; lit lamps, rotting corpses, eggs and figurines get 2,000 turns of object timers;
- 150 random objects of each usable class, the corpse, egg and tin of every monster eaten (three states each), and a wish for every object and artifact by name in four forms (plain, "blessed +2", "cursed -1", and plural "3 ..."), logging a MISMATCH line whenever a wish gives a different object than the one named.
The hero and the level are put back after each step. Prompts are answered by the game itself: in the tty window port, `tty_nhgetch()` returns space, escape, return, n, q, r and l in turn (never y, so "Die?" is always declined), `getlin` answers "none", escape or a wish of two magic lamps in turn, and the pauses are skipped; `getobj()` and `getdir()` take the object and direction the test chose. The hooks are compiled only with the macro.

It found, and this batch fixes:
- **Breaking a wand of stasis** logged "object 489 used?" and "What an interesting effect (489)": `do_break_wand()` had no case for it. Breaking it now has the effect of zapping it (`apply.c`).
- **Wishing for "3 wands of death"** gave a random wand (and "3 spellbooks of wizard lock" a random book): the check that stops "wand of death" being read as a figurine of Death only looked for "wand " and "spellbook " and not the plurals (`objnam.c`).
- **Wishing for a bardiche** gave nothing (read as the "bard" rank), and **for an Uruk-hai shield** gave a random shield (read as the "Uruk-hai" monster); both are now exempt like "samurai sword" (`objnam.c`).
- **Wishing for The Lyre of Orpheus** gave nothing, because `makesingular()` turned "Orpheus" into "Orpheu"; it is exempt like "Perseus" and "Proteus".
- **Plurals**: "3 staves of divination" (and healing, holiness, matter, escape, battle) was read as "stave"; "3 main gauches" and "3 bardiches" lost their "e"; "3 leashes" and "3 topazes" lost an "s" too many; "3 iron safes" was made "iron saves" by `makeplural()` and could not be read back; "3 becs de corbin" and "3 amulets versus poison" were not understood (the French "de" and "versus" were only handled when making plurals, not reading them). All fixed in `makeplural()` and `makesingular()`.

Things the test does not do: it passes no game turns except the hero's timeouts and the object timers, so monsters never act on their own; it uses one hero (a human Knight, always as strong as 16 in everything) so role-specific restrictions are not covered; and "nothing happened" is not a failure, only a crash, an `impossible()` message, a hang, or a wrong wish. A full run takes about five minutes (about 12,700 log lines) and passes on the current tree; a deliberately planted crash in the wand of stasis zap was reported as "GAME DIED (signal 11) at: use 489 stasis 0". The normal build is unaffected.

## Batch 77: new hallucinatory glow color

"donut seeds", "yellow spiders" and "purplish-blue pickle color" are added to the list of colors you see while hallucinating (`hcolors[]` in `do_name.c`), used for the glow of a blessed or cursed weapon, the color of a potion's vapors, a wand's light and so on. Only the binary changes.

## Batch 78: new hallucinatory monsters

Twenty-four names are added to the list of monsters you see while hallucinating (`bogusmons[]` in `do_name.c`): bunnit, crabulum, crabulus, grabitty rabbit, gravity squirrel, skwerl, freppit, reek fly, flying fart cloud of Farklur, bnuuy, monstrous frog, glop puddle, Master Belch, colossal terrorpede, giant funnel-web spider, green widow, hideous abomination, beaked lobster, walky-walk, turkey, giant swan, moldy pudding, mold soldier and fungus knight. Only the binary changes.

## Batch 79: phytosaur

A new lizard-class monster (`:`), the phytosaur, comes right after the crocodile in `monst.c`. It is a magenta, larger relative of the crocodile: level 8 (the crocodile is 6), speed 9, AC 4, a bite of 2d8 and a claw of 1d12 (the crocodile's are 4d2 and 1d12, about 11.5 a turn against about 15.5), weight 1200, and the crocodile's flags (swims, amphibious, thick hide, oviparous, carnivorous, strong, hostile). It is rare (frequency 1, like the crocodile) and is made in small groups (`G_SGROUP`). It has no baby form. The monster numbers after it all move up by one, so old save files and bones are not compatible; the data files must be rebuilt, including the utilities (`util/dgn_comp` has to be rebuilt too, or the game reports "Dungeon description not valid") and `nhdat` (`make dlb`). Created in wizard mode and run through the corpse stress test (5,951 kills, no crash).

## Batch 80: wizkit lookup and the "access denied" message

Starting a wizard-mode game with `WIZKIT=wizkit.txt` in the options file printed "Access to wizkit.txt denied (2)" and waited for a key, even though the kit was then found and loaded from your home directory. The name was looked for in the current directory first (the launcher runs the game from its own folder, where there is no such file), and `access()` failing with "no such file" (error 2) was reported as a permission problem before falling back to `~/wizkit.txt`. Now a plain wizkit name is looked for only in your home directory; the current directory is used only when the name is given explicitly as `./name` (a full path starting with `/` also works as before), and only then can the access message appear (`files.c`). Only the binary changes.

## Batch 81: Katana of Masamune; Kusanagi for neutral Samurai

- **The Tsurugi of Muramasa is now The Katana of Masamune**, a katana instead of a tsurugi (`artilist.h`, `artifact.c`, `role.c`, the quest level in `Samurai.des`, and the `data.base` entry). Its powers are unchanged (+d8 damage, luck, beheading on a natural 20, self-willed). It can now be wielded together with Kiku-Ichimonji, Kusanagi or Ichigeki for two-weapon combat, and each of those three accepts it in turn (`artifact_offhand_ok()`). Checked in wizard mode: Masamune in the off hand with Kiku-Ichimonji begins two-weapon combat. Data files must be rebuilt (the quest level changed).
- **Kusanagi is now neutral and the first sacrifice gift of a neutral Samurai** (it was a lawful `SPFX_NOGEN` artifact that crowned a lawful Samurai). It is a Samurai role artifact like Kiku-Ichimonji (the lawful Samurai's first gift), so a neutral Samurai's first gift is guaranteed to be Kusanagi, and since it is no longer `NOGEN` it can also turn up at random like any other neutral artifact. Its powers are unchanged. It is therefore no longer given when a lawful Samurai is crowned (the code in `gcrownu()` is gone; a lawful Samurai still gets Excalibur-or-nothing like other lawful characters). Unverified in play: the sacrifice gift itself (same path as Durandal for Knights).
- **The Katana of Masamune is now unaligned (`A_NONE`)**, so a neutral Samurai (or a lawful one) is not blasted for touching it; it is still self-willed and only a Samurai may take it without being blasted.
- **Role artifacts no longer take the hero's alignment for Kiku-Ichimonji and Kusanagi.** `hack_artifacts()` (`artifact.c`) gives every artifact whose role is yours your own alignment, which would have made both Samurai artifacts lawful or neutral to match the hero and, being tied, `mk_artifact()` would have picked Kiku-Ichimonji (first in the list) for both. They are now left alone: a lawful Samurai's first gift is Kiku-Ichimonji and a neutral one's is Kusanagi. Checked with the use stress test (which now logs "first gift for <race> <role>: <artifact>" at its start; run with `--keys s,H,M,N,.` or `s,H,M,L,.`): neutral gave Kusanagi, lawful gave Kiku-Ichimonji. A broadsword is a restricted skill for Samurai, but a gift weapon's skill is unrestricted when it is given, so a gifted Kusanagi can be trained (not tried in play).

## Batch 82: monsters hurtled into holes

Found by running the use stress test as a Samurai: about half the runs died with "relmon: mon not in list" or logged "dmonsfree: N removed doesn't match M pending" (the Knight never hit it, because only martial arts users and the wand of wind hurtle monsters often). `mhurtle()` (`dothrow.c`), used when a bare-handed strike staggers a monster, a lance jousts, or a wand of wind blows one around, walks the monster along a path and calls `mintrap()` on every square. If the monster fell through a hole or trap door, or into a level teleporter, it had already been moved to the migrating list (its position fields are overwritten with the place it arrives at), but the walk went on moving it on the map, and the callers went on hitting, killing and waking it. A hurtled monster now stops in the square before a hole, trap door, level teleporter or magic portal, and stops where it is if it dies or is teleported by the trap it lands on. Six Samurai runs of the use test afterwards were clean (before, about four in nine failed).

The use test also now finishes a theft that takes time (a nymph removing your armor) before the hero hits back: the hero is helpless while it happens, and the test was killing the thief in the middle, logging "stealarm(): dead monster stealing".

## Batch 83: crowning gifts (Dirge, Luck Blade, Excalibur or Kiku-Ichimonji)

Crowning (`gcrownu()` in `pray.c`) changes:
- **A lawful crowning is now an even chance of Excalibur or Kiku-Ichimonji**, whatever you are wielding. A new helper, `crowning_weapon()`, turns a wielded unnamed weapon of the right kind (a long sword for Excalibur, a katana for Kiku-Ichimonji) into the artifact, otherwise a new one appears at your feet; if the one rolled already exists you get the other, and if both exist you get nothing. (Before, you only got Excalibur, and only if you were wielding an unnamed long sword.) The long sword skill is unrestricted either way.
- **A chaotic Knight is crowned with Dirge** instead of Stormbringer (the same way: a wielded long sword becomes Dirge, otherwise a new one appears at your feet). Dirge was previously only made when a chaotic Knight sacrificed a human at an altar while wielding a long sword; that still works.
- **A Tourist is crowned with the Luck Blade** (a short sword; the short sword skill is unrestricted), instead of Vorpal Blade or Stormbringer. The Luck Blade is neutral, so `hack_artifacts()` (`artifact.c`) gives it the Tourist's own alignment, as it does for Demonbane for a Priest; a chaotic Tourist can wield it without being blasted. A Tourist's first sacrifice gift was already a random artifact of the hero's alignment (the Express Card is `NOGEN`); that is unchanged.
- The other alignments' crowning (Vorpal Blade, Stormbringer, the Pirate King's Reaver, the Wizard's and Monk's spellbooks, the lawful Healer's Aegis) is unchanged.

Checked in wizard mode with a test hook (`stress_crown()` in `pray.c`, compiled only with `USE_STRESS_TEST`, run by setting `STRESS_CROWN=1` in the environment of `util/stress_test.py --test use --keys ...`, which then crowns the hero six times with different things wielded and logs the artifacts made): a lawful Knight got Excalibur then Kiku-Ichimonji, then nothing once both existed; a lawful Priest the same; a chaotic Knight got Dirge from a wielded long sword; a neutral and a chaotic Tourist each got the Luck Blade at their feet; a lawful Samurai got Kiku-Ichimonji and then Excalibur at their feet. The ordinary use test still passes.

Correction to Batch 81: making the Katana of Masamune `A_NONE` changed nothing in play, because `hack_artifacts()` already sets the quest artifact's alignment to the hero's own at the start of every game, so a neutral Samurai was never blasted by it.

## Batch 84: spell failure message

Failing to cast a spell prints "The air around you crackles as you goof up." half the time instead of "You fail to cast the spell correctly." (`spelleffects()` in `spell.c`). The failure itself (half the energy lost, the turn used) is unchanged. Only the binary changes.

## Batch 85: venomous crab

A new sea-monster-class monster (`;`), the green venomous crab, comes right after the jellyfish in `monst.c`. It is a weaker, poisonous relative of the giant crab: level 4 (the giant crab is 5), speed 12, AC 3, one ordinary 1d6 claw and one poisonous 1d6 claw (`AD_DRST`: strength loss, with the usual small chance of instant death for the poison), against the giant crab's two 1d8 claws. It swims, is amphibious, hostile and `M1_NOTAKE` like the other `;` monsters, so it stays in the water. It is poison resistant, and its corpse is poisonous to eat (`M1_POIS`: the usual poisonous-corpse penalty unless you resist) and may give poison resistance. Like the rest of the class it is never made at random (`G_NOGEN`); it is added to swamp rooms in `mkroom.c`, where the water monsters are now 60% giant eels, 20% venomous crabs, 10% piranhas and 10% electric eels (they were 80%, 0%, 10%, 10%). The monster numbers after it move up by one, so old saves are not compatible, and the data files (including the utilities and `nhdat`) must be rebuilt as for Batch 79. Created in wizard mode and run through the corpse stress test (5,965 kills, no crash).

## Batch 86: Dirge can be generated at random

Dirge (`artilist.h`) lost its `SPFX_NOGEN` flag, so it is an ordinary chaotic artifact: a randomly made long sword can now be Dirge, and any chaotic character can be given it as a sacrifice gift (before it only came from a chaotic Knight's crowning or from sacrificing at an altar with a long sword in hand, so a chaotic non-Knight practically never saw it). Excalibur stays `NOGEN`. A chaotic Knight's first sacrifice gift is now a random chaotic artifact, which can be Dirge, in which case the crowning code just does nothing about it (`crowning_weapon()` is a no-op when the artifact already exists). Checked with the item stress test (every artifact is made, 20,026 log lines) and the use test as a chaotic Knight (12,738 log lines), both clean. Only the binary changes; no data files.

## Batch 87: Lancea Longini

A new lawful artifact, **Lancea Longini** (the Lance of Longinus), a silver spear (`artilist.h`, last before the terminator, so no existing artifact number moves). It is restricted (a non-lawful hero who touches it is blasted, like Grayswandir) and not `NOGEN`, so it can be generated at random and given as a lawful sacrifice gift. While wielded:
- **+d5 to hit and +d12 damage against anything that is not lawful** (`SPFX_DALIGN`, the same mechanism as the Sword of Kas).
- **Half its enchantment counts as armor class** (`find_ac()` in `do_wear.c`: a +6 spear gives 3 points; nothing for +1 or less). Enchanting it while wielded refreshes the AC (`chwepon()` in `wield.c`).
- **A 50% chance to destroy an angelic being (`A`) outright** on a hit, whatever its alignment ("The Lance of Longinus pierces the couatl, destroying it!", `artifact_hit()`; the check comes before the "damage bonus applies" test so lawful angels are covered too).
- **Drain resistance** (`SPDF_DRAIN`).
- **Enrages demon lords and princes as Excalibur does** (they are not made peaceful, and `demon_talk()` makes a bribable one hostile).
- **If blessed, +d6 against `&` and `i` monsters**, on top of the usual blessed d4 against undead and demons (`dmgval()` in `weapon.c`).
It has a data.base entry. The artifact count changes, so the data files and utilities must be rebuilt as for Batch 79 and old saves are not compatible. Checked in wizard mode: a blessed +6 spear took AC from -1 to -4 when wielded, and destroyed couatls with the message above (and missed others); the item and use stress tests were clean (20,030 and 12,739 log lines). Not checked in play: the demon-lord anger, the blessed bonus against `&`/`i` and the random-generation/gift paths.

## Batch 88: Vorpal Blade damage

The Vorpal Blade's damage bonus was a flat +1 (its `PHYS(5,1)` rolled `d1`). It is now **2d4 with exploding dice** (`exploding_dice()` and the flat-bonus switch in `spec_dbon()`, `artifact.c`): each die that comes up 4 is rolled again and added, repeating as long as it keeps coming up 4 (capped at 20 rerolls per die, a safety net the chance of reaching is about 4^-20). The average bonus is about 6.7 (2d4 alone averages 5), the range 2 and up with no ceiling. Like the old +1 it applies to every target, and a monster wielding the blade gets it too. The +d5 to hit and the beheading on a natural 20 (or any hit on a jabberwock) are unchanged. Only the binary changes.

## Batch 89: Vampire Killer

A new lawful artifact bullwhip, **Vampire Killer** (`artilist.h`, after Lancea Longini so no artifact number moves; restricted like the other lawful artifacts, not `NOGEN`, so it can also be generated or given as a sacrifice gift). While wielded:
- **+d5 to hit and +d8 damage against everything.**
- **Another +d10 to hit and +2d10 damage against the undead, demons (both `&` and `i`), werecreatures and vampires**, including any `V` that is not flagged undead (`vampire_killer_foe()`, `spec_abon()` and `spec_dbon()` in `artifact.c`; a lycanthrope hero counts as a foe when something with the whip hits you).
- **Drain and lycanthropy resistance** (`SPDF_DRAIN|SPDF_WERE`).
- **`#invoke` acts as an uncursed scroll of remove curse** (new invoked property `UNCURSE_ALL`, `artifact.h`; the usual invoke delay applies).
- **It may be held in the off hand beside any weapon**, like the Banes (`artifact_offhand_ok()`).
It has a data.base entry. `readobjnam()` (`objnam.c`) took the "vampire" in a wish for it for the monster and failed with "Nothing fitting that description exists"; it is now exempt like "samurai sword". The artifact count changes, so the data files and utilities must be rebuilt as for Batch 79 and old saves are not compatible. Checked in wizard mode: the wish works, `#invoke` says "You feel like someone is helping you.", and the item stress test is clean (20,034 log lines; it wishes for every artifact by name). Not checked in play: the damage figures against vampires (a level 1 Knight died to three of them), the off-hand rule and the whip's resistances.

## Batch 90: curses `curses_rtrim()` read before the string

A bus error when starting a new game (reported for a neutral male orcish Tourist, with `windowtype:curses`; the crash report points at `curses_rtrim()` called from `curses_add_nhmenu_item()` while the opening legend is shown). `curses_rtrim()` (`win/curses/cursmisc.c`) started at the last character with `--s` before testing it, so an empty string, such as the blank lines of the legend, made it read the byte in front of the string, and when the string happened to be the first block on a page of memory that byte is unmapped. Which game crashes therefore depends on where memory happens to be allocated, not on the role or race. It also cut a one-character line down to nothing. It now takes the length first and never looks before the start. Checked with a small program that puts an empty string at the start of a page with the page before it unmapped: the old function died with SIGBUS, the new one leaves "a" alone, trims "hi  \t" to "hi" and "   " to "". The tty port was never affected.

## Batch 91: Pirates start with a skeleton key

The Pirate's starting inventory (`u_init.c`) gains a skeleton key, after the oilskin sack (so the other items keep their letters). Checked in wizard mode: a human Pirate starts with "k - an uncursed skeleton key". Only the binary changes.

## Batch 92: roast chicken

A new food, **roast chicken** (`objects.c`, after the huge chunk of meat; never generated at random): 500 nutrition, weight 40 (a chicken corpse's), three turns to eat, flesh (so it breaks vegetarian conduct), and a carnivorous pet's favourite food (`dog.c`, like tripe). It is made from a chicken:
- **A chicken corpse on the floor hit by fire or lightning** becomes a roast chicken: any ray of fire or lightning passing over its square (wand of fire, wand of lightning, spells and monsters' breath alike, including a bolt zapped down at your own square) and any fire explosion (the fireball spell). Done in `zap_over_floor()` (`zap.c`) by the new `roast_chickens_at()` and `roast_chicken()` (`eat.c`); "The chicken corpse is roasted to a golden brown!"
- **A chicken killed by fire or lightning** leaves a roast chicken instead of a corpse (if it leaves a corpse at all: tiny monsters often leave none). A global flag, `roast_kill` (`decl.c`), is set around the kill by a ray of fire or lightning (`buzz()`), a fire or shock explosion (`explode()`: scroll of fire, fireball spell, exploding wands) and by a killing blow with Fire Brand, Sunsword or Mjollnir (`hmon_hitmon()`); `make_corpse()` (`mon.c`) checks it. A named chicken's name carries over.
Checked in wizard mode: zapping fire down at a chicken corpse roasts it; chickens killed by wands of fire and lightning and by scrolls of fire left roast chickens (one debug line confirmed the flag was set on the kill); eating one says "This roast chicken is delicious!" and fills you up; the corpse, item and use stress tests were clean (5,965, 20,050 and 12,753 log lines; the use test also wishes for it in four forms). Not checked in play: kills with Fire Brand, Sunsword and Mjollnir (they set the flag the same way) and pets eating it. A new object shifts every later object number, so old saves are not compatible and the data files and utilities must be rebuilt as for Batch 79.

## Batch 93: no Vecna shopkeepers; rodent colours

- **Vecna as a Mall shopkeeper.** `shkinit()` (`shknam.c`) gives scroll and spellbook shopkeepers a spellcaster's race with `PM_LICH + rn2(4)`, which in vanilla is lich, demilich, master lich or arch-lich. Vecna sits between the master lich and the arch-lich in `monst.c`, so one time in four a scroll or book shop (the Mall always has both) got Vecna, a unique, as its shopkeeper ("a Vecna"). It now picks the four liches by name. The other `PM_xxx + rn2(n)` ranges in that function were checked against `monst.c` and still name only what they should.
- **Colours.** The devil bunny is magenta (was red), the rabite bright yellow (was white) and the silktail bright yellow (was red), so none is mistaken for a rock mole (gray) any more. Only `monst.c` changes; no saved data is affected.

## Batch 94: wand of acid

A new ray wand, **wand of acid** (`objects.c`, right after the wand of lightning; probability 30, cost 175, METAL, shown yellow), appearance **"zirconium"**. The wand of stinking fog had that appearance, so it gives it up and takes the old spare "jeweled" slot back; every other wand's probability drops a little (light 72→66, striking 58→52, make invisible 44→40, slow/speed monster/undead turning 45→41, nothing 25→23) to keep the class at 1000.
- **Zapped** it fires a bolt of acid for 6d6, the same as the fire, cold and lightning wands: `weffects()` calls `buzz(ZT_WAND(ZT_ACID), 6, ...)` (the same ray as a yellow dragon's breath, so the existing acid code in `zhitm()`/`zhitu()` applies: acid resistance, and a chance to corrode the target's weapon and armour). `flash_types[7]` (`zap.c`) is now "bolt of acid". The wand identifies itself when you see it work.
- **Engraved** with it works like digging: it engraves into the floor (`type = ENGRAVE`) and identifies itself ("This zirconium wand is a wand of acid!"; "The floor sizzles and smokes as acid eats into it.", "You hear sizzling!" when blind) (`engrave.c`).
- **Zapped at yourself**: "The acid burns!" for 12d6 (none with acid resistance) and may corrode your weapon and armour (`zapyourself()`).
- **Broken** it makes a splash of acid explosion for twice the usual `4 x charges` (like a wand of fire or cold) (`apply.c`).
- **Monsters** use it like the other attack wands (`muse.c`: they stop if they have seen you resist acid; a new case 13 in `rnd_offensive_item()` lets tougher monsters carry one). The two `rnd_class(WAN_LIGHT, WAN_LIGHTNING)` replacement picks (wand of polymorph recharged, cancellation in a bag of holding) now range up to the wand of acid.
Checked in wizard mode: a wished-for wand of acid engraves in the floor and says what it is; zapping it at a pony killed it, the bolt bounced ("The bolt of acid bounces!") and hit me ("The acid burns!"); breaking it gives "The saddled pony is caught in the splash of acid!". A new object shifts every later object number, so old saves are not compatible and the data files and utilities must be rebuilt as for Batch 79.

## Batch 95: scroll of flood

A new scroll, **scroll of flood** (after the scroll of ice in `objects.c`; label "ZUN KEL MARA", probability 15, cost 200), as in UnNetHack. The identify scroll (170→160) and light (85→80) give up the 15 points. Implemented in `seffects()` (`read.c`) with an area callback, `flood_square()`:
- **Uncursed:** pools of water appear in the squares you can see within 4 squares of you. A square at distance *d* floods with chance 1/(1+d/2), so the ring next to you always floods; only plain floor floods (not stairs, doorways, corridors, boulders, or squares next to a door), traps there are removed, objects get wet, monsters may drown. Your own square floods one time in four (only floor or corridor), and then you fall in ("You fall into the water!", the usual crawl-out or drowning rules).
- **Blessed:** the same, but never the square under you.
- **Cursed:** only the square under you floods, always ("Water wells up from the floor beneath you!"), which is what you want when you are levitating or water walking.
- **Confused:** instead of making water it dries up ordinary pools in a circle around you (radius 5 blessed, 3 uncursed, 1 cursed): "The water around you drains away!", or "a faint gurgling" if there are none.
- No effect in Sokoban or on the Plane of Water or Air ("A little water drips from the scroll."), or when nothing could flood. The scroll identifies itself when water appears or drains. Monsters never use it. It can be written (ink cost 20, like stinking cloud), and it is not a possible starting-inventory scroll.
- A grave reading "Apres moi, le deluge" ("After me, the flood") with a scroll of flood on it now turns up now and then (1 in 12 of the graves that are not "Saved by the bell!"), in `mkgrave()` (`mklev.c`).
- Also fixed: the scroll of ice had no ink cost in `write.c` (writing one would have hit `impossible()`); it now costs 20 too.
Checked in wizard mode: blessed and uncursed scrolls made pools (and one drowned my pony); a cursed one dropped me in the water; a blessed scroll read while confused dried up the pools around me; on the stairs a cursed scroll floods nothing. The item and use stress tests were clean. A new object shifts every later object number, so old saves are not compatible and the data files and utilities must be rebuilt as for Batch 79.

## Batch 96: scroll of antimagic and nine more scroll labels

A new scroll, **scroll of antimagic** (after the scroll of flood in `objects.c`; label "HY DIDILY GINKY WINKS", probability 18, cost 250; identify 160→150, enchant weapon 75→70 and light 80→77 pay for it), after dNetHack:
- **Uncursed or blessed:** "An antimagic field surrounds you!": 400 turns (added to any time left) of magic resistance and, for the same 400 turns, no spellcasting at all ("The antimagic field around you stops you casting spells."; `u.uspellblock`, a new counter in `struct you` run down each move in `allmain.c`, checked in `docast()`; ^X says "unable to cast spells"). Magic resistance is a timed intrinsic: `youprop.h` now has `HAntimagic` and `Antimagic` includes it, with the expiry message "Your antimagic field fades away." in `timeout.c` and "You feel able to cast spells again." when the block ends.
- **Cursed:** you lose 400 energy (Pw goes to 0): "You feel drained of magical energy!"
- **Confused, not cursed:** any antimagic field you have goes away (magic resistance from this scroll and the casting block); otherwise "a faint tingle".
- **Confused and cursed:** your energy is refilled ("You feel full of magical energy!"), up to your maximum (not beyond).
- Can be written (ink 18, so 9 to 17 per scroll). Monsters never use it.
Nine more scroll labels join the random pool ("SUU KOL WUF GORGLE", "JAM BUK ORKO", "FALUMPHA", "CACKLE CACKLE", "YUMBLE YOTZ", "PELL MELL", "ZI ZEEBLE ZUMBLE ZO", "SNOO SNUNK SKO", "ANVIL BONK"), so there are now spare labels that no scroll uses in a given game.
Checked in wizard mode: reading it gives the message and `Z` is refused; the field lapses after 400 turns with both messages; a cursed scroll took Pw from 3 to 0; confused and cursed refilled Pw from 0 to 6; confused and uncursed, with no field, gave the faint tingle. A new object shifts every later object number, so old saves are not compatible and the data files and utilities must be rebuilt as for Batch 79.

## Batch 97: rabite and silktail orange

Rabites and silktails change from bright yellow to **orange** (`CLR_ORANGE`, `monst.c`), still clear of the rock mole's colour. Colour only: no change to monster numbering, so saves and data files are unaffected. (Many terminals draw orange as bright red; the interface's own colour table decides.)

## Batch 98: hallucinatory "rabtail" and "silkbite"

Two more names join the hallucination monster list (`bogusmons[]`, `do_name.c`): "rabtail" and "silkbite". Names only: no monster numbering or data-file change, so saves are unaffected.

## Batch 99: role hit dice; healers may learn attack spells

Per-level hit point dice (`hpadv.lornd` in `role.c`, used by `newhp()` below the role's cutoff level; starting HP, the flat gain after the cutoff and the race and Constitution bonuses are unchanged):

| Role | Die before | Die now |
|---|---|---|
| Barbarian | d10 | d12 |
| Caveman | d8 | d12 |
| Knight, Dark Knight, Valkyrie, Samurai | d8 | d10 |
| Healer | d8 | d6 |

Healers can now advance **attack spells** to Basic (`Skill_H` in `u_init.c`; it was restricted before). Checked in wizard mode with a human Healer: `#enhance` lists attack spells (Unskilled, trainable). No change to object or monster numbering, so saves still load.

## Batch 100: thirteen new monsters

All thirteen are in `monst.c`. Every monster after the insertion points moves up in the numbering, so old saves and bones are not compatible and the data files and utilities must be rebuilt as for Batch 79. "Rare" means frequency 1 and "uncommon" frequency 2.

| Monster | Class, colour | Level / speed / AC | Attacks | Notes |
|---|---|---|---|---|
| acid ant | `a`, bright green | 3 / 18 / 3 | bite 2d4, acid sting 2d4 | small groups, rare; acid resistant, acidic corpse (like the fire and snow ants otherwise) |
| thunder ant | `a`, bright yellow | 5 / 18 / 3 | bite 2d4, shock sting 3d4 | alone, rare; shock resistant |
| giant wasp | `a`, orange | 4 / 20 / -1 | poison sting 1d6 | large groups, uncommon; flies, poison resistant (a stronger killer bee) |
| orc warrior | `o`, orange | 4 / 6 / 8 | weapon 1d8 | small groups, uncommon; three times in four carries a sword (orcish short sword, orcish scimitar, scimitar, short sword, long sword or broadsword), usually orcish chain or ring mail, a shield half the time, iron shoes a third of the time; grows up into an orc-captain |
| orc warlock | `o`, bright green | 4 / 7 / 8 | weapon 1d6, spell (mage spells: psi bolt, cure self, haste self, stun) | alone, rare; three times in four carries a quarterstaff |
| acrocanthasaurus | `:`, bright green | 12 / 12 / 3 | bite 5d12, two claws 2d6 | alone, rare; huge |
| tyrannosaurus | `:`, orange | 16 / 12 / 2 | bite 6d12, tail slam 2d12 | alone, rare, Gehennom only; huge |
| frost gator | `:`, bright white | 7 / 9 / 4 | cold bite 3d6, breath of frost 2d6 | alone, uncommon, not in Gehennom; swims, cold resistant, vulnerable to fire |
| ogre devourer | `O`, magenta | 8 / 14 / 3 | weapon 3d6, bite 2d6 | small groups, rare; a mace, war hammer or club five times in six |
| gibbering mouther | `P`, orange | 9 / 6 / 9 | four bites 1d4, spits blinding venom | alone, rare; the poor AC, slow speed and level give it plenty of hit points; not vegetarian food like the other puddings |
| gargoyle knight | `g`, gray | 13 / 15 / -4 | claw 3d6, claw 3d6, bite 3d4, weapon 2d8 | alone, rare, Gehennom only; flies, stone resistant; five times in six carries a two-handed sword, tsurugi, long sword, katana, broadsword or runesword |
| ferroloth | `&`, cyan (iron) | 12 / 12 / -6 | claws 2d8 x2, bite 2d6, breath of molten metal 4d6 | neutral, alone, rare, Gehennom only, no corpse; magic resistance 55, fire and poison resistant |
| tar demodand | `&`, black | 10 / 12 / 2 | sticky claw 3d4 (grabs you, like a glue golem), two claws 2d8 | neutral, alone, rare, Gehennom only, no corpse; magic resistance 40, fire and poison resistant |

The colours for the gargoyle knight, the ferroloth and the tar demodand were not specified; I chose gray (stone), cyan (iron, like the iron ant) and black (tar).

New code:
- **Tail slam.** A new attack type `AT_TAIL` (`monattk.h`), handled like a butt or tentacle attack in `mhitu.c` ("The tyrannosaurus slams you with its tail!"), `mhitm.c` and `uhitm.c` (a polymorphed hero's tail attack).
- **Molten metal.** A new breath type `AD_METL`, a seventh gem-dragon-style beam in `gem_beams[]` (`mthrowu.c`): a straight beam that does not bounce, drawn as a fire beam, which a reflector can turn back. It burns armour, scrolls, potions and spellbooks like fire. Fire resistance only **halves** the damage (`gem_hitu()` scales with partial resistances: nothing at 0%, half at 100%; monsters that resist fire take half in `gem_hitm()`). A hero polymorphed into a ferroloth can breathe it with `#monster` (`polyself.c`).
- **Gear** (`m_initweap()`, `makemon.c`) for the orc warrior, orc warlock, ogre devourer and gargoyle knight as in the table; the orc warrior is added to the grow-up table (`mondata.c`) and the gibbering mouther is excluded from `vegetarian()` (`mondata.h`).

Checked in wizard mode: all thirteen can be created (the corpse stress test, which kills every monster, ran clean: 6,147 log lines); a ferroloth hit a hero wearing a ring of fire resistance with "The ferroloth breathes a gout of molten metal! The blast of molten metal hits you! You are scalded, but your resistance takes the worst of it!"; a tyrannosaurus bit and slammed ("slams you with its tail!"); a frost gator bit ("You're covered in frost!"). Not seen in play: the gator's frost breath, the mouther's venom, the warlock's spells, and the ants' and wasp's stings (all use existing code).

## Batch 101: silver damage is 2d6

The extra damage silver does to silver-haters (vampires, demons, werecreatures, shades and so on) was d20, so a silver weapon almost always beat any other weapon of its kind. It is now **2d6** (mean 7 instead of 10.5, and a much narrower spread). Every place that used the d20 changed (`weapon.c` `dmgval()` for silver weapons, silver arrows from Heartseeker and silver ammunition; `uhitm.c` for silver rings on bare hands, silver shield bashes, other silver objects and silver ammo, where ammo fired without a launcher gets d6 instead of the old d10 reduced bonus; `mthrowu.c` for silver objects thrown at a silver-hating hero). The Heartseeker entry earlier in this file says d20; it is 2d6 now. Only the binary changes; saves and bones still load.

The silver artifacts (Grayswandir, Sunsword, Werebane, Demonbane, Quicksilver, Lancea Longini) have no silver code of their own: their bonus against silver-haters comes from the same `dmgval()` line, so it is 2d6 too. (For an artifact that doubles damage the bonus is halved first so that it still comes out at the same size after the doubling, as before.) The remarks in earlier batches that Grayswandir's silver bonus is "unchanged" mean unchanged by those batches; it is the same 2d6 as every other silver weapon now.

## Batch 102: Heartseeker is neutral

Heartseeker was a lawful bow. It is now **neutral** (`artilist.h`), so a neutral Ranger can use it without being blasted for a cross-aligned artifact. That leaves the bows as Yoichi no Yumi (lawful), Heartseeker (neutral) and Plague (chaotic). Nothing else in the code depends on its alignment; it is still never a gift, still not self-willed. Only the binary changes; saves and bones still load (the item stress test, which wishes for every artifact, was clean: 20,371 log lines, no mismatches).

## Batch 103: the Ninja

A new role after SLASH'EM / SlashTHEM's Ninja, with the changes you asked for. Rebuild everything (the monster list, artifact list and quest data all changed; also `util/makedefs` and `src/questpgr.c`, because `N_HDR` in `qtext.h` rose from 16 to 24: the quest text now has too many role sections for the old limit); old saves and bones do not load.

**Role** (`role.c`, `u_init.c`, `attrib.c`, `charinit.c`, `botl.c`): human, elf, gnome or orc, neutral or chaotic, menu key `n` (the character-selection menu in `charinit.c` has one more row; Wizard is now on the row above "quit"). Gods Jiraiya (lawful, never used), Tsunade (neutral) and Orochimaru (chaotic). Attributes: minimums 10/10/7/12/10/7 (Str Int Wis Dex Con Cha), weights 25/12/8/28/19/8; 11 initial HP and d8 later, energy as the Rogue's; special spell **teleport away** (Intelligence). Intrinsics: stealth at XL1, searching at XL3, speed at XL5, swimming at XL10, warning at XL15. No techniques. Pet: a kitten (`petnum`).
- **Starting inventory:** a +0 ninja-to (a broadsword), 12 to 22 shuriken (**10 + 2d6**; shuriken break the way darts do when they hit, since both use the dart skill and the same breakage rule in `thitmonst()`, so I raised the count), an uncursed potion of sickness, a blessed potion of invisibility, an osaku (lock pick), a +1 leather cloak, +1 low boots and a +1 kaginawa (grappling hook). The kaginawa is not made the alternate weapon.
- **Titles:** Kukkyu (XL 1-2), Hachikyu (3-4), Shichikyu (5-7), Rokkyu (8-10), Gokyu (11-13), Yonkyu (14-17), Sankyu (18-23), Nikyu (24-27), Ikkyu (28-29), Shodan (30). The rank table has a tenth slot (`rank[10]` in `you.h`) and the Ninja alone uses `ninja_rank()` (`botl.c`) instead of the four-level steps.
- **Skills:** dagger, knife, short sword, flail, dart (shuriken and darts were already one skill in this game) and two-weapon combat to Expert; long sword (so the ninja-to) to **Expert** and curved sword to **Skilled** (changed afterwards, see Batch 105); mace (maces, morning stars, clubs), polearms and bow to Skilled; two-handed sword, hammer, spear and riding to Basic; martial arts to Master (`martial_bonus()` now includes Ninja). Spell schools: escape to **Expert**, matter and enchantment to **Skilled**.
- **Japanese theme as for the Samurai:** a Ninja sees wakizashi, ninja-to, nunchaku, naginata, osaku and the rest, now with **kaginawa** for the grappling hook (for Samurai too), and says "Konnichi wa", "Irasshaimase" and "Sayonara".
- **Gifts:** the first sacrifice gift is **Ichigeki** (it is now the Ninja's role artifact, in the Ninja's alignment; other heroes can still get it as before); crowning gives Vorpal Blade (neutral) or Stormbringer (chaotic) as usual.

**Monsters** (`monst.c`): the old hostile "ninja" of the Samurai quest is now the Ninja's player monster (it moved into the player-monster block, level 10; the Samurai quest's ninja are made hostile by `Samurai.des` as before), plus the quest leader **Ken Hayabusa**, the nemesis **Jaquio** (level 15, like Ashikaga Takauji) and the guardians, the roshi.

**Fuma-itto no Ken** (the quest artifact, `artilist.h`, `artifact.c`): a self-willed broadsword, a **flat +5 to hit** and **+2d8 damage** against everything (the Grayswandir-style flat bonus), drain resistance while carried, and `#invoke` makes 2d12 shuriken (new `CREATE_SHURIKEN`, blessed and enchanted like the sword). It can be wielded in the main hand with **Kusanagi or Ichigeki in the off hand** (`artifact_offhand_ok()`); it is not accepted in the off hand, so Kusanagi or Ichigeki in the main hand refuse it.

**Quest** (`dat/Ninja.des`, `dat/quest.txt`; levels `Nin-strt/loca/goal/fila/filb`): the Samurai quest's maps, with Ken Hayabusa and eight roshi in the throne room at the Green Plains, Jaquio with the Bell of Opening on the goal level and the Fuma-itto no Ken lying under him. Random monsters are as in the wiki: 55% wolves, 14% other `d`, 14% giant bats, 3% other `B`, 14% anything; the besieging and guarding monsters are wolves, giant bats and random hostiles instead of ninja and samurai. The text is the Samurai's, reworded (ninja village, the clan, "slip towards", and so on).

Checked in wizard mode: a neutral and a chaotic Ninja start with the inventory above (21 shuriken once), a kitten, the title Kukkyu and the skills listed; the ninja-to is wielded; Fuma-itto no Ken can be wished for, wielded, and two-weapon combat with Kusanagi in the off hand works; `#invoke` made 14 shuriken; Home 1 loads with the arrival text. Not played through: the quest itself, the +5 and 2d8 numbers in combat, the first gift, the spell. The item, use and corpse stress tests ran clean (20,417, 12,928 and 6,147 log lines, no mismatches).

## Batch 104: Bards start with a spellbook of sleep

`u_init.c`: the Bard's starting inventory gains a blessed **spellbook of sleep** (`Bard[]`). It teaches the spell through the ordinary `ini_inv` path, replacing the hand-made `initialspell()` call the Bard had before, so the hero still starts knowing sleep (level 1 enchantment, 0% fail, which Lullaby needs) and now also carries the book to relearn it from. Checked in wizard mode: a human Bard's inventory shows the book (`h - a blessed spellbook of sleep`) and `+` lists sleep at 100% memory.

## Batch 105: keese; parrots are orange

**Keese** (`B`, after the giant bat in `monst.c`, bright green): level 3, speed 22, AC 6, one bite of 1d8; flies, wanders, hostile, carnivorous, infravisible; generated in small groups (`G_SGROUP`), uncommon (frequency 2, genocidable). It is a bat for `is_bat()` (so it flutters erratically as bats do and is not counted as a bird; `mondata.h`) and its corpse stuns like a giant bat's (`eat.c`). **A boomerang kills a keese outright**, whether thrown or swung: in `hmon_hitmon()` (`uhitm.c`) the damage of any boomerang hit on a keese is raised to its current hit points. Adding a monster moves every later monster number up by one, so old saves and bones are not compatible; the data files and utilities must be rebuilt as in Batch 79.

**Parrots** are now orange (they were red), so they no longer look like giant bats. Colour only. The item, use and corpse stress tests ran clean with the keese (20,438, 12,937 and 6,161 log lines). Checked in wizard mode: a Valkyrie swinging a boomerang killed a keese in one blow.

**Ninja sword skills swapped:** the ninja-to's long sword skill now goes up to **Expert** and curved sword to **Skilled** (they were Skilled and Expert), in `Skill_N[]` (`u_init.c`). Everything else in the Ninja's skill list is as in Batch 103.

## Batch 106: the dao

A new curved sword, the **dao** (`objects.c`, after the messer; `P_CURVED_SWORD`, iron, always identified): damage **d6+d4** against small and large (`oc_wsdam` and `oc_wldam` are 6 and the +d4 comes from `dmgval()` in `weapon.c`, as for the kriegsmesser's +d6), weight 50 (the falchion is 45), cost 80 (the katana's), generation probability 8. The class must still add up to 1000, so the scimitar goes from 14 to 11, the falchion from 15 to 12 and the messer from 10 to 8. Monsters that pick a weapon rank it just below the rapier (`hwep[]`), and a Pirate's sacrificed curved sword can turn into Reaver from a dao as from a scimitar (`pray.c`). Every object number after it moves up by one, so old saves and bones are not compatible; the data files and utilities must be rebuilt as in Batch 79 (a game must not be running when the installed data files are replaced).

## Batch 107: scroll of shocking grasp

A new scroll (`objects.c`, after antimagic; label **THUNDEDE** in the table, shuffled like every other label; cost 100, ink 12 like confuse monster in `write.c`; generation probability 20, taken from scroll of identify, which goes from 150 to 130 so the class still adds up to 1000).

- **Reading it** (`read.c`): like confuse monster it does not identify itself, so after "Lightning crackles around your hands." (blind: "Your hands tingle.") the game asks you to name it. It sets `u.ushock` (new in `you.h`), a countdown of turns: **10-19 uncursed, 20-39 blessed**; reading another while charged adds to it ("even more"). Cursed, confused or not: the lightning arcs back for d10 (nothing if you are shock resistant). Confused and not cursed: half the time "Your hands are sheathed with rime" (the extra d10 is **cold** and monsters that resist cold take nothing: "is chilled!"), half the time "Your hands are wreathed in flame" (**fire**, fire resisters take nothing: "is scorched!"); `u.ushtype` (0 lightning, 1 rime, 2 flame) holds which, a second scroll of the same kind adds time ("even more"), a different kind replaces the old charge, and the expiry message and enlightenment line follow the kind. Blind readings say your hands feel cold or hot or tingle.
- **Effect** (`uhitm.c`, `hmon_hitmon()`): while `u.ushock` is running, every melee hit (weapon, martial arts, bare hands, not thrown or fired objects) does an extra **d10** unless the monster resists that element (electricity for the ordinary scroll) or is a shade, and prints "The X is shocked!".
- **Timeout** (`timeout.c`): one turn per move; "Your hands stop crackling with lightning." (blind: "stop tingling.") when it runs out. Enlightenment and the dump say "charged with lightning" (`cmd.c`).
- Every scroll number after it moves up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

Checked in wizard mode: reading it shows the message, a newt hit afterwards was "shocked" and killed; reading it confused gave the rime charge and hits were "chilled". The flame charge, cursed, blessed and blind readings and the expiry messages were not tried. The three stress tests ran clean.

## Batch 108: BEEEEEEEEEEEEEEEEES

Reading a **scroll** of create monster (the spell is unchanged) while both **confused and hallucinating** (`read.c`) prints "BEEEEEEEEEEEEEEEEES!" and makes a random `a` (`mkclass(S_ANT)`, so the usual difficulty limits apply) on every free square around you, up to eight, each **peaceful or hostile with equal chance** (`mpeaceful = rn2(2)`, then `set_malign()`). The scroll is identified. Nothing is created in squares that are blocked or occupied. Checked in wizard mode (confused and hallucinating Valkyrie: the message appeared and a ring of monsters with it); the three stress tests ran clean (they do not read scrolls while hallucinating, so this is only a sanity check). Only the binary changes.

## Batch 109: ghast

A new monster (`z`, the "weird monsters" class with the zruty, right after the zruty in `monst.c`, **orange**): level 3, speed 12 (the ghoul is 6), AC 8, alignment -2, a single bite of 1d6; resists cold, sleep and poison, vulnerable to fire, breathless, mindless, wanders and is hostile like the ghoul, but it is **not undead** (it cannot be turned). It is generated in small groups (`G_SGROUP`) at frequency 3 ("moderately common"; the ghoul is 1), genocidable, and leaves a corpse (poisonous to eat). It is modelled on the ghoul but does not have its special cases (paralysis, eating only old corpses, rising from your grave, disease immunity). Monster numbers after it move up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

## Batch 110: two more merozalghi, a weaker byakhee

- **Byakhee** is level **13** (was 22), a little above the nightgaunt's 12. Its speed, armour class and attacks are unchanged (bite 2d8, claws 2d6 x2, poison sting 1d10), so its damage is what it was but it is easier to kill and has fewer hit points.
- **Lesser merozalghu** (`z`, magenta, before the merozalghu in `monst.c`): level 8, speed 12, AC 4, alignment -6, bite 1d10, weapon 1d10, acid breath 2d12. Small groups, frequency 2; humanoid, strong and a collector of items like the merozalghu, and it is given a mace, morning star or falchion three times in four (`makemon.c`, as the merozalghu is).
- **Accursed merozalghu** (`z`, red, after the merozalghu): level 20, speed 14, AC -5, alignment -10, bite 3d10, tail 2d10 (the `AT_TAIL` attack of the tyrannosaurus), acid breath 4d12. Alone, frequency 1, nasty and strong; it carries no weapon.
- All three merozalghi are vulnerable to cold (`M4_VULNERABLE_COLD`, as Batch 71 gave the merozalghu) and **Pernicimordax now does double damage to all three** (`artifact.c`). The colours of the two new ones are my choice, as are the alignments, the monster weights and the lack of a weapon on the accursed one.
- Monster numbers after the lesser merozalghu move up by two; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

Checked in wizard mode: all three were created and attacked (the lesser one picked up a mace); the three stress tests ran clean.

## Batch 111: four more random scroll labels

`objects.c` gets four more unnamed scroll appearances at the end of the shuffled list (after ANVIL BONK): **ABRA CADABRA, HOCUS POCUS, HOCUS CADABRA, ABRA CA POCUS** (supplied by the player). Like the other spare labels they have no scroll behind them (probability 0): they only make the pool of labels larger than the number of scrolls, so which labels turn up in a given game varies a little more. "stamped" (mail) and "unlabeled" (blank paper) stay last with their fixed descriptions. Every object number after the last spare label (mail, blank paper and everything beyond) moves up by four; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

Checked: the game starts, a scroll reads normally, the labels are in the binary, and the three stress tests ran clean.

## Batch 111b: shallow water, acid pools, yellow dragon scale mail

(Batch 111 above is the four spare scroll labels; this is the next change.)

**Yellow dragon scale mail** (and scales) now also give **stoning resistance** (`do_wear.c`: `EStone_resistance` on and off with the stun and confusion resistance), shown as "petrification resistant" in enlightenment.

**Two new terrain types** (`rm.h`, added after `CLOUD` so no existing level type is renumbered; new map symbols `S_shallow` and `S_acidpool` after `S_water`, which shifts the later symbol numbers, so old saves and bones are not compatible). Both are ordinary walkable floor as far as the rest of the game is concerned: `is_pool()` is false, objects stay visible, nothing drowns. `dbridge.c` has `is_shallow()` and `is_acid_pool()`.

- **Shallow water**: `}` in **cyan**, "shallow water". No drowning, no need to swim or levitate. Walking into it from dry ground prints "You wade into the shallow water." and, one step in eight, rusts iron boots you are wearing. Objects dropped, thrown or landing on it take the same water damage as in a pool (`water_damage()`: scrolls blank, potions dilute, iron rusts) but stay where they are. It cannot be dug (down, or with wands/pick) because it is muddy and you cannot engrave on it ("You can't write on the water!"). Eels move through it without suffering; everything else treats it as floor. The wiki page for it (a patch terrain from L's Webbed Space) says little more than that standing water shallow enough for a leprechaun to wade, wetting dropped items and undiggable, so the details beyond that are my choice.
- **Acid pool**: `}` in **bright green**, "acid pool". Shallow like the water but each turn you spend in it (`acid_pool_burn()`, called from the once-per-turn part of `moveloop`) the acid does d10 damage (half, rounded up, if you wear boots; nothing if acid resistant or levitating/flying: "The acid burns you!", killed by "wading in acid"). Entering it asks "Really step into that acid?" (the `swimconfirm` option) unless you are acid resistant, and travel and running avoid it. Boots that can corrode or rot do so one step in three, and each turn you spend in it a shield, and the body armor (or the cloak covering it), has a one in six chance each of corroding or rotting, acid resistant or not (`acid_pool_burn()`); objects dropped or landing in it corrode (`_erode_obj`, AD_ACID), scrolls dissolve ("The scroll dissolves in the acid!"). You cannot dig it or engrave on it. **Monsters** (`mon.c`, `teleport.c`) never choose to enter one and are not placed in one unless they fly, float, cling, resist acid or are acidic; one that ends up in it takes d10 per move and may dissolve.

**Where they come from** (`mklev.c`)
- **Pool rooms** (the rare special room that rings itself with pools): were 80% lava, 20% water; now **60% lava, 20% water, 20% acid**.
- **`mkshallows()`**, run after every level is made: in the **main dungeon** (not Medusa's lake or the Castle moat) every connected body of `POOL`/`MOAT` has a **one in three** chance of becoming shallow water all the way through, skipping any body with a swimmer such as an eel in it. On the quest levels that have a **river** (Barbarian, Bard, Tourist) a third of the river's squares become shallow instead, so it has fords. Acid pools only come from pool rooms. The other quests' water, Medusa, the Castle, Ludios and Gehennom are unchanged.

`pager.c`: farlook's symbol list says "shallow water" without an article, like "water". Monster and level numbering are unchanged; the `S_` symbol numbering moved (see above), so rebuild the data files as in Batch 79.

Checked in wizard mode with a test hook (not kept): shallow squares show as cyan `}` "shallow water", the wade message and boot rust appear, dropped scrolls came back "unlabeled", engraving is refused; acid squares ask before entering, burn each turn and corrode the iron shoes, a pool room produces lava and acid rings. The three stress tests ran clean, but they never meet the new terrain. Not seen: monsters walking in or avoiding acid, the quest river fords, a shallow-water ring in a pool room, eels in shallow water.

## Batch 112: the Merozalghi Realm, Miralt, and two more merozalghi

**New monsters** (`monst.c`, all `z`; `is_merozalghu()` in `mondata.h` now names all five ordinary merozalghi and Pernicimordax doubles damage against each):
- **Lesser merozalghu soldier** (bright magenta): level 10, speed 12, AC 2, alignment -6; bite 1d12, weapon 2d8, acid breath 3d12. Always generated in **bronze plate mail** with a sword: half the time a messer, otherwise a long sword, short sword, broadsword, scimitar, falchion, rapier or saber (`makemon.c`).
- **Merozalghu captain** (bright magenta): level 16, speed 12, AC -1, alignment -8; the merozalghu's attacks with the weapon at 3d6 (bite 1d12, four 1d8 tentacles, breath 4d12). Always in bronze plate mail with a sword: half the time a dao, otherwise a long sword, broadsword, scimitar, falchion, katana, two-handed sword, rapier or saber.
- **Miralt** (magenta, unique, not randomly generated, no corpse): level 52, speed 16, AC -6, MR 80, alignment -12 (Yeenoghu is 56, 18, -5, 80, -15); weapon 3d8, weapon 2d8, bite 2d10, **clerical spells** (`AT_MAGC`/`AD_CLRC` 2d8), **acid breath 6d12**; resists acid, poison and sleep, sees invisible. Every melee round there is the demons' one in thirteen chance that he **gates in merozalghi** (`msummon()`, `mhitu.c`): one, or two a third of the time, of the five kinds, hostile and beside you; not if they are extinct. Hurt below a third of his hit points he **teleports to the stairs** (up, else down) with a one in three chance each move, flees for 10-19 turns and recovers d6 hit points a move while he stands on them (`monmove.c`). He is always generated with a **random neutral or chaotic artifact weapon** that has not been made yet (`mk_miralt_artifact()` in `artifact.c`: any weapon or weapon-tool artifact of neutral or chaotic alignment that is not no-generation, quest or already existing), or a **tsurugi** if none are left; if that is Heartseeker or Plague he also gets 20-30 **blessed poisoned +2 arrows**. Plus three scrolls of teleportation and two potions of extra healing (and whatever a monster of his kind picks up).

**The Merozalghi Realm** (`dungeon.def`, `dat/realm.des`): a branch of the Dungeons of Doom, **seven levels** ("Z"; chaotic, mazelike), entered by down stairs from a level anywhere from 12 down to the Castle. `parent_dlevel()` in `dungeon.c` avoids levels that already have a branch (as before) and, for this branch only, **any special level** (Medusa, the Castle, Oracle, bigroom, rogue level...), so it is never on the Mall's level or the Castle's. Checked over 80 new dungeons: levels 12 to 28, never on a branch or special level. Levels 1-2 and 4-6 are the fill level, 3 is **Skunj**, 7 the **Citadel**.
- **Fill level** (`merofill`; `makelevel()` loads it for any non-special level of the realm, `In_realm()`, `realm_dnum`): a lit-or-dark cave (the mines' generator) with **a river of acid** across it (running top to bottom, and half the time a second running left to right; two to six squares wide, `ACID_POOL`), **3 random weapons, 2 random armors, 4 random scrolls and a random wand**, and monsters: 2-4 lesser merozalghi, 2-3 merozalghi, half the time an accursed merozalghu and a soldier, a third of the time a captain, 40% a shoggoth, 20% a giant shoggoth, 1-3 ghasts, 60% a gug, 5-8 depth-appropriate random monsters, traps. `MON_GENERATION` makes 40% of later random monsters one of the realm's kinds.
- **Skunj** (`skunj`, "town"): a cave town, after the mines' Bazaar Town: **two general stores**, one other shop (a random one of tool, candle, food, weapon, armor, scroll or wand shop), a **temple with a chaotic altar and priest**, a plain house, three fountains, acid rivers in the caves at either end, and **two cages** (three-by-three blocks of iron bars with a locked door, a soldier inside, **peaceful nine times in ten** each). The soldiers are ordinary `@` soldiers. The level also has the usual 3 weapons, 2 armors, 4 scrolls and wand and a population of hostile merozalghi, a shoggoth or two, a ghast and perhaps a gug.
- **Citadel Miralt** (`citadel`, the bottom, no way down; castle-like like the Samurai and Ninja quests' goal levels): a walled citadel with gates north and south, a throne hall in the middle and a locked **side room** at each end, two acid ponds in the yard. **Miralt sits on the throne**, flanked by eight random merozalghi (any of the five kinds), with **twelve more** about the halls and corridors. Each side room has a **locked chest** holding a random wand (it can be a wand of wishing, at the ordinary one in two hundred chance), a **cursed potion of gain level**, a **blessed scroll of shocking grasp**, three random foods, and half the time 1-20 **blessed +1 silver arrows**. Teleporting on the level is allowed (so his scrolls work). The usual 3 weapons, 2 armors, 4 scrolls and a wand lie about too.

**Tool changes** that go with it: `lev_comp` map letters **`a`** (acid pool) and **`w`** (shallow water) (`lev_main.c`, `lev_comp.l`); the Makefiles build `realm.des` and list `merofill.lev skunj.lev citadel.lev`; **`dlb` could only hold 200 files** (`MAX_DLB_FILES`) and silently left out the last one, which with three more levels was `vaults.dat`, so level creation looped forever; it now holds 400. `dgn_topology` gained `d_realm_dnum`. Monster numbers after the lesser merozalghu change (three more entries, Miralt last in the `z` class) and the map-symbol, dungeon and level files change, so old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

Colours, alignments, speeds, the soldier's and captain's weapon lists, Miralt's arrow count and the spell and teleport rules are my choices where the request did not say.

Checked in wizard mode: the branch stairs lead to the realm; the fill level, Skunj (shops, temple, cages with soldiers) and the Citadel (throne hall, chests locked with the stated contents) generate; Miralt fights with spells, acid breath and summoning and teleports away when hurt; a created Miralt carried Magicbane, scrolls and potions. Not seen: Miralt with Heartseeker or Plague and his arrows, the "tsurugi when none are left" case, a captain's or soldier's gear in play, the 10% hostile cage soldiers, Pernicimordax against the new two. The three stress tests ran clean (they do not visit the realm).

## Batch 113: merozalghu warlock

A new `z` (`monst.c`, between the captain and the accursed merozalghu; **bright blue**): level 14, speed 12, AC 0, alignment -8, magic resistance 50, a solo monster (frequency 1, no group), vulnerable to cold like the other merozalghi. It trades the merozalghu's weapon and acid breath for **mage spells**: bite 1d12, two 1d8 tentacles and `AT_MAGC`/`AD_SPEL` 2d6; nasty, strong and picks up magic items. It is one of the kinds Miralt gates in (`minion.c`), Pernicimordax does double damage to it (`is_merozalghu()`), and in the realm (`realm.des`) the fill level and Skunj place it 40% of the time, it joins the realm's random monster mix (weight 2) and the six-way random pick for the Citadel's guard of honor and the dozen about the halls.

Monster numbers after the captain move up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79. Checked in wizard mode: a created warlock bit, sucked with its tentacles and cast a spell (it hasted itself); the Realm levels compile with it. Not seen in the realm itself. The three stress tests ran clean.

## Batch 114: merozalghu shaman

The warlock's clerical twin (`monst.c`, after the warlock; **bright cyan**): level 14, speed 12, AC 0, alignment -8, magic resistance 50, solo, vulnerable to cold; bite 1d12, two 1d8 tentacles and `AT_MAGC`/`AD_CLRC` 2d6 (clerical spells, as Miralt casts). Placed exactly as the warlock is: one of the kinds Miralt gates in, doubled by Pernicimordax (`is_merozalghu()`), 40% on the realm's fill levels and in Skunj, weight 2 in the realm's random monster mix and a seventh choice for the Citadel's guards. Monster numbers after the warlock move up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79. Checked in wizard mode only that a created shaman bites and sucks with its tentacles (the hero died before it cast; the same spell code ran for Miralt in Batch 112). The three stress tests ran clean.

## Batch 115: merozalghu necromancer and cultist; merozalghi rend

**Tentacles rend** (`mhitu.c`, `mhitm.c`): the merozalghi's tentacle attacks now read "The merozalghu's tentacles rend you!" (`is_merozalghu()`; every other tentacle attack still says "suck").

**Merozalghu necromancer** (`monst.c`, dark **gray**): the warlock's relative at level 14, speed 12, AC 0, alignment -10, magic resistance 50 and resists cold, sleep and poison (not cold-vulnerable like the rest); bite 1d12, two 1d8 tentacles, `AT_MAGC`/`AD_SPEL` 2d6. Half its casts (`mcastu.c`) are the new mage spell **summon undead** (`MGC_SUMMON_UNDEAD`, undirected like summon nasties, never cast by peacefuls or when it can't see you): one to three undead around you, each a random zombie (40%), mummy (30%), wraith (20%) or vampire (10%) from `mkclass()`, so the usual depth limits apply: "The dead rise around you!".

**Merozalghu cultist** (**yellow**): level 12, speed 12, AC 2, alignment -8, cold-vulnerable; it has a **weapon attack** 2d6, a bite 1d10 and two 1d8 tentacles, and is always given a mace, morning star or falchion. Each melee round it has a **one in six** chance (demons: one in thirteen) of `msummon()` calling up one random **`z` monster** (`mkclass(S_ZRUTY)`, so any ordinary `z`, not just merozalghi): "The merozalghu cultist chants, and something answers!".

Both are in `is_merozalghu()` (Pernicimordax, rending), are among the kinds Miralt gates in, and appear in the realm like the warlock and shaman: the fill levels and Skunj place the necromancer 25-30% and the cultist 35-40% of the time, both have weight 2 in the realm's random monster mix, and the Citadel's guard pick is now one of nine. Monster numbers after the shaman move up by two; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79. Colours, the spell mix, the undead mix and the one-in-six rate are my choices.

Checked in wizard mode: tentacles rend; a created cultist wielded a mace and chanted ("something answers" appeared); a necromancer's undead spell (forced for the test with a temporary hook, not kept) raised a zombie. Not seen: the necromancer choosing it unforced, wraiths or vampires from it, what a cultist actually summoned. The three stress tests ran clean.

## Batch 116: snipion

**Snipion** (`monst.c`, **cyan** `s`): a scorpion-like arachnid with big pincers instead of a stinger. Level 5, speed 15, AC 2 (the scorpion is AC 3), frequency 2, same size and the same concealing, oviparous, eagle-eyed build; two claw attacks of **1d8** each (the scorpion's are 1d2) and **no sting and no poison**, so the claws are the whole threat (average 9 a round against the scorpion's 5.5 plus poison). It is not poisonous to eat. Its claw hits read "The snipion snips you!" (`hitmsg()` in `mhitu.c`; "snips" for monster-on-monster fights in `mhitm.c`); every other claw attack still says "hits". It sits right after the scorpion in the `s` class, so monster numbers after it move up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

The request said "slightly AC" without a direction; I took it as slightly *better* AC (2 against 3), and the 1d8 claws, level and lack of a sting are my choices too.

Checked in wizard mode: a created snipion attacked with "The snipion snips you!" and killed a 16-HP starting wizard in a few rounds. Not seen: monster-on-monster "snips". The three stress tests ran clean.

## Batch 117: Gehennom-only demons stay out of the upper dungeon

A weredemon (`G_HELL`) killed a player on DL12. Ordinary generation honours `G_HELL` (`uncommon()`), but `mkclass(S_DEMON)` does not, and two places pick a random non-lord demon with it: graveyards (`morguemon()`, difficulty 12 and up) and demon summoning (`ndemon()`, used by `msummon()`, `summon_minion()` and the like). In vanilla that only let devils and the like turn up early; with the weredemon in the demon class it can too.

`ndemon()` (`minion.c`) now rejects any `G_HELL` demon unless you are in Gehennom or the endgame, so graveyards and summoning demons outside Gehennom can only produce the non-hell demons (succubus, incubus). If it finds none in its 20 tries it returns `NON_PM`, as it always could. `morguemon()` (`mkroom.c`) used `&mons[ndemon(A_NONE)]` unchecked, which would index `mons[-1]` on a miss; it now falls through to the usual ghost, wraith, vampire or zombie. Lords and princes were already kept out of `ndemon()`. Only the binary changes; saves and bones still load.

Checked with a temporary hook (not kept) calling `ndemon()` 20,000 times on DL1: no hell-only demon came back and about 28% returned none. The three stress tests ran clean. Not seen in play: a graveyard on DL12 or deeper.

## Batch 118: the mimic shop

A **mimic shop** is a general store in every respect but its stock: the shopkeeper greets you with "general store", the goods are random objects, and the shop shows up as a general store in the overview. At least **half** of the stocked squares hold a **mimic** instead of an item, and often a few more.

It is a new room type, `MIMICSHOP` (28, `mkroom.h`), with a matching `shtypes[]` entry in `shknam.c` named "general store" (probability 0, so it is never rolled as an ordinary shop), a "mimic shop" name for `.des` files (`lev_main.c`) and a case in `shop_string()` (`dungeon.c`). `stock_room()` counts the squares that get stock (the new helper `shop_square()` holds the old per-square door test) and picks `(n+1)/2` plus up to `n/4` of them at random, front to back, so the half is exact rather than a per-square chance; if a mimic cannot be placed on a square (a monster is there, or `mkclass(S_MIMIC)` finds none) the debt carries to the next square. `mkshobj_at()` now returns whether it made a mimic and takes a force flag; in a mimic shop the mimics always take the look of a shop item instead of sometimes showing as a strange object. Mimic species still come from `mkclass(S_MIMIC)`, so they follow the level difficulty as before.

Where it appears:
- **Random shops** (`mkshop()`, `mkroom.c`): below depth 20, one random shop in six is a mimic shop, whatever type it would have been.
- **Skunj** (`realm.des`): a one in three chance that the east general store is the mimic shop.
- **Orcus Town** (`gehennom.des`): a one in two chance that the north-west shop is the mimic shop.

The probabilities are my choices. Old saves still load (the new room type is only ever set on new levels) but the data files must be rebuilt as in Batch 79 because the level files change.

Checked in wizard mode with a temporary test hook (not kept) that made every shop a mimic shop: four generated shops had 13 of 20, 6 of 10, 5 of 6 and 22 of 40 squares as mimics. The three stress tests ran clean. The two `.des` changes compile into the data files but I did not see them generate in play, and I did not see the one-in-six rule fire below depth 20.

## Batch 119: a smaller Mall

The Mall (`dat/mall.des`) had twelve shops and was handing out too many artifacts. It now has **eight**, **three of them general stores**: the concourse is four shops wide instead of six (the level is 51 columns across instead of 75), two rows of four, each shop the same 11 by 5 size as before, so each one stocks as it did and the total stock is about two thirds of what it was. The row of doors, the fountain in the middle of the concourse, the up stairs at the west end, the down stairs at the east end, the watchmen and the random monsters are as before.

The shops, north row then south row, west to east: general store, weapon shop, armor shop, wand shop; potion shop, general store, scroll shop, general store. The food, ring, book, tool and candle shops are gone (the general stores sell all of those, and a deli, a jeweler, a bookstore, a hardware store or a lighting shop is no longer guaranteed). Which five specialist shops stay, and where, is my choice. Mimics, which come from the depth of the level, are unchanged. Only the data files change; saves and bones still load, but a Mall level you have already visited keeps its twelve shops.

Checked in wizard mode (a temporary hook, not kept, that sent the hero to the Mall): the level maps as eight shops with the right fronts, a fountain, stairs at both ends, and the overview lists a scroll shop. The three stress tests ran clean. Not counted: the artifacts a typical visit now yields.

## Batch 120: slower glue golems

The glue golem's speed goes from 9 to **7** (`monst.c`), the same as a clay golem; the rest is unchanged (level 5, AC 8, sticky claw 3d4). A hero at speed 12 now gets about five moves to its three. The monster numbering does not change, so saves and bones still load. The corpse stress test ran clean (it makes and kills every monster type, glue golems included).

## Batch 121: starting gold for Bards and Pirates

Until now only the Healer (1001 to 2000) and the Tourist (1 to 1000) started with gold (the Rogue's is explicitly zero, as in vanilla). The **Bard** now starts with **100 to 350** gold (`rn1(251, 100)`) and the **Pirate** with **100 + 5d10**, so 105 to 150 (`u_init.c`). Both go through the same starting-money path as the Tourist and Healer, so it shows in the inventory as gold pieces and counts as the starting amount for the score. Only the binary changes; saves and bones still load.

Checked in wizard mode: five Bards started with 134, 202, 238, 285 and 311 gold (plus two more, 209 and 315) and six Pirates with 121 to 142. The items and use stress tests ran clean.

## Batch 122: blue light

A new light (`y`, **blue**), `blue light` in `monst.c` between the yellow and black lights: level 4, speed 15, AC 0, **rare** (frequency 1; yellow is 4, black 2), no corpse, flies, mindless, and resists everything the other lights do. Its attack is an explosion, `AT_EXPL`/`AD_CONF` 6d10 (`explmu()`, `mhitu.c`): you are **confused** for that many turns (about 33, added to any confusion you already have) with the message **"You feel addled!"**. Blindness, confusion resistance and being a blue light yourself keep it out ("You seem unaffected by it."). Like the other lights it gives light, is amorphous (flows under doors, like the yellow light) and is visible, unlike the black light.

Against other monsters (`mdamagem()` in `mhitm.c`, only for `AT_EXPL`, so the existing `AD_CONF` attacks are unchanged) it confuses any monster that has eyes and can see, with no damage; if you explode as a blue light yourself (`explum()`, `uhitm.c`) the same happens to the monster you hit. Monster numbers after it move up by one, so old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79.

Checked in wizard mode: a created blue light exploded with "You feel addled!" and the status line showed Conf. The corpse, items and use stress tests ran clean. Not seen: it being blocked by blindness, monster-on-monster confusion, a self-made blue-light explosion. The level, frequency, duration and blue colour are my choices.

## Batch 123: orange and green lights

Two more lights (`y`, `monst.c`), sharing the other lights' build (fly, mindless, no corpse, amorphous, visible, immune to everything the yellow light is).

**Orange light** (**orange**, level 5, speed 15, AC 0, rare: frequency 1, hostile): its explosion, `AT_EXPL`/`AD_STUN` 3d8, **stuns** you for that many turns (about 13) with the usual "You stagger...". Blindness or being an orange light yourself keeps it out. Against a monster it stuns (no damage); if you explode as one, the monster you hit is stunned.

**Green light** (**green**, level 3, speed 15, AC 0, frequency 2): the explosion, `AT_EXPL` with the new `AD_CURE` (`monattk.h`) 2d6, **cures** whatever of confusion, blindness (the timed kind), hallucination, stun and illness (both disease and food poisoning, `make_sick(0, .., SICK_ALL)`) you have, "A soothing green glow washes your afflictions away.", and then does its damage as an ordinary blast (a Dexterity check halves it; Half_physical_damage halves it again; there is no resistance to it). It is **peaceful nine times in ten** whoever you are (`peace_minded()`, `makemon.c`; the usual alignment test would only make it peaceful to neutrals, since its alignment is zero). A peaceful one never explodes, so to be cured you have to attack it and confirm (a point of alignment for angering a peaceful monster); once hostile it explodes on its next turn, and since it dies by its own explosion you are not charged for killing a peaceful. Against another monster the blast cures confusion, stun and blindness and does its 2d6 damage; the same when you explode as one.

Both slide under doors like the yellow light (`monmove.c`). Monster numbers after the green light move up by one and after the orange light by one more; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79. Colours, levels, frequencies, durations, damage and the nine in ten are my choices.

Checked in wizard mode (a temporary hook, not kept, made green lights hostile): an orange light stunned ("You stagger...", Stun on the status line); a blue light's confusion plus that stun were both cleared by a green light, which then did 2 damage after a ducked blast; ten green lights made without the hook never exploded. The corpse, items and use stress tests ran clean. Not seen: blindness, hallucination or illness being cured, the monster-on-monster effects, the hostile one in ten in normal play.

## Batch 124: lightning cat, and merozalghi that cannot leave felines alone

**Lightning cat** (`f`, **orange**, `monst.c`, right after the large cat): the large cat's stats (level 6, speed 15, AC 4, bite 2d4, small, frequency 1, domestic and strong) plus a **lightning breath of 1d10** (`AT_BREA`/`AD_ELEC`), and shock resistance that its corpse can confer. It is **peaceful four times in five** whoever you are (`peace_minded()`); being `M2_DOMESTIC` it is **tamed with food like a large cat** (throw it something it likes). The fork scales a monster's breath to a number of d6, so 1d10 would have come out as 1d6; a lightning cat's breath is therefore rolled with real d10s: `zap.c` has a new `ray_sides` (6 unless set) that its ray damage uses, and `breamu()` (`mthrowu.c`) sets it to 10 for the cat's breath and back again. A bouncing bolt can still hit twice. A peaceful cat never breathes, and a tame one fights at close quarters, where breath is never used, so in practice only the hostile one in five breathes.

**Merozalghi and felines** (`is_merozalghu()`, so every kind but Miralt): they **attack an `f` in preference to anything else, you included.** Any feline they can see within seven or so squares, yours or peaceful, kitten or tiger or lightning cat, becomes their goal: `mm_aggression()` (`mon.c`) now lets a merozalghu attack an `f` next to it, and the new `feline_hunt()` (`monmove.c`, called from `dochug()` ahead of the normal move and attack) points `m_move()` at the nearest one through a one-shot goal override, even when you are standing next to it. They also go for **feline corpses** (any `f`'s) within about seven squares, nearest first and a corpse wins a tie with a live prey, and **stop whatever they are doing to eat one**: "The merozalghu eats a lightning cat corpse!", `meating` turns of 2 plus one per 32 weight units (about nine for a cat; it makes no attacks meanwhile) and a little healing. If the prey cannot be reached it goes back to attacking you; a blinded, confused, stunned or trapped one and a tame one do not hunt. Peaceful merozalghi hunt too.

Monster numbers after the lightning cat move up by one; old saves and bones are not compatible and the data files and utilities must be rebuilt as in Batch 79. The rarity of 1 (a "fairly rare" cat; it cannot go lower than the large cat's), four in five peaceful, the corpse eating times and the seven-square range are my choices.

Checked in wizard mode with temporary hooks (not kept): a made hostile lightning cat breathed at me and the damage sampled 1 to 10 per hit (a bounce twice gave up to 17); a merozalghu bit and hit a lightning cat and killed a kitten before it turned to me, and ate a lightning cat corpse put near it. The corpse, items and use stress tests ran clean. Not seen: a tamed lightning cat, the corpse's shock resistance, merozalghi other than the plain one, eating when the corpse was not in sight.

## Batch 125: scroll probabilities (less amnesia and punishment; more ice, shocking grasp, enchant armor, remove curse, enchant weapon)

In `objects.c` the scroll probabilities (out of 1000 for the class) changed: **amnesia 35 -> 20**, **punishment 15 -> 12**; the 3 points from punishment and 5 from amnesia went to **ice 20 -> 23**, **shocking grasp 20 -> 23** and **enchant armor 63 -> 65**, and the further 10 from amnesia split between **remove curse 65 -> 70** and **enchant weapon 70 -> 75**. The scroll class still sums to 1000, which `init_objects()` checks at startup. Nothing was renumbered, so existing saves and bones still load; only newly generated scrolls are affected.

The three stress tests (corpse, items, use) ran clean. The new odds themselves were not sampled in play; it is a change to table constants.

## Batch 126: bards, pirates and ninjas disarm traps like rogues

`trap.c` has a small `good_untrapper()` test (rogue, bard, pirate or ninja) that replaces the three rogue-only checks on disarming: the floor-trap failure roll in `untrap_prob()` (a `ulevel/60` chance of one less on the roll), the trapped-chest skill (`Dex + level`, doubled) and the trapped-door skill (`15 + 3 * level` rather than `15 + level`). The extra reduction for carrying the Master Key of Thievery stays **rogue-only**, as asked. The ranger's separate flat bonus on floor traps is unchanged. Nothing was renumbered; saves and bones are unaffected.

The three stress tests ran clean and it compiles; the disarm chances were not sampled in play.

## Batch 127: racial launcher and melee weapon bonuses

Two new helpers in `weapon.c`: `racial_launcher_shot(ammo, launcher)` and `racial_melee_weapon(weapon)`. Neither applies while polymorphed.

- **+1 to hit and +1 damage when firing the race's own kit** (`dothrow.c` for the to-hit roll, `uhitm.c` for damage): **orcs** with orcish arrows from an orcish bow, **gnomes** with crossbow bolts from a crossbow (any crossbow), **dwarves** with rocks or flint stones from a sling. They stack with the existing elf/samurai bow bonuses (which are unchanged) and with ordinary launcher skill and enchantment.
- **+1 to hit, no damage bonus, when wielding a racial melee weapon** (`find_roll_to_hit()`): elves with elven spear, dagger, short sword or broadsword; orcs with orcish spear, dagger, short sword or scimitar; dwarves with dwarvish spear, short sword, bearded axe or mattock. Only the wielded primary weapon counts. **Gnomes** have no gnomish melee weapons as such, so the **aklys**, the gnomes' own club, counts as theirs (added after the first version of this batch).

Checked in wizard mode with temporary markers (not kept): a dwarvish Valkyrie got the melee bonus with her dwarvish bearded axe and not with a dagger, and a dwarf slinging flint, an orc firing orcish arrows from an orcish bow and a gnome firing bolts from a crossbow each got both the to-hit and the damage bonus (including volleys). Not tried: elven/orcish melee weapons, two-weapon fighting (the off-hand weapon gets no bonus), and thrown racial daggers or spears (not "wielded", so no bonus). Nothing was renumbered; saves and bones are unaffected.

The aklys addition was checked the same way: a gnome wielding one got the +1 to hit and a human with the same weapon did not.

## Batch 128: bronze plate mail counts as orcish armor for orcs

In `find_ac()` (`do_wear.c`) an **orc** wearing **bronze plate mail** now gets the same racial bonus to its AC (+2) as for orcish chain mail and orcish ring mail. Only the AC bonus changed: bronze plate mail is not otherwise treated as orcish (`is_orcish_armor()` is untouched), so nothing else that cares about orcish gear, such as other races' wearing restrictions, is affected.

Checked in wizard mode (nothing kept): an orcish Barbarian in bare skin wearing a +0 bronze plate mail reached AC 2, a human Barbarian AC 4. Nothing was renumbered; saves and bones are unaffected. The three stress tests ran clean.

## Batch 129: parrots are omnivores

The parrot (`monst.c`) now has `M1_CARNIVORE` as well as `M1_HERBIVORE`, so a tame parrot eats meat and corpses as well as vegetarian food. Monster numbering is unchanged, so saves and bones are unaffected. It is still not `M2_DOMESTIC`: it cannot be tamed by throwing food, only by a scroll or spell of taming, charm and the like.

Checked in wizard mode (nothing kept): a parrot made tame with a scroll caught and ate a thrown tripe ration, which a herbivore would not. The three stress tests ran clean.

## Batch 130: parrots can be tamed with food; the macaw

**Parrots are now `M2_DOMESTIC`**, so throwing one something it likes to eat tames it, as with a dog or cat; being omnivores (Batch 129) almost any food will do. Whether a wild one is peaceful or hostile is unchanged.

**Macaw** (`B`, bright blue, `monst.c`, right after the parrot): a somewhat stronger parrot: level 4 (parrot 2), speed 20, AC 5 (parrot 6), a bite of 1d6 and a claw of 1d3 (parrot: one bite of 1d4), small and weighing 60 (parrot tiny, 30), frequency 1. It has the parrot's flags: flies, animal, no hands, lays eggs, carnivore and herbivore, wanders, domestic, so it too can be tamed with food. **Parrots grow up into macaws** (`grownups[]` in `mondata.c`, the same way a kitten becomes a housecat), and a macaw's egg hatches back into a parrot.

The monster numbers after the macaw move up by one, so old saves and bones are not compatible and the data files and utilities must be rebuilt as for Batch 79.

Checked in wizard mode with a temporary hook (not kept): a wild parrot became tame when I threw it an apple ("The parrot devours an apple."), and a tame parrot made to gain levels turned into a macaw at level 4 and then went on gaining levels as a macaw. The three stress tests ran clean. Not seen: growth by killing monsters in ordinary play (the same code path as every other growing pet), and hatching a macaw egg.

## Batch 131: weapon gifts from the gods favour trained skills

When a god gives a gift for a sacrifice and it turns out to be a plain weapon (half the time; the other half is armor, and the rare artifact gift is separate), `dosacrifice()` (`pray.c`) now picks it with `pick_gift_weapon()`: **three times in four the weapon is in a skill you already have at Basic or better**, and the other quarter it is any weapon in a skill you are not restricted in (the old rule). If the hero has no skill at Basic or better the 75% case gives up after 250 tries and settles for any unrestricted weapon, so it can never loop. Armor gifts, the odds of getting a gift at all, and artifact gifts are unchanged.

Checked in wizard mode with a temporary test hook (not kept) that drew 8,000 gift weapons: a Valkyrie got a Basic-or-better weapon 82% of the time (75% by design, plus the 25% unbiased picks that happen to land on one) and a Wizard 80%; a Tourist, with nothing at Basic in the gift range, got usable weapons every time (no restricted ones, no failures). The three stress tests ran clean. Not seen: an actual sacrifice gift in play. Nothing was renumbered; saves and bones are unaffected.

## Batch 132: a Bard casts every spell with Charisma

A Bard's casting stat (`urole.spelstat`, `role.c`) was still Intelligence, and `percent_success()` (`spell.c`) switched to Charisma only for enchantment spells. It is now **Charisma for every spell**: the role's stat is `A_CHA` and the enchantment special case is gone. Nothing else about the Bard's spellcasting (base, armor and shield penalties, emergency-spell bonus, the Sleep special spell) changed, and nothing else reads the role's casting stat. Reading a spellbook still uses Intelligence for every role, and the energy-regeneration formulae are untouched.

Checked in wizard mode: the failure rate of a level 2 attack spell (magic missile) came out at 85% for a Bard with Cha 16 and Int 8, and 96% for one with Cha 14 and Int 9, so it follows Charisma rather than Intelligence. (I could not get a ring of gain charisma to raise the stat in the test to compare the same character before and after.) The three stress tests ran clean. Nothing was renumbered; saves and bones are unaffected, but a Bard's displayed spell failure rates change at once.

## Batch 133: a Tourist casts every spell with Charisma

As for the Bard in Batch 132: the Tourist's casting stat (`urole.spelstat`, `role.c`) is now **Charisma** instead of Intelligence, for every spell. Nothing else about Tourist spellcasting changed (base, armor penalties, emergency-spell bonus, the Charm Monster special spell), and reading spellbooks still uses Intelligence.

Checked in wizard mode: a Tourist with Cha 15 and Int 12 had a 91% failure rate on magic missile (a level 2 spell); on Intelligence, 12 gives no chance at all and it would have been 100%. The three stress tests ran clean. Nothing was renumbered; saves and bones are unaffected, but a Tourist's displayed spell failure rates change at once.

## Batch 134: a Pirate casts every spell with Charisma

Like the Bard (Batch 132) and Tourist (Batch 133), the Pirate's casting stat (`urole.spelstat`, `role.c`) is now **Charisma** instead of Intelligence, for every spell. Nothing else about Pirate spellcasting changed (base, armor penalties, emergency-spell bonus, the Cause Fear special spell), and reading spellbooks still uses Intelligence. Every other role keeps its Intelligence or Wisdom casting stat.

## Batch 135: Witch, stage 1 -- sickle, broom, staff of enchantment, broomstick skill

First of six stages toward a new **Witch** role (see the design notes the user and I worked out: familiar, hexes, cauldron, the Boline of Cerridwen, a forest quest). This stage adds only the items and skill the role will lean on, all usable by existing roles. Objects were added, so this needs a full rebuild; old saves and bones are incompatible.

- **Staff of enchantment** -- the missing seventh school staff (alongside divination, healing, holiness, matter, escape and battle). Wielded, it gives +50 to the success chance of enchantment spells (`spell.c`, same code as the other staves).
- **Broom** -- a weak bludgeon (d4 / d3). When wielded it works as a staff of enchantment for spellcasting. It is a new weapon-class object with its own skill.
- **Broomstick skill** (`P_BROOMSTICK`, the last weapon skill, so every later skill number shifted by one): Expert for Witches later; Wizards cap at Skilled, Healers and Tourists at Basic; everyone else is restricted. 
- **Applying a broom** (`apply.c use_broom`) mounts it. Wielding is not required to ride. It is a status, not a steed (`u.ubroom`, kept in sync each turn by `broom_check`; losing the broom throws you off):
  - Restricted: you have no idea how to ride it. Unskilled: you cannot keep your balance, but each attempt trains the skill, so you can reach Basic.
  - Basic: levitation (at will; `>` or applying again dismounts).
  - Skilled: levitation plus Fast.
  - Expert: true flight plus Fast (`>` off the stairs lands you).
  - Riding practises the skill (every 20 turns). A cursed broom bucks you off half the time (d6 damage). You cannot ride while on a steed, trapped, swallowed, polymorphed into something handless or tiny, or already levitating or flying.
  - Levitation uses `ELevitation |= W_ARTI` (so it is "at will" like an invoked artifact); flight uses `EFlying |= W_ARTI`.
- **Sickle** -- a knife-skill blade, d6 against small and d3 against large, -1 to hit when thrown. Brooms are exempt from the "eroded tools may break when applied" rule.
- Object generation odds: arrows and darts lost 8 each to keep the weapon class summing to 1000 (staff of enchantment 5, sickle 8, broom 3).

Tested in wizard mode: unskilled apply trains, #enhance to Basic, mount/dismount at Basic, Skilled ("You feel swift") and Expert (flight, `>` lands); failure rate of charm monster (level 3, XL2 Wizard) fell from 100% to 51% with either the broom or a staff of enchantment wielded.

Still to come: Witch role core (Wit, Wisdom casting, titles, gods, kit), familiar and hex, cauldron, the Boline of Cerridwen, the quest.

## Batch 136: Witch, stage 2 -- the role

The **Witch** (`Wit`, menu key `W`) is now playable. A new player monster ("witch", numbered between the valkyrie and the wizard, so the player-monster range checks still hold) means another renumbering and full rebuild; old saves and bones are incompatible.

- **Who:** human, elf, gnome, orc or dwarf, either sex, any alignment the race allows (dwarf lawful, gnome neutral, elf and orc chaotic, human any).
- **Pantheon:** Crone (lawful), Mother (neutral), Maiden (chaotic).
- **Ranks:** Initiate, Hedgewitch, Herbalist, Brewer, Diviner, Scryer, Enchanter/Enchantress, Witch, Greybeard/Wise Woman.
- **Casting:** Wisdom; special spell Charm Monster. HP advance as the Priest, Pw advance as the Bard. Spell base 2, healing -2, shield 2, metal armor 10. Pet: kitten for now (stage 3 replaces it).
- **Intrinsics:** poison resistance at XL3, sleep resistance at XL6.
- **Skills** (`Skill_Wit`): dagger, knife, quarterstaff and broomstick Expert; short sword, unicorn horn, dart Skilled; mace and bow Basic; bare hands Basic. Spells: enchantment, healing, divination Expert; clerical and escape Skilled; matter and attack Basic.
- **Kit:** +1 uncursed athame (wielded), +0 broom (alternate weapon), +0 leather armor, +0 robe, 2 cram rations, three different herbs (garlic, carrot, eucalyptus leaf, wolfsbane; 1-3 each), sack, a random wand and ring, potions of healing and extra healing plus d4+2 random potions, blessed spellbook of sleep and a blessed random clerical, divination or healing book (level 3 or less). Her gnomish/elven/orcish race substitutions come from the usual machinery. She also starts knowing 1d6 more potion types, 3 scroll types and 1 spellbook type.
- **Curses:** she can always unwield a cursed weapon (it never welds) and take off cursed armor. Cursed rings, amulets and blindfolds still hold her.
- **Pw regeneration:** a little faster at night (up to 5%) and by moon phase (0-15%, full moon best), by shortening the regeneration interval.
- **Quest:** a placeholder until stage 6 -- `dat/Witch.des` is the Ranger quest renamed `Wit-*`, and the Witch's section of `quest.txt` is the Ranger's text. The leader, guardians and nemesis are Orion, hunters and Scorpius; the home-base name is already Medea's grove. (The goal artifact is the Longbow of Diana, which a Witch cannot handle.) Added to the quest-level lists in `sys/unix/Makefile.dat`.
- Also: Witches can appear as random player monsters (`mplayer.c`) and in the themed-room statue list (`mklev.c`); the character-creation menu (`charinit.c`) gained a `Witch` row.

Tested in wizard mode for all five races: kit, spells, skills, discoveries, titles by level, cursed helm removable, cursed dagger no longer welds, cursed ring still sticks, "You feel healthy!" at XL3, "You feel awake!" at XL6, and the `^V quest` teleport shows the placeholder quest message with Medea's grove.

## Batch 137: the broom is a weapon-tool

Like the pick-axe, the broom is now a **weapon-tool**: a `(` in the Tools class (`WEPTOOL`, damage d4/d3, broomstick skill) rather than a `)` weapon. Behaviour is unchanged (apply to ride it; wielded, it helps enchantment spells), and the Witch's starting broom still sits in her alternate-weapon slot. Generation odds moved with it: the broom's 3 points came back to darts (weapons) and out of tin whistles (tools), keeping both classes at 1000. Objects renumbered, so a full rebuild; old saves and bones are incompatible.

## Batch 138: Witch, stage 3 -- the familiar, the hex, and the familiar's eyes

New monsters (so another renumbering and full rebuild; old saves and bones are incompatible):
- **black kitten -> black cat -> large black cat** (a little magic resistant: 5, 15, 25), **lightning kitten -> lightning cat** (the cat already existed), **owlet -> owl -> giant owl** (flying, domestic), and **raven -> giant raven** (the raven existed, hostile; the giant is new). Growth is in `grownups[]` (`mondata.c`). Lightning kittens, like lightning cats, are usually born peaceful. Eating a black or lightning cat counts as the same cannibalism-style taboo as eating a housecat.

The **familiar bond** (`dog.c`) applies to a tame monster of one of these kinds (the kitten and housecat lines too), and only for a Witch:
- **Her starting familiar** depends on alignment: owlet (lawful), black kitten (neutral), raven (chaotic). The raven starts a good deal stronger than the others (it is a level-4 monster; the kitten and owlet are level 2).
- **It grows with her:** whenever she gains a level, each familiar on the level that is below her level gains levels (via `grow_up`, so it grows into the next form when it is big enough) until it matches her.
- **Its death costs power:** a quarter of her maximum Pw (at least 2), remembered in `u.ufamlost`; **taming any new familiar** (a charm monster spell, scroll of taming, the create-familiar spell, food) returns it all.

`#hex` (Witch only, 5 Pw): choose a direction; the first visible monster within 8 squares is hexed for 5-10 turns plus a few for level and Wisdom (`mhexed`): it moves a third slower, its attacks are -4 to hit, and its AC is 3 worse. The target gets a magic-resistance check against her level and Wisdom, and is woken (or angered) either way. Undead and mindless monsters can't be hexed (no cost); an empty line costs a turn but no Pw.

`#familiar` (Witch of XL7 or more, 8 Pw): shows everything within 8 squares that the nearest seeing, awake familiar can see, then returns to the normal map.

Tested in wizard mode: each alignment's pet; a kitten growing at XL2 and XL3 ("grows stronger along with you") and reaching large black cat by XL7; a familiar's death (max Pw 34 -> 26) and a scroll of taming a new kitten (back to 34); `#hex` on a newt (Pw 54 -> 49), a kobold zombie ("no mind to hex") and an empty line; and `#familiar` showing the cat's view of a room. The three stress tests ran clean. (A temporary hook to kill the familiar and a ray-free way to test it lived only in the build tree and has been removed.)

## Batch 139: Witch, stage 4 -- the cauldron

A new container tool, the **cauldron** (iron, 200 weight, odds 3; tin whistles gave up 3), so a full rebuild and incompatible saves. It sits in the container block of `objects.c` so everything that treats containers (put in, take out, #loot, weight) works on it. Apply it for a menu: **C**ontents, **F**ill, **B**rew, **S**tew, **G**aze.

- **Fill:** stand on a fountain (it may dry up) or next to a pool, moat or other water. A filled cauldron is shown as "water-filled" (`obj->spe`). Brewing, stewing and gazing spend the water.
- **Brew** (`potion.c brew_cauldron`): the potions inside are blended one after another with the dip-mixing recipes (`mixtype`, now exported). A herb (garlic, wolfsbane, eucalyptus leaf or carrot) in the cauldron blesses the result, or un-curses it. A Witch's brew never goes wrong ("The potions will not blend" and nothing is lost but the water); anyone else has a one-in-three chance per unmixable pair of an alchemic blast (d10 damage, brew lost).
- **Stew** (`eat.c stew_cauldron`): needs a lit lamp or candle carried. All corpses and ordinary food inside (not tins, eggs, lembas, cram, royal jelly) are cooked into a stew and eaten at once (nutrition up to 1200, a few turns). Cooking makes old and poisonous corpses safe; cockatrices, Medusa, green slime, were-creatures and your own kind are left in the pot. Each corpse may grant its intrinsic as if eaten (one chance in three for a Witch, one in six for others, on top of the corpse's own odds).
- **Gaze** (`detect.c scry_cauldron`): the crystal ball's visions, for 5 Pw (10 Pw for anyone else). Non-Witches may be overwhelmed (a Wisdom check, as with a crystal ball's Intelligence check): confusion, blindness or hallucination. A cursed cauldron always overwhelms.

Tested in wizard mode (with a temporary hook, since removed, that supplied water and ingredients): healing plus gain energy plus wolfsbane brewed a blessed extra healing; an unmixable pair left a Witch's potions alone and blasted a Priest once in three tries; a stew of a newt corpse and a carrot was eaten while a cockatrice corpse stayed in the cauldron; gazing detected coins. The three stress tests ran clean.

## Batch 140: Witch, stage 5 -- the Boline of Cerridwen

A new artifact sickle, **The Boline of Cerridwen** (`artilist.h`; one more artifact and a new `CREATE_POTION` invoke property, so a full rebuild and incompatible saves).

- **Weapon:** a flat d5 to hit and d10 damage against everything (`PHYS(5,10)`, like Excalibur's); restricted (it can't be wished for in normal play or renamed onto).
- **Spells:** wielded, it adds +50 to the success chance of healing and enchantment spells, the same bonus the school staves give (`spell.c`).
- **#invoke:** brews a random potion, never gain level, then rests for a long while (`rnz(400)` turns). A Witch or Wizard gets a blessed potion, and half the time two of it; a Priest or Healer gets one blessed potion; anyone else gets one uncursed potion.
- **Gift:** it is the Witch's first sacrifice gift. Like the Bard's Luck Blade, its alignment is set to the Witch's own at the start of the game (`hack_artifacts`), so a Witch of any alignment is offered it; for everyone else it is a neutral artifact that can turn up as an ordinary random artifact sickle or as a neutral altar's gift.

Tested in wizard mode: selecting a gift through `mk_artifact` for a lawful, neutral and chaotic Witch gives the Boline each time; `#invoke` as Witch (a blessed pair of healing once), Wizard, Priest (blessed) and Rogue (uncursed single); with the Boline wielded, a Witch's failure rates for cure blindness (healing) and slow monster (enchantment) dropped from 44% and 50% to 0%. The test hook that forced the gift lived only in the build tree and has been removed. The three stress tests ran clean.

## Batch 141: Witch, stage 6 -- the quest

The Ranger-copy placeholder is replaced by the real Witch quest. New monsters and an artifact (so a full rebuild; old saves and bones are incompatible).

- **Leader: Medea** (a flying human spellcaster, level 20, casts arcane and clerical spells). **Nemesis: Baba Yaga** (level 15, flying, two hard weapon attacks, a claw that steals the amulet, a spell attack; sleep and stoning resistant; waits for you and covets the artifact). **Guardians: coven sisters** (level 5, peaceful, melee and spells; they grow up into witches). **Enemy: witch-hunters** (level 5, hostile humans, each with a crossbow and 12 bolts, a short sword and a bullwhip, and half of them leather armor), with wolves, wargs and winter wolves.
- **Artifact: The Besom of Hecate** -- a quest broom (restricted to Witches and intelligent). Ridden as an Expert whatever your skill, so it flies and is fast; wielded it adds +50 to every school of spell; d5 to hit and d6 damage; carried it gives warning, magic resistance and half spell damage; #invoke restores Pw (the same boost as the Mitre of Holiness). Like every quest artifact it takes the Witch's own alignment.
- **Levels** (`dat/Witch.des`, adapted from the Ranger's): the home level is Medea's grove, a ring of forest besieged by witch-hunters and wolves, with her cauldron at its heart and her coven sisters about her; the locate level is the Witchwood (witch-hunters, wolves and ravens); the goal level is Baba Yaga's yard, where she waits on the throne spot with the Besom, a cauldron and the Bell of Opening; the fill levels hold hunters and wolves. The intermediate goal is named "the Witchwood" (`role.c`).
- **Text** (`dat/quest.txt`): the whole Witch dialogue is written -- the siege, Medea's assignment (the hunters, led by Baba Yaga, stole the Besom), the Witchwood, the bone-fenced hut on chicken legs, Baba Yaga's taunts and curse, and Medea's thanks and the Amulet speech.
- Wiring: `genus()` and `grownups[]` map coven sisters to Witches, `makemon.c` arms the hunters, and the Besom is a special case in `use_broom` and `percent_success`.

Tested in wizard mode with a temporary hook (since removed) that jumped to the quest levels and set the alignment record: the home-level intro, Medea refusing a hero who "strayed" (record too low), accepting a Witch of XL14 with her full assignment, the Witchwood arrival text, and the goal-level text with Baba Yaga picking up the Besom while hunters shot crossbow bolts and slashed with short swords. I did not play through the fight. The three stress tests ran clean.

## Batch 142: fledgling raven; cauldrons turn up in the dungeon

- **Fledgling raven** (new monster, so a full rebuild; old saves and bones are incompatible): a level-2, domestic bird (bite d4, blinding claw d3) that grows into the raven at level 4 and then into the giant raven. It is now the chaotic Witch's starting familiar, matching the black kitten and owlet; the raven itself is no longer a starting pet. It counts as a familiar for the bond (growth with her level, Pw loss and restore). Tested: a chaotic Witch's fledgling raven grew into a raven at experience level 4.
- **Cauldrons are common finds:** the random-tool odds went from 3 to 8 (tin whistles lost 5; the tool class still sums to 1000), and `mklev.c` now also puts a cauldron in a room now and then, as it does for boxes and chests (a room's chance is `1 / (5 x number of rooms)`, about half as often as a box).

## Batch 143: character-creation menu fits a short terminal

With the Witch the class column had 17 rows, which pushed the menu past the bottom of a short terminal (rows overwrote each other). `charinit.c` now lays the classes out in two columns (Archeologist..Pirate, Priest..Wizard), moves the race, gender and alignment columns to the right, and brings the quit/reroll/play row up to row 19, so the whole menu needs 21 rows instead of 23. Checked at 24 and 21 rows, and selecting a Witch with `W` still starts a game.

## Batch 144: the Witch's gods take an article

The Witch's pantheon is now **the Crone** (lawful), **the Mother** (neutral) and **the Maiden** (chaotic); without the article the names read wrongly in prayer messages. The names are stored as `_the Crone` and so on (the underscore still marks a goddess), so mid-sentence uses read right ("You feel that the Crone is displeased"). Where a god's name begins a sentence, a new `cap_gname()` (`pray.c`) capitalises it: the prayer-result and sacrifice messages in `pray.c` ("The Crone accepts your gift...", "... seems ...", "... is enraged..."), and the god's line in the attributes screen and dump (`cmd.c`). The one Witch quest line that starts with the god's name uses the `%dC` capitalising form. No other role's gods changed, and saves are unaffected.

Checked in wizard mode: "You begin praying to the Crone", "You feel that the Crone is displeased", and the attributes screen's deities listed as "the Maiden / the Mother / the Crone".

## Batch 145: Pw growth swaps (Witch like Priest; Bard and Knight trade)

- The **Witch** now has the Priest's Pw curve: start 4 + d3, `rn2(Wis/2+2)` per level, and the x2 multiplier in `enermod()` (`exper.c`). About 12 Pw a level at Wis 15 for a human, up from 6.
- The **Bard** and **Knight** traded energy curves: the Bard now starts with 1 + d4 and gains `rn2(Wis/2+1)` a level with the x1.5 multiplier (about 8 a level at Wis 15); the Knight starts with 6 and gains `rn2(Wis/2+2)` a level at x1 (about 6 a level). Cutoff levels were both 10 already.

No monsters or objects changed, so saves still load (existing characters keep the Pw they have; only future levels use the new curve). Checked in wizard mode: starting Pw for a human Witch 6-8, Bard 3-6, Knight 7; nine levels of gain level took Witches to about 113, Bards to about 55-70 and Knights to about 50-70.

## Batch 146: athames of the spell schools

Seven new weapons -- **athames of divination, healing, holiness, matter, escape, battle and enchantment** -- matching the school staves. Each is an ordinary athame (dagger skill, d4/d3, +2 to hit, engraves like an athame: `is_athame()` in `obj.h`, used by `engrave.c`) that, when wielded, helps casting in its one school (`spell.c`, `percent_success`). They look like a plain "athame" until identified (price tells: 300 against 4), as the staves look like plain staves.

- **Bonus:** +50 to the spell's success chance for a **Witch or a Bard** (the same as a staff), +30 for anyone else, so the staves remain the stronger casting focus for everyone but those two, who get both a good blade and a full staff's help.
- **Odds:** 2 in 1000 weapons each (14 total; arrows and darts lost 7 each to keep the class at 1000). Metal-eaters treat them as daggers.
- Objects were added, so a full rebuild; old saves and bones are incompatible.

Tested in wizard mode with a level-2 healing spell (cure blindness): failure was 62% bare, 12% for a Witch wielding an athame of healing, 32% for a Priest wielding the same, and unchanged with an athame of matter.

## Batch 147: a plain athame helps every school; the wizard staff

- **Any athame gives +10** to the success chance of spells of every school while wielded (`spell.c`, `percent_success`). The school athames keep their totals in their own school (+50 for a Witch or Bard, +30 for anyone else) and give +10 in all the others, so they are now "a plain athame, plus". A Witch's starting athame therefore helps her a little with everything.
- **Wizard staff** (new weapon, `objects.c`): a quarterstaff-like staff (d6/d6, two-handed, quarterstaff skill, price 500, 4 in 1000 weapons; arrows and darts each gave up 2 to keep the class at 1000) that gives the same +10 to every school. It looks like the other staves ("staff") until identified. A Wizard's starting quarterstaff is unchanged. It is meant for casters who use many schools (Wizards, Priests); a school staff is still much better for one school.
- `objnam.c`: wishing for a "wizard staff" no longer reads as the Wizard monster plus "staff" (and hands out a quarterstaff).
- Objects were added, so a full rebuild; old saves and bones are incompatible.

Tested in wizard mode with cure blindness (a level-2 healing spell): failure for a Wizard was 70% bare or with a quarterstaff, 60% with a plain athame or the wizard staff, 40% with an athame of healing; for a Witch, 54% bare, 44% with a plain athame or the wizard staff, 4% with an athame of healing.

## Batch 148: quarterstaff d8/d8

The quarterstaff now does d8 against small and large (was d6/d6); the wizard staff stays d6/d6, and the school staves and artifacts based on a quarterstaff are unchanged (the Staff of Aesculapius picks up the quarterstaff's new dice, since it is one). Only a damage figure changed, so saves and bones still load.

## Batch 149: the swordbreaker

A new short sword, the **swordbreaker** (`objects.c`): short-sword skill, d6/d6, weight 30, price 20, 4 in 1000 weapons (the short sword gave up 2 and darts 2 to keep the class at 1000). Like the parrying dagger and main gauche it adds 1 plus its enchantment to your armor class while wielded (in either hand when fighting with two weapons); it is part of `is_parry_wep()` (`obj.h`), so it follows the same no-stacking rule with the other parrying weapons and with bracers. Monsters rank it just below the short sword when choosing a weapon (`weapon.c`). Objects were added, so a full rebuild; old saves and bones are incompatible.

Tested in wizard mode with bare AC 5: a +3 swordbreaker gave AC 1 and a +0 one AC 4 (the same as a parrying dagger of the same enchantment); a +3 short sword changed nothing.
