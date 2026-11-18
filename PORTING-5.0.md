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

Mephistopheles's speed goes from 15 to 12 (`monst.c`), the same as an unhurried hero, so a hero with intrinsic speed, speed boots or Quicksilver can outrun him and fight hit-and-run. He is still covetous, so he teleports to a hero who carries the Amulet. Only the binary changes; saves and bones still load. (Batch 26's "speed 15" line is out of date.)

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
