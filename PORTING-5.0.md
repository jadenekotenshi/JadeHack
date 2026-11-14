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
