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
